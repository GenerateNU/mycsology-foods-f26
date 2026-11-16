#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <types.h>

struct debounced_button {
    const struct gpio_dt_spec *spec;
    enum button_event event;
    struct k_work_delayable work;
};

int init_all_buttons();
int init_limit_switch();
bool button_pressed(enum button_event btn);

// Callbacks

void start_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void mix_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void ui_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void limit_switch_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

#endif // BUTTONS_H