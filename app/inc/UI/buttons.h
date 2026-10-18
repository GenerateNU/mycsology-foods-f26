#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

int init_all_buttons();

// Callbacks

void start_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void mix_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void ui_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

#endif // BUTTONS_H