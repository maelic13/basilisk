#!/usr/bin/env python3
"""B.7.2's idle-host reads: the board microbenchmark beside Rarog's and
Reckless's boards, then the profile of the head at `bench 13`.

1. tools/diag/board_micro_run.py times Basilisk's microbenchmark
   (tests/board_micro.cpp, all its columns) and the other engines'
   cross-engine-board-v2 harnesses (tools/board_compare/), alternating the
   three in five rounds, one process at a time on one logical CPU. Output:
   tools/results/b72-micro/.
2. tools/profile.ps1 samples the head's Profile build over `bench 13`, five
   repeats, and attributes it to regions by inline chains. Output:
   tools/results/b72-profile/.

Both refuse a busy host. Every binary is checked against its recorded hash
first; a read already on disk is never overwritten (exit 2).

    python tools/run_b72_reads.py [--dry-run]
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
BIN = REPO / "tools" / "results" / "board-compare" / "bin"
MICRO = {
    "basilisk": REPO / "build" / "release" / "board_micro_bench.exe",
    "rarog": BIN / "rarog-board-v2.exe",
    "reckless": BIN / "reckless-board-v2.exe",
}
PROFILE_ENGINE = REPO / "tools" / "test_engines" / "basilisk-b72-head-prof-pext-profile-pgo.exe"
PROFILE_SHA = "692422C449C763CF904BAEDB45AAD8EC5D87ACAC4BA37519FA31DDA355D7BF42"
BENCH = 4247965
MICRO_OUT = REPO / "tools" / "results" / "b72-micro"
PROFILE_OUT = REPO / "tools" / "results" / "b72-profile"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=REPO, capture_output=True, text=True).stdout.strip()


def recorded_sha(exe: Path) -> str:
    manifest = exe.with_suffix(".manifest.txt")
    for line in manifest.read_text(encoding="utf-8").splitlines():
        if line.startswith("binary_sha256:"):
            return line.split(":", 1)[1].strip()
    raise SystemExit(f"{manifest} records no binary hash")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dry-run", action="store_true", help="check every input, run nothing")
    args = parser.parse_args()

    for out in (MICRO_OUT, PROFILE_OUT):
        if out.exists() and any(out.iterdir()):
            print(f"{out} already holds a read; it is never overwritten", file=sys.stderr)
            return 2
    if git("status", "--porcelain", "--untracked-files=no"):
        print("the checkout has uncommitted changes; the reads name a clean revision", file=sys.stderr)
        return 2
    for name, exe in MICRO.items():
        if not exe.is_file():
            print(f"no {name} harness at {exe}", file=sys.stderr)
            return 2
        if name != "basilisk" and sha256(exe) != recorded_sha(exe):
            print(f"{exe} does not match its manifest", file=sys.stderr)
            return 2
    if sha256(PROFILE_ENGINE) != PROFILE_SHA:
        print(f"{PROFILE_ENGINE} is not the recorded Profile build", file=sys.stderr)
        return 2
    for exe in MICRO.values():
        pre = subprocess.run([str(exe), str(REPO / "tests" / "data" / "board_micro_positions.fen"),
                              "--preflight-only"], capture_output=True, text=True)
        if pre.returncode != 0 or "preflight: PASS" not in pre.stdout:
            print(f"{exe} fails its preflight:\n{pre.stdout}{pre.stderr}", file=sys.stderr)
            return 2
    head = git("rev-parse", "HEAD")
    micro = [sys.executable, str(REPO / "tools" / "diag" / "board_micro_run.py"),
             "--out", str(MICRO_OUT), "--rounds", "5", "--samples", "15", "--sample-ms", "200",
             "--revision", f"basilisk={head}"]
    for name, exe in MICRO.items():
        micro += ["--binary", f"{name}={exe}"]
    profile = ["pwsh", "-NoProfile", "-File", str(REPO / "tools" / "profile.ps1"),
               "-Engine", str(PROFILE_ENGINE), "-ExpectBench", str(BENCH),
               "-Out", str(PROFILE_OUT), "-Repeats", "5"]
    if args.dry_run:
        print("inputs verified; would run:\n  " + " ".join(micro) + "\n  " + " ".join(profile))
        return 0
    code = subprocess.run(micro, cwd=REPO).returncode
    if code != 0:
        print(f"the microbenchmark read exited {code}; the profile was not taken", file=sys.stderr)
        return code
    return subprocess.run(profile, cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
