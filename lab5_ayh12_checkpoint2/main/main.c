#include <stdio.h>

#include "dht11.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "request.h"
#include "wifi.h"

#define POST_PERIOD 30000000
#define SAMPLING_PERIOD 2600000

static lv_disp_t *disp_handle;
static sensor_json_t weather;

static void sensor_data_callback(void *arg) {
    dht11_reading data = DHT11_read();
    printf("Temp: %.2f, Humudity %.d, Status: %d\n", data.temperature, data.humidity, data.status);

    if (data.status == DHT11_OK) {
        weather.humidity = data.humidity;
        weather.temp = data.temperature;
    } else {
        printf("Invalid data. Don't update weather\n");
    }
}

static void post_callback(void *arg) {
    post_sensor_data(weather);
}

static void disp_timer_callback(void *arg) {
    static lv_obj_t *temp_label = NULL;
    static lv_obj_t *humidity_label = NULL;

    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char temp_str[50];
    char humidity_str[50];

    sprintf(temp_str, "Temp: %.1fC", weather.temp);
    sprintf(humidity_str, "RH: %d%%", weather.humidity);

    if (temp_label == NULL) {
        temp_label = lv_label_create(scr);
    }
    if (humidity_label == NULL) {
        humidity_label = lv_label_create(scr);
    }
    lv_label_set_text(temp_label, temp_str);
    lv_label_set_text(humidity_label, humidity_str);

    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_14, 0);

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(temp_label, disp_handle->driver->hor_res);
    lv_obj_set_width(humidity_label, disp_handle->driver->hor_res);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -15);
    lv_obj_align(humidity_label, LV_ALIGN_CENTER, 0, 15);
}

void app_main(void) {
    disp_handle = oled_init();
    wifi_init();
    DHT11_init(GPIO_NUM_7);

    // Create debounce timer at end of setup
    esp_timer_create_args_t disp_timer_args = {
        .callback = &disp_timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t disp_timer;
    esp_timer_create(&disp_timer_args, &disp_timer);
    esp_timer_start_periodic(disp_timer, POST_PERIOD);

    // Get sensor data every second
    esp_timer_create_args_t post_timer_args = {
        .callback = &post_callback,
        .name = "post_timer",
    };
    esp_timer_handle_t post_timer;
    esp_timer_create(&post_timer_args, &post_timer);
    esp_timer_start_periodic(post_timer, POST_PERIOD);

    esp_timer_create_args_t sensor_timer_args = {
        .callback = &sensor_data_callback,
        .name = "sensor_timer",
    };
    esp_timer_handle_t sensor_timer;
    esp_timer_create(&sensor_timer_args, &sensor_timer);
    esp_timer_start_periodic(sensor_timer, SAMPLING_PERIOD);
}