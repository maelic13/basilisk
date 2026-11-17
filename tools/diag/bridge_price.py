#!/usr/bin/env python3
"""Price the evaluation bridge inside the search oracle and turn it into
time odds (PLAN B.11.1, RAR-O08's conditions).

Ported from Rarog's `tools/oracle-hybrids/bridge_price.py` at `89b4a92`
(2026-11-13). The method is Rarog's, unchanged: the same fixed-depth bench is
timed at bridge repeat counts 1 and 3, and half the difference in time per
node is one bridge's price. The oracle must be built with
`tools/oracle/bridge-repeat.patch` (`build_oracle.ps1 -ExtraPatch`), which
builds the board BASILISK_HCE_BRIDGE_REPEAT times per evaluation and changes
nothing else, so every run must report the same node count.

The odds hand that share back to the oracle: odds = t1 / (t1 - bridge), and
the clock is the base clock times the odds, rounded to the millisecond as
Rarog's 3.279+0.033 was. They equalise the bridge only; the oracle keeps its
own speed otherwise.

A timed read: run it on an idle host only (PLAN rule 12, agents/measurement.md).

Usage:
  python tools/diag/bridge_price.py --engine <oracle-price.exe> --out <json>
      [--cycles 3] [--command "bench 16 1 16 default depth"]
      [--base-ms 3000] [--increment-ms 30]
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import time

REPEAT_VARIABLE = "BASILISK_HCE_BRIDGE_REPEAT"
PATCH_NAME = "bridge-repeat.patch"
# Rarog's command for its classical oracle, which is this oracle's search.
DEFAULT_COMMAND = "bench 16 1 16 default depth"


class Refused(Exception):
    pass


def parse_bench(text: str) -> tuple[int, float]:
    """Nodes and milliseconds from Stockfish's bench summary (the last one)."""
    nodes = re.findall(r"Nodes searched\s*:\s*(\d+)", text)
    ms = re.findall(r"Total time \(ms\)\s*:\s*(\d+)", text)
    if not nodes or not ms:
        raise Refused("no bench summary in the output:\n" + text[-2000:])
    return int(nodes[-1]), float(ms[-1])


def summarise(rows: list[dict], base_ms: int, increment_ms: int) -> dict:
    """Rarog's summary: median time at each repeat, the bridge's share, the odds."""
    if not rows:
        raise Refused("no runs")
    failed = [r for r in rows if r["exit"] != 0]
    if failed:
        raise Refused(f"{len(failed)} run(s) exited non-zero")
    if len({r["nodes"] for r in rows}) != 1:
        raise Refused("node counts differ: the tree changed, so the read is void")
    by_repeat = {k: sorted(r["ms"] for r in rows if r["repeat"] == k) for k in (1, 3)}
    if any(not v for v in by_repeat.values()):
        raise Refused("each repeat count needs at least one run")
    median = {k: v[len(v) // 2] for k, v in by_repeat.items()}
    nodes = rows[0]["nodes"]
    per_node = {k: median[k] * 1e6 / nodes for k in median}
    bridge_ns = (per_node[3] - per_node[1]) / 2
    if bridge_ns <= 0:
        raise Refused("repeat 3 is not slower than repeat 1: the repeat wire is dead or the host was disturbed")
    odds = per_node[1] / (per_node[1] - bridge_ns)
    return {
        "nodes": nodes,
        "median_ms": {str(k): v for k, v in median.items()},
        "ns_per_node": {str(k): v for k, v in per_node.items()},
        "bridge_ns_per_node": bridge_ns,
        "bridge_share_at_1": bridge_ns / per_node[1],
        "time_odds": odds,
        "oracle_base_ms": round(base_ms * odds),
        "oracle_increment_ms": round(increment_ms * odds),
    }


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def check_manifest(engine: pathlib.Path) -> str:
    """The oracle's manifest must name the repeat patch, or the wire is absent."""
    manifest = engine.with_suffix(".manifest.txt")
    if not manifest.exists():
        raise Refused(f"no manifest beside {engine}; build it with tools/oracle/build_oracle.ps1")
    text = manifest.read_text(encoding="utf-8-sig", errors="replace")
    patches = re.search(r"^extra_patches:\s*(.*)$", text, re.M)
    if not patches or PATCH_NAME not in patches.group(1):
        raise Refused(f"{manifest.name} does not list {PATCH_NAME}: this oracle has no repeat wire")
    recorded = re.search(r"^binary_sha256:\s*([0-9A-Fa-f]{64})", text, re.M)
    actual = sha256(engine)
    if not recorded or recorded.group(1).upper() != actual:
        raise Refused(f"{engine.name} is {actual}, not the manifest's binary")
    return actual


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", required=True, type=pathlib.Path)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    parser.add_argument("--cycles", type=int, default=3)
    parser.add_argument("--command", default=DEFAULT_COMMAND)
    parser.add_argument("--base-ms", type=int, default=3000)
    parser.add_argument("--increment-ms", type=int, default=30)
    args = parser.parse_args()
    try:
        if args.cycles < 1:
            raise Refused("--cycles must be positive")
        engine = args.engine.resolve()
        engine_sha = check_manifest(engine)
        rows = []
        for cycle in range(args.cycles):
            for repeat in (1, 3):
                env = dict(os.environ, **{REPEAT_VARIABLE: str(repeat)})
                start = time.perf_counter()
                proc = subprocess.run([str(engine)], input=f"{args.command}\nquit\n", capture_output=True,
                                      text=True, env=env, timeout=3600, cwd=engine.parent)
                wall = time.perf_counter() - start
                nodes, ms = parse_bench(proc.stdout + proc.stderr)
                rows.append({"cycle": cycle, "repeat": repeat, "exit": proc.returncode, "nodes": nodes,
                             "ms": ms, "wall_s": round(wall, 3)})
                print(rows[-1], flush=True)
        summary = summarise(rows, args.base_ms, args.increment_ms)
    except Refused as refusal:
        print(f"REFUSED: {refusal}", file=sys.stderr)
        return 2
    record = {"schema": "basilisk-bridge-price-v1", "engine": str(engine), "engine_sha256": engine_sha,
              "command": args.command, "cycles": args.cycles, "base_ms": args.base_ms,
              "increment_ms": args.increment_ms, **summary, "rows": rows}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(record, indent=1) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in record.items() if k != "rows"}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
