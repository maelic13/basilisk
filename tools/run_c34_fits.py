#!/usr/bin/env python3
"""Frozen launcher for C.3.4's matched static fits on corpus A: one-k and k-phase.

The contract is analysis/c31_rebuild_2026-11-25.md (§11 and Amendment 7) and
the registration docs/experiments/BAS-O17.md (Run 2, Amendment 1: widened knob
bounds; Run 1 artifacts preserved in c34-fit-onek and c34-fit-kphase). Both arms
run tools/texel/fit.py with --rebuild and --relevel-material from a clean tree
on corpus A:

  one-k    one frozen constant K fitted on validation at the seed
  k-phase  K linear in phase, fitted once on validation at the seed and frozen

  all: one-k, then k-phase. A finished arm (result == "fitted") is skipped when
  running all; naming a finished arm refuses to overwrite it.

Usage:
  python tools/run_c34_fits.py --arm <one-k|k-phase|all> [--dry-run]
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]
FIT = REPO / "tools" / "texel" / "fit.py"
CORPUS = REPO / "tools" / "texel" / "data" / "corpusA" / "rows"
SURFACE = REPO / "tools" / "texel" / "surface_rebuild_v1.tsv"
BUILD_DIR = REPO / "build" / "c34-fit"
RESULTS = REPO / "tools" / "results"
EXPECT_BENCH = 3717775

COMMON_ARGS = [
    "--corpus", str(CORPUS),
    "--surface", str(SURFACE),
    "--build-dir", str(BUILD_DIR),
    "--rebuild",
    "--relevel-material",
    "--expect-bench", str(EXPECT_BENCH),
    "--stages", "kingsafety,linear,kingsafety,polish",
    "--lr", "0.5",
    "--epochs", "20000",
    "--patience", "50",
    "--min-delta", "1e-6",
    "--l2", "0",
    "--smooth", "0",
    "--max-positions", "0",
    "--ks-positions", "100000",
    "--rank-positions", "300000",
    "--min-support", "200",
]

ARMS = {
    "one-k": {"k_mode": "single", "out": RESULTS / "c34-fit-onek-r2"},
    "k-phase": {"k_mode": "phase", "out": RESULTS / "c34-fit-kphase-r2"},
}
ORDER = ["one-k", "k-phase"]


class Refused(Exception):
    pass


def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def check_clean_tree() -> str:
    dirty = subprocess.run(
        ["git", "status", "--porcelain", "--untracked-files=no"],
        cwd=REPO, capture_output=True, text=True, check=True,
    ).stdout.strip()
    if dirty:
        raise Refused(f"DIRTY TREE - commit or revert tracked changes before fitting:\n{dirty}")
    return subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=REPO, capture_output=True, text=True, check=True,
    ).stdout.strip()


def check_corpus() -> None:
    manifest_path = CORPUS / "manifest.json"
    if not manifest_path.is_file():
        raise Refused(f"MISSING CORPUS - {manifest_path} not found.")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    for split in ("train", "validation"):
        entry = manifest["output"][split]
        file_path = CORPUS / entry["file"]
        if not file_path.is_file():
            raise Refused(f"MISSING CORPUS FILE - {file_path} not found.")
        actual = sha256(file_path)
        if actual != entry["sha256"]:
            raise Refused(f"CORPUS HASH MISMATCH - {file_path} hashes {actual}, expected {entry['sha256']}.")


def arm_command(arm: str) -> list[str]:
    spec = ARMS[arm]
    return [
        sys.executable, str(FIT),
        *COMMON_ARGS,
        "--k-mode", spec["k_mode"],
        "--out", str(spec["out"]),
    ]


def is_finished(out_dir: pathlib.Path) -> bool:
    run_json = out_dir / "run.json"
    if not run_json.is_file():
        return False
    return json.loads(run_json.read_text(encoding="utf-8")).get("result") == "fitted"


def dry_run_checks() -> None:
    exe = ".exe" if sys.platform.startswith("win") else ""
    texel_dir = BUILD_DIR / "texel"
    subprocess.run(
        ["cmake", "--preset", "release", "-B", str(texel_dir), "-DCOMP=clang", "-DTEXEL=ON", "-DEVAL_REBUILD=ON"],
        cwd=REPO, check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(texel_dir), "--target", "basilisk-texel"],
        cwd=REPO, check=True,
    )
    tuner = texel_dir / f"basilisk-texel{exe}"
    subprocess.run(
        [sys.executable, str(REPO / "tools" / "texel" / "surface.py"), "check",
         "--tuner", str(tuner), "--manifest", str(SURFACE)],
        cwd=REPO, check=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--arm", required=True, choices=ORDER + ["all"])
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    arms = ORDER if args.arm == "all" else [args.arm]

    try:
        rev = check_clean_tree()
        check_corpus()
        print(f"Tree clean at {rev[:10]}; corpus A hashes verified; surface {SURFACE.name}.")
        if args.dry_run:
            dry_run_checks()
            for arm in arms:
                print(f"== {arm} (dry run)\n  {' '.join(arm_command(arm))}")
            return 0
        for arm in arms:
            out_dir = ARMS[arm]["out"]
            if is_finished(out_dir):
                if args.arm == "all":
                    print(f"{arm}: finished; skipped.")
                    continue
                raise Refused(f"STALE RESULT - {out_dir} already holds a finished fit.")
            if out_dir.exists() and any(out_dir.iterdir()):
                raise Refused(f"NON-EMPTY DIR - {out_dir} exists and is not a finished fit.")
            cmd = arm_command(arm)
            print(f"== {arm}\n  {' '.join(cmd)}", flush=True)
            status = subprocess.run(cmd, cwd=REPO).returncode
            if status != 0:
                print(f"{arm} stopped with exit {status}; the run stops here.", file=sys.stderr)
                return status
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as exc:
        print(f"Command failed with exit {exc.returncode}: {exc.cmd}", file=sys.stderr)
        return exc.returncode or 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
