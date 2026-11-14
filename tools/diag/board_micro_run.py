#!/usr/bin/env python3
"""Run the board microbenchmark on pinned binaries and record the read.

Each binary is copied into the output directory first and the copy is what
runs, so a rebuild during the read cannot change it. With several binaries
the runs alternate (A B A B ...) for `--rounds` rounds, one process per run,
pinned to one logical CPU. Every run's full output is kept beside a
manifest: the binary's SHA-256, its source revision when given, the host,
the CPU load and the competing processes seen before the read.

    python tools/diag/board_micro_run.py --out tools/results/b72-micro/head \\
        --binary head=build/release/board_micro_bench.exe [--binary new=...] \\
        [--rounds 3] [--samples 15] [--sample-ms 200] [--cpu 2] [--allow-busy]

The summary prints, per column, the median over runs of each run's median
ns/op, and with two or more binaries each one's ratio to the first. A
microbenchmark column is a screen for a component, never a speed verdict:
pooled-PGO NPS decides (agents/measurement.md).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import re
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
POSITIONS = REPO / "tests" / "data" / "board_micro_positions.fen"
# Processes that make a host unfit for timing: engines, harnesses, builds.
COMPETITORS = ("basilisk", "rarog", "colosseum", "fastchess", "cutechess", "stockfish",
               "cargo", "rustc", "clang", "ninja", "cmake", "python-chess")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def git(*args: str) -> str:
    out = subprocess.run(["git", *args], cwd=REPO, capture_output=True, text=True)
    return out.stdout.strip() if out.returncode == 0 else ""


# ---- Platform-specific: load, processes, affinity -----------------------------

def host_load() -> dict:
    """CPU use over about three seconds and the competing processes running.

    Windows reads typeperf and tasklist; Linux reads /proc/stat and ps.
    """
    if sys.platform == "win32":
        out = subprocess.run(["typeperf", r"\Processor(_Total)\% Processor Time", "-sc", "3"],
                             capture_output=True, text=True).stdout
        samples = [float(v) for v in re.findall(r'","([0-9.]+)"', out)]
        busy = statistics.mean(samples) if samples else None
        tasks = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True).stdout
        names = [line.split('","')[0].strip('"') for line in tasks.splitlines() if line]
    else:
        def stat() -> tuple[int, int]:
            fields = [int(x) for x in Path("/proc/stat").read_text().split("\n")[0].split()[1:]]
            return sum(fields), fields[3] + fields[4]
        t0, i0 = stat()
        time.sleep(3)
        t1, i1 = stat()
        busy = 100.0 * (1 - (i1 - i0) / max(t1 - t0, 1))
        names = subprocess.run(["ps", "-eo", "comm="], capture_output=True, text=True).stdout.split()
    seen = sorted({n for n in names if any(c in n.lower() for c in COMPETITORS)
                   and "board_micro" not in n.lower()})
    return {"busy_percent": busy, "competitors": seen}


def pin(pid: int, cpu: int) -> None:
    """Pin a process to one logical CPU (SetProcessAffinityMask on Windows)."""
    if sys.platform == "win32":
        import ctypes
        kernel = ctypes.windll.kernel32
        handle = kernel.OpenProcess(0x0200 | 0x0400, False, pid)  # SET_INFORMATION | QUERY_INFORMATION
        if not handle or not kernel.SetProcessAffinityMask(handle, ctypes.c_size_t(1 << cpu)):
            raise SystemExit(f"cannot pin process {pid} to CPU {cpu}")
        kernel.CloseHandle(handle)
    elif hasattr(__import__("os"), "sched_setaffinity"):
        __import__("os").sched_setaffinity(pid, {cpu})
    else:
        raise SystemExit("no way to pin a process on this platform")


# ---- Runs ---------------------------------------------------------------------

ROW = re.compile(r"^(\S+)\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)%\s+(\d+)$")
WORK = re.compile(r"^work (\S+)\s+(\d+) \S+\s+checksum ([0-9a-f]+)$")
SEMANTIC = re.compile(r"^semantic (\S+) (\d+)$")
NONCOMPARABLE = re.compile(r"^noncomparable (\S+) (.*)$")
# Engine-independent counts that must agree between engines; see_ge0 may
# differ with the engines' exchange rules (tools/board_compare/CONTRACT.md).
MUST_AGREE = ("positions", "legal", "checking", "perft", "quiet_positions", "quiet")
# Counts a harness built before cross-engine-board-v3 does not print: they
# must agree among the binaries that print them.
ADDED_IN_V3 = ("quiet_positions", "quiet")


def run_once(binary: Path, args: argparse.Namespace) -> tuple[str, dict, dict, dict]:
    command = [str(binary), str(args.positions), "--samples", str(args.samples),
               "--sample-ms", str(args.sample_ms)]
    if args.columns:
        command += ["--columns", args.columns]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    pin(proc.pid, args.cpu)
    output, _ = proc.communicate()
    if proc.returncode != 0:
        raise SystemExit(f"{binary} exited {proc.returncode}:\n{output}")
    medians, work, semantic = {}, {}, {}
    for line in output.splitlines():
        if m := ROW.match(line.strip()):
            medians[m.group(1)] = float(m.group(2))
        if m := WORK.match(line.strip()):
            work[m.group(1)] = {"ops": int(m.group(2)), "checksum": m.group(3)}
        if m := SEMANTIC.match(line.strip()):
            semantic[m.group(1)] = int(m.group(2))
        if m := NONCOMPARABLE.match(line.strip()):
            semantic[f"noncomparable {m.group(1)}"] = m.group(2)
    if not medians:
        raise SystemExit(f"no summary rows in the output of {binary}")
    if "preflight: PASS" not in output:
        raise SystemExit(f"{binary} did not pass its preflight")
    return output, medians, work, semantic


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--binary", action="append", required=True, metavar="LABEL=PATH")
    parser.add_argument("--revision", action="append", default=[], metavar="LABEL=SHA",
                        help="the source revision a binary was built from")
    parser.add_argument("--positions", type=Path, default=POSITIONS)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--samples", type=int, default=15)
    parser.add_argument("--sample-ms", type=float, default=200.0)
    parser.add_argument("--columns", default="")
    parser.add_argument("--cpu", type=int, default=2)
    parser.add_argument("--max-busy", type=float, default=10.0,
                        help="percent; nps_read.ps1's quiet ceiling")
    parser.add_argument("--allow-busy", action="store_true", help="read anyway; recorded")
    args = parser.parse_args()

    if args.out.exists() and any(args.out.iterdir()):
        print(f"{args.out} is not empty; a read is never overwritten", file=sys.stderr)
        return 2
    # The host is checked before anything is written, so a refusal leaves
    # nothing behind to block the next attempt.
    load = host_load()
    busy = load["busy_percent"]
    unfit = (busy is None or busy > args.max_busy) or bool(load["competitors"])
    print(f"host: {busy if busy is None else round(busy, 1)}% busy; competitors {load['competitors'] or 'none'}")
    if unfit and not args.allow_busy:
        print("the host is not idle; stop the other work or pass --allow-busy (recorded)", file=sys.stderr)
        return 3
    args.out.mkdir(parents=True, exist_ok=True)

    revisions = dict(r.split("=", 1) for r in args.revision)
    binaries: list[tuple[str, Path]] = []
    for spec in args.binary:
        label, _, path = spec.partition("=")
        source = Path(path)
        if not label or not source.is_file():
            print(f"bad --binary {spec}", file=sys.stderr)
            return 2
        copy = args.out / f"{label}{source.suffix}"
        shutil.copy2(source, copy)
        binaries.append((label, copy))


    manifest = {
        "profile": "basilisk-board-micro-v1",
        "started_utc": time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime()),
        "repo_revision": git("rev-parse", "HEAD"),
        "repo_dirty": bool(git("status", "--porcelain")),
        "host": {"platform": platform.platform(), "processor": platform.processor(),
                 "load_before": load, "allow_busy": args.allow_busy, "cpu": args.cpu},
        "positions": {"path": str(args.positions), "sha256": sha256(args.positions)},
        "samples": args.samples, "sample_ms": args.sample_ms, "rounds": args.rounds,
        "binaries": {label: {"sha256": sha256(path), "revision": revisions.get(label, "")}
                     for label, path in binaries},
        "runs": [],
    }
    per_binary: dict[str, list[dict]] = {label: [] for label, _ in binaries}
    work_seen: dict[str, dict] = {}
    semantic_seen: dict[str, dict] = {}
    for round_index in range(args.rounds):
        order = binaries if round_index % 2 == 0 else list(reversed(binaries))
        for label, path in order:
            output, medians, work, semantic = run_once(path, args)
            name = f"run{round_index:02d}_{label}.txt"
            (args.out / name).write_text(output, encoding="utf-8")
            per_binary[label].append(medians)
            work_seen.setdefault(label, work)
            semantic_seen.setdefault(label, semantic)
            manifest["runs"].append({"round": round_index, "binary": label, "output": name})
            print(f"round {round_index} {label}: done")

    manifest["completed_utc"] = time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime())
    manifest["load_after"] = host_load()
    labels = [label for label, _ in binaries]
    # Columns in the first binary's order, then any only later ones have.
    columns: list[str] = []
    for label in labels:
        columns += [c for c in per_binary[label][0] if c not in columns]
    summary = {}
    for column in columns:
        summary[column] = {label: statistics.median(run[column] for run in per_binary[label])
                           for label in labels if column in per_binary[label][0]}
    manifest["summary_ns_per_op"] = summary
    manifest["work"] = work_seen
    manifest["semantic"] = semantic_seen
    # Two builds of one engine do the same work and fold it to the same
    # checksums when the change is behaviour-neutral; between engines the
    # checksums differ by construction and the semantic counts are compared.
    differing = sorted(c for c in columns
                       if len({json.dumps(work_seen[label].get(c)) for label in labels
                               if c in work_seen[label]}) > 1)
    manifest["checksums_differ"] = differing
    disagree = sorted(k for k in MUST_AGREE
                      if len({semantic_seen[label].get(k) for label in labels} - (
                          {None} if k in ADDED_IN_V3 else set())) > 1)
    manifest["semantic_disagree"] = disagree
    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    header = f"{'column':16s}" + "".join(f"{label:>14s}" for label in labels)
    if len(labels) > 1:
        header += "".join(f"{label + '/' + labels[0]:>16s}" for label in labels[1:])
    print("\nmedian ns/op over runs\n" + header)
    def cell(row: dict, label: str) -> str:
        return f"{row[label]:14.3f}" if label in row else f"{'-':>14s}"

    def ratio(row: dict, label: str) -> str:
        if label in row and labels[0] in row:
            return f"{row[label] / row[labels[0]]:16.4f}"
        return f"{'-':>16s}"

    for column in columns:
        row = summary[column]
        print(f"{column:16s}" + "".join(cell(row, label) for label in labels)
              + "".join(ratio(row, label) for label in labels[1:]))
    if len(labels) > 1:
        keys = sorted({k for label in labels for k in semantic_seen[label]})
        print("\nsemantic counts\n" + f"{'':24s}" + "".join(f"{label:>14s}" for label in labels))
        for key in keys:
            print(f"{key:24s}" + "".join(f"{str(semantic_seen[label].get(key, '-')):>14s}"
                                          for label in labels))
    if differing:
        print(f"\nwork or checksums differ between binaries: {', '.join(differing)}")
    print(f"\nrecorded in {args.out}")
    if disagree:
        print(f"\nSEMANTIC DISAGREEMENT, the read is not comparable: {', '.join(disagree)}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
