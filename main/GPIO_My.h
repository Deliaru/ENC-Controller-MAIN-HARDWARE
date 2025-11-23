#ifndef GPIO_MY_H
#define GPIO_MY_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
// #include "hid.h"  // 移除循环依赖，使用winusb_new.h中的定义
#include "winusb_new.h"
#include <nvs_flash.h>
#include <nvs.h>

// GPIO number definitions
#define BTN_L1 GPIO_NUM_4 //switch
#define BTN_L2 GPIO_NUM_3
#define BTN_L3 GPIO_NUM_2
#define BTN_LS GPIO_NUM_5

#define BTN_R1 GPIO_NUM_6
#define BTN_R2 GPIO_NUM_7
#define BTN_R3 GPIO_NUM_8
#define BTN_RS GPIO_NUM_9

#define Key_1 GPIO_NUM_12
#define BTN_LM GPIO_NUM_10
#define BTN_RM GPIO_NUM_11

#define UART_TX GPIO_NUM_17
#define UART_RX GPIO_NUM_18

#define LED_1 GPIO_NUM_48
#define LED_2 GPIO_NUM_47
#define LED_3 GPIO_NUM_45
#define LED_4 GPIO_NUM_42
#define LED_5 GPIO_NUM_41
#define LED_6 GPIO_NUM_40
#define LED_7 GPIO_NUM_39
#define LED_8 GPIO_NUM_38

#define ANGLE_FILTER_SIZE 5


// Function
void gpio_my_init_input_pullup(gpio_num_t gpio_num);
void BTN_init(void);
void Ang_init(void);
void Update(output_data_t *out_data);


#endif /* GPIO_MY_H */