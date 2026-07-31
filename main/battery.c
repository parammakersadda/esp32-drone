#include "battery.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

#define TAG "battery"

#define ADC_CALIBRATION_FACTOR 1.0734f

#define BATTERY_GPIO ADC_CHANNEL_4   // GPIO32 on classic ESP32


// Your measured resistors
#define R_TOP     46520.0f
#define R_BOTTOM   9740.0f

#define DIVIDER_RATIO ((R_TOP + R_BOTTOM) / R_BOTTOM)


static adc_oneshot_unit_handle_t adc_handle;


static int battery_cells = 2;


void battery_init(void)
{
    printf("========== BATTERY INIT CALLED ==========\n");

    ESP_LOGI(TAG, "Starting battery init");

    adc_oneshot_unit_init_cfg_t init_config =
    {
        .unit_id = ADC_UNIT_1,
    };

    ESP_LOGI(TAG, "Creating ADC unit");

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &init_config,
            &adc_handle
        )
    );

    ESP_LOGI(TAG, "Configuring ADC channel");

    adc_oneshot_chan_cfg_t config =
    {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            BATTERY_GPIO,
            &config
        )
    );

    ESP_LOGI(TAG, "Battery ADC initialized");
}

float battery_get_voltage(void)
{
    int raw_sum = 0;

    // averaging reduces ADC noise
    for(int i = 0; i < 32; i++)
    {
        int raw;

        adc_oneshot_read(
            adc_handle,
            BATTERY_GPIO,
            &raw
        );
    ESP_LOGI("battery", "ADC raw = %d", raw);
        raw_sum += raw;

        vTaskDelay(
            pdMS_TO_TICKS(2)
        );
    }


	float raw =
	    raw_sum / 32.0f;

	float adc_voltage =
	    (raw / 4095.0f) * 3.3f;

	float battery_voltage =
	    adc_voltage * DIVIDER_RATIO * ADC_CALIBRATION_FACTOR;

	ESP_LOGI(
	    TAG,
	    "Raw: %.1f  ADC: %.3f V  Battery: %.3f V",
	    raw,
	    adc_voltage,
	    battery_voltage
	);

	return battery_voltage;
}



float battery_get_cell_voltage(void)
{
    return battery_get_voltage() /
           battery_cells;
}



int battery_get_percentage(void)
{
    float cell =
        battery_get_cell_voltage();


    /*
       Generic Li-ion/LiPo estimate:

       4.2V = full
       3.3V = empty
    */


    int percent =
        (cell - 3.3f) /
        (4.2f - 3.3f) *
        100;


    if(percent > 100)
        percent = 100;


    if(percent < 0)
        percent = 0;


    return percent;
}



void battery_set_cells(int cells)
{
    battery_cells = cells;
}
