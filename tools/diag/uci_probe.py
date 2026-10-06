#!/usr/bin/env python3
"""Black-box UCI probes for protocol and clock defects (PLAN A.8).

Each mode drives a fresh engine process per search through stdin/stdout and
prints what it measured; nothing is inferred from the engine's source.

  info    one search per position at `go depth N`; prints every info and
          bestmove line (field order, nps, seldepth, terminal roots)
  smp     Threads N, `go movetime` over a FEN file; counts searches whose
          bestmove is not the first move of the last `info ... pv` line
  clock   `go wtime T winc I` per position; prints the wall time of the move,
          the time of the last info line and the bestmove (clock sinks)

Usage:
  python tools/diag/uci_probe.py info  ENGINE [--depth 5] FEN...
  python tools/diag/uci_probe.py smp   ENGINE FENFILE [--threads 8] [--movetime 40,150,400]
  python tools/diag/uci_probe.py clock ENGINE [--wtime 60000 --winc 600] FEN...
Options set with --option "Name=Value" apply to every process (repeatable).
Exit status: 0, or 1 when `smp` finds a mismatch.
"""
from __future__ import annotations

import argparse
import queue
import subprocess
import sys
import threading
import time


def session(engine: str, options: list[str], commands: list[str],
            timeout: float) -> tuple[list[str], str | None, float]:
    """Run one search; return (output lines, bestmove line, seconds from go)."""
    proc = subprocess.Popen([engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, bufsize=1)
    lines: queue.Queue[str] = queue.Queue()
    threading.Thread(target=lambda: [lines.put(l.rstrip("\n")) for l in proc.stdout],
                     daemon=True).start()
    out: list[str] = []

    def send(command: str) -> None:
        proc.stdin.write(command + "\n")
        proc.stdin.flush()

    def until(prefix: str, seconds: float) -> str | None:
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            try:
                line = lines.get(timeout=0.05)
            except queue.Empty:
                if proc.poll() is not None and lines.empty():
                    return None
                continue
            out.append(line)
            if line.startswith(prefix):
                return line
        return None

    try:
        send("uci")
        until("uciok", 10)
        for option in options:
            name, value = option.split("=", 1)
            send(f"setoption name {name} value {value}")
        send("isready")
        until("readyok", 30)
        start = time.monotonic()
        for command in commands:
            send(command)
        best = until("bestmove", timeout)
        elapsed = time.monotonic() - start
        if proc.poll() is None:
            send("quit")
    finally:
        try:
            proc.wait(5)
        except subprocess.TimeoutExpired:
            proc.kill()
    return out, best, elapsed


def last_pv_line(out: list[str]) -> str | None:
    pv_lines = [l for l in out if l.startswith("info") and " pv " in l]
    return pv_lines[-1] if pv_lines else None


def position(fen: str) -> str:
    return "position startpos" if fen == "startpos" else f"position fen {fen}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=("info", "smp", "clock"))
    parser.add_argument("engine")
    parser.add_argument("args", nargs="*", help="FENs, or the FEN file for smp")
    parser.add_argument("--option", action="append", default=[])
    parser.add_argument("--depth", type=int, default=5)
    parser.add_argument("--threads", type=int, default=8)
    parser.add_argument("--movetime", default="40,150,400")
    parser.add_argument("--wtime", type=int, default=60000)
    parser.add_argument("--winc", type=int, default=600)
    a = parser.parse_intermixed_args()

    if a.mode == "info":
        for fen in a.args:
            out, best, _ = session(a.engine, a.option, [position(fen), f"go depth {a.depth}"], 120)
            print(f"== {fen}")
            print("\n".join(l for l in out if l.startswith(("info", "bestmove"))))
            if best is None:
                print("NO BESTMOVE (process ended or timed out)")
        return 0

    if a.mode == "smp":
        with open(a.args[0], encoding="utf-8") as handle:
            fens = [l.strip() for l in handle if l.strip()]
        options = a.option + [f"Threads={a.threads}"]
        mismatches = searches = 0
        for fen in fens:
            for movetime in (int(m) for m in a.movetime.split(",")):
                out, best, _ = session(a.engine, options,
                                       [position(fen), f"go movetime {movetime}"], 60)
                searches += 1
                line = last_pv_line(out)
                move = best.split()[1] if best else None
                if line is None or move is None or line.split(" pv ")[1].split()[0] != move:
                    mismatches += 1
                    print(f"MISMATCH movetime {movetime} | {fen} | last {line} | {best}")
        print(f"{mismatches}/{searches} searches whose bestmove is not the last line's "
              f"first PV move at Threads {a.threads}")
        return 1 if mismatches else 0

    for fen in a.args:
        go = f"go wtime {a.wtime} btime {a.wtime} winc {a.winc} binc {a.winc}"
        out, best, elapsed = session(a.engine, a.option, [position(fen), go], a.wtime / 1000 + 30)
        depth_lines = [l for l in out if l.startswith("info depth")]
        last = depth_lines[-1] if depth_lines else "none"
        print(f"{elapsed * 1000:.0f} ms | {best} | {fen} | last: {last}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
