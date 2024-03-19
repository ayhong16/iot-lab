#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_vendor.h"

static lv_disp_t * disp_handle;

#define I2C_HOST 0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (500 * 1000)
#define EXAMPLE_PIN_NUM_SDA 17
#define EXAMPLE_PIN_NUM_SCL 18
#define EXAMPLE_PIN_NUM_RST 21
#define EXAMPLE_I2C_HW_ADDR 0x3C
#define DISP_WIDTH 128
#define DISP_HEIGHT 64
#define EXAMPLE_LCD_CMD_BITS 8

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_t *disp_handle = (lv_disp_t *) user_ctx;
    lvgl_port_flush_ready(disp_handle);
    return false;
}

void app_main(void)
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER, // I2C LCD is a master node
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1, // refer to LCD spec
        .dc_bit_offset = 6, // refer to LCD spec
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = DISP_WIDTH * DISP_HEIGHT,
        .double_buffer = true,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
        .monochrome = true,
        /* Rotation values must be same as used in esp_lcd for initial settings of
        the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        },
    };
    disp_handle = lvgl_port_add_disp(&disp_cfg);

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp_handle);

    lv_disp_set_rotation(disp_handle, LV_DISP_ROT_NONE);

    char str[5] = "ayh12";

    while (1) {

        lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);
        lv_obj_clean(scr);

        lv_obj_t *label = lv_label_create(scr);
        lv_obj_t *label2 = lv_label_create(scr);
        lv_obj_t *label3 = lv_label_create(scr);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        lv_label_set_long_mode(label3, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        lv_label_set_text(label, str);
        lv_label_set_text(label2, str);
        lv_label_set_text(label3, str);

        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_font(label2, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_font(label3, &lv_font_montserrat_24, 0);

        /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
        lv_obj_set_width(label, disp_handle->driver->hor_res);
        lv_obj_set_width(label2, disp_handle->driver->hor_res);
        lv_obj_set_width(label3, disp_handle->driver->hor_res);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(label3, LV_ALIGN_BOTTOM_MID, 0, 0);


        vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
}




