@echo off
setlocal
set "ROOT=%~dp0"
cd /d "%ROOT%"

set "PYTHON="

if exist "C:\msys64\ucrt64\bin\python.exe" (
    C:\msys64\ucrt64\bin\python.exe -c "import pandas, matplotlib" >nul 2>&1
    if not errorlevel 1 set "PYTHON=C:\msys64\ucrt64\bin\python.exe"
)

if not defined PYTHON (
    where python >nul 2>&1
    if not errorlevel 1 (
        python -c "import pandas, matplotlib" >nul 2>&1
        if not errorlevel 1 set "PYTHON=python"
    )
)

if not defined PYTHON (
    where py >nul 2>&1
    if not errorlevel 1 (
        py -3 -c "import pandas, matplotlib" >nul 2>&1
        if not errorlevel 1 set "PYTHON=py -3"
    )
)

if not defined PYTHON (
    echo.
    echo ERROR: Python with pandas and matplotlib not found.
    echo.
    echo Option A - MSYS2 ^(if you use g++ from MSYS2^):
    echo   pacman -S mingw-w64-ucrt-x86_64-python-pandas mingw-w64-ucrt-x86_64-python-matplotlib
    echo.
    echo Option B - Standard Python:
    echo   pip install -r requirements.txt
    echo.
    exit /b 1
)

if not exist "%ROOT%outputs\timeline.csv" (
    echo ERROR: outputs\timeline.csv not found. Run .\run.bat first.
    exit /b 1
)

echo Using: %PYTHON%
echo Generating plots...
%PYTHON% "%ROOT%python\analyze.py" %*
if errorlevel 1 exit /b 1

set "PLOTS=%ROOT%outputs\plots.html"
echo.
echo Opening plots gallery...
start "" "%PLOTS%"
echo Done. Open "%PLOTS%" if the browser did not launch.
exit /b 0
