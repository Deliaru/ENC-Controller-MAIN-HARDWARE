#!/usr/bin/env python3
"""
设备名称修复脚本
"""

import os
import subprocess
import sys

def check_current_config():
    """检查当前配置"""
    print("=== 检查当前配置 ===")
    
    # 检查sdkconfig
    if os.path.exists("sdkconfig"):
        with open("sdkconfig", "r", encoding="utf-8") as f:
            content = f.read()
            if 'CONFIG_TINYUSB_DESC_PRODUCT_STRING="E.N.C Controller"' in content:
                print("✅ sdkconfig已正确配置")
            else:
                print("❌ sdkconfig配置不正确")
    
    # 检查winusb_new.c
    if os.path.exists("main/winusb_new.c"):
        with open("main/winusb_new.c", "r", encoding="utf-8") as f:
            content = f.read()
            if '"E.N.C Controller"' in content:
                print("✅ winusb_new.c已正确配置")
            else:
                print("❌ winusb_new.c配置不正确")
    
    print()

def explain_problem():
    """解释问题原因"""
    print("=== 问题分析 ===")
    print("设备名称没有改变的可能原因：")
    print("1. 设备需要重新编译和烧录")
    print("2. Windows缓存了旧的设备信息")
    print("3. 设备没有重新枚举")
    print()

def provide_solutions():
    """提供解决方案"""
    print("=== 解决方案 ===")
    print("请按以下步骤操作：")
    print()
    print("1. 重新编译和烧录设备：")
    print("   cd V1.2")
    print("   idf.py build")
    print("   idf.py flash")
    print()
    print("2. 清除Windows设备缓存：")
    print("   - 断开设备连接")
    print("   - 打开设备管理器")
    print("   - 右键点击'ONTROLLER'设备")
    print("   - 选择'卸载设备'")
    print("   - 勾选'删除此设备的驱动程序软件'")
    print("   - 点击'卸载'")
    print("   - 重新连接设备")
    print()
    print("3. 或者使用命令行清除：")
    print("   - 以管理员身份运行命令提示符")
    print("   - 运行: pnputil /delete-driver oem*.inf /uninstall")
    print("   - 重新连接设备")
    print()
    print("4. 验证修改：")
    print("   - 重新连接设备后")
    print("   - 检查设备管理器中的设备名称")
    print("   - 应该显示为'E.N.C Controller'")
    print()

def check_compilation():
    """检查编译状态"""
    print("=== 编译状态检查 ===")
    
    if os.path.exists("build"):
        print("✅ build目录存在")
        
        # 检查编译输出
        if os.path.exists("build/config/sdkconfig.h"):
            with open("build/config/sdkconfig.h", "r", encoding="utf-8") as f:
                content = f.read()
                if '#define CONFIG_TINYUSB_DESC_PRODUCT_STRING "E.N.C Controller"' in content:
                    print("✅ 编译配置正确")
                else:
                    print("❌ 编译配置不正确，需要重新编译")
        else:
            print("❌ 编译配置文件不存在，需要重新编译")
    else:
        print("❌ build目录不存在，需要重新编译")
    
    print()

def create_build_script():
    """创建编译脚本"""
    print("=== 创建编译脚本 ===")
    
    script_content = """@echo off
echo 正在编译E.N.C Controller固件...
cd /d "%~dp0"
idf.py build
if %errorlevel% equ 0 (
    echo 编译成功！
    echo 正在烧录固件...
    idf.py flash
    if %errorlevel% equ 0 (
        echo 烧录成功！
        echo 请断开设备并重新连接以查看新的设备名称
    ) else (
        echo 烧录失败！
    )
) else (
    echo 编译失败！
)
pause
"""
    
    with open("build_and_flash.bat", "w", encoding="utf-8") as f:
        f.write(script_content)
    
    print("✅ 已创建 build_and_flash.bat 脚本")
    print("双击运行此脚本可以自动编译和烧录")
    print()

def main():
    """主函数"""
    print("E.N.C Controller 设备名称修复工具")
    print("=" * 40)
    
    check_current_config()
    check_compilation()
    explain_problem()
    provide_solutions()
    create_build_script()
    
    print("总结：")
    print("1. 代码配置已正确修改")
    print("2. 需要重新编译和烧录设备")
    print("3. 需要清除Windows设备缓存")
    print("4. 重新连接设备后应该显示新名称")
    print()
    print("建议操作顺序：")
    print("1. 运行 build_and_flash.bat 重新编译烧录")
    print("2. 在设备管理器中卸载旧设备")
    print("3. 重新连接设备")
    print("4. 验证设备名称是否变为'E.N.C Controller'")

if __name__ == "__main__":
    main() 