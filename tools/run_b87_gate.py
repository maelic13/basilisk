#!/usr/bin/env python3
"""Frozen launcher for B.8.7's repair gate (BAS-S64): nodes counted as
Stockfish counts them, once per move made.

SPRT on the repair bracket [-5,5] nElo (sprt-repair.toml) at 3+0.03, cap
4,000 pairs, 1T, Hash 64, paired reversed UHO openings, no adjudication,
tablebases off, seed 87010. Arm A is the count at 838a3c6, arm B the head
before it at 01aa08e (a b82-head pool build); both final-PGO PEXT release
builds pinned by SHA-256. The arms are two revisions, which colosseum.ps1's
single -ExpectRevision cannot name, so this launcher checks each arm's
manifest (revision, clean tree, bench, hash) before the wrapper checks the
rest.

A finished gate (result.json) is never extended or replaced: the launcher exits
2. An interrupted one has only its checkpoint, and the same command resumes it.

Usage:
  python tools/run_b87_gate.py [--dry-run]
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b66_games import Refused, powershell, sha256  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
COUNT = {"path": ENGINES / "basilisk-b87-count-pext-pgo.exe",
         "sha": "A3A9EEB393F07173943074C4A64C2347D84E2DD711DEA5B48BE7950C085756EA",
         "bench": 4688537, "revision": "838a3c691b67384289f97d2ea0135ea33cba0c4d"}
BASE = {"path": ENGINES / "basilisk-b82-head-pgo1-pext-pgo.exe",
        "sha": "C9FD373C00F52FF47726DF066212D29A688BE4E2FAEF1094174859C43E99CC40",
        "bench": 4247965, "revision": "01aa08ef9df3f8d3448e57316b10574708a5b976"}
RUN_DIR = REPO / "tools" / "results" / "b87-gate"


def check_manifest(engine: dict) -> None:
    manifest = engine["path"].with_suffix(".manifest.txt")
    if not manifest.exists():
        raise Refused(f"PROVENANCE MISMATCH - no manifest beside {engine['path']}.")
    fields = {}
    for line in manifest.read_text(encoding="utf-8", errors="replace").splitlines():
        key, sep, value = line.partition(":")
        if sep:
            fields[key.strip()] = value.strip()
    want = {"revision": engine["revision"], "dirty_diff": "clean", "bench": str(engine["bench"]),
            "binary_sha256": engine["sha"]}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {manifest.name} records {key} {fields.get(key)!r}, not {value!r}.")


def wrapper_command(dry_run: bool) -> str:
    parts = [
        f"& '{REPO / 'tools' / 'colosseum.ps1'}'",
        "-Mode sprt", "-Bracket repair", "-MaxPairs 4000", "-Hash 64", "-Threads 1",
        "-MarginMs 20", "-BaseMs 3000", "-IncrementMs 30", "-Seed 87010",
        f"-EngineA '{COUNT['path']}'", "-NameA 'DonorCount'",
        f"-EngineB '{BASE['path']}'", "-NameB 'Head'",
        f"-ExpectBench {COUNT['bench']},{BASE['bench']}",
        f"-ExpectSha256 '{COUNT['sha']}','{BASE['sha']}'",
        f"-Dir '{RUN_DIR}'",
    ]
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        if not args.dry_run and (RUN_DIR / "result.json").exists():
            raise Refused(f"STALE RESULT - {RUN_DIR} holds a finished gate; it is not extended or replaced.")
        for engine in (COUNT, BASE):
            if not engine["path"].exists():
                raise Refused(f"PROVENANCE MISMATCH - no binary at {engine['path']}.")
            actual = sha256(engine["path"])
            if actual != engine["sha"]:
                raise Refused(f"PROVENANCE MISMATCH - {engine['path']} is {actual}, not {engine['sha']}.")
            check_manifest(engine)
        command = powershell() + [wrapper_command(args.dry_run)]
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(command, cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
