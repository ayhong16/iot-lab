#include "one_wire.h"

#include <driver/gpio.h>
#include <hal/gpio_types.h>
#include <stdio.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DHT_PIN 7

static void us_delay(int us) {
    uint32_t curr = esp_timer_get_time();
    while ((esp_timer_get_time() - curr) < us) {
        continue;
    }
}

bool check_start() {
    // Pull the GPIO pin low for at least 18ms to let DHT11 detect the start signal
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    us_delay(18000);

    // Pull the GPIO pin high and wait for DHT11 response (20-40 microseconds)
    gpio_set_level(DHT_PIN, 1);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    while (gpio_get_level(DHT_PIN) == 1) {
        continue;
    }

    while (gpio_get_level(DHT_PIN) == 0) {
        continue;
    }
    return true;
}

uint64_t read_one_bit() {
    // wait for the wire to go high
    while (gpio_get_level(DHT_PIN) == 0) {
        continue;
    }

    uint64_t start = esp_timer_get_time();

    // wait for wire to go low
    uint64_t end = 0;
    while (gpio_get_level(DHT_PIN)) {
        end = esp_timer_get_time();
        if ((end - start) > 150) {
            return 0;
        }
    }
    return end - start;
}

SensorData read_sensor_data() {
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    uint8_t data[5] = {0};
    int num_bits = 40;
    uint8_t checksum = 0;

    while (gpio_get_level(DHT_PIN)) {
        continue;
    }

    for (int i = 0; i < num_bits; i++) {
        uint64_t diff = read_one_bit();
        if (diff == 0) {
            return (SensorData){0, 0, 0, 0, 0};
        }
        int bit = diff > 40 ? 1 : 0;
        data[i / 8] |= bit << (7 - (i % 8));
    }

    for (int i = 0; i < 4; i++) {
        checksum += data[i];
    }

    // Verify checksum
    if (checksum != data[4]) {
        return (SensorData){0, 0, 0, 0, 0};
    }

    SensorData ret;
    ret.int_rh = data[0];
    ret.dec_rh = data[1];
    ret.int_temp = data[2];
    ret.dec_temp = data[3];
    ret.checksum = data[4];

    return ret;
}
