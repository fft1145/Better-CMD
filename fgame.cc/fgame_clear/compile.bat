@echo off
chcp 65001 >nul
title bcmd_clear 自动编译工具
setlocal enabledelayedexpansion

echo.
echo ========================================================
echo           bcmd_clear 自动编译脚本 v1.0
echo ========================================================
echo.
echo   源文件: bcmd_clear.cpp
echo   输出:   bcmd_clear.exe
echo   目标:  Windows 10 x86 (32位)
echo.
echo ========================================================
echo.

REM ========================================
REM 1. 检查源文件是否存在
REM ========================================
if not exist "bcmd_clear.cpp" (
    echo [错误] 未找到 bcmd_clear.cpp 文件!
    echo        请确保 compile.bat 和 bcmd_clear.cpp 在同一目录下。
    echo.
    pause
    exit /b 1
)

REM ========================================
REM 2. 检测可用的C++编译器
REM ========================================
echo [1/3] 检测C++编译器...

set "COMPILER=none"

REM 检查 g++ (MinGW)
where g++ >nul 2>&1
if %errorlevel% equ 0 (
    set "COMPILER=g++"
    echo       找到 g++ ^(MinGW^) - 将使用此编译器
    goto :found_compiler
)

REM 检查 cl.exe (MSVC)
where cl >nul 2>&1
if %errorlevel% equ 0 (
    set "COMPILER=cl"
    echo       找到 cl.exe ^(MSVC^) - 将使用此编译器
    goto :found_compiler
)

echo.
echo [错误] 未找到任何C++编译器!
echo.
echo   请安装以下之一:
echo     [1] MinGW-w64 ^(推荐^): https://jmeubank.github.io/tdm-gcc/
echo         安装后确保 g++.exe 在 PATH 环境变量中
echo     [2] Visual Studio Build Tools ^(MSVC^)
echo.
echo   VSCode 配置:
echo     1. 安装 C/C++ 扩展 ^(ms-vscode.cpptools^)
echo     2. 按 Ctrl+Shift+P -> "C/C++: Edit Configurations"
echo     3. 确保 compilerPath 指向 g++ 或 cl.exe
echo.
pause
exit /b 1

:found_compiler
echo.

REM ========================================
REM 3. 编译
REM ========================================
echo [2/3] 开始编译...

if "%COMPILER%"=="g++" (
    REM ---- MinGW g++ 编译 ----
    REM 第一次尝试: 静态链接 (独立exe)
    echo       尝试静态编译 ^(推荐, 生成独立exe^)...
    g++ -O2 -std=c++17 -static -o bcmd_clear.exe bcmd_clear.cpp -lurlmon -lole32 -ladvapi32 -lshell32 2>nul
    if !errorlevel! equ 0 (
        echo       静态编译成功!
        goto :compile_ok
    )

    REM 第二次尝试: 非静态链接
    echo       静态编译失败, 尝试普通编译...
    g++ -O2 -std=c++17 -o bcmd_clear.exe bcmd_clear.cpp -lurlmon -lole32 -ladvapi32 -lshell32
    if !errorlevel! equ 0 (
        echo       编译成功! ^(非静态链接, 运行时需DLL^)
        goto :compile_ok
    )

    REM 第三次尝试: 添加字符集选项
    echo       普通编译失败, 尝试添加字符集选项...
    g++ -O2 -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -o bcmd_clear.exe bcmd_clear.cpp -lurlmon -lole32 -ladvapi32 -lshell32
    if !errorlevel! equ 0 (
        echo       编译成功! ^(含字符集选项^)
        goto :compile_ok
    )

    echo.
    echo [错误] g++ 编译失败! 错误信息:
    echo --------------------------------------------------------
    g++ -O2 -std=c++17 -o bcmd_clear.exe bcmd_clear.cpp -lurlmon -lole32 -ladvapi32 -lshell32
    echo --------------------------------------------------------
    echo.
    echo   常见问题:
    echo     - 缺少 urlmon 库: 请安装完整的 MinGW-w64
    echo     - 缺少 windows.h: 请安装 MinGW-w64 ^(非纯GCC^)
    echo.
    pause
    exit /b 1
)

if "%COMPILER%"=="cl" (
    REM ---- MSVC cl.exe 编译 ----
    echo       使用 MSVC 编译...
    cl /O2 /EHsc /utf-8 /Fe:bcmd_clear.exe bcmd_clear.cpp /link urlmon.lib ole32.lib advapi32.lib shell32.lib
    if !errorlevel! equ 0 (
        echo       编译成功!
        goto :compile_ok
    )

    echo.
    echo [错误] MSVC 编译失败! 请检查错误信息。
    echo.
    pause
    exit /b 1
)

:compile_ok
echo.

REM ========================================
REM 4. 验证输出
REM ========================================
echo [3/3] 验证输出文件...

if exist "bcmd_clear.exe" (
    echo.
    echo ========================================================
    echo   编译成功!
    echo ========================================================
    echo.
    echo   输出文件: bcmd_clear.exe
    echo.
    for %%A in (bcmd_clear.exe) do (
        echo   文件大小: %%~zA 字节
    )
    echo.
    echo   使用方法:
    echo     1. 右键 bcmd_clear.exe
    echo     2. 选择 "以管理员身份运行"
    echo     3. 程序会自动:
    echo        - 终止并卸载360软件
    echo        - 移除IE浏览器
    echo        - 下载最新版Edge安装包
    echo        - 下载最新版Defender病毒库
    echo.
    echo   注意: 程序运行时需要联网下载Edge和Defender
    echo.
    echo ========================================================
    echo.
    echo 是否立即运行 bcmd_clear.exe? ^(将请求管理员权限^)
    set /p "runnow=输入 Y 运行, 其他键退出: "
    if /i "!runnow!"=="Y" (
        echo.
        echo 正在以管理员身份启动...
        powershell -Command "Start-Process 'bcmd_clear.exe' -Verb RunAs"
    )
) else (
    echo.
    echo [错误] 编译似乎成功但未找到输出文件!
    pause
    exit /b 1
)

echo.
pause
