#include "driver/ledc.h"
#include "esp_log.h"

#include "pwm.h"
#include "config.h"
#include "motor_test.h"

#define MOTOR_COUNT 4

volatile int test_motor = -1;
volatile int test_value = 1200;

// Change these pins to your wiring
static const int motor_pins[MOTOR_COUNT] =
{
    13,
    14,
    21,
    22
};


static const char *TAG = "PWM";


// Convert microseconds to LEDC duty
//
// 50Hz:
// period = 20,000us
//
// With 16-bit resolution:
// 65535 counts = 20000us
//
// duty = pulse_us / 20000 * 65535

static uint32_t us_to_duty(int us)
{
    return (uint32_t)(
        ((uint64_t)us * 65535) / 20000
    );
}

void pwm_init(void)
{

    /*
     * Timer configuration
     */

    ledc_timer_config_t timer =
    {
        .speed_mode =
            LEDC_LOW_SPEED_MODE,

        .timer_num =
            LEDC_TIMER_0,

        .duty_resolution =
            LEDC_TIMER_16_BIT,

        .freq_hz =
            50,

        .clk_cfg =
            LEDC_AUTO_CLK
    };


    ledc_timer_config(&timer);



    /*
     * Four PWM channels
     */

    for(int i=0;i<MOTOR_COUNT;i++)
    {

        ledc_channel_config_t channel =
        {
            .gpio_num =
                motor_pins[i],

            .speed_mode =
                LEDC_LOW_SPEED_MODE,

            .channel =
                i,

            .timer_sel =
                LEDC_TIMER_0,

            .duty =
                us_to_duty(1000),

            .hpoint =
                0
        };


        ledc_channel_config(
            &channel);
    }


    ESP_LOGI(TAG,
        "PWM initialized");

}

void set_logical_motor(logical_motor_t logical, int pwm)
{
    if(logical < 0 || logical >= LOGICAL_MOTOR_COUNT)
    {
        return;
    }

    int physical = drone_config.motor_map[logical];


    if(test_motor >= 0)
    {
        if(physical == test_motor)
        {
            pwm_set_motor(
                physical,
                test_value
            );
        }
        else
        {
            pwm_set_motor(
                physical,
                1000
            );
        }

        return;
    }


    pwm_set_motor(
        physical,
        pwm
    );
}

void pwm_set_motor(
    int motor,
    int throttle)
{

    if(motor < 0 ||
       motor >= MOTOR_COUNT)
        return;


    // Limit input
    if(throttle < 1000)
        throttle = 1000;

    if(throttle > 2000)
        throttle = 2000;


    uint32_t duty =
        us_to_duty(throttle);


    ledc_set_duty(
        LEDC_LOW_SPEED_MODE,
        motor,
        duty);


    ledc_update_duty(
        LEDC_LOW_SPEED_MODE,
        motor);
}
