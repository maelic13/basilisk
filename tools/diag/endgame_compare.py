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

Conversion is binary per position, so it is tested by McNemar on the
discordant positions only: agreement carries no information. Under
MIN_DISCORDANT discordant positions (or, for the per-position rates, positions
that moved) a result is INDETERMINATE, and a zero-variance sample never yields
an infinite sigma: three positions that each lose a conversion once read as a
block (C.2.7.1's audit), which a thin family must never manufacture.

The candidate may cover a subset of the baseline's families, compared family
by family under the cohort digest `endgame_contract.py` recomputes from the
records. There is no default baseline: it is always named.

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

from endgame_contract import load_truth_report


REPORT_SIGMA = 2.0
MIN_DISCORDANT = 6
FAMILY_BLOCK_SIGMA = 3.0
CONTRACT_FIELDS = (
    "nodes_per_move",
    "max_plies",
    "hash_mb",
    "persistent_tt_per_game",
    "score_adjudication",
)
COHORT_FIELDS = ("manifest_sha256", "book_sha256")
METRICS = ("conversion", "win_preservation", "dtz_progress")


def load_report(path: Path) -> dict:
    report = load_truth_report(path)
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
    names = list(candidate["families"])
    absent = [name for name in names if name not in baseline["families"]]
    if absent:
        raise ValueError(f"families absent from the baseline: {','.join(absent)}")
    if [name for name in baseline["families"] if name in candidate["families"]] != names:
        raise ValueError("family ordering differs")
    for name in names:
        if baseline["_family_digests"].get(name) != candidate["_family_digests"].get(name):
            raise ValueError(f"{name}: cohort digests differ")
    for field in CONTRACT_FIELDS:
        if baseline.get(field) != candidate.get(field):
            raise ValueError(f"contract mismatch for {field}")
    for field in COHORT_FIELDS:
        if baseline["cohort"].get(field) != candidate["cohort"].get(field):
            raise ValueError(f"cohort mismatch for {field}")

    paired = {}
    total_positions = 0
    for family in names:
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
    if total_positions != candidate["cohort"]["position_count"]:
        raise ValueError(
            f"paired records cover {total_positions} positions, expected "
            f"{candidate['cohort']['position_count']}"
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
    stats = {
        "n": len(values),
        "baseline": statistics.fmean(before for before, _ in values),
        "candidate": statistics.fmean(after for _, after in values),
        "delta": delta,
    }
    if metric == "conversion":
        improved = sum(1 for d in differences if d > 0)
        regressed = sum(1 for d in differences if d < 0)
        discordant = improved + regressed
        stats |= {"method": "mcnemar", "improved": improved,
                  "regressed": regressed, "discordant": discordant,
                  "standard_error": None}
        if discordant < MIN_DISCORDANT:
            return stats | {"sigma": None, "indeterminate":
                            f"{discordant} discordant positions, under {MIN_DISCORDANT}"}
        return stats | {"sigma": (improved - regressed) / math.sqrt(discordant)}
    moved = sum(1 for d in differences if d != 0)
    stats |= {"method": "paired_mean", "moved": moved}
    if moved < MIN_DISCORDANT:
        return stats | {"standard_error": None, "sigma": None, "indeterminate":
                        f"{moved} positions moved, under {MIN_DISCORDANT}"}
    standard_error = statistics.stdev(differences) / math.sqrt(len(differences))
    if standard_error == 0.0:
        return stats | {"standard_error": 0.0, "sigma": None,
                        "indeterminate": "zero variance"}
    return stats | {"standard_error": standard_error, "sigma": delta / standard_error}


def classify(stats: dict | None, family: bool) -> str:
    if stats is None:
        return "not_applicable"
    if stats.get("indeterminate"):
        return "indeterminate"
    sigma = stats["sigma"]
    if family and sigma <= -FAMILY_BLOCK_SIGMA:
        return "block"
    if sigma <= -REPORT_SIGMA:
        return "report_regression"
    if sigma >= REPORT_SIGMA:
        return "report_improvement"
    return "within_band"


def finite(value):
    if not isinstance(value, float) or math.isfinite(value):
        return value
    return "+infinity" if value > 0 else "-infinity"


def render_stats(stats: dict | None, action: str) -> dict:
    if stats is None:
        return {"action": action}
    return {key: finite(value) for key, value in stats.items()} | {"action": action}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True, type=Path)
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
    indeterminate = []

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
            elif action == "indeterminate":
                indeterminate.append(f"{family}/{metric}")

    verdict = "block" if blocks else "report" if reports else "pass"
    result = {
        "schema": "basilisk-endgame-paired-confidence-v2",
        "policy": {
            "conversion_test": "McNemar on discordant positions",
            "min_discordant": MIN_DISCORDANT,
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
        "indeterminate": indeterminate,
        "families_compared": list(families),
        "aggregate": aggregate,
        "families": family_results,
    }

    print(f"verdict: {verdict.upper()}")
    for metric, stats in aggregate.items():
        if stats["action"] not in ("not_applicable", "indeterminate"):
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
