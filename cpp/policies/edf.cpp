#include "policies.hpp"

int pick_edf(const std::vector<Job>& jobs, int current_time) {
    int best = -1;
    for (int i = 0; i < (int)jobs.size(); ++i) {
        const auto& j = jobs[i];
        if (j.remaining_time <= 0) continue;
        if (j.release_time > current_time) continue;

        if (best == -1) best = i;
        else {
            const auto& b = jobs[best];
            if (j.abs_deadline < b.abs_deadline ||
                (j.abs_deadline == b.abs_deadline && j.task_id < b.task_id) ||
                (j.abs_deadline == b.abs_deadline && j.task_id == b.task_id && j.job_id < b.job_id)) {
                best = i;
            }
        }
    }
    return best;
}
