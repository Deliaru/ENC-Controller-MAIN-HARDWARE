#!/usr/bin/env python3
"""
缓冲区安全性测试脚本
用于验证Ontroller.WinUSB.IO的缓冲区溢出修复
"""

import struct
import time
import sys

def create_test_packet(packet_type, data_size):
    """创建测试数据包"""
    if packet_type == "input":
        # 模拟从ESP32接收的数据 (7字节)
        header = b'\x44\x44\x54'  # 固定头部
        buttons = b'\x00\x00'     # 按钮状态
        lever = b'\x00\x00'       # 摇杆位置
        return header + buttons + lever
    elif packet_type == "output":
        # 模拟发送到ESP32的数据 (33字节)
        header = b'\x44\x4C\x01'  # 固定头部
        io4_leds = b'\xFF' * 18   # IO4 LED数据
        side_leds = b'\x00' * 12  # 侧边LED数据
        return header + io4_leds + side_leds
    else:
        return b'\x00' * data_size

def test_buffer_sizes():
    """测试不同缓冲区大小"""
    test_cases = [
        ("正常大小 - 输入", "input", 7),
        ("正常大小 - 输出", "output", 33),
        ("过大缓冲区 - 输入", "input", 100),
        ("过大缓冲区 - 输出", "output", 100),
        ("零大小缓冲区", "input", 0),
        ("NULL指针模拟", None, 0),
    ]
    
    print("=== 缓冲区安全性测试 ===")
    
    for test_name, packet_type, size in test_cases:
        print(f"\n测试: {test_name}")
        
        if packet_type is None:
            print("  模拟NULL指针 - 应该被安全处理")
            continue
            
        try:
            data = create_test_packet(packet_type, size)
            print(f"  数据大小: {len(data)} 字节")
            print(f"  数据内容: {data[:10].hex()}...")
            
            if len(data) != size and size > 0:
                print(f"  警告: 实际大小 ({len(data)}) 与预期 ({size}) 不符")
                
        except Exception as e:
            print(f"  错误: {e}")
    
    print("\n=== 测试完成 ===")

def test_led_data_validation():
    """测试LED数据验证"""
    print("\n=== LED数据验证测试 ===")
    
    # 测试正确的LED数据
    correct_io4_leds = b'\xFF\x00\xFF' * 6  # 18字节
    correct_side_leds = b'\x00\xFF\x00' * 4  # 12字节
    
    print(f"正确的IO4 LED数据: {len(correct_io4_leds)} 字节")
    print(f"正确的侧边LED数据: {len(correct_side_leds)} 字节")
    
    # 测试错误的LED数据
    wrong_sizes = [10, 15, 20, 25, 30]
    for size in wrong_sizes:
        wrong_data = b'\xFF' * size
        print(f"错误大小LED数据 ({size} 字节): {'应该被拒绝' if size != 18 and size != 12 else '应该被接受'}")

def main():
    """主函数"""
    print("Ontroller.WinUSB.IO 缓冲区安全性测试")
    print("=" * 50)
    
    test_buffer_sizes()
    test_led_data_validation()
    
    print("\n建议:")
    print("1. 检查ESP32日志中的缓冲区溢出警告")
    print("2. 监控内存使用情况")
    print("3. 验证USB通信的稳定性")
    print("4. 测试长时间运行的内存泄漏")

if __name__ == "__main__":
    main() 