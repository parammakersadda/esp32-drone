#include "webserver.h"
#include "config.h"
#include "pwm.h"
#include "motor_test.h"
#include "flight_control.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "web";


/*
 * HTML page
 */

extern const unsigned char index_html_start[] asm("_binary_index_html_start");
extern const unsigned char index_html_end[] asm("_binary_index_html_end");


extern const unsigned char config_html_start[] asm("_binary_config_html_start");
extern const unsigned char config_html_end[] asm("_binary_config_html_end");

/*
 * GET /
 */

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(
        req,
        "text/html"
    );


    httpd_resp_send(
        req,
        (const char *)index_html_start,
        index_html_end - index_html_start
    );


    return ESP_OK;
}

static esp_err_t config_handler(httpd_req_t *req)
{
    httpd_resp_set_type(
        req,
        "text/html"
    );


    httpd_resp_send(
        req,
        (const char *)config_html_start,
        config_html_end - config_html_start
    );


    return ESP_OK;
}

static esp_err_t disarm_handler(httpd_req_t *req)
{
    armed = false;
    throttle = 1000;

    ESP_LOGI(
        TAG,
        "DRONE DISARMED"
    );

    httpd_resp_sendstr(
        req,
        "DISARMED"
    );

    return ESP_OK;
}
static esp_err_t arm_handler(httpd_req_t *req)
{
    char query[32];

    if(httpd_req_get_url_query_str(
            req,
            query,
            sizeof(query)) == ESP_OK)
    {
        char state[8];

        if(httpd_query_key_value(
                query,
                "state",
                state,
                sizeof(state)) == ESP_OK)
        {
            int value = atoi(state);

            if(value == 1)
            {
                armed = true;
                ESP_LOGI(TAG, "ARMED");
            }
            else
            {
                armed = false;
                throttle = 1000;
                ESP_LOGI(TAG, "DISARMED");
            }
        }
    }

    httpd_resp_sendstr(req, "OK");

    return ESP_OK;
}
/*
 * GET /throttle
 */

static bool mapping_valid(
    int fl,
    int fr,
    int rr,
    int rl
)
{
    // range check
    if(fl < 0 || fl >= LOGICAL_MOTOR_COUNT ||
       fr < 0 || fr >= LOGICAL_MOTOR_COUNT ||
       rr < 0 || rr >= LOGICAL_MOTOR_COUNT ||
       rl < 0 || rl >= LOGICAL_MOTOR_COUNT)
    {
        return false;
    }


    // duplicate check
    if(fl == fr ||
       fl == rr ||
       fl == rl ||
       fr == rr ||
       fr == rl ||
       rr == rl)
    {
        return false;
    }


    return true;
}



static esp_err_t mapping_handler(httpd_req_t *req)
{
    char query[128];

    ESP_LOGI(TAG, "MAPPING REQUEST RECEIVED");


    if(httpd_req_get_url_query_str(
            req,
            query,
            sizeof(query)) == ESP_OK)
    {
        ESP_LOGI(TAG, "QUERY: %s", query);


        char value[8];


        int fl = drone_config.motor_map[FRONT_LEFT];
        int fr = drone_config.motor_map[FRONT_RIGHT];
        int rr = drone_config.motor_map[REAR_RIGHT];
        int rl = drone_config.motor_map[REAR_LEFT];


        // Read new values into temporary variables

        if(httpd_query_key_value(
                query,
                "fl",
                value,
                sizeof(value)) == ESP_OK)
        {
            fl = atoi(value);
        }


        if(httpd_query_key_value(
                query,
                "fr",
                value,
                sizeof(value)) == ESP_OK)
        {
            fr = atoi(value);
        }


        if(httpd_query_key_value(
                query,
                "rr",
                value,
                sizeof(value)) == ESP_OK)
        {
            rr = atoi(value);
        }


        if(httpd_query_key_value(
                query,
                "rl",
                value,
                sizeof(value)) == ESP_OK)
        {
            rl = atoi(value);
        }



        // Validate BEFORE saving

        if(!mapping_valid(fl, fr, rr, rl))
        {
            ESP_LOGE(
                TAG,
                "INVALID MAPPING: duplicate PWM assignment"
            );


            httpd_resp_sendstr(
                req,
                "ERROR: Duplicate PWM mapping"
            );


            return ESP_OK;
        }



        // Copy validated mapping

        drone_config.motor_map[FRONT_LEFT]  = fl;
        drone_config.motor_map[FRONT_RIGHT] = fr;
        drone_config.motor_map[REAR_RIGHT]  = rr;
        drone_config.motor_map[REAR_LEFT]   = rl;



        ESP_LOGI(
            TAG,
            "FRONT_LEFT  -> PWM%d",
            fl
        );

        ESP_LOGI(
            TAG,
            "FRONT_RIGHT -> PWM%d",
            fr
        );

        ESP_LOGI(
            TAG,
            "REAR_RIGHT  -> PWM%d",
            rr
        );

        ESP_LOGI(
            TAG,
            "REAR_LEFT   -> PWM%d",
            rl
        );


        config_save();


        ESP_LOGI(
            TAG,
            "CONFIG SAVED"
        );
    }
    else
    {
        ESP_LOGW(
            TAG,
            "NO QUERY STRING"
        );
    }


    httpd_resp_sendstr(
        req,
        "OK"
    );


    return ESP_OK;
}

static esp_err_t motor_stop_handler(
    httpd_req_t *req
)
{
    test_motor = -1;
    test_value = 1000;


    ESP_LOGI(
        TAG,
        "Motor test stopped"
    );


    httpd_resp_sendstr(
        req,
        "stopped"
    );


    return ESP_OK;
}

static esp_err_t motor_status_handler(
    httpd_req_t *req
)
{
    char response[64];


    sprintf(
        response,
        "{\"motor\":%d,\"value\":%d}",
        test_motor,
        test_value
    );


    httpd_resp_set_type(
        req,
        "application/json"
    );


    httpd_resp_send(
        req,
        response,
        HTTPD_RESP_USE_STRLEN
    );


    return ESP_OK;
}

static esp_err_t throttle_handler(httpd_req_t *req)
{
    char query[64];

    if (httpd_req_get_url_query_str(
            req,
            query,
            sizeof(query)) == ESP_OK)
    {
        char value[16];

        if (httpd_query_key_value(
                query,
                "value",
                value,
                sizeof(value)) == ESP_OK)
        {
            throttle = atoi(value);

            ESP_LOGI(
                TAG,
                "Throttle = %d",
                throttle);
        }
    }

    httpd_resp_sendstr(req, "OK");

    return ESP_OK;
}



static esp_err_t motor_handler(httpd_req_t *req)
{
    char query[64];

    if(httpd_req_get_url_query_str(
            req,
            query,
            sizeof(query)
        ) == ESP_OK)
    {
        char motor_str[8];
        char value_str[8];


        httpd_query_key_value(
            query,
            "motor",
            motor_str,
            sizeof(motor_str)
        );


        httpd_query_key_value(
            query,
            "value",
            value_str,
            sizeof(value_str)
        );


        int motor = atoi(motor_str);
        int value = atoi(value_str);


        test_motor = motor;
        test_value = value;


        ESP_LOGI(
            TAG,
            "TEST MOTOR %d = %d",
            motor,
            value
        );
    }


    httpd_resp_sendstr(req, "OK");

    return ESP_OK;
}

static esp_err_t get_config_handler(httpd_req_t *req)
{
    char json[128];

    snprintf(
        json,
        sizeof(json),
        "{\"fl\":%d,\"fr\":%d,\"rr\":%d,\"rl\":%d}",
        drone_config.motor_map[FRONT_LEFT],
        drone_config.motor_map[FRONT_RIGHT],
        drone_config.motor_map[REAR_RIGHT],
        drone_config.motor_map[REAR_LEFT]
    );

    ESP_LOGI(
        TAG,
        "Sending config: %s",
        json
    );

    httpd_resp_set_type(
        req,
        "application/json"
    );

    httpd_resp_send(
        req,
        json,
        HTTPD_RESP_USE_STRLEN
    );

    return ESP_OK;
}

/*
 * URI definitions
 */

static httpd_uri_t root =
{
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL
};

static httpd_uri_t throttle_uri =
{
    .uri = "/throttle",
    .method = HTTP_GET,
    .handler = throttle_handler,
    .user_ctx = NULL
};

static httpd_uri_t config_uri =
{
    .uri = "/config",
    .method = HTTP_GET,
    .handler = config_handler,
    .user_ctx = NULL
};

static httpd_uri_t mapping_uri =
{
    .uri = "/mapping",
    .method = HTTP_GET,
    .handler = mapping_handler,
    .user_ctx = NULL
};

static httpd_uri_t motor_uri =
{
    .uri = "/motor",
    .method = HTTP_GET,
    .handler = motor_handler,
    .user_ctx = NULL
};

static httpd_uri_t motor_status_uri =
{
    .uri="/motor_status",
    .method=HTTP_GET,
    .handler=motor_status_handler
};


static httpd_uri_t motor_stop_uri =
{
    .uri="/motor_stop",
    .method=HTTP_GET,
    .handler=motor_stop_handler
};

httpd_uri_t config_get_uri =
{
    .uri="/get_config",
    .method=HTTP_GET,
    .handler=get_config_handler
};

static httpd_uri_t disarm_uri =
{
    .uri = "/disarm",
    .method = HTTP_GET,
    .handler = disarm_handler,
    .user_ctx = NULL
};
static httpd_uri_t arm_uri =
{
    .uri = "/arm",
    .method = HTTP_GET,
    .handler = arm_handler,
    .user_ctx = NULL
};

void webserver_start(void)
{
    httpd_config_t config =
        HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 20;

    httpd_handle_t server = NULL;

    if (httpd_start(
            &server,
            &config) == ESP_OK)
    {
        httpd_register_uri_handler(
            server,
            &root);

        httpd_register_uri_handler(
            server,
            &throttle_uri);
            
        httpd_register_uri_handler(
            server,
            &config_uri
        );
        httpd_register_uri_handler(
            server,
            &mapping_uri
        );
        
	httpd_register_uri_handler(
	    server,
	    &motor_uri
	);
	httpd_register_uri_handler(
	    server,
	    &motor_status_uri
	);


	httpd_register_uri_handler(
	    server,
	    &motor_stop_uri
	);

	httpd_register_uri_handler(
	    server,
	    &config_get_uri
	);

	httpd_register_uri_handler(
	    server,
	    &disarm_uri
	);
	httpd_register_uri_handler(
	    server,
	    &arm_uri
	);

	ESP_LOGI(
            TAG,
            "HTTP server started");
    }
}
