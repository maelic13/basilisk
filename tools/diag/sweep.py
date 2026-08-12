#!/usr/bin/env python3
"""Sweep engine options against the fixed suite, reporting tree shape only.

`run_suite.py` does the full internal breakdown, which is the slow part and is
not what an option sweep needs. This runs only the fixed-node pass and reports
mean completed depth - the metric BAS-O03/D02 identified as the one that
actually tracks the gap to the oracle.

Counters explain; games accept. Nothing here is a verdict.

Usage:
  python tools/diag/sweep.py --engine build/tune/basilisk.exe \
      --config "baseline:" --config "cap2:CheckExtPathCap=2"
"""

import argparse
import pathlib
import statistics
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_suite import REPO, last_depth_nodes, load_suite, uci_run  # noqa: E402


def measure(engine, fens, nodes, options):
    depths, node_counts = [], []
    for fen in fens:
        out = uci_run(engine, [*options, "ucinewgame", f"position fen {fen}",
                               f"go nodes {nodes}"])
        d, n = last_depth_nodes(out)
        if d:
            depths.append(d)
            node_counts.append(n)
    return depths, node_counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", required=True)
    ap.add_argument("--suite", default=str(REPO / "tools" / "diag" / "suite_v1.epd"))
    ap.add_argument("--nodes", type=int, default=300000)
    ap.add_argument("--config", action="append", required=True,
                    metavar="label:Name=Value,Name=Value")
    args = ap.parse_args()

    fens = load_suite(pathlib.Path(args.suite))
    engine = pathlib.Path(args.engine)

    print(f"{len(fens)} positions at {args.nodes:,} nodes")
    print("Deltas are PAIRED per position against the first config. An unpaired")
    print("mean over this suite is dominated by a few forced-mate positions that")
    print("run to depth 100; pairing removes position-to-position variance.\n")
    print(f"{'config':28} {'mean depth':>11} {'d_paired':>9} {'better':>7} "
          f"{'worse':>6} {'same':>5}")
    print("-" * 74)

    baseline_depths = None
    for cfg in args.config:
        label, _, opt_str = cfg.partition(":")
        opts = []
        for kv in filter(None, opt_str.split(",")):
            name, _, value = kv.partition("=")
            opts.append(f"setoption name {name} value {value}")

        # Keep per-position alignment: a position that failed must not shift
        # the pairing, so record None rather than dropping it.
        per_pos = []
        for fen in fens:
            out = uci_run(engine, [*opts, "ucinewgame", f"position fen {fen}",
                                   f"go nodes {args.nodes}"])
            d, _ = last_depth_nodes(out)
            per_pos.append(d if d else None)

        got = [d for d in per_pos if d is not None]
        if not got:
            print(f"{label:28}  NO DATA - engine reported depth 0 everywhere")
            continue

        if baseline_depths is None:
            baseline_depths = per_pos
            print(f"{label:28} {statistics.mean(got):11.2f} {'-':>9} "
                  f"{'-':>7} {'-':>6} {'-':>5}")
            continue

        pairs = [(a, b) for a, b in zip(per_pos, baseline_depths)
                 if a is not None and b is not None]
        diffs = [a - b for a, b in pairs]
        better = sum(1 for d in diffs if d > 0)
        worse = sum(1 for d in diffs if d < 0)
        same = sum(1 for d in diffs if d == 0)
        print(f"{label:28} {statistics.mean(got):11.2f} "
              f"{statistics.mean(diffs):+9.3f} {better:7} {worse:6} {same:5}")


if __name__ == "__main__":
    main()
