#!/usr/bin/env python3
"""A fake UCI engine with planted delays: the planted case for wallclock.py.

It searches nothing. Its options set what an honest instrument must report:

  PlantNodesMs    `go nodes`: answers after this many ms and reports `time 0`,
                  so the true overhead is exactly this value
  PlantOvershootMs  `go movetime M`: answers after M plus this many ms
  PlantPonderMs   `ponderhit`: answers this many ms after the ponderhit

Waits are spins on the QPC clock, not sleeps, so they are exact to well
under 0.1 ms. Output is flushed on every line.
"""
from __future__ import annotations

import sys
import threading
import time

planted = {"PlantNodesMs": 0.0, "PlantOvershootMs": 0.0, "PlantPonderMs": 0.0}
lock = threading.Lock()


def say(line: str) -> None:
    with lock:
        sys.stdout.write(line + "\n")
        sys.stdout.flush()


def spin(ms: float) -> None:
    end = time.perf_counter() + ms / 1000.0
    while time.perf_counter() < end:
        pass


def main() -> None:
    for raw in sys.stdin:
        tokens = raw.split()
        if not tokens:
            continue
        cmd = tokens[0]
        if cmd == "uci":
            say("id name WallclockFake")
            for name in planted:
                say(f"option name {name} type string default 0")
            say("uciok")
        elif cmd == "isready":
            say("readyok")
        elif cmd == "setoption" and len(tokens) >= 5 and tokens[2] in planted:
            planted[tokens[2]] = float(tokens[4])
        elif cmd == "go" and "ponder" in tokens:
            pass  # waits for ponderhit
        elif cmd == "go" and "nodes" in tokens:
            spin(planted["PlantNodesMs"])
            say("info depth 1 nodes 1000 time 0 pv e2e4")
            say("bestmove e2e4")
        elif cmd == "go" and "movetime" in tokens:
            movetime = float(tokens[tokens.index("movetime") + 1])
            spin(movetime + planted["PlantOvershootMs"])
            say(f"info depth 1 nodes 1 time {int(movetime)} pv e2e4")
            say("bestmove e2e4")
        elif cmd == "ponderhit":
            spin(planted["PlantPonderMs"])
            say("bestmove e2e4")
        elif cmd == "quit":
            return


if __name__ == "__main__":
    main()
