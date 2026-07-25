#ifndef IMU_H
#define IMU_H

#include <stdbool.h>

bool imu_init(void);
void imu_calibrate(void);
bool imu_get_angle(float *pitch, float *roll);
bool imu_get_gyro(
    float *pitch_rate,
    float *roll_rate
);

#endif
