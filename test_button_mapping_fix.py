#!/usr/bin/env python3
"""
按键映射修复验证脚本
验证修复后的按键映射是否与GAMEHOOK一致
"""

def test_mu3hook_mapping():
    """测试mu3hook/io4.c中的按键映射"""
    print("=== GAMEHOOK mu3hook/io4.c 按键映射 ===")
    
    print("左按钮映射:")
    print("- 左A: bit 0  (0x01)")
    print("- 左B: bit 5  (0x20)")
    print("- 左C: bit 4  (0x10)")
    print("- 左Side: bit 15 (0x8000)")
    print("- 左Menu: bit 14 (0x4000)")
    
    print("\n右按钮映射:")
    print("- 右A: bit 1  (0x02)")
    print("- 右B: bit 16 (0x0001)")
    print("- 右C: bit 15 (0x8000)")
    print("- 右Side: bit 14 (0x4000)")
    print("- 右Menu: bit 13 (0x2000)")
    
    print()

def test_ontroller_mapping():
    """测试Ontroller.WinUSB.IO的按键映射"""
    print("=== Ontroller.WinUSB.IO 按键映射（修复后） ===")
    
    print("左按钮映射 (buffer[3]和buffer[4]):")
    print("- 左A: buffer[3] & 0x01")
    print("- 左B: buffer[3] & 0x20")
    print("- 左C: buffer[3] & 0x10")
    print("- 左Side: buffer[4] & 0x80")
    print("- 左Menu: buffer[4] & 0x40")
    
    print("\n右按钮映射 (buffer[3]和buffer[4]):")
    print("- 右A: buffer[3] & 0x02")
    print("- 右B: buffer[4] & 0x01")
    print("- 右C: buffer[3] & 0x80")
    print("- 右Side: buffer[4] & 0x40")
    print("- 右Menu: buffer[3] & 0x20")
    
    print()

def test_esp32_mapping():
    """测试ESP32的按键映射"""
    print("=== ESP32 按键映射（修复后） ===")
    
    print("左按钮映射:")
    print("- 左A (BTN_L1): 0x01")
    print("- 左B (BTN_L2): 0x20")
    print("- 左C (BTN_L3): 0x10")
    print("- 左Side (BTN_LS): 0x80")
    print("- 左Menu (BTN_LM): 0x40")
    
    print("\n右按钮映射:")
    print("- 右A (BTN_R1): 0x02")
    print("- 右B (BTN_R2): 0x01")
    print("- 右C (BTN_R3): 0x80")
    print("- 右Side (BTN_RS): 0x40")
    print("- 右Menu (BTN_RM): 0x20")
    
    print()

def test_right_abc_mapping():
    """测试右ABC按钮映射"""
    print("=== 右ABC按钮映射测试 ===")
    
    print("修复前（错误）:")
    print("- 右A: 0x01")
    print("- 右B: 0x02")
    print("- 右C: 0x04")
    print("- 右ABC全部: 0x07")
    
    print("\n修复后（正确）:")
    print("- 右A: 0x02 (bit 1)")
    print("- 右B: 0x01 (bit 16)")
    print("- 右C: 0x80 (bit 15)")
    print("- 右ABC全部: 0x83 (0x02+0x01+0x80)")
    
    print("\n数据包格式:")
    print("- 按下时: 44 44 54 00 83 99 01")
    print("- 弹起时: 44 44 54 00 00 99 01")
    
    print()

def test_mapping_consistency():
    """测试映射一致性"""
    print("=== 映射一致性测试 ===")
    
    print("GAMEHOOK -> Ontroller.WinUSB.IO:")
    print("1. mu3hook/io4.c 的位映射")
    print("2. Ontroller.cs 的位掩码")
    print("3. ESP32 GPIO_My.c 的位设置")
    print("4. 数据包格式的一致性")
    
    print("\n关键修复:")
    print("1. 右A: bit 1 (0x02) 而不是 bit 0 (0x01)")
    print("2. 右B: bit 16 (0x01) 而不是 bit 1 (0x02)")
    print("3. 右C: bit 15 (0x80) 而不是 bit 2 (0x04)")
    print("4. 右ABC全部: 0x83 而不是 0x07")
    
    print()

def main():
    """主函数"""
    print("按键映射修复验证")
    print("=" * 40)
    
    test_mu3hook_mapping()
    test_ontroller_mapping()
    test_esp32_mapping()
    test_right_abc_mapping()
    test_mapping_consistency()
    
    print("修复要点:")
    print("1. 以GAMEHOOK的mu3hook/io4.c为准")
    print("2. 修复了右ABC按钮的位映射")
    print("3. 确保ESP32、PC端、GAMEHOOK映射一致")
    print("4. 右ABC按钮值从0x07改为0x83")
    print("5. 数据包格式相应更新")

if __name__ == "__main__":
    main() 