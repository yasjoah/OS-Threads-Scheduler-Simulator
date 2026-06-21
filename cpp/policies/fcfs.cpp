#include "policies.hpp"
#include <limits>

int pick_fcfs(const std::vector<Job>& jobs, int current_time) {
    int best = -1;
    int best_release = std::numeric_limits<int>::max();

    for (int i = 0; i < (int)jobs.size(); ++i) {
        const auto& j = jobs[i];
        if (j.remaining_time <= 0) continue;
        if (j.release_time > current_time) continue;

        // FCFS: smallest release_time; tie by task/job
        if (j.release_time < best_release) {
            best_release = j.release_time;
            best = i;
        } else if (j.release_time == best_release && best != -1) {
            const auto& b = jobs[best];
            if (j.task_id < b.task_id || (j.task_id == b.task_id && j.job_id < b.job_id)) {
                best = i;
            }
        }
    }
    return best;
}
