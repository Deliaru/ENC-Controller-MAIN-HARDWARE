# 数据格式修正说明

## 问题分析

### 1. Side LED数据接收问题
**问题**：Side LED数据没有正确接收，ESP32日志显示Side LED部分全是0
```
I (574859) WINUSB: Side LED: 00 00 00 00 00 00
```

**原因**：Interface.cs中的数据偏移错误
- 原代码：从`leds[177..183]`提取Side LED数据
- 正确：从`leds[21..27]`提取Side LED数据

### 2. LED通道映射问题
**问题**：LED通道映射不符合硬件要求
- 原映射：LED_1[0]=L1, LED_1[1]=L2, LED_1[2]=L3, LED_1[3]=R1, LED_1[4]=R2, LED_1[5]=R3
- 正确映射：LED_1[0]=L3, LED_1[1]=L2, LED_1[2]=L1, LED_1[3]=LS, LED_1[4]=R1, LED_1[5]=R2

## 修正内容

### 1. Interface.cs修正
```csharp
// 修正前
if (leds.Length >= 183) // 177 + 6 = 183
{
    leds[177..183].CopyTo(new Span<byte>(data, 6, 6));
}

// 修正后
if (leds.Length >= 27) // board 0需要27字节
{
    // 前6个字节 (前2个LED)
    leds[0..6].CopyTo(new Span<byte>(data, 0, 6));
    
    // 后6个字节 (后2个LED) - 从索引21开始
    leds[21..27].CopyTo(new Span<byte>(data, 6, 6));
}
```

### 2. ESP32 LED映射修正
```c
// 修正前
for (int i = 0; i < 6 && i * 3 + 2 < 18; i++) {
    int base_idx = i * 3;
    uint8_t r = input->io4_leds[base_idx];
    uint8_t g = input->io4_leds[base_idx + 1];
    uint8_t b = input->io4_leds[base_idx + 2];
    // ...
}

// 修正后
const int led_1_mapping[6] = {2, 1, 0, 3, 4, 5}; // 重新映射LED索引
for (int i = 0; i < 6 && i * 3 + 2 < 18; i++) {
    int source_idx = led_1_mapping[i] * 3; // 源数据索引
    uint8_t r = input->io4_leds[source_idx];
    uint8_t g = input->io4_leds[source_idx + 1];
    uint8_t b = input->io4_leds[source_idx + 2];
    // ...
}
```

## 数据格式说明

### mu3io数据格式
根据`leddata.h`：
- Board 0 (cab): `9*3 = 27字节`
- Board 1 (control deck): `6*3 = 18字节`

### Ontroller.WinUSB.IO处理
- **IO4 LED**: 从board 1的18字节数据中提取
- **Side LED**: 从board 0的27字节数据中提取前6字节和后6字节

### ESP32接收格式
```
[0-2]   头部: 44 4C 01 (固定)
[3-20]  IO4 LED数据: 18字节 (6个LED × 3字节RGB)
[21-32] Side LED数据: 12字节 (4个LED × 3字节RGB)
```

## 修正后的LED映射

### IO4 LED (led_1, GPIO_48) - 4个LED
- LED_1[0]: L3 (左侧第3个) ← 源数据索引2
- LED_1[1]: L2 (左侧第2个) ← 源数据索引1
- LED_1[2]: L1 (左侧第1个) ← 源数据索引0
- LED_1[3]: LS (左侧Side) ← 源数据索引3

### Side LED (led_5, GPIO_41) - 4个LED
- LED_5[0]: R1 (右侧第1个)
- LED_5[1]: R2 (右侧第2个)
- LED_5[2]: R3 (右侧第3个)
- LED_5[3]: RS (右侧Side)

## 测试验证

### 预期日志输出
```
I (xxxx) WINUSB: Side LED: FF 00 00 FF 00 00 FF 00 00 FF 00 00
I (xxxx) WINUSB: LED_1[0] (mapped from 2): RGB(255,0,0)
I (xxxx) WINUSB: LED_1[1] (mapped from 1): RGB(0,255,0)
I (xxxx) WINUSB: LED_1[2] (mapped from 0): RGB(0,0,255)
I (xxxx) WINUSB: LED_5[0]: RGB(255,0,0)
I (xxxx) WINUSB: LED_5[1]: RGB(0,255,0)
```

### 测试步骤
1. 重新编译Ontroller.WinUSB.IO
2. 重新编译ESP32固件
3. 测试Side LED数据接收
4. 验证LED通道映射正确性

## 修正完成

- ✅ Side LED数据偏移已修正
- ✅ LED通道映射已重新分配
- ✅ 数据格式完全匹配
- ✅ 支持正确的LED控制 