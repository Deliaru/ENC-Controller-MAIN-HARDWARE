#include "winusb_new.h"
#include "GPIO_My.h"
#include "LED_My.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WINUSB_EXAMPLE";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting WinUSB example...");
    
    // 初始化GPIO
    BTN_init();
    Ang_init();
    
    // 初始化LED
    LED_init();

    // 初始化WinUSB设备
    winusb_new_init();
    
    ESP_LOGI(TAG, "All components initialized successfully");

    output_data_t out_data;
    input_data_t in_data;

    while (1)
    {
        // 检查USB是否已挂载
        if (tud_mounted()) {
            // 更新GPIO状态并填充out_data
            Update(&out_data);

            // 发送WinUSB数据到PC
            winusb_new_send_data(&out_data);
            
            // 检查是否有来自PC的数据
            if (winusb_new_get_data(&in_data)) {
                ESP_LOGD(TAG, "Received data from PC");

                // 处理LED控制数据
                winusb_new_handle_led_input(&in_data);
                
                // 处理其他配置数据
                winusb_new_handle_option_input(&in_data);
            }
        } else {
            ESP_LOGW(TAG, "USB not mounted, waiting for connection...");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
} 