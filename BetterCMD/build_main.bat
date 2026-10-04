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

echo 编译图标资源...
windres bcmd.rc -O coff -o bcmd.res

g++ -std=c++17 -mwindows -O2 -static -static-libstdc++ -static-libgcc bcmd.cpp bcmd.res -o bcmd.exe -finput-charset=UTF-8 -fexec-charset=UTF-8 -lws2_32 -liphlpapi -lpsapi -lwinhttp -lshlwapi -lwinmm -lcomctl32 -lgdi32 -luser32 -ladvapi32 -lshell32 -lpowrprof -lwtsapi32 -ldwmapi

if %errorlevel% equ 0 (
    echo.
    echo ✅编译成功！输出：bcmd.exe
) else (
    echo.
    echo ❌编译出错！
)
pause
