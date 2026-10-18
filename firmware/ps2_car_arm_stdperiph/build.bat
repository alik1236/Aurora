@echo off
rem ============================================================
rem  build.bat — 一键编译（使用 STM32CubeIDE 自带工具链）
rem  生成 firmware.elf / firmware.hex
rem ============================================================
setlocal
set "PLUGINS=F:\Aurora-\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins"
set "GCC=%PLUGINS%\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin"
set "MAKE=%PLUGINS%\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin"
set "PATH=%GCC%;%MAKE%;%PATH%"

cd /d "%~dp0"
echo [1/2] 清理旧文件...
make clean
echo [2/2] 编译...
make all
if errorlevel 1 (
    echo.
    echo 编译失败，请检查上方错误信息。
    pause
    exit /b 1
)
echo.
echo 编译成功：firmware.hex 已生成，插好 ST-Link 后双击 flash.bat 烧录。
pause
