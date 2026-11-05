#!/usr/bin/env python3
"""Tests of tools/diag/pmc_read.py on synthetic traces."""
from __future__ import annotations

import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import pmc_read  # noqa: E402

# Two CPUs. CPU 2: engine thread 7 runs from t=10 to t=20 and again from t=40
# to t=50; CPU 3: its start-up thread 8 runs from t=12 to t=15. Field 16 of a
# CSwitch row is the CPU, field 9 the thread switched out, field 8 its process.
HEADER = """\
                    Pmc,  TimeStamp,   ThreadID, InstructionRetired, TotalCycles, BranchMispredictions
                CSwitch,  TimeStamp, New Process Name ( PID),    New TID, NPri, NQnt, TmSinceLast, WaitTime, Old Process Name ( PID),    Old TID, OPri, OQnt,        OldState,      Wait Reason, Swapable, InSwitchTime, CPU
"""


def cswitch(ts, new_proc, new_tid, old_proc, old_tid, cpu):
    return (f"CSwitch, {ts}, {new_proc}, {new_tid}, 8, -1, 0, 0, {old_proc}, {old_tid}, 8, -1, "
            f"Waiting, Executive, NonSwap, 1, {cpu}, 0, 0\n")


def pmc(ts, tid, instr, cycles, miss):
    return f"Pmc, {ts}, {tid}, {instr}, {cycles}, {miss}\n"


ENGINE, IDLE = "basilisk.exe (500)", "Idle (   0)"
TRACE = HEADER + "".join([
    pmc(10, 7, 1000, 2000, 10), cswitch(10, ENGINE, 7, IDLE, 0, 2),
    pmc(12, 8, 50, 60, 1), cswitch(12, ENGINE, 8, IDLE, 0, 3),
    pmc(15, 0, 80, 100, 2), cswitch(15, IDLE, 0, ENGINE, 8, 3),        # thread 8: 30, 40, 1
    pmc(20, 0, 9000, 6000, 90), cswitch(20, IDLE, 0, ENGINE, 7, 2),    # thread 7: 8000, 4000, 80
    pmc(40, 7, 9500, 6500, 95), cswitch(40, ENGINE, 7, IDLE, 0, 2),    # idle: not the engine
    pmc(50, 0, 11500, 7500, 105), cswitch(50, IDLE, 0, ENGINE, 7, 2),  # thread 7: 2000, 1000, 10
    pmc(51, 9, 1, 1, 1),                                               # no CSwitch follows
])


class ParseTest(unittest.TestCase):
    def test_thread_counts(self):
        per, stats = pmc_read.thread_counts(TRACE.splitlines(), 500)
        self.assertEqual(per[7], [10000, 5000, 90, 2])
        self.assertEqual(per[8], [30, 40, 1, 1])
        self.assertEqual(stats, {"pmc": 7, "paired": 6, "negative": 0})

    def test_other_process_ignored(self):
        per, _ = pmc_read.thread_counts(TRACE.splitlines(), 501)
        self.assertEqual(per, {})

    def test_negative_delta_counted(self):
        bad = TRACE.replace("pmc(50", "").replace("Pmc, 50, 0, 11500", "Pmc, 50, 0, 100")
        _, stats = pmc_read.thread_counts(bad.splitlines(), 500)
        self.assertEqual(stats["negative"], 1)

    def test_parse_run(self):
        with tempfile.TemporaryDirectory() as d:
            path = pathlib.Path(d) / "dumper.csv"
            path.write_text(TRACE, encoding="utf-8")
            run = pmc_read.parse_run(path, 500, "Nodes searched  : 1000\n")
        self.assertEqual(run["search_thread"], 7)
        self.assertEqual(run["cycles_per_node"], 5.0)
        self.assertEqual(run["instructions_per_node"], 10.0)
        self.assertEqual(run["startup"], {"instructions": 30, "cycles": 40, "branch_misses": 1})


def run(arm, build, cpn, nodes=1000):
    return {"arm": arm, "build": build, "nodes": nodes, "cycles_per_node": cpn,
            "instructions_per_node": 2 * cpn, "branch_misses_per_node": 0.01 * cpn,
            "search": {"cycles": 99}, "startup": {"cycles": 1}}


class AnalyseTest(unittest.TestCase):
    def write(self, d, runs, contrasts):
        (d / "config.json").write_text(json.dumps({"label": "t", "contrasts": contrasts}))
        (d / "runs.jsonl").write_text("".join(json.dumps(r) + "\n" for r in runs))

    def test_pools_and_null(self):
        runs = []
        for cycle in range(3):
            for b, level in enumerate((100.0, 101.0, 99.0, 100.5)):
                runs.append(run("base", f"b{b}", level + 0.1 * cycle))
                runs.append(run("cand", f"c{b}", 0.98 * level + 0.1 * cycle))
            runs.append(run("copy", "b0-copy", 100.0 + 0.1 * cycle))
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            self.write(d, runs, [{"name": "p25", "base": "base", "cand": "cand"},
                                 {"name": "null", "base": "base", "base_build": "b0",
                                  "cand": "copy"}])
            report = pmc_read.analyse(d)
        p25 = report["contrasts"][0]["cycles_per_node"]
        self.assertEqual(p25["samples"], "builds")
        self.assertAlmostEqual(p25["delta_percent"], -2.0, delta=0.05)
        self.assertLess(p25["interval_percent"][1], 0)
        null = report["contrasts"][1]["cycles_per_node"]
        self.assertEqual(null["samples"], "runs")
        self.assertAlmostEqual(null["delta_percent"], 0.0)
        self.assertAlmostEqual(report["arms"]["base"]["startup_cycle_share"], 0.01)

    def test_one_tree_required(self):
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            self.write(d, [run("base", "b0", 100.0), run("cand", "c0", 98.0, nodes=999)],
                       [{"name": "x", "base": "base", "cand": "cand"}])
            with self.assertRaises(ValueError):
                pmc_read.analyse(d)


if __name__ == "__main__":
    unittest.main()
