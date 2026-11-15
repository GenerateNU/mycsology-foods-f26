#ifndef TYPES_H
#define TYPES_H

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>

enum button_event {
    BTN_START,
    BTN_MIX,
    BTN_UI,
};

typedef struct {
    volatile bool limit_switch_state;
    atomic_t button_events;

    volatile float *set_points;
    volatile float *current_values;
} mcu_t;

extern mcu_t mcu;

#endif