#include "seven_seg.h"

static inline uint8_t siekoo_from_char(char c) {
    if (c >= '0' && c <= '9') return siekoo_seg[c - '0'];
    if (c >= 'a' && c <= 'z') return siekoo_seg[10 + (c - 'a')];
    if (c >= 'A' && c <= 'Z') return siekoo_seg[10 + (c - 'A')];
    if (c == '-')             return 0x40;
    return 0x00;
}   

uint8_t display_seven_seg(const struct device *dev, char* chars) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Seven_seg: device not ready.");
        return 1;
    }
    uint8_t len = (sizeof(chars) > 4) ? 4 : sizeof(chars);
    for (int i = 0; i < len; i++) {
        uint8_t seg = siekoo_from_char(chars[i]);
        for (int j = 0; j < 8; j++) {
            if (seg & (1 << j)) {
                uint8_t ret = led_on(dev, digit_offset[i] + j);
                if (ret) {
                    return ret;
                }
            } else {
                uint8_t ret = led_off(dev, digit_offset[i] + j);
                if (ret) {
                    return ret;
                }
            }
        }
    }
    return 0;
}