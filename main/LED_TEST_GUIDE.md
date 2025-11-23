# LED硬件功能测试指南

## 数据格式说明

### 数据包结构 (33字节)
```
[0-2]   头部: 44 4C 01 (固定)
[3-20]  IO4 LED数据: 18字节 (6个LED × 3字节RGB)
[21-32] Side LED数据: 12字节 (4个LED × 3字节RGB)
```

### LED映射
- **IO4 LED (led_1, GPIO_48)**: 4个LED
  - LED_1[0]: L3 (左侧第3个)
  - LED_1[1]: L2 (左侧第2个)
  - LED_1[2]: L1 (左侧第1个)
  - LED_1[3]: LS (左侧Side)

- **Side LED (led_5, GPIO_41)**: 4个LED
  - LED_5[0]: R1 (右侧第1个)
  - LED_5[1]: R2 (右侧第2个)
  - LED_5[2]: R3 (右侧第3个)
  - LED_5[3]: RS (右侧Side)

## 测试数据

### 1. 全红色测试
```
44 4C 01 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00
```
- IO4 LED: 6个红色LED
- Side LED: 4个红色LED

### 2. 全绿色测试
```
44 4C 01 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00
```
- IO4 LED: 6个绿色LED
- Side LED: 4个绿色LED

### 3. 全蓝色测试
```
44 4C 01 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF 00 00 FF
```
- IO4 LED: 6个蓝色LED
- Side LED: 4个蓝色LED

### 4. 彩虹渐变测试
```
44 4C 01 FF 00 00 FF 7F 00 FF FF 00 7F FF 00 00 FF 00 00 7F FF 00 00 FF 00 00 7F FF 00 00 FF 00 00 7F FF
```
- IO4 LED: 红、橙、黄、绿、青、蓝
- Side LED: 红、橙、黄、绿

### 5. 只设置IO4 LED (侧边LED关闭)
```
44 4C 01 FF 00 00 00 FF 00 00 00 FF 00 00 00 FF 00 00 00 FF 00 00 00 FF 00 00 00 00 00 00 00 00 00 00 00 00
```
- IO4 LED: 6个不同颜色
- Side LED: 全部关闭

### 6. 只设置Side LED (IO4 LED关闭)
```
44 4C 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF 00 00 00 FF 00 00 00 FF 00 00
```
- IO4 LED: 全部关闭
- Side LED: 4个红色

## 测试步骤

### 1. 编译和烧录
```bash
idf.py clean
idf.py build
idf.py flash monitor
```

### 2. 使用WinUSB调试工具测试
1. 打开WinUSB调试工具
2. 选择ESP32设备
3. 发送上述HEX数据
4. 观察ESP32日志和LED效果

### 3. 使用Ontroller.WinUSB.IO测试
1. 运行Ontroller.WinUSB.IO程序
2. 连接ESP32设备
3. 调用SetIO4Leds()或SetSideLeds()方法
4. 观察LED效果

## 预期日志输出

### 成功接收数据
```
I (xxxx) WINUSB: === TinyUSB Vendor RX Callback ===
I (xxxx) WINUSB: Interface: 0, Size: 33 bytes
I (xxxx) WINUSB: Received 33 bytes
I (xxxx) WINUSB: First few bytes: 44 4C 01 FF
I (xxxx) WINUSB: Data stored successfully: 33 bytes
I (xxxx) WINUSB: LED packet received, will be processed in main loop
```

### LED设置成功
```
I (xxxx) WINUSB: === Fast LED Processing ===
I (xxxx) WINUSB: LED packet validated, header: 44 4C 01
I (xxxx) WINUSB: LED_1[0]: RGB(255,0,0)
I (xxxx) WINUSB: LED_1[1]: RGB(0,255,0)
I (xxxx) WINUSB: LED_1[2]: RGB(0,0,255)
I (xxxx) WINUSB: LED_1 refresh successful
I (xxxx) WINUSB: LED_5[0]: RGB(255,0,0)
I (xxxx) WINUSB: LED_5 refresh successful
I (xxxx) WINUSB: === LED Processing Complete ===
```

## 故障排除

### 如果LED不亮
1. 检查GPIO引脚连接
2. 确认LED_1 (GPIO_48) 和 LED_5 (GPIO_41) 正确连接
3. 检查电源供应
4. 查看ESP32日志中的错误信息

### 如果颜色不正确
1. 检查RGB顺序是否正确
2. 确认数据包格式匹配
3. 验证LED类型和驱动方式

### 如果部分LED不工作
1. 检查LED硬件连接
2. 确认LED数量配置正确
3. 验证led_strip_set_pixel参数

## 数据格式验证

### Ontroller.WinUSB.IO发送的数据格式
- 头部: `44 4C 01` (固定)
- IO4 LED: 18字节，偏移3-20
- Side LED: 12字节，偏移21-32
- 总长度: 33字节

### ESP32接收的数据格式
- 与发送格式完全匹配
- 自动解析RGB值
- 直接设置到对应LED 