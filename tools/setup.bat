@echo off
setlocal
rem Project root = parent of tools
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

rem ============================================================
rem  One-time environment setup.
rem
rem    1. Checks Git / Python
rem    2. Downloads ESP-IDF v5.5.6 into .\esp-idf (skipped if present)
rem    3. Installs the toolchain into %IDF_TOOLS_PATH%
rem
rem  To change the ESP-IDF version, edit IDF_VER below.
rem ============================================================
set "IDF_VER=v5.5.6"
set "IDF_TARGETS=esp32s3"

echo ============================================================
echo   ESP32 environment setup
echo   ESP-IDF: %IDF_VER%    targets: %IDF_TARGETS%
echo ============================================================
echo.

call "%ROOT%\tools\env.bat"
if errorlevel 1 goto :no_python

"%PYEXE%" -c "import sys; sys.exit(0 if (3,8) <= sys.version_info[:2] <= (3,13) else 1)" >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Unsupported Python version. Need 3.8 - 3.13 ^(3.11 recommended^).
    echo        Current: %PYEXE%
    goto :fail
)
echo [INFO] Python     : %PYEXE%
echo [INFO] Tools path : %IDF_TOOLS_PATH%
echo.

where git >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Git not found. Install Git for Windows:
    echo        https://git-scm.com/download/win
    goto :fail
)

rem ---------------- download ESP-IDF ----------------
if exist "esp-idf\export.bat" (
    echo [1/2] esp-idf already present, skipping download
) else (
    echo [1/2] Downloading ESP-IDF %IDF_VER% ... ^(first run: 5-10 min^)
    git clone -b %IDF_VER% --depth 1 --recursive https://github.com/espressif/esp-idf.git esp-idf
    if errorlevel 1 (
        echo.
        echo [FAIL] Download failed. Common causes:
        echo        1. GitHub is slow or blocked - use a proxy and retry
        echo        2. Not enough disk space - need at least 6 GB
        goto :fail
    )
)

rem ---------------- install toolchain ----------------
echo.
echo [2/2] Installing toolchain ... ^(first run: about 10 min^)
rem Use the Tsinghua mirror for Python packages (faster in China)
set "PIP_INDEX_URL=https://pypi.tuna.tsinghua.edu.cn/simple"
call "%ROOT%\esp-idf\install.bat" %IDF_TARGETS%
if errorlevel 1 (
    echo.
    echo [FAIL] Toolchain installation failed. Paste the log above in the group chat.
    goto :fail
)

echo ready> "%ROOT%\.setup_done"

echo.
echo ============================================================
echo   Done. Next steps:
echo     tools\build_all.bat
echo     tools\flash.bat hub COM5
echo ============================================================
if /i not "%~1"=="--no-pause" pause
exit /b 0

:no_python
echo [FAIL] Python not found. Install Python 3.11:
echo        https://www.python.org/downloads/release/python-3118/
echo        Check "Add python.exe to PATH" during installation, then run this again.
goto :fail

:fail
if /i not "%~1"=="--no-pause" pause
exit /b 1
