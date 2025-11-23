#ifndef HID_H
#define HID_H
#pragma once

#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "esp_system.h"
// #include "nvs_flash.h"
// #include "nvs.h"
// Removed to break circular dependency

#pragma pack(push, 1)
typedef struct {
    uint8_t buffer[10];
} aime_id_t;

typedef struct {
    union {
        char buffer[64];
        struct {
            uint8_t buttons[10];
            uint16_t lever;
            uint8_t scan;
            aime_id_t aime_id;
        };
    };
} output_data_t;

typedef uint8_t color_t[3];

typedef struct {
    uint8_t ledBrightness;
    color_t ledColors[10];
} led_t;

typedef struct {
    aime_id_t aimeId;
} option_t;

typedef struct {
    uint8_t type;
    union {
        char buffer[63];
        led_t led;
        option_t option;
    };
} input_data_t;

extern const char* hid_string_descriptor[];
extern  uint16_t g_last_input_data_len;

// 注册自定义HID报告描述符
extern const uint8_t hid_report_descriptor[];

//补充hid.c中的函数声明
void hid_init();
void Push(const output_data_t *out_data, input_data_t *input_data);
void handle_led_input(const input_data_t *input);
#endif