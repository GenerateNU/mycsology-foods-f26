#include "heater.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(heater);

int init_heater(const struct pwm_dt_spec *dev) {
    if (!pwm_is_ready_dt(dev)) {
        LOG_ERR("heater: device not ready.");
        return 1;
    }
    LOG_INF("heater: init success.");
    return 0;
}

int set_heater_percent(const struct pwm_dt_spec *dev, uint8_t pct) {
    if (pct > 100) {
        pct = 100;
    }
    uint32_t pulse = (uint32_t)(((uint64_t)dev->period * pct) / 100);
    return pwm_set_pulse_dt(dev, pulse);
}