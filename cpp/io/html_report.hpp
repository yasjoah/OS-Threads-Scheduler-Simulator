#pragma once
#include "../scheduler.hpp"
#include <string>

void write_timeline_csv(const std::string& path, const std::vector<Segment>& timeline);
void write_jobs_csv(const std::string& path, const std::vector<Job>& jobs);
void write_html_report(
    const std::string& path,
    const std::string& policy,
    const SimConfig& cfg,
    const SimResult& result,
    const std::vector<Task>& tasks = {},
    bool random_tasks = false,
    unsigned seed = 0
);
