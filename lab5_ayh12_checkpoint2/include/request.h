#include <stdlib.h>

#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"

typedef struct {
    float temp;
    int humidity;
} sensor_json_t;

void post_sensor_data(sensor_json_t);