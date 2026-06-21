#pragma once
#include "../scheduler.hpp"

enum class PolicyKind { EDF, FCFS, RM, RR };

SimResult run_simulation(
    const std::vector<Task>& tasks,
    const SimConfig& cfg,
    PolicyKind policy
);
