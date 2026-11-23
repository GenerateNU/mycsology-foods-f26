#include "buttons.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buttons);

extern mcu_t mcu;

// Buttons fall back to an empty spec (skipped in init_button) until their aliases exist in app.overlay
static const struct gpio_dt_spec start_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(b1), gpios, {0});
static struct gpio_callback start_button_cb_data;
static const struct gpio_dt_spec mix_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(b2), gpios, {0});
static struct gpio_callback mix_button_cb_data;
static const struct gpio_dt_spec ui_button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(b0), gpios, {0});
static struct gpio_callback ui_button_cb_data;
static const struct gpio_dt_spec limit_switch = GPIO_DT_SPEC_GET(DT_ALIAS(limit_sw), gpios);
static struct gpio_callback limit_switch_cb_data;

static const struct gpio_dt_spec ext_led;
//= GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static bool led_state = false;

/*
 * Debounce: each edge (re)starts a DEBOUNCE_MS timer. When it expires the pin is read again,
 * and the press only counts if the button is still held. Bounces (including on release) and
 * short noise spikes never get recorded.
 */
#define DEBOUNCE_MS 20

static struct debounced_button start_btn = { .spec = &start_button, .event = BTN_START };
static struct debounced_button mix_btn   = { .spec = &mix_button,   .event = BTN_MIX };
static struct debounced_button ui_btn    = { .spec = &ui_button,    .event = BTN_UI };

static void debounce_handler(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct debounced_button *btn = CONTAINER_OF(dwork, struct debounced_button, work);

    if (gpio_pin_get_dt(btn->spec) == 1) { // logical level: 1 = pressed (active low handled by devicetree)
        atomic_set_bit(&mcu.button_events, btn->event);
        // gpio_pin_set_dt(&ext_led, (led_state) ? 1 : 0); // test led, toggles once per real press
        led_state = !led_state;
        LOG_DBG("button %d pressed", btn->event);
    }
}

int init_led();
int init_button(
    const struct gpio_dt_spec *button, 
    struct gpio_callback *button_cb_data, 
    gpio_callback_handler_t callback_func
);

/**
 * @brief Initializeds all buttons as well as an led for testing purposes (move led init later)
 * 
 * @return int 
 */
int init_all_buttons() {
    // work items must be ready before interrupts are enabled in init_button
    k_work_init_delayable(&start_btn.work, debounce_handler);
    k_work_init_delayable(&mix_btn.work, debounce_handler);
    k_work_init_delayable(&ui_btn.work, debounce_handler);

    return init_led() ||
    init_button(&start_button, &start_button_cb_data, start_button_callback) ||
    init_button(&mix_button, &mix_button_cb_data, mix_button_callback) ||
    init_button(&ui_button, &ui_button_cb_data, ui_button_callback);
}

int init_limit_switch() {
    return init_button(&limit_switch, &limit_switch_cb_data, limit_switch_callback);
}

/**
 * @brief Checks if a button was pressed since the last call, and clears it
 * 
 * @param btn which button to check
 * @return true once per debounced press
 */
bool button_pressed(enum button_event btn) {
    return atomic_test_and_clear_bit(&mcu.button_events, btn);
}

/**
 * @brief led init for testing, TODO: move later
 * 
 * @return int 
 */
int init_led() {
    return 0; //gpio_pin_configure_dt(&ext_led, GPIO_OUTPUT_INACTIVE);
}

/**
 * @brief inits one button
 * 
 * @param button button gpio
 * @param button_cb_data callback data object (idk what this does just that we need it)
 * @param callback_func callback function
 * @return int 
 */
int init_button(
    const struct gpio_dt_spec *button, 
    struct gpio_callback *button_cb_data, 
    gpio_callback_handler_t callback_func
) 
{
    int ret;

    if (button->port == NULL) {
        LOG_WRN("Button not defined in devicetree, skipping");
        return 0;
    }
 
    if (!gpio_is_ready_dt(button)) {
        LOG_ERR("Button device not ready");
        return 1;
    }
 
    ret = gpio_pin_configure_dt(button, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure pin (%d)", ret);
        return 1;
    }
 
    ret = gpio_pin_interrupt_configure_dt(button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Failed to configure interrupt (%d)\n", ret);
        return 1;
    }
 
    gpio_init_callback(button_cb_data, callback_func, BIT(button->pin));
    gpio_add_callback(button->port, button_cb_data);

    return 0;
}

// Button callbacks (ISR): just restart the debounce timer, debounce_handler records the press

void start_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_reschedule(&start_btn.work, K_MSEC(DEBOUNCE_MS));
}

void mix_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_reschedule(&mix_btn.work, K_MSEC(DEBOUNCE_MS));
}

void ui_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_reschedule(&ui_btn.work, K_MSEC(DEBOUNCE_MS));
}

void limit_switch_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    // Add logic here
    mcu.limit_switch_state = !mcu.limit_switch_state;
}