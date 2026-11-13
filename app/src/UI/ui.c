#include "ui.h"
#include "buttons.h"
#include "buzzer.h"
#include "dial.h"
#include "seven_seg.h"
#include <stdio.h>

static const struct gpio_dt_spec *cata_leds;

int ui_sequence(const struct device *current, const struct device *setpoint) {
    clear_seven_seg(current);
    clear_seven_seg(setpoint);

    gpio_pin_set_dt(&cata_leds[0], 1);
    int32_t last_pos = dial_get_position();
    uint8_t last_ui_btn = mcu.ui_button;
    int8_t current_cata = 0;

    while (1) {
        if (mcu.ui_button != last_ui_btn) {
            
        }
        int32_t pos = dial_get_position();
        int32_t delta = pos - last_pos;
        delta = delta / 2;
        last_pos = pos;
        if (delta == 0) {

        } else if (current_cata + delta >= 7) {
            gpio_pin_set_dt(&cata_leds[current_cata], 0);
            current_cata = current_cata + delta - 8;
        } else if (current_cata + delta < 0) {
            gpio_pin_set_dt(&cata_leds[current_cata], 0);
            current_cata = current_cata + delta + 8;
        } else {
            gpio_pin_set_dt(&cata_leds[current_cata], 0);
            current_cata += delta;
        }
        gpio_pin_set_dt(&cata_leds[current_cata], 1);

        if (mcu.current_values[current_cata] != -1) {
            char curr_str[5];
            snprintf(curr_str, sizeof(curr_str), "%.1f", mcu.current_values[current_cata]);
            display_seven_seg(current, curr_str, 4);
        }
        if (mcu.set_points[current_cata] != -1) {
            char set_str[5];
            snprintf(set_str, sizeof(set_str), "%.1f", mcu.set_points[current_cata]);
            display_seven_seg(setpoint, set_str, 4);
        }

        k_sleep(K_MSEC(100));
    }
}

int edit_cata(int8_t cata) {

}
