#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "imu.h"
#include "config.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <math.h>

#define TAG "IMU"

#define I2C_PORT 0

#define SDA_PIN 19
#define SCL_PIN 18

#define MPU6050_ADDR 0x68

#define IMU_AVG_SAMPLES 20

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t mpu_handle;

static float pitch_offset = 0;
static float roll_offset = 0;

static float filtered_pitch = 0;
static float filtered_roll = 0;

static int64_t last_imu_time = 0;

static float gyro_x_offset = 0;
static float gyro_y_offset = 0;

void imu_calibrate(void)
{
    float gx_sum = 0;
    float gy_sum = 0;

    printf("Calibrating gyro. Keep still...\n");

    for(int i = 0; i < 500; i++)
    {
        float roll_rate;
        float pitch_rate;

        imu_get_gyro(&pitch_rate, &roll_rate);

        gx_sum += roll_rate;
        gy_sum += pitch_rate;

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    gyro_x_offset = gx_sum / 500.0f;
    gyro_y_offset = gy_sum / 500.0f;


    printf("gyro offsets gx %.3f gy %.3f\n",
           gyro_x_offset,
           gyro_y_offset);


    printf("Calibrating angle. Keep still...\n");

    float pitch_sum = 0;
    float roll_sum = 0;

    for(int i = 0; i < 100; i++)
    {
        float p,r;

        imu_get_angle(&p,&r);

        pitch_sum += p;
        roll_sum += r;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    pitch_offset = pitch_sum / 100.0f;
    roll_offset = roll_sum / 100.0f;


    printf("Offsets: pitch %.2f roll %.2f\n",
           pitch_offset,
           roll_offset);
}
void i2c_scan()
{
    for(uint8_t addr = 1; addr < 127; addr++)
    {
        esp_err_t err = i2c_master_probe(
            bus_handle,
            addr,
            100
        );

        if(err == ESP_OK)
        {
            printf("Found I2C device at 0x%02X\n", addr);
        }
    }
}
static bool mpu_read(
    uint8_t reg,
    uint8_t *data,
    size_t len
)
{
    esp_err_t err = i2c_master_transmit_receive(
        mpu_handle,
        &reg,
        1,
        data,
        len,
        -1
    );

    if (err != ESP_OK)
    {
        printf("I2C read failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}
bool imu_get_gyro(float *pitch_rate, float *roll_rate)
{
    uint8_t data[6];

    if (!mpu_read(0x43, data, 6))
    {
        return false;
    }

    int16_t gx = (data[0] << 8) | data[1];
    int16_t gy = (data[2] << 8) | data[3];

    // Default sensitivity is ±250 deg/s
    *roll_rate  = gx / 131.0f;
    *pitch_rate = gy / 131.0f;

    return true;
}

bool imu_init(void)
{
    i2c_master_bus_config_t bus_config =
    {
        .i2c_port = I2C_PORT,

        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,

        .clk_source = I2C_CLK_SRC_DEFAULT,

        .glitch_ignore_cnt = 7,

        .flags.enable_internal_pullup = true
    };


    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &bus_handle
        )
    );

    printf("I2C scan start\n");
    i2c_scan();

	for(uint8_t addr = 1; addr < 127; addr++)
	{
	    esp_err_t err = i2c_master_probe(
		bus_handle,
		addr,
		100
	    );

	    if(err == ESP_OK)
	    {
		printf("Found device: 0x%02X\n", addr);
	    }
	}

	printf("I2C scan done\n");
    i2c_device_config_t dev_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,

        .device_address = MPU6050_ADDR,

        .scl_speed_hz = 400000
    };


    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            bus_handle,
            &dev_config,
            &mpu_handle
        )
    );

	uint8_t who = 0;

	esp_err_t who_err = i2c_master_transmit_receive(
	    mpu_handle,
	    (uint8_t[]){0x75},
	    1,
	    &who,
	    1,
	    -1
	);

	printf(
	    "WHO_AM_I: 0x%02X (%s)\n",
	    who,
	    esp_err_to_name(who_err)
	);    


    // Wake MPU6050
    uint8_t wake[] =
    {
        0x6B,
        0x00
    };

	esp_err_t err = i2c_master_transmit(
	    mpu_handle,
	    wake,
	    sizeof(wake),
	    -1
	);

	if (err != ESP_OK)
	{
	    printf("imu_init: %s (%d)\n", esp_err_to_name(err), err);
	    return false;   // or return; if imu_init() is void
	}

    ESP_LOGI(
        TAG,
        "MPU6050 ready"
    );

	uint8_t regs[] = {
	    0x75, // WHO_AM_I
	    0x6B, // PWR_MGMT_1
	    0x1B, // GYRO_CONFIG
	    0x1C  // ACCEL_CONFIG
	};

	for(int i = 0; i < sizeof(regs); i++)
	{
	    uint8_t val = 0;

	    if(mpu_read(regs[i], &val, 1))
	    {
		printf("REG 0x%02X = 0x%02X\n",
		       regs[i],
		       val);
	    }
	}

    return true;
}



bool imu_get_angle(float *pitch, float *roll)
{
    uint8_t data[6];

    if (!mpu_read(0x3B, data, 6))
    {
        return false;
    }


    int16_t ax =
        (data[0] << 8) | data[1];

    int16_t ay =
        (data[2] << 8) | data[3];

    int16_t az =
        (data[4] << 8) | data[5];


    float accel_pitch =
        atan2f(
            ay,
            sqrtf((float)ax * ax + (float)az * az)
        )
        * 180.0f / M_PI;


    float accel_roll =
        atan2f(
            ax,
            sqrtf((float)ay * ay + (float)az * az)
        )
        * 180.0f / M_PI;


    /*
       Read gyro
    */

    uint8_t gyro_data[6];

    if (!mpu_read(0x43, gyro_data, 6))
    {
        return false;
    }


    int16_t gx =
        (gyro_data[0] << 8) |
         gyro_data[1];

    int16_t gy =
        (gyro_data[2] << 8) |
         gyro_data[3];

	float gyro_roll_rate =
	    (gx / 131.0f) - gyro_x_offset;

	float gyro_pitch_rate =
	    (gy / 131.0f) - gyro_y_offset;

    /*
       Time step
    */

    int64_t now = esp_timer_get_time();

if(last_imu_time == 0)
{
    last_imu_time = now;
    return false;
}

    float dt = (now - last_imu_time) / 1000000.0f;

    last_imu_time = now;


    // prevent startup spikes
    if (dt > 0.1f)
    {
        dt = 0.01f;
    }


    /*
       Complementary filter
    */

    const float alpha = 0.98f;


    filtered_pitch =
        alpha *
        (filtered_pitch + gyro_pitch_rate * dt)
        +
        (1.0f - alpha) *
        accel_pitch;


    filtered_roll =
        alpha *
        (filtered_roll + gyro_roll_rate * dt)
        +
        (1.0f - alpha) *
        accel_roll;



    *pitch = filtered_pitch - pitch_offset;
    *roll  = filtered_roll - roll_offset;


    return true;
}
