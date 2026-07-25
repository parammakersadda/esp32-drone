#include "wifi.h"
#include "esp_mac.h"
#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "esp_netif.h"

#include "nvs_flash.h"

#define WIFI_SSID      "DroneController"
#define WIFI_PASSWORD  "12345678"
#define WIFI_CHANNEL   1
#define MAX_CLIENTS    4

static const char *TAG = "wifi";



static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_AP_STACONNECTED:
            {
                wifi_event_ap_staconnected_t *event =
                    (wifi_event_ap_staconnected_t *)event_data;

                ESP_LOGI(
                    TAG,
                    "Station connected: "
                    MACSTR,
                    MAC2STR(event->mac));

                break;
            }

            case WIFI_EVENT_AP_STADISCONNECTED:
            {
                wifi_event_ap_stadisconnected_t *event =
                    (wifi_event_ap_stadisconnected_t *)event_data;

                ESP_LOGI(
                    TAG,
                    "Station disconnected: "
                    MACSTR,
                    MAC2STR(event->mac));

                break;
            }

            default:
                break;
        }
    }
}



void wifi_init_softap(void)
{
    /*
     * NVS
     */

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(
            nvs_flash_erase());

        ESP_ERROR_CHECK(
            nvs_flash_init());
    }


    /*
     * Network stack
     */

    ESP_ERROR_CHECK(
        esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default());


    /*
     * Create AP network interface
     */

    esp_netif_create_default_wifi_ap();


    /*
     * Initialize WiFi driver
     */

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg));


    /*
     * Register event handler
     */

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL));


    /*
     * Configure AP
     */

    wifi_config_t wifi_config =
    {
        .ap =
        {
            .ssid = WIFI_SSID,

            .ssid_len = strlen(WIFI_SSID),

            .channel = WIFI_CHANNEL,

            .password = WIFI_PASSWORD,

            .max_connection = MAX_CLIENTS,

            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };


    /*
     * Open AP if password empty
     */

    if (strlen(WIFI_PASSWORD) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }


    /*
     * Start AP
     */

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_AP,
            &wifi_config));

    ESP_ERROR_CHECK(
        esp_wifi_start());


    ESP_LOGI(
        TAG,
        "SoftAP started");

    ESP_LOGI(
        TAG,
        "SSID: %s",
        WIFI_SSID);

    ESP_LOGI(
        TAG,
        "Password: %s",
        WIFI_PASSWORD);

    ESP_LOGI(
        TAG,
        "Open http://192.168.4.1");
}