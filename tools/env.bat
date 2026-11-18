@echo off
rem ============================================================
rem  env.bat - shared environment for the helper scripts.
rem  Called by setup.bat / build_all.bat / flash.bat / idf_cmd.bat.
rem  Do not run it directly.
rem
rem  It does three things:
rem    1. Clears MSYSTEM (injected by Git Bash; ESP-IDF rejects it)
rem    2. Chooses the toolchain directory (IDF_TOOLS_PATH)
rem    3. Finds a Python and puts its folder first in PATH
rem
rem  IMPORTANT: ESP-IDF picks its venv name from the Python version that
rem  starts export.bat (idf5.5_py3.11_env, idf5.5_py3.13_env, ...).
rem  So if an IDF venv already exists we use THAT python, otherwise a
rem  newer system Python would make export.bat look for a venv that does
rem  not exist and fail.
rem
rem  NOTE: keep this file ASCII-only. cmd.exe reads .bat files with the
rem  OEM code page, so UTF-8 text here would be mangled on Chinese Windows.
rem ============================================================

set "MSYSTEM="

if not defined IDF_TOOLS_PATH set "IDF_TOOLS_PATH=%USERPROFILE%\.espressif"

rem ---------------- find Python ----------------
set "PYEXE="

rem 1) Prefer the python inside an existing ESP-IDF venv
if defined IDF_TOOLS_PATH (
    for /d %%D in ("%IDF_TOOLS_PATH%\python_env\idf*_env") do (
        if not defined PYEXE if exist "%%~D\Scripts\python.exe" set "PYEXE=%%~D\Scripts\python.exe"
    )
)

rem 2) Then the Python launcher (installed by python.org by default)
if not defined PYEXE (
    where py >nul 2>&1
    if not errorlevel 1 (
        for /f "delims=" %%P in ('py -3 -c "import sys;print(sys.executable)" 2^>nul') do set "PYEXE=%%P"
    )
)

rem 3) Then python.exe on PATH (skip the Microsoft Store stub)
if not defined PYEXE (
    for /f "delims=" %%P in ('where python.exe 2^>nul ^| findstr /i /v "WindowsApps"') do (
        if not defined PYEXE set "PYEXE=%%P"
    )
)

rem 4) Finally try common install locations
if not defined PYEXE (
    for %%D in (
        "%LOCALAPPDATA%\Programs\Python\Python313\python.exe"
        "%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
        "%LOCALAPPDATA%\Programs\Python\Python311\python.exe"
        "%USERPROFILE%\miniconda3\envs\espidf\python.exe"
        "%USERPROFILE%\anaconda3\envs\espidf\python.exe"
        "C:\Python313\python.exe"
        "C:\Python312\python.exe"
        "C:\Python311\python.exe"
    ) do (
        if not defined PYEXE if exist "%%~D" set "PYEXE=%%~D"
    )
)

if not defined PYEXE exit /b 1

rem Put Python's folder first: install.bat and export.bat call python.exe directly
for %%P in ("%PYEXE%") do set "PATH=%%~dpP;%PATH%"

exit /b 0
