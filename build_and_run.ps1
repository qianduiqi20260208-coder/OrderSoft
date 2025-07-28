# 一键编译并运行脚本
Write-Host "正在配置CMake项目..." -ForegroundColor Green
cmake -S . -B build -G "MinGW Makefiles"
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake配置失败！" -ForegroundColor Red
    Read-Host "按任意键退出"
    exit 1
}

Write-Host "正在编译项目..." -ForegroundColor Green
cmake --build build
if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败！" -ForegroundColor Red
    Read-Host "按任意键退出"
    exit 1
}

Write-Host "编译成功！正在运行程序..." -ForegroundColor Green
Write-Host ""
& "build\bin\ModelLifeManager.exe"
Write-Host ""
Write-Host "程序运行结束。" -ForegroundColor Yellow
Read-Host "按任意键退出"
