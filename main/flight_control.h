#ifndef FLIGHT_CONTROL_H
#define FLIGHT_CONTROL_H

#include <stdint.h>

extern volatile bool armed;

extern volatile int throttle;

extern volatile int runtime_correction[4];

void flight_control_start(void);

#endif
