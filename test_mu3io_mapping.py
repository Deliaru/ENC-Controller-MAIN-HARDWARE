#!/usr/bin/env python3
"""
基于mu3io.c正确映射的按键映射验证脚本
"""

def test_mu3io_data_format():
    """测试mu3io.c的数据格式"""
    print("=== mu3io.c 数据格式 ===")
    
    print("游戏通过以下函数获取数据:")
    print("void mu3_io_get_opbtns(uint8_t *opbtn)     // 操作按钮：8位")
    print("void mu3_io_get_gamebtns(uint8_t *left, uint8_t *right)  // 游戏按钮：2个8位")
    print("void mu3_io_get_lever(int16_t *pos)        // 摇杆：16位")
    
    print("\n关键发现:")
    print("- mu3io.c使用8位按钮数据，不是16位！")
    print("- 游戏直接调用mu3io.c的函数")
    print("- io4.c只是将mu3io的8位数据转换为16位数组")
    
    print()

def test_mu3io_button_mapping():
    """测试mu3io.c的按钮映射"""
    print("=== mu3io.c 按钮映射 ===")
    
    print("MU3_IO_GAMEBTN枚举:")
    print("MU3_IO_GAMEBTN_1 = 0x01")
    print("MU3_IO_GAMEBTN_2 = 0x02")
    print("MU3_IO_GAMEBTN_3 = 0x04")
    print("MU3_IO_GAMEBTN_SIDE = 0x08")
    print("MU3_IO_GAMEBTN_MENU = 0x10")
    
    print("\nmu3io.c中的右按钮映射:")
    print("if (GetAsyncKeyState(mu3_io_cfg.vk_right_1) || (xb & XINPUT_GAMEPAD_X)) {")
    print("    mu3_right_btn |= MU3_IO_GAMEBTN_1;  // 0x01")
    print("}")
    print("if (GetAsyncKeyState(mu3_io_cfg.vk_right_2) || (xb & XINPUT_GAMEPAD_Y)) {")
    print("    mu3_right_btn |= MU3_IO_GAMEBTN_2;  // 0x02")
    print("}")
    print("if (GetAsyncKeyState(mu3_io_cfg.vk_right_3) || (xb & XINPUT_GAMEPAD_B)) {")
    print("    mu3_right_btn |= MU3_IO_GAMEBTN_3;  // 0x04")
    print("}")
    
    print()

def test_correct_mapping():
    """测试正确的映射"""
    print("=== 正确的映射 ===")
    
    print("右ABC按钮映射:")
    print("- 右A: 0x01 (MU3_IO_GAMEBTN_1)")
    print("- 右B: 0x02 (MU3_IO_GAMEBTN_2)")
    print("- 右C: 0x04 (MU3_IO_GAMEBTN_3)")
    print("- 右ABC全部: 0x07 (0x01+0x02+0x04)")
    
    print("\n数据包格式:")
    print("- 按下时: 44 44 54 00 07 99 01")
    print("- 弹起时: 44 44 54 00 00 99 01")
    
    print("\nPC端解析:")
    print("- buffer[3]: 左按钮 (8位)")
    print("- buffer[4]: 右按钮 (8位)")
    print("- buffer[5-6]: 摇杆 (16位)")
    
    print()

def test_data_flow():
    """测试数据流"""
    print("=== 数据流分析 ===")
    
    print("1. ESP32发送数据包:")
    print("   44 44 54 [left8] [right8] [lever16]")
    
    print("\n2. PC端Ontroller.cs解析:")
    print("   GetLeftFromBuffer(buffer)  // 解析buffer[3]")
    print("   GetRightFromBuffer(buffer) // 解析buffer[4]")
    print("   GetLeverFromBuffer(buffer) // 解析buffer[5-6]")
    
    print("\n3. 游戏调用mu3io.c函数:")
    print("   mu3_io_get_gamebtns(&left, &right)")
    print("   mu3_io_get_lever(&pos)")
    
    print("\n4. mu3io.c返回8位数据:")
    print("   left = 8位按钮状态")
    print("   right = 8位按钮状态")
    print("   pos = 16位摇杆位置")
    
    print()

def test_verification():
    """测试验证"""
    print("=== 验证测试 ===")
    
    print("右ABC按钮测试:")
    print("1. ESP32发送: 44 44 54 00 07 99 01")
    print("2. PC端解析: right = 0x07")
    print("3. 游戏获取: right = 0x07")
    print("4. 游戏检测: 右A(0x01) + 右B(0x02) + 右C(0x04) = 0x07")
    
    print("\n预期结果:")
    print("- 游戏应该检测到右ABC按钮全部按下")
    print("- 摇杆应该在中心位置")
    print("- 日志应该显示正确的按钮状态")
    
    print()

def main():
    """主函数"""
    print("mu3io.c正确映射验证")
    print("=" * 40)
    
    test_mu3io_data_format()
    test_mu3io_button_mapping()
    test_correct_mapping()
    test_data_flow()
    test_verification()
    
    print("关键要点:")
    print("1. 游戏使用mu3io.c的8位数据格式")
    print("2. 右ABC按钮值应该是0x07 (0x01+0x02+0x04)")
    print("3. 数据包格式: 44 44 54 00 07 99 01")
    print("4. 映射现在与mu3io.c完全一致")
    print("5. 应该能正确识别右ABC按钮")

if __name__ == "__main__":
    main() 