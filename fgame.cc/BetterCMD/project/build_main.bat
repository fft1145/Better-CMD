@echo off
chcp 936 >nul
echo ==================================
echo 编译：BetterCMD\project\bcmd.cpp
echo ==================================
set /p opt="确认编译?(Y确认 / 其他取消): "
if /i not "%opt%"=="Y" (
    echo 已取消
    pause
    exit
)

g++ "\fgame.cc\BetterCMD\project\bcmd.cpp" -o "\fgame.cc\BetterCMD\project\bcmd.exe" -finput-charset=UTF-8 -fexec-charset=GBK -lshell32

if %errorlevel% equ 0 (
    echo.
    echo ✅编译成功！输出：BetterCMD\project\bcmd.exe
) else (
    echo.
    echo ❌编译出错！
)
pause