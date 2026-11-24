#!/usr/bin/env python3
"""Frozen launcher for C.2.10's game reads: the family meter and the frontier pair.

The contract is analysis/c210_family_order_2026-11-24.md (BAS-O14, BAS-M24).
Every read is 3,000 games at `3+0.03`, margin 20 ms, 1T, Hash 64, UHO in random
order, paired colour-reversed openings, concurrency 14, no adjudication, no
tablebases, every binary pinned by SHA-256.

  The meter reads (BAS-O14): a two-engine gauntlet of 1,500 cycles, seed
  210001 for all of them. Both engines are the oracle built with the
  evaluation-family mask (Stockfish 9587eeeb's search); the first arm runs
  `Use Basilisk HCE=false` (Stockfish's evaluation), the second `=true`
  (Basilisk's) and is fixed at 1500, both at one `EvalAblationMask`. E(mask)
  is the first arm's rating minus 1500, zero faults permitted.

    e0 (mask 0), m7, m8, m48, m64, m127, m2, m1, m4, m16, m32

  frontier (BAS-M24): a fixed match of 3,000 games, seed 210002: the head's
  final PGO PEXT build against Stockfish 16 with `Use NNUE=false` as an
  external arm B, with the wrapper's 1% fault allowance for it.

  all: the registered order, e0, m7, m8, m48, m64, frontier, m127, m2, m1, m4,
  m16, m32. A finished read is skipped; the first failure stops the run.

Wire checks refuse a read before any game: every binary's hash; the head's
manifest (revision, clean tree, bench); for a mask other than 0, the oracle's
`bench 16 1 8 default depth` must differ from mask 0's under both values of
`Use Basilisk HCE`. Colosseum refuses an engine that does not advertise an
option it is given, and after its dry run this launcher refuses a launch
record that does not carry the read's `EvalAblationMask` for both arms.

A finished read (result.json) is never extended or replaced. An interrupted
one has only its checkpoint, and the same command resumes it.

Usage:
  python tools/run_c210_reads.py --read <name|all> [--dry-run]
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
MASK_OPTION = "EvalAblationMask"
HCE_OPTION = "Use Basilisk HCE"
WIRE_BENCH = "bench 16 1 8 default depth"

# The oracle with tools/oracle/oracle-eval-ablate.patch, around src/ of
# e56f1e1 (the accepted head's evaluation).
ORACLE = {"path": ENGINES / "oracle-c29-e56f1e1-evalablate.exe",
          "sha": "CAA8B70BEBA72B84AE118F5ECB29A8803256A2DC25C836DA7334F91E5DF8FF79"}
HEAD = {"path": ENGINES / "basilisk-c210-head-pext-pgo.exe",
        "sha": "8D8DA3420ECF915A16DBA125E8E2675FA9806E33935F09770BE409FEDBBA1D14",
        "bench": 3359329, "revision": "d8793051bfb2ca184727c81a5ded35db96fee990"}
# BAS-M18's Stockfish 16 classical build.
SF16 = {"path": pathlib.Path(r"D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe"),
        "sha": "041396A4B14EC25BAE0B57F90E917A17E7FCEB1FBBAB80859EE2EC3774D25442"}

METER_SEED = 210001
FRONTIER_SEED = 210002
MASKS = {"e0": 0, "m7": 7, "m8": 8, "m48": 48, "m64": 64, "m127": 127,
         "m2": 2, "m1": 1, "m4": 4, "m16": 16, "m32": 32}
ORDER = ["e0", "m7", "m8", "m48", "m64", "frontier", "m127", "m2", "m1", "m4", "m16", "m32"]


def run_dir(read: str) -> pathlib.Path:
    return RESULTS / f"c210-{read}"


def check_head() -> None:
    check_binary(HEAD["path"], HEAD["sha"])
    manifest = HEAD["path"].with_suffix(".manifest.txt")
    if not manifest.exists():
        raise Refused(f"PROVENANCE MISMATCH - no manifest beside {HEAD['path'].name}.")
    fields = {}
    for line in manifest.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        key, sep, value = line.partition(":")
        if sep:
            fields[key.strip()] = value.strip()
    want = {"revision": HEAD["revision"], "dirty_diff": "clean", "bench": str(HEAD["bench"]),
            "binary_sha256": HEAD["sha"]}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {manifest.name} records {key} {fields.get(key)!r}, not {value!r}.")


def oracle_bench(hce: str, mask: int) -> int:
    """The oracle's node count on a short bench at one evaluation and one mask."""
    script = (f"setoption name {HCE_OPTION} value {hce}\nsetoption name {MASK_OPTION} value {mask}\n"
              f"{WIRE_BENCH}\nquit\n")
    done = subprocess.run([str(ORACLE["path"])], input=script, capture_output=True, text=True)
    match = re.search(r"Nodes searched\s*:\s*(\d+)", done.stdout + done.stderr)
    if done.returncode != 0 or not match:
        raise Refused(f"ORACLE REFUSED - no bench node count with {HCE_OPTION}={hce}, {MASK_OPTION}={mask}.")
    return int(match.group(1))


def check_oracle(mask: int) -> None:
    """The mask's wire: at a mask other than 0 both evaluations must search differently."""
    check_binary(ORACLE["path"], ORACLE["sha"])
    if mask == 0:
        return
    for hce in ("false", "true"):
        stock, masked = oracle_bench(hce, 0), oracle_bench(hce, mask)
        if stock == masked:
            raise Refused(f"DEAD WIRE - {MASK_OPTION}={mask} does not move the oracle's bench with "
                          f"{HCE_OPTION}={hce} ({stock} nodes both ways).")
        print(f"Wire live: {HCE_OPTION}={hce}, {WIRE_BENCH}: {stock} nodes at mask 0, {masked} at mask {mask}.")


def meter_params(read: str) -> dict:
    mask = MASKS[read]
    check_oracle(mask)
    engines = [("Oracle 9587 eval", str(ORACLE["path"]), ORACLE["sha"], 1500, "Threads",
                [f"{HCE_OPTION}=false", f"{MASK_OPTION}={mask}"]),
               ("Oracle Basilisk eval", str(ORACLE["path"]), ORACLE["sha"], 1500, "Threads",
                [f"{HCE_OPTION}=true", f"{MASK_OPTION}={mask}"])]
    extra = gauntlet_args(engines, CYCLES) + ["--max-engine-faults", "0"]
    return {"Mode": "gauntlet", "Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30, "MarginMs": 20,
            "Seed": METER_SEED, "Dir": str(run_dir(read)), "Concurrency": 14, "OpponentFaultRatePercent": 0,
            "ExpectBench": [-1, -1], "ExpectSha256": [e[2] for e in engines],
            "ThreadOptionNames": [e[4] for e in engines], "ExtraArgs": extra}


def frontier_params() -> dict:
    check_head()
    check_binary(SF16["path"], SF16["sha"])
    return {"Mode": "match", "Games": 3000, "Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30,
            "MarginMs": 20, "Seed": FRONTIER_SEED, "Dir": str(run_dir("frontier")), "Concurrency": 14,
            "ExpectRevision": HEAD["revision"],
            "EngineA": str(HEAD["path"]), "NameA": "Basilisk head",
            "EngineB": str(SF16["path"]), "NameB": "Stockfish 16 HCE", "OptionsB": ["Use NNUE=false"],
            "ExpectBench": [HEAD["bench"], -1], "ExpectSha256": [HEAD["sha"], SF16["sha"]]}


def command(read: str, dry_run: bool) -> str:
    params = frontier_params() if read == "frontier" else meter_params(read)
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in params.items():
        parts.append(f"-{key} {ps(value)}")
    if params["Mode"] == "match":
        parts.append("-ExternalB")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE"


def latest_dry_run(read: str) -> pathlib.Path:
    """colosseum.ps1 writes each dry run's record beside the results, named by the run directory."""
    found = sorted(RESULTS.glob(f"colosseum_*_{run_dir(read).name}_*.dry-run.json"), key=lambda p: p.stat().st_mtime)
    if not found:
        raise Refused(f"NO LAUNCH RECORD - colosseum wrote no dry-run record for {run_dir(read).name} in {RESULTS}.")
    return found[-1]


def options_in(record) -> list[dict]:
    """Every option map in a resolved configuration, wherever the schema nests it."""
    maps = []
    if isinstance(record, dict):
        if isinstance(record.get("options"), dict):
            maps.append(record["options"])
        for value in record.values():
            maps.extend(options_in(value))
    elif isinstance(record, list):
        for value in record:
            maps.extend(options_in(value))
    return maps


def option_value(options: dict, name: str):
    for key, value in options.items():
        if key.casefold() == name.casefold():
            return value.get("value") if isinstance(value, dict) else value
    return None


def check_launch_record(read: str) -> None:
    """The resolved launch must give both arms the read's mask and their evaluation switch."""
    if read == "frontier":
        return
    path = latest_dry_run(read)
    resolved = json.loads(path.read_text(encoding="utf-8-sig"))["resolved_configuration"]
    seen = [(option_value(o, HCE_OPTION), option_value(o, MASK_OPTION)) for o in options_in(resolved)
            if option_value(o, MASK_OPTION) is not None or option_value(o, HCE_OPTION) is not None]
    mask = str(MASKS[read])
    want = {("false", mask), ("true", mask)}
    got = {(str(h).lower(), str(m)) for h, m in seen}
    if got != want:
        raise Refused(f"LAUNCH RECORD - {path.name} gives the arms {sorted(got)}, not {sorted(want)}.")
    print(f"Launch record {path.name}: both arms carry {MASK_OPTION}={mask}.")


def play(read: str, dry_run: bool) -> int:
    if (run_dir(read) / "result.json").exists():
        if dry_run:
            print(f"{read}: finished; nothing to dry-run.")
            return 0
        raise Refused(f"STALE RESULT - {run_dir(read)} holds a finished read; it is not extended or replaced.")
    text = command(read, dry_run)
    status = subprocess.run(powershell() + [text], cwd=REPO).returncode
    if status == 0 and dry_run:
        check_launch_record(read)
    return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--read", required=True, choices=ORDER + ["all"])
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    reads = ORDER if args.read == "all" else [args.read]
    try:
        for read in reads:
            if args.read == "all" and not args.dry_run and (run_dir(read) / "result.json").exists():
                print(f"{read}: finished; skipped.")
                continue
            print(f"== {read}")
            status = play(read, args.dry_run)
            if status != 0:
                print(f"{read} stopped with exit {status}; the run stops here.", file=sys.stderr)
                return status
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
