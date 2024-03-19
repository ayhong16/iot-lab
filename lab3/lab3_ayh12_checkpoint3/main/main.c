#include <stdio.h>

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "mpu6050.h"

static lv_disp_t *disp_handle;
static volatile bool button_pressed = false;
static mpu6050_acce_value_t acce;
static mpu6050_gyro_value_t gyro;
static float xa = 0.0;
static float xg = 0.0;
static float ya = 0.0;
static float yg = 0.0;
static float za = 0.0;
static float zg = 0.0;

// OLED defines
#define I2C_HOST 0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (500 * 1000)
#define DEBOUNCE_DELAY 50000

#define EXAMPLE_PIN_NUM_SDA 17
#define EXAMPLE_PIN_NUM_SCL 18
#define EXAMPLE_PIN_NUM_RST 21

#define EXAMPLE_I2C_HW_ADDR 0x3C
#define DISP_WIDTH 128
#define DISP_HEIGHT 64
#define EXAMPLE_LCD_CMD_BITS 8

// Button and IMU defines
#define BUTTON_PIN 19
#define SCL_PIN 20
#define SDA_PIN 26
static mpu6050_handle_t mpu_dev = NULL;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_t *disp_handle = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp_handle);
    return false;
}

static void debounce_timer_callback(void *arg) {
    static lv_obj_t *x_label = NULL;
    static lv_obj_t *y_label = NULL;
    static lv_obj_t *z_label = NULL;
    if (button_pressed) {
        button_pressed = false;
        printf("Button pressed\n");
        // simulate data:
        xa += 0.1;
        xg += 0.1;
        ya += 0.1;
        yg += 0.1;
        za += 0.1;
        zg += 0.1;
    }
    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char x_str[50];
    char y_str[50];
    char z_str[50];

    sprintf(x_str, "Xa %.1f, Xg %.1f", xa, xg);
    sprintf(y_str, "Ya %.1f, Yg %.1f", ya, yg);
    sprintf(z_str, "Za %.1f, Zg %.1f", za, zg);

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
    lv_obj_align(x_label, LV_ALIGN_CENTER, 0, -15);
    lv_obj_align(y_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(z_label, LV_ALIGN_CENTER, 0, 15);
}

static void button_isr(void *arg) {
    static uint64_t prev_press = 0;
    uint64_t curr_press = esp_timer_get_time();
    if (curr_press - prev_press > DEBOUNCE_DELAY) {
        button_pressed = true;
        prev_press = curr_press;
    }
}

stastic void mpu_setup() {
    mpu_dev = mpu6050_create(BSP_I2C_NUM, MPU6050_I2C_ADDRESS);
    mpu6050_config(mpu6050_dev, ACCE_FS_4G, GYRO_FS_500DPS);
    mpu6050_wake_up(mpu6050_dev);
}

static void
setup() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *)BUTTON_PIN);

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,  // I2C LCD is a master node
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
        .control_phase_bytes = 1,  // refer to LCD spec
        .dc_bit_offset = 6,        // refer to LCD spec
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
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

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

    // Create debounce timer at end of setup
    esp_timer_create_args_t debounce_timer_args = {
        .callback = &debounce_timer_callback,
        .name = "debounce_timer",
    };
    esp_timer_handle_t debounce_timer;
    esp_timer_create(&debounce_timer_args, &debounce_timer);
    esp_timer_start_periodic(debounce_timer, DEBOUNCE_DELAY);
}

void app_main(void) {
    setup();
}