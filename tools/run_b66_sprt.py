#!/usr/bin/env python3
"""Frozen launcher for PLAN B.6.6's gate (BAS-S60): the two adopted donor forms
against the head.

SPRT [0,3] nElo (sprt-default.toml) at 3+0.03, cap 5,000 pairs, 1T, Hash 64,
paired reversed UHO openings, no adjudication, tablebases off, seed 66010. Arm
A is the B6_DONOR_FORMS=ON release build, arm B the OFF build of the same
revision; both final-PGO and pinned by SHA-256.

A finished gate (result.json) is never extended or replaced: the launcher exits
2. An interrupted one has only its checkpoint, and the same command resumes it.

Usage:
  python tools/run_b66_sprt.py [--dry-run]
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
REVISION = "65448d06bc0df3679cc887c7724e293b0bd0277a"
ON = {"path": ENGINES / "basilisk-b66s-on-pext-pgo.exe",
      "sha": "A2BF3537F6EBECFF761765AF9A10E73502E4E5DB528DA1982993318BE9B77F98", "bench": 3268437}
OFF = {"path": ENGINES / "basilisk-b66s-off-pext-pgo.exe",
       "sha": "07BB73F32CBB7D6942C09A0CA93B1CD7646069BF40BD1AD6FD3560B6EA9FED79", "bench": 3783135}
RUN_DIR = REPO / "tools" / "results" / "b66-sprt"


def wrapper_command(dry_run: bool) -> str:
    parts = [
        f"& '{REPO / 'tools' / 'colosseum.ps1'}'",
        "-Mode sprt", "-MaxPairs 5000", "-Hash 64", "-Threads 1", "-MarginMs 20",
        "-BaseMs 3000", "-IncrementMs 30", "-Seed 66010",
        f"-EngineA '{ON['path']}'", "-NameA 'DonorForms'",
        f"-EngineB '{OFF['path']}'", "-NameB 'Head'",
        f"-ExpectRevision '{REVISION}'",
        f"-ExpectBench {ON['bench']},{OFF['bench']}",
        f"-ExpectSha256 '{ON['sha']}','{OFF['sha']}'",
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
        for engine in (ON, OFF):
            if not engine["path"].exists():
                raise Refused(f"PROVENANCE MISMATCH - no binary at {engine['path']}.")
            actual = sha256(engine["path"])
            if actual != engine["sha"]:
                raise Refused(f"PROVENANCE MISMATCH - {engine['path']} is {actual}, not {engine['sha']}.")
        command = powershell() + [wrapper_command(args.dry_run)]
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(command, cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
