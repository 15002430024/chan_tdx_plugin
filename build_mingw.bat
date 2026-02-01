@echo off
:: ============================================================================
:: 缠论通达信DLL插件 - MinGW 构建脚本
:: ============================================================================

echo ============================================
echo   缠论通达信DLL插件 - MinGW 构建
echo ============================================
echo.

cd /d "%~dp0"

:: 设置MinGW路径（如果不在PATH中，修改这里）
set GCC_PATH=D:\Mingw64\mingw64\bin

:: 检查编译器
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    if exist "%GCC_PATH%\g++.exe" (
        set PATH=%GCC_PATH%;%PATH%
    ) else (
        echo 错误: 未找到g++编译器
        pause
        exit /b 1
    )
)

:: 创建输出目录
if not exist "bin" mkdir bin

echo.
echo [编译] 32位 Release DLL...
echo.

g++ -m32 -shared -static -std=c++17 -O2 -Wall ^
    -DNDEBUG -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS ^
    -I"include" ^
    -o "bin/chan.dll" ^
    "src/dllmain.cpp" ^
    "src/tdx_interface.cpp" ^
    "src/logger.cpp" ^
    -lkernel32 -luser32

if %errorlevel% neq 0 (
    echo.
    echo 编译失败!
    pause
    exit /b 1
)

:: 验证DLL
echo.
echo [验证] 检查导出函数...
objdump -p bin/chan.dll | findstr "RegisterTdxFunc"
if %errorlevel% neq 0 (
    echo 警告: 未找到导出函数 RegisterTdxFunc
) else (
    echo 导出函数正常
)

:: 显示文件信息
echo.
echo [完成] 输出文件:
dir bin\chan.dll

echo.
echo ============================================
echo   构建成功!
echo ============================================
echo.
echo 下一步: 将 bin\chan.dll 复制到通达信 T0002\dlls\ 目录
echo.
pause
