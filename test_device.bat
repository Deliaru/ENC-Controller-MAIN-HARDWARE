@echo off
chcp 65001 >nul
title E.N.C Controller 设备测试工具

echo.
echo ========================================
echo    E.N.C Controller 设备测试工具
echo ========================================
echo.

echo 正在检查设备状态...
echo.

:: 检查设备是否连接
echo [1/3] 检查设备连接...
devcon find USB\VID_0E8F^&PID_1216 >nul 2>&1
if %errorLevel% neq 0 (
    echo 未检测到 E.N.C Controller 设备
    echo.
    echo 请确保：
    echo 1. 设备已连接到USB端口
    echo 2. 设备已通电（LED灯亮起）
    echo 3. 驱动已正确安装
    echo.
    pause
    exit /b 1
) else (
    echo 检测到 E.N.C Controller 设备
)

:: 检查设备管理器状态
echo.
echo [2/3] 检查设备管理器状态...
echo 正在查询设备信息...

:: 使用wmic获取设备信息
for /f "tokens=2 delims==" %%i in ('wmic path win32_pnpentity where "DeviceID like '%%VID_0E8F&PID_1216%%'" get Name /format:list ^| findstr "Name="') do (
    set device_name=%%i
)

if defined device_name (
    echo 设备名称: %device_name%
) else (
    echo 无法获取设备名称
)

:: 检查设备状态
for /f "tokens=2 delims==" %%i in ('wmic path win32_pnpentity where "DeviceID like '%%VID_0E8F&PID_1216%%'" get Status /format:list ^| findstr "Status="') do (
    set device_status=%%i
)

if defined device_status (
    if "%device_status%"=="OK" (
        echo 设备状态: 正常
    ) else (
        echo 设备状态: %device_status%
    )
) else (
    echo 无法获取设备状态
)

:: 检查驱动信息
echo.
echo [3/3] 检查驱动信息...
for /f "tokens=2 delims==" %%i in ('wmic path win32_pnpentity where "DeviceID like '%%VID_0E8F&PID_1216%%'" get Service /format:list ^| findstr "Service="') do (
    set device_service=%%i
)

if defined device_service (
    if "%device_service%"=="WinUSB" (
        echo 驱动服务: WinUSB (正确)
    ) else (
        echo 驱动服务: %device_service%
    )
) else (
    echo 无法获取驱动信息
)

echo.
echo ========================================
echo           测试结果
echo ========================================
echo.

if defined device_name (
    if "%device_name%"=="E.N.C Controller" (
        echo 设备名称正确
    ) else (
        echo 设备名称可能需要更新
    )
)

if defined device_status (
    if "%device_status%"=="OK" (
        echo 设备工作正常
    ) else (
        echo 设备存在问题
    )
)

if defined device_service (
    if "%device_service%"=="WinUSB" (
        echo 驱动安装正确
    ) else (
        echo 驱动可能有问题
    )
)

echo.
echo 使用建议：
echo    1. 如果所有项目都显示正常，设备已准备就绪
echo    2. 如果有问题，请参考安装指南
echo    3. 可以尝试断开并重新连接设备
echo.
echo 按任意键退出...
pause >nul 