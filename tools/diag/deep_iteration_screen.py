#!/usr/bin/env python3
"""Deep-iteration cost screen: an arm against a baseline at depths 16-19.

A search mechanism that bounds its tree at bench depth can still explode in
the deep iterations a game reaches (Rarog's IIR, a ply bound that did not
bound depth). This screen runs each position on both engines with one
`go depth MAX` and reads the cumulative node count of every completed
iteration. A per-depth watchdog stops a search whose next iteration has not
completed within the limit, so an explosion reads as an unfinished depth
rather than a hung run.

Per position it reports, over the depths in range that both engines finish,
the geometric mean of the arm's cumulative nodes over the baseline's, and
whether the arm finishes every depth in range the baseline finishes. The
registered verdict (PLAN B.3, packet §10) is a geometric mean of at most 3
and no depth the baseline finishes left unfinished by the arm.

Usage:
  python tools/diag/deep_iteration_screen.py --base off.exe --arm on.exe \
      --positions positions.epd --out screen.json
"""

from __future__ import annotations

import argparse
import json
import math
import pathlib
import queue
import re
import subprocess
import sys
import threading
import time

DEPTH_LINE = re.compile(r"^info depth (\d+)\b.*?\bnodes (\d+)\b")


def completed_iterations(lines: list[str]) -> dict[int, int]:
    """Cumulative nodes of each completed iteration; bound lines are not one."""
    out: dict[int, int] = {}
    for line in lines:
        text = f" {line.strip()} "
        if " lowerbound " in text or " upperbound " in text:
            continue
        m = DEPTH_LINE.match(line.strip())
        if m:
            out[int(m.group(1))] = int(m.group(2))
    return out


def run_position(engine: pathlib.Path, fen: str, max_depth: int, watchdog: float,
                 hash_mb: int) -> dict:
    """One search to `max_depth`; stopped when an iteration overruns the watchdog."""
    proc = subprocess.Popen([str(engine)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, text=True, bufsize=1)
    lines: queue.Queue[str | None] = queue.Queue()

    def reader():
        try:
            for line in proc.stdout:
                lines.put(line.rstrip("\r\n"))
        finally:
            lines.put(None)

    threading.Thread(target=reader, daemon=True).start()
    seen: list[str] = []
    stopped = False
    try:
        proc.stdin.write(f"setoption name Hash value {hash_mb}\nucinewgame\n"
                         f"position fen {fen}\ngo depth {max_depth}\n")
        proc.stdin.flush()
        last_progress = time.monotonic()
        while True:
            try:
                line = lines.get(timeout=0.25)
            except queue.Empty:
                line = ""
            if line is None:
                break
            if line:
                seen.append(line)
                if DEPTH_LINE.match(line) and " lowerbound " not in f" {line} " \
                        and " upperbound " not in f" {line} ":
                    last_progress = time.monotonic()
                if line.startswith("bestmove"):
                    break
            if not stopped and time.monotonic() - last_progress > watchdog:
                proc.stdin.write("stop\n")
                proc.stdin.flush()
                stopped = True
        try:
            proc.stdin.write("quit\n")
            proc.stdin.flush()
            proc.wait(timeout=10)
        except (OSError, ValueError, subprocess.TimeoutExpired):
            pass
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait(timeout=10)
    iterations = completed_iterations(seen)
    # A stop can print the interrupted iteration's line; only iterations the
    # watchdog did not cut count as finished.
    if stopped and iterations:
        iterations.pop(max(iterations), None)
    return {"iterations": iterations, "watchdog_stopped": stopped}


def compare(base: dict[int, int], arm: dict[int, int], lo: int, hi: int) -> dict:
    """The geometric mean of arm/base over the depths in range both finish."""
    both = [d for d in range(lo, hi + 1) if d in base and d in arm and base[d] > 0]
    ratios = {d: arm[d] / base[d] for d in both}
    geomean = (math.exp(sum(math.log(r) for r in ratios.values()) / len(ratios))
               if ratios else None)
    base_done = [d for d in range(lo, hi + 1) if d in base]
    missing = [d for d in base_done if d not in arm]
    return {"depths_compared": both, "ratios": ratios, "geomean": geomean,
            "arm_missing_depths": missing}


def verdict(row: dict, limit: float) -> bool:
    return (row["geomean"] is not None and row["geomean"] <= limit
            and not row["arm_missing_depths"])


def load_positions(path: pathlib.Path) -> list[str]:
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            out.append(line)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--base", required=True, help="the baseline engine (the OFF arm)")
    ap.add_argument("--arm", required=True, help="the engine under test (the ON arm)")
    ap.add_argument("--positions", required=True, help="one FEN per line")
    ap.add_argument("--depth-min", type=int, default=16)
    ap.add_argument("--depth-max", type=int, default=19)
    ap.add_argument("--watchdog", type=float, default=60.0,
                    help="seconds an iteration may take before the search is stopped")
    ap.add_argument("--hash", type=int, default=64)
    ap.add_argument("--limit", type=float, default=3.0,
                    help="the registered geometric-mean ceiling")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    fens = load_positions(pathlib.Path(args.positions))
    if not fens:
        sys.exit("no positions")
    rows = []
    for i, fen in enumerate(fens, 1):
        base = run_position(pathlib.Path(args.base), fen, args.depth_max, args.watchdog, args.hash)
        arm = run_position(pathlib.Path(args.arm), fen, args.depth_max, args.watchdog, args.hash)
        row = {"position": i, "fen": fen, "base": base, "arm": arm,
               **compare(base["iterations"], arm["iterations"], args.depth_min, args.depth_max)}
        row["pass"] = verdict(row, args.limit)
        rows.append(row)
        gm = "n/a" if row["geomean"] is None else f"{row['geomean']:.3f}"
        print(f"{i}: geomean {gm} over {row['depths_compared']} "
              f"missing {row['arm_missing_depths']} -> {'pass' if row['pass'] else 'FAIL'}")
    report = {"base": args.base, "arm": args.arm, "depth_min": args.depth_min,
              "depth_max": args.depth_max, "watchdog_s": args.watchdog,
              "hash_mb": args.hash, "limit": args.limit, "rows": rows,
              "pass": all(r["pass"] for r in rows)}
    pathlib.Path(args.out).write_text(json.dumps(report, indent=2), encoding="utf-8")
    print("screen", "PASS" if report["pass"] else "FAIL")
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
