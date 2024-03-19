#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "request.h"
#include "wifi.h"

#define PERIOD 200000

static lv_disp_t *disp_handle;
static weather_t weather;

static void disp_timer_callback(void *arg) {
    static lv_obj_t *temp_label = NULL;
    static lv_obj_t *humidity_label = NULL;
    static lv_obj_t *wind_label = NULL;

    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char temp_str[50];
    char humidity_str[50];
    char wind_str[50];

    sprintf(temp_str, "Temp: %.2f F", weather.temp);
    sprintf(humidity_str, "Humd: %d%%", weather.humidity);
    sprintf(wind_str, "Wind: %.2f m/s", weather.wind);

    if (temp_label == NULL) {
        temp_label = lv_label_create(scr);
    }
    if (humidity_label == NULL) {
        humidity_label = lv_label_create(scr);
    }
    if (wind_label == NULL) {
        wind_label = lv_label_create(scr);
    }
    lv_label_set_text(temp_label, temp_str);
    lv_label_set_text(humidity_label, humidity_str);
    lv_label_set_text(wind_label, wind_str);

    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(wind_label, &lv_font_montserrat_14, 0);

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(temp_label, disp_handle->driver->hor_res);
    lv_obj_set_width(humidity_label, disp_handle->driver->hor_res);
    lv_obj_set_width(wind_label, disp_handle->driver->hor_res);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -18);
    lv_obj_align(humidity_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(wind_label, LV_ALIGN_CENTER, 0, 18);
}

void app_main(void) {
    disp_handle = oled_init();
    wifi_init();
    geolocation_t geolocation = get_geolocation();
    weather = get_weather(geolocation);

    // Create debounce timer at end of setup
    esp_timer_create_args_t disp_timer_args = {
        .callback = &disp_timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t disp_timer;
    esp_timer_create(&disp_timer_args, &disp_timer);
    esp_timer_start_periodic(disp_timer, PERIOD);
}