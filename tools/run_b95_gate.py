#!/usr/bin/env python3
"""Frozen launcher for B.9.5's repair gate (BAS-S66): the lifecycle,
protocol and verdict repairs together against the head before them.

SPRT on the repair bracket [-5,5] nElo (sprt-repair.toml) at 3+0.03, cap
4,000 pairs, 1T, Hash 64, paired reversed UHO openings, no adjudication,
tablebases off, seed 95010. Arm A is the repaired head, arm B the head before
the repairs at 9a4a94b; both final-PGO PEXT release builds pinned by SHA-256.
The arms are two revisions, which colosseum.ps1's single -ExpectRevision
cannot name, so this launcher checks each arm's manifest (revision, clean
tree, bench, hash) before the wrapper checks the rest.

A finished gate (result.json) is never extended or replaced: the launcher exits
2. An interrupted one has only its checkpoint, and the same command resumes it.

Usage:
  python tools/run_b95_gate.py [--dry-run]
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
REPAIRED = {"path": ENGINES / "basilisk-b95-repair-pext-pgo.exe",
            "sha": "6B5C2981F031193CA80B3E4D92BA511D3E6E413DF4F7218EB012E1163E3709DF",
            "bench": 4777412, "revision": "39982c07c14620e721ce6d6604f7ff8474489609"}
BASE = {"path": ENGINES / "basilisk-b95-base-pext-pgo.exe",
        "sha": "6512294E4CAA9E5465BDE4B85D63B9DE19F08A8B83E78E78EA16E625A954C4C8",
        "bench": 4688537, "revision": "9a4a94b0358abb30adacbdf58ca8b85d5a5cbfac"}
RUN_DIR = REPO / "tools" / "results" / "b95-gate"


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
        "-MarginMs 20", "-BaseMs 3000", "-IncrementMs 30", "-Seed 95010",
        f"-EngineA '{REPAIRED['path']}'", "-NameA 'Repaired'",
        f"-EngineB '{BASE['path']}'", "-NameB 'Head'",
        f"-ExpectBench {REPAIRED['bench']},{BASE['bench']}",
        f"-ExpectSha256 '{REPAIRED['sha']}','{BASE['sha']}'",
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
        for engine in (REPAIRED, BASE):
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
