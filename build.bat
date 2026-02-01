@echo off
:: ============================================================================
:: 缠论通达信DLL插件 - 一键构建脚本
:: ============================================================================

echo ============================================
echo   缠论通达信DLL插件 构建脚本
echo ============================================
echo.

:: 检查是否在正确目录
if not exist "CMakeLists.txt" (
    echo 错误: 请在项目根目录运行此脚本!
    pause
    exit /b 1
)

:: 创建构建目录
if not exist "build" mkdir build
cd build

:: 检测Visual Studio版本
set VS_VERSION=
where cl >nul 2>&1
if %errorlevel%==0 (
    echo 检测到MSVC编译器
) else (
    echo 警告: 未检测到MSVC编译器，尝试使用默认设置...
)

:: 生成项目 (Win32)
echo.
echo [1/2] 生成CMake项目...
cmake -G "Visual Studio 17 2022" -A Win32 .. 
if %errorlevel% neq 0 (
    echo.
    echo 尝试使用 Visual Studio 2019...
    cmake -G "Visual Studio 16 2019" -A Win32 ..
)
if %errorlevel% neq 0 (
    echo.
    echo CMake 配置失败!
    cd ..
    pause
    exit /b 1
)

:: 编译Release版本
echo.
echo [2/2] 编译Release版本...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo.
    echo 编译失败!
    cd ..
    pause
    exit /b 1
)

:: 编译Debug版本 (可选)
echo.
echo 编译Debug版本...
cmake --build . --config Debug

:: 完成
echo.
echo ============================================
echo   构建完成!
echo ============================================
echo.
echo 输出文件:
echo   Release: build\bin\Release\chan.dll
echo   Debug:   build\bin\Debug\chan.dll
echo.

cd ..
pause
