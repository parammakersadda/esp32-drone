idf_component_register(
    SRCS
        "main.c"
        "wifi.c"
        "webserver.c"
        "pwm.c"
        "imu.c"
        "flight_control.c"
        "config.c"

    INCLUDE_DIRS "."

    EMBED_FILES
        "www/index.html"
        "www/config.html"

    PRIV_REQUIRES
        esp_wifi
        nvs_flash
        esp_http_server
        esp_eth
        esp_driver_ledc
        esp_driver_i2c
	esp_timer
)
