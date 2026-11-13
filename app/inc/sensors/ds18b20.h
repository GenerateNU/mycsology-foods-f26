#ifndef DS18B20_H
#define DS18B20_H

#include <zephyr/device.h>
#include <stdint.h>

/*
 * DS18B20: 1-Wire temperature-only sensor (Maxim / Analog Devices).
 *
 * Hybrid approach:
 *   - Zephyr's w1 API owns bit timing / reset / byte I/O
 *     (here: DS2482-800 channel via maxim,ds2482-800 @ I2C 0x1a)
 *   - This file owns the DS18B20 transaction + our app API (like sht30.h)
 *
 * Protocol (mirrors in-tree drivers/sensor/maxim/ds18b20/ds18b20.c):
 *   1) reset → Skip ROM → Convert T (0x44) → wait
 *   2) reset → Skip ROM → Read Scratchpad (0xBE) → CRC → temp
 *
 * Assumptions for v1:
 *   - External VDD (not parasite power)
 *   - One sensor on the bus (Skip ROM)
 */

enum ds18b20_resolution {
	DS18B20_RES_9BIT = 0,  /* ~94 ms */
	DS18B20_RES_10BIT,     /* ~188 ms */
	DS18B20_RES_11BIT,     /* ~376 ms */
	DS18B20_RES_12BIT,     /* ~750 ms (chip power-on default) */
};

struct ds18b20_sample {
	int16_t temp_c_x100; /* hundredths of °C, e.g. 2950 == 29.50°C */
};

/**
 * Take one temperature measurement from a DS18B20 on @p w1.
 *
 * @param w1   1-Wire master device (e.g. DEVICE_DT_GET(DT_NODELABEL(w1)))
 * @param res  Conversion resolution (sets wait after Convert T)
 * @param out  Filled on success
 *
 * @return 0 on success, negative errno on failure
 *         (-ENODEV no presence / bus not ready, -EIO CRC, etc.)
 */
int ds18b20_read(const struct device *w1,
		 enum ds18b20_resolution res,
		 struct ds18b20_sample *out);

#endif /* DS18B20_H */
