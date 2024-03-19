#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#define BUTTON_PIN 48
#define SPEAKER_PIN 19
#define LED_PIN 7
#define DEBOUNCE_DELAY 25000  // 25 ms

static int periods[4] = {0, 500000, 250000, 125000};  // in us, translates 0, 1, 2, 4 Hz
static int current_period = 0;
static bool speaker_state = false;
static esp_timer_handle_t led_timer;
static volatile bool button_pressed = false;
static volatile bool button_depressed = false;
int led_val = 0;

static void button_isr(void *arg) {
    button_pressed = true;
    if (gpio_get_level(BUTTON_PIN) == 0) {
        button_depressed = true;
    }
}

static void led_timer_callback(void *arg) {
    led_val = !led_val;
    gpio_set_level(LED_PIN, led_val);
}

static void debounce_timer_callback(void *arg) {
    if (button_pressed) {
        speaker_state = !speaker_state;
        gpio_set_level(SPEAKER_PIN, speaker_state);
        button_pressed = false;
    }
    if (button_depressed) {
        // Create LED timer
        if (led_timer) {
            esp_timer_stop(led_timer);
        }
        current_period = (current_period + 1) % 4;
        if (periods[current_period] > 0) {
            esp_timer_start_periodic(led_timer, periods[current_period]);
        } else {
            gpio_set_level(LED_PIN, 0);
        }
        button_depressed = false;
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
    gpio_config_t io_conf1 = (gpio_config_t){
        .pin_bit_mask = (1ULL << SPEAKER_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf1);

    // Configure LED pin
    gpio_config_t io_conf2 = (gpio_config_t){
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf2);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *)BUTTON_PIN);

    // Create debounce timer
    esp_timer_create_args_t debounce_timer_args = {
        .callback = &debounce_timer_callback,
        .name = "debounce_timer",
    };
    esp_timer_handle_t debounce_timer;
    esp_timer_create(&debounce_timer_args, &debounce_timer);
    esp_timer_start_periodic(debounce_timer, DEBOUNCE_DELAY);

    esp_timer_create_args_t led_timer_args = {
        .callback = &led_timer_callback,
        .name = "led_timer",
    };
    esp_timer_create(&led_timer_args, &led_timer);
}