#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb.h"
#include "main.h"
#include "winusb_new.h"

static const char *TAG = "MAIN";

// 测试状态变量 - 已移除未使用的变量

/**
 * @brief 测试函数：模拟右ABC按钮每隔1秒交替按下和弹起
 */
void test_update(output_data_t *out_data)
{
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 设置固定头部 0x44, 0x44, 0x54
    out_data->header[0] = 0x44;
    out_data->header[1] = 0x44;
    out_data->header[2] = 0x54;
    
    // 计算按钮状态 - 每隔1秒切换
    bool buttons_pressed = ((current_time / 1000) % 2) == 0; // 偶数秒按下，奇数秒弹起
    
    // 设置按钮状态（使用修复后的映射）
    uint8_t left_buttons = 0x00;   // 左按钮全部释放
    uint8_t right_buttons = buttons_pressed ? 0x07 : 0x00;  // 右ABC：按下(0x01+0x02+0x04=0x07)或释放(0x00)
    uint8_t operation_buttons = 0x00; // 操作按钮释放
    
    // 组合按钮数据
    out_data->buttons[0] = left_buttons;
    out_data->buttons[1] = right_buttons | operation_buttons;
    
    // 摇杆保持在中心位置（修复后的映射）
    // 中心位置：angle = 180度 -> lever_value = 0 -> raw_lever = 409
    uint16_t raw_lever = 409; // (0 + 32767) / 80 = 409
    
    out_data->lever[0] = (uint8_t)(raw_lever & 0xFF);        // 低字节
    out_data->lever[1] = (uint8_t)((raw_lever >> 8) & 0xFF); // 高字节
    
    // 只在启动时输出一次状态
    static bool first_run = true;
    if (first_run) {
        ESP_LOGI(TAG, "测试模式: 右ABC按钮每隔1秒交替按下和弹起");
        ESP_LOGI(TAG, "按钮状态: 左=0x%02X, 右=0x%02X, 操作=0x%02X", 
                 left_buttons, right_buttons, operation_buttons);
        ESP_LOGI(TAG, "摇杆位置: 0x%04X (中心)", raw_lever);
        first_run = false;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting application...");
    
    // 初始化GPIO
    BTN_init();
    Ang_init();
    
    // 初始化LED
    LED_init();

    // 初始化新的WinUSB设备
    winusb_new_init();
    
    ESP_LOGI(TAG, "All components initialized successfully");
    
    // 选择运行模式：true=测试模式，false=真实GPIO模式
    bool test_mode = false;
    
    if (test_mode) {
        ESP_LOGI(TAG, "进入测试模式 - 模拟按钮和摇杆");
    } else {
        ESP_LOGI(TAG, "进入真实模式 - 使用GPIO输入");
    }

    input_data_t input_data;

    while (1)
    {
        // 定期打印USB状态调试信息
        static uint32_t last_debug_time = 0;
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (current_time - last_debug_time > 300000) { // 每5分钟调试一次
            winusb_detailed_status();
            last_debug_time = current_time;
        }
        
        // 检查USB是否已挂载
        if (tud_mounted()) {
            // 优先处理待处理的LED数据（在主循环中，避免USB回调阻塞）
            winusb_new_process_pending_led_data();
            
            output_data_t out_data;
            
            if (test_mode) {
                // 使用测试函数
                test_update(&out_data);
            } else {
                // 使用真实GPIO
                Update(&out_data);
            }

            // 发送WinUSB数据到PC - 降低发送频率以提高稳定性
            winusb_new_send_data(&out_data);
            
            // 检查是否有来自PC的数据
            if (winusb_new_get_data(&input_data)) {
                ESP_LOGD(TAG, "Got data from PC, processing...");

                // 处理其他配置数据
                winusb_new_handle_option_input(&input_data);
                
                ESP_LOGD(TAG, "PC data processing completed");
            }
            
            // 检查USB状态，如果发现问题则重置连接
            static uint32_t last_reset_check = 0;
            static uint32_t consecutive_failures = 0;
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            if (current_time - last_reset_check > 30000) { // 每30秒检查一次
                if (!tud_vendor_mounted()) {
                    consecutive_failures++;
                    ESP_LOGW(TAG, "Vendor interface not mounted, failure count: %lu", consecutive_failures);
                    
                    if (consecutive_failures >= 3) {
                        ESP_LOGE(TAG, "Too many consecutive failures, resetting USB connection");
                        winusb_reset_connection();
                        consecutive_failures = 0;
            }
                } else {
                    consecutive_failures = 0; // 重置失败计数
                }
                last_reset_check = current_time;
            }
        } else {
            static uint32_t last_usb_log = 0;
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            // 减少USB未挂载的日志频率
            if (current_time - last_usb_log > 30000) {
                ESP_LOGW(TAG, "USB not mounted, waiting for connection...");
                last_usb_log = current_time;
            }
        }

        // 降低更新频率到50Hz，减少USB发送压力
        vTaskDelay(pdMS_TO_TICKS(20)); // 从10ms改为20ms以减少USB压力
    }
}
