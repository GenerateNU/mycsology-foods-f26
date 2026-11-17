#include "ui.h"
#include <zephyr/logging/log.h>
#include <math.h>
#include "buzzer.h" 

LOG_MODULE_REGISTER(ui);

static const char *const cata_names[NUM_UI_STATES] = {
    "SUBSTRATE_LEVEL", "TIME", "HUMIDITY", "SUBSTRATE_TEMP", "CHAMBER_TEMP", "FAN_SPEED", "AUTO",
};

// Category LEDs, one per ui_state_t. Missing aliases (cata-led0..cata-led6 in app.overlay) are skipped.
static const struct gpio_dt_spec cata_leds[NUM_UI_STATES] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led2), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led3), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led4), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led5), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(cata_led6), gpios, {0}),
};
const struct device *current;
extern const struct device *setpoint;

static void init_cata_leds(void) {
    for (int i = 0; i < NUM_UI_STATES; i++) {
        if (cata_leds[i].port != NULL && gpio_is_ready_dt(&cata_leds[i])) {
            gpio_pin_configure_dt(&cata_leds[i], GPIO_OUTPUT_INACTIVE);
        }
    }
}

/* Shows a value on a display, or "----" if value is -1 (no value). Skips missing displays. */
static void show_value(const struct device *dev, const char *name, float value) {
    if (dev == NULL) {
        return;
    }
    char str[5];
    if (value == -1) {
        snprintf(str, sizeof(str), "----");
    } else {
        snprintf(str, sizeof(str), "%.1f", (double)value);
    }
    int ret = display_seven_seg(dev, str);
    if (ret) {
        LOG_ERR("%s display: failed to show \"%s\" (%d)", name, str, ret);
    }
}

static void set_cata_led(ui_state_t cata, int value) {
    if (cata_leds[cata].port != NULL) {
        gpio_pin_set_dt(&cata_leds[cata], value);
    }
}

int ui_sequence() {
    LOG_INF("UI sequence start");
    if (current == NULL) {
        LOG_WRN("current display not set, skipping it");
    } else {
        clear_seven_seg(current);
    }
    if (setpoint == NULL) {
        LOG_WRN("setpoint display not set, skipping it");
    } else {
        clear_seven_seg(setpoint);
    }

    init_cata_leds();
    set_cata_led(0, 1);
    int32_t last_pos = dial_get_position();
    ui_state_t current_cata = 0;

    while (1) {
        if (button_pressed(BTN_START)) {
            LOG_INF("bake started normally.");
            mcu.bake_start = true;
            return 0;
        }
        if (button_pressed(BTN_MIX)) {
            //MIX
        }
        if (button_pressed(BTN_UI) && 
        (current_cata == SUBSTRATE_LEVEL || 
            current_cata == TIME || 
            current_cata == HUMIDITY || 
            current_cata == SUBSTRATE_TEMP || 
            current_cata == FAN_SPEED)) 
            {
            LOG_INF("editing %s", cata_names[current_cata]);

            int ret = edit_cata(current_cata, &last_pos);
            if (ret) {
                LOG_INF("bake started during editing.");
                mcu.bake_start = true;int32_t dial_delta(int32_t *last_pos);

                return 0;
            }
            LOG_INF("done editing %s", cata_names[current_cata]);
        }

        int32_t delta = dial_delta(&last_pos);
        
        if (delta != 0) {
            set_cata_led(current_cata, 0);
            int next = ((int)current_cata + delta) % NUM_UI_STATES;
            if (next < 0) {
                next += NUM_UI_STATES;
            }
            current_cata = (ui_state_t)next;
            set_cata_led(current_cata, 1);
            LOG_INF("dial delta=%d -> category %s", delta, cata_names[current_cata]);
        }

        show_value(current, "current", mcu.current_values[current_cata]);
        show_value(setpoint, "setpoint", mcu.set_points[current_cata]);

        k_sleep(K_MSEC(100));
    }
}

static int edit_cata(ui_state_t cata, int32_t *last_pos) {
    float current_value = mcu.set_points[cata];
    while(1) {
        if (button_pressed(BTN_START)) {
            return 1;
        }
        if (button_pressed(BTN_UI)) {
            mcu.set_points[cata] = current_value;
            char saved_str[8];
            snprintf(saved_str, sizeof(saved_str), "%.1f", (double)current_value);
            LOG_INF("%s setpoint saved: %s", cata_names[cata], saved_str);
            return 0;
        }
        int32_t delta = dial_delta(last_pos);
        current_value += 0.1 * delta;

        current_value = CLAMP(current_value, 0, 99.9);

        char set_str[5];
        snprintf(set_str, sizeof(set_str), "%.1f", current_value);
        display_seven_seg(setpoint, set_str);
        k_sleep(K_MSEC(50));
    }
}

int32_t dial_delta(int32_t *last_pos) {
    int32_t pos = dial_get_position();
    int32_t delta = pos - *last_pos;
    if (delta != 0) {
        LOG_DBG("dial pos=%d raw=%d", pos, delta);
    }
    // delta = delta / 2 + delta % 2;
    *last_pos = pos;
    return delta;
}
