@echo off
chcp 65001 >nul
title E.N.C Controller 驱动安装工具

echo.
echo ========================================
echo    E.N.C Controller 驱动安装工具
echo ========================================
echo.

:: 检查管理员权限
echo [1-4]检查管理员权限...
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo.
    echo 错误：需要管理员权限才能安装驱动！
    echo.
    echo 解决方法：
    echo    1. 右键点击此脚本
    echo    2. 选择"以管理员身份运行"
    echo    3. 点击"是"确认
    echo.
    pause
    exit /b 1
)

echo 已获得管理员权限
echo

:: 检查驱动文件
echo [2-4]检查驱动文件...
if not exist "winusb_driver\winusb.inf" (
    echo 错误：找不到驱动文件！
    echo.
    echo 请确保以下文件存在：
    echo    winusb_driver\winusb.inf
    echo    winusb_driver\WinUSB.sys
    echo    winusb_driver\winusbcoinstaller2.dll
    echo.
    pause
    exit /b 1
)

if not exist "winusb_driver\WinUSB.sys" (
    echo 错误：找不到 WinUSB.sys 文件！
    pause
    exit /b 1
)

if not exist "winusb_driver\winusbcoinstaller2.dll" (
    echo 错误：找不到 winusbcoinstaller2.dll 文件！
    pause
    exit /b 1
)

echo 所有驱动文件完整
echo

:: 安装驱动
echo [3-4]安装 WinUSB 驱动...
echo 正在安装驱动文件...

:: 复制驱动文件到系统目录
echo 复制 WinUSB.sys...
copy "winusb_driver\WinUSB.sys" "%SystemRoot%\System32\drivers\" >nul 2>&1
if %errorLevel% neq 0 (
    echo WinUSB.sys 复制失败，可能已存在
)

echo 复制 winusbcoinstaller2.dll...
copy "winusb_driver\winusbcoinstaller2.dll" "%SystemRoot%\System32\" >nul 2>&1
if %errorLevel% neq 0 (
    echo winusbcoinstaller2.dll 复制失败，可能已存在
)

:: 安装驱动包
echo 注册驱动包...
pnputil /add-driver "winusb_driver\winusb.inf" /install
if %errorLevel% neq 0 (
    echo 驱动安装失败！
    echo.
    echo 可能的原因：
    echo    1. 驱动文件损坏
    echo    2. 系统不支持此驱动
    echo    3. 权限不足
    echo.
    echo 尝试手动安装：
    echo    1. 打开设备管理器
    echo    2. 找到 E.N.C Controller 设备
    echo    3. 右键 → 更新驱动程序
    echo    4. 浏览我的电脑以查找驱动程序
    echo    5. 选择 winusb_driver 文件夹
    echo.
    pause
    exit /b 1
)

echo 驱动安装成功！
echo

:: 配置设备
echo [4-4]配置设备...
echo 正在配置设备驱动...

:: 扫描设备
echo 扫描设备...
pnputil /scan-devices >nul 2>&1

:: 等待设备重新枚举
echo 等待设备重新枚举...
timeout /t 3 /nobreak >nul

echo 设备配置完成
echo

:: 安装完成
echo
echo ========================================
echo           安装完成！
echo ========================================
echo
echo 驱动已成功安装
echo 设备已配置完成
echo
echo 现在您可以：
echo    1. 在设备管理器中看到 "E.N.C Controller" 设备
echo    2. 使用 E.N.C Controller 进行游戏
echo    3. 享受即插即用的体验
echo
echo 如果遇到问题：
echo    1. 断开设备连接
echo    2. 重新连接设备
echo    3. 或在设备管理器中右键设备 → 更新驱动程序
echo
echo 按任意键退出...
pause >nul 