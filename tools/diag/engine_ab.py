#!/usr/bin/env python3
"""The engine screen: alternating `bench 13` runs of engine builds on one CPU.

Each binary is copied into the output directory and the copy runs, one
process per run pinned to one logical CPU at high priority. Every binary
first runs once unrecorded (it warms the file cache and checks the node
count); then each round runs every binary once, the order reversed on odd
rounds. Every run must search exactly the bench fingerprint GUIDE declares.

    python tools/diag/engine_ab.py --out tools/results/b75/screen \\
        --binary head=build/b75-plain/basilisk.exe --binary new=build/x/basilisk.exe \\
        [--rounds 16] [--cpu 2] [--allow-busy]

Per binary it prints the median time and, against the first binary, the
per-round time ratio (median and range), the NPS change it implies, and the
rounds in which the binary was faster. A null pair -- the same build twice
under two labels -- is read first and gives the screen's resolution.

Plain release builds, one thread, about two seconds a run: a screen with the
right sign. Pooled-PGO NPS decides what ships (agents/measurement.md).
"""

from __future__ import annotations

import argparse
import json
import platform
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import board_micro_run as host  # noqa: E402  (host load, pinning, hashes)
import check_roadmap  # noqa: E402  (the single fingerprint parser)

REPO = Path(__file__).resolve().parents[2]
NODES = re.compile(r"Nodes searched\s*:\s*(\d+)")
TIME = re.compile(r"Total time \(ms\)\s*:\s*(\d+)")


def raise_priority(pid: int) -> None:
    """High priority for the timed process: SetPriorityClass on Windows, a
    best-effort nice of -5 elsewhere (it needs privileges; refused quietly)."""
    if sys.platform == "win32":
        import ctypes
        kernel = ctypes.windll.kernel32
        handle = kernel.OpenProcess(0x0200 | 0x0400, False, pid)
        if handle:
            kernel.SetPriorityClass(handle, 0x00000080)  # HIGH_PRIORITY_CLASS
            kernel.CloseHandle(handle)
    else:
        try:
            import os
            os.setpriority(os.PRIO_PROCESS, pid, -5)
        except (OSError, AttributeError):
            pass


def bench(binary: Path, cpu: int, expected: int) -> tuple[str, int]:
    proc = subprocess.Popen([str(binary)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    host.pin(proc.pid, cpu)
    raise_priority(proc.pid)
    output, _ = proc.communicate("bench 13\nquit\n")
    nodes, ms = NODES.search(output), TIME.search(output)
    if proc.returncode != 0 or not nodes or not ms:
        raise SystemExit(f"{binary}: exit {proc.returncode}, no bench summary\n{output}")
    if int(nodes.group(1)) != expected:
        raise SystemExit(f"{binary} searched {nodes.group(1)} nodes, not the fingerprint {expected}:"
                         " the screen compares behaviour-neutral builds only")
    return output, int(ms.group(1))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--binary", action="append", required=True, metavar="LABEL=PATH")
    parser.add_argument("--revision", action="append", default=[], metavar="LABEL=SHA")
    parser.add_argument("--rounds", type=int, default=16)
    parser.add_argument("--cpu", type=int, default=2)
    parser.add_argument("--max-busy", type=float, default=10.0)
    parser.add_argument("--allow-busy", action="store_true", help="read anyway; recorded")
    args = parser.parse_args()

    if args.out.exists() and any(args.out.iterdir()):
        print(f"{args.out} is not empty; a read is never overwritten", file=sys.stderr)
        return 2
    binaries: list[tuple[str, Path]] = []
    for spec in args.binary:
        label, _, path = spec.partition("=")
        if not label or not Path(path).is_file() or label in dict(binaries):
            print(f"bad --binary {spec}", file=sys.stderr)
            return 2
        binaries.append((label, Path(path)))
    if len(binaries) < 2:
        print("give two binaries or more; a null pair is one build under two labels", file=sys.stderr)
        return 2
    expected = int(check_roadmap.validate_fingerprint(REPO).replace(",", ""))

    load = host.host_load()
    busy = load["busy_percent"]
    unfit = (busy is None or busy > args.max_busy) or bool(load["competitors"])
    print(f"host: {busy if busy is None else round(busy, 1)}% busy; competitors {load['competitors'] or 'none'}")
    if unfit and not args.allow_busy:
        print("the host is not idle; stop the other work or pass --allow-busy (recorded)", file=sys.stderr)
        return 3
    args.out.mkdir(parents=True, exist_ok=True)

    copies: list[tuple[str, Path]] = []
    for label, path in binaries:
        copy = args.out / f"{label}{path.suffix}"
        shutil.copy2(path, copy)
        copies.append((label, copy))
    revisions = dict(r.split("=", 1) for r in args.revision)
    hashes = {label: host.sha256(path) for label, path in copies}
    manifest = {
        "profile": "basilisk-engine-screen-v1",
        "started_utc": time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime()),
        "repo_revision": host.git("rev-parse", "HEAD"),
        "repo_dirty": bool(host.git("status", "--porcelain")),
        "host": {"platform": platform.platform(), "processor": platform.processor(),
                 "load_before": load, "allow_busy": args.allow_busy, "cpu": args.cpu},
        "fingerprint": expected, "rounds": args.rounds,
        "binaries": {label: {"sha256": hashes[label], "source": str(path),
                             "revision": revisions.get(label, "")} for label, path in binaries},
        "null_pair": len(set(hashes.values())) == 1,
        "runs": [],
    }
    for label, path in copies:
        output, ms = bench(path, args.cpu, expected)
        (args.out / f"warmup_{label}.txt").write_text(output, encoding="utf-8")
        print(f"{label}: fingerprint {expected}, warm-up {ms} ms")

    times: dict[str, list[int]] = {label: [] for label, _ in copies}
    for round_index in range(args.rounds):
        for label, path in (copies if round_index % 2 == 0 else list(reversed(copies))):
            output, ms = bench(path, args.cpu, expected)
            name = f"run{round_index:02d}_{label}.txt"
            (args.out / name).write_text(output, encoding="utf-8")
            times[label].append(ms)
            manifest["runs"].append({"round": round_index, "binary": label, "ms": ms, "output": name})
            (args.out / "times.json").write_text(json.dumps(times, indent=1) + "\n", encoding="utf-8")
        print(f"round {round_index}: " + "  ".join(f"{label} {times[label][-1]} ms" for label, _ in copies))

    first = copies[0][0]
    summary = {}
    for label, _ in copies:
        ratios = [t / r for t, r in zip(times[label], times[first])]
        median = statistics.median(ratios)
        summary[label] = {
            "median_ms": statistics.median(times[label]),
            "ratio_median": median, "ratio_min": min(ratios), "ratio_max": max(ratios),
            "nps_change_percent": 100.0 * (1.0 / median - 1.0),
            "nps_change_range_percent": [100.0 * (1.0 / max(ratios) - 1.0),
                                         100.0 * (1.0 / min(ratios) - 1.0)],
            "rounds_faster": sum(t < r for t, r in zip(times[label], times[first])),
        }
    manifest["summary"] = summary
    manifest["completed_utc"] = time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime())
    manifest["load_after"] = host.host_load()
    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print(f"\n{'binary':10s} {'median ms':>9s} {'ratio':>8s} {'min':>8s} {'max':>8s}"
          f" {'NPS change (range)':>26s} {'faster':>8s}")
    for label, _ in copies:
        row = summary[label]
        low, high = row["nps_change_range_percent"]
        print(f"{label:10s} {row['median_ms']:9.1f} {row['ratio_median']:8.4f} {row['ratio_min']:8.4f}"
              f" {row['ratio_max']:8.4f} {row['nps_change_percent']:+8.2f}% ({low:+.2f} to {high:+.2f})"
              f" {row['rounds_faster']:4d}/{args.rounds}")
    if manifest["null_pair"]:
        print("\nnull pair: every binary is the same build; the spread is the screen's resolution")
    print(f"\nrecorded in {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
