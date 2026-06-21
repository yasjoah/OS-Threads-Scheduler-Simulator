#pragma once
#include "../scheduler.hpp"

int pick_fcfs(const std::vector<Job>& jobs, int current_time);
int pick_edf(const std::vector<Job>& jobs, int current_time);
int pick_rm(const std::vector<Job>& jobs, int current_time, const std::vector<Task>& tasks);

struct RRState {
    int quantum_ms{5};
    int last_idx{-1};
    std::vector<int> ready;
};

int pick_rr(std::vector<Job>& jobs, int current_time, RRState& st, bool force_rotate);
