#!/usr/bin/env python3
"""Frozen launcher for C.3.6's two game reads, BAS-E66 and BAS-E67.

The contract is analysis/c36_own_forms_2026-11-26.md, Amendment 1 (the rules
for both SPRTs), and the registrations docs/experiments/BAS-E66.md and
BAS-E67.md. Both arms are clean final-PGO PEXT release builds
(`tools/build_test.ps1 -Flavor Release`) named on the command line and checked
by SHA-256 and by their manifests: the same compiler, a clean tree, the
recorded bench.

  sprt   SPRT [0,3] nElo (Colosseum, sprt-default.toml) at 3+0.03 1T, Hash 64,
         margin 20 ms, paired colour-reversed UHO_Lichess_4852_v1 in random
         order, no adjudication, no tablebases; cap 10,000 pairs for E66,
         20,000 for E67; seed 360066 or 360067 (<read>-sprt/).
  veto   the C.2.7 endgame bracket at 21,000, 64,000 and 190,000 nodes, the
         base and the candidate, then the veto (<read>-veto/). For this run
         only (the maintainer's ruling) its clean-win discards and conversion
         counts neither block nor hold; a new_rule50_failure, engine_anomaly
         or malformed_record is a hard failure and returns to research.

A finished leg (result.json, or veto.json) is never extended or replaced.

Usage:
  python tools/run_c36_reads.py --read e66|e67 --leg sprt|veto \\
      --cand EXE --cand-sha SHA --cand-bench N --base EXE --base-sha SHA --base-bench N [--dry-run]
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b111_reads import check_binary, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
RESULTS = REPO / "tools" / "results"
READS = {"e66": {"cap": 10000, "seed": 360066, "names": ("Basilisk refit C", "Basilisk shipped")},
         "e67": {"cap": 20000, "seed": 360067, "names": ("Basilisk own forms", "Basilisk base")}}
BUDGETS = "21000,64000,190000"
SYZYGY = "D:/chess/tablebases/syzygy3456"


def manifest_fields(path: pathlib.Path) -> dict[str, str]:
    manifest = path.with_suffix(".manifest.txt")
    if not manifest.exists():
        raise Refused(f"PROVENANCE MISMATCH - no manifest beside {path.name}.")
    fields = {}
    for line in manifest.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        key, sep, value = line.partition(":")
        if sep:
            fields[key.strip()] = value.strip()
    return fields


def check_arm(path: pathlib.Path, sha: str, bench: int) -> tuple[str, str]:
    check_binary(path, sha)
    fields = manifest_fields(path)
    want = {"dirty_diff": "clean", "bench": str(bench), "binary_sha256": sha, "flavor": "pext-pgo-release"}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {path.name}'s manifest records {key} "
                          f"{fields.get(key)!r}, not {value!r}.")
    if not fields.get("compiler"):
        raise Refused(f"PROVENANCE MISMATCH - {path.name}'s manifest records no compiler.")
    return fields["compiler"], fields.get("revision", "")


def run_sprt(args, read: dict, dry_run: bool) -> int:
    run_dir = RESULTS / f"c36-{args.read}-sprt"
    if (run_dir / "result.json").is_file() and not dry_run:
        raise Refused(f"STALE RESULT - {run_dir} holds a finished SPRT; it is not extended or replaced.")
    params = {
        "Mode": "sprt", "Bracket": "default", "MaxPairs": read["cap"], "BaseMs": 3000, "IncrementMs": 30,
        "MarginMs": 20, "Seed": read["seed"], "Dir": str(run_dir), "Hash": 64, "Threads": 1,
        "EngineA": str(args.cand), "NameA": read["names"][0],
        "EngineB": str(args.base), "NameB": read["names"][1],
        "ExpectBench": [args.cand_bench, args.base_bench], "ExpectSha256": [args.cand_sha, args.base_sha],
    }
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"] + [f"-{k} {ps(v)}" for k, v in params.items()]
    if dry_run:
        parts.append("-DryRun")
    return subprocess.run(powershell() + [" ".join(parts) + "; exit $LASTEXITCODE"], cwd=REPO).returncode


def run_veto(args, dry_run: bool) -> int:
    out = RESULTS / f"c36-{args.read}-veto"
    if (out / "veto.json").is_file() and not dry_run:
        raise Refused(f"STALE RESULT - {out} holds a finished veto; it is not replaced.")
    bracket = [sys.executable, str(REPO / "tools" / "diag" / "endgame_budget_bracket.py")]
    steps = [
        [*bracket, "run", "--engine", str(args.base), "--syzygy", SYZYGY, "--budgets", BUDGETS,
         "--workers", "30", "--out-dir", str(out / "base")],
        [*bracket, "run", "--engine", str(args.cand), "--syzygy", SYZYGY, "--budgets", BUDGETS,
         "--workers", "30", "--out-dir", str(out / "cand")],
        [*bracket, "veto", "--baseline-dir", str(out / "base"), "--candidate-dir", str(out / "cand"),
         "--budgets", BUDGETS, "--output", str(out / "veto.json")],
    ]
    if dry_run:
        for step in steps:
            print(" ".join(step))
        return 0
    out.mkdir(parents=True, exist_ok=True)
    status = 0
    for step in steps:
        done = subprocess.run(step, cwd=REPO)
        print(f"exit {done.returncode}: {' '.join(step[2:4])}", flush=True)
        if step[2] == "run" and done.returncode != 0:
            return done.returncode
        status = status or done.returncode
    return status


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--read", required=True, choices=sorted(READS))
    ap.add_argument("--leg", required=True, choices=("sprt", "veto"))
    for side in ("cand", "base"):
        ap.add_argument(f"--{side}", type=pathlib.Path, required=True)
        ap.add_argument(f"--{side}-sha", required=True)
        ap.add_argument(f"--{side}-bench", type=int, required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    try:
        c_cand, r_cand = check_arm(args.cand, args.cand_sha, args.cand_bench)
        c_base, r_base = check_arm(args.base, args.base_sha, args.base_bench)
        if c_cand != c_base:
            raise Refused(f"COMPILER MISMATCH - cand {c_cand!r}, base {c_base!r}.")
        print(f"Arms verified: cand {r_cand[:10]} bench {args.cand_bench}, base {r_base[:10]} "
              f"bench {args.base_bench}; compilers match.")
        if args.leg == "sprt":
            return run_sprt(args, READS[args.read], args.dry_run)
        return run_veto(args, args.dry_run)
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
