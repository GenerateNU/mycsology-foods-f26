#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

int init_motors();
int stepper_move_deg(const struct device *dev, int32_t deg);
int mover_stepper_move_deg(int32_t deg);
int rake_stepper_move_deg(int32_t deg);
int rake_motion();
int go_home();


#endif //MOTOR_H