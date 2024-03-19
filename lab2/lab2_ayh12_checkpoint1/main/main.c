#include <driver/gpio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#define BLUE_LED 7
#define GREEN_LED 19
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT
#define MAX_DUTY (1 << LEDC_DUTY_RES) - 1
#define LEDC_FREQUENCY 1000

void app_main() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLUE_LED),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    gpio_config_t io_conf1 = {
        .pin_bit_mask = (1ULL << GREEN_LED),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf1);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = LEDC_FREQUENCY,
        .speed_mode = LEDC_LOW_SPEED_MODE,  // Use low-speed mode
        .timer_num = LEDC_TIMER_0           // Use timer 0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t blue_channel = {
        .gpio_num = BLUE_LED,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&blue_channel);

    ledc_channel_config_t green_channel = {
        .gpio_num = GREEN_LED,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ledc_channel_config(&green_channel);

    int duty_level0 = 0;
    int duty_level1 = 0;
    bool increasing = true;

    while (1) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_level0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_level1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

        if (increasing) {
            duty_level0 += .02 * MAX_DUTY;
            if (duty_level0 >= MAX_DUTY) {
                duty_level0 = MAX_DUTY;
                increasing = false;
            }
            duty_level1 -= .02 * MAX_DUTY;
            if (duty_level1 <= .02 * MAX_DUTY) {
                duty_level1 = .02 * MAX_DUTY;
            }
        } else {
            duty_level0 -= .02 * MAX_DUTY;
            if (duty_level0 <= .02 * MAX_DUTY) {
                duty_level0 = .02 * MAX_DUTY;
                increasing = true;
            }
            duty_level1 += .02 * MAX_DUTY;
            if (duty_level1 >= MAX_DUTY) {
                duty_level1 = MAX_DUTY;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
