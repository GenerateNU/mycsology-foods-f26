#ifndef TYPES_H
#define TYPES_H

#include <zephyr/device.h>

typedef struct {
    volatile bool limit_switch_state;
} mcu_t;

extern mcu_t mcu;

#endif