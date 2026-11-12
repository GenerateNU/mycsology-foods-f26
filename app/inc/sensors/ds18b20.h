#ifndef DS18B20_H
#define DS18B20_H

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

/*
 * DS18B20: 1-Wire temperature-only sensor (Maxim / Analog Devices).
 *
 * Same API *shape* as sht30.h on purpose — so the rest of the app learns one
 * pattern: "pass a bus/pin handle, get a sample struct back, check errno."
 *
 * Decisions for this API (change them if you disagree — tell me why):
 *
 * 1. Fixed-point, not float:
 *    temp_c_x100 = temperature in hundredths of °C (e.g. 2950 == 29.50°C)
 *    Why: same reason as SHT30 — keep MCU math integer until display/PID needs float.
 *
 * 2. enum for resolution:
 *    Maps 1:1 to the datasheet's 9 / 10 / 11 / 12-bit Convert-T wait times.
 *    Higher resolution = slower conversion (up to 750 ms at 12-bit).
 *
 * 3. Pass gpio_dt_spec* for the DQ (data) pin:
 *    1-Wire is one GPIO wire, not I2C. DeviceTree owns "which pin";
 *    the caller picks which sensor wire by which spec they pass in —
 *    same idea as SHT30 passing a different i2c_dt_spec per tray.
 *
 * 4. Temp only — no humidity field.
 *    Why: the chip only measures temperature. Don't fake an RH member.
 */

enum ds18b20_resolution {
	DS18B20_RES_9BIT = 0,  /* ~93.75 ms max */
	DS18B20_RES_10BIT,     /* ~187.5 ms max */
	DS18B20_RES_11BIT,     /* ~375 ms max */
	DS18B20_RES_12BIT,     /* ~750 ms max (power-on default) */
};

struct ds18b20_sample {
	int16_t temp_c_x100;
};

/**
 * Take one temperature measurement from a DS18B20.
 *
 * Mental model this function will eventually run (two bus conversations):
 *   1) reset → Skip ROM → Convert T → wait
 *   2) reset → Skip ROM → Read Scratchpad → CRC → fill out
 *
 * @param dq    DeviceTree GPIO spec for the 1-Wire DQ pin
 * @param res   Conversion resolution (sets wait time)
 * @param out   Filled on success
 *
 * @return 0 on success, negative errno on failure
 *         (e.g. -ENODEV pin/bus not ready, -EIO no presence / CRC fail)
 */
int ds18b20_read(const struct gpio_dt_spec *dq,
		 enum ds18b20_resolution res,
		 struct ds18b20_sample *out);

#endif /* DS18B20_H */
