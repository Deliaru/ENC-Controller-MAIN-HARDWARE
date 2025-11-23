@echo off
chcp 65001 >nul
title ESP32 模拟数据测试工具

echo.
echo ========================================
echo    ESP32 模拟数据测试工具 - Windows
echo ========================================
echo.

echo 正在检查Python环境...
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ 未找到Python，请先安装Python 3.6+
    echo 💡 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

echo ✅ Python已安装
echo.

echo 正在检查pyusb库...
python -c "import usb.core" >nul 2>&1
if errorlevel 1 (
    echo ❌ 未找到pyusb库，正在安装...
    pip install pyusb
    if errorlevel 1 (
        echo ❌ 安装失败，请手动安装: pip install pyusb
        pause
        exit /b 1
    )
    echo ✅ pyusb安装成功
) else (
    echo ✅ pyusb库已安装
)

echo.
echo 正在启动测试程序...
echo.

python test_simulation.py

echo.
echo 测试完成，按任意键退出...
pause >nul 