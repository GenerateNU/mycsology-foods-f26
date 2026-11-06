#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

int init_motor(const struct device *dev);
int stepper_move_deg(const struct device *dev, int32_t deg);

#endif //MOTOR_H