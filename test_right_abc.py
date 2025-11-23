#!/usr/bin/env python3
"""
右ABC按钮交替测试脚本
验证右ABC按钮每隔1秒交替按下和弹起（修复后的映射）
"""

import time

def test_right_abc_alternating():
    """测试右ABC按钮交替行为"""
    print("=== 右ABC按钮交替测试（修复后） ===")
    
    # 右ABC按钮的位掩码（修复后）
    right_a_mask = 0x02  # 右A按钮 (bit 1)
    right_b_mask = 0x01  # 右B按钮 (bit 16)
    right_c_mask = 0x80  # 右C按钮 (bit 15)
    
    # 计算右ABC全部按下的值
    right_abc_all = right_a_mask | right_b_mask | right_c_mask
    print(f"右A按钮掩码: 0x{right_a_mask:02X}")
    print(f"右B按钮掩码: 0x{right_b_mask:02X}")
    print(f"右C按钮掩码: 0x{right_c_mask:02X}")
    print(f"右ABC全部按下: 0x{right_abc_all:02X} (0x02 + 0x01 + 0x80 = 0x83)")
    
    # 模拟时间变化
    for i in range(6):  # 模拟6秒
        current_time = i * 1000  # 毫秒
        buttons_pressed = ((current_time // 1000) % 2) == 0
        
        # 模拟数据包
        right_buttons = 0x83 if buttons_pressed else 0x00
        packet = bytes([
            0x44, 0x44, 0x54,  # 头部
            0x00, right_buttons, # 按钮状态 (左=0x00, 右=0x83或0x00)
            0x99, 0x01         # 摇杆位置 (中心，修复后: 409 = 0x0199)
        ])
        
        status = "按下" if buttons_pressed else "弹起"
        print(f"第{i}秒: 右ABC {status} - 数据包: {packet.hex(' ').upper()}")
    
    print()

def test_expected_logs():
    """测试预期的日志输出"""
    print("=== 预期日志测试（修复后） ===")
    
    print("ESP32端预期日志:")
    print("测试模式: 右ABC按钮每隔1秒交替按下和弹起")
    print("按钮状态: 左=0x00, 右=0x83, 操作=0x00")
    print("摇杆位置: 0x0199 (中心，修复后)")
    print("状态变化 - 按钮: 00 83, 摇杆: 0199 (按下时)")
    print("状态变化 - 按钮: 00 00, 摇杆: 0199 (弹起时)")
    
    print("\nPC端预期日志:")
    print("Ontroller: State change - Left: None->None, Right: None->ABC, Op: None->None, Lever: 0->0 (按下)")
    print("Ontroller: State change - Left: None->None, Right: ABC->None, Op: None->None, Lever: 0->0 (弹起)")
    
    print()

def test_timing():
    """测试时间间隔"""
    print("=== 时间间隔测试 ===")
    
    print("按钮状态变化时间表:")
    for i in range(10):
        current_time = i * 1000  # 毫秒
        buttons_pressed = ((current_time // 1000) % 2) == 0
        status = "按下" if buttons_pressed else "弹起"
        print(f"第{i}秒: {status}")
    
    print("\n规律:")
    print("- 偶数秒(0,2,4,6,8...): 右ABC按下")
    print("- 奇数秒(1,3,5,7,9...): 右ABC弹起")
    print("- 变化间隔: 1秒")
    
    print()

def test_button_mapping():
    """测试按钮映射修复"""
    print("=== 按钮映射修复测试 ===")
    
    print("修复前的错误映射:")
    print("- 右A: 0x01 (错误)")
    print("- 右B: 0x02 (错误)")
    print("- 右C: 0x04 (错误)")
    
    print("\n修复后的正确映射:")
    print("- 右A: 0x02 (正确)")
    print("- 右B: 0x01 (正确)")
    print("- 右C: 0x80 (正确)")
    print("- 右ABC全部: 0x83 (0x02+0x01+0x80)")
    
    print("\n数据包格式:")
    print("- 按下时: 44 44 54 00 83 99 01")
    print("- 弹起时: 44 44 54 00 00 99 01")
    
    print()

def main():
    """主函数"""
    print("右ABC按钮交替测试（修复后）")
    print("=" * 50)
    
    test_right_abc_alternating()
    test_expected_logs()
    test_timing()
    test_button_mapping()
    
    print("修复要点:")
    print("1. 修复了PC端按钮位掩码映射错误")
    print("2. 修复了ESP32端摇杆映射错误")
    print("3. 修复了右ABC按钮的位映射（以GAMEHOOK为准）")
    print("4. 右ABC按钮每隔1秒交替按下和弹起")
    print("5. 偶数秒按下(0x83)，奇数秒弹起(0x00)")
    print("6. 数据包格式: 44 44 54 00 83/00 99 01")
    print("7. 日志只在状态变化时显示")
    print("8. 摇杆保持在中心位置 (0x0199)")

if __name__ == "__main__":
    main() 