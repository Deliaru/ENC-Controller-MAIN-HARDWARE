# LED映射问题分析

## 问题现象

### 当前日志显示
```
I (159639) WINUSB: IO4 LED Full Data:
I (159649) WINUSB:   [00-05]: FF 00 00 FF 00 00  (L1红色, L2红色)
I (159649) WINUSB:   [06-11]: FF 00 00 FF 00 00  (L3红色, LS红色)  
I (159659) WINUSB:   [12-17]: FF 00 00 FF 00 00  (R1红色, R2红色)
I (159659) WINUSB: Side LED Full Data:
I (159659) WINUSB:   [00-05]: 00 00 00 00 00 00  (全是0)
I (159669) WINUSB:   [06-11]: 00 00 00 00 00 00  (全是0)
```

### 问题分析
1. **Side LED数据全是0** - Ontroller.WinUSB.IO的修复还没有生效
2. **LED映射不正确** - 需要重新理解数据源分配

## 数据源重新分析

### mu3io数据格式
根据`leddata.h`：
- Board 1 (IO4): `6*3 = 18字节` (L1, L2, L3, R1, R2, R3)
- Board 0 (Side): `9*3 = 27字节` (包含多个LED)

### 实际数据分布
从日志看，IO4数据包含：
- 索引0-2: L1 RGB
- 索引3-5: L2 RGB
- 索引6-8: L3 RGB
- 索引9-11: LS RGB
- 索引12-14: R1 RGB
- 索引15-17: R2 RGB

### 用户要求的LED映射
- **LED_1[0~3]**: L3, L2, L1, LS
- **LED_5[0~3]**: R1, R2, R3, RS

## 修正方案

### 新的LED映射关系

#### LED_1 (GPIO_48) - 4个LED
- LED_1[0]: L3 ← IO4数据索引2 (6-8)
- LED_1[1]: L2 ← IO4数据索引1 (3-5)
- LED_1[2]: L1 ← IO4数据索引0 (0-2)
- LED_1[3]: LS ← IO4数据索引3 (9-11)

#### LED_5 (GPIO_41) - 4个LED
- LED_5[0]: R1 ← IO4数据索引4 (12-14)
- LED_5[1]: R2 ← IO4数据索引5 (15-17)
- LED_5[2]: R3 ← Side数据索引0 (0-2)
- LED_5[3]: RS ← Side数据索引1 (3-5)

## 代码实现

### LED_1映射
```c
const int led_1_mapping[4] = {2, 1, 0, 3}; // L3, L2, L1, LS
for (int i = 0; i < 4; i++) {
    int source_idx = led_1_mapping[i] * 3;
    // 从IO4数据获取
    r = input->io4_leds[source_idx];
    g = input->io4_leds[source_idx + 1];
    b = input->io4_leds[source_idx + 2];
}
```

### LED_5映射
```c
for (int i = 0; i < 4; i++) {
    if (i < 2) {
        // R1, R2从IO4数据获取
        int source_idx = (i + 4) * 3;
        r = input->io4_leds[source_idx];
        g = input->io4_leds[source_idx + 1];
        b = input->io4_leds[source_idx + 2];
    } else {
        // R3, RS从Side数据获取
        int source_idx = (i - 2) * 3;
        r = input->side_leds[source_idx];
        g = input->side_leds[source_idx + 1];
        b = input->side_leds[source_idx + 2];
    }
}
```

## 预期效果

### 修复前
```
LED_1[0]: RGB(255,0,0)  // L3 - 正确
LED_1[1]: RGB(255,0,0)  // L2 - 正确
LED_1[2]: RGB(255,0,0)  // L1 - 正确
LED_1[3]: RGB(0,0,0)    // LS - 错误(应该是红色)
LED_5[0]: RGB(0,0,0)    // R1 - 错误(应该是红色)
LED_5[1]: RGB(0,0,0)    // R2 - 错误(应该是红色)
LED_5[2]: RGB(0,0,0)    // R3 - 错误(应该是红色)
LED_5[3]: RGB(0,0,0)    // RS - 错误(应该是红色)
```

### 修复后
```
LED_1[0]: RGB(255,0,0)  // L3 - 正确
LED_1[1]: RGB(255,0,0)  // L2 - 正确
LED_1[2]: RGB(255,0,0)  // L1 - 正确
LED_1[3]: RGB(255,0,0)  // LS - 正确
LED_5[0]: RGB(255,0,0)  // R1 - 正确
LED_5[1]: RGB(255,0,0)  // R2 - 正确
LED_5[2]: RGB(255,0,0)  // R3 - 正确
LED_5[3]: RGB(255,0,0)  // RS - 正确
```

## 待解决问题

1. **Side LED数据为0** - 需要确保Ontroller.WinUSB.IO的修复生效
2. **重新编译测试** - 验证新的LED映射是否正确
3. **数据源验证** - 确认Side LED数据的实际来源

## 下一步

1. 重新编译ESP32固件
2. 测试新的LED映射
3. 如果Side LED仍为0，检查Ontroller.WinUSB.IO的编译和运行 