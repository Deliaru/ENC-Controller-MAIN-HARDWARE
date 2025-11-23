#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
模拟数据测试脚本 - Windows版本
用于验证ESP32设备的模拟按钮和摇杆数据
"""

import sys
import time
from datetime import datetime

# Windows USB库导入
try:
    import usb.core
    import usb.util
    import usb.backend.libusb1
except ImportError:
    print("❌ 缺少USB库，请安装: pip install pyusb")
    print("   如果安装失败，请先安装libusb驱动")
    sys.exit(1)

def find_ontroller_device():
    """查找Ontroller设备"""
    try:
        # 查找VID=0x0E8F, PID=0x1216的设备
        device = usb.core.find(idVendor=0x0E8F, idProduct=0x1216)
        return device
    except Exception as e:
        print(f"❌ 查找设备时出错: {e}")
        return None

def decode_button_state(left_buttons, right_buttons):
    """解码按钮状态"""
    buttons = []
    
    # 左按钮解码
    if left_buttons & 0x20: buttons.append("左A")
    if left_buttons & 0x10: buttons.append("左B")
    if left_buttons & 0x08: buttons.append("左C")
    if left_buttons & 0x80: buttons.append("左Side")
    if left_buttons & 0x20: buttons.append("左Menu")
    
    # 右按钮解码
    if right_buttons & 0x04: buttons.append("右A")
    if right_buttons & 0x02: buttons.append("右B")
    if right_buttons & 0x01: buttons.append("右C")
    if right_buttons & 0x40: buttons.append("右Side")
    if right_buttons & 0x10: buttons.append("右Menu")
    if right_buttons & 0x08: buttons.append("Test")
    if right_buttons & 0x04: buttons.append("Service")
    
    return buttons if buttons else ["无按钮"]

def decode_lever_value(lever_low, lever_high):
    """解码摇杆值"""
    raw_value = (lever_high << 8) | lever_low
    # Ontroller.cs的计算方式: (raw * 80) - 32767
    lever_value = (raw_value * 80) - 32767
    return lever_value, raw_value

def test_simulation():
    """测试模拟数据"""
    print("🔍 正在查找Ontroller设备...")
    device = find_ontroller_device()
    if device is None:
        print("❌ 未找到Ontroller设备")
        print("💡 请确保:")
        print("   1. ESP32设备已连接")
        print("   2. WinUSB驱动已安装")
        print("   3. 设备VID=0x0E8F, PID=0x1216")
        return False
    
    print("✅ 找到Ontroller设备")
    print(f"设备信息: VID=0x{device.idVendor:04X}, PID=0x{device.idProduct:04X}")
    
    try:
        # 设置配置
        print("🔧 配置设备...")
        device.set_configuration()
        
        # 查找输入端点 (EP 0x84)
        print("🔍 查找输入端点...")
        ep_in = None
        for cfg in device:
            for intf in cfg:
                for ep in intf:
                    if ep.bEndpointAddress == 0x84:
                        ep_in = ep
                        break
                if ep_in:
                    break
            if ep_in:
                break
        
        if ep_in is None:
            print("❌ 未找到输入端点0x84")
            return False
        
        print("✅ 找到输入端点0x84")
        
        print("\n🎮 开始测试模拟数据...")
        print("预期行为:")
        print("- 按钮: 每秒按顺序切换 (左A → 左B → 左C → ... → 无按钮)")
        print("- 摇杆: 5秒周期内线性变化 (0° → 360°)")
        print("- 按Ctrl+C停止测试")
        print()
        
        start_time = time.time()
        last_button_time = start_time
        last_lever_time = start_time
        data_count = 0
        
        while True:
            try:
                # 读取数据
                data = ep_in.read(7, timeout=1000)
                current_time = time.time()
                data_count += 1
                
                # 解析数据
                header = data[0:3]
                left_buttons = data[3]
                right_buttons = data[4]
                lever_low = data[5]
                lever_high = data[6]
                
                # 验证头部
                if header[0] != 0x44 or header[1] != 0x44 or header[2] != 0x54:
                    print(f"⚠️  警告: 头部验证失败 {[f'{b:02X}' for b in header]}")
                    continue
                
                # 解码按钮状态
                active_buttons = decode_button_state(left_buttons, right_buttons)
                
                # 解码摇杆值
                lever_value, raw_lever = decode_lever_value(lever_low, lever_high)
                
                # 计算角度 (反向计算)
                angle = ((lever_value + 32767) / 60000.0) * 360.0
                if angle < 0: angle = 0
                if angle > 360: angle = 360
                
                # 输出状态
                timestamp = datetime.now().strftime("%H:%M:%S")
                print(f"[{timestamp}] #{data_count:4d} 按钮: {', '.join(active_buttons):<15} | "
                      f"摇杆: {angle:5.1f}° ({lever_value:+6d}) | "
                      f"原始: {' '.join([f'{b:02X}' for b in data])}")
                
                # 检查按钮变化
                if current_time - last_button_time >= 1.0:
                    print(f"🔄 按钮切换: {', '.join(active_buttons)}")
                    last_button_time = current_time
                
                # 检查摇杆周期
                lever_cycle = (current_time - start_time) % 5.0
                if lever_cycle < 0.1:  # 每5秒开始时
                    print(f"🔄 摇杆周期开始: 角度={angle:.1f}°")
                
                time.sleep(0.1)  # 100ms采样间隔
                
            except usb.core.USBError as e:
                if e.errno == 110:  # 超时
                    print("⏰ 读取超时，设备可能断开连接")
                    break
                else:
                    print(f"❌ USB读取错误: {e}")
                    break
            except KeyboardInterrupt:
                print("\n⏹️  测试停止 (用户中断)")
                break
            except Exception as e:
                print(f"❌ 未知错误: {e}")
                break
                
    except Exception as e:
        print(f"❌ 设备访问错误: {e}")
        print("💡 可能的原因:")
        print("   1. 设备未正确连接")
        print("   2. 驱动未正确安装")
        print("   3. 权限不足 (尝试以管理员身份运行)")
        return False
    
    print(f"\n📊 测试统计: 共读取 {data_count} 个数据包")
    return True

def analyze_expected_pattern():
    """分析预期的数据模式"""
    print("📋 预期数据模式分析:")
    print()
    print("按钮序列 (每秒切换):")
    buttons = [
        ("左A", 0x20, 0x00),
        ("左B", 0x10, 0x00),
        ("左C", 0x08, 0x00),
        ("左Side", 0x80, 0x00),
        ("左Menu", 0x20, 0x00),
        ("右A", 0x00, 0x04),
        ("右B", 0x00, 0x02),
        ("右C", 0x00, 0x01),
        ("右Side", 0x00, 0x40),
        ("右Menu", 0x00, 0x10),
        ("Test", 0x00, 0x08),
        ("Service", 0x00, 0x04),
        ("无按钮", 0x00, 0x00)
    ]
    
    for i, (name, left, right) in enumerate(buttons):
        print(f"  {i+1:2d}. {name:8s}: 左=0x{left:02X}, 右=0x{right:02X}")
    
    print()
    print("摇杆变化 (5秒周期):")
    print("  时间  角度    原始值    编码值")
    for t in range(0, 6):
        angle = (t / 5.0) * 360.0
        lever_value = int((angle / 360.0) * 60000 - 30000)
        raw_lever = int((lever_value + 32767) / 80)
        print(f"  {t}s   {angle:5.1f}°  {lever_value:+6d}  0x{raw_lever:04X}")

def check_requirements():
    """检查运行环境"""
    print("🔍 检查运行环境...")
    
    # 检查Python版本
    if sys.version_info < (3, 6):
        print("❌ 需要Python 3.6或更高版本")
        return False
    
    # 检查USB库
    try:
        import usb.core
        print("✅ pyusb库已安装")
    except ImportError:
        print("❌ 缺少pyusb库")
        print("💡 安装命令: pip install pyusb")
        return False
    
    # 检查libusb
    try:
        import usb.backend.libusb1
        print("✅ libusb后端可用")
    except ImportError:
        print("⚠️  libusb后端不可用，可能影响设备访问")
    
    return True

def main():
    """主函数"""
    print("🎮 ESP32 模拟数据测试工具 - Windows版本")
    print("=" * 60)
    
    # 检查运行环境
    if not check_requirements():
        print("\n❌ 环境检查失败，请解决上述问题后重试")
        return
    
    print("\n" + "=" * 60)
    analyze_expected_pattern()
    
    print("\n" + "=" * 60)
    test_simulation()
    
    print("\n💡 使用说明:")
    print("1. 确保ESP32设备已连接并运行测试程序")
    print("2. 确保WinUSB驱动已正确安装")
    print("3. 如果遇到权限问题，请以管理员身份运行")
    print("4. 按钮测试: 每秒按顺序切换一个按钮")
    print("5. 摇杆测试: 5秒内从0°线性变化到360°")
    print("6. 数据格式: 44 44 54 [左按钮] [右按钮] [摇杆低] [摇杆高]")
    print("7. 按Ctrl+C停止测试")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n⏹️  程序被用户中断")
    except Exception as e:
        print(f"\n❌ 程序异常: {e}")
        import traceback
        traceback.print_exc()
    
    print("\n按任意键退出...")
    try:
        input()
    except:
        pass 