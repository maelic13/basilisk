#!/usr/bin/env python3
"""Frozen launcher for C.0's game reads: the evaluation programme investigation.

Every read is a two-engine gauntlet in BAS-O06's form: 1,500 cycles of one
opening played twice with colours reversed, 3,000 games, `3+0.03`, margin
20 ms, 1T, Hash 64, UHO in random order, concurrency 14, no adjudication, no
tablebases, zero permitted faults, every binary pinned by SHA-256. The first
engine is rated against the second, which is fixed at 1500, so a read's
result is the first engine's rating minus 1500.

  --read meter       BAS-O08, the same-search evaluation deficit E(0): the
                     oracle (Stockfish 9587eeeb's search) with its own
                     evaluation against the same binary with the head's
                     (`Use Basilisk HCE` false against true).
  --read donor-sf16  BAS-O09, Stockfish 16's search: 9587eeeb's evaluation
                     ported in against Stockfish 16's own classical one.
  --read donor-9587  BAS-O09, 9587eeeb's search: Stockfish 16's classical
                     evaluation ported back against 9587eeeb's own.

The donor engines are built by tools/donor/build_donor.py and proven by
tools/donor/verify_donor.py; this launcher refuses a binary whose hash or
bench differs from the pinned build.

A finished read (result.json) is never extended or replaced: the launcher
exits 2. An interrupted one has only its checkpoint, and the same command
resumes it.

Usage:
  python tools/run_c0_reads.py --read <meter|donor-sf16|donor-9587> [--dry-run]
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b111_reads import check_binary, gauntlet_args, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
RESULTS = REPO / "tools" / "results"
CYCLES = 1500

# Stockfish 9587eeeb's search around the head's evaluation (`src/eval.cpp`
# differs from 4d2742f in comments only).
ORACLE = {"path": ENGINES / "oracle-b111-4d2742f.exe",
          "sha": "8ADEE9AE80FE0B7AF7135C766045E846824383A0EDB70241B82E4F20336C020C"}
ORACLE_OPTION = "Use Basilisk HCE"
# Built 2026-11-18 from the donor patches (tools/foreign_patches.sha256); bench is the build's own
# signature (`bench 16 1 13 default depth classical` on the sf_16 hosts,
# plain `bench` on the 9587eeeb hosts).
DONOR = {
    "sf16-stock": {"sha": "C85B7F8280D56D5A23FD50BEE121F42408F0B94FAC00C2B387AD5EE8325D95E4", "bench": 3684832},
    "sf16-eval9587": {"sha": "C870CD697EEC7FD877986C336B2ECE09FA35A1A9B66E7F7271DB62639F88851C", "bench": 2360513},
    "sf9587-stock": {"sha": "2F2FE66BCDE3A205EE586E6F802554B3C5E496EC9A5C647C8206129823E635E3", "bench": 4746616},
    "sf9587-eval16": {"sha": "AF84AAFB437D8F7CBE6A8956332C275BA65136329A79C492481258827B826C26", "bench": 4572257},
}
READS = {
    "meter": {"seed": 120001, "arms": [("Oracle 9587 eval", "oracle", [f"{ORACLE_OPTION}=false"]),
                                       ("Oracle Basilisk eval", "oracle", [f"{ORACLE_OPTION}=true"])]},
    "donor-sf16": {"seed": 120002, "arms": [("SF16 search 9587 eval", "sf16-eval9587", []),
                                            ("SF16 search SF16 eval", "sf16-stock", [])]},
    "donor-9587": {"seed": 120003, "arms": [("9587 search SF16 eval", "sf9587-eval16", []),
                                            ("9587 search 9587 eval", "sf9587-stock", [])]},
}


def donor(name: str) -> dict:
    """A donor build, refused unless its hash and its manifest's bench are the pinned ones."""
    path = ENGINES / f"donor-{name}.exe"
    check_binary(path, DONOR[name]["sha"])
    manifest = path.with_suffix(".manifest.txt")
    if not manifest.exists():
        raise Refused(f"PROVENANCE MISMATCH - no manifest beside {path.name}.")
    fields = dict(line.split(":", 1) for line in manifest.read_text(encoding="utf-8").splitlines() if ":" in line)
    bench = fields.get("bench_nodes", "").strip()
    if bench != str(DONOR[name]["bench"]):
        raise Refused(f"PROVENANCE MISMATCH - {manifest.name} records bench {bench!r}, not {DONOR[name]['bench']}.")
    return {"path": path, "sha": DONOR[name]["sha"]}


def oracle_bench(option_value: str) -> int:
    """The oracle's node count on a short bench with its evaluation switch set."""
    script = f"setoption name {ORACLE_OPTION} value {option_value}\nbench 16 1 8 default depth\nquit\n"
    done = subprocess.run([str(ORACLE["path"])], input=script, capture_output=True, text=True)
    match = re.search(r"Nodes searched\s*:\s*(\d+)", done.stdout + done.stderr)
    if done.returncode != 0 or not match:
        raise Refused(f"ORACLE REFUSED - no bench node count with {ORACLE_OPTION}={option_value}.")
    return int(match.group(1))


def check_oracle() -> None:
    """The meter's only difference between its arms is one option: prove it moves the search."""
    check_binary(ORACLE["path"], ORACLE["sha"])
    own, bridged = oracle_bench("false"), oracle_bench("true")
    if own == bridged:
        raise Refused(f"DEAD WIRE - `{ORACLE_OPTION}` does not move the oracle's bench ({own} nodes both ways); "
                      "the match would compare an engine with itself.")
    print(f"Oracle wire live: bench 16 1 8 = {own} nodes with its own evaluation, {bridged} with the head's.")


def command(read: str, dry_run: bool) -> tuple[str, pathlib.Path]:
    spec = READS[read]
    run_dir = RESULTS / f"c0-{read}"
    engines = []
    for label, source, options in spec["arms"]:
        binary = ORACLE if source == "oracle" else donor(source)
        engines.append((label, str(binary["path"]), binary["sha"], 1500, "Threads", options))
    if read == "meter":
        check_oracle()
    extra = gauntlet_args(engines, CYCLES) + ["--max-engine-faults", "0"]
    params = {"Mode": "gauntlet", "Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30, "MarginMs": 20,
              "Seed": spec["seed"], "Dir": str(run_dir), "Concurrency": 14, "OpponentFaultRatePercent": 0,
              "ExpectBench": [-1, -1], "ExpectSha256": [e[2] for e in engines],
              "ThreadOptionNames": [e[4] for e in engines], "ExtraArgs": extra}
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in params.items():
        parts.append(f"-{key} {ps(value)}")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE", run_dir


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--read", required=True, choices=sorted(READS))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        text, run_dir = command(args.read, args.dry_run)
        if not args.dry_run and (run_dir / "result.json").exists():
            raise Refused(f"STALE RESULT - {run_dir} holds a finished read; it is not extended or replaced.")
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(powershell() + [text], cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
