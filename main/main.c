#include "pwm.h"
#include "wifi.h"
#include "webserver.h"
#include "imu.h"
#include "flight_control.h"

#include "config.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>

volatile bool correction_enabled = true;
volatile bool armed = false;

void motor_task(void *arg)
{
    static bool last_arm_state = false;

    while(1)
    {

        if(last_arm_state != armed)
        {
            printf(
                "ARM STATE: %s\n",
                armed ? "ARMED" : "DISARMED"
            );

            last_arm_state = armed;
        }    
	int corr_fl = correction_enabled ? runtime_correction[FRONT_LEFT]  : 0;
        int corr_fr = correction_enabled ? runtime_correction[FRONT_RIGHT] : 0;
        int corr_rr = correction_enabled ? runtime_correction[REAR_RIGHT]  : 0;
        int corr_rl = correction_enabled ? runtime_correction[REAR_LEFT]   : 0;


        int fl = armed ?
            throttle + drone_config.trim[FRONT_LEFT] + corr_fl :
            1000;

        int fr = armed ?
            throttle + drone_config.trim[FRONT_RIGHT] + corr_fr :
            1000;

        int rr = armed ?
            throttle + drone_config.trim[REAR_RIGHT] + corr_rr :
            1000;

        int rl = armed ?
            throttle + drone_config.trim[REAR_LEFT] + corr_rl :
            1000;


        set_logical_motor(FRONT_LEFT, fl);
        set_logical_motor(FRONT_RIGHT, fr);
        set_logical_motor(REAR_RIGHT, rr);
        set_logical_motor(REAR_LEFT, rl);


        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void correction_task(void *arg)
{
    while(1)
    {
        float pitch;
        float roll;


        imu_get_angle(
            &pitch,
            &roll
        );


        int correction = 10;


        // Example:
        // drone nose is down
        if(pitch > 30)
        {
            runtime_correction[REAR_RIGHT] = correction;
            runtime_correction[3] = correction;

            runtime_correction[FRONT_LEFT] = -correction;
            runtime_correction[FRONT_RIGHT] = -correction;
        }


        else if(pitch < -30)
        {
            runtime_correction[FRONT_LEFT] = correction;
            runtime_correction[FRONT_RIGHT] = correction;

            runtime_correction[REAR_RIGHT] = -correction;
            runtime_correction[REAR_LEFT] = -correction;
        }


        else
        {
            runtime_correction[FRONT_LEFT] = 0;
            runtime_correction[FRONT_RIGHT] = 0;
            runtime_correction[REAR_RIGHT] = 0;
            runtime_correction[REAR_LEFT] = 0;
        }


        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}

void app_main(void)
{
    // Initialize flash storage first
    ESP_ERROR_CHECK(
        nvs_flash_init()
    );


    // Load motor mapping and trims
    config_load();


    wifi_init_softap();

    webserver_start();

    pwm_init();

	bool imu_ok = imu_init();

	if (!imu_ok)
	{
	    printf("IMU initialization failed, stopping flight control\n");
	    return;
	}

    imu_calibrate();


    flight_control_start();

    xTaskCreate(
        motor_task,
        "motor_task",
        2048,
        NULL,
        5,
        NULL
    );


}
