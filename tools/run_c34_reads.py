#!/usr/bin/env python3
"""Frozen launcher for C.3.4's seed reads: BAS-M25 and BAS-O16.

The contract is analysis/c31_rebuild_2026-11-25.md, §11 and Amendment 7, and
the two registrations, docs/experiments/BAS-M25.md and BAS-O16.md. Every
read is a two-engine Colosseum gauntlet of 1,500 cycles (3,000 games) at
`3+0.03`, margin 20 ms, 1T, Hash 64, UHO `UHO_Lichess_4852_v1.epd` in random
order, paired colour-reversed openings, concurrency 14, no adjudication, no
tablebases, zero permitted faults, every binary pinned by SHA-256. The
second arm is fixed at 1500 and the first arm's rating less 1500 is the
read's Elo.

  m25  BAS-M25, seed 210004: the new arm (EVAL_REBUILD on, at its seed
       values) against the head (EVAL_REBUILD off), clean final PGO PEXT
       builds of one commit. Each arm's manifest must name that commit, a
       clean tree, the Release flavour, its arm and its bench fingerprint.
  o16  BAS-O16, seed 210005: one oracle built by tools/oracle/build_oracle.ps1
       around that commit's src/ with EVAL_REBUILD on (Stockfish 9587eeeb's
       search). `Use Basilisk HCE=false`, the donor's evaluation, against
       `Use Basilisk HCE=true`, the rebuilt one, `EvalAblationMask=0` on
       both. Its manifest must name the commit's src/, byte-identical.

  all: m25, then o16. A finished read is skipped; the first failure stops
  the run.

Before a read: every binary's hash and manifest, and for o16 the switch's
wire: `Use Basilisk HCE` must move the oracle's `bench 16 1 8 default
depth`. Colosseum refuses an engine that does not advertise an option it is
given, and after its dry run this launcher refuses a launch record in which
an arm does not carry exactly its options: Hash and Threads, and for o16
its two evaluation options.

A finished read (result.json) is never extended or replaced. An interrupted
one has only its checkpoint, and the same command resumes it.

--smoke plays two cycles per read under other arm names into
tools/results/c34-smoke-<read>, read for its exit only. The results table is
tools/diag/c34_table.py.

Usage:
  python tools/run_c34_reads.py --read <m25|o16|all> [--dry-run | --smoke]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b111_reads import check_binary, gauntlet_args, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
RESULTS = REPO / "tools" / "results"
CYCLES = 1500
SMOKE_CYCLES = 2
HCE_OPTION = "Use Basilisk HCE"
MASK_OPTION = "EvalAblationMask"
WIRE_BENCH = "bench 16 1 8 default depth"

# The commit both arms and the oracle are built from (its src/ is the
# engine ticket's).
REVISION = "7d68e25b3174da34cdfe00035bc85a15bf0bd505"
NEW = {"path": ENGINES / "basilisk-c34-on-pext-pgo.exe", "sha": "325B660438FF44F4B8EB650114CA4D774A7AB6A0E11E817BB081BAEAC264AAF9",
       "bench": 3717775, "arm": "ON"}
HEAD = {"path": ENGINES / "basilisk-c34-off-pext-pgo.exe", "sha": "DF4D302194AE277E7D0C8455F6E2082CD52EB49A10E5F6CF1E3BD735FF24B8A1",
        "bench": 3359329, "arm": "OFF"}
# tools/oracle/build_oracle.ps1 with oracle-eval-ablate.patch,
# oracle-eval-transplant.patch, oracle-eval-reference.patch and
# oracle-eval-rebuild.patch, in that order, around REVISION's src/.
ORACLE = {"path": ENGINES / "oracle-c34-7d68e25-rebuild.exe", "sha": "A53A9BA17F04248531F4892619354A9EF5B699F202965FF0B0FED0331EE1BD08"}

READS = {
    "m25": {"seed": 210004, "arms": [
        ("Basilisk rebuilt seed", NEW, {}),
        ("Basilisk head", HEAD, {})]},
    "o16": {"seed": 210005, "arms": [
        ("Oracle 9587 eval", ORACLE, {HCE_OPTION: "false", MASK_OPTION: "0"}),
        ("Oracle rebuilt eval", ORACLE, {HCE_OPTION: "true", MASK_OPTION: "0"})]},
}
ORDER = ["m25", "o16"]


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


def check_arm(arm: dict) -> None:
    """A Basilisk arm: its hash, and a manifest naming the commit, a clean tree, its arm and bench."""
    check_binary(arm["path"], arm["sha"])
    fields = manifest_fields(arm["path"])
    want = {"revision": REVISION, "dirty_diff": "clean", "bench": str(arm["bench"]),
            "binary_sha256": arm["sha"], "flavor": "pext-pgo-release", "arm_option": "EVAL_REBUILD",
            "arm_state": arm["arm"]}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {arm['path'].with_suffix('.manifest.txt').name} records "
                          f"{key} {fields.get(key)!r}, not {value!r}.")


def oracle_bench(hce: str) -> int:
    script = (f"setoption name {HCE_OPTION} value {hce}\nsetoption name {MASK_OPTION} value 0\n"
              f"{WIRE_BENCH}\nquit\n")
    done = subprocess.run([str(ORACLE["path"])], input=script, capture_output=True, text=True)
    match = re.search(r"Nodes searched\s*:\s*(\d+)", done.stdout + done.stderr)
    if done.returncode != 0 or not match:
        raise Refused(f"ORACLE REFUSED - no bench node count with {HCE_OPTION}={hce}.")
    return int(match.group(1))


def check_oracle() -> None:
    """The oracle: its hash, a manifest naming REVISION's src/, and a live evaluation switch."""
    check_binary(ORACLE["path"], ORACLE["sha"])
    fields = manifest_fields(ORACLE["path"])
    if f"({REVISION})" not in fields.get("basilisk_src", "") or "byte-identical" not in fields.get("basilisk_src", ""):
        raise Refused(f"PROVENANCE MISMATCH - the oracle's basilisk_src is {fields.get('basilisk_src')!r}, "
                      f"not {REVISION}'s, byte-identical.")
    if fields.get("binary_sha256", "").upper() != ORACLE["sha"]:
        raise Refused(f"PROVENANCE MISMATCH - the oracle's manifest records {fields.get('binary_sha256')!r}.")
    donor, rebuilt = oracle_bench("false"), oracle_bench("true")
    if donor == rebuilt:
        raise Refused(f"DEAD WIRE - {HCE_OPTION} does not move the oracle's {WIRE_BENCH} ({donor} nodes both ways).")
    print(f"Oracle: hash and manifest pinned; {WIRE_BENCH}: {donor} nodes with {HCE_OPTION}=false, "
          f"{rebuilt} with true.")


def check_read(read: str) -> None:
    if read == "m25":
        check_arm(NEW)
        check_arm(HEAD)
        print(f"Arms: hashes and manifests pinned ({REVISION[:10]}, clean, bench {NEW['bench']} and {HEAD['bench']}).")
    else:
        check_oracle()


def params(read: str, smoke: bool) -> dict:
    spec = READS[read]
    engines = [(label(name, smoke), str(arm["path"]), arm["sha"], 1500, "Threads",
                [f"{option}={value}" for option, value in options.items()])
               for name, arm, options in spec["arms"]]
    extra = gauntlet_args(engines, SMOKE_CYCLES if smoke else CYCLES) + ["--max-engine-faults", "0"]
    benches = [arm.get("bench", -1) for _, arm, _ in spec["arms"]]
    out = {"Mode": "gauntlet", "Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30, "MarginMs": 20,
           "Seed": spec["seed"], "Dir": str(run_dir(read, smoke)), "Concurrency": 4 if smoke else 14,
           "OpponentFaultRatePercent": 0, "ExpectBench": benches, "ExpectSha256": [e[2] for e in engines],
           "ThreadOptionNames": [e[4] for e in engines], "ExtraArgs": extra}
    if read == "m25":
        out["ExpectRevision"] = REVISION
    return out


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
    """Each arm must carry exactly its options: Hash, Threads and the read's own."""
    path = latest_dry_run(read, smoke)
    arms = launched_arms(json.loads(path.read_text(encoding="utf-8-sig"))["resolved_configuration"])
    want = {label(name, smoke): {**options, "Hash": "64", "Threads": "1"} for name, _, options in READS[read]["arms"]}
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
