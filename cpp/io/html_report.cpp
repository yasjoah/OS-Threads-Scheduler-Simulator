#include "html_report.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <algorithm>

static const char* TASK_COLORS[] = {
    "#4e79a7", "#f28e2b", "#e15759", "#76b7b2", "#59a14f",
    "#edc948", "#b07aa1", "#ff9da7", "#9c755f", "#bab0ac"
};

static std::string html_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

static int task_id_from_label(const std::string& label) {
    if (label.size() < 2 || label[0] != 'T') return 0;
    size_t j = label.find('J');
    if (j == std::string::npos) return 0;
    try { return std::stoi(label.substr(1, j - 1)); }
    catch (...) { return 0; }
}

static std::string svg_gantt(const std::vector<Segment>& timeline, int sim_ms) {
    const int row_h = 28;
    const int margin_l = 90;
    const int margin_t = 30;
    const int margin_b = 40;
    const int chart_w = 900;

    std::unordered_map<int, int> task_rows;
    int num_rows = 0;

    for (const auto& seg : timeline) {
        if (seg.cpu_state != "RUN") continue;
        int tid = task_id_from_label(seg.job);
        if (tid == 0) continue;
        if (task_rows.find(tid) == task_rows.end()) {
            task_rows[tid] = num_rows++;
        }
    }

    if (num_rows == 0) num_rows = 1;

    int chart_h = num_rows * row_h;
    int total_h = margin_t + chart_h + margin_b;
    int total_w = margin_l + chart_w + 20;

    std::ostringstream svg;
    svg << "<svg width=\"" << total_w << "\" height=\"" << total_h
        << "\" xmlns=\"http://www.w3.org/2000/svg\" class=\"gantt\">\n";

    svg << "<text x=\"" << margin_l + chart_w / 2 << "\" y=\"18\" text-anchor=\"middle\" "
        << "font-size=\"14\" font-weight=\"600\">CPU Timeline (0–" << sim_ms << " ms)</text>\n";

    for (int t = 0; t <= sim_ms; t += 20) {
        double x = margin_l + (double)t / sim_ms * chart_w;
        svg << "<line x1=\"" << x << "\" y1=\"" << margin_t
            << "\" x2=\"" << x << "\" y2=\"" << margin_t + chart_h
            << "\" stroke=\"#e0e0e0\" stroke-width=\"1\"/>\n";
        svg << "<text x=\"" << x << "\" y=\"" << total_h - 10
            << "\" text-anchor=\"middle\" font-size=\"11\" fill=\"#666\">" << t << "</text>\n";
    }

    for (const auto& [tid, row] : task_rows) {
        double y = margin_t + row * row_h + 4;
        svg << "<text x=\"" << margin_l - 8 << "\" y=\"" << y + 16
            << "\" text-anchor=\"end\" font-size=\"12\">Task " << tid << "</text>\n";
    }

    for (const auto& seg : timeline) {
        if (seg.cpu_state != "RUN") continue;
        int tid = task_id_from_label(seg.job);
        if (tid == 0) continue;

        double x = margin_l + (double)seg.start / sim_ms * chart_w;
        double w = std::max(1.0, (double)(seg.end - seg.start) / sim_ms * chart_w);
        double y = margin_t + task_rows[tid] * row_h + 4;
        const char* color = TASK_COLORS[(tid - 1) % 10];

        svg << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w
            << "\" height=\"20\" fill=\"" << color << "\" rx=\"3\">\n";
        svg << "<title>" << html_escape(seg.job) << " [" << seg.start << "–" << seg.end << " ms]</title>\n";
        svg << "</rect>\n";

        if (w > 30) {
            svg << "<text x=\"" << x + w / 2 << "\" y=\"" << y + 14
                << "\" text-anchor=\"middle\" font-size=\"10\" fill=\"white\">"
                << html_escape(seg.job) << "</text>\n";
        }
    }

    svg << "</svg>\n";
    return svg.str();
}

void write_html_report(
    const std::string& path,
    const std::string& policy,
    const SimConfig& cfg,
    const SimResult& result,
    const std::vector<Task>& tasks,
    bool random_tasks,
    unsigned seed
) {
    int finished = 0, missed = 0, unfinished = 0;
    double total_response = 0;
    for (const auto& j : result.jobs) {
        if (j.finish_time == -1) unfinished++;
        else {
            finished++;
            total_response += j.finish_time - j.release_time;
            if (j.missed_deadline) missed++;
        }
    }
    double avg_response = finished ? total_response / finished : 0;

    std::ofstream out(path);
    out << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    out << "<title>Scheduler Report — " << html_escape(policy) << "</title>\n";
    out << "<style>\n";
    out << "body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#f5f6f8;color:#1a1a2e;}\n";
    out << ".wrap{max-width:1100px;margin:0 auto;padding:24px;}\n";
    out << "h1{margin:0 0 4px;font-size:1.75rem;}\n";
    out << ".sub{color:#555;margin-bottom:24px;}\n";
    out << ".cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;margin-bottom:28px;}\n";
    out << ".card{background:#fff;border-radius:10px;padding:16px;box-shadow:0 1px 4px rgba(0,0,0,.08);}\n";
    out << ".card .val{font-size:1.6rem;font-weight:700;}\n";
    out << ".card .lbl{font-size:.85rem;color:#666;margin-top:4px;}\n";
    out << ".card.miss .val{color:#c0392b;}\n";
    out << "section{background:#fff;border-radius:10px;padding:20px;margin-bottom:20px;box-shadow:0 1px 4px rgba(0,0,0,.08);overflow-x:auto;}\n";
    out << "h2{margin-top:0;font-size:1.15rem;}\n";
    out << "table{width:100%;border-collapse:collapse;font-size:.9rem;}\n";
    out << "th,td{padding:8px 10px;text-align:left;border-bottom:1px solid #eee;}\n";
    out << "th{background:#f8f9fa;font-weight:600;}\n";
    out << "tr.missed{background:#fdecea;}\n";
    out << ".badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:.75rem;font-weight:600;}\n";
    out << ".badge.ok{background:#d4edda;color:#155724;}\n";
    out << ".badge.fail{background:#f8d7da;color:#721c24;}\n";
    out << ".gantt{display:block;max-width:100%;height:auto;}\n";
    out << "</style>\n</head>\n<body>\n<div class=\"wrap\">\n";

    out << "<h1>Real-Time Scheduler Report</h1>\n";
    out << "<p class=\"sub\">Policy: <strong>" << html_escape(policy) << "</strong> &nbsp;|&nbsp; "
        << "Simulation: <strong>" << cfg.sim_ms << " ms</strong>";
    if (random_tasks) {
        out << " &nbsp;|&nbsp; Tasks: <strong>random</strong> (seed " << seed << ")";
    }
    out << "</p>\n";

    if (!tasks.empty()) {
        out << "<section><h2>Task Set</h2>\n<table>\n";
        out << "<tr><th>Task</th><th>Period</th><th>WCET</th><th>Deadline</th><th>Phase</th></tr>\n";
        for (const auto& t : tasks) {
            out << "<tr><td>T" << t.id << "</td><td>" << t.period_ms << " ms</td><td>"
                << t.wcet_ms << " ms</td><td>" << t.deadline_ms << " ms</td><td>"
                << t.phase_ms << " ms</td></tr>\n";
        }
        out << "</table></section>\n";
    }

    out << "<div class=\"cards\">\n";
    out << "<div class=\"card\"><div class=\"val\">" << result.jobs.size() << "</div><div class=\"lbl\">Total jobs</div></div>\n";
    out << "<div class=\"card\"><div class=\"val\">" << finished << "</div><div class=\"lbl\">Finished</div></div>\n";
    out << "<div class=\"card\"><div class=\"val\">" << unfinished << "</div><div class=\"lbl\">Unfinished</div></div>\n";
    out << "<div class=\"card" << (missed > 0 ? " miss" : "") << "\"><div class=\"val\">" << missed
        << "</div><div class=\"lbl\">Deadline misses</div></div>\n";
    out << "<div class=\"card\"><div class=\"val\">" << static_cast<int>(avg_response + 0.5)
        << " ms</div><div class=\"lbl\">Avg response</div></div>\n";
    out << "</div>\n";

    if (missed > 0 || unfinished > 0) {
        out << "<section class=\"violations\"><h2>Deadline Violations</h2>\n";
        if (missed > 0) {
            out << "<h3>Missed deadlines</h3>\n<table>\n";
            out << "<tr><th>Job</th><th>Release</th><th>Deadline</th><th>Finish</th><th>Late by</th></tr>\n";
            for (const auto& j : result.jobs) {
                if (j.finish_time == -1 || !j.missed_deadline) continue;
                int late = j.finish_time - j.abs_deadline;
                out << "<tr class=\"missed\"><td>" << j.label() << "</td><td>" << j.release_time
                    << " ms</td><td>" << j.abs_deadline << " ms</td><td>" << j.finish_time
                    << " ms</td><td>+" << late << " ms</td></tr>\n";
            }
            out << "</table>\n";
        }
        if (unfinished > 0) {
            out << "<h3>Unfinished at end of simulation</h3>\n<table>\n";
            out << "<tr><th>Job</th><th>Release</th><th>Deadline</th><th>Work remaining</th></tr>\n";
            for (const auto& j : result.jobs) {
                if (j.finish_time != -1) continue;
                out << "<tr class=\"missed\"><td>" << j.label() << "</td><td>" << j.release_time
                    << " ms</td><td>" << j.abs_deadline << " ms</td><td>"
                    << j.remaining_time << " ms</td></tr>\n";
            }
            out << "</table>\n";
        }
        out << "</section>\n";
    } else {
        out << "<section style=\"background:#d4edda;border:1px solid #c3e6cb;\">"
            << "<h2 style=\"margin:0;color:#155724;\">All jobs met their deadlines</h2></section>\n";
    }

    out << "<section><h2>Gantt Chart</h2>\n";
    out << svg_gantt(result.timeline, cfg.sim_ms);
    out << "</section>\n";

    out << "<section><h2>Job Details</h2>\n<table>\n";
    out << "<tr><th>Job</th><th>Release</th><th>Start</th><th>Finish</th>"
        << "<th>Deadline</th><th>Response</th><th>Preemptions</th><th>Status</th></tr>\n";

    for (const auto& j : result.jobs) {
        bool done = j.finish_time != -1;
        int response = done ? j.finish_time - j.release_time : -1;
        out << "<tr" << (j.missed_deadline ? " class=\"missed\"" : "") << ">";
        out << "<td>T" << j.task_id << "J" << j.job_id << "</td>";
        out << "<td>" << j.release_time << "</td>";
        out << "<td>" << (j.start_time == -1 ? "—" : std::to_string(j.start_time)) << "</td>";
        out << "<td>" << (done ? std::to_string(j.finish_time) : "—") << "</td>";
        out << "<td>" << j.abs_deadline << "</td>";
        out << "<td>" << (done ? std::to_string(response) : "—") << "</td>";
        out << "<td>" << j.preemptions << "</td>";
        out << "<td>";
        if (!done) out << "<span class=\"badge fail\">UNFINISHED</span>";
        else if (j.missed_deadline) out << "<span class=\"badge fail\">MISSED</span>";
        else out << "<span class=\"badge ok\">MET</span>";
        out << "</td></tr>\n";
    }

    out << "</table></section>\n";
    out << "</div>\n</body>\n</html>\n";
}
