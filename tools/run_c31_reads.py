#!/usr/bin/env python3
"""Frozen launcher for C.3.1's transplant reads (BAS-O15).

The contract is analysis/c31_transplant_2026-11-25.md, §5 as amended. Every
read is a two-engine Colosseum gauntlet of 1,500 cycles (3,000 games) at
`3+0.03`, margin 20 ms, 1T, Hash 64, UHO in random order, paired
colour-reversed openings, seed 210003, concurrency 14, no adjudication, no
tablebases, zero permitted faults. Both engines are one oracle binary, pinned
by SHA-256, with `EvalMatchedCost=true` on both arms; the second arm is fixed
at 1500 and the first arm's rating less 1500 is the read's Elo.

  t40-b   T(40), passers and king safety transplanted, against Basilisk's evaluation  (g)
  d-t40   the donor's evaluation against T(40)                                        (s)
  d-t127  the donor's evaluation against T(127), all seven transplanted                (c)

  all: that order. A finished read is skipped; the first failure stops the run.

Before any read: the binary's hash; the six zero-game checks' record
(tools/diag/c31_checks.py's tools/results/c31-checks/checks.json) must be a
PASS for this binary; and checks 1, 2, 3 and 6 run again. Colosseum refuses
an engine that does not advertise an option it is given, and after its dry
run this launcher refuses a launch record in which an arm does not carry
exactly its options.

A finished read (result.json) is never extended or replaced. An interrupted
one has only its checkpoint, and the same command resumes it.

--smoke plays two cycles per read under other arm names into
tools/results/c31-smoke-<read>, read for its exit only.

Usage:
  python tools/run_c31_reads.py --read <name|all> [--dry-run | --smoke]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent / "diag"))
from run_b111_reads import check_binary, gauntlet_args, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402
import c31_checks  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
RESULTS = REPO / "tools" / "results"
CHECKS = RESULTS / "c31-checks" / "checks.json"
CYCLES = 1500
SMOKE_CYCLES = 2
SEED = 210003

# The oracle with tools/oracle/oracle-eval-ablate.patch and
# tools/oracle/oracle-eval-transplant.patch, around src/ of 3d2d8f0
# (byte-identical to e56f1e1's, the accepted head's evaluation).
ORACLE = {"path": ENGINES / "oracle-c31-3d2d8f0-transplant.exe",
          "sha": "ECF6C8BD71BB011899F0AFCBF5431C2100D500C8DE74A18BA11CD87FC37F1C4D"}


def arm_options(hce: bool, transplant: int) -> dict[str, str]:
    """Every arm states every evaluation option, so no arm rides on a default."""
    return {c31_checks.HCE: "true" if hce else "false", c31_checks.ABLATE: "0",
            c31_checks.TRANSPLANT: str(transplant), c31_checks.MG_SCALE: str(c31_checks.DEFAULT_MG),
            c31_checks.EG_SCALE: str(c31_checks.DEFAULT_EG), c31_checks.MATCHED: "true"}


ARMS = {"T40": ("Oracle T40", arm_options(True, 40)),
        "T127": ("Oracle T127", arm_options(True, 127)),
        "B": ("Oracle Basilisk eval", arm_options(True, 0)),
        "D": ("Oracle 9587 eval", arm_options(False, 0))}
READS = {"t40-b": ("T40", "B"), "d-t40": ("D", "T40"), "d-t127": ("D", "T127")}
ORDER = ["t40-b", "d-t40", "d-t127"]


def run_dir(read: str, smoke: bool) -> pathlib.Path:
    return RESULTS / (f"c31-smoke-{read}" if smoke else f"c31-{read}")


def label(arm: str, smoke: bool) -> str:
    return ARMS[arm][0] + (" smoke" if smoke else "")


def check_oracle() -> None:
    """The pinned binary, its six-check record, and the cheap checks again."""
    check_binary(ORACLE["path"], ORACLE["sha"])
    if not CHECKS.exists():
        raise Refused(f"NO CHECKS - run: python tools/diag/c31_checks.py --oracle {ORACLE['path']} --out {CHECKS}")
    record = json.loads(CHECKS.read_text(encoding="utf-8"))
    if record.get("oracle_sha256") != ORACLE["sha"] or record.get("verdict") != "PASS" or "5" not in record:
        raise Refused(f"CHECKS - {CHECKS} is not a full PASS for {ORACLE['sha']}: {record.get('verdict')!r}.")
    try:
        c31_checks.wire_checks(ORACLE["path"])
    except c31_checks.Failed as failure:
        raise Refused(f"WIRE CHECK - {failure}") from failure
    print("Oracle: hash pinned, checks 1 to 6 on record, checks 1, 2, 3 and 6 passed again.")


def params(read: str, smoke: bool) -> dict:
    first, second = READS[read]
    engines = [(label(arm, smoke), str(ORACLE["path"]), ORACLE["sha"], 1500, "Threads",
                [f"{name}={value}" for name, value in ARMS[arm][1].items()]) for arm in (first, second)]
    extra = gauntlet_args(engines, SMOKE_CYCLES if smoke else CYCLES) + ["--max-engine-faults", "0"]
    return {"Mode": "gauntlet", "Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30, "MarginMs": 20,
            "Seed": SEED, "Dir": str(run_dir(read, smoke)), "Concurrency": 4 if smoke else 14,
            "OpponentFaultRatePercent": 0, "ExpectBench": [-1, -1], "ExpectSha256": [e[2] for e in engines],
            "ThreadOptionNames": [e[4] for e in engines], "ExtraArgs": extra}


def command(read: str, dry_run: bool, smoke: bool) -> str:
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in params(read, smoke).items():
        parts.append(f"-{key} {ps(value)}")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE"


def latest_dry_run(read: str, smoke: bool) -> pathlib.Path:
    """colosseum.ps1 writes each dry run's record beside the results, named by the run directory."""
    name = run_dir(read, smoke).name
    found = sorted(RESULTS.glob(f"colosseum_*_{name}_*.dry-run.json"), key=lambda p: p.stat().st_mtime)
    if not found:
        raise Refused(f"NO LAUNCH RECORD - colosseum wrote no dry-run record for {name} in {RESULTS}.")
    return found[-1]


def launched_arms(record) -> dict[str, dict[str, str]]:
    """Every labelled launch in a resolved configuration with its option values, wherever the schema nests it."""
    found = {}
    if isinstance(record, dict):
        if isinstance(record.get("label"), str) and isinstance(record.get("options"), dict):
            found[record["label"]] = {k: str(v.get("value") if isinstance(v, dict) else v)
                                      for k, v in record["options"].items()}
        for value in record.values():
            found.update(launched_arms(value))
    elif isinstance(record, list):
        for value in record:
            found.update(launched_arms(value))
    return found


def check_launch_record(read: str, smoke: bool) -> None:
    """Each arm must carry exactly its evaluation options, and nothing else but Hash and Threads."""
    path = latest_dry_run(read, smoke)
    arms = launched_arms(json.loads(path.read_text(encoding="utf-8-sig"))["resolved_configuration"])
    want = {label(arm, smoke): {**ARMS[arm][1], "Hash": "64", "Threads": "1"} for arm in READS[read]}
    got = {name: {k: v.lower() for k, v in options.items()} for name, options in arms.items()}
    want = {name: {k: v.lower() for k, v in options.items()} for name, options in want.items()}
    if got != want:
        raise Refused(f"LAUNCH RECORD - {path.name} gives the arms {got}, not {want}.")
    print(f"Launch record {path.name}: each arm carries exactly its options.")


def play(read: str, dry_run: bool, smoke: bool) -> int:
    if (run_dir(read, smoke) / "result.json").exists():
        if dry_run:
            print(f"{read}: finished; nothing to dry-run.")
            return 0
        raise Refused(f"STALE RESULT - {run_dir(read, smoke)} holds a finished read; it is not extended or replaced.")
    status = subprocess.run(powershell() + [command(read, dry_run, smoke)], cwd=REPO).returncode
    if status == 0 and dry_run:
        check_launch_record(read, smoke)
    return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--read", required=True, choices=ORDER + ["all"])
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    reads = ORDER if args.read == "all" else [args.read]
    try:
        check_oracle()
        for read in reads:
            if args.read == "all" and not args.dry_run and (run_dir(read, args.smoke) / "result.json").exists():
                print(f"{read}: finished; skipped.")
                continue
            print(f"== {read}{' (smoke)' if args.smoke else ''}")
            status = play(read, args.dry_run, args.smoke)
            if status != 0:
                print(f"{read} stopped with exit {status}; the run stops here.", file=sys.stderr)
                return status
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
