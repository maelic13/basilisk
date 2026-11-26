#!/usr/bin/env python3
"""Frozen launcher for C.3.5's gate, LTC read, frontier pair and endgame veto.

The contract is analysis/c31_rebuild_2026-11-25.md (§12) and the registrations
docs/experiments/BAS-E65.md, BAS-M26.md and BAS-M27.md. Both Basilisk arms are
clean final-PGO PEXT release builds from 0374cfd (`src/` at 5128e6e, `k-phase`
Run 2 baked into `src/eval_rb_params.h`, Clang 23.1.5, pinned by SHA-256 and
by their manifests):

  veto      C.2.7 endgame bracket veto at 21,000, 64,000 and 190,000 nodes
            against the head arm and tools/results/c274/head (c35/veto/)
  sprt      BAS-E65: SPRT [0,3] nElo (sprt-default.toml) at 3+0.03 1T, Hash 64,
            margin 20 ms, paired reversed UHO in random order, no adjudication,
            no tablebases, cap 4,000 pairs, seed 350001 (c35-sprt/)
  ltc       BAS-M26: 1,000-game fixed match at 10+0.1 1T, Hash 64, margin 20 ms,
            match-fixed-ltc.toml, seed 350002 (c35-ltc/)
  frontier  BAS-M27: 3,000-game fixed match against Stockfish 16 HCE
            (`Use NNUE=false`) at 3+0.03 1T, Hash 64, margin 20 ms, concurrency
            14, seed 350003 (c35-frontier/)

  all: veto, sprt, ltc, frontier. A finished leg (result.json, or both
  veto.json and veto_against_c274_head.json for veto) is skipped when running all;
  naming a finished leg refuses to overwrite or extend it. An interrupted SPRT
  or match resumes from its checkpoint with the same command. If veto returns
  BLOCK (exit 2 after writing veto.json), `--run all` records it, finishes the
  remaining legs, and exits with veto's non-zero code at the end.

Usage:
  python tools/run_c35_gate.py --run <veto|sprt|ltc|frontier|all> [--dry-run]
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b111_reads import check_binary, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
RESULTS = REPO / "tools" / "results"
ENGINES = REPO / "tools" / "test_engines"

REVISION = "0374cfd8f36005e84fe0c1678e5352d14973c0c8"
CAND = {
    "path": ENGINES / "basilisk-c35-cand-pext-pgo.exe",
    "sha": "DF1FAA70DFBBC30417CF238514F647822D749C979D3F98342231391BA787A473",
    "bench": 3880333,
    "arm": "ON",
    "revision": REVISION,
}
BASE = {
    "path": ENGINES / "basilisk-c35-base-pext-pgo.exe",
    "sha": "0A4D9CCB64E1435400DCE4CDD47AF77C6AD20513A0D56E7B52B93C7A7F1C4F20",
    "bench": 3359329,
    "arm": "OFF",
    "revision": REVISION,
}
SF16 = {
    "path": pathlib.Path(r"D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe"),
    "sha": "041396A4B14EC25BAE0B57F90E917A17E7FCEB1FBBAB80859EE2EC3774D25442",
}

SEEDS = {"sprt": 350001, "ltc": 350002, "frontier": 350003}
ORDER = ["veto", "sprt", "ltc", "frontier"]
BUDGETS = "21000,64000,190000"
SYZYGY = "D:/chess/tablebases/syzygy3456"


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
        "arm_state": arm["arm"],
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


def check_arms() -> None:
    c_cand = check_arm(CAND)
    c_base = check_arm(BASE)
    if c_cand != c_base:
        raise Refused(f"COMPILER MISMATCH - cand uses {c_cand!r}, base uses {c_base!r}.")
    check_binary(SF16["path"], SF16["sha"])


def leg_finished(leg: str) -> bool:
    if leg == "veto":
        out = RESULTS / "c35" / "veto"
        return (out / "veto.json").is_file() and (out / "veto_against_c274_head.json").is_file()
    return (RESULTS / f"c35-{leg}" / "result.json").is_file()


def run_colosseum(leg: str, dry_run: bool) -> int:
    run_dir = RESULTS / f"c35-{leg}"
    common = {
        "MarginMs": 20,
        "Seed": SEEDS[leg],
        "Dir": str(run_dir),
        "Hash": 64,
        "Threads": 1,
        "ExpectRevision": REVISION,
    }
    if leg == "sprt":
        params = {
            **common,
            "Mode": "sprt",
            "Bracket": "default",
            "MaxPairs": 4000,
            "BaseMs": 3000,
            "IncrementMs": 30,
            "EngineA": str(CAND["path"]),
            "NameA": "Basilisk C.3 rebuild",
            "EngineB": str(BASE["path"]),
            "NameB": "Basilisk head",
            "ExpectBench": [CAND["bench"], BASE["bench"]],
            "ExpectSha256": [CAND["sha"], BASE["sha"]],
        }
    elif leg == "ltc":
        params = {
            **common,
            "Mode": "match",
            "Games": 1000,
            "BaseMs": 10000,
            "IncrementMs": 100,
            "RunFile": str(REPO / "tools" / "colosseum" / "match-fixed-ltc.toml"),
            "EngineA": str(CAND["path"]),
            "NameA": "Basilisk C.3 rebuild",
            "EngineB": str(BASE["path"]),
            "NameB": "Basilisk head",
            "ExpectBench": [CAND["bench"], BASE["bench"]],
            "ExpectSha256": [CAND["sha"], BASE["sha"]],
        }
    else:  # frontier
        params = {
            **common,
            "Mode": "match",
            "Games": 3000,
            "BaseMs": 3000,
            "IncrementMs": 30,
            "Concurrency": 14,
            "EngineA": str(CAND["path"]),
            "NameA": "Basilisk C.3 rebuild",
            "EngineB": str(SF16["path"]),
            "NameB": "Stockfish 16 HCE",
            "OptionsB": ["Use NNUE=false"],
            "ExpectBench": [CAND["bench"], -1],
            "ExpectSha256": [CAND["sha"], SF16["sha"]],
        }
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    parts += [f"-{key} {ps(value)}" for key, value in params.items()]
    if leg == "frontier":
        parts.append("-ExternalB")
    if dry_run:
        parts.append("-DryRun")
    return subprocess.run(powershell() + [" ".join(parts) + "; exit $LASTEXITCODE"], cwd=REPO).returncode


def run_veto(dry_run: bool) -> int:
    out = RESULTS / "c35" / "veto"
    bracket = [sys.executable, str(REPO / "tools" / "diag" / "endgame_budget_bracket.py")]
    steps = [
        [*bracket, "run", "--engine", str(BASE["path"]), "--syzygy", SYZYGY, "--budgets", BUDGETS,
         "--workers", "30", "--out-dir", str(out / "base")],
        [*bracket, "run", "--engine", str(CAND["path"]), "--syzygy", SYZYGY, "--budgets", BUDGETS,
         "--workers", "30", "--out-dir", str(out / "cand")],
        [*bracket, "veto", "--baseline-dir", str(out / "base"), "--candidate-dir", str(out / "cand"),
         "--budgets", BUDGETS, "--output", str(out / "veto.json")],
        [*bracket, "veto", "--baseline-dir", str(RESULTS / "c274" / "head"), "--candidate-dir",
         str(out / "cand"), "--budgets", BUDGETS, "--output", str(out / "veto_against_c274_head.json")],
    ]
    if dry_run:
        for step in steps:
            print(" ".join(step))
        return 0
    out.mkdir(parents=True, exist_ok=True)
    status = 0
    for step in steps:
        done = subprocess.run(step, cwd=REPO)
        print(f"exit {done.returncode}: {' '.join(step[2:4])}", flush=True)
        status = status or done.returncode
    return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run", required=True, choices=ORDER + ["all"])
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    legs = ORDER if args.run == "all" else [args.run]
    try:
        check_arms()
        print(f"Arms verified at {REVISION[:10]} (cand bench {CAND['bench']}, base bench {BASE['bench']}; "
              f"compilers match; SF16 hash verified).")
        deferred_exit = 0
        for leg in legs:
            if leg_finished(leg):
                if args.run == "all" and not args.dry_run:
                    print(f"{leg}: finished; skipped.")
                    continue
                if not args.dry_run:
                    raise Refused(f"STALE RESULT - {leg} already holds a finished result; it is not extended or replaced.")
            print(f"== {leg}")
            status = run_veto(args.dry_run) if leg == "veto" else run_colosseum(leg, args.dry_run)
            if status != 0:
                if args.run == "all" and leg == "veto" and (RESULTS / "c35" / "veto" / "veto.json").is_file():
                    print(f"veto reported BLOCK (exit {status}); continuing remaining legs.", file=sys.stderr)
                    deferred_exit = status
                    continue
                print(f"{leg} stopped with exit {status}; the run stops here.", file=sys.stderr)
                return status
        return deferred_exit
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
