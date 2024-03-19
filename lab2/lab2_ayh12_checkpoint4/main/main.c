#include <driver/adc.h>
#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
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
static adc_oneshot_unit_handle_t adc_handle;
static volatile bool button_pressed = false;
static float fahrenheit = 0.0;
static int alignments[3] = {LV_ALIGN_TOP_MID, LV_ALIGN_CENTER, LV_ALIGN_BOTTOM_MID};
static char prev_temps[3][30] = {"", "", ""};
static int count = 0;
static esp_timer_handle_t temp_timer;
static lv_obj_t *scr;

#define I2C_HOST 0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (500 * 1000)
#define DEBOUNCE_DELAY 50000

#define EXAMPLE_PIN_NUM_SDA 17
#define EXAMPLE_PIN_NUM_SCL 18
#define EXAMPLE_PIN_NUM_RST 21
#define BUTTON_PIN 5

#define EXAMPLE_I2C_HW_ADDR 0x3C
#define DISP_WIDTH 128
#define DISP_HEIGHT 64
#define EXAMPLE_LCD_CMD_BITS 8

// ADC defs
#define ADC_CHL 6
#define VMAX 3.3
#define DMAX 4095

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_t *disp_handle = (lv_disp_t *) user_ctx;
    lvgl_port_flush_ready(disp_handle);
    return false;
}

static void debounce_timer_callback(void* arg) {
    if (button_pressed) {
        button_pressed = false;
        for (int i = 0; i < 3; i++) {
            strcpy(prev_temps[i], "");
        }
        count = 0;
        lv_obj_clean(scr);
    }
}

static void temp_timer_callback(void* arg) {
    int reading = 0;
    count += 1;
    adc_oneshot_read(adc_handle, ADC_CHL, &reading);
    float voltage = (float)reading * VMAX / DMAX;
    float celsius = (voltage - 0.5) * 100;
    fahrenheit = (celsius * 9 / 5) + 32;

    char str[30];
    sprintf(str, "Temp #%d: %.2f °F", count, fahrenheit);
    for (int i = 1; i < 3; i++) {
        strcpy(prev_temps[i - 1], prev_temps[i]);
    }
    strcpy(prev_temps[2], str);

    scr = lv_disp_get_scr_act(disp_handle);
    lv_obj_clean(scr);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_text(label, prev_temps[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_set_width(label, disp_handle->driver->hor_res);
        lv_obj_align(label, alignments[i], 0, 0);
    }
}

static void button_isr(void *arg) {
    static uint64_t prev_press = 0;
    uint64_t curr_press = esp_timer_get_time();
    if (curr_press - prev_press > DEBOUNCE_DELAY)
    {
        button_pressed = true;
        prev_press = curr_press;
    }
}

void timer_setup() {
    // Create debounce timer
    esp_timer_create_args_t debounce_timer_args = {
        .callback = &debounce_timer_callback,
        .name = "debounce_timer",
    };
    esp_timer_handle_t debounce_timer;
    esp_timer_create(&debounce_timer_args, &debounce_timer);
    esp_timer_start_periodic(debounce_timer, DEBOUNCE_DELAY);

    esp_timer_create_args_t temp_timer_args = {
        .callback = &temp_timer_callback,
        .name = "temp_timer",
    };
    esp_timer_create(&temp_timer_args, &temp_timer);
    esp_timer_start_periodic(temp_timer, 1000000);
}

static void setup() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *) BUTTON_PIN);

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

    adc_oneshot_unit_init_cfg_t init_config1 = {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
      .bitwidth = 12,
      .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHL, &config));

    timer_setup();
}

void app_main(void) {
    setup();
}
