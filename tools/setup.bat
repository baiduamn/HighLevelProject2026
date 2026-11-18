@echo off
setlocal
rem Project root = parent of tools
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

rem ============================================================
rem  One-click environment setup. Just double-click this file.
rem
rem  Safe to run any number of times. Every step skips work that is
rem  already done, so an interrupted download continues instead of
rem  starting over.
rem
rem    1. Checks Git / Python
rem    2. Gets ESP-IDF IDF_VER (git clone, or a local zip)
rem    3. Fetches submodules with retries (resumable)
rem    4. Installs the toolchain into IDF_TOOLS_PATH
rem
rem  Optional environment variables (set before running):
rem    IDF_MIRROR  git mirror base used for espressif submodules,
rem                e.g. https://jihulab.com/esp-mirror/espressif
rem    IDF_ZIP     path to a complete esp-idf-*.zip; skips git entirely
rem
rem  To change the ESP-IDF version, edit IDF_VER below.
rem ============================================================
set "IDF_VER=v5.5.6"
set "IDF_TARGETS=esp32s3"
set "SUB_TRIES_MAX=10"

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

rem ---------------- 1. obtain ESP-IDF ----------------
rem If a complete esp-idf-*.zip sits in the project root, use it.
rem This is the offline path: hand teammates the zip, they just double-click.
if not defined IDF_ZIP (
    for %%Z in ("%ROOT%\esp-idf-*.zip") do if exist "%%~Z" set "IDF_ZIP=%%~Z"
)
if defined IDF_ZIP goto :from_zip

if exist "esp-idf\.git" goto :have_repo

if exist "esp-idf" (
    echo [FAIL] The esp-idf folder exists but is not a git repository.
    echo        Delete it and run this script again.
    goto :fail
)

echo [1/3] Cloning ESP-IDF %IDF_VER% ^(shallow^) ...
git -c http.lowSpeedLimit=1000 -c http.lowSpeedTime=60 clone -b %IDF_VER% --depth 1 https://github.com/espressif/esp-idf.git esp-idf
if errorlevel 1 goto :clone_fail
goto :have_repo

:have_repo
if exist "esp-idf\export.bat" goto :submodules

echo [1/3] Restoring ESP-IDF checkout %IDF_VER% ...
git -C esp-idf fetch --depth 1 origin tag %IDF_VER% >nul 2>&1
git -C esp-idf checkout -f %IDF_VER%
if errorlevel 1 goto :clone_fail
goto :submodules

:from_zip
call :extract_zip
if errorlevel 1 goto :fail
echo [INFO] ESP-IDF loaded from %IDF_ZIP%
goto :toolchain

rem ---------------- 2. submodules (resumable, retried) ----------------
:submodules
echo [2/3] Fetching submodules ^(resumable, up to %SUB_TRIES_MAX% attempts^) ...

rem Give up on stalled transfers instead of hanging forever
git -C esp-idf config http.lowSpeedLimit 1000
git -C esp-idf config http.lowSpeedTime 60

rem Optional mirror for the espressif/* submodules
if defined IDF_MIRROR (
    echo       Using mirror: %IDF_MIRROR%
    git -C esp-idf config "url.%IDF_MIRROR%/.insteadOf" "https://github.com/espressif/"
)

set "SUB_TRIES=0"
:sub_retry
set /a SUB_TRIES+=1
echo       attempt %SUB_TRIES% of %SUB_TRIES_MAX% ...
git -C esp-idf submodule update --init --recursive --depth 1
if not errorlevel 1 goto :sub_ok
if %SUB_TRIES% GEQ %SUB_TRIES_MAX% goto :sub_fail

rem Back off a little longer after each failure (5s, 10s, ... max 30s)
set /a WAIT=SUB_TRIES*5
if %WAIT% GTR 30 set "WAIT=30"
echo       Network error. Retrying in %WAIT% s ^(finished submodules are kept^) ...
ping -n %WAIT% 127.0.0.1 >nul
goto :sub_retry

:sub_ok

rem ---------------- 3. install toolchain ----------------
:toolchain
echo.
echo [3/3] Installing toolchain ... ^(first run: about 10 min^)
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

rem ---------------- helpers ----------------
:extract_zip
rem Consider esp-idf complete only if export.bat and a few submodule files exist
set "IDF_OK=1"
if not exist "esp-idf\export.bat" set "IDF_OK="
if not exist "esp-idf\components\lwip\lwip\CMakeLists.txt" set "IDF_OK="
if not exist "esp-idf\components\json\cJSON\cJSON.c" set "IDF_OK="
if not exist "esp-idf\components\esp_wifi\lib\LICENSE" set "IDF_OK="
if defined IDF_OK (
    echo [1/3] esp-idf already present and complete, skipping archive
    exit /b 0
)
if not exist "%IDF_ZIP%" (
    echo [FAIL] IDF_ZIP points to a missing file: %IDF_ZIP%
    exit /b 1
)
if exist "esp-idf" (
    echo [1/3] Found a partial esp-idf, removing it before extracting ...
    attrib -r -s -h "esp-idf\*.*" /s /d >nul 2>&1
    rmdir /s /q "esp-idf"
    if exist "esp-idf" (
        echo [FAIL] Could not remove the partial esp-idf folder.
        echo        Close any program using it, delete it manually, then run again.
        exit /b 1
    )
)
echo [1/3] Extracting %IDF_ZIP% ... ^(this may take a few minutes^)
set "ZIP_TMP=%ROOT%\_idf_extract"
if exist "%ZIP_TMP%" rmdir /s /q "%ZIP_TMP%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -LiteralPath '%IDF_ZIP%' -DestinationPath '%ZIP_TMP%' -Force"
if errorlevel 1 (
    echo [FAIL] Failed to extract the archive.
    exit /b 1
)
if exist "%ZIP_TMP%\export.bat" (
    move "%ZIP_TMP%" "%ROOT%\esp-idf"
) else (
    for /d %%D in ("%ZIP_TMP%\*") do if exist "%%~D\export.bat" move "%%~D" "%ROOT%\esp-idf"
)
if not exist "esp-idf\export.bat" (
    echo [FAIL] Archive does not contain export.bat at its root or one level down.
    rmdir /s /q "%ZIP_TMP%" 2>nul
    exit /b 1
)
rmdir /s /q "%ZIP_TMP%" 2>nul
exit /b 0

rem ---------------- error paths ----------------
:no_python
echo [FAIL] Python not found. Install Python 3.11:
echo        https://www.python.org/downloads/release/python-3118/
echo        Check "Add python.exe to PATH" during installation, then run this again.
goto :fail

:clone_fail
echo.
echo [FAIL] Could not clone ESP-IDF. Common causes:
echo        1. GitHub is slow or blocked - enable a proxy and run this script again
echo           PowerShell:  $env:HTTPS_PROXY="http://127.0.0.1:7890"
echo           cmd:         set HTTPS_PROXY=http://127.0.0.1:7890
echo        2. Not enough disk space - need at least 6 GB
echo        Progress is kept. Fix the network, then run this script again.
goto :fail

:sub_fail
echo.
echo [FAIL] Could not fetch all submodules after %SUB_TRIES_MAX% attempts.
echo        This is a network problem. Finished submodules are kept, so run this
echo        script again later and it will continue from where it stopped.
echo.
echo        Options:
echo          - enable a proxy:      set HTTPS_PROXY=http://127.0.0.1:7890
echo          - use a git mirror:    set IDF_MIRROR=https://jihulab.com/esp-mirror/espressif
echo          - use an offline zip:  set IDF_ZIP=D:\path\esp-idf-v5.5.6.zip
goto :fail

:fail
if /i not "%~1"=="--no-pause" pause
exit /b 1
