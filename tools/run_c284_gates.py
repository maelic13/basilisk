#!/usr/bin/env python3
"""Frozen launcher for C.2.8.4's gates: the whole-surface refit against the head.

Both arms are final-PGO PEXT release builds pinned by SHA-256 and by their
manifests (`tools/build_test.ps1`):
* the baseline is the head with C.2.8's top-tier repair (Amendment 2,
  `cd4e453`), `basilisk-c284-base-pext-pgo.exe`;
* the candidate is the same head with BAS-O13's A fit (run 2) baked,
  `basilisk-c284-refit-pext-pgo.exe`.
One causal axis: the fitted surface. Both carry the repair.

  --run veto    C.2.7's bracket veto: endgame_truth.py at 21,000, 64,000 and
                190,000 nodes for both arms, then the veto rule, candidate
                against the repaired head (tools/results/c284/veto/).
  --run sprt    BAS-E59: SPRT [0,3] nElo (sprt-default.toml) at 3+0.03 1T,
                Hash 64, margin 20 ms, paired reversed UHO in random order,
                no adjudication, no tablebases, cap 4,000 pairs, seed 280401
                (tools/results/c284-sprt/).
  --run companion  BAS-E61: the same candidate against the head before the
                top-tier repair (basilisk-c273-head, 901e519), seed 280403
                (tools/results/c284-companion/).

A finished SPRT (result.json) is never extended or replaced: the launcher
exits 2. An interrupted one resumes from its checkpoint with the same command.

Usage:
  python tools/run_c284_gates.py --run <veto|sprt> [--dry-run]
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b66_games import Refused, powershell  # noqa: E402
from run_b111_reads import check_binary, ps  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
RESULTS = REPO / "tools" / "results"
ENGINES = REPO / "tools" / "test_engines"
BASE = {"path": ENGINES / "basilisk-c284-base-pext-pgo.exe",
        "sha": "10F6D74ED5D583A4E601B832E2D9EE226A16B64673D93FD130FA1E422250CDA3",
        "bench": 4392040, "revision": "b912051f28d78f793549601664a63e1017a9d982"}
CAND = {"path": ENGINES / "basilisk-c284-refit-pext-pgo.exe",
        "sha": "B2495083137483857DB82C2A9A556D1262BD454140F014FF2804439D86AC7C8C",
        "bench": 3359329, "revision": "cbc3676454ac97a28c88f1c739c845b66006be18"}
OLD_HEAD = {"path": ENGINES / "basilisk-c273-head-pext-pgo.exe",
            "sha": "EDDBA36E38C212B3132A9399ACE9F49CC943416DEF1F33B4B7EB90DFE7363A9B",
            "bench": 4777412, "revision": "901e5199419f0d46bc56617de60bec4ab97dba85"}
SEED = 280401
COMPANION_SEED = 280403
BUDGETS = "21000,64000,190000"
SYZYGY = "D:/chess/tablebases/syzygy3456"


def check_arm(arm: dict) -> None:
    check_binary(arm["path"], arm["sha"])
    manifest = arm["path"].with_suffix(".manifest.txt")
    fields = {}
    for line in manifest.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        key, sep, value = line.partition(":")
        if sep:
            fields[key.strip()] = value.strip()
    want = {"revision": arm["revision"], "dirty_diff": "clean", "bench": str(arm["bench"]),
            "binary_sha256": arm["sha"]}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {manifest.name} records {key} {fields.get(key)!r}, "
                          f"not {value!r}.")


def sprt(dry_run: bool, baseline: dict | None = None, name: str = "c284-sprt", seed: int = SEED,
         base_name: str = "Basilisk head") -> int:
    base = baseline or BASE
    run_dir = RESULTS / name
    if not dry_run and (run_dir / "result.json").exists():
        raise Refused(f"STALE RESULT - {run_dir} holds a finished run; it is not extended or replaced.")
    params = {"MarginMs": 20, "Seed": seed, "Dir": str(run_dir), "Mode": "sprt", "Bracket": "default",
              "MaxPairs": 4000, "Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30,
              "EngineA": str(CAND["path"]), "NameA": "Basilisk C.2.8 refit",
              "EngineB": str(base["path"]), "NameB": base_name,
              "ExpectBench": [CAND["bench"], base["bench"]], "ExpectSha256": [CAND["sha"], base["sha"]]}
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    parts += [f"-{key} {ps(value)}" for key, value in params.items()]
    if dry_run:
        parts.append("-DryRun")
    return subprocess.run(powershell() + [" ".join(parts) + "; exit $LASTEXITCODE"], cwd=REPO).returncode


def veto(dry_run: bool) -> int:
    out = RESULTS / "c284" / "veto"
    bracket = [sys.executable, str(REPO / "tools" / "diag" / "endgame_budget_bracket.py")]
    steps = [
        [*bracket, "run", "--engine", str(BASE["path"]), "--syzygy", SYZYGY, "--budgets", BUDGETS,
         "--workers", "30", "--out-dir", str(out / "base")],
        [*bracket, "run", "--engine", str(CAND["path"]), "--syzygy", SYZYGY, "--budgets", BUDGETS,
         "--workers", "30", "--out-dir", str(out / "refit")],
        [*bracket, "veto", "--baseline-dir", str(out / "base"), "--candidate-dir", str(out / "refit"),
         "--budgets", BUDGETS, "--output", str(out / "veto.json")],
        [*bracket, "veto", "--baseline-dir", str(RESULTS / "c274" / "head"), "--candidate-dir",
         str(out / "refit"), "--budgets", BUDGETS,
         "--output", str(out / "veto_against_c274_head.json")],
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
        status = status or done.returncode
    return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run", required=True, choices=("veto", "sprt", "companion"))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        check_arm(BASE)
        check_arm(CAND)
        if args.run == "companion":
            check_arm(OLD_HEAD)
            return sprt(args.dry_run, OLD_HEAD, "c284-companion", COMPANION_SEED, "Basilisk head before the repair")
        return sprt(args.dry_run) if args.run == "sprt" else veto(args.dry_run)
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
