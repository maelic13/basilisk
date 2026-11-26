#!/usr/bin/env python3
"""Frozen launcher for C.3.4's paired fit reads against the rebuilt seed: BAS-E63 and BAS-E64.

The contract is analysis/c31_rebuild_2026-11-25.md (§11) and the registrations
docs/experiments/BAS-E63.md and BAS-E64.md. Each read is a two-engine Colosseum
gauntlet of 1,000 cycles (2,000 games) at `3+0.03`, margin 20 ms, 1T, Hash 64,
UHO `UHO_Lichess_4852_v1.epd` in random order, paired colour-reversed openings,
concurrency 14, no adjudication, no tablebases, zero permitted faults, every
binary pinned by SHA-256 and clean manifest. Both reads share seed 210006 so
both fitted arms face the same 1,000 openings against the rebuilt seed (second
arm fixed at 1500; first arm's rating less 1500 is the read's Elo):

  e63  BAS-E63: one-k Run 2 baked fit against the rebuilt seed (902c681 vs 7d68e25)
  e64  BAS-E64: k-phase Run 2 baked fit against the rebuilt seed (59de721 vs 7d68e25)

  all: e63, then e64. A finished read (result.json) is skipped when running all;
  naming a finished read refuses to overwrite or extend it. An interrupted one
  resumes from its checkpoint with the same command.

--smoke plays two cycles per read into tools/results/c34-smoke-<read>, read
for its exit only.

Usage:
  python tools/run_c34_fit_reads.py --read <e63|e64|all> [--dry-run | --smoke]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b111_reads import check_binary, gauntlet_args, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
RESULTS = REPO / "tools" / "results"
CYCLES = 1000
SMOKE_CYCLES = 2
SHARED_SEED = 210006

SEED = {
    "path": ENGINES / "basilisk-c34-on-pext-pgo.exe",
    "sha": "325B660438FF44F4B8EB650114CA4D774A7AB6A0E11E817BB081BAEAC264AAF9",
    "bench": 3717775,
    "revision": "7d68e25b3174da34cdfe00035bc85a15bf0bd505",
}
ONEK = {
    "path": ENGINES / "basilisk-c34-onek-r2-pext-pgo.exe",
    "sha": "F1D0178043493B8C9DB4972B9FA1BD0122B4E94C0BEA70A6DF5446EE5657AD72",
    "bench": 3882274,
    "revision": "902c681e5f6004ebbe1affee71ca657ec0278259",
}
KPHASE = {
    "path": ENGINES / "basilisk-c34-kphase-r2-pext-pgo.exe",
    "sha": "AC206511DF91F19048BE29BCBACD8EA1C29AB50D07983A9239A7A952662E2BB1",
    "bench": 3880333,
    "revision": "59de7211e7f528456716f3dc5bd6eff1e7e4a235",
}

READS = {
    "e63": {
        "seed": SHARED_SEED,
        "arms": [("Basilisk one-k fit", ONEK), ("Basilisk rebuilt seed", SEED)],
    },
    "e64": {
        "seed": SHARED_SEED,
        "arms": [("Basilisk k-phase fit", KPHASE), ("Basilisk rebuilt seed", SEED)],
    },
}
ORDER = ["e63", "e64"]


def run_dir(read: str, smoke: bool) -> pathlib.Path:
    return RESULTS / (f"c34-smoke-{read}" if smoke else f"c34-{read}")


def label(name: str, smoke: bool) -> str:
    return name + (" smoke" if smoke else "")


def manifest_fields(path: pathlib.Path) -> dict[str, str]:
    manifest = path.with_suffix(".manifest.txt")
    if not manifest.exists():
        raise Refused(f"PROVENANCE MISMATCH - no manifest beside {path.name}.")
    fields = {}
    for line in manifest.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        key, sep, value = line.partition(":")
        if sep:
            fields[key.strip()] = value.strip()
    return fields


def check_arm(arm: dict) -> str:
    """Verify binary SHA-256 and clean manifest provenance; return compiler string."""
    check_binary(arm["path"], arm["sha"])
    fields = manifest_fields(arm["path"])
    want = {
        "revision": arm["revision"],
        "dirty_diff": "clean",
        "bench": str(arm["bench"]),
        "binary_sha256": arm["sha"],
        "flavor": "pext-pgo-release",
        "arm_option": "EVAL_REBUILD",
        "arm_state": "ON",
    }
    manifest_name = arm["path"].with_suffix(".manifest.txt").name
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(
                f"PROVENANCE MISMATCH - {manifest_name} records {key} {fields.get(key)!r}, not {value!r}."
            )
    compiler = fields.get("compiler", "")
    if not compiler:
        raise Refused(f"PROVENANCE MISMATCH - {manifest_name} records no compiler.")
    return compiler


def check_read(read: str) -> None:
    compilers = []
    summaries = []
    for name, arm in READS[read]["arms"]:
        compilers.append(check_arm(arm))
        summaries.append(f"{arm['revision'][:7]} bench {arm['bench']}")
    if len(set(compilers)) != 1:
        raise Refused(f"COMPILER MISMATCH - arms built with different compilers: {compilers}.")
    print(f"Arms: hashes, manifests and compiler verified ({'; '.join(summaries)}).")


def params(read: str, smoke: bool) -> dict:
    spec = READS[read]
    engines = [
        (label(name, smoke), str(arm["path"]), arm["sha"], 1500, "Threads", [])
        for name, arm in spec["arms"]
    ]
    extra = gauntlet_args(engines, SMOKE_CYCLES if smoke else CYCLES) + ["--max-engine-faults", "0"]
    return {
        "Mode": "gauntlet",
        "Hash": 64,
        "Threads": 1,
        "BaseMs": 3000,
        "IncrementMs": 30,
        "MarginMs": 20,
        "Seed": spec["seed"],
        "Dir": str(run_dir(read, smoke)),
        "Concurrency": 4 if smoke else 14,
        "OpponentFaultRatePercent": 0,
        "ExpectBench": [arm["bench"] for _, arm in spec["arms"]],
        "ExpectSha256": [arm["sha"] for _, arm in spec["arms"]],
        "ThreadOptionNames": ["Threads", "Threads"],
        "ExtraArgs": extra,
    }


def command(read: str, dry_run: bool, smoke: bool) -> str:
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in params(read, smoke).items():
        parts.append(f"-{key} {ps(value)}")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE"


def latest_dry_run(read: str, smoke: bool) -> pathlib.Path:
    name = run_dir(read, smoke).name
    found = sorted(RESULTS.glob(f"colosseum_*_{name}_*.dry-run.json"), key=lambda p: p.stat().st_mtime)
    if not found:
        raise Refused(f"NO LAUNCH RECORD - colosseum wrote no dry-run record for {name} in {RESULTS}.")
    return found[-1]


def launched_arms(record) -> dict[str, dict[str, str]]:
    found = {}
    if isinstance(record, dict):
        if isinstance(record.get("label"), str) and isinstance(record.get("options"), dict):
            found[record["label"]] = {
                k: str(v.get("value") if isinstance(v, dict) else v)
                for k, v in record["options"].items()
            }
        for value in record.values():
            found.update(launched_arms(value))
    elif isinstance(record, list):
        for value in record:
            found.update(launched_arms(value))
    return found


def check_launch_record(read: str, smoke: bool) -> None:
    path = latest_dry_run(read, smoke)
    arms = launched_arms(json.loads(path.read_text(encoding="utf-8-sig"))["resolved_configuration"])
    want = {label(name, smoke): {"hash": "64", "threads": "1"} for name, _ in READS[read]["arms"]}
    got = {name: {k.lower(): v.lower() for k, v in options.items()} for name, options in arms.items()}
    if got != want:
        raise Refused(f"LAUNCH RECORD - {path.name} gives the arms {got}, not {want}.")
    print(f"Launch record {path.name}: each arm carries exactly Hash 64 and Threads 1.")


def play(read: str, dry_run: bool, smoke: bool) -> int:
    if (run_dir(read, smoke) / "result.json").exists():
        if dry_run:
            print(f"{read}: finished; nothing to dry-run.")
            return 0
        raise Refused(f"STALE RESULT - {run_dir(read, smoke)} holds a finished read; it is not extended or replaced.")
    check_read(read)
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
