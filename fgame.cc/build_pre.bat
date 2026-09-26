@echo off
chcp 936 >nul
echo ==================================
echo 编译【发布版静态exe】给别人用
echo ==================================
set /p opt=确认编译?(Y确认/其他退出):
if /i not "%opt%"=="Y" goto end

g++ "bcmd_prerequisite_files.cpp" -o "bcmd_prerequisite_files.exe" ^
-finput-charset=UTF-8 -fexec-charset=GBK ^
-lshell32 -liphlpapi -static -O2

if %errorlevel% equ 0 (
    echo.
    echo ✅发布版编译成功！
) else (
    echo.
    echo ❌编译出错！
)
:end
pause