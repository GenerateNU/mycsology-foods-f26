#include "seven_seg.h"
#include <errno.h>
#include <stddef.h>

LOG_MODULE_REGISTER(seven_seg);

#define SEG_INVALID 0xFF  /* character has no seven segment representation */
#define SEG_DP      0x80  /* decimal point bit within a digit */
#define COLON_LED   33

static inline uint8_t siekoo_from_char(char c) {
    if (c >= '0' && c <= '9') return siekoo_seg[c - '0'];
    if (c >= 'a' && c <= 'z') return siekoo_seg[10 + (c - 'a')];
    if (c >= 'A' && c <= 'Z') return siekoo_seg[10 + (c - 'A')];
    if (c == '-')             return 0x40;
    if (c == ' ')             return 0x00;
    return SEG_INVALID;
}

/**
 * @brief Displays a string on the seven segment display, right aligned
 *
 * Up to 4 characters (0-9, a-z, A-Z, '-', ' '). A '.' lights the decimal point of the
 * preceding character. A ':' lights the colon and must have exactly 2 characters after it.
 * Every segment, decimal point, and the colon is rewritten on each call.
 * ex. "12:34", "1.234", "12.3.4", "belo.", "-5" (shows as "  -5")
 *
 * @param dev seven_seg_dev
 * @param str null terminated string to display
 * @return int 0 on success, -ENODEV if the device is not ready, -EINVAL if str is
 *  NULL or malformed (nothing is written), or a negative errno from the LED driver
 */
int display_seven_seg(const struct device *dev, const char *str) {
    if (!device_is_ready(dev)) {
        LOG_ERR("Seven_seg: device not ready.");
        return -ENODEV;
    }
    if (str == NULL) {
        return -EINVAL;
    }

    uint8_t parsed[4] = {0};
    size_t count = 0;
    int colon_pos = -1; /* number of characters before the colon */

    for (const char *p = str; *p != '\0'; p++) {
        if (*p == '.') {
            if (count == 0 || (parsed[count - 1] & SEG_DP)) {
                return -EINVAL;
            }
            parsed[count - 1] |= SEG_DP;
        } else if (*p == ':') {
            if (colon_pos != -1) {
                return -EINVAL;
            }
            colon_pos = count;
        } else {
            uint8_t seg = siekoo_from_char(*p);
            if (seg == SEG_INVALID || count == 4) {
                return -EINVAL;
            }
            parsed[count++] = seg;
        }
    }

    if (colon_pos != -1 && (count - colon_pos) != 2) {
        return -EINVAL;
    }

    /* right align */
    uint8_t segs[4] = {0};
    for (size_t i = 0; i < count; i++) {
        segs[4 - count + i] = parsed[i];
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            uint32_t led = digit_offset[i] + j;
            int ret = (segs[i] & (1u << j)) ? led_on(dev, led) : led_off(dev, led);
            if (ret) {
                return ret;
            }
        }
    }

    return (colon_pos != -1) ? led_on(dev, COLON_LED) : led_off(dev, COLON_LED);
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