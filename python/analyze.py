#!/usr/bin/env python3
"""
Reads outputs/timeline.csv and outputs/jobs.csv and produces PNG charts plus
outputs/plots.html (gallery page).

Run from repo root:
  python python/analyze.py
  python python/analyze.py --open
"""

from __future__ import annotations

import argparse
import os
import re
import webbrowser

import matplotlib.pyplot as plt
import pandas as pd

JOB_RE = re.compile(r"^T(?P<task>\d+)J(?P<job>\d+)$")

TASK_COLORS = [
    "#4e79a7", "#f28e2b", "#e15759", "#76b7b2", "#59a14f",
    "#edc948", "#b07aa1", "#ff9da7", "#9c755f", "#bab0ac",
]


def repo_root() -> str:
    return os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


def parse_job_label(label: str):
    if not isinstance(label, str):
        return None, None
    m = JOB_RE.match(label.strip())
    if not m:
        return None, None
    return int(m.group("task")), int(m.group("job"))


def task_color(task_id: int) -> str:
    return TASK_COLORS[(task_id - 1) % len(TASK_COLORS)]


def read_inputs(root: str, policy: str | None):
    out_dir = os.path.join(root, "outputs")
    timeline_path = (
        os.path.join(out_dir, "timeline.csv")
        if not policy
        else os.path.join(out_dir, f"timeline_{policy}.csv")
    )
    jobs_path = (
        os.path.join(out_dir, "jobs.csv")
        if not policy
        else os.path.join(out_dir, f"jobs_{policy}.csv")
    )

    if not os.path.exists(timeline_path):
        raise FileNotFoundError(
            f"Missing {timeline_path}. Run the simulator first (e.g. .\\run.bat edf)."
        )
    if not os.path.exists(jobs_path):
        raise FileNotFoundError(
            f"Missing {jobs_path}. Run the simulator first (e.g. .\\run.bat edf)."
        )

    return out_dir, pd.read_csv(timeline_path), pd.read_csv(jobs_path)


def make_gantt(out_dir: str, timeline: pd.DataFrame) -> str | None:
    tl = timeline[timeline["cpu_state"].astype(str).str.upper() == "RUN"].copy()
    if tl.empty:
        print("No RUN segments found. Skipping gantt.")
        return None

    parsed = tl["job"].apply(parse_job_label)
    tl["task_id"] = [t for t, _ in parsed]
    tl = tl.dropna(subset=["task_id"]).copy()
    tl["task_id"] = tl["task_id"].astype(int)

    task_ids = sorted(tl["task_id"].unique().tolist())
    y_map = {tid: i for i, tid in enumerate(task_ids)}

    plt.figure(figsize=(12, max(3, 0.6 * len(task_ids) + 2)))
    ax = plt.gca()

    for _, row in tl.iterrows():
        tid = int(row["task_id"])
        y = y_map[tid]
        start = float(row["time_start"])
        end = float(row["time_end"])
        ax.barh(y=y, width=end - start, left=start, height=0.6, color=task_color(tid), edgecolor="white")

    ax.set_yticks(range(len(task_ids)))
    ax.set_yticklabels([f"Task {tid}" for tid in task_ids])
    ax.set_xlabel("Time (ms)")
    ax.set_title("CPU Schedule (Gantt)")
    ax.grid(True, axis="x", linestyle="--", linewidth=0.5)

    path = os.path.join(out_dir, "gantt.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")
    return path


def make_response_times(out_dir: str, jobs: pd.DataFrame) -> str | None:
    df = jobs[jobs["response"] >= 0].copy()
    if df.empty:
        print("No completed jobs found. Skipping response_times.")
        return None

    task_ids = sorted(df["task_id"].unique().tolist())
    plt.figure(figsize=(12, 6))
    ax = plt.gca()

    for tid in task_ids:
        d = df[df["task_id"] == tid].sort_values("release")
        ax.plot(
            d["release"], d["response"],
            marker="o", linestyle="-", color=task_color(tid), label=f"Task {tid}",
        )

    ax.set_xlabel("Release time (ms)")
    ax.set_ylabel("Response time (ms)")
    ax.set_title("Response Time vs Release")
    ax.grid(True, linestyle="--", linewidth=0.5)
    ax.legend()

    path = os.path.join(out_dir, "response_times.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")
    return path


def make_jitter(out_dir: str, jobs: pd.DataFrame) -> str | None:
    df = jobs[jobs["response"] >= 0].copy()
    if df.empty:
        print("No completed jobs found. Skipping jitter.")
        return None

    g = df.groupby("task_id")["response"]
    jitter = (g.max() - g.min()).reset_index()
    jitter.columns = ["task_id", "jitter_ms"]

    plt.figure(figsize=(10, 5))
    ax = plt.gca()
    colors = [task_color(int(tid)) for tid in jitter["task_id"]]
    ax.bar(jitter["task_id"].astype(str), jitter["jitter_ms"], color=colors)
    ax.set_xlabel("Task")
    ax.set_ylabel("Jitter (ms) = max(response) - min(response)")
    ax.set_title("Jitter per Task")
    ax.grid(True, axis="y", linestyle="--", linewidth=0.5)

    path = os.path.join(out_dir, "jitter.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")
    return path


def make_deadlines(out_dir: str, jobs: pd.DataFrame) -> str | None:
    df = jobs[jobs["finish"] >= 0].copy()
    if df.empty:
        print("No finished jobs found. Skipping deadlines plot.")
        return None

    df["slack"] = df["deadline"] - df["finish"]
    misses = df[df["missed"] == 1]
    meets = df[df["missed"] == 0]

    plt.figure(figsize=(12, 6))
    ax = plt.gca()
    ax.scatter(meets["release"], meets["slack"], marker="o", color="#59a14f", label="Met deadline")
    ax.scatter(misses["release"], misses["slack"], marker="x", color="#e15759", label="Missed deadline")
    ax.axhline(0, linewidth=1, color="#333")
    ax.set_xlabel("Release time (ms)")
    ax.set_ylabel("Slack (ms) = deadline - finish")
    ax.set_title("Deadline Slack over Time")
    ax.grid(True, linestyle="--", linewidth=0.5)
    ax.legend()

    path = os.path.join(out_dir, "deadlines.png")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()
    print(f"Wrote {path}")
    return path


def write_plots_html(out_dir: str, images: list[tuple[str, str, str]]):
    """images: list of (filename, title, description)"""
    rows = []
    for fname, title, desc in images:
        rows.append(f"""
<section>
  <h2>{title}</h2>
  <p>{desc}</p>
  <img src="{fname}" alt="{title}">
</section>""")

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Scheduler Plots</title>
  <style>
    body {{ font-family: system-ui, sans-serif; margin: 0; background: #f5f6f8; color: #1a1a2e; }}
    .wrap {{ max-width: 1100px; margin: 0 auto; padding: 24px; }}
    h1 {{ margin: 0 0 8px; }}
    .sub {{ color: #555; margin-bottom: 24px; }}
    section {{ background: #fff; border-radius: 10px; padding: 20px; margin-bottom: 20px;
              box-shadow: 0 1px 4px rgba(0,0,0,.08); }}
    img {{ max-width: 100%; height: auto; display: block; }}
    a {{ color: #4e79a7; }}
  </style>
</head>
<body>
<div class="wrap">
  <h1>Scheduler Visualizations</h1>
  <p class="sub">Generated from <code>outputs/timeline.csv</code> and <code>outputs/jobs.csv</code>.
     Also see <a href="report.html">report.html</a> for the C++ HTML report.</p>
  {''.join(rows)}
</div>
</body>
</html>
"""
    path = os.path.join(out_dir, "plots.html")
    with open(path, "w", encoding="utf-8") as f:
        f.write(html)
    print(f"Wrote {path}")
    return path


def main():
    ap = argparse.ArgumentParser(description="Generate scheduler PNG plots from CSV outputs.")
    ap.add_argument("--policy", type=str, default=None,
                    help="Use timeline_<policy>.csv and jobs_<policy>.csv if suffixed")
    ap.add_argument("--open", action="store_true", help="Open plots.html in the browser")
    args = ap.parse_args()

    root = repo_root()
    out_dir, timeline, jobs = read_inputs(root, args.policy)

    required_tl = {"time_start", "time_end", "cpu_state", "job"}
    required_jobs = {
        "task_id", "job_id", "release", "start", "finish",
        "deadline", "response", "missed", "preemptions",
    }
    if not required_tl.issubset(timeline.columns):
        raise ValueError(f"timeline.csv missing columns: {sorted(required_tl - set(timeline.columns))}")
    if not required_jobs.issubset(jobs.columns):
        raise ValueError(f"jobs.csv missing columns: {sorted(required_jobs - set(jobs.columns))}")

    gallery: list[tuple[str, str, str]] = []

    if make_gantt(out_dir, timeline):
        gallery.append(("gantt.png", "Gantt Chart", "Which task ran on the CPU over time."))
    if make_response_times(out_dir, jobs):
        gallery.append(("response_times.png", "Response Times", "How long each job took from release to finish."))
    if make_jitter(out_dir, jobs):
        gallery.append(("jitter.png", "Jitter", "Variation in response time per task (max − min)."))
    if make_deadlines(out_dir, jobs):
        gallery.append(("deadlines.png", "Deadline Slack", "Positive slack = met deadline; negative = miss."))

    if not gallery:
        print("No plots were generated.")
        return 1

    plots_html = write_plots_html(out_dir, gallery)
    print("Done.")

    if args.open:
        webbrowser.open(f"file:///{plots_html.replace(os.sep, '/')}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
