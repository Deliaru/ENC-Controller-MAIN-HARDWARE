# Mu3io侧键灯光数据格式分析

## 数据格式定义

### 1. 基本数据结构
根据`GAMEHOOK/mu3io/leddata.h`：

```c
static uint8_t ongeki_led_board_data_lens[LED_BOARDS_TOTAL] = {9*3, 6*3};
```

- **Board 0 (Cab)**: 27字节 (9个LED × 3字节RGB)
- **Board 1 (IO4)**: 18字节 (6个LED × 3字节RGB)

### 2. Board 0 (Cab) - 27字节数据格式

```
[0-2]   LED 1 RGB - 可能是机台顶部LED
[3-5]   LED 2 RGB - 可能是机台左侧LED
[6-8]   LED 3 RGB - 可能是左侧Side LED (LS)
[9-11]  LED 4 RGB - 可能是机台右侧LED
[12-14] LED 5 RGB - 可能是机台底部LED
[15-17] LED 6 RGB - 可能是机台内部LED
[18-20] LED 7 RGB - 可能是机台装饰LED
[21-23] LED 8 RGB - 可能是机台边框LED
[24-26] LED 9 RGB - 可能是右侧Side LED (RS)
```

## 当前实现分析

### 1. 我们的假设
```csharp
// 在Interface.cs中
// LS数据 - 从索引6开始 (第3个LED)
leds[6..9].CopyTo(new Span<byte>(data, 0, 3));

// RS数据 - 从索引24开始 (第9个LED)
leds[24..27].CopyTo(new Span<byte>(data, 3, 3));
```

### 2. 问题分析

#### 问题1：数据位置假设
- 我们假设LS在索引6，RS在索引24
- 但这个假设可能不正确
- 需要确认Ongeki机台的实际LED布局

#### 问题2：数据完整性
- 我们只提取了2个LED (LS, RS)
- 但Board 0有9个LED，可能包含更多重要信息
- 其他7个LED可能也影响游戏体验

## 需要确认的信息

### 1. Ongeki机台LED布局
需要确认27字节数据中每个LED的实际位置：
- 哪些是Side LED？
- 哪些是机台装饰LED？
- 哪些是按键周围的LED？

### 2. 游戏中的使用
- 游戏是否只使用LS和RS？
- 其他LED是否也重要？
- 是否有其他LED影响游戏效果？

### 3. 数据验证
需要实际测试来确认：
- 发送全红色时，哪些LED亮起？
- 发送全蓝色时，哪些LED亮起？
- 不同颜色组合的效果如何？

## 建议的解决方案

### 方案1：完整数据提取
```csharp
// 提取所有9个LED的数据
byte[] data = new byte[27]; // 完整27字节
leds.CopyTo(data);
```

### 方案2：验证当前假设
```csharp
// 添加调试信息，显示所有LED数据
for (int i = 0; i < 9; i++)
{
    Logging.WriteLine($"LED {i}: RGB({leds[i*3]:X2}, {leds[i*3+1]:X2}, {leds[i*3+2]:X2})");
}
```

### 方案3：灵活映射
```csharp
// 允许配置LED映射
int lsIndex = 2; // 可配置
int rsIndex = 8; // 可配置
```

## 测试建议

### 1. 数据记录测试
- 记录游戏发送的所有27字节数据
- 分析不同游戏状态下的数据模式
- 确认LS和RS的确切位置

### 2. 视觉效果测试
- 测试不同的LED组合
- 确认哪些LED影响游戏体验
- 验证当前2个LED是否足够

### 3. 性能测试
- 测试完整27字节vs 6字节的性能差异
- 确认数据量对USB传输的影响

## 结论

**当前只取2个LED可能不够**，原因：

1. **数据完整性**：27字节包含9个LED，我们只用了2个
2. **位置假设**：LS和RS的位置假设需要验证
3. **游戏体验**：其他LED可能也影响游戏效果
4. **数据浪费**：忽略了87%的数据

**建议**：
1. 先验证LS和RS的确切位置
2. 测试完整27字节数据的效果
3. 根据实际效果决定是否需要更多LED
4. 添加配置选项允许用户选择LED数量 