# 扩展Side LED映射方案

## 问题描述
根据用户观察，mu3io的Side LED数据中：
- LED 0 对应 LS1和RS1
- LED 1 对应 LS2和RS2
- LED 2-8 是跑马灯装饰，暂时忽略

需要将Side LED映射扩展到4个LED，充分利用LED 0和LED 1的数据。

## 新的LED映射方案

### 映射关系
```
LED_1 (左侧) - 5个LED:
- LED_1[0]: L3 (左侧第3个) - 来自IO4数据
- LED_1[1]: L2 (左侧第2个) - 来自IO4数据  
- LED_1[2]: L1 (左侧第1个) - 来自IO4数据
- LED_1[3]: LS1 (左侧Side 1) - 来自Side LED 0
- LED_1[4]: LS2 (左侧Side 2) - 来自Side LED 1

LED_5 (右侧) - 5个LED:
- LED_5[0]: R1 (右侧第1个) - 来自IO4数据
- LED_5[1]: R2 (右侧第2个) - 来自IO4数据
- LED_5[2]: R3 (右侧第3个) - 来自IO4数据
- LED_5[3]: RS1 (右侧Side 1) - 来自Side LED 0
- LED_5[4]: RS2 (右侧Side 2) - 来自Side LED 1
```

### 数据来源
```
IO4 LED数据 (18字节):
- L1: input->io4_leds[0-2]   (RGB)
- L2: input->io4_leds[3-5]   (RGB)
- L3: input->io4_leds[6-8]   (RGB)
- R1: input->io4_leds[9-11]  (RGB)
- R2: input->io4_leds[12-14] (RGB)
- R3: input->io4_leds[15-17] (RGB)

Side LED数据 (6字节):
- LED 0: input->side_leds[0-2] (RGB) - 用于LS1和RS1
- LED 1: input->side_leds[3-5] (RGB) - 用于LS2和RS2
```

## 实现细节

### 1. LED_1映射数组
```c
const int led_1_mapping[5] = {2, 1, 0, -1, -2}; 
// L3, L2, L1, LS1(-1), LS2(-2)
```

### 2. LED_5映射逻辑
```c
for (int i = 0; i < 5; i++) {
    if (i < 3) {
        // R1, R2, R3从IO4数据获取
        int source_idx = (i + 3) * 3;
        r = input->io4_leds[source_idx];
        g = input->io4_leds[source_idx + 1];
        b = input->io4_leds[source_idx + 2];
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
    }
}
```

### 3. LED_1映射逻辑
```c
for (int i = 0; i < 5; i++) {
    if (led_1_mapping[i] >= 0) {
        // 从IO4数据获取 (L3, L2, L1)
        int source_idx = led_1_mapping[i] * 3;
        r = input->io4_leds[source_idx];
        g = input->io4_leds[source_idx + 1];
        b = input->io4_leds[source_idx + 2];
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
}
```

## 硬件支持

### LED配置
- **LED_1**: 配置8个LED，使用5个 (GPIO_48)
- **LED_5**: 配置8个LED，使用5个 (GPIO_41)

### 硬件初始化
```c
void LED_init(void)
{
    led_1 = configure_led(LED_1, 8); // 支持8个LED
    led_5 = configure_led(LED_5, 8); // 支持8个LED
}
```

## 预期效果

### 1. 充分利用Side LED数据
- LED 0的数据同时用于LS1和RS1
- LED 1的数据同时用于LS2和RS2
- 避免数据浪费

### 2. 扩展LED显示
- 左侧：L3, L2, L1, LS1, LS2 (5个LED)
- 右侧：R1, R2, R3, RS1, RS2 (5个LED)
- 总共10个LED，提供更丰富的视觉效果

### 3. 保持数据一致性
- LS1和RS1使用相同的数据源 (LED 0)
- LS2和RS2使用相同的数据源 (LED 1)
- 符合游戏的设计逻辑

## 测试步骤

### 1. 编译和烧录
1. 重新编译ESP32固件
2. 烧录到设备

### 2. 测试IO4 LED
1. 在游戏中改变主要按键颜色
2. 验证L1-L3, R1-R3正确显示

### 3. 测试Side LED
1. 在游戏中改变Side LED设置
2. 观察日志：
   ```
   LED_1[3] (mapped from Side[LED 0] - LS1): RGB(255,0,0)
   LED_1[4] (mapped from Side[LED 1] - LS2): RGB(0,255,0)
   LED_5[3] (mapped from Side[LED 0] - RS1): RGB(255,0,0)
   LED_5[4] (mapped from Side[LED 1] - RS2): RGB(0,255,0)
   ```
3. 验证LS1/RS1和LS2/RS2正确显示

### 4. 验证数据一致性
1. 确认LS1和RS1显示相同颜色
2. 确认LS2和RS2显示相同颜色
3. 确认颜色变化与游戏设置一致

## 日志输出示例

### 正常数据接收
```
I (1234) WINUSB: === Fast LED Processing ===
I (1234) WINUSB: LED packet validated, header: 44 4C 01
I (1244) WINUSB: IO4 LED: 00 FF 00 00 FF 00
I (1254) WINUSB: Side LED: FF 00 00 00 FF 00
I (1264) WINUSB: LED_1[0] (mapped from IO4[2]): RGB(0,255,0)
I (1274) WINUSB: LED_1[1] (mapped from IO4[1]): RGB(0,255,0)
I (1284) WINUSB: LED_1[2] (mapped from IO4[0]): RGB(0,255,0)
I (1294) WINUSB: LED_1[3] (mapped from Side[LED 0] - LS1): RGB(255,0,0)
I (1304) WINUSB: LED_1[4] (mapped from Side[LED 1] - LS2): RGB(0,255,0)
I (1314) WINUSB: LED_5[0] (mapped from IO4[3]): RGB(0,0,0)
I (1324) WINUSB: LED_5[1] (mapped from IO4[4]): RGB(0,0,0)
I (1334) WINUSB: LED_5[2] (mapped from IO4[5]): RGB(0,0,0)
I (1344) WINUSB: LED_5[3] (mapped from Side[LED 0] - RS1): RGB(255,0,0)
I (1354) WINUSB: LED_5[4] (mapped from Side[LED 1] - RS2): RGB(0,255,0)
```

## 潜在问题

### 1. LED数量限制
**问题**：如果硬件只支持4个LED
**解决方案**：检查硬件规格，必要时调整映射

### 2. 数据格式变化
**问题**：如果Side LED数据格式发生变化
**解决方案**：更新数据解析逻辑

### 3. 性能影响
**问题**：增加LED数量可能影响刷新性能
**解决方案**：优化刷新频率或使用异步刷新

## 结论

通过扩展Side LED映射，我们：

1. **充分利用数据**：LED 0和LED 1的数据分别用于LS1/RS1和LS2/RS2
2. **扩展显示效果**：从4个LED扩展到5个LED，提供更丰富的视觉效果
3. **保持一致性**：LS1/RS1和LS2/RS2使用相同的数据源，符合游戏逻辑
4. **硬件兼容**：LED_1和LED_5都支持8个LED，5个LED完全可行

现在Side LED映射已经扩展到充分利用mu3io提供的数据了！ 