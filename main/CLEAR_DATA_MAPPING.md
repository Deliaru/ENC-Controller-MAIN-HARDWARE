# 清晰的数据映射设计

## 数据源分配

### 从游戏获取的RGB信息
- **L1~3 + R1~3**: 填充到IO4 LED数据中 (18字节)
- **LS + RS**: 填充到Side LED数据中 (6字节)

## 数据包结构 (33字节)

```
[0-2]   头部: 44 4C 01 (固定)
[3-20]  IO4 LED数据: 18字节 (L1, L2, L3, R1, R2, R3)
[21-26] Side LED数据: 6字节 (LS, RS)
[27-32] 保留: 6字节 (未使用)
```

## LED映射关系

### IO4 LED (led_1, GPIO_48) - 4个LED
- **LED_1[0]**: L3 ← IO4数据索引2 (6-8字节)
- **LED_1[1]**: L2 ← IO4数据索引1 (3-5字节)
- **LED_1[2]**: L1 ← IO4数据索引0 (0-2字节)
- **LED_1[3]**: LS ← Side数据索引0 (21-23字节)

### Side LED (led_5, GPIO_41) - 4个LED
- **LED_5[0]**: R1 ← IO4数据索引3 (9-11字节)
- **LED_5[1]**: R2 ← IO4数据索引4 (12-14字节)
- **LED_5[2]**: R3 ← IO4数据索引5 (15-17字节)
- **LED_5[3]**: RS ← Side数据索引1 (24-26字节)

## 代码实现

### Ontroller.WinUSB.IO - Interface.cs

#### IO4 LED处理 (board == 1)
```csharp
// 数据格式：L1, L2, L3, R1, R2, R3 (每个LED占用3字节RGB)
ReadOnlySpan<byte> leds = new(rgb, 3 * 6);
_connection.SetIO4Leds(leds);
```

#### Side LED处理 (board == 0)
```csharp
// 从27字节数据中提取LS和RS
byte[] data = new byte[6]; // LS, RS (每个3字节RGB)

// LS数据 - 从索引6开始 (第3个LED)
leds[6..9].CopyTo(new Span<byte>(data, 0, 3));

// RS数据 - 从索引24开始 (第9个LED)
leds[24..27].CopyTo(new Span<byte>(data, 3, 3));

_connection.SetSideLeds(data);
```

### Ontroller.WinUSB.IO - Ontroller.cs

#### SetIO4Leds函数
```csharp
// 复制IO4 LED数据 (L1, L2, L3, R1, R2, R3)
for (int i = 0; i < 18; i++)
{
    packet[3 + i] = leds[i];
}

// 复制Side LED数据 (LS, RS)
for (int i = 0; i < 6; i++)
{
    packet[21 + i] = _lastSideLedData[i];
}
```

#### SetSideLeds函数
```csharp
// 复制Side LED数据 (LS, RS)
for (int i = 0; i < 6; i++)
{
    packet[21 + i] = leds[i];
}
```

### ESP32 - winusb_new.c

#### LED_1映射
```c
const int led_1_mapping[4] = {2, 1, 0, -1}; // L3, L2, L1, LS

for (int i = 0; i < 4; i++) {
    if (led_1_mapping[i] >= 0) {
        // 从IO4数据获取 (L3, L2, L1)
        int source_idx = led_1_mapping[i] * 3;
        r = input->io4_leds[source_idx];
        g = input->io4_leds[source_idx + 1];
        b = input->io4_leds[source_idx + 2];
    } else {
        // 从Side数据获取 (LS)
        r = input->side_leds[0];
        g = input->side_leds[1];
        b = input->side_leds[2];
    }
}
```

#### LED_5映射
```c
for (int i = 0; i < 4; i++) {
    if (i < 3) {
        // R1, R2, R3从IO4数据获取 (索引3, 4, 5)
        int source_idx = (i + 3) * 3;
        r = input->io4_leds[source_idx];
        g = input->io4_leds[source_idx + 1];
        b = input->io4_leds[source_idx + 2];
    } else {
        // RS从Side数据获取 (索引1)
        r = input->side_leds[3];
        g = input->side_leds[4];
        b = input->side_leds[5];
    }
}
```

## 数据流向

### 1. 游戏发送数据
```
游戏 → mu3io → Interface.cs → Ontroller.cs → ESP32
```

### 2. 数据转换
```
Board 1 (IO4): L1,L2,L3,R1,R2,R3 → SetIO4Leds()
Board 0 (Side): LS,RS → SetSideLeds()
```

### 3. 数据包发送
```
SetIO4Leds: [IO4数据] + [存储的Side数据]
SetSideLeds: [零] + [Side数据]
```

### 4. ESP32接收
```
IO4数据: L1,L2,L3,R1,R2,R3
Side数据: LS,RS
```

### 5. LED设置
```
LED_1: L3,L2,L1,LS
LED_5: R1,R2,R3,RS
```

## 预期效果

当游戏发送全红色时：
```
IO4数据: FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00
Side数据: FF 00 00 FF 00 00

LED_1[0]: RGB(255,0,0)  // L3
LED_1[1]: RGB(255,0,0)  // L2
LED_1[2]: RGB(255,0,0)  // L1
LED_1[3]: RGB(255,0,0)  // LS

LED_5[0]: RGB(255,0,0)  // R1
LED_5[1]: RGB(255,0,0)  // R2
LED_5[2]: RGB(255,0,0)  // R3
LED_5[3]: RGB(255,0,0)  // RS
```

## 测试步骤

1. 重新编译Ontroller.WinUSB.IO
2. 重新编译ESP32固件
3. 游戏发送全红色LED指令
4. 验证所有8个LED都显示红色 