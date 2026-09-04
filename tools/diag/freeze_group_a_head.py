#!/usr/bin/env python3
"""Freeze the accepted Group A head and its truth report (PLAN 6.4.c).

Group A closes with 6.1's KBNK work accepted, 6.2 gated, 6.3 closed empty and
6.4 audited. This records exactly what "the accepted head" means so that 6.5
starts from a fixed, checkable point rather than from whatever is on disk.

The artifact is deliberately content-addressed rather than commit-addressed:
binary hashes, bench, the shipped coefficient vector and the per-family truth
numbers. A commit SHA would name the revision that WROTE the freeze, which is
not the same thing as the state being frozen, and it would go stale the moment
documentation moved without the engine changing.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def bench_nodes(engine: Path) -> int:
    out = subprocess.run([str(engine)], input="bench 13\nquit\n",
                         capture_output=True, text=True, timeout=600).stdout
    for line in out.splitlines():
        if "Nodes searched" in line:
            return int(line.split(":")[1].strip())
    raise ValueError(f"no bench line from {engine}")


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path,
                        default=root / "build/release-pext/basilisk.exe")
    parser.add_argument("--tune", type=Path,
                        default=root / "build/tune/basilisk.exe")
    parser.add_argument("--truth", type=Path,
                        default=root / "tools/results/group-a-head/truth.json")
    parser.add_argument("--ceilings", type=Path,
                        default=root / "tools/diag/endgame_ceilings_v2.json")
    parser.add_argument("--output", type=Path,
                        default=root / "tools/diag/group_a_head_v1.json")
    args = parser.parse_args()

    for path in (args.release, args.tune, args.truth, args.ceilings):
        if not path.is_file():
            parser.error(f"missing input: {path}")

    truth = json.loads(args.truth.read_text(encoding="utf-8"))
    if truth["engine_sha256"] != sha256(args.tune):
        parser.error("truth report was not produced by the tune binary given")

    families, converted, won = {}, 0, 0
    for name, family in truth["families"].items():
        families[name] = {
            "clean_wins": family["theoretically_won"],
            "converted": family["converted"],
            "win_preserving_rate": family["win_preserving_rate"],
            "dtz_progress_rate": family["dtz_progress_rate"],
        }
        converted += family["converted"]
        won += family["theoretically_won"]

    release_bench = bench_nodes(args.release)
    tune_bench = bench_nodes(args.tune)
    if release_bench != tune_bench:
        parser.error(f"bench differs: release {release_bench}, tune {tune_bench}")

    frozen = {
        "schema": "basilisk-group-a-head-v1",
        "purpose": "PLAN 6.4.c: the accepted Group A head and its truth report",
        "accepted_kbnk_drive": "15600,1900,0,460,0",
        "bench_13_nodes": release_bench,
        "binaries": {
            "release_pext": {"path": str(args.release), "sha256": sha256(args.release)},
            "tune_pext": {"path": str(args.tune), "sha256": sha256(args.tune)},
        },
        "truth_report": {
            "path": str(args.truth),
            "sha256": sha256(args.truth),
            "cohort_manifest_sha256": truth["cohort"]["manifest_sha256"],
            "nodes_per_move": truth["nodes_per_move"],
            "max_plies": truth["max_plies"],
            "hash_mb": truth["hash_mb"],
            "start_halfmove_clock": truth.get("start_halfmove_clock", 0),
            "score_adjudication": truth["score_adjudication"],
        },
        "aggregate": {
            "clean_wins": won,
            "converted": converted,
            "conversion_rate": round(converted / won, 6) if won else None,
        },
        "families": families,
        "reference_ceilings": {
            "path": str(args.ceilings),
            "sha256": sha256(args.ceilings),
            "note": "attained reference results, not theoretical ceilings; "
                    "regenerated in 6.0.c from the corrected arms (BAS-E50)",
        },
        "what_this_licenses": [
            "a fixed baseline for 6.5 candidates to be paired against",
            "the identity of the binary any Group B gate must start from",
        ],
        "what_this_does_not_license": [
            "any Elo claim: 6.2.a stopped undecided at practical equivalence",
            "reuse of the 198-position KBNK cohort for SELECTION, which is spent",
            "reading conversion across families without matching DTZ slack",
        ],
    }
    args.output.write_text(json.dumps(frozen, indent=2) + "\n",
                           encoding="utf-8", newline="\n")
    print(f"Froze Group A head: {converted}/{won} clean wins "
          f"({100 * converted / won:.2f}%), bench {release_bench}")
    print(f"Wrote {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
