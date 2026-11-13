#ifndef TYPES_H
#define TYPES_H

#include <zephyr/device.h>
#include <stdint.h>

typedef struct {
    volatile bool limit_switch_state;
    volatile bool ui_button;

    volatile float *set_points;
    volatile float *current_values;
} mcu_t;

extern mcu_t mcu;

#endif