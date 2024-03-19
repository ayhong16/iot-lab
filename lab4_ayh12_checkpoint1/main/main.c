#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "wifi.h"

static lv_disp_t *disp_handle;

#define PERIOD 200000
#define AP_PERIOD 2000000

static wifi_scan_result_t result;
static int count;

static void ap_timer_callback(void *arg) {
    count += 1;
    if (count >= result.num_records) {
        count = 0;
    }
}

static void disp_timer_callback(void *arg) {
    static lv_obj_t *wifi_label = NULL;
    static lv_obj_t *ssid_label = NULL;
    static lv_obj_t *rssi_label = NULL;

    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char wifi_str[50];
    char ssid_str[50];
    char rssi_str[50];

    sprintf(wifi_str, "Wi-Fi network #%d", count + 1);
    sprintf(ssid_str, "SSID: %s", result.ap_records[count].ssid);
    sprintf(rssi_str, "RSSI: %d", result.ap_records[count].rssi);

    if (wifi_label == NULL) {
        wifi_label = lv_label_create(scr);
    }
    if (ssid_label == NULL) {
        ssid_label = lv_label_create(scr);
    }
    if (rssi_label == NULL) {
        rssi_label = lv_label_create(scr);
    }
    lv_label_set_text(wifi_label, wifi_str);
    lv_label_set_text(ssid_label, ssid_str);
    lv_label_set_text(rssi_label, rssi_str);

    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(rssi_label, &lv_font_montserrat_14, 0);

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(wifi_label, disp_handle->driver->hor_res);
    lv_obj_set_width(ssid_label, disp_handle->driver->hor_res);
    lv_obj_set_width(rssi_label, disp_handle->driver->hor_res);
    lv_obj_align(wifi_label, LV_ALIGN_CENTER, 0, -18);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(rssi_label, LV_ALIGN_CENTER, 0, 18);
}

void app_main(void) {
    disp_handle = oled_init();
    wifi_init();
    result = wifi_scan();
    count = result.num_records;

    // Create debounce timer at end of setup
    esp_timer_create_args_t disp_timer_args = {
        .callback = &disp_timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t disp_timer;
    esp_timer_create(&disp_timer_args, &disp_timer);
    esp_timer_start_periodic(disp_timer, PERIOD);

    esp_timer_create_args_t ap_timer_args = {
        .callback = &ap_timer_callback,
        .name = "ap_timer",
    };
    esp_timer_handle_t ap_timer;
    esp_timer_create(&ap_timer_args, &ap_timer);
    esp_timer_start_periodic(ap_timer, AP_PERIOD);
}