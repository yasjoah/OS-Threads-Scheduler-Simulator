#pragma once
#include <string>
#include <vector>

struct Task {
    int id{};
    int period_ms{};    // T
    int wcet_ms{};      // C
    int deadline_ms{};  // D (relative)
    int phase_ms{};     // phi
    int priority{};     // for fixed-priority (optional; if 0, RM can derive)
};

struct Job {
    int task_id{};
    int job_id{};

    int release_time{};     // ms
    int abs_deadline{};     // release + D
    int remaining_time{};   // ms left

    int start_time{-1};     // first time it ran
    int finish_time{-1};    // end time
    int preemptions{0};
    bool missed_deadline{false};

    std::string label() const {
        return "T" + std::to_string(task_id) + "J" + std::to_string(job_id);
    }
};

struct Segment {
    int start{};
    int end{};
    std::string cpu_state; // "RUN" or "IDLE"
    std::string job;       // label or "IDLE"
};

struct SimConfig {
    int sim_ms{200};
    int rr_quantum_ms{5}; // used only for RR
};

struct SimResult {
    std::vector<Job> jobs;
    std::vector<Segment> timeline;
};
