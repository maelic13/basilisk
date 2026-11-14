#!/usr/bin/env python3
"""Frozen launcher for B.7.1's repair gate (BAS-S63): a return to the root
counts as a repetition only when it recurs.

SPRT on the repair bracket [-5,5] nElo (sprt-repair.toml) at 3+0.03, cap
4,000 pairs, 1T, Hash 64, paired reversed UHO openings, no adjudication,
tablebases off, seed 71010. Arm A is the repair at 8bfbbff, arm B the head
before it at b9cdba3; both final-PGO PEXT release builds pinned by SHA-256.
The arms are two revisions, which colosseum.ps1's single -ExpectRevision
cannot name, so this launcher checks each arm's manifest (revision, clean
tree, bench, hash) before the wrapper checks the rest.

A finished gate (result.json) is never extended or replaced: the launcher exits
2. An interrupted one has only its checkpoint, and the same command resumes it.

Usage:
  python tools/run_b71_gate.py [--dry-run]
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
FIX = {"path": ENGINES / "basilisk-b71-fix-pext-pgo.exe",
       "sha": "2B5C8B3057A9689A3B662530B1B1E770EAF115225F7462A2825A6103DB61515F",
       "bench": 4247965, "revision": "8bfbbfffe9d39ed9786a10288f788f3b4fc1d8b0"}
BASE = {"path": ENGINES / "basilisk-b71-base-pext-pgo.exe",
        "sha": "9632D49BE3C278E61CEFF1A4D9B9BA2BC54D0758A5829FC72F9069EA843EEB18",
        "bench": 4278307, "revision": "b9cdba3245653af47166b3c81552793e6d8a8ccd"}
RUN_DIR = REPO / "tools" / "results" / "b71-gate"


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
        "-MarginMs 20", "-BaseMs 3000", "-IncrementMs 30", "-Seed 71010",
        f"-EngineA '{FIX['path']}'", "-NameA 'RootRepeat'",
        f"-EngineB '{BASE['path']}'", "-NameB 'Head'",
        f"-ExpectBench {FIX['bench']},{BASE['bench']}",
        f"-ExpectSha256 '{FIX['sha']}','{BASE['sha']}'",
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
        for engine in (FIX, BASE):
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
