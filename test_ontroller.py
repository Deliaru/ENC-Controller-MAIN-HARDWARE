#!/usr/bin/env python3
"""
Ontroller.WinUSB.IO 兼容性测试脚本
用于测试ESP32设备是否能正确与Ontroller.WinUSB.IO库通信
"""

import usb.core
import usb.util
import time
import struct

# Ontroller设备信息
VENDOR_ID = 0x0E8F
PRODUCT_ID = 0x1216

def find_ontroller_device():
    """查找Ontroller设备"""
    device = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
    if device is None:
        print(f"未找到Ontroller设备 (VID: {VENDOR_ID:04X}, PID: {PRODUCT_ID:04X})")
        return None
    
    print(f"找到Ontroller设备: {device}")
    return device

def test_device_communication(device):
    """测试设备通信"""
    try:
        # 设置配置
        device.set_configuration()
        
        # 获取接口
        cfg = device.get_active_configuration()
        intf = cfg[(0,0)]
        
        # 查找端点
        ep_in = usb.util.find_descriptor(
            intf,
            custom_match=lambda e: 
                usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN
        )
        
        ep_out = usb.util.find_descriptor(
            intf,
            custom_match=lambda e: 
                usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT
        )
        
        if ep_in is None or ep_out is None:
            print("未找到输入或输出端点")
            return False
            
        print(f"输入端点: {ep_in}")
        print(f"输出端点: {ep_out}")
        
        # 测试读取数据（7字节）
        print("\n测试读取设备数据...")
        try:
            data = device.read(ep_in.bEndpointAddress, 7, timeout=1000)
            print(f"读取到数据: {data.hex()}")
            
            # 验证数据格式
            if len(data) == 7 and data[0] == 0x44 and data[1] == 0x44 and data[2] == 0x54:
                print("✓ 数据格式正确")
            else:
                print("✗ 数据格式错误")
                return False
                
        except usb.core.USBError as e:
            print(f"读取数据失败: {e}")
            return False
        
        # 测试发送LED数据（33字节）
        print("\n测试发送LED数据...")
        try:
            # 构造测试LED数据
            led_data = bytearray(33)
            led_data[0:3] = [0x44, 0x4C, 0x01]  # 头部
            
            # 设置一些测试颜色
            for i in range(6):
                base = 3 + i * 3
                led_data[base] = 255 if i % 3 == 0 else 0      # R
                led_data[base + 1] = 255 if i % 3 == 1 else 0  # G
                led_data[base + 2] = 255 if i % 3 == 2 else 0  # B
            
            # 设置侧边LED
            for i in range(4):
                base = 21 + i * 3
                led_data[base] = 128      # R
                led_data[base + 1] = 128  # G
                led_data[base + 2] = 128  # B
            
            device.write(ep_out.bEndpointAddress, led_data, timeout=1000)
            print(f"发送LED数据: {led_data.hex()}")
            print("✓ LED数据发送成功")
            
        except usb.core.USBError as e:
            print(f"发送LED数据失败: {e}")
            return False
        
        return True
        
    except Exception as e:
        print(f"通信测试失败: {e}")
        return False

def main():
    """主函数"""
    print("Ontroller.WinUSB.IO 兼容性测试")
    print("=" * 50)
    
    # 查找设备
    device = find_ontroller_device()
    if device is None:
        return
    
    # 测试通信
    if test_device_communication(device):
        print("\n✓ 所有测试通过！设备与Ontroller.WinUSB.IO兼容")
    else:
        print("\n✗ 测试失败，设备可能不兼容")
    
    print("\n测试完成")

if __name__ == "__main__":
    main() 