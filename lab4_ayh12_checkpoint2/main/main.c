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

static float delay = 0.0;
static int packets = 0;
static int bytes = 0;
static char *status = "Success";

static void disp_timer_callback(void *arg) {
    static lv_obj_t *status_label = NULL;
    static lv_obj_t *delay_label = NULL;
    static lv_obj_t *data_label = NULL;

    lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

    char status_str[50];
    char delay_str[50];
    char data_str[50];

    sprintf(status_str, "Status: %s", status);
    sprintf(delay_str, "Delay: %.1f ms", delay);
    sprintf(data_str, "%d Pkt (%d Bytes)", packets, bytes);

    if (status_label == NULL) {
        status_label = lv_label_create(scr);
    }
    if (delay_label == NULL) {
        delay_label = lv_label_create(scr);
    }
    if (data_label == NULL) {
        data_label = lv_label_create(scr);
    }
    lv_label_set_text(status_label, status_str);
    lv_label_set_text(delay_label, delay_str);
    lv_label_set_text(data_label, data_str);

    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(delay_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_font(data_label, &lv_font_montserrat_14, 0);

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(status_label, disp_handle->driver->hor_res);
    lv_obj_set_width(delay_label, disp_handle->driver->hor_res);
    lv_obj_set_width(data_label, disp_handle->driver->hor_res);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -18);
    lv_obj_align(delay_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(data_label, LV_ALIGN_CENTER, 0, 18);
}

void on_ping_success(esp_ping_handle_t hdl, void *args) {
    uint32_t elapsed_time, recv_len;
    uint32_t received;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &packets, sizeof(packets));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    status = received > 0 ? "Success" : "Fail";
    bytes += recv_len;
    delay = (delay * (packets - 1) + elapsed_time) / packets;
}

void send_ping() {
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));
    getaddrinfo("www.duke.com", NULL, &hint, &res);
    struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    freeaddrinfo(res);

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.count = ESP_PING_COUNT_INFINITE;  // Ping once

    esp_ping_callbacks_t callback_config = {
        .on_ping_success = on_ping_success,
    };

    esp_ping_handle_t ping;
    esp_ping_new_session(&ping_config, &callback_config, &ping);
    esp_ping_start(ping);
}

void app_main(void) {
    disp_handle = oled_init();
    wifi_init();
    send_ping();
    // Create debounce timer at end of setup
    esp_timer_create_args_t disp_timer_args = {
        .callback = &disp_timer_callback,
        .name = "timer",
    };
    esp_timer_handle_t disp_timer;
    esp_timer_create(&disp_timer_args, &disp_timer);
    esp_timer_start_periodic(disp_timer, PERIOD);
}