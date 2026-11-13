/*
 * DS18B20 app driver — protocol from Zephyr's maxim/ds18b20, API like sht30.
 *
 * Bus I/O is Zephyr w1 (DS2482-800 channel here); we only issue commands.
 */

#include "sensors/ds18b20.h"

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(ds18b20, LOG_LEVEL_INF);

#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

/* Same waits as zephyr/drivers/sensor/maxim/ds18b20/ds18b20.c */
static const uint16_t measure_wait_ms[] = { 94, 188, 376, 750 };

struct ds18b20_scratchpad {
	int16_t temp;
	uint8_t alarm_temp_high;
	uint8_t alarm_temp_low;
	uint8_t config;
	uint8_t res[3];
	uint8_t crc;
} __packed;

static bool slave_responded(const uint8_t *buf, size_t len)
{
	uint8_t cmp = 0xff;

	for (size_t i = 0; i < len; i++) {
		cmp &= buf[i];
	}
	return cmp != 0xff;
}

static int convert_t(const struct device *w1)
{
	int ret;

	/*
	 * Keep reset → Skip ROM → Convert T under one bus lock.
	 * Do not call w1_reset_select() here — it takes the same mutex
	 * and would deadlock (Zephyr k_mutex is not recursive).
	 */
	(void)w1_lock_bus(w1);

	ret = w1_reset_bus(w1);
	if (ret == 0) {
		ret = -ENODEV;
		goto out;
	}
	if (ret < 0) {
		goto out;
	}

	ret = w1_write_byte(w1, W1_CMD_SKIP_ROM);
	if (ret < 0) {
		goto out;
	}

	ret = w1_write_byte(w1, DS18B20_CMD_CONVERT_T);

out:
	(void)w1_unlock_bus(w1);
	return ret;
}

static int read_scratchpad(const struct device *w1,
			   const struct w1_slave_config *cfg,
			   struct ds18b20_scratchpad *sp)
{
	uint8_t cmd = DS18B20_CMD_READ_SCRATCHPAD;
	uint8_t crc;
	int ret;

	memset(sp, 0, sizeof(*sp));
	ret = w1_write_read(w1, cfg, &cmd, 1, (uint8_t *)sp, sizeof(*sp));
	if (ret != 0) {
		return ret;
	}

	if (!slave_responded((uint8_t *)sp, sizeof(*sp))) {
		LOG_WRN("Slave not reachable");
		return -ENODEV;
	}

	crc = w1_crc8((uint8_t *)sp, sizeof(*sp) - 1);
	if (crc != sp->crc) {
		LOG_WRN("CRC mismatch (got 0x%02x want 0x%02x)", crc, sp->crc);
		return -EIO;
	}

	return 0;
}

int ds18b20_read(const struct device *w1,
		 enum ds18b20_resolution res,
		 struct ds18b20_sample *out)
{
	struct w1_slave_config slave = { 0 };
	struct ds18b20_scratchpad sp;
	int16_t raw;
	int ret;

	if (w1 == NULL || out == NULL) {
		return -EINVAL;
	}
	if (res > DS18B20_RES_12BIT) {
		return -EINVAL;
	}
	if (!device_is_ready(w1)) {
		LOG_ERR("W1 bus not ready");
		return -ENODEV;
	}

	ret = convert_t(w1);
	if (ret < 0) {
		LOG_ERR("Convert T failed: %d", ret);
		return ret;
	}

	k_msleep(measure_wait_ms[res]);

	ret = read_scratchpad(w1, &slave, &sp);
	if (ret < 0) {
		LOG_ERR("Read scratchpad failed: %d", ret);
		return ret;
	}

	raw = sys_get_le16((uint8_t *)&sp.temp);
	/* LSB = 1/16 °C → hundredths: (raw * 100) / 16 */
	out->temp_c_x100 = (int16_t)((int32_t)raw * 100 / 16);

	return 0;
}
