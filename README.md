Real-Time OS Scheduler Simulator

A single-core CPU scheduling simulator for periodic real-time tasks. Implements EDF, Rate-Monotonic, FCFS, and Round-Robin in C++, then produces CSV data and an HTML report you can open in any browser.

## Quick Start (Windows)

```bat
run.bat
```

This builds the simulator (if needed), runs EDF, and opens `outputs/report.html` in your browser.

Run a specific policy:

```bat
run.bat rm
run.bat fcfs
run.bat rr
```

Generate **random tasks** — each seed randomly picks schedulable (~30–70% CPU) or overloaded (~110–160% CPU):

```bat
.\run.bat edf --random
.\run.bat edf --random --seed 7
```

Force one or the other:

```bat
.\run.bat edf --random --safe     rem low CPU, usually no misses
.\run.bat edf --random --heavy    rem high CPU, usually misses
```

Fixed overload preset (always misses):

```bat
.\run.bat edf --overload
```

Or directly:

```bat
cd cpp
sim.exe edf --random --tasks 5 --seed 42
```

## Build manually

```bat
build.bat
cd cpp
sim.exe edf
```

Or with CMake:

```bat
cd cpp
cmake -B build
cmake --build build
build\sim.exe edf
```

## Policies

| Policy | Description |
|--------|-------------|
| `edf`  | Earliest Deadline First (dynamic priority, optimal on single core) |
| `rm`   | Rate-Monotonic (fixed priority by period) |
| `fcfs` | First-Come First-Served (non-preemptive baseline) |
| `rr`   | Round-Robin with configurable quantum |

## Outputs

Each run writes to `outputs/`:

- `timeline.csv` — CPU execution trace (Gantt segments)
- `jobs.csv` — per-job release, finish, response, deadline status
- `report.html` — interactive summary with Gantt chart and job table

## Optional Python plots

Generate PNG charts and a gallery page:

```bat
.\plots.bat
```

Or simulate and plot in one step:

```bat
.\run.bat --plots edf
.\run.bat --plots rm --random --tasks 5
```

This creates in `outputs/`:

- `gantt.png` — CPU timeline
- `response_times.png` — response time per job
- `jitter.png` — jitter per task
- `deadlines.png` — deadline slack / misses
- `plots.html` — gallery of all charts (opens in browser)

### Python setup

**MSYS2** (matches the g++ toolchain):

```bat
pacman -S mingw-w64-ucrt-x86_64-python-pandas mingw-w64-ucrt-x86_64-python-matplotlib
```

**Standard Python:**

```bat
pip install -r requirements.txt
```

## Task set

Default tasks in `cpp/main.cpp`:

| Task | Period | WCET | Deadline | Phase |
|------|--------|------|----------|-------|
| T1   | 10 ms  | 3 ms | 10 ms    | 0     |
| T2   | 25 ms  | 8 ms | 25 ms    | 0     |
| T3   | 40 ms  | 6 ms | 40 ms    | 5 ms  |

Edit these to experiment with different workloads.

## Requirements

- C++17 compiler (GCC via MSYS2 recommended on Windows)
- Web browser for HTML report
- Python 3 + pandas/matplotlib (optional, for PNG plots)
