#ifndef PWM_H
#define PWM_H

#include "config.h"

extern volatile int test_motor;
extern volatile int test_value;

void set_logical_motor(
    logical_motor_t logical,
    int pwm
);

void pwm_init(void);

void pwm_set_motor(
    int motor,
    int throttle
);

#endif