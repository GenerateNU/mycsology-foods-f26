#ifndef BUZZER_H
#define BUZZER_H

#include <zephyr/drivers/buzzer.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buzzer);

#define NOTE_DURATION_MS 1000

static const uint32_t confirm_tune[2] = {
    880,
    1318,
};

static const uint32_t done_tune[4] = {
    880,
    1318,
    880,
    1760,
};

static const uint32_t error_tune[4] = {
    880,
    880,
    880,
};

int buzzer_play_tune(const struct device *dev, uint32_t* tune);

#endif // BUZZER_H