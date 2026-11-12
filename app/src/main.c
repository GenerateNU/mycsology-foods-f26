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

#if 0


/*
 * Reference only — SHT30 history smoke test (not built).
 * Swap this in locally when flashing sensor-only; do not enable on main.
 */
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

	LOG_INF("SHT30 smoke test starting");
	if (sht30_init(&sht30_i2c) != 0) {
		LOG_ERR("sht30_init failed");
		return -1;
	}

	sht30_history_reset();

	while (1) {
		int err = sht30_read(&sht30_i2c, SHT30_REPEAT_HIGH, &sample);

		if (err == 0) {
			sht30_history_push(&sample);
			int16_t avgt = sht30_history_avg_temp();
			uint16_t avgh = sht30_history_avg_hum();

			int t = sample.temp_c_x100;
			unsigned t_frac = (unsigned)((t < 0) ? -t : t) % 100;
			unsigned avgt_frac = (unsigned)((avgt < 0) ? -avgt : avgt) % 100;

			LOG_INF("T=%d.%02u C  RH=%u.%02u %%  (history=%u)",
				t / 100, t_frac,
				sample.rh_x100 / 100,
				sample.rh_x100 % 100,
				(unsigned)sht30_history_count());

			LOG_INF("AVG T=%d.%02u C  AVG RH=%u.%02u %%",
				avgt / 100, avgt_frac,
				avgh / 100, avgh % 100);
		} else {
			LOG_ERR("sht30_read failed: %d", err);
		}

		k_msleep(1000);
	}

	return 0;
}
#endif
