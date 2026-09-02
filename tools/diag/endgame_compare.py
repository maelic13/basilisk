#!/usr/bin/env python3
"""Position-paired confidence analysis for Basilisk endgame truth reports.

The independent unit is one frozen position, never one move. Conversion and
rule-50 avoidance are binary per position. Win-preservation and DTZ-progress
are first reduced to one rate per position, then paired, so long games cannot
manufacture confidence by contributing more correlated moves.

Policy (PLAN 6.0.d):
  * report aggregate changes at or beyond 2 standard errors;
  * report family changes at or beyond 2 standard errors;
  * block family regressions at or beyond 3 standard errors.

This statistical layer does not replace the absolute theory/anomaly vetoes in
PLAN 6.0.e.
"""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from pathlib import Path


REPORT_SIGMA = 2.0
FAMILY_BLOCK_SIGMA = 3.0
CONTRACT_FIELDS = (
    "nodes_per_move",
    "max_plies",
    "hash_mb",
    "persistent_tt_per_game",
    "score_adjudication",
)
COHORT_FIELDS = ("manifest_sha256", "book_sha256", "position_count")
METRICS = ("conversion", "win_preservation", "dtz_progress")


def load_report(path: Path) -> dict:
    try:
        report = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot read {path}: {exc}") from exc
    if report.get("schema") != "basilisk-endgame-truth-v1":
        raise ValueError(f"{path}: not a basilisk-endgame-truth-v1 report")
    if not isinstance(report.get("families"), dict):
        raise ValueError(f"{path}: missing families")
    if not isinstance(report.get("cohort"), dict):
        raise ValueError(f"{path}: not a frozen-cohort report")
    return report


def position_map(report: dict, family: str) -> dict[str, dict]:
    records = report["families"][family].get("positions")
    if not isinstance(records, list):
        raise ValueError(f"{family}: report lacks per-position records")
    mapped = {}
    for record in records:
        position_id = record.get("id")
        if not position_id or position_id in mapped:
            raise ValueError(f"{family}: missing or duplicate position id")
        mapped[position_id] = record
    return mapped


def paired_families(baseline: dict, candidate: dict) -> dict[str, list[tuple]]:
    if list(baseline["families"]) != list(candidate["families"]):
        raise ValueError("family names or ordering differ")
    for field in CONTRACT_FIELDS:
        if baseline.get(field) != candidate.get(field):
            raise ValueError(f"contract mismatch for {field}")
    for field in COHORT_FIELDS:
        if baseline["cohort"].get(field) != candidate["cohort"].get(field):
            raise ValueError(f"cohort mismatch for {field}")

    paired = {}
    total_positions = 0
    for family in baseline["families"]:
        left = position_map(baseline, family)
        right = position_map(candidate, family)
        if set(left) != set(right):
            raise ValueError(f"{family}: position IDs differ")
        rows = []
        for position_id, base_record in left.items():
            cand_record = right[position_id]
            for field in ("fen", "theory_wdl", "theory_dtz"):
                if base_record.get(field) != cand_record.get(field):
                    raise ValueError(f"{family}/{position_id}: {field} differs")
            rows.append((position_id, base_record, cand_record))
        paired[family] = rows
        total_positions += len(rows)
    if total_positions != baseline["cohort"]["position_count"]:
        raise ValueError(
            f"paired records cover {total_positions} positions, expected "
            f"{baseline['cohort']['position_count']}"
        )
    return paired


def metric_value(record: dict, metric: str) -> float | None:
    if record.get("theory_wdl") != 2:
        return None
    if metric == "conversion":
        return float(record.get("outcome") == "mated")
    if metric == "win_preservation":
        denominator = record.get("graded_moves", 0)
        return (
            record.get("win_preserving_moves", 0) / denominator
            if denominator else None
        )
    if metric == "dtz_progress":
        denominator = record.get("dtz_checked_moves", 0)
        if denominator:
            return record.get("dtz_progress_moves", 0) / denominator
        # On a clean-win start, zero checked moves normally means the first
        # graded move discarded the win. Excluding it would let regression
        # remove its own worst observation from the paired sample.
        return 0.0 if record.get("graded_moves", 0) else None
    raise ValueError(f"unknown metric {metric}")


def paired_stats(pairs: list[tuple], metric: str) -> dict | None:
    values = []
    for _, baseline, candidate in pairs:
        before = metric_value(baseline, metric)
        after = metric_value(candidate, metric)
        if before is not None and after is not None:
            values.append((before, after))
    if not values:
        return None

    differences = [after - before for before, after in values]
    delta = statistics.fmean(differences)
    standard_error = (
        statistics.stdev(differences) / math.sqrt(len(differences))
        if len(differences) > 1 else None
    )
    if standard_error is None:
        sigma = None
    elif standard_error == 0.0:
        sigma = 0.0 if delta == 0.0 else math.copysign(math.inf, delta)
    else:
        sigma = delta / standard_error
    return {
        "n": len(values),
        "baseline": statistics.fmean(before for before, _ in values),
        "candidate": statistics.fmean(after for _, after in values),
        "delta": delta,
        "standard_error": standard_error,
        "sigma": sigma,
    }


def classify(stats: dict | None, family: bool) -> str:
    if stats is None or stats["sigma"] is None:
        return "not_applicable"
    sigma = stats["sigma"]
    if family and sigma <= -FAMILY_BLOCK_SIGMA:
        return "block"
    if sigma <= -REPORT_SIGMA:
        return "report_regression"
    if sigma >= REPORT_SIGMA:
        return "report_improvement"
    return "within_band"


def finite(value: float | None) -> float | str | None:
    if value is None or math.isfinite(value):
        return value
    return "+infinity" if value > 0 else "-infinity"


def render_stats(stats: dict | None, action: str) -> dict:
    if stats is None:
        return {"action": action}
    return {key: finite(value) for key, value in stats.items()} | {"action": action}


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
        families = paired_families(baseline, candidate)
    except ValueError as exc:
        parser.error(str(exc))

    aggregate_pairs = [pair for pairs in families.values() for pair in pairs]
    aggregate = {}
    family_results = {}
    blocks = []
    reports = []

    for metric in METRICS:
        stats = paired_stats(aggregate_pairs, metric)
        action = classify(stats, family=False)
        aggregate[metric] = render_stats(stats, action)
        if action.startswith("report_"):
            reports.append(f"aggregate/{metric}")

    for family, pairs in families.items():
        family_results[family] = {}
        for metric in METRICS:
            stats = paired_stats(pairs, metric)
            action = classify(stats, family=True)
            family_results[family][metric] = render_stats(stats, action)
            if action == "block":
                blocks.append(f"{family}/{metric}")
            elif action.startswith("report_"):
                reports.append(f"{family}/{metric}")

    verdict = "block" if blocks else "report" if reports else "pass"
    result = {
        "schema": "basilisk-endgame-paired-confidence-v1",
        "policy": {
            "independent_unit": "frozen position",
            "aggregate_report_sigma": REPORT_SIGMA,
            "family_report_sigma": REPORT_SIGMA,
            "family_block_sigma": FAMILY_BLOCK_SIGMA,
            "aggregate_reports_do_not_block": True,
            "move_rates_are_reduced_to_one_value_per_position": True,
        },
        "baseline": str(args.baseline.resolve()),
        "candidate": str(args.candidate.resolve()),
        "verdict": verdict,
        "blocks": blocks,
        "reports": reports,
        "aggregate": aggregate,
        "families": family_results,
    }

    print(f"verdict: {verdict.upper()}")
    for metric, stats in aggregate.items():
        if stats["action"] != "not_applicable":
            print(
                f"aggregate/{metric}: {stats['baseline']:.4f} -> "
                f"{stats['candidate']:.4f}, delta {stats['delta']:+.4f}, "
                f"{stats['sigma']} SE [{stats['action']}]"
            )
    if reports:
        print("reports: " + ", ".join(reports))
    if blocks:
        print("blocks: " + ", ".join(blocks))

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n"
        )
        print(f"Report: {args.output.resolve()}")
    return 2 if blocks else 0


if __name__ == "__main__":
    sys.exit(main())
