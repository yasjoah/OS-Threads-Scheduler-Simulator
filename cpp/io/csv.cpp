#include "csv.hpp"
#include <fstream>

void write_timeline_csv(const std::string& path, const std::vector<Segment>& timeline) {
    std::ofstream out(path);
    out << "time_start,time_end,cpu_state,job\n";
    for (const auto& seg : timeline) {
        out << seg.start << "," << seg.end << "," << seg.cpu_state << "," << seg.job << "\n";
    }
}

void write_jobs_csv(const std::string& path, const std::vector<Job>& jobs) {
    std::ofstream out(path);
    out << "task_id,job_id,release,start,finish,deadline,response,missed,preemptions\n";
    for (const auto& j : jobs) {
        int response = (j.finish_time == -1) ? -1 : (j.finish_time - j.release_time);
        out << j.task_id << "," << j.job_id << ","
            << j.release_time << ","
            << j.start_time << ","
            << j.finish_time << ","
            << j.abs_deadline << ","
            << response << ","
            << (j.missed_deadline ? 1 : 0) << ","
            << j.preemptions << "\n";
    }
}

void write_misses_csv(const std::string& path, const std::vector<Job>& jobs, int sim_ms) {
    std::ofstream out(path);
    out << "job,task_id,release,deadline,finish,lateness_ms,status\n";
    for (const auto& j : jobs) {
        bool unfinished = j.finish_time == -1;
        if (!unfinished && !j.missed_deadline) continue;

        std::string status = unfinished ? "UNFINISHED" : "MISSED";
        int finish = unfinished ? -1 : j.finish_time;
        int lateness = unfinished ? -1 : (j.finish_time - j.abs_deadline);

        out << j.label() << ","
            << j.task_id << ","
            << j.release_time << ","
            << j.abs_deadline << ","
            << finish << ","
            << lateness << ","
            << status << "\n";
    }
}
