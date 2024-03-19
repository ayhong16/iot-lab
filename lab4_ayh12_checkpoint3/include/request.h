#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"

typedef struct {
    int zip;
    float lat;
    float lon;
} geolocation_t;

geolocation_t get_http();
