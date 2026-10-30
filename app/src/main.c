#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#include "seven_seg.h"
#include "buzzer.h"
#include "buttons.h"
#include "dial.h"
#include "fan.h"

static const struct device *seven_seg_dev = DEVICE_DT_GET(DT_NODELABEL(ht16k33));
static const struct device *buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(buzzer));
static const struct pwm_dt_spec fan = PWM_DT_SPEC_GET(DT_ALIAS(fan_pwm));

int main(void) {
    clear_seven_seg(seven_seg_dev);
    char test[] = {'b','e','l','o'};
    display_seven_seg(seven_seg_dev, test, 4);
    display_seven_seg_punctuation(seven_seg_dev, 0b00001);
    // Should display: belo.

    buzzer_play_tune(buzzer_dev, done_tune, 4);
    // Should play done tune.

    init_all_buttons();
    
    uint8_t fan_speed = 0;

    const int *dial_value_diff = init_dial();

    init_fan(&fan);

    while (1) {
        if (*dial_value_diff > 0 && fan_speed < 100) {
            fan_speed++;
        } else if (*dial_value_diff < 0 && fan_speed > 0) {
            fan_speed--;
        }
        set_fan_percent(&fan, fan_speed);
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}
