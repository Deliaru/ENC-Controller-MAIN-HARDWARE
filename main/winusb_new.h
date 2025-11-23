#ifndef WINUSB_NEW_H
#define WINUSB_NEW_H

// 包含自定义TinyUSB配置
#include "tusb_config_custom.h"

#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/vendor/vendor_device.h"
#include "esp_system.h"

// TinyUSB配置已在tusb_config_custom.h中定义

// 数据包大小定义 - 匹配Ontroller.WinUSB.IO
#define WINUSB_INPUT_REPORT_SIZE     7
#define WINUSB_OUTPUT_REPORT_SIZE    33
#define MAX_AIME_ID_LEN           18
#define MAX_LED_COUNT             10

// 安全检查宏
#define SAFE_MEMCPY(dst, src, size, max_size) \
    do { \
        if ((size) <= (max_size)) { \
            memcpy((dst), (src), (size)); \
        } else { \
            ESP_LOGE("WINUSB", "Buffer overflow prevented: size=%d, max=%d", (size), (max_size)); \
            memcpy((dst), (src), (max_size)); \
        } \
    } while(0)

#define SAFE_SPAN_COPY(dst, src, size, max_size) \
    do { \
        if ((size) <= (max_size)) { \
            (src).CopyTo(new Span<byte>((dst), 0, (size))); \
        } else { \
            ESP_LOGE("WINUSB", "Span overflow prevented: size=%d, max=%d", (size), (max_size)); \
            (src).Slice(0, (max_size)).CopyTo(new Span<byte>((dst), 0, (max_size))); \
        } \
    } while(0)

// 数据包类型定义
#define PACKET_TYPE_LED           0
#define PACKET_TYPE_OPTION        1

#pragma pack(push, 1)

// AIME结构（18字节）
typedef struct {
    uint8_t scan;              // 1字节
    uint8_t data[MAX_AIME_ID_LEN]; // 18字节
} aime_t;

// 输出数据结构（发送到PC，7字节）- 匹配Ontroller输入格式
typedef struct {
    uint8_t header[3];         // 0x44, 0x44, 0x54 (固定头部)
    uint8_t buttons[2];        // 2字节按钮状态
    uint8_t lever[2];          // 2字节摇杆位置
} output_data_t;

// LED颜色结构
typedef struct {
    uint8_t r, g, b;              // RGB颜色值
} led_color_t;

// LED控制结构
typedef struct {
    uint8_t brightness;           // 全局亮度 (0-255)
    led_color_t colors[MAX_LED_COUNT]; // LED颜色数组
} led_control_t;

// 选项配置结构
typedef struct {
    uint8_t aime_id[MAX_AIME_ID_LEN];            // AIME ID配置
} option_config_t;

// 输入数据结构（从PC接收，33字节）- 匹配Ontroller输出格式
typedef struct {
    uint8_t header[3];         // 0x44, 0x4C, 0x01 (固定头部)
    uint8_t io4_leds[18];      // IO4 LED数据 (3*6)
    uint8_t side_leds[12];     // 侧边LED数据 (3*4)
} input_data_t;

#pragma pack(pop)

// 全局变量声明
extern uint16_t g_last_input_data_len;
extern input_data_t g_last_input_data;
extern volatile bool g_led_data_pending;

// 函数声明
void winusb_new_init(void);
void winusb_new_send_data(const output_data_t *out_data);
bool winusb_new_get_data(input_data_t *input_data);
void winusb_new_handle_led_input(const input_data_t *input);
void winusb_new_handle_led_input_nonblocking(const input_data_t *input); // 非阻塞LED处理
void winusb_new_handle_option_input(const input_data_t *input);
void winusb_new_process_pending_led_data(void); // 在主循环中处理待处理的LED数据
void winusb_debug_status(void); // 添加USB状态调试函数
void winusb_detailed_status(void); // 详细的USB状态监控函数
void winusb_reset_connection(void); // USB连接重置函数
void winusb_prepare_rx_endpoint(void); // 手动准备接收端点
void led_refresh_task(void *pvParameters); // 后台任务：定期重写LED数据
void winusb_new_send_mapping_mode_indicator(void); // 进入映射模式时的LED提示（L1+R3白，其它灭）
void winusb_new_set_side_leds(uint8_t r, uint8_t g, uint8_t b); // 设置侧边LED（LS/RS）颜色并刷新

// TinyUSB回调函数声明
uint8_t const *tud_vendor_descriptor_report_cb(uint8_t instance);
uint16_t tud_vendor_get_report_cb(uint8_t instance, uint8_t report_id, uint8_t* buffer, uint16_t reqlen);
void tud_vendor_set_report_cb(uint8_t instance, uint8_t report_id, uint8_t const* buffer, uint16_t bufsize);
// 注意：tud_vendor_rx_cb已在TinyUSB中声明，我们只需实现它

// 自定义Vendor请求处理
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

// BOS描述符回调
uint8_t const *tud_descriptor_bos_cb(void);

#endif // WINUSB_NEW_H
