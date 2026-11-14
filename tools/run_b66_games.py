#!/usr/bin/env python3
"""Frozen launcher for PLAN B.6.6 (BAS-S59): the donor-form paired runs.

Each run is a 2,000-game match (1,000 paired UHO openings), one thread, Hash
64, no adjudication, tablebases off, on the Colosseum match-fixed policy
(match-fixed-ltc for 10+0.1), on
one Tune build: arm A plays the donor's form by UCI option, arm B the
defaults, so a positive result favours the donor's form.

  TtCap     CoreTtCutoffBonusCap=1 at 3+0.03
  TtCap10   CoreTtCutoffBonusCap=1 at 10+0.1
  Razor     CoreRazorDonor=1 at 3+0.03
  Key50     CoreTtKey50=1 at 3+0.03
  Kings     CoreCorrNonPawnKings=1 at 3+0.03

A run refuses unless its arm passed BAS-S59's vetoes, read from the floors
summary before any game (`tools/results/b66-20261112/floors/summary.json`):
the 25-budget conversion floor PASS, the correctness suite and the wrong-bishop
draws passing, and WAC at 100,000 nodes no more than 5 below the head's.
A refusal exits 2 and plays nothing, so a chain of runs moves on to the next.

Usage:
  python tools/run_b66_games.py TtCap [--dry-run]
  python tools/run_b66_games.py --check-vetoes TtCap [--floors FILE]
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINE = REPO / "tools" / "test_engines" / "basilisk-b66-pext-tune-pgo.exe"
SHA256 = "9AE3C8C98FA1653D3CDF3ED94ADBC960A8DFFD108D4387220EB8A0B0AEE1F649"
BENCH = 3783135
REVISION = "1b7256abf375e0b39508a2cd0ebd3a4994837b23"
FLOORS = REPO / "tools" / "results" / "b66-20261112" / "floors" / "summary.json"
WAC_MARGIN = 5

RUNS = {
    "TtCap":   {"arm": "ttcap", "option": "CoreTtCutoffBonusCap=1", "seed": 66001, "base": 3000, "inc": 30},
    "TtCap10": {"arm": "ttcap", "option": "CoreTtCutoffBonusCap=1", "seed": 66002, "base": 10000, "inc": 100,
                "run_file": "match-fixed-ltc.toml"},
    "Razor":   {"arm": "razor", "option": "CoreRazorDonor=1",       "seed": 66003, "base": 3000, "inc": 30},
    "Key50":   {"arm": "key50", "option": "CoreTtKey50=1",          "seed": 66004, "base": 3000, "inc": 30},
    "Kings":   {"arm": "kings", "option": "CoreCorrNonPawnKings=1", "seed": 66005, "base": 3000, "inc": 30},
}


class Refused(Exception):
    pass


def veto_reasons(summary: dict, arm: str) -> list[str]:
    """Every registered veto the arm fails; empty when it may play."""
    head, row = summary.get("head"), summary.get(arm)
    if not head or not row:
        return [f"no floors read for {'the head' if not head else arm}"]
    reasons = []
    for name, r in (("head", head), (arm, row)):
        if r.get("build_exit") != 0:
            reasons.append(f"{name} did not build")
    if reasons:
        return reasons
    if row.get("bench13") is None:
        reasons.append(f"{arm} has no bench read")
    if head.get("bench13") != BENCH:
        reasons.append(f"the head's bench {head.get('bench13')} is not {BENCH}")
    if row.get("test_endgames_exit") != 0:
        reasons.append(f"{arm} fails the correctness suite or the wrong-bishop draws")
    if row.get("floors_grid_exit") != 0 or "floors grid: PASS" not in row.get("floors_grid_tail", []):
        reasons.append(f"{arm} fails the conversion floor")
    for name, r in (("head", head), (arm, row)):
        if r.get("wac_records") != 300 or r.get("wac_nodes_exit") != 0:
            reasons.append(f"{name}'s WAC read is incomplete")
    if not reasons and row["wac_100k"] < head["wac_100k"] - WAC_MARGIN:
        reasons.append(f"{arm}'s WAC {row['wac_100k']} is more than {WAC_MARGIN} below the head's "
                       f"{head['wac_100k']}")
    return reasons


def check_vetoes(name: str, floors: pathlib.Path) -> None:
    if not floors.exists():
        raise Refused(f"VETO - {floors} does not exist: the floors have not been read.")
    reasons = veto_reasons(json.loads(floors.read_text(encoding="utf-8")), RUNS[name]["arm"])
    if reasons:
        raise Refused(f"VETO - {name} does not play: " + "; ".join(reasons) + ".")


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def powershell() -> list[str]:
    """The Colosseum wrapper is PowerShell (Windows today): pwsh where it exists."""
    exe = shutil.which("pwsh") or shutil.which("powershell")
    if not exe:
        raise Refused("no PowerShell to start tools/colosseum.ps1 on this host.")
    return [exe, "-NoProfile", "-NonInteractive", "-Command"]


def wrapper_command(name: str, dry_run: bool) -> str:
    r = RUNS[name]
    run_dir = REPO / "tools" / "results" / f"b66-{name}"
    parts = [
        f"& '{REPO / 'tools' / 'colosseum.ps1'}'",
        "-Mode match", "-Games 2000", "-Hash 64", "-Threads 1", "-MarginMs 20",
        f"-BaseMs {r['base']}", f"-IncrementMs {r['inc']}",
        f"-EngineA '{ENGINE}'", f"-NameA 'Donor{name}'",
        f"-EngineB '{ENGINE}'", "-NameB 'Defaults'",
        f"-OptionsA '{r['option']}'",
        f"-Seed {r['seed']}",
        f"-ExpectRevision '{REVISION}'",
        f"-ExpectBench {BENCH},{BENCH}",
        f"-ExpectSha256 '{SHA256}','{SHA256}'",
        f"-Dir '{run_dir}'",
    ]
    if "run_file" in r:
        parts.append(f"-RunFile '{REPO / 'tools' / 'colosseum' / r['run_file']}'")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("run", choices=sorted(RUNS))
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--check-vetoes", action="store_true",
                        help="apply the vetoes and stop (for testing them on synthetic summaries)")
    parser.add_argument("--floors", type=pathlib.Path, default=FLOORS)
    args = parser.parse_args()
    try:
        check_vetoes(args.run, args.floors)
        if args.check_vetoes:
            print(f"VETOES OK - {args.run} may play.")
            return 0
        actual = sha256(ENGINE)
        if actual != SHA256:
            raise Refused(f"PROVENANCE MISMATCH - {ENGINE} is {actual}, not {SHA256}.")
        command = powershell() + [wrapper_command(args.run, args.dry_run)]
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(command, cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
