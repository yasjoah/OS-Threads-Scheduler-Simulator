#pragma once
#include "../scheduler.hpp"
#include <string>

void write_timeline_csv(const std::string& path, const std::vector<Segment>& timeline);
void write_jobs_csv(const std::string& path, const std::vector<Job>& jobs);
void write_misses_csv(const std::string& path, const std::vector<Job>& jobs, int sim_ms);
