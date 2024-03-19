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
static geolocation_t geolocation;

static void disp_timer_callback(void *arg) {
    static lv_obj_t *zip_label = NULL;
    static lv_obj_t *lat_label = NULL;
    static lv_obj_t *lon_label = NULL;

    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char zip_str[50];
    char lat_str[50];
    char lon_str[50];

    sprintf(zip_str, "Zip: %d", geolocation.zip);
    sprintf(lat_str, "Lat: %.2f", geolocation.lat);
    sprintf(lon_str, "Lon: %.2f", geolocation.lon);

    if (zip_label == NULL) {
        zip_label = lv_label_create(scr);
    }
    if (lat_label == NULL) {
        lat_label = lv_label_create(scr);
    }
    if (lon_label == NULL) {
        lon_label = lv_label_create(scr);
    }
    lv_label_set_text(zip_label, zip_str);
    lv_label_set_text(lat_label, lat_str);
    lv_label_set_text(lon_label, lon_str);

    lv_obj_set_style_text_font(zip_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(lat_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(lon_label, &lv_font_montserrat_14, 0);

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(zip_label, disp_handle->driver->hor_res);
    lv_obj_set_width(lat_label, disp_handle->driver->hor_res);
    lv_obj_set_width(lon_label, disp_handle->driver->hor_res);
    lv_obj_align(zip_label, LV_ALIGN_CENTER, 0, -18);
    lv_obj_align(lat_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(lon_label, LV_ALIGN_CENTER, 0, 18);
}

void app_main(void) {
    disp_handle = oled_init();
    wifi_init();
    geolocation = get_http();

    // Create debounce timer at end of setup
    esp_timer_create_args_t disp_timer_args = {
        .callback = &disp_timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t disp_timer;
    esp_timer_create(&disp_timer_args, &disp_timer);
    esp_timer_start_periodic(disp_timer, PERIOD);
}