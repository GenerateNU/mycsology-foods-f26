/*
 * Zephyr sensor driver for the Littelfuse 55300 rotary Hall sensor
 * (analog output), with magnet presence detection.
 *
 * Channels:
 *   SENSOR_CHAN_MAGNET_PRESENT - val1 = 1 magnet detected, 0 not detected
 *   SENSOR_CHAN_ROTATION       - angle in degrees (0..360); -ENODATA if no magnet
 *   Both return -EAGAIN for the first (debounce + 1) fetches after power-up,
 *   while presence is still being decided.
 *   SENSOR_CHAN_VOLTAGE        - sensor output voltage (before the divider)
 * Attribute:
 *   SENSOR_ATTR_OFFSET on SENSOR_CHAN_ROTATION - raw angle that reads as 0
 *
 * The 55300 has no field-strength output, so presence is inferred:
 *   1. Output outside the valid voltage band -> diagnostic level / no magnet
 *   2. Output jumping around randomly (high jitter) -> no usable field
 * Both are debounced, and the jitter check has hysteresis.
 */
#define DT_DRV_COMPAT littelfuse_55300

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <errno.h>
#include "hall_effect.h"

LOG_MODULE_REGISTER(hall55300, CONFIG_SENSOR_LOG_LEVEL);

#define FULL_TURN_MDEG 360000
#define HALF_TURN_MDEG 180000

struct hall55300_config {
	struct adc_dt_spec adc;
	int32_t out_min_mv;
	int32_t out_max_mv;
	int32_t band_margin_mv;
	int32_t jitter_thresh_mdeg;
	uint32_t full_ohms;
	uint32_t output_ohms;
	uint8_t debounce;
	uint8_t samples;
	uint8_t filter_shift;
};

struct hall55300_data {
	int32_t sensor_mv;
	int32_t angle_mdeg;   /* filtered, offset applied  */
	int32_t filt_mdeg;    /* filtered, no offset       */
	int32_t offset_mdeg;
	int32_t prev_raw_mdeg;
	int32_t jitter_mdeg;  /* EMA of |step| between fetches */
	uint8_t debounce_cnt;
	uint8_t fetch_count;  /* counts up to the settle point, then stops */
	bool prev_valid;
	bool filt_valid;
	bool present;
};

static int32_t wrap_mdeg(int32_t a)
{
	a %= FULL_TURN_MDEG;
	return (a < 0) ? a + FULL_TURN_MDEG : a;
}

/* Shortest signed distance from a to b around the circle */
static int32_t circ_diff(int32_t b, int32_t a)
{
	int32_t d = b - a;

	if (d > HALF_TURN_MDEG) {
		d -= FULL_TURN_MDEG;
	} else if (d < -HALF_TURN_MDEG) {
		d += FULL_TURN_MDEG;
	}
	return d;
}

static int read_sensor_mv(const struct device *dev, int32_t *out_mv)
{
	const struct hall55300_config *cfg = dev->config;
	int16_t buf;
	struct adc_sequence seq = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};
	int32_t acc = 0;
	int32_t mv;
	int err;

	err = adc_sequence_init_dt(&cfg->adc, &seq);
	if (err) {
		return err;
	}

	for (int i = 0; i < cfg->samples; i++) {
		err = adc_read_dt(&cfg->adc, &seq);
		if (err) {
			LOG_ERR("ADC read failed (%d)", err);
			return err;
		}
		acc += buf;
	}

	mv = acc / cfg->samples;
	err = adc_raw_to_millivolts_dt(&cfg->adc, &mv);
	if (err) {
		return err;
	}

	/* Undo the input divider */
	*out_mv = (int32_t)(((int64_t)mv * cfg->full_ohms) / cfg->output_ohms);
	return 0;
}

static void update_presence(const struct device *dev, bool candidate)
{
	const struct hall55300_config *cfg = dev->config;
	struct hall55300_data *data = dev->data;

	if (candidate == data->present) {
		data->debounce_cnt = 0;
		return;
	}

	if (++data->debounce_cnt < cfg->debounce) {
		return;
	}

	data->present = candidate;
	data->debounce_cnt = 0;

	if (candidate) {
		data->filt_valid = false;  /* restart the angle filter cleanly */
		LOG_INF("magnet detected");
	} else {
		LOG_INF("magnet lost (%d mV, jitter %d mdeg)",
			data->sensor_mv, data->jitter_mdeg);
	}
}

static int hall55300_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct hall55300_config *cfg = dev->config;
	struct hall55300_data *data = dev->data;
	int32_t mv;
	int err;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION &&
	    chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_MAGNET_PRESENT) {
		return -ENOTSUP;
	}

	err = read_sensor_mv(dev, &mv);
	if (err) {
		return err;
	}
	data->sensor_mv = mv;

	/* Presence needs debounce + 1 fetches (jitter needs a previous sample) */
	if (data->fetch_count <= cfg->debounce) {
		data->fetch_count++;
	}

	/* Check 1: output outside the valid band */
	bool in_band = mv >= cfg->out_min_mv - cfg->band_margin_mv &&
		       mv <= cfg->out_max_mv + cfg->band_margin_mv;

	if (!in_band) {
		data->prev_valid = false;
		update_presence(dev, false);
		return 0;
	}

	int32_t span = cfg->out_max_mv - cfg->out_min_mv;
	int32_t raw = (int32_t)(((int64_t)(mv - cfg->out_min_mv) * FULL_TURN_MDEG) / span);

	raw = CLAMP(raw, 0, FULL_TURN_MDEG - 1);

	/* Check 2: jitter between consecutive fetches (EMA, alpha = 1/4) */
	if (data->prev_valid) {
		int32_t step = abs(circ_diff(raw, data->prev_raw_mdeg));

		data->jitter_mdeg += (step - data->jitter_mdeg) / 4;
	}
	data->prev_raw_mdeg = raw;
	data->prev_valid = true;

	bool candidate = true;

	if (cfg->jitter_thresh_mdeg > 0) {
		/* Hysteresis: lose at threshold, regain below half of it */
		int32_t limit = data->present ? cfg->jitter_thresh_mdeg
					      : cfg->jitter_thresh_mdeg / 2;
		candidate = data->jitter_mdeg < limit;
	}
	update_presence(dev, candidate);

	if (!data->present) {
		return 0;
	}

	/* Circular low-pass filter on the angle */
	if (!data->filt_valid || cfg->filter_shift == 0) {
		data->filt_mdeg = raw;
		data->filt_valid = true;
	} else {
		int32_t d = circ_diff(raw, data->filt_mdeg);

		data->filt_mdeg = wrap_mdeg(data->filt_mdeg + d / (1 << cfg->filter_shift));
	}

	data->angle_mdeg = wrap_mdeg(data->filt_mdeg - data->offset_mdeg);
	return 0;
}

static int hall55300_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct hall55300_data *data = dev->data;

	const struct hall55300_config *cfg = dev->config;
	bool settled = data->fetch_count > cfg->debounce;

	switch ((int)chan) {
	case SENSOR_CHAN_MAGNET_PRESENT:
		if (!settled) {
			return -EAGAIN;  /* still deciding after power-up */
		}
		val->val1 = data->present ? 1 : 0;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_ROTATION:
		if (!settled) {
			return -EAGAIN;
		}
		if (!data->present) {
			return -ENODATA;
		}
		val->val1 = data->angle_mdeg / 1000;
		val->val2 = (data->angle_mdeg % 1000) * 1000;
		return 0;
	case SENSOR_CHAN_VOLTAGE:
		val->val1 = data->sensor_mv / 1000;
		val->val2 = (data->sensor_mv % 1000) * 1000;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int hall55300_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	struct hall55300_data *data = dev->data;

	if (chan != SENSOR_CHAN_ROTATION || attr != SENSOR_ATTR_OFFSET) {
		return -ENOTSUP;
	}

	/* Offset = raw angle (degrees) that should read as 0 */
	data->offset_mdeg = wrap_mdeg(val->val1 * 1000 + val->val2 / 1000);
	data->angle_mdeg = wrap_mdeg(data->filt_mdeg - data->offset_mdeg);
	return 0;
}

static const struct sensor_driver_api hall55300_api = {
	.sample_fetch = hall55300_sample_fetch,
	.channel_get = hall55300_channel_get,
	.attr_set = hall55300_attr_set,
};

static int hall55300_init(const struct device *dev)
{
	const struct hall55300_config *cfg = dev->config;
	int err;

	if (cfg->out_max_mv <= cfg->out_min_mv || cfg->output_ohms == 0 ||
	    cfg->samples == 0 || cfg->debounce == 0) {
		LOG_ERR("invalid devicetree configuration");
		return -EINVAL;
	}

	if (!adc_is_ready_dt(&cfg->adc)) {
		LOG_ERR("ADC %s not ready", cfg->adc.dev->name);
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&cfg->adc);
	if (err) {
		LOG_ERR("ADC channel setup failed (%d)", err);
	}
	return err;
}

#define HALL55300_DEFINE(inst)                                                         \
	static struct hall55300_data hall55300_data_##inst;                              \
	static const struct hall55300_config hall55300_cfg_##inst = {                    \
		.adc = ADC_DT_SPEC_INST_GET(inst),                                       \
		.out_min_mv = DT_INST_PROP(inst, output_min_mv),                         \
		.out_max_mv = DT_INST_PROP(inst, output_max_mv),                         \
		.band_margin_mv = DT_INST_PROP(inst, band_margin_mv),                    \
		.jitter_thresh_mdeg = DT_INST_PROP(inst, jitter_threshold_mdeg),         \
		.full_ohms = DT_INST_PROP(inst, full_ohms),                              \
		.output_ohms = DT_INST_PROP(inst, output_ohms),                          \
		.debounce = DT_INST_PROP(inst, debounce),                                \
		.samples = DT_INST_PROP(inst, samples),                                  \
		.filter_shift = DT_INST_PROP(inst, filter_shift),                        \
	};                                                                               \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hall55300_init, NULL,                         \
				     &hall55300_data_##inst, &hall55300_cfg_##inst,      \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,           \
				     &hall55300_api);

DT_INST_FOREACH_STATUS_OKAY(HALL55300_DEFINE)