#include "motor.h"
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(motor);

#define STEPS_PER_REV 200
#define DEGS_PER_STEP 1.8

const struct device *rake_dev;
const struct device *move_dev;

static int32_t angle_to_steps(int32_t deg);

int init_motor(const struct device *dev) {
    if (!device_is_ready(dev)) {
        LOG_ERR("motor: device not ready.");
        return 1;
    }
    /* 10 ms between microsteps = 100 microsteps/s 30 RPM*/
    stepper_ctrl_set_microstep_interval(dev, 10000000);

    LOG_INF("motor: device init success");
    return 0;
}

int stepper_move_deg(const struct device *dev, int32_t deg) {
    int32_t steps = angle_to_steps(deg);
    int ret = stepper_ctrl_move_by(dev, steps);
    return ret;
}

static int32_t angle_to_steps(int32_t deg) {
    return roundf(deg / DEGS_PER_STEP);
}

static int32_t angle_to_steps(int32_t deg) {
    return roundf(deg / DEGS_PER_STEP);
}

int rake_motion() {
    // go home
    go_home();
    // rake 180
    stepper_motor_move(rake_dev, -180); // ccw
    // move carrige to back (exact number to come)
    stepper_motor_move(move_dev, -1);
    // put in (ccw)
    // do first sweep (full track)
    // move back a little
    // flip long way around (cw)
    // sweep back (full)
    // maybe repeat (?)
    // bring rake straight up
    // move to front limit switch
}

// go home = move both until limit switch
int go_home() {
    // move both until limit switches
}

