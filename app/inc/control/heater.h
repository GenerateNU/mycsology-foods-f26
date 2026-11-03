#ifndef HEATER_H
#define HEATER_H

#include <zephyr/device.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

int init_heater(const struct pwm_dt_spec *dev);
int set_heater_percent(const struct pwm_dt_spec *dev, uint8_t pct);

#endif // HEATER_H