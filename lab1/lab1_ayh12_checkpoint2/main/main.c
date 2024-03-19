#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_timer.h>

#define BUTTON_PIN 48
#define SPEAKER_PIN 19
#define DEBOUNCE_DELAY 25000 // 25ms

static volatile bool button_pressed = false;

static void button_isr(void *arg) {
    button_pressed = true;
}

static void debounce_timer_callback(void* arg) {
    static bool speaker_state = false;
    
    if (button_pressed) {
        speaker_state = !speaker_state;
        gpio_set_level(SPEAKER_PIN, speaker_state);
        button_pressed = false;
    }
}

void app_main() {
    // Configure the button pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    // Configure the speaker pin
    gpio_config_t io_conf1 = (gpio_config_t) {
        .pin_bit_mask = (1ULL << SPEAKER_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf1);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *) BUTTON_PIN);

    // Create debounce timer
    esp_timer_create_args_t debounce_timer_args = {
        .callback = &debounce_timer_callback,
        .name = "debounce_timer",
    };
    esp_timer_handle_t debounce_timer;
    esp_timer_create(&debounce_timer_args, &debounce_timer);
    esp_timer_start_periodic(debounce_timer, DEBOUNCE_DELAY);
}