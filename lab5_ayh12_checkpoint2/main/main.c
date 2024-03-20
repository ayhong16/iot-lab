#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "one_wire.h"
#include "request.h"
#include "wifi.h"

#define PERIOD 200000
#define SECOND 1000000

static lv_disp_t *disp_handle;
static sensor_json_t weather;

static void sensor_data_callback(void *arg) {
    if (check_start()) {
        SensorData data = read_sensor_data();
        if (data.checksum == 0 && data.int_rh == 0 && data.int_temp == 0 && data.dec_temp == 0 && data.dec_rh == 0) {
            return;
        }
        weather.humidity = (data.int_rh + ((float)data.dec_rh / 10.0));
        weather.temp = (data.int_temp + ((float)data.dec_temp / 10.0));
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

    sprintf(temp_str, "Temp: %.2fC", weather.temp);
    sprintf(humidity_str, "RH: %.2f%%", weather.humidity);

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

    // Create debounce timer at end of setup
    esp_timer_create_args_t disp_timer_args = {
        .callback = &disp_timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t disp_timer;
    esp_timer_create(&disp_timer_args, &disp_timer);
    esp_timer_start_periodic(disp_timer, SECOND);

    // Get sensor data every second
    esp_timer_create_args_t post_timer_args = {
        .callback = &post_callback,
        .name = "post_timer",
    };
    esp_timer_handle_t post_timer;
    esp_timer_create(&post_timer_args, &post_timer);
    esp_timer_start_periodic(post_timer, SECOND);

    esp_timer_create_args_t sensor_timer_args = {
        .callback = &sensor_data_callback,
        .name = "sensor_timer",
    };
    esp_timer_handle_t sensor_timer;
    esp_timer_create(&sensor_timer_args, &sensor_timer);
    esp_timer_start_periodic(sensor_timer, PERIOD);
}