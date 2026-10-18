#ifndef BUZZER_H
#define BUZZER_H

#include <zephyr/drivers/buzzer.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

#define NOTE_DURATION_MS 1000

extern const uint32_t confirm_tune[2];
extern const uint32_t done_tune[4];
extern const uint32_t error_tune[3];

int buzzer_play_tune(const struct device *dev, const uint32_t* tune, size_t size);

#endif // BUZZER_H