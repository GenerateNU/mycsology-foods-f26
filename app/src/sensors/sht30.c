/*
 * SHT30-D driver (application-level wrapper over Zephyr I2C).
 *
 * Flow: bus ready → pick measure cmd from repeatability → write → wait →
 * read 6 bytes → CRC check → convert to temp_c_x100 / rh_x100.
 */

#include "sensors/sht30.h"
#include <stddef.h> /*for size_t */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Register this .c file with Zephyr's logging system.
 * LOG_MODULE_REGISTER goes in exactly ONE .c file (never in a header).
 * After this, LOG_ERR / LOG_DBG / etc. are tagged as "sht30" in the console.
 */
LOG_MODULE_REGISTER(sht30, LOG_LEVEL_INF);
static uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
	uint8_t crc = 0xFF;
	for (size_t i = 0; i < len; i++){
		crc ^= data[i];
		for (int j = 0; j < 8; j++){
			if (crc & 0x80){
				crc <<= 1;
				crc ^= 0x31;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

int init_sht30(){

/* Step 0: basic sanity checks on the arguments the caller passed in.
	 * If someone calls us with a NULL pointer, fail early with a clear
	 * error instead of crashing later.
	 */
	 if (i2c == NULL || out == NULL) {
		return -EINVAL; /* "invalid argument" — standard C errno */
	}
	/*
	* Check if the I2C bus is ready, this is so that we cn see if i2c 
	* communication is usable before trying to talk to sensor
	*/
	if (!i2c_is_ready_dt(i2c)){
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	return 0;
}

int sht30_read(const struct i2c_dt_spec *i2c,
	       enum sht30_repeatability rep,
	       struct sht30_sample *out)
{
	

	/*
	* this selects the actual repeatability option using the 2 bits
	*/
	uint8_t cmd[2];
	switch (rep){
	case SHT30_REPEAT_HIGH:
		cmd[0] = 0x24;
		cmd[1] = 0x00;
		break;
	case SHT30_REPEAT_MEDIUM:
		cmd[0] = 0x24;
		cmd[1] = 0x0B;
		break;
	case SHT30_REPEAT_LOW:
		cmd[0] = 0x24;
		cmd[1] = 0x16;
		break;
	default:
		LOG_ERR("Invalid repeatability");
		return -EINVAL;


    }

	uint32_t wait_ms;
	switch (rep){
	case SHT30_REPEAT_HIGH:
		wait_ms = 14;
		break;
	case SHT30_REPEAT_MEDIUM:
		wait_ms = 5;
		break;
	case SHT30_REPEAT_LOW:
		wait_ms = 3;
		break;
	default:
		LOG_ERR("Invalid repeatability");
		return -EINVAL;

	}

	int i2c_write_error = i2c_write_dt(i2c, cmd, sizeof(cmd));
	if (i2c_write_error <0){
		LOG_ERR("Failed to write command to sensor: %d", i2c_write_error);
		return i2c_write_error;
	}

	k_msleep(wait_ms);

	uint8_t data[6];
	int i2c_read_error = i2c_read_dt(i2c, data, sizeof(data));
	if (i2c_read_error <0){
		LOG_ERR("Failed to read data from sensor: %d", i2c_read_error);
		return i2c_read_error;
	}
	if (sht30_crc8(&data[0], 2) != data[2] ||
    sht30_crc8(&data[3], 2) != data[5]) {
	LOG_ERR("CRC mismatch");
	return -EIO;
	}
	uint16_t raw_temp = ((uint16_t)data[0] << 8) | data[1];
	uint16_t raw_rh   = ((uint16_t)data[3] << 8) | data[4];

	/* Convert raw counts to real units in hundredths, then store in out.
	 * Do NOT assign raw into out first — the caller wants °C and %RH.
	 */
	out->temp_c_x100 =
		(int16_t)(-4500 + (17500 * (int32_t)raw_temp) / 65535);
	out->rh_x100 =
		(uint16_t)((10000 * (uint32_t)raw_rh) / 65535);

	return 0;


	
}
