#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>


#include "seven_seg.h"
#include "buzzer.h"
#include "buttons.h"
#include "sht30.h"

LOG_MODULE_REGISTER(main);

static const struct device *seven_seg_dev = DEVICE_DT_GET(DT_NODELABEL(ht16k33));
static const struct device *buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(buzzer));
static const struct i2c_dt_spec sht30_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(sht30));

int main(void)
{
    clear_seven_seg(seven_seg_dev);
    char test[] = {'b','e','l','o'};
    display_seven_seg(seven_seg_dev, test, 4);
    display_seven_seg_punctuation(seven_seg_dev, 0b00001);
    // Should display: belo.

    buzzer_play_tune(buzzer_dev, done_tune, 4);
    // Should play done tune.

    init_all_buttons();
    init_sht30();

    
    
    while (1) {
            int err = sht30_read(&sht30, SHT30_REPEAT_HIGH, &sample);
        if (err == 0) {
            LOG_INF("T=%d.%02d C  RH=%d.%02d %%", sample.temp_c_x100 / 100,
                abs(sample.temp_c_x100 % 100),
                sample.rh_x100 / 100,
                sample.rh_x100 % 100);
        }
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}
