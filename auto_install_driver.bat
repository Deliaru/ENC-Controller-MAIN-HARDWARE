@echo off
chcp 65001 >nul
title E.N.C Controller 驱动自动安装工具

echo.
echo ========================================
echo    E.N.C Controller 驱动自动安装工具
echo ========================================
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ❌ 错误：需要管理员权限才能安装驱动！
    echo.
    echo 请右键点击此脚本，选择"以管理员身份运行"
    echo.
    pause
    exit /b 1
)

echo ✅ 已获得管理员权限
echo.

:: 检查驱动文件是否存在
if not exist "winusb_driver\winusb.inf" (
    echo ❌ 错误：找不到驱动文件！
    echo 请确保 winusb_driver 文件夹存在且包含驱动文件
    echo.
    pause
    exit /b 1
)

echo ✅ 找到驱动文件
echo.

:: 检查设备是否已连接
echo 正在检查设备连接状态...
devcon find USB\VID_0E8F^&PID_1216 >nul 2>&1
if %errorLevel% neq 0 (
    echo ⚠️  警告：未检测到 E.N.C Controller 设备
    echo.
    echo 请确保：
    echo 1. 设备已连接到USB端口
    echo 2. 设备已通电
    echo 3. 设备正常工作
    echo.
    echo 是否继续安装驱动？(Y/N)
    set /p choice=
    if /i "%choice%" neq "Y" (
        echo 安装已取消
        pause
        exit /b 0
    )
) else (
    echo ✅ 检测到 E.N.C Controller 设备
)

echo.
echo 开始安装驱动...

:: 备份现有驱动（如果存在）
echo 备份现有驱动...
if exist "%SystemRoot%\System32\DriverStore\FileRepository\winusb.inf_*" (
    echo 发现现有驱动，正在备份...
)

:: 安装驱动
echo 正在安装 WinUSB 驱动...
pnputil /add-driver "winusb_driver\winusb.inf" /install
if %errorLevel% neq 0 (
    echo ❌ 驱动安装失败！
    echo.
    echo 可能的原因：
    echo 1. 驱动文件损坏
    echo 2. 系统不支持此驱动
    echo 3. 权限不足
    echo.
    pause
    exit /b 1
)

echo ✅ 驱动安装成功！
echo.

:: 更新设备驱动
echo 正在更新设备驱动...
devcon update "winusb_driver\winusb.inf" USB\VID_0E8F^&PID_1216
if %errorLevel% neq 0 (
    echo ⚠️  设备驱动更新失败，但驱动已安装
    echo 请手动在设备管理器中更新驱动
) else (
    echo ✅ 设备驱动更新成功！
)

echo.
echo ========================================
echo           安装完成！
echo ========================================
echo.
echo ✅ 驱动已成功安装
echo ✅ 设备已配置完成
echo.
echo 现在您可以：
echo 1. 在设备管理器中看到 "E.N.C Controller" 设备
echo 2. 使用 E.N.C Controller 进行游戏
echo 3. 享受即插即用的体验
echo.
echo 如果设备显示为黄色感叹号，请：
echo 1. 断开设备连接
echo 2. 重新连接设备
echo 3. 或在设备管理器中右键设备 → 更新驱动程序
echo.
pause 