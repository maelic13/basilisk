#!/usr/bin/env python3
"""The depth-bound read: does every search of a fixed depth finish?

Runs each position of an EPD at a fixed depth, in a fresh engine process, under a
time limit, and names the ones with no `bestmove` in time. A fixed-depth search has
only its own loops to end it, so a loop that cannot end shows here and nowhere a
node or clock limit is in force. An engine that dies, or says nothing, counts as
unfinished. Each search prints its node count, so a read shows its options were
live: an option that moves nothing moves no count.

Exit status: 0 when everything finished, 1 when anything did not, 2 on bad input.

Usage:
  python tools/diag/find_spin.py --engine build/x/basilisk.exe --depth 6 --limit 20
  python tools/diag/find_spin.py --engine ... --option B5AspBase=500 --option B5AspGrowth=32
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys
import threading
import time

REPO = pathlib.Path(__file__).resolve().parents[2]
NODES = re.compile(r"^info .*\bnodes (\d+)")


def read_positions(path: pathlib.Path) -> list[str]:
    """The FENs of an EPD: its first four fields, completed with the two counters."""
    fens = []
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split()
        if len(fields) >= 4:
            fens.append(" ".join(fields[:4]) + " 0 1")
    return fens


def last_nodes(lines: list[str]) -> int | None:
    """The node count of the last `info` line that has one."""
    for line in reversed(lines):
        match = NODES.search(line)
        if match:
            return int(match.group(1))
    return None


def search_finishes(command: list[str], fen: str, depth: int, limit: float, options: list[str], hash_mb: int) -> tuple[bool, list[str]]:
    """One position in a fresh process: whether `bestmove` came in time, and the last lines."""
    engine = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True)
    lines: list[str] = []
    answered = threading.Event()

    def reader() -> None:
        for line in engine.stdout:
            lines.append(line.rstrip())
            del lines[:-8]
            if line.startswith("bestmove"):
                answered.set()
                return

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    script = f"setoption name Hash value {hash_mb}\n"
    for option in options:
        name, value = option.split("=", 1)
        script += f"setoption name {name} value {value}\n"
    script += f"position fen {fen}\ngo depth {depth}\n"
    try:
        engine.stdin.write(script)
        engine.stdin.flush()
    except OSError:
        pass  # it has gone already: nothing will answer, and that is what is counted
    # An engine that dies cannot answer, so its reader's end ends the wait too.
    deadline = time.monotonic() + limit
    while not answered.is_set() and thread.is_alive() and time.monotonic() < deadline:
        answered.wait(0.05)
    finished = answered.is_set()
    engine.kill()
    engine.wait()
    thread.join(5)
    if engine.stdout:
        engine.stdout.close()
    if engine.stdin:
        try:
            engine.stdin.close()
        except OSError:
            pass
    return finished, list(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", required=True, type=pathlib.Path)
    parser.add_argument("--epd", default=REPO / "src/wac.epd", type=pathlib.Path)
    parser.add_argument("--depth", default=6, type=int)
    parser.add_argument("--limit", default=20.0, type=float, help="seconds per position")
    parser.add_argument("--hash", default=16, type=int)
    parser.add_argument("--option", action="append", default=[], metavar="NAME=VALUE",
                        help="a UCI option; only a Tune build advertises the search's coordinates")
    parser.add_argument("--out", type=pathlib.Path, help="a JSON record of the read")
    args = parser.parse_args()

    if not args.engine.exists():
        print(f"no engine at {args.engine}", file=sys.stderr)
        return 2
    if any("=" not in option for option in args.option):
        print("--option takes NAME=VALUE", file=sys.stderr)
        return 2
    positions = read_positions(args.epd)
    if not positions:
        print(f"no positions in {args.epd}", file=sys.stderr)
        return 2

    command = [str(args.engine.resolve())]
    unfinished = []
    node_counts: list[int | None] = []
    for index, fen in enumerate(positions):
        finished, lines = search_finishes(command, fen, args.depth, args.limit, args.option, args.hash)
        nodes = last_nodes(lines)
        node_counts.append(nodes)
        if finished:
            print(f"{index} nodes {nodes}", flush=True)
        else:
            unfinished.append(fen)
            print(f"UNFINISHED {index} {fen} nodes {nodes}", flush=True)
            for line in lines:
                print("   ", line[:170], flush=True)
    print(f"depth {args.depth}, {args.limit:g} s each, {args.option or 'defaults'}: "
          f"{len(unfinished)} of {len(positions)} unfinished; {sum(n or 0 for n in node_counts):,} nodes", flush=True)
    if args.out:
        args.out.write_text(json.dumps({"engine": str(args.engine), "depth": args.depth, "limit": args.limit,
                                        "options": args.option, "positions": len(positions),
                                        "unfinished": unfinished, "nodes": node_counts}, indent=1), encoding="utf-8")
    return 1 if unfinished else 0


if __name__ == "__main__":
    sys.exit(main())
