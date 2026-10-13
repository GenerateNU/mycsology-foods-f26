#include <buzzer.h>

int buzzer_play_tune(const struct device *dev, uint32_t* tune) {
    if (!device_is_ready(dev)) {
        LOG_ERR("buzzer: device not ready.");
        return 1;
    }

    for (int i = 0; i < sizeof(tune); i++) {
        uint8_t ret = buzzer_tone(dev, tune[i], NOTE_DURATION_MS);
        if (ret) {
            return ret;
        }
    }
    return 0;
}