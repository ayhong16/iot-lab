#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define LED_PIN 35

void app_main() {
// Configure the LED pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    while (1) {
        for (int i = 0; i < 3; i++) {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        for (int i = 0; i < 3; i++) {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        for (int i = 0; i < 3; i++) {
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

