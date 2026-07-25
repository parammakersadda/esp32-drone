#include "config.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"

drone_config_t drone_config =
{
	.motor_map =
	{
	    [FRONT_LEFT]  = 0,
	    [FRONT_RIGHT] = 1,
	    [REAR_RIGHT]  = 2,
	    [REAR_LEFT]   = 3,
	},

	.trim =
	{
	    [FRONT_LEFT]  = 0,
	    [FRONT_RIGHT] = 0,
	    [REAR_RIGHT]  = 0,
	    [REAR_LEFT]   = 0,
	},
};

void config_load(void)
{
    nvs_handle_t nvs;

    if(
        nvs_open(
            "drone",
            NVS_READWRITE,
            &nvs
        ) != ESP_OK
    )
    {
        return;
    }


    size_t size =
        sizeof(drone_config);

	esp_err_t err = nvs_get_blob(
	    nvs,
	    "config",
	    &drone_config,
	    &size
	);


	if(err == ESP_OK)
	{
	    ESP_LOGI(
		"CONFIG",
		"Loaded from flash"
	    );
	}
	else
	{
	    ESP_LOGW(
		"CONFIG",
		"No saved config: %s",
		esp_err_to_name(err)
	    );
	}

    ESP_LOGI(
	    "CONFIG",
	    "MAP FL=%d FR=%d RR=%d RL=%d",
	    drone_config.motor_map[FRONT_LEFT],
	    drone_config.motor_map[FRONT_RIGHT],
	    drone_config.motor_map[REAR_RIGHT],
	    drone_config.motor_map[REAR_LEFT]
    );	

    nvs_close(nvs);
}



void config_save(void)
{
    nvs_handle_t nvs;

    esp_err_t err;


    err = nvs_open(
        "drone",
        NVS_READWRITE,
        &nvs
    );

    if(err != ESP_OK)
    {
        ESP_LOGE("CONFIG",
            "NVS open failed: %s",
            esp_err_to_name(err));
        return;
    }


    err = nvs_set_blob(
        nvs,
        "config",
        &drone_config,
        sizeof(drone_config)
    );

    if(err != ESP_OK)
    {
        ESP_LOGE("CONFIG",
            "NVS set failed: %s",
            esp_err_to_name(err));
    }


    err = nvs_commit(nvs);

    if(err != ESP_OK)
    {
        ESP_LOGE("CONFIG",
            "NVS commit failed: %s",
            esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI("CONFIG",
            "NVS commit OK");
    }


    nvs_close(nvs);
}
