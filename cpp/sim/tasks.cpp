#include "tasks.hpp"
#include <random>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

double compute_utilization(const std::vector<Task>& tasks) {
    double u = 0.0;
    for (const auto& t : tasks) {
        if (t.period_ms > 0) u += static_cast<double>(t.wcet_ms) / t.period_ms;
    }
    return u;
}

static void assign_wcets_for_target(std::vector<Task>& tasks, double target_u, std::mt19937& rng, int wcet_min) {
    std::vector<double> weights(tasks.size());
    std::uniform_real_distribution<double> wdist(0.4, 1.0);
    for (auto& w : weights) w = wdist(rng);
    double wsum = std::accumulate(weights.begin(), weights.end(), 0.0);

    for (size_t i = 0; i < tasks.size(); ++i) {
        double share = (wsum > 0) ? weights[i] / wsum : 1.0 / tasks.size();
        double ui = target_u * share;
        int wcet = static_cast<int>(std::round(ui * tasks[i].period_ms));
        wcet = std::max(wcet_min, wcet);
        wcet = std::min(wcet, tasks[i].period_ms - 1);
        tasks[i].wcet_ms = wcet;
    }

    // Nudge total utilization toward target
    double actual = compute_utilization(tasks);
    if (actual <= 0) return;

    double scale = target_u / actual;
    for (auto& t : tasks) {
        int wcet = static_cast<int>(std::round(t.wcet_ms * scale));
        wcet = std::max(wcet_min, wcet);
        wcet = std::min(wcet, t.period_ms - 1);
        t.wcet_ms = wcet;
    }
}

RandomTaskSet generate_random_tasks(const TaskGenConfig& cfg) {
    std::mt19937 rng(cfg.seed);
    std::uniform_int_distribution<int> period_dist(cfg.period_min, cfg.period_max);
    std::uniform_int_distribution<int> phase_dist(0, cfg.phase_max);
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    bool want_overload = false;
    switch (cfg.workload) {
        case RandomWorkload::Safe:   want_overload = false; break;
        case RandomWorkload::Heavy:  want_overload = true;  break;
        case RandomWorkload::Mixed:  want_overload = coin(rng) < 0.5; break;
    }

    double target_u;
    if (want_overload) {
        std::uniform_real_distribution<double> heavy(1.10, 1.60);
        target_u = heavy(rng);
    } else {
        std::uniform_real_distribution<double> light(0.30, 0.70);
        target_u = light(rng);
    }

    RandomTaskSet result;
    result.overloaded_intent = want_overload;
    result.workload_label = want_overload ? "overloaded" : "schedulable";
    result.tasks.reserve(cfg.count);

    for (int i = 0; i < cfg.count; ++i) {
        Task t;
        t.id = i + 1;
        t.period_ms = period_dist(rng);
        t.deadline_ms = t.period_ms;
        t.phase_ms = std::min(phase_dist(rng), t.period_ms - 1);
        t.priority = 0;
        t.wcet_ms = cfg.wcet_min;
        result.tasks.push_back(t);
    }

    assign_wcets_for_target(result.tasks, target_u, rng, cfg.wcet_min);
    result.utilization = compute_utilization(result.tasks);
    return result;
}

std::vector<Task> overload_tasks() {
    return {
        {1, 10, 8, 10, 0, 0},
        {2, 10, 8, 10, 0, 0},
        {3, 20, 7, 20, 5, 0},
    };
}

void print_tasks(const std::vector<Task>& tasks) {
    std::cout << "Tasks:\n";
    std::cout << "  id  period  wcet  deadline  phase  util\n";
    for (const auto& t : tasks) {
        double u = (t.period_ms > 0)
            ? 100.0 * t.wcet_ms / t.period_ms
            : 0.0;
        std::cout << "  T" << t.id << "   "
                  << t.period_ms << " ms   "
                  << t.wcet_ms << " ms   "
                  << t.deadline_ms << " ms    "
                  << t.phase_ms << " ms   "
                  << static_cast<int>(u + 0.5) << "%\n";
    }
    std::cout << "  Total CPU utilization: "
              << static_cast<int>(compute_utilization(tasks) * 100 + 0.5) << "%\n";
}

void print_deadline_report(const std::vector<Job>& jobs, int sim_ms) {
    int missed = 0, unfinished = 0;
    std::unordered_map<int, int> misses_by_task;
    std::unordered_map<int, int> unfinished_by_task;

    for (const auto& j : jobs) {
        if (j.finish_time == -1) {
            unfinished++;
            unfinished_by_task[j.task_id]++;
        } else if (j.missed_deadline) {
            missed++;
            misses_by_task[j.task_id]++;
        }
    }

    if (missed == 0 && unfinished == 0) {
        std::cout << "\nDeadline check: ALL JOBS MET THEIR DEADLINES\n";
        return;
    }

    std::cout << "\n=== DEADLINE VIOLATIONS ===\n";

    if (missed > 0) {
        std::cout << "\nMissed deadlines (" << missed << " job(s)):\n";
        std::cout << "  job      release  deadline  finish  late_by\n";
        for (const auto& j : jobs) {
            if (j.finish_time == -1 || !j.missed_deadline) continue;
            int late = j.finish_time - j.abs_deadline;
            std::cout << "  " << j.label()
                      << "   " << j.release_time << " ms"
                      << "   " << j.abs_deadline << " ms"
                      << "   " << j.finish_time << " ms"
                      << "   +" << late << " ms\n";
        }
    }

    if (unfinished > 0) {
        std::cout << "\nUnfinished when sim ended at " << sim_ms << " ms (" << unfinished << " job(s)):\n";
        std::cout << "  job      release  deadline  remaining\n";
        for (const auto& j : jobs) {
            if (j.finish_time != -1) continue;
            std::cout << "  " << j.label()
                      << "   " << j.release_time << " ms"
                      << "   " << j.abs_deadline << " ms"
                      << "   " << j.remaining_time << " ms left\n";
        }
    }

    std::cout << "\nBy task:\n";
    std::unordered_map<int, int> all_tasks;
    for (const auto& j : jobs) all_tasks[j.task_id] = 0;

    std::vector<int> task_ids;
    task_ids.reserve(all_tasks.size());
    for (const auto& [tid, _] : all_tasks) task_ids.push_back(tid);
    std::sort(task_ids.begin(), task_ids.end());

    for (int tid : task_ids) {
        int m = misses_by_task.count(tid) ? misses_by_task[tid] : 0;
        int u = unfinished_by_task.count(tid) ? unfinished_by_task[tid] : 0;
        if (m == 0 && u == 0) {
            std::cout << "  Task " << tid << ": OK (all jobs met deadline)\n";
        } else {
            std::cout << "  Task " << tid << ": "
                      << m << " missed, " << u << " unfinished\n";
        }
    }

    std::cout << "\nSee outputs/misses.csv for the full violation list.\n";
}
