#include "policies.hpp"
#include <unordered_map>

// Rate Monotonic: higher priority = smaller period.
// If Task.priority is nonzero, we use it instead (smaller number = higher priority here).
int pick_rm(const std::vector<Job>& jobs, int current_time, const std::vector<Task>& tasks) {
    std::unordered_map<int, Task> task_by_id;
    for (const auto& t : tasks) task_by_id[t.id] = t;

    int best = -1;

    auto prio = [&](int task_id) -> int {
        const auto& t = task_by_id[task_id];
        if (t.priority != 0) return t.priority;
        return t.period_ms; // smaller period => higher priority
    };

    for (int i = 0; i < (int)jobs.size(); ++i) {
        const auto& j = jobs[i];
        if (j.remaining_time <= 0) continue;
        if (j.release_time > current_time) continue;

        if (best == -1) best = i;
        else {
            const auto& b = jobs[best];
            int pj = prio(j.task_id);
            int pb = prio(b.task_id);

            if (pj < pb ||
                (pj == pb && j.release_time < b.release_time) ||
                (pj == pb && j.release_time == b.release_time && j.task_id < b.task_id) ||
                (pj == pb && j.release_time == b.release_time && j.task_id == b.task_id && j.job_id < b.job_id)) {
                best = i;
            }
        }
    }
    return best;
}
