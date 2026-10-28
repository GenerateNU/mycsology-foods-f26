#include "seven_seg.h"

LOG_MODULE_REGISTER(seven_seg);

static inline uint8_t siekoo_from_char(char c) {
    if (c >= '0' && c <= '9') return siekoo_seg[c - '0'];
    if (c >= 'a' && c <= 'z') return siekoo_seg[10 + (c - 'a')];
    if (c >= 'A' && c <= 'Z') return siekoo_seg[10 + (c - 'A')];
    if (c == '-')             return 0x40;
    return 0x00;
}   

/**
 * @brief Diplays numbers and letters on the seven segment display
 * 
 * @param dev seven_seg_dev
 * @param chars array of char to display
 * @param size number of char to display
 * @return int 
 */
int display_seven_seg(const struct device *dev, char* chars, size_t size) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Seven_seg: device not ready.");
        return 1;
    }
    uint8_t len = (size > 4) ? 4 : size;
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

/**
 * @brief Displays punctuation on the seven segment display (decimal points for every digit and colon)
 * 
 * @param dev seven_seg_dev
 * @param punc_index pass in a 5 digit binary number representing which digits sould have the punctuation 
 *  ex. 0b00100 = 12:34, 0b01000 = 1.234, 0b00110 = 12:3.4 etc
 * @return uint8_t
 */
int display_seven_seg_punctuation(const struct device *dev, uint8_t punc_index) { 
    if (!device_is_ready(dev)) {
        LOG_ERR("Seven_seg: device not ready.");
        return 1;
    }
    for (int i = 0; i < 5; i++) {
        if (punc_index & (1 << i)) {
            if (i == 2) {
                uint8_t ret = led_on(dev, 0x04);
                if (ret) {
                    return ret;
                }
            } else {
                uint8_t ret = led_on(dev, (i * 16) + 7);
                if (ret) {
                    return ret;
                }
            }
        }
    }
    return 0;
}

/**
 * @brief Turns off all leds in the seven segment display
 * 
 * @param dev seven_seg_dev
 * @return uint8_t 
 */
int clear_seven_seg(const struct device *dev) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Seven_seg: device not ready.");
        return 1;
    }
    for (int i = 0; i < 128; i++) {
        led_off(dev, i);
    }
    return 0;
}