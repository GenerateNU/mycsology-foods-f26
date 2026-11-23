/*
 * Public API for the Littelfuse 55300 flat pack rotary Hall sensor driver.
 *
 * The driver uses the standard Zephyr sensor API. This header adds the
 * driver-specific channel and a few inline helpers built on top of it.
 *
 * Channels:
 *   SENSOR_CHAN_MAGNET_PRESENT - val1 = 1 magnet detected, 0 not detected
 *   SENSOR_CHAN_ROTATION       - angle in degrees, 0..360
 *                                (channel_get returns -ENODATA if no magnet)
 *   SENSOR_CHAN_VOLTAGE        - sensor output voltage, before the divider
 *
 * Attributes:
 *   SENSOR_ATTR_OFFSET on SENSOR_CHAN_ROTATION - raw angle that reads as 0 deg
 *
 * Typical use:
 *   const struct device *knob = DEVICE_DT_GET(DT_NODELABEL(knob));
 *   int32_t mdeg;
 *
 *   if (sensor_sample_fetch(knob) == 0 &&
 *       hall55300_magnet_present(knob) &&
 *       hall55300_get_angle_mdeg(knob, &mdeg) == 0) {
 *       // mdeg = angle in milli-degrees (0..359999)
 *   }
 */

#ifndef HALL55300_H_
#define HALL55300_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Driver-specific channel: magnet presence (val1 = 1 present, 0 absent). */
#define SENSOR_CHAN_MAGNET_PRESENT ((enum sensor_channel)SENSOR_CHAN_PRIV_START)

/**
 * @brief Check whether a magnet is in front of the sensor.
 *
 * Uses the state from the last sensor_sample_fetch().
 *
 * @param dev 55300 sensor device.
 * @return true if a magnet is detected, false if not (or on error).
 */
static inline bool hall55300_magnet_present(const struct device *dev)
{
	struct sensor_value v;

	if (sensor_channel_get(dev, SENSOR_CHAN_MAGNET_PRESENT, &v) != 0) {
		return false;
	}
	return v.val1 != 0;
}

/**
 * @brief Get the angle from the last fetch in milli-degrees.
 *
 * @param dev  55300 sensor device.
 * @param mdeg Output: angle, 0..359999 milli-degrees.
 * @return 0 on success, -ENODATA if no magnet, other negative errno on error.
 */
static inline int hall55300_get_angle_mdeg(const struct device *dev, int32_t *mdeg)
{
	struct sensor_value v;
	int err = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &v);

	if (err == 0) {
		*mdeg = v.val1 * 1000 + v.val2 / 1000;
	}
	return err;
}

/**
 * @brief Get the sensor output voltage from the last fetch in millivolts.
 *
 * This is the voltage on the sensor's signal wire, with the input
 * divider already undone by the driver.
 *
 * @param dev 55300 sensor device.
 * @param mv  Output: voltage in mV.
 * @return 0 on success, negative errno on error.
 */
static inline int hall55300_get_voltage_mv(const struct device *dev, int32_t *mv)
{
	struct sensor_value v;
	int err = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &v);

	if (err == 0) {
		*mv = v.val1 * 1000 + v.val2 / 1000;
	}
	return err;
}

/**
 * @brief Set the raw angle that should read as 0 degrees.
 *
 * @param dev  55300 sensor device.
 * @param mdeg Raw angle in milli-degrees (0..359999). 0 clears the offset.
 * @return 0 on success, negative errno on error.
 */
static inline int hall55300_set_offset_mdeg(const struct device *dev, int32_t mdeg)
{
	struct sensor_value v = {
		.val1 = mdeg / 1000,
		.val2 = (mdeg % 1000) * 1000,
	};

	return sensor_attr_set(dev, SENSOR_CHAN_ROTATION, SENSOR_ATTR_OFFSET, &v);
}

/**
 * @brief Make the current shaft position read as 0 degrees.
 *
 * Clears the offset, takes a fresh reading and uses it as the new offset.
 * Call it with the magnet in place and the shaft not moving.
 *
 * @param dev 55300 sensor device.
 * @return 0 on success, -ENODATA if no magnet, other negative errno on error.
 */
static inline int hall55300_set_zero(const struct device *dev)
{
	int32_t mdeg;
	int err;

	err = hall55300_set_offset_mdeg(dev, 0);
	if (err) {
		return err;
	}

	err = sensor_sample_fetch(dev);
	if (err) {
		return err;
	}

	err = hall55300_get_angle_mdeg(dev, &mdeg);
	if (err) {
		return err;
	}

	return hall55300_set_offset_mdeg(dev, mdeg);
}

#ifdef __cplusplus
}
#endif

#endif /* HALL55300_H_ */