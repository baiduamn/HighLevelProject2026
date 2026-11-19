@echo off
setlocal
rem Project root = parent of tools
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

call "%ROOT%\tools\env.bat"
if errorlevel 1 goto :no_python

rem Run setup automatically if the environment is not ready yet
if not exist "esp-idf\export.bat" goto :do_setup
if not exist ".setup_done" goto :do_setup
goto :build

:do_setup
echo [INFO] Environment not ready, running setup.bat ...
call "%ROOT%\tools\setup.bat" --no-pause
if errorlevel 1 exit /b 1
call "%ROOT%\tools\env.bat" >nul 2>&1

:build
call "%ROOT%\esp-idf\export.bat"
if errorlevel 1 (
    echo [FAIL] Failed to load the ESP-IDF environment. See the message above.
    echo        Delete .setup_done in the project root, then run tools\setup.bat again.
    pause
    exit /b 1
)
where idf.py >nul 2>&1
if errorlevel 1 (
    echo [FAIL] idf.py is not available after export.
    echo        The Python found by env.bat does not match the ESP-IDF venv.
    echo        Delete .setup_done and run tools\setup.bat again.
    pause
    exit /b 1
)

call :build_one firmware\hub  esp32s3
if errorlevel 1 goto :fail
call :build_one firmware\node esp32s3
if errorlevel 1 goto :fail

echo.
echo ===== All projects built successfully =====
echo   Firmware: ^<project^>\build\merged.bin
pause
exit /b 0

:build_one
echo.
echo ===== Building %~1  (target: %~2) =====
pushd "%ROOT%\%~1"
set "RC=0"
idf.py -DIDF_TARGET=%~2 build
if errorlevel 1 set "RC=1"
if "%RC%"=="0" (
    echo       merging bootloader + partition table + app ...
    idf.py merge-bin -o merged.bin
    if errorlevel 1 set "RC=1"
)
popd
if not "%RC%"=="0" exit /b 1
exit /b 0

:no_python
echo [FAIL] Python 3.8-3.13 not found.
echo        Run tools\setup.bat first.
pause
exit /b 1

:fail
echo.
echo [FAIL] Build stopped. See the error above.
pause
exit /b 1
