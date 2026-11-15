#include "ui.h"
#include "buttons.h"
#include "buzzer.h"
#include "dial.h"
#include "seven_seg.h"
#include <stdio.h>

static const struct gpio_dt_spec *cata_leds;

typedef enum {
    SUBSTRATE_LEVEL,
    TIME,
    HUMIDITY,
    SUBSTRATE_TEMP,
    CHAMBER_TEMP,
    FAN_SPEED,
    AUTO
} ui_state_t;

int ui_sequence(const struct device *current, const struct device *setpoint) {
    clear_seven_seg(current);
    clear_seven_seg(setpoint);

    gpio_pin_set_dt(&cata_leds[0], 1);
    int32_t last_pos = dial_get_position();
    ui_state_t current_cata = 0;

    while (1) {
        if (button_pressed(BTN_UI) && 
        (current_cata == SUBSTRATE_LEVEL || 
            current_cata == TIME || 
            current_cata == HUMIDITY || 
            current_cata == SUBSTRATE_TEMP || 
            current_cata == FAN_SPEED)) 
            {
            edit_cata(current_cata, last_pos);
        }

        int32_t delta = dial_delta(&last_pos);
        
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
            display_seven_seg(current, curr_str);
        }
        if (mcu.set_points[current_cata] != -1) {
            char set_str[5];
            snprintf(set_str, sizeof(set_str), "%.1f", mcu.set_points[current_cata]);
            display_seven_seg(setpoint, set_str);
        }

        k_sleep(K_MSEC(100));
    }
}

int edit_cata(ui_state_t cata, int32_t last_pos) {
    float current_value = mcu.set_points[cata];
    while(1) {
        if (button_pressed(BTN_UI)) {
            mcu.set_points[cata] = current_value;
            return 0;
        }
        int32_t delta = dial_delta(&last_pos);
        current_value += 0.1 * delta;

        char set_str[5];
        snprintf(set_str, sizeof(set_str), "%.1f", mcu.set_points[cata]);
        display_seven_seg(setpoint, )
        k_sleep(K_MSEC(50));
    }
}

int32_t dial_delta(int32_t *last_pos) {
    int32_t pos = dial_get_position();
    int32_t delta = pos - *last_pos;
    delta = delta / 2;
    *last_pos = pos;
    return delta;
}
