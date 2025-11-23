#!/usr/bin/env python3
"""
USB通信测试脚本
用于验证ESP32和PC之间的数据交换
"""

import time
import struct

def test_expected_data():
    """测试预期的数据格式"""
    print("=== 预期数据格式测试 ===")
    
    # 模拟ESP32发送的数据包
    expected_packet = bytes([
        0x44, 0x44, 0x54,  # 头部
        0x20, 0x00,        # 按钮状态 (左A按钮)
        0xA2, 0x00         # 摇杆位置
    ])
    
    print(f"预期数据包: {expected_packet.hex(' ').upper()}")
    print(f"数据包长度: {len(expected_packet)} 字节")
    
    # 解析数据包
    header = expected_packet[0:3]
    buttons = expected_packet[3:5]
    lever = expected_packet[5:7]
    
    print(f"头部: {header.hex(' ').upper()}")
    print(f"按钮: {buttons.hex(' ').upper()}")
    print(f"摇杆: {lever.hex(' ').upper()}")
    
    # 解析按钮状态
    left_buttons = buttons[0]
    right_buttons = buttons[1]
    
    print(f"左按钮: 0x{left_buttons:02X}")
    print(f"右按钮: 0x{right_buttons:02X}")
    
    # 解析摇杆位置
    lever_raw = struct.unpack('<H', lever)[0]  # 小端序
    lever_value = (lever_raw * 80 - 32767)
    print(f"摇杆原始值: 0x{lever_raw:04X} ({lever_raw})")
    print(f"摇杆计算值: {lever_value}")
    
    print()

def test_button_mapping():
    """测试按钮映射"""
    print("=== 按钮映射测试 ===")
    
    button_tests = [
        (0x20, "左A"),
        (0x10, "左B"),
        (0x08, "左C"),
        (0x80, "左Side"),
        (0x04, "右A"),
        (0x02, "右B"),
        (0x01, "右C"),
        (0x40, "右Side"),
    ]
    
    for mask, name in button_tests:
        packet = bytes([0x44, 0x44, 0x54, mask, 0x00, 0x00, 0x00])
        print(f"{name}: 0x{mask:02X} -> {packet.hex(' ').upper()}")
    
    print()

def test_communication_flow():
    """测试通信流程"""
    print("=== 通信流程测试 ===")
    
    print("1. ESP32启动并初始化USB")
    print("2. ESP32进入主循环 (100Hz)")
    print("3. ESP32生成测试数据")
    print("4. ESP32通过tud_vendor_write发送数据")
    print("5. PC端通过WinUSB读取数据")
    print("6. PC端处理数据并更新游戏状态")
    print("7. PC端发送LED控制数据")
    print("8. ESP32接收并处理LED数据")
    
    print("\n预期日志:")
    print("ESP32: Sending data: 44 44 54 20 00 A2 00")
    print("ESP32: WinUSB data sent successfully")
    print("PC: Ontroller: Read data: 44 44 54 20 00 A2 00")
    print("PC: Ontroller: State change - Left: None->A")
    
    print()

def main():
    """主函数"""
    print("Ontroller USB通信测试")
    print("=" * 50)
    
    test_expected_data()
    test_button_mapping()
    test_communication_flow()
    
    print("修复建议:")
    print("1. 确保ESP32主循环已启用")
    print("2. 检查USB挂载状态")
    print("3. 验证数据发送函数")
    print("4. 监控ESP32串口日志")
    print("5. 检查PC端读取超时设置")

if __name__ == "__main__":
    main() 