#pragma once
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "buttons.h"
#include "buzzer.h"
#include "dial.h"
#include "seven_seg.h"
#include <stdio.h>

typedef enum {
    SUBSTRATE_LEVEL,
    TIME,
    HUMIDITY,
    SUBSTRATE_TEMP,
    CHAMBER_TEMP,
    FAN_SPEED,
    AUTO,
    NUM_UI_STATES
} ui_state_t;

int edit_cata(ui_state_t cata, int32_t *last_pos);
int32_t dial_delta(int32_t *last_pos);
int ui_sequence();

