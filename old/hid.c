#include "hid.h"
#include "tusb.h"
#include <string.h>
#include "LED_My.h"

// 全局缓冲区保存最近一次收到的数据
uint16_t g_last_input_data_len = 0;
static input_data_t g_last_input_data;
// static uint16_t g_last_input_data_len = 0;

const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "SJ",                  // 1: Manufacturer
    "ENC",                 // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
};
const uint8_t hid_report_descriptor[] = {
    0x06, 0x00, 0xFF,      // Usage Page (Vendor Defined)
    0x09, 0x01,            // Usage (Vendor Usage 1)
    0xA1, 0x01,            // Collection (Application)

    // 10个按钮（每个1字节）
    0x75, 0x08,            // Report Size (8)
    0x95, 0x0A,            // Report Count (10)
    0x15, 0x00,            // Logical Minimum (0)
    0x25, 0x01,            // Logical Maximum (1)
    0x09, 0x01,            // Usage (Vendor Usage 1)
    0x81, 0x02,            // Input (Data,Var,Abs)

    // 摇杆（16位）
    0x75, 0x10,            // Report Size (16)
    0x95, 0x01,            // Report Count (1)
    0x15, 0x00,            // Logical Minimum (0)
    0x26, 0xFF, 0xFF,      // Logical Maximum (65535)
    0x09, 0x02,            // Usage (Vendor Usage 2)
    0x81, 0x02,            // Input (Data,Var,Abs)

    // scan（8位）
    0x75, 0x08,            // Report Size (8)
    0x95, 0x01,            // Report Count (1)
    0x15, 0x00,            // Logical Minimum (0)
    0x25, 0xFF,            // Logical Maximum (255)
    0x09, 0x03,            // Usage (Vendor Usage 3)
    0x81, 0x02,            // Input (Data,Var,Abs)

    // aime_id（10字节）
    0x75, 0x08,            // Report Size (8)
    0x95, 0x0A,            // Report Count (10)
    0x15, 0x00,            // Logical Minimum (0)
    0x25, 0xFF,            // Logical Maximum (255)
    0x09, 0x04,            // Usage (Vendor Usage 4)
    0x81, 0x02,            // Input (Data,Var,Abs)

    0xC0                   // End Collection
};

// TinyUSB回调：返回自定义报告描述符
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hid_report_descriptor;
}

// HID初始化函数
void hid_init(void)
{
    tinyusb_config_t tusb_cfg = { 0 };
    tusb_cfg.device_descriptor = NULL; // 使用默认
    tusb_cfg.string_descriptor = hid_string_descriptor; // 使用自定义描述符
    tusb_cfg.external_phy = false;
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // 创建TinyUSB任务

}

// TinyUSB HID控制端点SET_REPORT回调
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // 保存PC发来的数据到全局input_data
    uint16_t copy_len = bufsize > sizeof(g_last_input_data.buffer) ? sizeof(g_last_input_data.buffer) : bufsize;
    memcpy(g_last_input_data.buffer, buffer, copy_len);
    g_last_input_data_len = copy_len;
}

// Push函数：发送out_data并获取最近一次收到的HID数据
void Push(const output_data_t *out_data, input_data_t *input_data)
{
    // 发送数据到PC
    tud_hid_report(0, (const void*)out_data->buffer, 23);

    // 拷贝最近一次收到的数据到input_data
    if (g_last_input_data_len > 0)
    {
        memcpy(input_data->buffer, g_last_input_data.buffer, g_last_input_data_len);
        g_last_input_data_len = 0; // 清空，避免重复读取
    }
    else
    {
        memset(input_data->buffer, 0, sizeof(input_data->buffer)); // 没有新数据则清零
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    return 0;
}

void handle_led_input(const input_data_t *input) {
    // 检查type是否为LED控制类型(0)
    if (input->type != 0) {
        return ;
    }

    // 获取LED配置
    led_t* led_config = (led_t*)&input->led;
    
    // 如果亮度为0，则关闭所有LED
    if (led_config->ledBrightness == 0) {
        led_strip_clear(led_1);
        led_strip_clear(led_5);
        led_strip_refresh(led_1);
        led_strip_refresh(led_5);
        return;
    }
    
    // 获取全局亮度
    uint8_t brightness = led_config->ledBrightness;
    
    // 处理前4个LED (使用led_1通道)
    for (int i = 0; i < 4; i++) {
        // 获取RGB颜色值并应用亮度
        uint8_t r = (uint8_t)((led_config->ledColors[i][0] * brightness) / 255);
        uint8_t g = (uint8_t)((led_config->ledColors[i][1] * brightness) / 255);
        uint8_t b = (uint8_t)((led_config->ledColors[i][2] * brightness) / 255);
        
        // 设置LED_1通道的对应灯珠
        led_strip_set_pixel(led_1, i, r, g, b);
    }
    
    // 处理后4个LED (使用led_5通道)
    for (int i = 4; i < 8; i++) {
        // 获取RGB颜色值并应用亮度
        uint8_t r = (uint8_t)((led_config->ledColors[i][0] * brightness) / 255);
        uint8_t g = (uint8_t)((led_config->ledColors[i][1] * brightness) / 255);
        uint8_t b = (uint8_t)((led_config->ledColors[i][2] * brightness) / 255);
        
        // 设置LED_5通道的对应灯珠 (索引需要减4)
        led_strip_set_pixel(led_5, i-4, r, g, b);
    }
    
    // 刷新两个LED通道的显示
    led_strip_refresh(led_1);
    led_strip_refresh(led_5);
}