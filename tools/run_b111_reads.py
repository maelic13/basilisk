#!/usr/bin/env python3
"""Frozen launcher for B.11.1's game reads: the search programme checkpoint.

Arm A is the head (`4c0bc0e`, engine source equal to master's `4d2742f`):
Release PEXT final PGO, bench 4,777,412. Every run is 1T, Hash 64, `3+0.03`,
margin 20 ms, UHO in random order, paired colour-reversed openings, no
adjudication, no tablebases, every binary pinned by SHA-256.

  --read g0-equal  BAS-O05's protocol unchanged: a two-engine gauntlet against
                   the oracle rebuilt with the head's HCE
                   (`build_oracle.ps1 -BasiliskRevision 4d2742f`), 1,500 cycles,
                   3,000 games, concurrency 14, oracle fixed at 1500 with
                   `Use Basilisk HCE=true`, zero faults; G(0) = 1500 minus the
                   head's rating.
  --read g0-odds   RAR-O08's conditions: the same pairing as a fixed match of
                   3,000 games, concurrency 14, the oracle as an external arm B
                   at the time odds that tools/diag/bridge_price.py measured
                   (tools/results/b111-checkpoint/bridge_price.json); G(0) is
                   minus the head's Elo.
  --read pool      The 1T pool gauntlet against the four targets at their Super
                   Rating Tournament ratings (Houdini 3 3287, Critter 1.6a 3192,
                   Fritz 16 3173, Rybka 4.1 3111), 200 cycles, 400 games each.
  --read rarog     Rarog 2.6.0 as a 400-game fixed match beside the pool (an
                   external arm B; it has no rating on the pool's scale), the
                   version BAS-M15 played.
  --read rarog27   Rarog 2.7.0, Rarog's latest release (2026-11-13), the same
                   match: the official `rarog-2.7.0-windows-x64.exe`, pinned
                   by the release's published SHA-256, at its defaults.

A finished read (result.json) is never extended or replaced: the launcher
exits 2. An interrupted one has only its checkpoint, and the same command
resumes it.

Usage:
  python tools/run_b111_reads.py --read <g0-equal|g0-odds|pool|rarog|rarog27> [--dry-run]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b66_games import Refused, powershell, sha256  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
RESULTS = REPO / "tools" / "results"
HEAD = {"path": ENGINES / "basilisk-b111-head-pext-pgo.exe",
        "sha": "31BC229A6ED4C05715CEF8F44EF97866C91FE4ACE7929D8B86D8316CF12B994D",
        "bench": 4777412, "revision": "4c0bc0ea43b63c632ee5e6647eb8aac6887269b6"}
ORACLE = {"path": ENGINES / "oracle-b111-4d2742f.exe",
          "sha": "8ADEE9AE80FE0B7AF7135C766045E846824383A0EDB70241B82E4F20336C020C"}
# The price is read on the instrumented twin; its tree equals ORACLE's
# (both bench 875,469 nodes), and only ORACLE plays.
PRICE_ORACLE_SHA = "DF6FD7D31CAF9EFE16F9A75325754F49D68CE247058E627CF5834F3C394357A7"
PRICE_FILE = RESULTS / "b111-checkpoint" / "bridge_price.json"
PRICE_COMMAND = "bench 16 1 16 default depth"
# Rarog's classical-oracle clock (RAR-O08), used only by a dry run before the price exists.
DRY_RUN_ODDS = (3279, 33)
TARGETS = [
    ("Houdini 3", r"D:\chess\engines\houdini\Houdini_3_Pro_x64.exe",
     "B0E7A631083B9F5B9FF9D5976133FD1C183219C8BB8C70E9CFB58FF7761FF026", 3287, "Threads", []),
    ("Critter 1.6a", r"D:\chess\engines\Critter_1.6a_64bit.exe",
     "A8988AC94B0513B29A4F91DB9618F412B01DACCBAC0B32C2E98205D05B3E63D2", 3192, "Threads",
     ["OwnBook=false", "Tablebase Usage=Disable"]),
    ("Fritz 16", r"D:\chess\engines\rybka\Fritz 16.exe",
     "264F2EEDE4C737F938C726E3C91E5CE07460E998110CF197ABFC463DFA4622AE", 3173, "Max CPUs", []),
    ("Rybka 4.1", r"D:\chess\engines\rybka\Deep Rybka 4.1 SSE42 x64.exe",
     "15901A12179EBD773871AE556CCFC05CAB85C30D014345F6EA64B77F44DC9923", 3111, "Max CPUs", []),
]
RAROGS = {
    "rarog": {"path": pathlib.Path(r"D:\chess\engines\rarog\rarog-v2.6.0-windows-pext-pgo.exe"),
              "sha": "274EAB9728432D25EE1E4342EC96F860390AD5AC76CA8C7052407B3BFE011AE1", "name": "Rarog 2.6.0"},
    # The release asset's digest as GitHub publishes it (maelic13/rarog v2.7.0).
    "rarog27": {"path": pathlib.Path(r"D:\chess\engines\rarog\rarog-2.7.0-windows-x64.exe"),
                "sha": "C3BB03718680D6D86E4A8366F0D76E05E000D0C12EACB7B249E631AF6C655832", "name": "Rarog 2.7.0"},
}
SEEDS = {"g0-equal": 111101, "g0-odds": 111102, "pool": 111103, "rarog": 111104, "rarog27": 111105}


def ps(value) -> str:
    """A PowerShell literal: single-quoted strings, @() arrays, plain integers."""
    if isinstance(value, (list, tuple)):
        return "@(" + ",".join(ps(v) for v in value) + ")"
    if isinstance(value, int):
        return str(value)
    return "'" + str(value).replace("'", "''") + "'"


def check_binary(path: pathlib.Path, sha: str) -> None:
    if not path.exists():
        raise Refused(f"PROVENANCE MISMATCH - no binary at {path}.")
    actual = sha256(path)
    if actual != sha:
        raise Refused(f"PROVENANCE MISMATCH - {path} is {actual}, not {sha}.")


def check_head() -> None:
    check_binary(HEAD["path"], HEAD["sha"])
    manifest = HEAD["path"].with_suffix(".manifest.txt")
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


def read_odds(dry_run: bool) -> tuple[int, int]:
    """The oracle's clock from the registered bridge-price read."""
    if not PRICE_FILE.exists():
        if dry_run:
            print(f"(dry run: no {PRICE_FILE.name} yet, so Rarog's {DRY_RUN_ODDS[0]}+{DRY_RUN_ODDS[1]} ms stands in)")
            return DRY_RUN_ODDS
        raise Refused(f"NO PRICE - run tools/diag/bridge_price.py on an idle host first; {PRICE_FILE} is missing.")
    price = json.loads(PRICE_FILE.read_text(encoding="utf-8"))
    problems = []
    if price.get("schema") != "basilisk-bridge-price-v1":
        problems.append("schema")
    if str(price.get("engine_sha256", "")).upper() != PRICE_ORACLE_SHA:
        problems.append("engine (not the price oracle of this head)")
    if price.get("command") != PRICE_COMMAND:
        problems.append(f"command {price.get('command')!r}")
    if int(price.get("cycles", 0)) < 3:
        problems.append("fewer than 3 cycles")
    if (price.get("base_ms"), price.get("increment_ms")) != (3000, 30):
        problems.append("base clock")
    odds = float(price.get("time_odds", 0))
    if not 1.0 < odds < 1.5:
        problems.append(f"odds {odds}")
    if problems:
        raise Refused(f"PRICE REFUSED - {PRICE_FILE.name}: " + "; ".join(problems) + ".")
    return int(price["oracle_base_ms"]), int(price["oracle_increment_ms"])


def gauntlet_args(engines: list[tuple[str, str, str, int, str, list[str]]], cycles: int) -> list[str]:
    extra = ["--format", "gauntlet", "--seeds", "1", "--cycles", str(cycles), "--games-per-pair", "2"]
    for engine in engines:
        extra += ["--engine", engine[1]]
    for engine in engines:
        extra += ["--label", engine[0]]
    for engine in engines:
        extra += ["--rating", str(engine[3])]
    for index, engine in enumerate(engines[1:], start=2):
        extra += ["--fixed", f"{index}:{engine[3]}"]
    for index, engine in enumerate(engines, start=1):
        for option in engine[5]:
            extra += ["--engine-option", f"{index}:{option}"]
    return extra


def command(read: str, dry_run: bool) -> tuple[str, pathlib.Path]:
    run_dir = RESULTS / f"b111-{read}"
    common = {"Hash": 64, "Threads": 1, "BaseMs": 3000, "IncrementMs": 30, "MarginMs": 20,
              "Seed": SEEDS[read], "Dir": str(run_dir), "ExpectRevision": HEAD["revision"]}
    head = ("Basilisk head", str(HEAD["path"]), HEAD["sha"], 1500, "Threads", [])
    if read == "g0-equal":
        check_binary(ORACLE["path"], ORACLE["sha"])
        oracle = ("Oracle head HCE", str(ORACLE["path"]), ORACLE["sha"], 1500, "Threads", ["Use Basilisk HCE=true"])
        engines = [head, oracle]
        extra = gauntlet_args(engines, 1500) + ["--max-engine-faults", "0"]
        # BAS-O05 permitted zero faults; the wrapper's 1% foreign allowance came later.
        params = {**common, "Mode": "gauntlet", "Concurrency": 14, "OpponentFaultRatePercent": 0,
                  "ExpectBench": [HEAD["bench"], -1],
                  "ExpectSha256": [e[2] for e in engines], "ThreadOptionNames": [e[4] for e in engines],
                  "ExtraArgs": extra}
    elif read == "g0-odds":
        check_binary(ORACLE["path"], ORACLE["sha"])
        base_b, increment_b = read_odds(dry_run)
        params = {**common, "Mode": "match", "Games": 3000, "Concurrency": 14,
                  "EngineA": str(HEAD["path"]), "NameA": "Basilisk head",
                  "EngineB": str(ORACLE["path"]), "NameB": "Oracle head HCE odds",
                  "OptionsB": ["Use Basilisk HCE=true"], "BaseMsB": base_b, "IncrementMsB": increment_b,
                  "ExpectBench": [HEAD["bench"], -1], "ExpectSha256": [HEAD["sha"], ORACLE["sha"]]}
    elif read == "pool":
        for target in TARGETS:
            check_binary(pathlib.Path(target[1]), target[2])
        # The head's start rating is only the fit's starting point; the four are fixed.
        engines = [head[:3] + (3000,) + head[4:]] + TARGETS
        # Foreign games earn the runner its 1% fault cap (colosseum.ps1 checks the value).
        extra = gauntlet_args(engines, 200) + ["--max-engine-faults", "16"]
        params = {**common, "Mode": "gauntlet", "ExpectBench": [HEAD["bench"]] + [-1] * len(TARGETS),
                  "ExpectSha256": [e[2] for e in engines], "ThreadOptionNames": [e[4] for e in engines],
                  "ExtraArgs": extra}
    else:
        rarog = RAROGS[read]
        check_binary(rarog["path"], rarog["sha"])
        params = {**common, "Mode": "match", "Games": 400, "EngineA": str(HEAD["path"]), "NameA": "Basilisk head",
                  "EngineB": str(rarog["path"]), "NameB": rarog["name"],
                  "ExpectBench": [HEAD["bench"], -1], "ExpectSha256": [HEAD["sha"], rarog["sha"]]}
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in params.items():
        parts.append(f"-{key} {ps(value)}")
    if params["Mode"] == "match":
        parts.append("-ExternalB")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE", run_dir


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--read", required=True, choices=sorted(SEEDS))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        check_head()
        text, run_dir = command(args.read, args.dry_run)
        if not args.dry_run and (run_dir / "result.json").exists():
            raise Refused(f"STALE RESULT - {run_dir} holds a finished read; it is not extended or replaced.")
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return subprocess.run(powershell() + [text], cwd=REPO).returncode


if __name__ == "__main__":
    sys.exit(main())
