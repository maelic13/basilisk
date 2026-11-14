#!/usr/bin/env python3
"""Time the search-replay board benchmark per operation class, and record it.

Records a fresh `bench 13` trace with a BASILISK_BOARD_TRACE build (or takes
one recorded by this tool before), verifies it on every replayer, then times
each (replayer, class list) as its own process pinned to one logical CPU,
once as the timed replay and once as its stub control (tests/board_replay.cpp
says what each is). The jobs alternate, the order reversed on odd rounds.
Every replayer is copied into the output directory first and the copy runs.

    python tools/diag/board_replay_run.py --out tools/results/b75/head \\
        --record build/b75-trace/basilisk.exe \\
        --replay head=build/b75-plain/board_replay.exe [--replay new=...] \\
        [--lists none,gen,see,...] [--rounds 3] [--passes 5] [--cpu 2] [--allow-busy]

`--trace T` reuses a trace this tool recorded (its `T.json` holds the node
count and hashes) instead of `--record`. A trace is about 470 MB for
`bench 13`; it stays in tools/results/ and is never committed.

Per list it prints, in ns per node (medians over every pass of every round):
the timed replay and its spread, the stub, the board (timed less stub), and
each class's marginal cost over `none` (make, unmake and null moves alone),
uncorrected (timed) and corrected (board). With two or more replayers, each
one's timed difference to the first: an A/B on one trace needs no
correction. The replay prices the board alone, a screen with the right sign;
pooled-PGO NPS decides what ships (agents/measurement.md).
"""

from __future__ import annotations

import argparse
import json
import os
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
LISTS = ["none", "gen", "see", "gives_check", "check_squares", "is_legal", "draw", "misc", "all"]
PASS = re.compile(r"^pass \d+: total \d+ ns\s+[0-9.]+ ns/record\s+([0-9.]+) ns/node\s+checksum ([0-9a-f]+)$")
NODES = re.compile(r"Nodes searched\s*:\s*(\d+)")


def fingerprint() -> int:
    return int(check_roadmap.validate_fingerprint(REPO).replace(",", ""))


def record_trace(engine: Path, trace: Path) -> dict:
    """Records `bench 13` into `trace` and writes its sidecar `trace.json`."""
    env = dict(os.environ, BASILISK_BOARD_TRACE_FILE=str(trace))
    proc = subprocess.run([str(engine), "bench", "13"], env=env, capture_output=True, text=True)
    match = NODES.search(proc.stdout)
    if proc.returncode != 0 or not match:
        raise SystemExit(f"recording with {engine}: exit {proc.returncode}\n{proc.stdout}{proc.stderr}")
    nodes = int(match.group(1))
    if nodes != fingerprint():
        raise SystemExit(f"the trace build searched {nodes} nodes, not the fingerprint {fingerprint()}")
    if not trace.is_file() or not Path(f"{trace}.fens").is_file():
        raise SystemExit(f"{engine} wrote no trace; is it a BASILISK_BOARD_TRACE build?")
    meta = {"nodes": nodes, "engine": str(engine), "engine_sha256": host.sha256(engine),
            "trace_sha256": host.sha256(trace), "trace_bytes": trace.stat().st_size,
            "records": trace.stat().st_size // 8,
            "recorded_utc": time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime())}
    Path(f"{trace}.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
    return meta


def verify(replay: Path, trace: Path) -> str:
    proc = subprocess.run([str(replay), str(trace), "--verify"], capture_output=True, text=True)
    if proc.returncode != 0 or "verify: PASS" not in proc.stdout:
        raise SystemExit(f"{replay} does not reproduce {trace}:\n{proc.stdout}{proc.stderr}")
    return proc.stdout


def timed(replay: Path, trace: Path, classes: str, passes: int, nodes: int, cpu: int,
          stub: bool) -> tuple[str, list[float], str]:
    command = [str(replay), str(trace), "--classes", classes, "--passes", str(passes),
               "--nodes", str(nodes)] + (["--stub"] if stub else [])
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    host.pin(proc.pid, cpu)
    output, _ = proc.communicate()
    if proc.returncode != 0:
        raise SystemExit(f"{' '.join(command)}: exit {proc.returncode}\n{output}")
    samples, sums = [], set()
    for line in output.splitlines():
        if m := PASS.match(line.strip()):
            samples.append(float(m.group(1)))
            sums.add(m.group(2))
    if len(samples) != passes or len(sums) != 1:
        raise SystemExit(f"{' '.join(command)}: expected {passes} passes with one checksum\n{output}")
    return output, samples, sums.pop()


def spread(values: list[float]) -> float:
    return 100.0 * (max(values) - min(values)) / statistics.median(values)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, required=True)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--record", type=Path, metavar="ENGINE",
                        help="a BASILISK_BOARD_TRACE build of basilisk; records <out>/bench13.trace")
    source.add_argument("--trace", type=Path, help="a trace this tool recorded before")
    parser.add_argument("--replay", action="append", required=True, metavar="LABEL=PATH")
    parser.add_argument("--revision", action="append", default=[], metavar="LABEL=SHA")
    parser.add_argument("--lists", default=",".join(LISTS),
                        help="class lists, comma-separated; join classes of one list with +")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--passes", type=int, default=5)
    parser.add_argument("--no-stub", action="store_true", help="time no stub controls (an A/B)")
    parser.add_argument("--cpu", type=int, default=2)
    parser.add_argument("--max-busy", type=float, default=10.0)
    parser.add_argument("--allow-busy", action="store_true", help="read anyway; recorded")
    args = parser.parse_args()

    if args.out.exists() and any(args.out.iterdir()):
        print(f"{args.out} is not empty; a read is never overwritten", file=sys.stderr)
        return 2
    lists = [entry.replace("+", ",") for entry in args.lists.split(",") if entry]
    if "none" not in lists:
        print("the lists must include `none`, the base every marginal is read from", file=sys.stderr)
        return 2
    replays: list[tuple[str, Path]] = []
    for spec in args.replay:
        label, _, path = spec.partition("=")
        if not label or not Path(path).is_file() or label in dict(replays):
            print(f"bad --replay {spec}", file=sys.stderr)
            return 2
        replays.append((label, Path(path)))

    # The host is checked before anything is written, so a refusal leaves
    # nothing behind to block the next attempt.
    load = host.host_load()
    busy = load["busy_percent"]
    unfit = (busy is None or busy > args.max_busy) or bool(load["competitors"])
    print(f"host: {busy if busy is None else round(busy, 1)}% busy; competitors {load['competitors'] or 'none'}")
    if unfit and not args.allow_busy:
        print("the host is not idle; stop the other work or pass --allow-busy (recorded)", file=sys.stderr)
        return 3
    args.out.mkdir(parents=True, exist_ok=True)

    if args.record:
        trace = args.out / "bench13.trace"
        meta = record_trace(args.record, trace)
    else:
        trace = args.trace
        sidecar = Path(f"{trace}.json")
        if not sidecar.is_file():
            print(f"{sidecar} is missing: record traces with --record", file=sys.stderr)
            return 2
        meta = json.loads(sidecar.read_text(encoding="utf-8"))
        if host.sha256(trace) != meta["trace_sha256"]:
            print(f"{trace} does not match its sidecar's hash", file=sys.stderr)
            return 2
    nodes = meta["nodes"]
    print(f"trace: {trace} ({meta['records']} records, {nodes} nodes, {meta['trace_sha256'][:16]})")

    copies: list[tuple[str, Path]] = []
    for label, path in replays:
        copy = args.out / f"{label}{path.suffix}"
        shutil.copy2(path, copy)
        copies.append((label, copy))
        (args.out / f"verify_{label}.txt").write_text(verify(copy, trace), encoding="utf-8")
        print(f"{label}: verify PASS")

    revisions = dict(r.split("=", 1) for r in args.revision)
    manifest = {
        "profile": "basilisk-board-replay-v1",
        "started_utc": time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime()),
        "repo_revision": host.git("rev-parse", "HEAD"),
        "repo_dirty": bool(host.git("status", "--porcelain")),
        "host": {"platform": platform.platform(), "processor": platform.processor(),
                 "load_before": load, "allow_busy": args.allow_busy, "cpu": args.cpu},
        "trace": {"path": str(trace), **meta},
        "lists": lists, "rounds": args.rounds, "passes": args.passes, "stub": not args.no_stub,
        "replays": {label: {"sha256": host.sha256(path), "revision": revisions.get(label, "")}
                    for label, path in copies},
        "runs": [],
    }
    jobs = [(label, path, classes, stub) for classes in lists for label, path in copies
            for stub in ((False,) if args.no_stub else (False, True))]
    samples: dict[str, list[float]] = {}
    checksums: dict[str, str] = {}
    for round_index in range(args.rounds):
        for label, path, classes, stub in (jobs if round_index % 2 == 0 else list(reversed(jobs))):
            output, values, checksum = timed(path, trace, classes, args.passes, nodes, args.cpu, stub)
            key = f"{label}|{classes}|{'stub' if stub else 'timed'}"
            name = f"run{round_index:02d}_{label}_{classes.replace(',', '+')}_{'stub' if stub else 'timed'}.txt"
            (args.out / name).write_text(output, encoding="utf-8")
            samples.setdefault(key, []).extend(values)
            checksums.setdefault(key, checksum)
            manifest["runs"].append({"round": round_index, "key": key, "output": name,
                                     "ns_per_node": values, "checksum": checksum})
            # Every sample is on disk before anything is printed.
            (args.out / "samples.json").write_text(json.dumps(samples, indent=1) + "\n", encoding="utf-8")
        print(f"round {round_index}: done")

    med = {key: statistics.median(values) for key, values in samples.items()}
    summary: dict[str, dict] = {}
    for label, _ in copies:
        base_t = med[f"{label}|none|timed"]
        base_s = med.get(f"{label}|none|stub")
        for classes in lists:
            t = med[f"{label}|{classes}|timed"]
            s = med.get(f"{label}|{classes}|stub")
            row = {"timed": t, "timed_spread_percent": spread(samples[f"{label}|{classes}|timed"]),
                   "marginal_uncorrected": t - base_t if classes != "none" else None,
                   "checksum": checksums[f"{label}|{classes}|timed"]}
            if s is not None:
                row.update(stub=s, stub_spread_percent=spread(samples[f"{label}|{classes}|stub"]),
                           board=t - s,
                           marginal_board=(t - s) - (base_t - base_s) if classes != "none" else None)
            summary[f"{label}|{classes}"] = row
    manifest["summary_ns_per_node"] = summary
    first = copies[0][0]
    differing = sorted({classes for classes in lists for label, _ in copies
                        if checksums[f"{label}|{classes}|timed"] != checksums[f"{first}|{classes}|timed"]})
    manifest["checksums_differ"] = differing
    manifest["completed_utc"] = time.strftime("%Y-%m-%d %H:%M:%SZ", time.gmtime())
    manifest["load_after"] = host.host_load()
    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    def num(value: float | None, width: int = 9) -> str:
        return f"{value:{width}.2f}" if value is not None else f"{'-':>{width}s}"

    print(f"\nns per node, median of {args.rounds} rounds x {args.passes} passes"
          f" (marginal: over `none`)")
    print(f"{'replay':10s} {'list':24s} {'timed':>9s} {'spr%':>5s} {'stub':>9s} {'board':>9s}"
          f" {'marg.unc':>9s} {'marg.brd':>9s}")
    for label, _ in copies:
        for classes in lists:
            row = summary[f"{label}|{classes}"]
            print(f"{label:10s} {classes:24s} {row['timed']:9.2f} {row['timed_spread_percent']:5.1f}"
                  f" {num(row.get('stub'))} {num(row.get('board'))}"
                  f" {num(row['marginal_uncorrected'])} {num(row.get('marginal_board'))}")
    if len(copies) > 1:
        print(f"\ntimed difference to {first}, ns per node (negative is faster)")
        print(f"{'list':24s}" + "".join(f"{label:>12s}" for label, _ in copies[1:]))
        for classes in lists:
            print(f"{classes:24s}" + "".join(
                f"{summary[f'{label}|{classes}']['timed'] - summary[f'{first}|{classes}']['timed']:12.2f}"
                for label, _ in copies[1:]))
    if differing:
        print(f"\nCHECKSUMS DIFFER from {first} on: {', '.join(differing)}"
              " (the replayers did different work)")
    print(f"\nrecorded in {args.out}")
    return 1 if differing else 0


if __name__ == "__main__":
    sys.exit(main())
