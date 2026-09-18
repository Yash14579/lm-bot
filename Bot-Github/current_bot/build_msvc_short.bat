@echo off
setlocal
cd /d "%~dp0"
set "BUILD_DIR=%~dp0build"
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
cmake -S . -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%" --config Debug --parallel
if errorlevel 1 exit /b 1
echo Build complete: %BUILD_DIR%\Debug\client.exe
endlocal
