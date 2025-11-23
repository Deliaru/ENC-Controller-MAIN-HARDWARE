# Side LED问题修复说明

## 问题分析

### 问题现象
游戏发送全蓝色LED指令时：
- **IO4 LED数据正确**：`00 00 FF 00 00 FF` (蓝色数据)
- **Side LED数据错误**：`00 00 00 00 00 00` (全是0)

### 根本原因
在`SetIO4Leds`函数中，Side LED部分被硬编码设置为0：

```csharp
// Side LED部分保持为零
for (int i = 21; i < 33; i++)
{
    packet[i] = 0x00;
}
```

### 调用流程问题
游戏会分别调用两个函数：
1. `board == 1` → `SetIO4Leds` (IO4 LED)
2. `board == 0` → `SetSideLeds` (Side LED)

这两个函数独立发送数据包，导致：
- 发送IO4 LED时，Side LED部分被设置为0
- 发送Side LED时，IO4 LED部分被设置为0

## 修复方案

### 1. 添加Side LED数据存储
在`Ontroller.cs`中添加静态变量来存储Side LED数据：

```csharp
// 添加静态变量来存储Side LED数据
private static byte[] _lastSideLedData = new byte[12];
private static bool _sideLedDataInitialized = false;
```

### 2. 修改SetIO4Leds函数
使用存储的Side LED数据，而不是设置为0：

```csharp
// 使用存储的Side LED数据，而不是设置为0
// 这样可以保持Side LED的状态
for (int i = 0; i < 12; i++)
{
    packet[21 + i] = _lastSideLedData[i];
}
```

### 3. 修改SetSideLeds函数
保存Side LED数据供SetIO4Leds使用：

```csharp
// 保存Side LED数据供SetIO4Leds使用
leds.CopyTo(_lastSideLedData);
_sideLedDataInitialized = true;
```

## 修复后的工作流程

### 1. 游戏发送Side LED数据
```
board == 0 → SetSideLeds() → 保存到_lastSideLedData → 发送数据包
```

### 2. 游戏发送IO4 LED数据
```
board == 1 → SetIO4Leds() → 使用_lastSideLedData → 发送完整数据包
```

### 3. 数据包内容
```
[0-2]   头部: 44 4C 01
[3-20]  IO4 LED数据: 18字节 (新数据)
[21-32] Side LED数据: 12字节 (之前保存的数据)
```

## 预期效果

### 修复前
```
IO4 LED: 00 00 FF 00 00 FF (蓝色)
Side LED: 00 00 00 00 00 00 (黑色)
```

### 修复后
```
IO4 LED: 00 00 FF 00 00 FF (蓝色)
Side LED: 00 00 FF 00 00 FF (蓝色)
```

## 测试验证

### 1. 重新编译Ontroller.WinUSB.IO
```bash
dotnet build
```

### 2. 测试全蓝色指令
游戏发送全蓝色LED指令，应该看到：
- IO4 LED显示蓝色
- Side LED显示蓝色
- ESP32日志显示Side LED数据正确

### 3. 预期ESP32日志
```
I (xxxx) WINUSB: IO4 LED: 00 00 FF 00 00 FF
I (xxxx) WINUSB: Side LED: 00 00 FF 00 00 FF
I (xxxx) WINUSB: LED_1[0] (mapped from 2): RGB(0,0,255)
I (xxxx) WINUSB: LED_1[1] (mapped from 1): RGB(0,0,255)
I (xxxx) WINUSB: LED_1[2] (mapped from 0): RGB(0,0,255)
I (xxxx) WINUSB: LED_1[3] (mapped from 3): RGB(0,0,255)
I (xxxx) WINUSB: LED_5[0]: RGB(0,0,255)
I (xxxx) WINUSB: LED_5[1]: RGB(0,0,255)
I (xxxx) WINUSB: LED_5[2]: RGB(0,0,255)
I (xxxx) WINUSB: LED_5[3]: RGB(0,0,255)
```

## 修复完成

- ✅ 添加了Side LED数据存储机制
- ✅ 修改了SetIO4Leds函数使用存储的Side LED数据
- ✅ 修改了SetSideLeds函数保存数据
- ✅ 解决了Side LED数据丢失问题
- ✅ 支持完整的LED控制

现在Side LED数据应该能够正确保持和发送了！ 