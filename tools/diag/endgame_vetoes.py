#!/usr/bin/env python3
"""Absolute theory and engine-anomaly vetoes for endgame truth candidates.

Existing accepted-head theory failures are grandfathered as measured debt; a
candidate may fix them but may not introduce a clean-win discard or rule-50
failure on a position where the baseline avoided it. Engine crashes, protocol
errors, illegal moves and no-move responses are absolute vetoes anywhere.

Statistical movement is intentionally absent here. PLAN 6.0.d owns aggregate
and family confidence; this tool owns failures that no strength result can
override.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from endgame_compare import load_report, paired_families


ANOMALY_OUTCOMES = {
    "engine_crash",
    "engine_error",
    "illegal_move",
    "no_move",
}
KNOWN_OUTCOMES = ANOMALY_OUTCOMES | {
    "mated",
    "wrong_mate",
    "stalemate",
    "insufficient_material",
    "fifty_move",
    "material_lost",
    "ply_limit",
}
REQUIRED_FIELDS = {
    "outcome",
    "plies",
    "graded_moves",
    "win_preserving_moves",
    "dtz_checked_moves",
    "dtz_progress_moves",
    "first_discard_ply",
}


def discarded_clean_win(record: dict) -> bool:
    if record.get("theory_wdl") != 2:
        return False
    return (
        record.get("first_discard_ply") is not None
        or record.get("win_preserving_moves", 0) < record.get("graded_moves", 0)
    )


def malformed_record(record: dict) -> str | None:
    missing = sorted(REQUIRED_FIELDS - set(record))
    if missing:
        return "missing=" + ",".join(missing)
    if record["outcome"] not in KNOWN_OUTCOMES:
        return f"unknown_outcome={record['outcome']}"
    for numerator, denominator in (
        ("win_preserving_moves", "graded_moves"),
        ("dtz_progress_moves", "dtz_checked_moves"),
    ):
        if (
            not isinstance(record[numerator], int)
            or not isinstance(record[denominator], int)
            or record[numerator] < 0
            or record[denominator] < 0
            or record[numerator] > record[denominator]
        ):
            return f"invalid_counts={numerator}/{denominator}"
    return None


def evaluate_vetoes(families: dict[str, list[tuple]]) -> dict[str, list[str]]:
    vetoes = {
        "malformed_record": [],
        "engine_anomaly": [],
        "new_clean_win_discard": [],
        "new_rule50_failure": [],
    }
    for family, pairs in families.items():
        for position_id, baseline, candidate in pairs:
            label = f"{family}/{position_id}"
            malformed = malformed_record(candidate)
            if malformed:
                vetoes["malformed_record"].append(f"{label}:{malformed}")
            if candidate.get("outcome") in ANOMALY_OUTCOMES:
                vetoes["engine_anomaly"].append(
                    f"{label}:{candidate.get('outcome')}"
                )
            if candidate.get("theory_wdl") != 2:
                continue
            if (
                not discarded_clean_win(baseline)
                and discarded_clean_win(candidate)
            ):
                vetoes["new_clean_win_discard"].append(label)
            if (
                baseline.get("outcome") != "fifty_move"
                and candidate.get("outcome") == "fifty_move"
            ):
                vetoes["new_rule50_failure"].append(label)
    return vetoes


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--baseline",
        type=Path,
        default=root / "tools/results/endgame-truth-6.0.b/basilisk.json",
    )
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    try:
        baseline = load_report(args.baseline)
        candidate = load_report(args.candidate)
        paired = paired_families(baseline, candidate)
    except ValueError as exc:
        parser.error(str(exc))

    vetoes = evaluate_vetoes(paired)
    count = sum(len(items) for items in vetoes.values())
    verdict = "block" if count else "pass"
    result = {
        "schema": "basilisk-endgame-hard-veto-v1",
        "policy": {
            "malformed_or_incomplete_records_block": True,
            "engine_anomalies_are_absolute": sorted(ANOMALY_OUTCOMES),
            "new_clean_win_discard_blocks": True,
            "new_rule50_failure_blocks": True,
            "existing_baseline_theory_failures_are_grandfathered": True,
            "strength_cannot_override": True,
        },
        "baseline": str(args.baseline.resolve()),
        "candidate": str(args.candidate.resolve()),
        "verdict": verdict,
        "veto_count": count,
        "vetoes": vetoes,
    }

    print(f"verdict: {verdict.upper()} ({count} hard vetoes)")
    for category, items in vetoes.items():
        if items:
            print(f"{category}: " + ", ".join(items))
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n"
        )
        print(f"Report: {args.output.resolve()}")
    return 2 if count else 0


if __name__ == "__main__":
    sys.exit(main())
