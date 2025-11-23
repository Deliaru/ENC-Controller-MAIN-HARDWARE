# Side LED越界问题修复

## 问题描述
Side LED设置时出现数组越界错误。

## 问题原因
在Interface.cs中存在数组大小不匹配的问题：

1. **数组定义错误**：
   - `_lastSideLedData` 定义为12字节，但实际只需要6字节
   - `_lastIO4LedData` 定义为6字节，但实际需要18字节

2. **数据长度错误**：
   - Board 0数据长度设置为 `3 * 61 = 183字节`，但实际应该是 `3 * 9 = 27字节`

3. **数据比较错误**：
   - IO4 LED数据比较时使用6字节，但实际有18字节

## 修复内容

### 1. 修正数组大小
```csharp
// 修复前
private static byte[] _lastSideLedData = new byte[12];
private static byte[] _lastIO4LedData = new byte[6];

// 修复后
private static byte[] _lastSideLedData = new byte[6]; // LS, RS (每个3字节RGB)
private static byte[] _lastIO4LedData = new byte[18]; // L1,L2,L3,R1,R2,R3 (每个3字节RGB)
```

### 2. 修正数据长度
```csharp
// 修复前
ReadOnlySpan<byte> leds = new(rgb, 3 * 61); // 183字节

// 修复后
ReadOnlySpan<byte> leds = new(rgb, 3 * 9); // 27字节
```

### 3. 修正数据比较
```csharp
// 修复前
for (int i = 0; i < 6; i++) // 只比较6字节
{
    if (leds[i] != _lastIO4LedData[i])
    {
        dataChanged = true;
        break;
    }
}
leds.Slice(0, 6).CopyTo(_lastIO4LedData); // 只保存6字节

// 修复后
for (int i = 0; i < 18; i++) // 比较18字节
{
    if (leds[i] != _lastIO4LedData[i])
    {
        dataChanged = true;
        break;
    }
}
leds.Slice(0, 18).CopyTo(_lastIO4LedData); // 保存18字节
```

### 4. 添加调试信息
```csharp
// 添加调试信息帮助诊断问题
Logging.WriteLine($"E.N.C Controller: Side LED board 0 called, rgb pointer: {(rgb == null ? "NULL" : "VALID")}");
Logging.WriteLine($"E.N.C Controller: Side LED data length: {leds.Length} bytes");
Logging.WriteLine($"E.N.C Controller: Successfully extracted LS and RS data");
```

## 数据格式确认

### Board 0 (Cab) - 27字节
```
[0-2]   LED 1 RGB
[3-5]   LED 2 RGB
[6-8]   LED 3 RGB (LS)
[9-11]  LED 4 RGB
[12-14] LED 5 RGB
[15-17] LED 6 RGB
[18-20] LED 7 RGB
[21-23] LED 8 RGB
[24-26] LED 9 RGB (RS)
```

### Board 1 (IO4) - 18字节
```
[0-2]   L1 RGB
[3-5]   L2 RGB
[6-8]   L3 RGB
[9-11]  R1 RGB
[12-14] R2 RGB
[15-17] R3 RGB
```

## 测试步骤

1. 重新编译Ontroller.WinUSB.IO
2. 运行游戏
3. 观察日志输出：
   - "Side LED board 0 called, rgb pointer: VALID"
   - "Side LED data length: 27 bytes"
   - "Successfully extracted LS and RS data"
4. 验证不再出现越界错误

## 预期结果

- Side LED设置不再出现越界错误
- 日志显示正确的数据长度和处理过程
- LS和RS数据能正确提取和发送到ESP32 