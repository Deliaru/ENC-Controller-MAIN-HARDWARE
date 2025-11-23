#ifndef LED_MY_H
#define LED_MY_H
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
// #include "hid.h"  // 移除循环依赖
#include "GPIO_My.h"

extern led_strip_handle_t led_1;
extern led_strip_handle_t led_2;
extern led_strip_handle_t led_3;
extern led_strip_handle_t led_4;
extern led_strip_handle_t led_5;
extern led_strip_handle_t led_6;
extern led_strip_handle_t led_7;
extern led_strip_handle_t led_8;

extern int8_t *nvs_value[8];


led_strip_handle_t configure_led(int BLINK_GPIO, int MAX_LEDS);
void LED_init(void);

// led_strip_set_pixel (led_strip, 0, i, 0, 0); 
//  作用: 设置 LED 灯条上指定索引位置的像素颜色为 RGB 格式。
//  参数:
//  strip: LED 灯条的句柄。
//  index: 要设置颜色的像素索引。
//  red, green, blue: 颜色的红、绿、蓝分量值。
//  返回值: esp_err_t 类型的错误码，表示操作是否成功。

// led_strip_refresh (led_strip);
#endif /* LED_MY_H */