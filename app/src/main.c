#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#include "seven_seg.h"
#include "buzzer.h"
#include "buttons.h"
#include "dial.h"
#include "fan.h"

LOG_MODULE_REGISTER(main_thread);

static const struct device *seven_seg_dev = DEVICE_DT_GET(DT_NODELABEL(ht16k33));
static const struct device *buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(buzzer));
static const struct pwm_dt_spec fan = PWM_DT_SPEC_GET(DT_ALIAS(fan_pwm));

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
