#include <driver/gpio.h>
#include <hal/gpio_types.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    uint8_t int_rh;
    uint8_t dec_rh;
    uint8_t int_temp;
    uint8_t dec_temp;
    uint8_t checksum;
    bool error;
} SensorData;

// bool check_start();
SensorData read_sensor_data();