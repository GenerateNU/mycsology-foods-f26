#ifndef SHT30_H
#define SHT30_H

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>

/*
 * SHT30-D: I2C humidity + temperature sensor (addr 0x44).
 *
 * Decisions for this API (change them if you disagree — tell me why):
 *
 * 1. Fixed-point, not float:
 *    temp_c_x100  = temperature in hundredths of °C  (e.g. 2950 == 29.50°C)
 *    rh_x100      = humidity in hundredths of %RH    (e.g. 7250 == 72.50%RH)
 *    Why: Zephyr/MCU code usually avoids float until you need it for display/math.
 *
 * 2. enum for repeatability:
 *    Maps 1:1 to the datasheet's High / Medium / Low measurement commands.
 *
 * 3. Pass i2c_dt_spec*:
 *    Each physical sensor (top tray / bottom tray) is a different I2C node in
 *    the DeviceTree. The caller picks which one by which spec they pass in —
 *    same idea as PR #2 passing a different `const struct device *` for UI.
 */

enum sht30_repeatability {
	SHT30_REPEAT_HIGH = 0,   /* ~13.5 ms max */
	SHT30_REPEAT_MEDIUM,     /* ~5 ms max */
	SHT30_REPEAT_LOW,        /* ~3 ms max */
};

struct sht30_sample {
	int16_t temp_c_x100;
	uint16_t rh_x100;
};
int init_sht30();
/**
 * Take one measurement from the SHT30.
 *
 * @param i2c   DeviceTree I2C spec for this sensor (address + bus)
 * @param rep   Measurement repeatability
 * @param out   Filled on success
 *
 * @return 0 on success, negative errno on failure
 *         (e.g. -ENODEV bus not ready, -EIO CRC / NACK)
 */
int sht30_read(const struct i2c_dt_spec *i2c,
	       enum sht30_repeatability rep,
	       struct sht30_sample *out);

#endif /* SHT30_H */

