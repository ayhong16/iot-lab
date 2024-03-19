#include "request.h"

#define TAG "request"
#define MAX_HTTP_OUTPUT_BUFFER 1024
#define API_KEY "ad3088c1c365ea148e06b7e5ae8d30be"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *)calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

geolocation_t get_geolocation() {
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
    esp_http_client_config_t config = {
        .url = "http://ip-api.com/json",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,  // Pass address of local buffer to get response
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    cJSON *root = cJSON_Parse(local_response_buffer);
    if (root == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON: %s", cJSON_GetErrorPtr());
    } else {
        cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");
        if (cJSON_IsString(status) && (status->valuestring != NULL)) {
            ESP_LOGI(TAG, "Status: %s", status->valuestring);
        } else {
            ESP_LOGE(TAG, "Failed to parse status");
        }
    }
    geolocation_t geo;
    cJSON *lat = cJSON_GetObjectItemCaseSensitive(root, "lat");
    cJSON *lon = cJSON_GetObjectItemCaseSensitive(root, "lon");
    cJSON *zip = cJSON_GetObjectItemCaseSensitive(root, "zip");
    if (cJSON_IsNumber(lat)) {
        geo.lat = (float)cJSON_GetNumberValue(lat);
    }
    if (cJSON_IsNumber(lon)) {
        geo.lon = (float)cJSON_GetNumberValue(lon);
    }
    if (cJSON_IsString(zip)) {
        char *str_zip = cJSON_GetStringValue(zip);
        geo.zip = atoi(str_zip);
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    return geo;
}

weather_t get_weather(geolocation_t geolocation) {
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    char query[256];
    sprintf(query, "lat=%f&lon=%f&appid=%s", geolocation.lat, geolocation.lon, API_KEY);

    esp_http_client_config_t config = {
        .host = "api.openweathermap.org",
        .path = "/data/2.5/weather",
        .query = query,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,  // Pass address of local buffer to get response
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    cJSON *root = cJSON_Parse(local_response_buffer);
    if (root == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON: %s", cJSON_GetErrorPtr());
    }

    weather_t weather;
    cJSON *main = cJSON_GetObjectItemCaseSensitive(root, "main");
    cJSON *wind = cJSON_GetObjectItemCaseSensitive(root, "wind");

    cJSON *temp = cJSON_GetObjectItemCaseSensitive(main, "temp");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(main, "humidity");
    cJSON *speed = cJSON_GetObjectItemCaseSensitive(wind, "speed");

    if (cJSON_IsObject(main) && cJSON_IsObject(wind)) {
        if (cJSON_IsNumber(temp)) {
            weather.temp = ((float)cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(main, "temp")) - 273.15) * 9.0 / 5.0 + 32.0;
        }
        if (cJSON_IsNumber(humidity)) {
            weather.humidity = (int)cJSON_GetNumberValue(humidity);
        }
        if (cJSON_IsNumber(speed)) {
            weather.wind = (float)cJSON_GetNumberValue(speed);
        }
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    return weather;
}
