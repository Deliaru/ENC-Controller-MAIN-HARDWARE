# SharedLedBuffer越界问题修复

## 问题描述
Side LED设置时出现"Offset and length were out of bounds"错误。

## 问题原因
`SharedLedBuffer`类的`SetData`方法期望12字节数据，但传入的是6字节数据，导致越界。

## 错误分析

### 错误发生位置
```csharp
// 在Interface.cs中
_ledBuffer.SetData(data); // data是6字节，但SharedLedBuffer期望12字节
```

### 原始SharedLedBuffer实现
```csharp
internal class SharedLedBuffer
{
    byte[] _buffer = new byte[12]; // 12字节缓冲区
    
    public void SetData(byte[] data)
    {
        _accessor.WriteArray(0, data, 0, 12); // 强制写入12字节
    }
}
```

## 修复内容

### 1. 移除不必要的调用
```csharp
// 修复前
_ledBuffer.SetData(data);
_connection.SetSideLeds(data);

// 修复后
_connection.SetSideLeds(data); // 直接调用，不需要SharedLedBuffer
```

### 2. 修复SharedLedBuffer大小
```csharp
// 修复前
byte[] _buffer = new byte[12];
_file = MemoryMappedFile.CreateOrOpen("ontroller_ipc_led_buffer", 12);
_accessor.WriteArray(0, data, 0, 12);

// 修复后
byte[] _buffer = new byte[6]; // 改为6字节：LS, RS
_file = MemoryMappedFile.CreateOrOpen("ontroller_ipc_led_buffer", 6);
if (data.Length <= 6)
{
    _accessor.WriteArray(0, data, 0, data.Length);
}
else
{
    Logging.WriteLine($"Ontroller: Warning: LED data too large ({data.Length} bytes), truncating to 6 bytes");
    _accessor.WriteArray(0, data, 0, 6);
}
```

## 数据流分析

### 当前数据流
```
游戏 → mu3io → Interface.cs → Ontroller.cs → ESP32
```

### SharedLedBuffer的作用
- 原本用于进程间通信(IPC)
- 存储Side LED数据供其他进程读取
- 在当前实现中不是必需的

### 修复后的数据流
```
游戏 → mu3io → Interface.cs → Ontroller.cs → ESP32
                ↓
            SharedLedBuffer (可选，用于IPC)
```

## 测试步骤

1. 重新编译Ontroller.WinUSB.IO
2. 运行游戏
3. 观察日志输出：
   - "Side LED board 0 called, rgb pointer: VALID"
   - "Side LED data length: 27 bytes"
   - "Successfully extracted LS and RS data"
   - 不再出现越界错误
4. 验证Side LED数据正确发送到ESP32

## 预期结果

- Side LED设置不再出现越界错误
- LS和RS数据能正确提取和发送
- SharedLedBuffer大小与数据匹配
- 日志显示正确的处理过程

## 注意事项

1. **SharedLedBuffer用途**：主要用于进程间通信，如果不需要IPC功能可以完全移除
2. **数据大小**：Side LED数据固定为6字节(LS, RS各3字节)
3. **错误处理**：添加了数据长度检查和截断处理 