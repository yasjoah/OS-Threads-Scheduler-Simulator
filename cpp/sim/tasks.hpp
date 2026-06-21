#pragma once
#include "../scheduler.hpp"
#include <string>

enum class RandomWorkload { Mixed, Safe, Heavy };

struct TaskGenConfig {
    int count{3};
    unsigned seed{42};
    int period_min{10};
    int period_max{50};
    int wcet_min{1};
    int phase_max{10};
    RandomWorkload workload{RandomWorkload::Mixed};
};

struct RandomTaskSet {
    std::vector<Task> tasks;
    bool overloaded_intent{};   // what we aimed for this run
    double utilization{};     // sum(wcet/period)
    std::string workload_label; // e.g. "schedulable" or "overloaded"
};

RandomTaskSet generate_random_tasks(const TaskGenConfig& cfg);
std::vector<Task> overload_tasks();
double compute_utilization(const std::vector<Task>& tasks);
void print_tasks(const std::vector<Task>& tasks);
void print_deadline_report(const std::vector<Job>& jobs, int sim_ms);
