#include "flight_control.h"

#include "imu.h"
#include "pwm.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <math.h>


#define KP 2.0f
#define KD 0.7f

volatile int runtime_correction[4] =
{
    0,
    0,
    0,
    0
};

volatile int throttle = 1000;


static void flight_task(void *arg)
{
    int debug_counter = 0;

    while(1)
    {
        float pitch = 0.0f;
        float roll = 0.0f;
        float pitch_rate = 0.0f;
        float roll_rate = 0.0f;

        // Get filtered angle
        if (!imu_get_angle(&pitch, &roll))
        {
            continue;
        }

        // Get gyro speed
        if (!imu_get_gyro(&pitch_rate, &roll_rate))
        {
            continue;
        }


        //printf("pitch %.2f roll %.2f\n", pitch, roll);


        // Your IMU orientation:
        // forward tilt = +roll
        // right tilt   = -pitch

        int forward_back =
            roll * KP -
            roll_rate * KD;


        int left_right =
            -pitch * KP -
            pitch_rate * KD;

        runtime_correction[FRONT_LEFT] =
            forward_back - left_right;

        runtime_correction[FRONT_RIGHT] =
            forward_back + left_right;

        runtime_correction[REAR_RIGHT] =
           -forward_back + left_right;

        runtime_correction[REAR_LEFT] =
           -forward_back - left_right;

        debug_counter++;

        if(debug_counter >= 25)
        {
/*            printf(
                "\nCORR FL:%d FR:%d RR:%d RL:%d",
                runtime_correction[FRONT_LEFT],
                runtime_correction[FRONT_RIGHT],
                runtime_correction[REAR_RIGHT],
                runtime_correction[REAR_LEFT]
            );*/

            debug_counter = 0;
        }


        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


void flight_control_start(void)
{
    xTaskCreate(
        flight_task,
        "flight",
        4096,
        NULL,
        10,
        NULL
    );
}
