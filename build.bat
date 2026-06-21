@echo off
setlocal
cd /d "%~dp0cpp"

where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: g++ not found. Install MSYS2/UCRT64 and add it to PATH.
    exit /b 1
)

echo Building simulator...
g++ -std=c++17 -O2 -Wall -Wextra ^
    main.cpp sim/engine.cpp ^
    policies/fcfs.cpp policies/edf.cpp policies/rm.cpp policies/rr.cpp ^
    io/csv.cpp io/html_report.cpp sim/tasks.cpp ^
    -o sim.exe

if errorlevel 1 (
    echo Build FAILED.
    exit /b 1
)

echo Build OK: cpp\sim.exe
exit /b 0
