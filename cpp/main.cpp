#include "scheduler.hpp"
#include "sim/engine.hpp"
#include "sim/tasks.hpp"
#include "policies/policies.hpp"
#include "io/csv.hpp"
#include "io/html_report.hpp"

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

static PolicyKind parse_policy(const std::string& raw) {
    std::string p = raw;
    std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c){ return std::tolower(c); });

    if (p == "edf")  return PolicyKind::EDF;
    if (p == "fcfs") return PolicyKind::FCFS;
    if (p == "rm")   return PolicyKind::RM;
    if (p == "rr")   return PolicyKind::RR;
    return PolicyKind::EDF;
}

static const char* policy_name(PolicyKind p) {
    switch (p) {
        case PolicyKind::EDF:  return "edf";
        case PolicyKind::FCFS: return "fcfs";
        case PolicyKind::RM:   return "rm";
        case PolicyKind::RR:   return "rr";
    }
    return "unknown";
}

static bool is_flag(const std::string& s) {
    return s.size() >= 2 && s[0] == '-' && s[1] == '-';
}

static std::vector<Task> default_tasks() {
    return {
        {1, 10, 3, 10, 0, 0},
        {2, 25, 8, 25, 0, 0},
        {3, 40, 6, 40, 5, 0}
    };
}

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <policy> [options]\n"
              << "  policy: edf | fcfs | rm | rr\n\n"
              << "Options:\n"
              << "  --random          Random tasks; each seed randomly schedulable OR overloaded\n"
              << "  --safe            With --random: always low CPU (aim for no misses)\n"
              << "  --heavy           With --random: always high CPU (aim for misses)\n"
              << "  --overload        Fixed overloaded task set (guaranteed misses)\n"
              << "  --tasks N         Number of random tasks (default: 3)\n"
              << "  --seed N          RNG seed (default: 42)\n"
              << "  --sim MS          Simulation length in ms (default: 200)\n\n"
              << "Examples:\n"
              << "  " << prog << " edf --random\n"
              << "  " << prog << " edf --random --seed 7\n"
              << "  " << prog << " edf --random --safe\n"
              << "  " << prog << " edf --random --heavy\n"
              << "  " << prog << " edf --overload\n";
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    std::string policy_arg = (argc >= 2) ? argv[1] : "edf";
    PolicyKind policy = parse_policy(policy_arg);

    bool use_random = false;
    bool use_overload = false;
    TaskGenConfig gen_cfg;
    RandomTaskSet random_set;
    SimConfig cfg;
    cfg.sim_ms = 200;
    cfg.rr_quantum_ms = 5;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "random" || arg == "--random") {
            use_random = true;
        } else if (arg == "overload" || arg == "--overload") {
            use_overload = true;
        } else if (arg == "safe" || arg == "--safe") {
            gen_cfg.workload = RandomWorkload::Safe;
            use_random = true;
        } else if (arg == "heavy" || arg == "--heavy") {
            gen_cfg.workload = RandomWorkload::Heavy;
            use_random = true;
        } else if (arg == "--tasks" && i + 1 < argc) {
            gen_cfg.count = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--seed" && i + 1 < argc) {
            gen_cfg.seed = static_cast<unsigned>(std::stoul(argv[++i]));
        } else if (arg == "--sim" && i + 1 < argc) {
            cfg.sim_ms = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (use_random && use_overload) {
        std::cerr << "Use only one of --random or --overload.\n";
        return 1;
    }

    std::vector<Task> tasks;
    if (use_random) {
        random_set = generate_random_tasks(gen_cfg);
        tasks = random_set.tasks;
    } else if (use_overload) {
        tasks = overload_tasks();
    } else {
        tasks = default_tasks();
    }
    const std::string name = policy_name(policy);

    SimResult result = run_simulation(tasks, cfg, policy);

    write_timeline_csv("../outputs/timeline.csv", result.timeline);
    write_jobs_csv("../outputs/jobs.csv", result.jobs);
    write_misses_csv("../outputs/misses.csv", result.jobs, cfg.sim_ms);
    write_html_report("../outputs/report.html", name, cfg, result, tasks, use_random, gen_cfg.seed);

    int finished = 0, missed = 0, unfinished = 0;
    for (const auto& j : result.jobs) {
        if (j.finish_time == -1) unfinished++;
        else {
            finished++;
            if (j.missed_deadline) missed++;
        }
    }

    std::cout << "Policy: " << name << "\n";
    if (use_random) {
        std::cout << "Task source: random (seed=" << gen_cfg.seed
                  << ", count=" << gen_cfg.count << ")\n";
        std::cout << "Workload target: " << random_set.workload_label
                  << " (" << static_cast<int>(random_set.utilization * 100 + 0.5)
                  << "% CPU)\n";
        print_tasks(tasks);
    } else if (use_overload) {
        std::cout << "Task source: overload (fixed preset)\n";
        print_tasks(tasks);
    } else {
        std::cout << "Task source: built-in\n";
    }
    std::cout << "Sim: " << cfg.sim_ms << " ms\n";
    std::cout << "Jobs total: " << result.jobs.size()
              << " | finished: " << finished
              << " | unfinished: " << unfinished
              << " | missed: " << missed << "\n";
    print_deadline_report(result.jobs, cfg.sim_ms);
    std::cout << "Wrote: outputs/timeline.csv, outputs/jobs.csv, outputs/misses.csv, outputs/report.html\n";
    return 0;
}
