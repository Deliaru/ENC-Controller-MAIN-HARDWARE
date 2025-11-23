#!/usr/bin/env python3
"""
GPIO状态测试脚本
用于验证ESP32设备的GPIO配置和按钮映射
"""

import usb.core
import usb.util
import time

def find_ontroller_device():
    """查找Ontroller设备"""
    # 查找VID=0x0E8F, PID=0x1216的设备
    device = usb.core.find(idVendor=0x0E8F, idProduct=0x1216)
    return device

def test_gpio_status():
    """测试GPIO状态"""
    device = find_ontroller_device()
    if device is None:
        print("❌ 未找到Ontroller设备")
        return False
    
    print("✅ 找到Ontroller设备")
    print(f"设备信息: VID=0x{device.idVendor:04X}, PID=0x{device.idProduct:04X}")
    
    try:
        # 设置配置
        device.set_configuration()
        
        # 读取输入数据 (EP 0x84)
        ep_in = device[0][(0, 0)][0x84]
        
        print("\n🔍 开始读取GPIO状态...")
        print("请确保所有GPIO引脚都没有连接任何东西")
        print("预期结果: 所有按钮都应该为0")
        
        for i in range(10):
            try:
                data = ep_in.read(7, timeout=1000)
                print(f"数据 {i+1}: {[f'{b:02X}' for b in data]}")
                
                # 解析按钮状态
                header = data[0:3]
                left_buttons = data[3]
                right_buttons = data[4]
                lever = (data[6] << 8) | data[5]
                
                print(f"  头部: {[f'{b:02X}' for b in header]}")
                print(f"  左按钮: 0x{left_buttons:02X} (二进制: {left_buttons:08b})")
                print(f"  右按钮: 0x{right_buttons:02X} (二进制: {right_buttons:08b})")
                print(f"  摇杆值: 0x{lever:04X} ({lever})")
                
                # 检查是否有意外的按钮按下
                if left_buttons != 0 or right_buttons != 0:
                    print("⚠️  警告: 检测到按钮被按下，但GPIO应该什么都没接")
                    print("   可能的原因:")
                    print("   1. GPIO上拉电阻未启用")
                    print("   2. GPIO引脚浮空导致误读")
                    print("   3. 硬件连接问题")
                else:
                    print("✅ GPIO状态正常，所有按钮都为0")
                
                print()
                time.sleep(1)
                
            except usb.core.USBError as e:
                print(f"❌ USB读取错误: {e}")
                break
                
    except Exception as e:
        print(f"❌ 设备访问错误: {e}")
        return False
    
    return True

def analyze_button_mapping():
    """分析按钮映射"""
    print("\n📋 按钮映射分析:")
    print("根据Ontroller.cs的映射:")
    print()
    print("左按钮 (字节3):")
    print("  A按钮:    0x20 (32)  - 位5")
    print("  B按钮:    0x10 (16)  - 位4") 
    print("  C按钮:    0x08 (8)   - 位3")
    print("  Side按钮: 0x80 (128) - 位7")
    print("  Menu按钮: 0x20 (32)  - 位5")
    print()
    print("右按钮 (字节4):")
    print("  A按钮:    0x04 (4)   - 位2")
    print("  B按钮:    0x02 (2)   - 位1")
    print("  C按钮:    0x01 (1)   - 位0")
    print("  Side按钮: 0x40 (64)  - 位6")
    print("  Menu按钮: 0x10 (16)  - 位4")
    print("  Test按钮: 0x08 (8)   - 位3")
    print("  Service按钮: 0x04 (4) - 位2")

if __name__ == "__main__":
    print("🔧 ESP32 GPIO状态测试工具")
    print("=" * 50)
    
    analyze_button_mapping()
    
    print("\n" + "=" * 50)
    test_gpio_status()
    
    print("\n💡 建议:")
    print("1. 确保GPIO配置为输入模式并启用上拉电阻")
    print("2. 检查GPIO引脚是否有外部连接")
    print("3. 验证按钮映射逻辑是否正确")
    print("4. 如果问题持续，可能需要添加下拉电阻") 