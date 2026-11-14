#!/usr/bin/env python3
"""Frozen launcher for PLAN B.6's gate (BAS-S62): the B.6.7 bake against the head
before it.

SPRT [0,3] nElo (sprt-default.toml) at 3+0.03, cap 6,400 pairs, 1T, Hash 64,
paired reversed UHO openings, no adjudication, tablebases off, seed 67010. Arm
A is the baked release build at 6a9f81f, arm B the release build of 0f30bf1,
the commit before the bake; both final-PGO and pinned by SHA-256. The two arms
are two revisions, which colosseum.ps1's single -ExpectRevision cannot name,
so this launcher checks each arm's manifest (revision, clean tree, bench)
before the wrapper checks the rest.

A finished gate (result.json) is never extended or replaced: the launcher exits
2. An interrupted one has only its checkpoint, and the same command resumes it.

Usage:
  python tools/run_b67_sprt.py [--dry-run]
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
ON = {"path": ENGINES / "basilisk-b67s-on-pext-pgo.exe",
      "sha": "F43338F99B3693BACC9A18F7F2BE97C21BCF9C132964791FE5618151294DD1E7",
      "bench": 4278307, "revision": "6a9f81f42c585b2dfc622cca39210e01dc1b4bbe"}
OFF = {"path": ENGINES / "basilisk-b67s-off-pext-pgo.exe",
       "sha": "8C93E4F1DFDB687E7F0CA39FE9FF5E96169427BBC3C87818FD2FBDF82BB4D3B6",
       "bench": 3783135, "revision": "0f30bf1d5d6aaf6c5b9dc08fcb5e5e8c1ac9df88"}
RUN_DIR = REPO / "tools" / "results" / "b67-sprt"


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
        "-Mode sprt", "-MaxPairs 6400", "-Hash 64", "-Threads 1", "-MarginMs 20",
        "-BaseMs 3000", "-IncrementMs 30", "-Seed 67010",
        f"-EngineA '{ON['path']}'", "-NameA 'Baked'",
        f"-EngineB '{OFF['path']}'", "-NameB 'Head'",
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
            check_manifest(engine)
        command = powershell() + [wrapper_command(args.dry_run)]
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(command, cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
