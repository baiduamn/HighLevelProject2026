@echo off
setlocal
rem Project root = parent of tools
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

call "%ROOT%\tools\env.bat"
if errorlevel 1 (
    echo [FAIL] Python not found. Run tools\setup.bat first.
    pause
    exit /b 1
)
if not exist "esp-idf\export.bat" (
    echo [FAIL] esp-idf not found. Run tools\setup.bat first.
    pause
    exit /b 1
)

call "%ROOT%\esp-idf\export.bat"
where idf.py >nul 2>&1
if errorlevel 1 (
    echo [FAIL] idf.py is not available after export.
    echo        Delete .setup_done and run tools\setup.bat again.
    pause
    exit /b 1
)
cd /d "%ROOT%"

echo ============================================================
echo   ESP-IDF shell ready. Project root: %ROOT%
echo   Try:  idf.py --version
echo   Exit: exit
echo ============================================================
cmd /k
