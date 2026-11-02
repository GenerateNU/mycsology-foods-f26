#ifndef FAN_H
#define FAN_H

#include <stdint.h>
#include <zephyr/drivers/pwm.h>

int init_fan(const struct pwm_dt_spec *dev);
int set_fan_percent(const struct pwm_dt_spec *dev, uint8_t pct);

#endif // FAN_H