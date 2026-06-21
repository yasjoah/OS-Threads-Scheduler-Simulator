@echo off
setlocal EnableDelayedExpansion
set "ROOT=%~dp0"
cd /d "%ROOT%"

set "DO_PLOTS=0"
set "SIM_ARGS="

:parse
if "%~1"=="" goto done_parse
if /I "%~1"=="--plots" (
    set "DO_PLOTS=1"
    shift
    goto parse
)
set "SIM_ARGS=!SIM_ARGS! %1"
shift
goto parse

:done_parse

if not exist "%ROOT%cpp\sim.exe" (
    call "%ROOT%build.bat"
    if errorlevel 1 exit /b 1
)

echo.
cd /d "%ROOT%cpp"
if not defined SIM_ARGS (
    echo Running simulation with policy: edf
    sim.exe edf
) else (
    echo Running: sim.exe!SIM_ARGS!
    sim.exe!SIM_ARGS!
)
if errorlevel 1 (
    echo Simulation FAILED.
    exit /b 1
)

cd /d "%ROOT%"

if "%DO_PLOTS%"=="1" (
    call "%ROOT%plots.bat"
    if errorlevel 1 exit /b 1
)

set "REPORT=%ROOT%outputs\report.html"
echo.
echo Opening report in browser...
start "" "%REPORT%"

echo.
echo Done.
if "%DO_PLOTS%"=="1" (
    echo   Report:  "%REPORT%"
    echo   Plots:   "%ROOT%outputs\plots.html"
) else (
    echo   Report:  "%REPORT%"
    echo   PNG plots: .\plots.bat   ^(or .\run.bat --plots edf^)
)
exit /b 0
