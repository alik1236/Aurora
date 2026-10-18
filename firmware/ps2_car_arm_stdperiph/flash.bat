@echo off
rem ============================================================
rem  flash.bat — 一键烧录（STM32CubeIDE 内置 STM32_Programmer_CLI）
rem  前置：ST-Link 已插好并安装驱动，核心板已接 SWD（PA13/PA14）
rem        烧录前检查：ST-Link SWDIO->PA13、SWCLK->PA14、GND->GND
rem ============================================================
setlocal
set "PLUGINS=F:\Aurora-\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins"
set "CLI=%PLUGINS%\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"

cd /d "%~dp0"
if not exist firmware.hex (
    echo 找不到 firmware.hex，请先运行 build.bat 编译。
    pause
    exit /b 1
)

echo 正在连接 ST-Link 并烧录...
"%CLI%" -c port=SWD mode=UR -w firmware.hex -v -rst
if errorlevel 1 (
    echo.
    echo 烧录失败。常见原因：
    echo   1. ST-Link 未插好 / 驱动未装（用 D:\桌面\ST-Link驱动\stlink_winusb_install.bat 安装）
    echo   2. SWD 线序接错（SWDIO-PA13、SWCLK-PA14、GND 必须共地）
    echo   3. 核心板没有供电
    pause
    exit /b 1
)
echo.
echo 烧录成功！程序已写入并复位运行。
pause
