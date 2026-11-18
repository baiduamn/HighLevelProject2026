@echo off
setlocal
rem Project root = parent of tools
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

if "%~2"=="" goto usage

set "PROJ="
if /i "%~1"=="hub"  set "PROJ=firmware\hub"
if /i "%~1"=="node" set "PROJ=firmware\node"
if not defined PROJ goto usage
set "PORT=%~2"

call "%ROOT%\tools\env.bat"
if errorlevel 1 goto :no_python
if not exist "esp-idf\export.bat" goto :need_setup
if not exist ".setup_done" goto :need_setup

call "%ROOT%\esp-idf\export.bat"
if errorlevel 1 goto :need_setup
where idf.py >nul 2>&1
if errorlevel 1 goto :need_setup

pushd "%ROOT%\%PROJ%"
echo ===== Flashing %PROJ% to %PORT% =====
idf.py -p %PORT% flash monitor
set "RC=%errorlevel%"
popd
exit /b %RC%

:usage
echo Usage:
echo   flash.bat hub  COM5    ^(hub,  ESP32-S3^)
echo   flash.bat node COM6    ^(node, ESP32-S3^)
echo.
echo Press Ctrl+] to exit the serial monitor.
exit /b 1

:no_python
echo [FAIL] Python not found. Run tools\setup.bat first.
pause
exit /b 1

:need_setup
echo [FAIL] Environment not ready. Run tools\setup.bat first.
pause
exit /b 1
