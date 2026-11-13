#include "ui.h"
#include "buttons.h"
#include "buzzer.h"
#include "dial.h"
#include "seven_seg.h"

static const struct gpio_dt_spec *cata_leds;

int ui_sequence(const struct device *dev1, const struct device *dev2) {
    clear_seven_seg(dev1);
    clear_seven_seg(dev2);

    gpio_pin_set_dt(&cata_leds[0], 1);
    int32_t last_pos = dial_get_position();
    int8_t current_cata = 0;
    while (1) {
        if (mcu.ui_button == 1) {
            
        }
        int32_t pos = dial_get_position();
        int32_t delta = pos - last_pos;
        delta = delta / 2;
        last_pos = pos;
        if (delta == 0) {

        } else if (current_cata + delta >= 7) {
            current_cata = current_cata + delta - 8;
        } else if (current_cata + delta < 0) {
            current_cata = current_cata + delta + 8;
        } else {
            current_cata += delta;
        }
        gpio_pin_set_dt(&cata_leds[current_cata], 1);
        k_sleep(K_MSEC(100));
    }
}
