# Mu3io和Mu3hook RGB交互分析

## 数据流概述

```
游戏 → mu3hook → mu3io → WinUSB.io → ESP32
```

## Mu3hook处理

### 1. IO4 LED处理 (board == 1)
**文件**: `GAMEHOOK/mu3hook/io4.c`

```c
// 在mu3_io4_write_gpio函数中
uint8_t rgb_out[6 * 3] = {
    lights_data & MU3_IO_LED_L1_R ? 0xFF : 0x00,  // L1 R
    lights_data & MU3_IO_LED_L1_G ? 0xFF : 0x00,  // L1 G
    lights_data & MU3_IO_LED_L1_B ? 0xFF : 0x00,  // L1 B
    lights_data & MU3_IO_LED_L2_R ? 0xFF : 0x00,  // L2 R
    lights_data & MU3_IO_LED_L2_G ? 0xFF : 0x00,  // L2 G
    lights_data & MU3_IO_LED_L2_B ? 0xFF : 0x00,  // L2 B
    lights_data & MU3_IO_LED_L3_R ? 0xFF : 0x00,  // L3 R
    lights_data & MU3_IO_LED_L3_G ? 0xFF : 0x00,  // L3 G
    lights_data & MU3_IO_LED_L3_B ? 0xFF : 0x00,  // L3 B
    lights_data & MU3_IO_LED_R1_R ? 0xFF : 0x00,  // R1 R
    lights_data & MU3_IO_LED_R1_G ? 0xFF : 0x00,  // R1 G
    lights_data & MU3_IO_LED_R1_B ? 0xFF : 0x00,  // R1 B
    lights_data & MU3_IO_LED_R2_R ? 0xFF : 0x00,  // R2 R
    lights_data & MU3_IO_LED_R2_G ? 0xFF : 0x00,  // R2 G
    lights_data & MU3_IO_LED_R2_B ? 0xFF : 0x00,  // R2 B
    lights_data & MU3_IO_LED_R3_R ? 0xFF : 0x00,  // R3 R
    lights_data & MU3_IO_LED_R3_G ? 0xFF : 0x00,  // R3 G
    lights_data & MU3_IO_LED_R3_B ? 0xFF : 0x00,  // R3 B
};

mu3_dll.led_set_leds(1, rgb_out);  // board = 1
```

**数据格式**: 18字节，包含L1,L2,L3,R1,R2,R3的RGB数据

### 2. Cab LED处理 (board == 0)
**文件**: `GAMEHOOK/mu3hook/dllmain.c`

```c
unsigned int led_port_no[2] = {3, 0};
hr = led15093_hook_init(&mu3_hook_cfg.led15093, 
    mu3_dll.led_init, mu3_dll.led_set_leds, led_port_no);
```

**注意**: mu3hook使用led15093_hook来处理board 0的LED，但具体实现不在当前代码中。

## Mu3io处理

### 1. LED数据长度定义
**文件**: `GAMEHOOK/mu3io/leddata.h`

```c
static uint8_t ongeki_led_board_data_lens[LED_BOARDS_TOTAL] = {9*3, 6*3};
```

- **Board 0 (Cab)**: 27字节 (9个LED × 3字节RGB)
- **Board 1 (IO4)**: 18字节 (6个LED × 3字节RGB)

### 2. LED输出更新
**文件**: `GAMEHOOK/mu3io/ledoutput.c`

```c
void mu3_led_output_update(int board, const uint8_t* rgb)
{
    if (board == 0) {
        // cab - 处理27字节数据
        if (mu3_io_config->cab_led_output_pipe) {
            mu3_led_pipe_update(escaped_data);
        }
    } else {
        // slider/IO4 - 处理18字节数据
        if (mu3_io_config->controller_led_output_pipe) {
            mu3_led_pipe_update(escaped_data);
        }
    }
}
```

### 3. LED设置函数
**文件**: `GAMEHOOK/mu3io/mu3io.c`

```c
void mu3_io_led_set_colors(uint8_t board, uint8_t *rgb)
{
    mu3_led_output_update(board, rgb);
}
```

## WinUSB.io处理

### 1. Interface.cs
**Board 1 (IO4)处理**:
```csharp
// 数据格式：L1, L2, L3, R1, R2, R3 (每个LED占用3字节RGB)
ReadOnlySpan<byte> leds = new(rgb, 3 * 6);
_connection.SetIO4Leds(leds);
```

**Board 0 (Cab)处理**:
```csharp
// 从27字节数据中提取LS和RS
byte[] data = new byte[6]; // LS, RS (每个3字节RGB)

// LS数据 - 从索引6开始 (第3个LED)
leds[6..9].CopyTo(new Span<byte>(data, 0, 3));

// RS数据 - 从索引24开始 (第9个LED)
leds[24..27].CopyTo(new Span<byte>(data, 3, 3));

_connection.SetSideLeds(data);
```

## 数据映射验证

### 1. IO4 LED数据 (Board 1)
**来源**: mu3hook的IO4 GPIO写入
**数据**: L1,L2,L3,R1,R2,R3 (18字节)
**目标**: WinUSB.io的SetIO4Leds

### 2. Side LED数据 (Board 0)
**来源**: mu3hook的led15093处理
**数据**: 27字节，包含9个LED的RGB数据
**提取**: WinUSB.io从索引6和24提取LS和RS
**目标**: WinUSB.io的SetSideLeds

## 潜在问题分析

### 1. Board 0数据提取
**问题**: WinUSB.io从27字节数据中提取LS和RS，但需要确认：
- 索引6是否真的是LS的位置
- 索引24是否真的是RS的位置

### 2. 数据同步
**问题**: SetIO4Leds和SetSideLeds是独立调用的，可能导致数据不同步

### 3. 数据长度不匹配
**问题**: 
- mu3io期望board 0有27字节
- WinUSB.io只提取6字节(LS,RS)
- 其他21字节数据被忽略

## 建议

1. **验证Board 0数据格式**: 需要确认27字节数据中LS和RS的确切位置
2. **数据同步机制**: 确保IO4和Side LED数据能正确组合
3. **测试数据流**: 从游戏到ESP32的完整数据流测试

## 结论

当前的WinUSB.io和游戏RGB交互基本正确，但需要验证：
1. Board 0数据中LS和RS的位置是否正确
2. 数据同步机制是否有效
3. 完整数据流的端到端测试 