# WinUSB New Implementation

## 概述

这是一个基于TinyUSB的WinUSB Vendor设备实现，专门为与Ontroller.WinUSB.IO库兼容而设计。

## 主要特性

- **协议**: WinUSB Vendor类
- **设备识别**: VID_0E8F, PID_1216
- **数据格式**: 7字节输入，33字节输出
- **功能**: 游戏控制器按钮、摇杆、LED控制

## 文件结构

- `winusb_new.h` - 头文件，定义数据结构和函数接口
- `winusb_new.c` - 实现文件，包含WinUSB协议实现
- `winusb_new_example.c` - 使用示例
- `usb_descriptors.h` - USB设备描述符定义

## 数据结构

### 输出数据 (ESP32 → PC, 7字节)
```c
typedef struct {
    uint8_t header[3];     // 0x44, 0x44, 0x54
    uint8_t buttons[2];    // 按钮状态
    uint8_t lever[2];      // 摇杆位置
} output_data_t;
```

### 输入数据 (PC → ESP32, 33字节)
```c
typedef struct {
    uint8_t header[3];     // 0x44, 0x4C, 0x01
    uint8_t io4_leds[18];  // IO4 LED数据 (3*6)
    uint8_t side_leds[12]; // 侧边LED数据 (3*4)
} input_data_t;
```

## 主要函数

- `winusb_new_init()` - 初始化WinUSB设备
- `winusb_new_send_data()` - 发送数据到PC
- `winusb_new_get_data()` - 接收PC数据
- `winusb_new_handle_led_input()` - 处理LED控制
- `winusb_new_handle_option_input()` - 处理配置数据

## 使用示例

```c
#include "winusb_new.h"

void app_main(void)
{
    // 初始化WinUSB设备
    winusb_new_init();
    
    output_data_t out_data;
    input_data_t in_data;
    
    while (1) {
        if (tud_mounted()) {
            // 发送数据
            winusb_new_send_data(&out_data);
            
            // 接收数据
            if (winusb_new_get_data(&in_data)) {
                winusb_new_handle_led_input(&in_data);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

## 兼容性

此实现与Ontroller.WinUSB.IO库完全兼容，可以直接用于Windows平台。 