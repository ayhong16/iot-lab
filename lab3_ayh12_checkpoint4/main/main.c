#include <stdio.h>

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu.h"
#include "oled.h"

static lv_disp_t *disp_handle;
static mpu6050_acce_value_t acce;
static mpu6050_gyro_value_t gyro;

#define BUTTON_PIN 19
#define PERIOD 200000

static void read_task(void *arg) {
    acce = mpu_read_acce();
    gyro = mpu_read_gyro();
    vTaskDelete(NULL);
}

static void timer_callback(void *arg) {
    static lv_obj_t *x_label = NULL;
    static lv_obj_t *y_label = NULL;
    static lv_obj_t *z_label = NULL;

    xTaskCreate(&read_task, "read_task", 2048, NULL, 5, NULL);
    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char x_str[50];
    char y_str[50];
    char z_str[50];

    sprintf(x_str, "Xa %.1f, Xg %.1f", acce.acce_x, gyro.gyro_x);
    sprintf(y_str, "Ya %.1f, Yg %.1f", acce.acce_y, gyro.gyro_y);
    sprintf(z_str, "Za %.1f, Zg %.1f", acce.acce_z, gyro.gyro_z);

    if (x_label == NULL) {
        x_label = lv_label_create(scr);
    }
    if (y_label == NULL) {
        y_label = lv_label_create(scr);
    }
    if (z_label == NULL) {
        z_label = lv_label_create(scr);
    }
    lv_label_set_text(x_label, x_str);
    lv_label_set_text(y_label, y_str);
    lv_label_set_text(z_label, z_str);

    lv_obj_set_style_text_font(x_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(y_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(z_label, &lv_font_montserrat_14, 0);

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(x_label, disp_handle->driver->hor_res);
    lv_obj_set_width(y_label, disp_handle->driver->hor_res);
    lv_obj_set_width(z_label, disp_handle->driver->hor_res);
    lv_obj_align(x_label, LV_ALIGN_CENTER, 0, -18);
    lv_obj_align(y_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(z_label, LV_ALIGN_CENTER, 0, 18);
}

void app_main(void) {
    disp_handle = oled_init();
    mpu_setup();

    // Create debounce timer at end of setup
    esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, PERIOD);
}