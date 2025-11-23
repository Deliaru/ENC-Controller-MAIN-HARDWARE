# 进程间通信解决方案

## 问题描述
Side LED数据从`mu3`进程丢失，因为只有`amdaemon`进程有USB连接。需要通过进程间通信将Side LED数据从`mu3`进程传递给`amdaemon`进程。

## 解决方案

### 方案：共享内存通信 ✅ (已实施)

#### 架构设计
```
mu3进程 (游戏)     → 共享内存 → amdaemon进程 → USB连接 → ESP32
amdaemon进程 (IO)  → 直接USB连接 → ESP32
```

#### 数据流
1. **mu3进程**：接收Side LED数据 → 写入共享内存
2. **amdaemon进程**：定期检查共享内存 → 读取新数据 → 发送到USB

### 实现细节

#### 1. SharedLedBuffer类修复 ✅
```csharp
// 修复了GetData()方法的bug
public ReadOnlySpan<byte> GetData()
{
    _accessor.ReadArray(0, _buffer, 0, 6); // 修复：读取6字节而不是12字节
    return _buffer;
}
```

#### 2. Side LED处理逻辑 ✅
```csharp
// 进程间通信处理
if (processName.ToLower().Contains("amdaemon"))
{
    // amdaemon进程：直接发送到USB
    if (_connection != null)
    {
        _connection.SetSideLeds(data);
    }
}
else
{
    // 非amdaemon进程：通过共享内存传递给amdaemon进程
    if (_ledBuffer != null)
    {
        _ledBuffer.SetData(data);
    }
}
```

#### 3. Poll()方法检查共享内存 ✅
```csharp
// 检查共享内存中的Side LED数据（amdaemon进程）
if (_ledBuffer != null)
{
    var sharedData = _ledBuffer.GetData();
    bool hasNewData = false;
    
    // 检查是否有新的Side LED数据
    for (int i = 0; i < 6; i++)
    {
        if (sharedData[i] != _lastSideLedData[i])
        {
            hasNewData = true;
            break;
        }
    }
    
    if (hasNewData)
    {
        // 发送到USB
        if (_connection != null)
        {
            _connection.SetSideLeds(sharedData.ToArray());
        }
    }
}
```

## 数据格式

### 共享内存结构
- **大小**：6字节
- **内容**：LS, RS (每个3字节RGB)
- **格式**：`[LS_R, LS_G, LS_B, RS_R, RS_G, RS_B]`

### 数据来源
- **LS数据**：来自mu3io Board 0的LED 0
- **RS数据**：来自mu3io Board 0的LED 1

## 预期效果

### 1. 解决数据丢失
- mu3进程的Side LED数据不再丢失
- 通过共享内存传递给amdaemon进程
- amdaemon进程负责USB通信

### 2. 避免USB冲突
- 只有amdaemon进程创建USB连接
- 其他进程通过共享内存通信
- 避免多进程抢连接

### 3. 实时性保证
- amdaemon进程在Poll()中检查共享内存
- 发现新数据立即发送到USB
- 最小化数据延迟

## 测试步骤

### 1. 验证mu3进程写入
1. 运行游戏，改变Side LED设置
2. 观察日志：
   ```
   E.N.C Controller: Side LED board 0 called from process: mu3
   E.N.C Controller: Writing Side LED data to shared buffer for amdaemon process
   E.N.C Controller: Side LED data written to shared buffer: FF 00 00 | FF 00 00
   ```

### 2. 验证amdaemon进程读取
1. 确认amdaemon进程运行
2. 观察日志：
   ```
   E.N.C Controller: Found new Side LED data in shared buffer: FF 00 00 | FF 00 00
   E.N.C Controller: Side LED data sent from shared buffer to USB
   ```

### 3. 验证ESP32接收
1. 检查ESP32日志
2. 确认收到Side LED数据
3. 验证LED_1[3]和LED_5[3]正确显示

## 潜在问题

### 1. 数据同步延迟
**问题**：共享内存检查可能有延迟
**解决方案**：优化Poll()频率或使用事件通知

### 2. 内存泄漏
**问题**：共享内存可能没有正确释放
**解决方案**：添加Dispose()方法调用

### 3. 数据竞争
**问题**：多进程同时写入可能导致数据不一致
**解决方案**：添加同步机制

## 性能优化

### 1. 减少检查频率
```csharp
// 可以添加时间间隔控制
private static DateTime _lastSharedBufferCheck = DateTime.MinValue;
private const int SHARED_BUFFER_CHECK_INTERVAL_MS = 100; // 100ms检查一次
```

### 2. 事件驱动
```csharp
// 可以考虑使用命名事件通知
// 当有新数据时通知amdaemon进程
```

### 3. 批量处理
```csharp
// 可以批量处理多个LED更新
// 减少USB通信次数
```

## 监控和调试

### 1. 日志记录
- 记录共享内存读写操作
- 记录数据变化检测
- 记录USB发送状态

### 2. 数据验证
- 验证共享内存数据完整性
- 验证数据传递延迟
- 验证USB发送成功率

### 3. 错误处理
- 处理共享内存访问异常
- 处理USB连接异常
- 处理数据格式异常

## 结论

通过共享内存实现进程间通信，可以有效解决Side LED数据丢失的问题。这种方案：

1. **简单可靠**：使用系统提供的共享内存机制
2. **性能良好**：最小化数据传递延迟
3. **易于调试**：有完整的日志记录
4. **扩展性强**：可以轻松添加更多数据传递

现在应该能解决Side LED数据丢失的问题了！ 