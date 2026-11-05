#!/usr/bin/env python3
"""Speed at an identical tree, read from hardware counters (PLAN B.16.4).

An elevated driver (tools/run_b164_pmc.ps1) records an ETW kernel trace with
xperf's PMC-on-CSWITCH while `bench 13` runs pinned to one core: at every
context switch the CPU's retired instructions, cycles and branch
mispredictions are logged. In `xperf -a dumper` output each `Pmc` row (the
CPU's running totals, and the incoming thread) comes just before its
`CSwitch` row with the same timestamp; the difference from the previous
`Pmc` on that CPU belongs to the thread switched out. The search thread is
the engine's thread with the most cycles; the others are start-up.

Modes:
  parse    one run's dumper output: the search thread's and start-up's
           counts, per node of the bench (`Nodes searched`)
  analyse  a read's runs.jsonl against its config.json: per arm, the median
           of each build over the cycles; each contrast as the difference of
           the arms' means in percent of the base with its Welch 95%
           interval over builds (over runs when both sides are one build: a
           null names with `base_build` the build its copy was made from)

Usage:
  python tools/diag/pmc_read.py parse DUMPER.csv --pid PID --bench BENCH.txt
  python tools/diag/pmc_read.py analyse DIR
"""
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re
import statistics
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from nps_read import welch_interval  # noqa: E402

COUNTERS = ("instructions", "cycles", "branch_misses")
METRICS = ("cycles_per_node", "instructions_per_node", "branch_misses_per_node")
NODES_LINE = re.compile(r"Nodes searched\s*:\s*(\d+)")
PROCESS = re.compile(r"\((\s*\d+)\)\s*$")


def thread_counts(lines, pid: int) -> tuple[dict[int, list[int]], dict]:
    """Per thread of `pid`: [instructions, cycles, branch misses, slices]."""
    last: dict[int, list[int]] = {}
    pending = None
    per: dict[int, list[int]] = collections.defaultdict(lambda: [0, 0, 0, 0])
    stats = {"pmc": 0, "paired": 0, "negative": 0}
    for raw in lines:
        line = raw.lstrip()
        if line.startswith("Pmc,"):
            fields = [f.strip() for f in line.split(",")]
            if fields[1] == "TimeStamp":
                continue
            stats["pmc"] += 1
            pending = (int(fields[1]), int(fields[2]), [int(x) for x in fields[3:6]])
        elif line.startswith("CSwitch,"):
            fields = [f.strip() for f in line.split(",")]
            if fields[1] == "TimeStamp" or pending is None:
                continue
            ts, new_tid = int(fields[1]), int(fields[3])
            if (ts, new_tid) != pending[:2]:
                pending = None
                continue
            stats["paired"] += 1
            cpu = int(fields[16])
            if cpu in last:
                delta = [a - b for a, b in zip(pending[2], last[cpu])]
                if min(delta) < 0:
                    stats["negative"] += 1
                else:
                    m = PROCESS.search(fields[8])
                    if m and int(m.group(1)) == pid:
                        slot = per[int(fields[9])]
                        for i in range(3):
                            slot[i] += delta[i]
                        slot[3] += 1
            last[cpu] = pending[2]
            pending = None
    return dict(per), stats


def parse_run(dumper: pathlib.Path, pid: int, bench_text: str) -> dict:
    m = NODES_LINE.search(bench_text)
    if not m:
        raise ValueError("no `Nodes searched` line in the bench output")
    nodes = int(m.group(1))
    with dumper.open(encoding="utf-8", errors="replace") as fh:
        per, stats = thread_counts(fh, pid)
    if not per:
        raise ValueError(f"no counted slice of process {pid}")
    if stats["negative"]:
        raise ValueError(f"{stats['negative']} negative counter deltas")
    search_tid = max(per, key=lambda t: per[t][1])
    search = per[search_tid]
    startup = [sum(per[t][i] for t in per if t != search_tid) for i in range(3)]
    return {"nodes": nodes, "search_thread": search_tid, "search_slices": search[3],
            "search": dict(zip(COUNTERS, search[:3])), "startup": dict(zip(COUNTERS, startup)),
            "cycles_per_node": search[1] / nodes, "instructions_per_node": search[0] / nodes,
            "branch_misses_per_node": search[2] / nodes, "trace": stats}


def analyse(directory: pathlib.Path) -> dict:
    config = json.loads((directory / "config.json").read_text(encoding="utf-8"))
    runs = [json.loads(l) for l in (directory / "runs.jsonl").read_text(encoding="utf-8").splitlines()]
    nodes = {r["nodes"] for r in runs}
    if len(nodes) != 1:
        raise ValueError(f"the arms do not search one tree: node counts {sorted(nodes)}")
    by_build: dict[str, dict[str, list[dict]]] = collections.defaultdict(lambda: collections.defaultdict(list))
    for r in runs:
        by_build[r["arm"]][r["build"]].append(r)
    report = {"label": config.get("label"), "nodes": nodes.pop(), "runs": len(runs), "arms": {},
              "contrasts": []}
    for arm, builds in by_build.items():
        report["arms"][arm] = {
            metric: {b: statistics.median(r[metric] for r in rs) for b, rs in builds.items()}
            for metric in METRICS}
        report["arms"][arm]["startup_cycle_share"] = statistics.median(
            r["startup"]["cycles"] / (r["startup"]["cycles"] + r["search"]["cycles"])
            for rs in builds.values() for r in rs)
    for c in config["contrasts"]:
        base, cand = by_build[c["base"]], by_build[c["cand"]]
        # A null names the one build its copy was made from.
        if c.get("base_build"):
            base = {c["base_build"]: base[c["base_build"]]}
        row = {"name": c["name"], "base": c["base"], "cand": c["cand"]}
        single = len(base) == 1 and len(cand) == 1
        for metric in METRICS:
            if single:
                xs = [r[metric] for rs in cand.values() for r in rs]
                ys = [r[metric] for rs in base.values() for r in rs]
            else:
                xs = list(report["arms"][c["cand"]][metric].values())
                ys = list(report["arms"][c["base"]][metric].values())
            delta, lo, hi, df = welch_interval(xs, ys)
            row[metric] = {"delta_percent": delta, "interval_percent": [lo, hi], "df": df,
                           "samples": "runs" if single else "builds"}
        report["contrasts"].append(row)
    return report


def render(report: dict) -> str:
    out = [f"{report['label']}: {report['runs']} runs, {report['nodes']:,} nodes each (one tree)"]
    for arm, a in report["arms"].items():
        cpn = a["cycles_per_node"]
        out.append(f"  {arm:16} cycles/node by build: "
                   + ", ".join(f"{v:.1f}" for v in cpn.values())
                   + f"   start-up {100 * a['startup_cycle_share']:.2f}% of cycles")
    for c in report["contrasts"]:
        out.append(f"  {c['name']}: {c['cand']} against {c['base']} ({c['cycles_per_node']['samples']})")
        for metric in METRICS:
            m = c[metric]
            lo, hi = m["interval_percent"]
            out.append(f"    {metric:24} {m['delta_percent']:+7.3f}%  [{lo:+.3f}%, {hi:+.3f}%]")
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="mode", required=True)
    p = sub.add_parser("parse")
    p.add_argument("dumper")
    p.add_argument("--pid", type=int, required=True)
    p.add_argument("--bench", required=True)
    a = sub.add_parser("analyse")
    a.add_argument("dir")
    args = ap.parse_args()
    if args.mode == "parse":
        bench = pathlib.Path(args.bench).read_text(encoding="utf-8", errors="replace")
        print(json.dumps(parse_run(pathlib.Path(args.dumper), args.pid, bench)))
        return 0
    report = analyse(pathlib.Path(args.dir))
    (pathlib.Path(args.dir) / "report.json").write_text(json.dumps(report, indent=1),
                                                         encoding="utf-8")
    print(render(report))
    return 0


if __name__ == "__main__":
    sys.exit(main())
