#!/usr/bin/env python3
"""Validate and summarize the paired PLAN 6.1.c KBNK coefficient screen."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

VARIANTS = {
    "baseline": "800,900,220,220",
    "diagonal-600": "600,900,220,220",
    "diagonal-1000": "1000,900,220,220",
    "no-edge": "800,0,220,220",
    "no-king": "800,900,0,220",
    "no-knight": "800,900,220,0",
    "no-edge-knight": "800,0,220,0",
    "dominant-diagonal": "1000,0,220,0",
    "rarog-shape": "1000,0,100,0",
    "diagonal-only": "1000,0,0,0",
}
HARD_OUTCOMES = {"engine_crash", "illegal_move", "no_move"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result_dir", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    reports = {}
    for name, weights in VARIANTS.items():
        path = args.result_dir / f"{name}.json"
        if not path.is_file():
            parser.error(f"missing variant report: {path}")
        report = json.loads(path.read_text(encoding="utf-8"))
        if report.get("engine_options", {}).get("KBNK Drive") != weights:
            parser.error(f"{name}: KBNK Drive provenance does not match registry")
        reports[name] = report

    baseline = reports["baseline"]
    invariant_keys = ("engine_sha256", "nodes_per_move", "max_plies", "hash_mb")
    baseline_ids = [p["id"] for p in baseline["families"]["KBN-K"]["positions"]]
    for name, report in reports.items():
        for key in invariant_keys:
            if report.get(key) != baseline.get(key):
                parser.error(f"{name}: mismatched {key}")
        if report.get("cohort", {}).get("book_sha256") != baseline["cohort"]["book_sha256"]:
            parser.error(f"{name}: mismatched frozen cohort")
        ids = [p["id"] for p in report["families"]["KBN-K"]["positions"]]
        if ids != baseline_ids:
            parser.error(f"{name}: position pairing/order differs")

    base_positions = {
        p["id"]: p for p in baseline["families"]["KBN-K"]["positions"]
    }
    rows = []
    for name, weights in VARIANTS.items():
        family = reports[name]["families"]["KBN-K"]
        positions = {p["id"]: p for p in family["positions"]}
        gained = sum(
            base_positions[key]["outcome"] != "mated" and p["outcome"] == "mated"
            for key, p in positions.items()
        )
        lost = sum(
            base_positions[key]["outcome"] == "mated" and p["outcome"] != "mated"
            for key, p in positions.items()
        )
        hard = sum(
            p["outcome"] in HARD_OUTCOMES or p.get("anomaly") is not None
            for p in positions.values()
        )
        discarded = sum(p.get("first_discard_ply") is not None for p in positions.values())
        rows.append({
            "variant": name,
            "weights_diagonal_edge_king_knight": weights,
            "converted": family["converted"],
            "conversion_rate": family["conversion_rate"],
            "conversion_delta_vs_baseline": family["converted"] - baseline["families"]["KBN-K"]["converted"],
            "paired_conversions_gained": gained,
            "paired_conversions_lost": lost,
            "win_preserving_rate": family["win_preserving_rate"],
            "dtz_progress_rate": family["dtz_progress_rate"],
            "median_mate_plies": family["median_mate_plies"],
            "mate_efficiency": family["mate_efficiency"],
            "discarded_clean_wins": discarded,
            "hard_anomalies": hard,
            "outcomes": family["outcomes"],
        })

    ranked = sorted(
        rows,
        key=lambda row: (
            row["hard_anomalies"],
            -row["converted"],
            row["discarded_clean_wins"],
            row["median_mate_plies"] if row["median_mate_plies"] is not None else 10**9,
        ),
    )
    summary = {
        "schema": "basilisk-kbnk-coefficient-sweep-v1",
        "purpose": "6.1.c paired 60-position screen; selection requires review, not automatic adoption",
        "engine_sha256": baseline["engine_sha256"],
        "cohort_book_sha256": baseline["cohort"]["book_sha256"],
        "position_ids": baseline_ids,
        "nodes_per_move": baseline["nodes_per_move"],
        "max_plies": baseline["max_plies"],
        "workers": baseline["workers"],
        "ranking_policy": [
            "reject engine/protocol anomalies",
            "prefer conversion count on identical positions",
            "use clean-win preservation, DTZ progress and mate efficiency diagnostically",
            "prefer the simpler coefficient vector when practical results are tied",
            "confirm the selected vector on all 198 frozen positions in step 6.1.e",
        ],
        "variants": rows,
        "ranked_variants": [row["variant"] for row in ranked],
    }
    rendered = json.dumps(summary, indent=2) + "\n"
    output = args.output or args.result_dir / "summary.json"
    output.write_text(rendered, encoding="utf-8", newline="\n")

    print("variant              weights             conv  delta  gain/loss  discard  hard")
    for row in ranked:
        print(
            f'{row["variant"]:<20} {row["weights_diagonal_edge_king_knight"]:<19} '
            f'{row["converted"]:>3}/{len(baseline_ids):<3} {row["conversion_delta_vs_baseline"]:>+5} '
            f'{row["paired_conversions_gained"]:>3}/{row["paired_conversions_lost"]:<3} '
            f'{row["discarded_clean_wins"]:>7} {row["hard_anomalies"]:>5}'
        )
    print(f"Summary: {output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
