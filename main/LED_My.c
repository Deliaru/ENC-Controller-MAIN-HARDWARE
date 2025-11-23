#include "LED_My.h"
led_strip_handle_t led_1;
led_strip_handle_t led_2;
led_strip_handle_t led_3;
led_strip_handle_t led_4;
led_strip_handle_t led_5;
led_strip_handle_t led_6;
led_strip_handle_t led_7;
led_strip_handle_t led_8;
led_strip_handle_t configure_led(int BLINK_GPIO, int MAX_LEDS)
{
    static led_strip_handle_t led_strip;
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = MAX_LEDS,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
    return led_strip;
}
void LED_init(void)
{
    led_1 = configure_led(LED_1, 8);
    led_5 = configure_led(LED_5, 8);
}

// LED控制处理函数

