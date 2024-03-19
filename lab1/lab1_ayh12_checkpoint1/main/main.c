#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

#define LED_PIN 35
#define EXTERNAL_LED_PIN 48
#define LONG_PERIOD 1000000
#define SHORT_PERIOD 500000

int led_val = 0;
int external_val = 0;

static void interrupt_handler(void* arg) {
    int* timer_id = (int*) arg;
    if (*timer_id > 0) {
        external_val = !external_val;
        gpio_set_level(EXTERNAL_LED_PIN, external_val);
    } else {
        led_val = !led_val;
        gpio_set_level(LED_PIN, led_val);
    }
}
void app_main() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    gpio_config_t io_conf1 = {
        .pin_bit_mask = (1ULL << EXTERNAL_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf1);


    int timer0_id = 0;
    int timer1_id = 1;

    esp_timer_create_args_t timer0_args = {
        .callback = &interrupt_handler,
        .name = "timer0",
        .arg = &timer0_id,
    };
    esp_timer_handle_t timer0;
    esp_timer_create(&timer0_args, &timer0);
    esp_timer_start_periodic(timer0, SHORT_PERIOD);

    esp_timer_create_args_t timer1_args = {
        .callback = &interrupt_handler,
        .name = "timer1",
        .arg = &timer1_id
    };
    esp_timer_handle_t timer1;
    esp_timer_create(&timer1_args, &timer1);
    esp_timer_start_periodic(timer1, LONG_PERIOD);
}

