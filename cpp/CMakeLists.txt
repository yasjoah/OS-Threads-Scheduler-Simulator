cmake_minimum_required(VERSION 3.16)
project(scheduler_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sim
    main.cpp
    sim/engine.cpp
    policies/fcfs.cpp
    policies/edf.cpp
    policies/rm.cpp
    policies/rr.cpp
    io/csv.cpp
    io/html_report.cpp
    sim/tasks.cpp
)

if(MSVC)
    target_compile_options(sim PRIVATE /W4)
else()
    target_compile_options(sim PRIVATE -Wall -Wextra -O2)
endif()
