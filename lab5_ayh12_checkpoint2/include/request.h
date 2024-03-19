#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"

typedef struct {
    int zip;
    float lat;
    float lon;
} geolocation_t;

typedef struct {
    float temp;
    int humidity;
    float wind;
} weather_t;

geolocation_t get_geolocation();
weather_t get_weather(geolocation_t);
