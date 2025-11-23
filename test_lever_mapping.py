#!/usr/bin/env python3
"""
摇杆映射验证脚本
验证ESP32、PC端和GAMEHOOK的摇杆映射是否一致
"""

def test_lever_mapping():
    """测试摇杆映射"""
    print("=== 摇杆映射验证 ===")
    
    # GAMEHOOK的摇杆范围
    print("GAMEHOOK (mu3io.c):")
    print("- 范围: INT16_MIN 到 INT16_MAX (-32768 到 32767)")
    print("- 中心位置: 0")
    print("- 数据类型: int16_t")
    
    print("\nOntroller.WinUSB.IO (Ontroller.cs):")
    print("- 转换公式: raw * 80 - short.MaxValue")
    print("- 输入: 2字节 raw值")
    print("- 输出: int16_t")
    
    print("\nESP32 (GPIO_My.c):")
    print("- 输入: 角度 (0-360度)")
    print("- 转换: angle/360 * 65536 - 32768")
    print("- 输出: 2字节 raw值")
    
    print("\n映射验证:")
    
    # 测试几个关键位置
    test_positions = [
        (0, "最左"),
        (90, "左"),
        (180, "中心"),
        (270, "右"),
        (360, "最右")
    ]
    
    for angle, name in test_positions:
        # ESP32端计算
        lever_value = int((angle / 360.0) * 65536 - 32768)
        raw_lever = (lever_value + 32767) // 80
        
        # PC端计算（反向验证）
        pc_lever = raw_lever * 80 - 32767
        
        print(f"{name} ({angle}°):")
        print(f"  ESP32 lever_value: {lever_value}")
        print(f"  ESP32 raw_lever: {raw_lever} (0x{raw_lever:04X})")
        print(f"  PC端 lever: {pc_lever}")
        print()
    
    print("数据包格式:")
    center_raw = 409
    print(f"中心位置数据包: 44 44 54 00 07 {center_raw & 0xFF:02X} {(center_raw >> 8) & 0xFF:02X}")
    
    print()

def test_expected_values():
    """测试预期值"""
    print("=== 预期值测试 ===")
    
    # 中心位置
    center_angle = 180
    center_lever_value = int((center_angle / 360.0) * 65536 - 32768)  # = 0
    center_raw = (center_lever_value + 32767) // 80  # = 409
    
    print(f"中心位置 (180°):")
    print(f"  lever_value: {center_lever_value}")
    print(f"  raw_lever: {center_raw} (0x{center_raw:04X})")
    print(f"  数据包: 44 44 54 00 07 {center_raw & 0xFF:02X} {(center_raw >> 8) & 0xFF:02X}")
    
    # 最左位置
    left_angle = 0
    left_lever_value = int((left_angle / 360.0) * 65536 - 32768)  # = -32768
    left_raw = (left_lever_value + 32767) // 80  # = 0
    
    print(f"\n最左位置 (0°):")
    print(f"  lever_value: {left_lever_value}")
    print(f"  raw_lever: {left_raw} (0x{left_raw:04X})")
    print(f"  数据包: 44 44 54 00 07 {left_raw & 0xFF:02X} {(left_raw >> 8) & 0xFF:02X}")
    
    # 最右位置
    right_angle = 360
    right_lever_value = int((right_angle / 360.0) * 65536 - 32768)  # = 32767
    right_raw = (right_lever_value + 32767) // 80  # = 819
    
    print(f"\n最右位置 (360°):")
    print(f"  lever_value: {right_lever_value}")
    print(f"  raw_lever: {right_raw} (0x{right_raw:04X})")
    print(f"  数据包: 44 44 54 00 07 {right_raw & 0xFF:02X} {(right_raw >> 8) & 0xFF:02X}")
    
    print()

def test_pc_conversion():
    """测试PC端转换"""
    print("=== PC端转换测试 ===")
    
    # 测试几个raw值
    test_raws = [0, 409, 819]
    
    for raw in test_raws:
        pc_lever = raw * 80 - 32767
        print(f"raw={raw} (0x{raw:04X}) -> PC lever={pc_lever}")
    
    print()

def main():
    """主函数"""
    print("摇杆映射验证")
    print("=" * 40)
    
    test_lever_mapping()
    test_expected_values()
    test_pc_conversion()
    
    print("验证要点:")
    print("1. ESP32角度 -> lever_value -> raw_lever")
    print("2. PC端 raw_lever -> lever")
    print("3. 中心位置: 180° -> 0 -> 409")
    print("4. 最左位置: 0° -> -32768 -> 0")
    print("5. 最右位置: 360° -> 32767 -> 819")
    print("6. 数据包格式: 44 44 54 00 07 [lever_low] [lever_high]")

if __name__ == "__main__":
    main() 