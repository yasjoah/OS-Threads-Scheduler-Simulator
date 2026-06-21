#include "policies.hpp"
#include <algorithm>

static void rr_refresh_ready(std::vector<Job>& jobs, int current_time, RRState& st) {
    for (int i = 0; i < (int)jobs.size(); ++i) {
        const auto& j = jobs[i];
        if (j.remaining_time <= 0) continue;
        if (j.release_time > current_time) continue;

        if (std::find(st.ready.begin(), st.ready.end(), i) == st.ready.end()) {
            st.ready.push_back(i);
        }
    }

    st.ready.erase(std::remove_if(st.ready.begin(), st.ready.end(),
                  [&](int idx){ return jobs[idx].remaining_time <= 0; }),
                  st.ready.end());
}

int pick_rr(std::vector<Job>& jobs, int current_time, RRState& st, bool force_rotate) {
    rr_refresh_ready(jobs, current_time, st);
    if (st.ready.empty()) return -1;

    if (!force_rotate && st.last_idx != -1) {
        auto it = std::find(st.ready.begin(), st.ready.end(), st.last_idx);
        if (it != st.ready.end() && jobs[st.last_idx].remaining_time > 0) {
            return st.last_idx;
        }
    }

    auto it = std::find(st.ready.begin(), st.ready.end(), st.last_idx);
    int pos = (it == st.ready.end()) ? -1 : (int)std::distance(st.ready.begin(), it);
    int next_pos = (pos + 1) % (int)st.ready.size();
    st.last_idx = st.ready[next_pos];
    return st.last_idx;
}
