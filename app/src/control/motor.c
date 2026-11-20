#include "motor.h"
#include <zephyr/logging/log.h>
#include <math.h>
#include "types.h"
#include "hall_effect.h"

LOG_MODULE_REGISTER(motor);

#define STEPS_PER_REV 200
#define DEGS_PER_STEP 1.8

extern mcu_t mcu;

static const struct device *rake_dev = DEVICE_DT_GET(DT_ALIAS(stepper));
static const struct device *move_dev = DEVICE_DT_GET(DT_ALIAS(mover_stepper));

static const struct device *hall = DEVICE_DT_GET(DT_NODELABEL(hall));

inline static int32_t angle_to_steps(int32_t deg) {
    return roundf(deg / DEGS_PER_STEP);
};

int init_motors() {
    if (!device_is_ready(rake_dev)) {
        LOG_ERR("motor: rake device not ready.");
        return 1;
    }
    if (!device_is_ready(move_dev)) {
        LOG_ERR("motor: move device not ready.");
        return 1;
    }

    if (!device_is_ready(hall)) {
        LOG_ERR("hall: hall device not ready.");
        return 1;
    }

    /* 10 ms between microsteps = 100 microsteps/s 30 RPM*/
    stepper_ctrl_set_microstep_interval(rake_dev, 10000000);
    stepper_ctrl_set_microstep_interval(move_dev, 10000000);

    LOG_INF("motor: device init success");
    return 0;
}

int stepper_move_deg(const struct device *dev, int32_t deg) {
    int32_t steps = angle_to_steps(deg);
    int ret = stepper_ctrl_move_by(dev, steps);
    return ret;
}

int mover_stepper_move_deg(int32_t deg) {
    return stepper_move_deg(move_dev, deg);
}

int rake_stepper_move_deg(int32_t deg) {
    return stepper_move_deg(rake_dev, deg);
}

// go home = move until limit switch
int go_home_mover() {
    stepper_ctrl_run(move_dev, STEPPER_CTRL_DIRECTION_POSITIVE);
    bool prev_ls_state = mcu.limit_switch_state;
    while(prev_ls_state == mcu.limit_switch_state) {
        k_sleep(K_MSEC(10));
    }
    stepper_ctrl_stop(move_dev);
    return 0;
};

int go_home_rake() {
    stepper_ctrl_run(rake_dev, STEPPER_CTRL_DIRECTION_POSITIVE);
    struct sensor_value present;
    while(1) {
        if (sensor_sample_fetch(hall) != 0) {
			LOG_INF("ADC error\n");
			k_msleep(500);
			continue;
		}
        sensor_channel_get(hall, SENSOR_CHAN_MAGNET_PRESENT, &present);
        if (present.val1 == 1) {
            stepper_ctrl_stop(rake_dev);
            LOG_INF("*** MAGNET %s ***\n", present.val1 ? "PRESENT" : "NOT PRESENT");
            return 0;
        }
        LOG_INF("*** MAGNET %s ***\n", present.val1 ? "PRESENT" : "NOT PRESENT");
        k_msleep(10);
    }
}

inline int32_t lower_angle() {
    return -1; // Shrey will give a formula
};

int rake_motion() {
    // go home
    go_home_mover();
    k_msleep(1000);
    go_home_rake();

    // rake 180
    rake_stepper_move_deg(-180); // ccw
    k_sleep(K_MSEC(1000));

    // move carrige to back (exact number to come)
    mover_stepper_move_deg(-1);

    // put in (ccw)
    rake_stepper_move_deg(lower_angle());
    k_sleep(K_MSEC(1000));

    // do first sweep (full track)
    mover_stepper_move_deg(10);
    k_sleep(K_MSEC(10000)); //10sec

    // move back a little
    mover_stepper_move_deg(10);

    // flip long way around (cw)
    rake_stepper_move_deg(-2 * lower_angle());
    k_sleep(K_MSEC(2000));

    // sweep back (full)
    mover_stepper_move_deg(-10);
    k_sleep(K_MSEC(10000));//10sec

    // maybe repeat (?)

    // bring rake straight up
    rake_stepper_move_deg(lower_angle());
    k_sleep(K_MSEC(1000));

    // move to front limit switch
    go_home_mover();
    go_home_rake();

    return 0;
} //need from shrey: length of track, formula for tray height to rake angle, maybe how far to move back, probably cm to deg for mover.

