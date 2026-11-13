#ifndef TYPES_H
#define TYPES_H

#include <zephyr/device.h>
#include <stdint.h>

typedef struct {
    volatile bool limit_switch_state;
    volatile uint8_t ui_button;
} mcu_t;

extern mcu_t mcu;

#endif