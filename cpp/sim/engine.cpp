#include "engine.hpp"
#include "../policies/policies.hpp"
#include <algorithm>

static std::vector<Job> generate_jobs(const std::vector<Task>& tasks, int sim_ms) {
    std::vector<Job> jobs;
    for (const auto& t : tasks) {
        int job_counter = 1;
        for (int rel = t.phase_ms; rel < sim_ms; rel += t.period_ms) {
            Job j;
            j.task_id = t.id;
            j.job_id = job_counter++;
            j.release_time = rel;
            j.abs_deadline = rel + t.deadline_ms;
            j.remaining_time = t.wcet_ms;
            jobs.push_back(j);
        }
    }

    std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        if (a.release_time != b.release_time) return a.release_time < b.release_time;
        if (a.task_id != b.task_id) return a.task_id < b.task_id;
        return a.job_id < b.job_id;
    });

    return jobs;
}

static int pick_job(
    PolicyKind policy,
    std::vector<Job>& jobs,
    int t,
    const std::vector<Task>& tasks,
    RRState& rr,
    bool force_rr_rotate
) {
    switch (policy) {
        case PolicyKind::EDF:  return pick_edf(jobs, t);
        case PolicyKind::FCFS: return pick_fcfs(jobs, t);
        case PolicyKind::RM:   return pick_rm(jobs, t, tasks);
        case PolicyKind::RR:   return pick_rr(jobs, t, rr, force_rr_rotate);
    }
    return -1;
}

SimResult run_simulation(
    const std::vector<Task>& tasks,
    const SimConfig& cfg,
    PolicyKind policy
) {
    SimResult res;
    res.jobs = generate_jobs(tasks, cfg.sim_ms);

    RRState rr;
    rr.quantum_ms = cfg.rr_quantum_ms;

    int current_job_idx = -1;
    int rr_quantum_left = cfg.rr_quantum_ms;

    std::string current_label = "IDLE";
    int seg_start = 0;

    auto label_for = [&](int idx) -> std::string {
        return (idx == -1) ? "IDLE" : res.jobs[idx].label();
    };

    for (int t = 0; t < cfg.sim_ms; ++t) {
        bool force_rotate = (policy == PolicyKind::RR && current_job_idx != -1 && rr_quantum_left <= 0);
        int next_idx = pick_job(policy, res.jobs, t, tasks, rr, force_rotate);

        std::string next_label = label_for(next_idx);

        if (t == 0) {
            current_job_idx = next_idx;
            current_label = next_label;
            seg_start = 0;
            rr_quantum_left = cfg.rr_quantum_ms;
        } else if (next_label != current_label) {
            res.timeline.push_back({seg_start, t, (current_label == "IDLE" ? "IDLE" : "RUN"), current_label});
            seg_start = t;

            if (current_job_idx != -1 && next_idx != current_job_idx) {
                if (res.jobs[current_job_idx].remaining_time > 0) {
                    res.jobs[current_job_idx].preemptions += 1;
                }
            }

            current_job_idx = next_idx;
            current_label = next_label;
            rr_quantum_left = cfg.rr_quantum_ms;
        }

        if (current_job_idx != -1) {
            Job& j = res.jobs[current_job_idx];
            if (j.release_time <= t && j.remaining_time > 0) {
                if (j.start_time == -1) j.start_time = t;

                j.remaining_time -= 1;
                if (policy == PolicyKind::RR) rr_quantum_left -= 1;

                if (j.remaining_time == 0) {
                    j.finish_time = t + 1;
                    if (j.finish_time > j.abs_deadline) j.missed_deadline = true;
                    rr_quantum_left = cfg.rr_quantum_ms;
                }
            }
        } else {
            rr_quantum_left = cfg.rr_quantum_ms;
        }
    }

    res.timeline.push_back({seg_start, cfg.sim_ms, (current_label == "IDLE" ? "IDLE" : "RUN"), current_label});
    return res;
}
