#include "buzzer.h"

LOG_MODULE_REGISTER(buzzer);

const uint32_t confirm_tune[2] = {
    880,
    1318,
};

const uint32_t done_tune[4] = {
    880,
    1318,
    880,
    1760,
};

const uint32_t error_tune[3] = {
    880,
    880,
    880,
};

/**
 * @brief Plays a tune on the buzzer.
 * 
 * @param dev buzzer_dev
 * @param tune one of the tunes defined in the buzzer.h file
 * @param size length of the tune
 * @return int 
 */
int buzzer_play_tune(const struct device *dev, const uint32_t* tune, size_t size) {
    if (!device_is_ready(dev)) {
        LOG_ERR("buzzer: device not ready.");
        return 1;
    }

    for (int i = 0; i < size; i++) {
        uint8_t ret = buzzer_tone(dev, tune[i], NOTE_DURATION_MS);
        if (ret) {
            return ret;
        }
    }
    return 0;
}