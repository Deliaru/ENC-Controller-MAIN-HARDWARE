#include "winusb_new.h"
#include "tusb.h"
#include <string.h>
#include "LED_My.h"
#include "usb_descriptors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "WINUSB";

// 全局变量定义
uint16_t g_last_input_data_len = 0;
input_data_t g_last_input_data = {0};
volatile bool g_led_data_pending = false; // LED数据待处理标志

// 新增：上一帧IO4颜色缓存与有效标志
static uint8_t s_prev_io4_leds[18] = {0};
static bool s_prev_io4_valid = false;
// 缓存最后一次通过非阻塞处理的LED输入，用于后台定期刷新
static input_data_t s_last_led_input = {0};
static bool s_last_led_valid = false;
// 递归互斥锁用于保护对LED硬件的并发访问
static SemaphoreHandle_t s_led_mutex = NULL;

// 保护共享输入缓冲区的临界区锁
static portMUX_TYPE s_winusb_lock = portMUX_INITIALIZER_UNLOCKED;

// Microsoft OS 2.0 描述符请求代码
#define VENDOR_REQUEST_MICROSOFT 0xEE
#define VENDOR_REQUEST_WEBUSB    0xED

// Microsoft OS 2.0 描述符类型
#define MS_OS_20_SET_HEADER_DESCRIPTOR          0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION    0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION         0x02
#define MS_OS_20_FEATURE_COMPATBLE_ID           0x03
#define MS_OS_20_FEATURE_REG_PROPERTY           0x04

// WebUSB 描述符类型
#define WEBUSB_VENDOR_CODE                      0x20
#define WEBUSB_LANDING_PAGE_INDEX               1

// Ontroller.WinUSB.IO 指定的GUID
#define ONTROLLER_GUID "{A5DCBF10-6530-11D2-901F-00C04FB951ED}"

/************* TinyUSB 描述符 ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_VENDOR * TUD_VENDOR_DESC_LEN)

/**
 * @brief 字符串描述符 - 匹配Ontroller设备
 */
const char* winusb_string_descriptor[5] = {
    (char[]){0x09, 0x04},  // 0: 支持的语言是英语 (0x0409)
    "SJ",                  // 1: 制造商
    "E.N.C Controller",    // 2: 产品名称 - 用户可见的设备名称
    "123456",              // 3: 序列号
    "Vendor Device",       // 4: 接口描述
};

/**
 * @brief 配置描述符 - Vendor类设备
 */
static const uint8_t winusb_configuration_descriptor[] = {
    // 配置编号，接口数量，字符串索引，总长度，属性，功耗(mA)
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // 接口编号，字符串索引，EP Out地址，EP In地址，EP大小
    TUD_VENDOR_DESCRIPTOR(0, 4, 0x03, 0x84, 64),
};

/**
 * @brief Microsoft OS 2.0 描述符
 */
static const uint8_t ms_os_20_descriptor[] = {
    // Microsoft OS 2.0 设置头描述符
    0x0A, 0x00,                         // wLength
    MS_OS_20_SET_HEADER_DESCRIPTOR, 0x00, // wDescriptorType
    0x00, 0x00, 0x03, 0x06,             // dwWindowsVersion (Windows 10)
    0x00, 0x00,                         // wMSOSDescriptorSetTotalLength

    // 配置子集头描述符
    0x08, 0x00,                         // wLength
    MS_OS_20_SUBSET_HEADER_CONFIGURATION, 0x00, // wDescriptorType
    0x00,                               // bConfigurationValue
    0x00,                               // bReserved
    0x00, 0x00,                         // wTotalLength

    // 功能子集头描述符
    0x08, 0x00,                         // wLength
    MS_OS_20_SUBSET_HEADER_FUNCTION, 0x00, // wDescriptorType
    0x00,                               // bFirstInterface
    0x00,                               // bReserved
    0x00, 0x00,                         // wSubsetLength

    // 兼容ID描述符
    0x14, 0x00,                         // wLength
    MS_OS_20_FEATURE_COMPATBLE_ID, 0x00, // wDescriptorType
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, // compatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // subCompatibleID

    // 注册表属性描述符
    0x84, 0x00,                         // wLength
    MS_OS_20_FEATURE_REG_PROPERTY, 0x00, // wDescriptorType
    0x07, 0x00,                         // wPropertyDataType (REG_MULTI_SZ)
    0x2A, 0x00,                         // wPropertyNameLength
    // PropertyName: "DeviceInterfaceGUIDs" (UTF-16)
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00,
    'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,
    'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00,
    'D', 0x00, 's', 0x00, 0x00, 0x00,
    0x50, 0x00,                         // wPropertyDataLength
    // PropertyData: Ontroller GUID (UTF-16)
    '{', 0x00, 'A', 0x00, '5', 0x00, 'D', 0x00, 'C', 0x00, 'B', 0x00,
    'F', 0x00, '1', 0x00, '0', 0x00, '-', 0x00, '6', 0x00, '5', 0x00,
    '3', 0x00, '0', 0x00, '-', 0x00, '1', 0x00, '1', 0x00, 'D', 0x00,
    '2', 0x00, '-', 0x00, '9', 0x00, '0', 0x00, '1', 0x00, 'F', 0x00,
    '-', 0x00, '0', 0x00, '0', 0x00, 'C', 0x00, '0', 0x00, '4', 0x00,
    'F', 0x00, 'B', 0x00, '9', 0x00, '5', 0x00, '1', 0x00, 'E', 0x00,
    'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00
};

// WebUSB 描述符已移除 - 未使用

/**
 * @brief BOS 描述符
 */
static const uint8_t bos_descriptor[] = {
    0x05,                               // bLength
    0x0F,                               // bDescriptorType (BOS)
    0x00, 0x00,                         // wTotalLength
    0x02,                               // bNumDeviceCaps

    // WebUSB 设备能力描述符
    0x18, 0x00,                         // wLength
    0x10, 0x01,                         // wDescriptorType (WebUSB)
    0x01, 0x00,                         // bcdVersion (1.0)
    0x01,                               // bVendorCode
    0x01,                               // iLandingPage

    // Microsoft OS 2.0 设备能力描述符
    0x20, 0x00,                         // wLength
    0x10, 0x01,                         // wDescriptorType (Microsoft OS 2.0)
    0x00, 0x00, 0x03, 0x06,             // dwWindowsVersion (Windows 10)
    0x00, 0x00,                         // wMSOSDescriptorSetTotalLength
    0xEE,                               // bMS_VendorCode
    0x00                                // bAltEnumCode
};

/********* TinyUSB Vendor 回调函数 ***************/

uint8_t const *tud_vendor_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return NULL; // Vendor类不需要报告描述符
}

uint16_t tud_vendor_get_report_cb(uint8_t instance, uint8_t report_id, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    
    // 检查缓冲区有效性
    if (buffer == NULL) {
        return 0;
    }
    
    // 游戏可能期望特定的设备状态或能力信息
    // 尝试返回一些基本的设备信息
    if (reqlen > 0) {
        // 清零缓冲区
        memset(buffer, 0, reqlen);
        
        // 设置一些基本状态 - 模拟设备就绪状态
        if (reqlen >= 4) {
            buffer[0] = 0x01; // 设备状态：就绪
            buffer[1] = 0x00; // 扩展状态
            buffer[2] = 0xFF; // 功能标志
            buffer[3] = 0x00; // 保留
        }
        
        return reqlen;
    }
    
    return 0;
}

// TinyUSB Vendor类接收回调函数 - 正确的函数签名
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize)
{
    if (buffer == NULL) {
        ESP_LOGW(TAG, "Received NULL buffer in vendor RX callback");
        return;
    }
    
    if (bufsize > 0) {
        // 严格检查缓冲区大小
        if (bufsize > sizeof(input_data_t)) {
            ESP_LOGW(TAG, "Buffer overflow prevented: received %u bytes, max allowed %u", 
                     (unsigned int)bufsize, (unsigned int)sizeof(input_data_t));
            bufsize = sizeof(input_data_t);
        }
        
        // 清零接收缓冲区
        memset(&g_last_input_data, 0, sizeof(input_data_t));
        
        // 安全拷贝数据
        if (bufsize > 0) {
            SAFE_MEMCPY(&g_last_input_data, buffer, bufsize, sizeof(input_data_t));
        }
        
        g_last_input_data_len = bufsize;
        
        // 如果是LED数据包，设置待处理标志
        if (bufsize >= 3 && buffer[0] == 0x44 && buffer[1] == 0x4C && buffer[2] == 0x01) {
            g_led_data_pending = true; // 设置待处理标志
        }
    }
}

// TinyUSB设备挂载回调
void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB Device Mounted");
}

// TinyUSB设备卸载回调
void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB Device Unmounted");
}

// TinyUSB总线挂起回调
void tud_suspend_cb(bool remote_wakeup_en)
{
    ESP_LOGI(TAG, "USB Bus Suspended");
}

// TinyUSB总线恢复回调
void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB Bus Resumed");
}



// 注释掉tud_vendor_set_report_cb函数，因为TinyUSB vendor类可能不会调用它
// 正确的回调是tud_vendor_rx_cb，我们已经在那里处理了数据
/*
void tud_vendor_set_report_cb(uint8_t instance, uint8_t report_id, uint8_t const* buffer, uint16_t bufsize)
{
    // 这个函数可能不会被TinyUSB vendor类调用
    // 数据已经在tud_vendor_rx_cb中处理
}
*/

/********* 自定义Vendor请求处理 ***************/

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
    (void) rhport;
    
    // 处理Microsoft OS 2.0 描述符请求
    if (request->bmRequestType == 0xC1 && // Vendor, Device to Host, Interface
        request->bRequest == VENDOR_REQUEST_MICROSOFT &&
        request->wIndex == 0xEE) {
        
        if (stage == CONTROL_STAGE_SETUP) {
            return tud_control_xfer(rhport, request, (void*) ms_os_20_descriptor, sizeof(ms_os_20_descriptor));
        }
        return true;
    }
    
    // 处理WebUSB请求
    if (request->bmRequestType == 0xC0 && // Vendor, Device to Host, Device
        request->bRequest == VENDOR_REQUEST_WEBUSB) {
        
        if (stage == CONTROL_STAGE_SETUP) {
            // 返回简单的URL
            const char* url = "https://example.com";
            return tud_control_xfer(rhport, request, (void*) url, strlen(url));
        }
        return true;
    }
    
    return false;
}

/********* BOS 描述符回调 ***************/

uint8_t const *tud_descriptor_bos_cb(void)
{
    return bos_descriptor;
}

/********* 应用程序函数 ***************/

void winusb_new_init(void)
{
    ESP_LOGI(TAG, "Initializing WinUSB device...");
    
    // 初始化全局变量
    g_last_input_data_len = 0;
    memset(&g_last_input_data, 0, sizeof(input_data_t));
    g_led_data_pending = false;
    // 初始化上一帧IO4缓存
    memset(s_prev_io4_leds, 0, sizeof(s_prev_io4_leds));
    s_prev_io4_valid = false;
    
    // TinyUSB配置
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &winusb_device_descriptor,
        .string_descriptor = winusb_string_descriptor,
        .external_phy = false,
        .configuration_descriptor = winusb_configuration_descriptor,
#ifdef CONFIG_TINYUSB_CDC_ENABLED
        .self_powered = true,
        .vbus_monitor_io = GPIO_NUM_NC,
#endif
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    
    // 等待USB挂载
    int wait_count = 0;
    while (!tud_mounted() && wait_count < 100) {
        vTaskDelay(pdMS_TO_TICKS(100));
        wait_count++;
    }
    
    if (!tud_mounted()) {
        ESP_LOGW(TAG, "USB not mounted after 10 seconds, continuing anyway");
    }
    
    // 强制USB重置，确保设备描述符正确
    tud_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));
    tud_connect();
    
    // 给游戏一些时间来完成初始化检测
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 启动后台LED刷新任务，确保在没有新数据时也能维持LED显示
    // 创建递归互斥锁
    s_led_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_led_mutex == NULL) {
        ESP_LOGW(TAG, "Failed to create LED mutex");
    }

    BaseType_t xres = xTaskCreate(led_refresh_task, "led_refresh", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    if (xres != pdPASS) {
        ESP_LOGW(TAG, "Failed to create led_refresh_task");
    }
    
    ESP_LOGI(TAG, "WinUSB device initialized successfully");
}

void winusb_new_send_data(const output_data_t *out_data)
{
    if (!tud_mounted() || !tud_vendor_mounted()) {
        return;
    }
    
    if (out_data == NULL) {
        ESP_LOGE(TAG, "Output data is NULL");
        return;
    }
    
    // 验证头部数据正确性
    if (out_data->header[0] != 0x44 || out_data->header[1] != 0x44 || out_data->header[2] != 0x54) {
        ESP_LOGE(TAG, "Invalid header: %02X %02X %02X", 
                 out_data->header[0], out_data->header[1], out_data->header[2]);
        return;
    }
    


    // 检查发送缓冲区是否可用
    uint32_t available_space = tud_vendor_write_available();
    if (available_space < WINUSB_INPUT_REPORT_SIZE) {
        tud_vendor_write_flush();
        vTaskDelay(pdMS_TO_TICKS(1));
        available_space = tud_vendor_write_available();
        if (available_space < WINUSB_INPUT_REPORT_SIZE) {
            return;
        }
    }
    
    // 发送数据到PC
    bool success = tud_vendor_write((const void*)out_data, WINUSB_INPUT_REPORT_SIZE);
    if (success) {
        tud_vendor_write_flush();
    }
}



bool winusb_new_get_data(input_data_t *input_data)
{
    if (input_data == NULL) {
        ESP_LOGE(TAG, "Input data pointer is NULL");
        return false;
    }
    
    bool has_data = false;
    
    // 检查是否有数据可用
    if (g_last_input_data_len > 0 && g_last_input_data_len <= sizeof(input_data_t)) {
        // 使用临界区保护读取，防止与 USB 回调并发写入冲突
        portENTER_CRITICAL(&s_winusb_lock);
        uint16_t copy_len = g_last_input_data_len; // 保存长度
        // 清零目标缓冲区
        memset(input_data, 0, sizeof(input_data_t));
        // 只拷贝有效的字节
        memcpy(input_data, &g_last_input_data, copy_len);
        // 日志
        ESP_LOGD(TAG, "Copied %u bytes of input data", (unsigned int)copy_len);
        // 清空标志（在临界区内清零，保证一致性）
        g_last_input_data_len = 0;
        portEXIT_CRITICAL(&s_winusb_lock);

        has_data = true;
    } else if (g_last_input_data_len > sizeof(input_data_t)) {
        ESP_LOGW(TAG, "Input data too large: %d bytes, max allowed %d", 
                 g_last_input_data_len, sizeof(input_data_t));
        g_last_input_data_len = 0; // 清空无效数据
    }
    
    return has_data;
}

void winusb_new_handle_led_input(const input_data_t *input)
{
    // 保护整个LED处理流程，避免与后台刷新任务并发冲突
    if (s_led_mutex) {
        if (xSemaphoreTakeRecursive(s_led_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "winusb_new_handle_led_input: take mutex timeout");
        }
    }
    if (input == NULL) {
        ESP_LOGE(TAG, "LED input data is NULL");
        if (s_led_mutex) xSemaphoreGiveRecursive(s_led_mutex);
        return;
    }
    
    // 检查LED硬件是否已初始化
    extern led_strip_handle_t led_1, led_5;
    if (led_1 == NULL || led_5 == NULL) {
        ESP_LOGE(TAG, "LED hardware not initialized in full processing: led_1=%p, led_5=%p", led_1, led_5);
        return;
    }
    
    // 首先进行快速处理
    winusb_new_handle_led_input_nonblocking(input);
    
    // 然后进行刷新操作（可能阻塞）
    esp_err_t err1 = led_strip_refresh(led_1);
    esp_err_t err5 = led_strip_refresh(led_5);
    
    if (err1 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh LED_1: %s", esp_err_to_name(err1));
    }
    
    if (err5 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh LED_5: %s", esp_err_to_name(err5));
    }

    if (s_led_mutex) xSemaphoreGiveRecursive(s_led_mutex);
}

// 非阻塞LED处理函数，用于USB回调中的快速处理
void winusb_new_handle_led_input_nonblocking(const input_data_t *input)
{
    // 互斥保护，允许递归（因为可能从外层再调用本函数）
    if (s_led_mutex) {
        if (xSemaphoreTakeRecursive(s_led_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "winusb_new_handle_led_input_nonblocking: take mutex timeout");
        }
    }
    if (input == NULL) {
        ESP_LOGE(TAG, "LED input data is NULL");
        if (s_led_mutex) xSemaphoreGiveRecursive(s_led_mutex);
        return;
    }
    
    // 检查LED硬件是否已初始化
    extern led_strip_handle_t led_1, led_5;
    if (led_1 == NULL || led_5 == NULL) {
        ESP_LOGE(TAG, "LED hardware not initialized: led_1=%p, led_5=%p", led_1, led_5);
        return;
    }
    
    // 验证数据包头部
    if (input->header[0] != 0x44 || input->header[1] != 0x4C || input->header[2] != 0x01) {
        ESP_LOGW(TAG, "Invalid LED packet header: %02X %02X %02X", 
                 input->header[0], input->header[1], input->header[2]);
        return;
    }

    // 粘滞合并：当本帧IO4全零而Side存在颜色时，沿用上一帧的IO4颜色
    uint8_t io4_leds_merged[18];
    memcpy(io4_leds_merged, input->io4_leds, sizeof(io4_leds_merged));
    bool io4_all_zero = true;
    for (int i = 0; i < 18; i++) {
        if (input->io4_leds[i] != 0) { io4_all_zero = false; break; }
    }
    bool side_any_nonzero = false;
    for (int i = 0; i < 12; i++) {
        if (input->side_leds[i] != 0) { side_any_nonzero = true; break; }
    }
    if (io4_all_zero && side_any_nonzero && s_prev_io4_valid) {
        memcpy(io4_leds_merged, s_prev_io4_leds, sizeof(io4_leds_merged));
    }
    // 更新上一帧缓存：仅当本帧IO4非全零时才更新
    if (!io4_all_zero) {
        memcpy(s_prev_io4_leds, input->io4_leds, sizeof(s_prev_io4_leds));
        s_prev_io4_valid = true;
    }
    
    // 设置IO4 LED (led_1) - 5个LED，每个3字节RGB
    // 数据格式：L1, L2, L3, R1, R2, R3 (每个LED占用3字节：R, G, B)
    // 数据偏移：input->io4_leds[0-17] 对应 packet[3-20]
    // LED映射：led_1[0]=L3, led_1[1]=L2, led_1[2]=L1, led_1[3]=LS1, led_1[4]=LS2
    // L3, L2, L1从IO4数据获取，LS1,LS2从Side数据获取 (LED 0, LED 1)
    const int led_1_mapping[6] = {2, 1, 0, -1, -2, -2}; // 扩展到6颗：第6颗复用LS2（-2）
    
    for (int i = 0; i < 6; i++) {
        uint8_t r = 0, g = 0, b = 0; // 初始化默认值
        
        if (led_1_mapping[i] >= 0) {
            // 从IO4数据获取 (L3, L2, L1)
            int source_idx = led_1_mapping[i] * 3;
            r = io4_leds_merged[source_idx];
            g = io4_leds_merged[source_idx + 1];
            b = io4_leds_merged[source_idx + 2];
        } else if (led_1_mapping[i] == -1) {
            // 从Side数据获取 (LS1 - LED 0)
            r = input->side_leds[0];
            g = input->side_leds[1];
            b = input->side_leds[2];
        } else if (led_1_mapping[i] == -2) {
            // 从Side数据获取 (LS2 - LED 1)
            r = input->side_leds[3];
            g = input->side_leds[4];
            b = input->side_leds[5];
        }
        
        // 设置LED颜色
        esp_err_t err = led_strip_set_pixel(led_1, i, r, g, b);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED_1[%d]: RGB(%d,%d,%d), error: %s", 
                     i, (int)r, (int)g, (int)b, esp_err_to_name(err));
        }
    }
    
    // 设置侧边LED (led_5) - 5个LED，每个3字节RGB
    // 数据偏移：input->side_leds[0-5] 对应 packet[21-26]
    // LED映射：led_5[0]=R1, led_5[1]=R2, led_5[2]=R3, led_5[3]=RS1, led_5[4]=RS2
    // R1, R2, R3从IO4数据获取，RS1,RS2从Side数据获取 (LED 0, LED 1)
    for (int i = 0; i < 6; i++) {
        uint8_t r = 0, g = 0, b = 0;
        
        if (i < 3) {
            // R1, R2, R3从IO4数据获取 (索引3, 4, 5)
            int source_idx = (i + 3) * 3; // R1=索引3, R2=索引4, R3=索引5
            r = io4_leds_merged[source_idx];
            g = io4_leds_merged[source_idx + 1];
            b = io4_leds_merged[source_idx + 2];
        } else if (i == 3) {
            // RS1从Side数据获取 (LED 0)
            r = input->side_leds[0];
            g = input->side_leds[1];
            b = input->side_leds[2];
        } else if (i == 4) {
            // RS2从Side数据获取 (LED 1)
            r = input->side_leds[3];
            g = input->side_leds[4];
            b = input->side_leds[5];
        } else if (i == 5) {
            // RS3复用RS2
            r = input->side_leds[3];
            g = input->side_leds[4];
            b = input->side_leds[5];
        }
        
        // 设置LED颜色
        esp_err_t err = led_strip_set_pixel(led_5, i, r, g, b);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set LED_5[%d]: RGB(%d,%d,%d), error: %s", 
                     i, (int)r, (int)g, (int)b, esp_err_to_name(err));
        }
    }
    
    // 刷新LED显示
    esp_err_t err1 = led_strip_refresh(led_1);
    esp_err_t err5 = led_strip_refresh(led_5);
    
    if (err1 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh LED_1: %s", esp_err_to_name(err1));
    }
    
    if (err5 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh LED_5: %s", esp_err_to_name(err5));
    }

    // 缓存这次通过非阻塞函数写入的LED数据，供后台任务重发使用
    memcpy(&s_last_led_input, input, sizeof(input_data_t));
    s_last_led_valid = true;

    if (s_led_mutex) xSemaphoreGiveRecursive(s_led_mutex);
}

// 后台任务：每50ms重发最后一次通过非阻塞写入的LED数据，防止LED断电后重上电没有新数据导致不亮
void led_refresh_task(void *pvParameters)
{
    (void) pvParameters;
    const TickType_t delay = pdMS_TO_TICKS(50);
    while (1) {
        if (s_last_led_valid) {
            // 以非阻塞方式重新应用缓存的LED数据
            // 这里不需要单独加锁，因为 winusb_new_handle_led_input_nonblocking 内部已加锁（递归互斥）
            winusb_new_handle_led_input_nonblocking(&s_last_led_input);
        }
        vTaskDelay(delay);
    }
}

// 进入映射模式时的提示灯：L1 和 R3 亮白，其他灯熄灭（只发送一次）
void winusb_new_send_mapping_mode_indicator(void)
{
    input_data_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.header[0] = 0x44;
    tmp.header[1] = 0x4C;
    tmp.header[2] = 0x01;

    // io4_leds 布局: L1,L2,L3,R1,R2,R3 (每个3字节)
    // 将 L1 (index 0) 与 R3 (index 5) 设置为白色 (255,255,255)
    tmp.io4_leds[0] = 255; tmp.io4_leds[1] = 255; tmp.io4_leds[2] = 255; // L1
    tmp.io4_leds[15] = 255; tmp.io4_leds[16] = 255; tmp.io4_leds[17] = 255; // R3

    // 侧边LED保持为0（熄灭）

    // 通过非阻塞接口发送一次提示帧
    winusb_new_handle_led_input_nonblocking(&tmp);
}

// 设置侧边 LED（LS + RS）颜色并刷新
void winusb_new_set_side_leds(uint8_t r, uint8_t g, uint8_t b)
{
    extern led_strip_handle_t led_1, led_5;
    if (led_1 == NULL || led_5 == NULL) {
        ESP_LOGW(TAG, "winusb_new_set_side_leds: LED hardware not initialized");
        return;
    }

    // 保护 LED 写入
    if (s_led_mutex) {
        if (xSemaphoreTakeRecursive(s_led_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "winusb_new_set_side_leds: take mutex timeout");
        }
    }

    // led_1 indices 3,4 => LS1, LS2
    esp_err_t err;
    err = led_strip_set_pixel(led_1, 3, r, g, b);
    if (err != ESP_OK) ESP_LOGE(TAG, "set LED_1[3] failed: %s", esp_err_to_name(err));
    err = led_strip_set_pixel(led_1, 4, r, g, b);
    if (err != ESP_OK) ESP_LOGE(TAG, "set LED_1[4] failed: %s", esp_err_to_name(err));

    // led_5 indices 3,4 => RS1, RS2
    err = led_strip_set_pixel(led_5, 3, r, g, b);
    if (err != ESP_OK) ESP_LOGE(TAG, "set LED_5[3] failed: %s", esp_err_to_name(err));
    err = led_strip_set_pixel(led_5, 4, r, g, b);
    if (err != ESP_OK) ESP_LOGE(TAG, "set LED_5[4] failed: %s", esp_err_to_name(err));

    // 刷新显示
    esp_err_t err1 = led_strip_refresh(led_1);
    esp_err_t err5 = led_strip_refresh(led_5);
    if (err1 != ESP_OK) ESP_LOGE(TAG, "refresh LED_1 failed: %s", esp_err_to_name(err1));
    if (err5 != ESP_OK) ESP_LOGE(TAG, "refresh LED_5 failed: %s", esp_err_to_name(err5));
    // 同步更新缓存，保证后台刷新任务不会覆盖这些侧边LED设置
    // s_last_led_input 是同一文件的静态缓存
    // set header so nonblocking handler accepts this cached frame
    s_last_led_input.header[0] = 0x44;
    s_last_led_input.header[1] = 0x4C;
    s_last_led_input.header[2] = 0x01;
    // side_leds layout: 6 bytes (3*2 leds) or 12? defined as 12 in input_data_t, but we only set first two RGBs.
    s_last_led_input.side_leds[0] = r;
    s_last_led_input.side_leds[1] = g;
    s_last_led_input.side_leds[2] = b;
    s_last_led_input.side_leds[3] = r;
    s_last_led_input.side_leds[4] = g;
    s_last_led_input.side_leds[5] = b;
    s_last_led_valid = true;

    if (s_led_mutex) xSemaphoreGiveRecursive(s_led_mutex);
}



// 在主循环中处理LED数据
void winusb_new_process_pending_led_data(void)
{
    if (g_led_data_pending) {
        // 创建临时数据结构进行处理
        input_data_t temp_data;
        memcpy(&temp_data, &g_last_input_data, sizeof(input_data_t));
        
        // 清除待处理标志（在处理前清除，避免重复处理）
        g_led_data_pending = false;
        
        // 处理LED数据
        winusb_new_handle_led_input_nonblocking(&temp_data);
        
        // 手动准备接收端点，确保能接收下一次数据
        winusb_prepare_rx_endpoint();
    }
}

void winusb_new_handle_option_input(const input_data_t *input)
{
    if (input == NULL) {
        return;
    }
    
    // 这里可以添加选项配置的处理逻辑
    // 例如保存配置到NVS等
}

// 添加USB状态监控函数
void winusb_debug_status(void)
{
    ESP_LOGI(TAG, "=== USB Status Debug ===");
    ESP_LOGI(TAG, "USB Mounted: %s", tud_mounted() ? "YES" : "NO");
    ESP_LOGI(TAG, "Vendor Mounted: %s", tud_vendor_mounted() ? "YES" : "NO");
    ESP_LOGI(TAG, "Write Available: %lu bytes", (unsigned long)tud_vendor_write_available());
    ESP_LOGI(TAG, "Read Available: %lu bytes", (unsigned long)tud_vendor_available());
    ESP_LOGI(TAG, "LED Data Pending: %s", g_led_data_pending ? "YES" : "NO");
    ESP_LOGI(TAG, "Input Data Length: %u bytes", (unsigned int)g_last_input_data_len);
    ESP_LOGI(TAG, "========================");
}

// 详细的USB端点状态监控
void winusb_detailed_status(void)
{
    ESP_LOGI(TAG, "=== Detailed USB Status ===");
    ESP_LOGI(TAG, "USB Mounted: %s", tud_mounted() ? "YES" : "NO");
    ESP_LOGI(TAG, "Vendor Mounted: %s", tud_vendor_mounted() ? "YES" : "NO");
    ESP_LOGI(TAG, "Write Available: %lu bytes", (unsigned long)tud_vendor_write_available());
    ESP_LOGI(TAG, "Read Available: %lu bytes", (unsigned long)tud_vendor_available());
    ESP_LOGI(TAG, "LED Data Pending: %s", g_led_data_pending ? "YES" : "NO");
    ESP_LOGI(TAG, "Input Data Length: %u bytes", (unsigned int)g_last_input_data_len);
    
    // 检查内存状态
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
    
    // 检查任务状态
    UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "Stack high water mark: %d", stack_high_water_mark);
    
    // 检查系统时间
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "System time: %lu ms", (unsigned long)current_time);
    
    ESP_LOGI(TAG, "===========================");
}

// USB状态重置函数，用于处理连接问题
void winusb_reset_connection(void)
{
    // 清空所有缓冲区
    if (tud_mounted() && tud_vendor_mounted()) {
        tud_vendor_write_flush();
        // 清空接收缓冲区
        while (tud_vendor_available() > 0) {
            uint8_t temp_buffer[64];
            uint32_t read_size = tud_vendor_read(temp_buffer, sizeof(temp_buffer));
            if (read_size == 0) break;
        }
    }
}

// 手动准备接收端点，确保能接收下一次数据
void winusb_prepare_rx_endpoint(void)
{
    if (!tud_mounted() || !tud_vendor_mounted()) {
        return;
    }
    
    // 清空接收缓冲区
    uint32_t available = tud_vendor_available();
    if (available > 0) {
        uint8_t temp_buffer[64];
        while (tud_vendor_available() > 0) {
            uint32_t read_size = tud_vendor_read(temp_buffer, sizeof(temp_buffer));
            if (read_size == 0) break;
        }
    }
    
    // 强制刷新所有缓冲区
    tud_vendor_write_flush();
    
    // 短暂延时，让USB处理完成
    vTaskDelay(pdMS_TO_TICKS(1));
}




