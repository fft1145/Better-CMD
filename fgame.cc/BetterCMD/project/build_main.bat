@echo off
chcp 936 >nul
echo ==================================
echo 编译：bcmd.cpp (主体程序)
echo ==================================
set /p opt="确认编译?(Y确认 / 其他取消): "
if /i not "%opt%"=="Y" (
    echo 已取消
    pause
    exit
)

g++ -std=c++17 -mwindows -O2 bcmd.cpp -o bcmd.exe -finput-charset=UTF-8 -fexec-charset=GBK -lws2_32 -liphlpapi -lpsapi -lwinhttp -lshlwapi -lwinmm -lcomctl32 -lgdi32 -luser32 -ladvapi32 -lshell32 -lpowrprof -lwtsapi32

if %errorlevel% equ 0 (
    echo.
    echo ✅编译成功！输出：bcmd.exe
) else (
    echo.
    echo ❌编译出错！
)
pause
