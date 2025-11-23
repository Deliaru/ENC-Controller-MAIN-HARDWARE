# 进程合并解决方案

## 问题描述
多进程抢USB连接导致IO4 LED无法正常工作，需要将Side LED (board 0) 也归入amdaemon进程。

## 根本原因

### 1. 多进程USB连接冲突
```
进程A (amdaemon)     → IO4 LED (board 1) → USB连接A
进程B (其他进程)     → Side LED (board 0) → USB连接B (冲突!)
```

### 2. USB设备独占性
- USB设备在同一时间只能被一个进程访问
- 多个进程同时尝试连接会导致冲突
- 后连接的进程会抢占先连接的进程

## 解决方案

### 方案1：进程限制 ✅ (已实施)

#### 修改前
```csharp
// 为所有进程创建连接，不限制只有amdaemon
if (_connection == null)
{
    _connection = new Connection();
    // ...
}
```

#### 修改后
```csharp
// 只有amdaemon进程才创建USB连接，避免多进程抢连接
if (!processName.ToLower().Contains("amdaemon"))
{
    Logging.WriteLine($"E.N.C Controller: Skipping USB connection for non-amdaemon process: {processName}");
    return 0;
}

if (_connection == null)
{
    _connection = new Connection();
    // ...
}
```

### 方案2：Side LED处理限制 ✅ (已实施)

#### 修改前
```csharp
// game - Side LED设置（事件驱动）
if (board == 0)
{
    // 所有进程都处理Side LED
    // ...
}
```

#### 修改后
```csharp
// game - Side LED设置（事件驱动）
if (board == 0)
{
    var processName = Process.GetCurrentProcess().ProcessName;
    
    // 只有amdaemon进程才处理Side LED，避免多进程抢USB连接
    if (!processName.ToLower().Contains("amdaemon"))
    {
        Logging.WriteLine($"E.N.C Controller: Skipping Side LED processing for non-amdaemon process: {processName}");
        return;
    }
    
    // 只有amdaemon进程才处理Side LED
    // ...
}
```

## 架构变化

### 修改前
```
游戏进程A → mu3hook → mu3io → Interface.cs (进程A) → USB连接A
游戏进程B → mu3hook → mu3io → Interface.cs (进程B) → USB连接B (冲突!)
```

### 修改后
```
游戏进程A → mu3hook → mu3io → Interface.cs (进程A) → 跳过USB连接
游戏进程B → mu3hook → mu3io → Interface.cs (进程B) → 跳过USB连接
amdaemon进程 → mu3hook → mu3io → Interface.cs (amdaemon) → USB连接 (唯一)
```

## 数据流分析

### mu3io架构
根据`GAMEHOOK/mu3io/ledoutput.c`：
```c
void mu3_led_output_update(int board, const uint8_t* rgb)
{
    if (board == 0) {
        // cab - 发送到cab_led_output_pipe或cab_led_output_serial
    } else {
        // slider - 发送到controller_led_output_pipe或controller_led_output_serial
    }
}
```

### mu3hook架构
根据`GAMEHOOK/mu3hook/dllmain.c`：
```c
// 两个LED端口
unsigned int led_port_no[2] = {3, 0};
hr = led15093_hook_init(&mu3_hook_cfg.led15093, 
    mu3_dll.led_init, mu3_dll.led_set_leds, led_port_no);
```

## 预期效果

### 1. 解决USB连接冲突
- 只有amdaemon进程创建USB连接
- 其他进程跳过USB连接
- 避免多进程抢连接

### 2. 统一数据处理
- 所有LED数据都在amdaemon进程中处理
- 统一的数据发送逻辑
- 避免数据不一致

### 3. 性能优化
- 减少USB连接数量
- 降低系统资源占用
- 提高稳定性

## 测试步骤

### 1. 验证进程限制
1. 重新编译Ontroller.WinUSB.IO
2. 运行游戏
3. 观察日志：
   ```
   E.N.C Controller: Init from [进程名]
   E.N.C Controller: Skipping USB connection for non-amdaemon process: [进程名]
   ```

### 2. 验证Side LED处理
1. 改变Side LED设置
2. 观察日志：
   ```
   E.N.C Controller: Side LED board 0 called from process: [进程名]
   E.N.C Controller: Skipping Side LED processing for non-amdaemon process: [进程名]
   ```

### 3. 验证amdaemon处理
1. 确认amdaemon进程处理Side LED
2. 观察日志：
   ```
   E.N.C Controller: Side LED board 0 called from process: amdaemon
   E.N.C Controller: Connection status: VALID
   E.N.C Controller: Calling SetSideLeds with data: ...
   ```

## 潜在问题

### 1. Side LED数据丢失
**问题**：非amdaemon进程的Side LED数据可能丢失
**解决方案**：通过共享内存或命名管道传递数据

### 2. 数据同步延迟
**问题**：Side LED数据可能有延迟
**解决方案**：优化数据传递机制

### 3. 进程依赖
**问题**：依赖amdaemon进程运行
**解决方案**：添加进程监控和自动重连

## 替代方案

### 方案A：共享内存通信
```csharp
// 使用共享内存在不同进程间传递LED数据
// 只有amdaemon进程负责USB通信
```

### 方案B：命名管道通信
```csharp
// 使用命名管道进行进程间通信
// 统一数据发送到amdaemon进程
```

### 方案C：全局命名对象
```csharp
// 使用全局命名对象确保只有一个USB连接
// 所有进程共享同一个连接实例
```

## 结论

通过限制只有amdaemon进程处理USB连接和Side LED数据，可以有效解决多进程抢连接的问题。这种方案简单有效，但需要注意数据传递的及时性和可靠性。

如果发现Side LED数据丢失或延迟，可以考虑实现更复杂的进程间通信机制。 