#!/usr/bin/env python3
"""Frozen launcher for B.11.4's release licence: the 1.11.0 candidate against 1.10.2.

The candidate is the frozen search head's release build, the binary B.11.1 played
(`basilisk-b111-head-pext-pgo.exe`, `4c0bc0e`, bench 4,777,412). Its engine
source is master's `4d2742f`, and the branch since then removed only the
ablation flavour, which a release build compiles out. The baseline is 1.10.2
rebuilt from its tag (`fac5064`) with the candidate's Clang 23.1.5
(`basilisk-b114-v1102-pgo1-pext-pgo.exe`, built by `tools/nps_pool.ps1
-Revision fac5064 -Builds 1`, bench 14,978,465). Both are final-PGO
PEXT release builds pinned by SHA-256 and by their manifests. Every run uses
UHO in random order, paired colour-reversed, margin 20 ms, no adjudication and
no tablebases.

  --run sprt    BAS-S67: SPRT [0,3] nElo (sprt-default.toml) at 3+0.03 1T,
                Hash 64, cap 2,000 pairs, seed 114001.
  --run ltc     BAS-M19 (a): a 1,000-game fixed match at 10+0.1 1T, Hash 64,
                seed 114002.
  --run 4t      BAS-M19 (b): a 1,000-game fixed match at 3+0.03, 4 threads a
                side, Hash 512, sibling-free, seed 114003.
  --run pool4t  BAS-M20: the 4T pool read, a gauntlet against the four targets
                at their 1T Super Rating Tournament ratings (fixed), 200
                cycles, 400 games each, 4 threads a side, Hash 512,
                sibling-free, seed 114004.

Multi-thread runs are sibling-free through colosseum.ps1's placement rule
(PROCESS *Harness*, BAS-X41). A finished run (result.json) is never extended
or replaced: the launcher exits 2. An interrupted one resumes from its
checkpoint with the same command.

Usage:
  python tools/run_b114_gates.py --run <sprt|ltc|4t|pool4t> [--dry-run]
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b66_games import Refused, powershell  # noqa: E402
from run_b111_reads import HEAD, TARGETS, check_binary, check_head, gauntlet_args, ps  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
RESULTS = REPO / "tools" / "results"
BASE = {"path": REPO / "tools" / "test_engines" / "basilisk-b114-v1102-pgo1-pext-pgo.exe",
        "sha": "21BE11923F0012F865662606647D067C2E3706481CA2B1B56DA812F73CAF7A94",
        "bench": 14978465, "revision": "fac506446364ee229e7aa18c9c4fb107dfafc57a"}
SEEDS = {"sprt": 114001, "ltc": 114002, "4t": 114003, "pool4t": 114004}


def check_base() -> None:
    check_binary(BASE["path"], BASE["sha"])
    manifest = BASE["path"].with_suffix(".manifest.txt")
    fields = {}
    for line in manifest.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        key, sep, value = line.partition(":")
        if sep:
            fields[key.strip()] = value.strip()
    want = {"revision": BASE["revision"], "dirty_diff": "clean", "bench": str(BASE["bench"]),
            "binary_sha256": BASE["sha"]}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {manifest.name} records {key} {fields.get(key)!r}, not {value!r}.")


def command(run: str, dry_run: bool) -> tuple[str, pathlib.Path]:
    run_dir = RESULTS / f"b114-{run}"
    # Two revisions, which -ExpectRevision cannot name: the manifests are checked above.
    common = {"MarginMs": 20, "Seed": SEEDS[run], "Dir": str(run_dir)}
    pair = {"EngineA": str(HEAD["path"]), "NameA": "Basilisk 1.11.0 candidate",
            "EngineB": str(BASE["path"]), "NameB": "Basilisk 1.10.2",
            "ExpectBench": [HEAD["bench"], BASE["bench"]], "ExpectSha256": [HEAD["sha"], BASE["sha"]]}
    if run == "sprt":
        params = {**common, "Mode": "sprt", "Bracket": "default", "MaxPairs": 2000, "Hash": 64, "Threads": 1,
                  "BaseMs": 3000, "IncrementMs": 30, **pair}
    elif run == "ltc":
        params = {**common, "Mode": "match", "Games": 1000, "Hash": 64, "Threads": 1,
                  "BaseMs": 10000, "IncrementMs": 100,
                  "RunFile": str(REPO / "tools" / "colosseum" / "match-fixed-ltc.toml"), **pair}
    elif run == "4t":
        params = {**common, "Mode": "match", "Games": 1000, "Hash": 512, "Threads": 4,
                  "BaseMs": 3000, "IncrementMs": 30, "Placement": "sibling-free", **pair}
    else:
        for target in TARGETS:
            check_binary(pathlib.Path(target[1]), target[2])
        head = ("Basilisk 1.11.0 candidate", str(HEAD["path"]), HEAD["sha"], 3400, "Threads", [])
        engines = [head] + TARGETS
        # Foreign games earn the runner its 1% fault cap (colosseum.ps1 checks the value).
        extra = gauntlet_args(engines, 200) + ["--max-engine-faults", "16"]
        params = {**common, "ExpectRevision": HEAD["revision"], "Mode": "gauntlet", "Hash": 512, "Threads": 4, "BaseMs": 3000, "IncrementMs": 30,
                  "Placement": "sibling-free", "ExpectBench": [HEAD["bench"]] + [-1] * len(TARGETS),
                  "ExpectSha256": [e[2] for e in engines], "ThreadOptionNames": [e[4] for e in engines],
                  "ExtraArgs": extra}
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in params.items():
        parts.append(f"-{key} {ps(value)}")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE", run_dir


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run", required=True, choices=sorted(SEEDS))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        check_head()
        check_base()
        text, run_dir = command(args.run, args.dry_run)
        if not args.dry_run and (run_dir / "result.json").exists():
            raise Refused(f"STALE RESULT - {run_dir} holds a finished run; it is not extended or replaced.")
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(powershell() + [text], cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
