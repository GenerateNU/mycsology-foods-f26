#if 0 /* UI main — restore when sensor bring-up is done */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "seven_seg.h"
#include "buzzer.h"
#include "buttons.h"
#include "dial.h"
#include "fan.h"
#include "heater.h"

LOG_MODULE_REGISTER(main_thread);

static const struct device *seven_seg_dev = DEVICE_DT_GET(DT_NODELABEL(ht16k33));
static const struct device *buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(buzzer));
static const struct pwm_dt_spec fan = PWM_DT_SPEC_GET(DT_ALIAS(fan_pwm));
static const struct pwm_dt_spec heater = PWM_DT_SPEC_GET(DT_ALIAS(heater_pwm));

int main(void) {
    k_sleep(K_MSEC(3000));

    clear_seven_seg(seven_seg_dev);
    char test[] = {'b','e','l','o'};
    display_seven_seg(seven_seg_dev, test, 4);
    display_seven_seg_punctuation(seven_seg_dev, 0b00001);
    // Should display: belo.

    buzzer_play_tune(buzzer_dev, done_tune, 4);
    // Should play done tune.

    init_all_buttons();
    
    int32_t fan_speed = 0;

    if (init_dial() != 0) {
        return -1;
    }

    init_fan(&fan);

    int32_t last_pos = dial_get_position();

    init_heater(&heater);

    while (1) {
        int32_t pos = dial_get_position();
        int32_t delta = pos - last_pos;
        delta = delta / 2;
        last_pos = pos;
        LOG_INF("delta = %d", delta);

        fan_speed = CLAMP(fan_speed + delta, 0, 100);
        LOG_INF("fan speed = %d", fan_speed);
        set_fan_percent(&fan, fan_speed);
        k_sleep(K_MSEC(50));
    }
    
    return 0;
}
#endif /* UI main */

#if 1
/*
 * Sensor smoke tests — enable exactly one of SHT30 / DS18B20 below.
 * DS18B20: DS2482-800 @ I2C 0x1a, channel 0 (DT_NODELABEL(w1)).
 */

#if 0 /* SHT30 history handoff */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "sensors/sht30.h"
#include "sensors/sht30_history.h"

LOG_MODULE_REGISTER(main);

static const struct i2c_dt_spec sht30_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(sht30));

int main(void)
{
	struct sht30_sample sample;
	struct sht30_reading reading;

	LOG_INF("SHT30 handoff smoke test starting");
	sht30_history_init();
	sht30_history_reset();

	while (1) {
		int err = sht30_read(&sht30_i2c, SHT30_REPEAT_HIGH, &sample);

		if (err == 0) {
			sht30_history_push(&sample);

			if (sht30_history_get_latest(&reading) == 0) {
				int t = reading.newest.temp_c_x100;
				unsigned t_frac = (unsigned)((t < 0) ? -t : t) % 100;
				int at = reading.avg_temp_c_x100;
				unsigned at_frac = (unsigned)((at < 0) ? -at : at) % 100;

				LOG_INF("T=%d.%02u C  RH=%u.%02u %%  hist=%u seq=%u",
					t / 100, t_frac,
					reading.newest.rh_x100 / 100,
					reading.newest.rh_x100 % 100,
					(unsigned)sht30_history_count(),
					reading.seq);
				LOG_INF("AVG T=%d.%02u C  AVG RH=%u.%02u %%",
					at / 100, at_frac,
					reading.avg_rh_x100 / 100,
					reading.avg_rh_x100 % 100);
			}
		} else {
			LOG_ERR("sht30_read failed: %d", err);
		}

		k_msleep(1000);
	}

	return 0;
}
#endif /* SHT30 */

#if 1 /* DS18B20 — via DS2482-800 @ 0x1a, ch0 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensors/ds18b20.h"

LOG_MODULE_REGISTER(main);

static const struct device *w1 = DEVICE_DT_GET(DT_NODELABEL(w1));

int main(void)
{
	struct ds18b20_sample sample;

	LOG_INF("DS18B20 smoke test starting (DS2482-800 @ 0x1a ch0)");

	while (1) {
		int err = ds18b20_read(w1, DS18B20_RES_12BIT, &sample);

		if (err == 0) {
			int t = sample.temp_c_x100;
			unsigned t_frac = (unsigned)((t < 0) ? -t : t) % 100;

			LOG_INF("T=%d.%02u C", t / 100, t_frac);
		} else {
			LOG_ERR("ds18b20_read failed: %d", err);
		}

		k_msleep(2000);
	}

	return 0;
}
#endif /* DS18B20 */

#endif
