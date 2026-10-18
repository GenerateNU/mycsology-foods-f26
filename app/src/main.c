#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "seven_seg.h"
#include "buzzer.h"

static const struct device *seven_seg_dev = DEVICE_DT_GET(DT_NODELABEL(ht16k33));
static const struct device *buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(buzzer));

int main(void)
{
    clear_seven_seg(seven_seg_dev);
    char test[] = {'b','e','l','o'};
    display_seven_seg(seven_seg_dev, test, 4);
    display_seven_seg_punctuation(seven_seg_dev, 0b00001);
    // Should display: belo.

    buzzer_play_tune(buzzer_dev, done_tune, 4);
    // Should play done tune.

    
    return 0;
}
