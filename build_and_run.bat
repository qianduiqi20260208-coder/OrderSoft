@echo off
echo 正在配置CMake项目...
cmake -S . -B build -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo CMake配置失败！
    pause
    exit /b 1
)

echo 正在编译项目...
cmake --build build
if %errorlevel% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo 编译成功！正在运行程序...
echo.
build\bin\ModelLifeManager.exe
echo.
echo 程序运行结束。
pause
