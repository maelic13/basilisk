#!/usr/bin/env python3
"""Measure reference-endgame family occurrence inside Basilisk search trees.

Runs every root in a fixed EPD suite independently at fixed depth, with one
engine thread per root, then sums exact tune-build `Diag` counters. External
workers only schedule independent roots, so changing --workers changes wall
time but not the deterministic node/count result.

The 20 family names match the pre-NNUE reference dispatcher used by Rarog's
corresponding instrument. KPK overlaps the KPsK aggregate and KBNK overlaps
KXK; family counts therefore must not be summed as a partition.

Usage:
  python tools/diag/endgame_search_occurrence.py --engine build/tune/basilisk.exe
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import pathlib
import sys
import time

from run_suite import load_suite, parse_diag, uci_run


REPO = pathlib.Path(__file__).resolve().parents[2]
FAMILIES = [
    "eg_krpkr", "eg_krpkb", "eg_kpsk", "eg_kpk", "eg_krkp",
    "eg_kbpsk", "eg_kpkp", "eg_kqkp", "eg_kbpkb", "eg_kbppkb",
    "eg_krkn", "eg_krkb", "eg_kbpkn", "eg_knnkp", "eg_knnk",
    "eg_kqkr", "eg_kqkrps", "eg_krppkrp", "eg_kxk", "eg_kbnk",
]


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def search_one(engine, fen, depth, hash_mb, timeout):
    output = uci_run(engine, [
        f"setoption name Hash value {hash_mb}",
        "setoption name Threads value 1",
        "setoption name Diag value true",
        "ucinewgame",
        f"position fen {fen}",
        f"go depth {depth}",
    ], timeout=timeout)
    counters = parse_diag(output)
    if "eg_classified" not in counters:
        raise RuntimeError(
            "no endgame counters; --engine must name a TUNE build containing "
            "the search-occurrence instrument")
    return counters


def add_counters(total, row):
    for name, value in row.items():
        total[name] = total.get(name, 0) + value


def pct(value, denominator):
    return 100.0 * value / denominator if denominator else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", required=True, type=pathlib.Path,
                    help="Basilisk TUNE executable")
    ap.add_argument("--suite", type=pathlib.Path,
                    default=REPO / "tools" / "diag" / "suite_v1.epd")
    ap.add_argument("--depth", type=int, default=13)
    ap.add_argument("--hash", type=int, default=16, dest="hash_mb")
    ap.add_argument("--workers", type=int, default=None,
                    help="independent 1-thread engines (default: logical CPUs - 2, capped at 30)")
    ap.add_argument("--timeout", type=int, default=600,
                    help="seconds allowed per root")
    ap.add_argument("--out", type=pathlib.Path, default=None)
    args = ap.parse_args()

    engine = args.engine.resolve()
    suite = args.suite.resolve()
    if not engine.is_file():
        ap.error(f"engine does not exist: {engine}")
    if not suite.is_file():
        ap.error(f"suite does not exist: {suite}")
    if args.depth < 1 or args.hash_mb < 1:
        ap.error("--depth and --hash must be positive")

    fens = load_suite(suite)
    if not fens:
        ap.error("suite is empty")
    default_workers = max(1, min(30, (os.cpu_count() or 1) - 2))
    workers = args.workers if args.workers is not None else default_workers
    if workers < 1:
        ap.error("--workers must be positive")
    workers = min(workers, len(fens))

    print(f"search-tree census: {len(fens)} roots, depth {args.depth}, "
          f"{workers} external workers x Threads=1")
    started = time.monotonic()
    total = {}
    completed = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as pool:
        futures = [pool.submit(search_one, engine, fen, args.depth,
                               args.hash_mb, args.timeout) for fen in fens]
        for future in concurrent.futures.as_completed(futures):
            add_counters(total, future.result())
            completed += 1
            if completed % 10 == 0 or completed == len(fens):
                print(f"  {completed}/{len(fens)}")

    nodes = total.get("interior_nodes", 0) + total.get("qs_nodes", 0)
    evals = total.get("eval_calls", 0)
    classified = total.get("eg_classified", 0)
    if nodes <= 0 or evals <= 0:
        sys.exit("invalid diagnostic output: zero nodes or evaluations")

    rows = [{
        "family": name[3:].upper(),
        "counter": name,
        "count": total.get(name, 0),
        "share_upto7_pct": pct(total.get(name, 0), classified),
        "share_eval_pct": pct(total.get(name, 0), evals),
        "share_node_pct": pct(total.get(name, 0), nodes),
    } for name in FAMILIES]
    rows.sort(key=lambda row: (-row["count"], row["family"]))

    print(f"\nnodes {nodes:,}; evaluations {evals:,}; <=7-men evaluations "
          f"{classified:,} ({pct(classified, evals):.3f}% eval, "
          f"{pct(classified, nodes):.3f}% node)\n")
    print(f"{'family':10} {'count':>12} {'<=7 share':>11} {'eval share':>11} {'node share':>11}")
    for row in rows:
        print(f"{row['family']:10} {row['count']:12,d} "
              f"{row['share_upto7_pct']:10.3f}% "
              f"{row['share_eval_pct']:10.4f}% "
              f"{row['share_node_pct']:10.4f}%")
    print("\nKPK is included in KPsK; KBNK is included in KXK. "
          "Occurrence screens candidates; it does not establish Elo.")

    report = {
        "schema": "basilisk-endgame-search-occurrence-v1",
        "engine": str(engine),
        "engine_sha256": sha256(engine),
        "suite": str(suite),
        "suite_sha256": sha256(suite),
        "positions": len(fens),
        "depth": args.depth,
        "hash_mb": args.hash_mb,
        "engine_threads": 1,
        "external_workers": workers,
        "elapsed_seconds": time.monotonic() - started,
        "nodes": nodes,
        "evaluations": evals,
        "upto7_evaluations": classified,
        "families": rows,
        "counters": total,
        "notes": [
            "KPK overlaps KPsK and KBNK overlaps KXK.",
            "Tree occurrence is a candidate-priority screen, not an Elo gate.",
        ],
    }
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
