#!/usr/bin/env python3
"""Derive PLAN 6.0.c finite-node family ceilings from paired truth reports.

The ceiling is the best conversion count actually attained by either complete
single-engine run under the frozen contract. It is a reachable comparison
target, not a claim that stronger performance is impossible. The union says
how many positions at least one engine converted, but is stretch evidence only:
no single engine necessarily attained that combination.

This tool deliberately does not derive confidence intervals or pass/fail
tolerances. Those belong to PLAN 6.0.d.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


CONTRACT_FIELDS = (
    "nodes_per_move",
    "max_plies",
    "hash_mb",
    "persistent_tt_per_game",
    "score_adjudication",
)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


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


def rate(count: int, total: int) -> float | None:
    return round(count / total, 6) if total else None


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--accepted",
        type=Path,
        default=root / "tools/results/endgame-truth-6.0.b/basilisk.json",
    )
    parser.add_argument(
        "--reference",
        type=Path,
        default=root / "tools/results/endgame-truth-6.0.b/reference.json",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).with_name("endgame_ceilings_v1.json"),
    )
    args = parser.parse_args()

    try:
        accepted = load_report(args.accepted)
        reference = load_report(args.reference)
    except ValueError as exc:
        parser.error(str(exc))

    accepted_families = list(accepted["families"])
    if accepted_families != list(reference["families"]):
        parser.error("family names or ordering differ between reports")
    for field in CONTRACT_FIELDS:
        if accepted.get(field) != reference.get(field):
            parser.error(f"contract mismatch for {field}")
    for field in ("manifest_sha256", "book_sha256", "position_count"):
        if accepted["cohort"].get(field) != reference["cohort"].get(field):
            parser.error(f"cohort mismatch for {field}")
    for report_name, report in (("accepted", accepted), ("reference", reference)):
        if report["cohort"].get("verified_positions") != report["cohort"].get(
            "position_count"
        ):
            parser.error(f"{report_name} report did not verify the complete cohort")
    if accepted.get("score_adjudication") is not False:
        parser.error("ceiling source must have score adjudication disabled")

    families = {}
    aggregate = {
        "clean_wins": 0,
        "accepted_converted": 0,
        "reference_converted": 0,
        "attained_single_engine_ceiling": 0,
        "demonstrated_solvable_union": 0,
        "both": 0,
        "accepted_only": 0,
        "reference_only": 0,
        "neither": 0,
    }
    total_positions = 0

    for family in accepted_families:
        accepted_positions = position_map(accepted, family)
        reference_positions = position_map(reference, family)
        if set(accepted_positions) != set(reference_positions):
            parser.error(f"{family}: position IDs differ")
        total_positions += len(accepted_positions)

        paired = {"both": 0, "accepted_only": 0, "reference_only": 0,
                  "neither": 0}
        clean_wins = 0
        for position_id, left in accepted_positions.items():
            right = reference_positions[position_id]
            for field in ("fen", "theory_wdl", "theory_dtz"):
                if left.get(field) != right.get(field):
                    parser.error(f"{family}/{position_id}: {field} differs")
            if left["theory_wdl"] != 2:
                continue
            clean_wins += 1
            accepted_won = left["outcome"] == "mated"
            reference_won = right["outcome"] == "mated"
            if accepted_won and reference_won:
                paired["both"] += 1
            elif accepted_won:
                paired["accepted_only"] += 1
            elif reference_won:
                paired["reference_only"] += 1
            else:
                paired["neither"] += 1

        accepted_count = paired["both"] + paired["accepted_only"]
        reference_count = paired["both"] + paired["reference_only"]
        if clean_wins != accepted["families"][family]["theoretically_won"]:
            parser.error(f"{family}: accepted win count disagrees with records")
        if clean_wins != reference["families"][family]["theoretically_won"]:
            parser.error(f"{family}: reference win count disagrees with records")
        if accepted_count != accepted["families"][family]["converted"]:
            parser.error(f"{family}: accepted aggregate disagrees with records")
        if reference_count != reference["families"][family]["converted"]:
            parser.error(f"{family}: reference aggregate disagrees with records")

        ceiling_count = max(accepted_count, reference_count)
        ceiling_source = (
            "tie" if accepted_count == reference_count
            else "accepted" if accepted_count > reference_count
            else "reference"
        )
        union_count = clean_wins - paired["neither"]
        families[family] = {
            "clean_wins": clean_wins,
            "accepted": {
                "converted": accepted_count,
                "rate": rate(accepted_count, clean_wins),
            },
            "reference": {
                "converted": reference_count,
                "rate": rate(reference_count, clean_wins),
            },
            "attained_single_engine_ceiling": {
                "converted": ceiling_count if clean_wins else None,
                "rate": rate(ceiling_count, clean_wins),
                "source": ceiling_source if clean_wins else None,
            },
            "demonstrated_solvable_union": {
                "converted": union_count if clean_wins else None,
                "rate": rate(union_count, clean_wins),
            },
            "accepted_gap_to_ceiling": (
                ceiling_count - accepted_count if clean_wins else None
            ),
            "paired": paired,
            "conversion_applicable": clean_wins > 0,
        }
        aggregate["clean_wins"] += clean_wins
        aggregate["accepted_converted"] += accepted_count
        aggregate["reference_converted"] += reference_count
        aggregate["attained_single_engine_ceiling"] += ceiling_count
        aggregate["demonstrated_solvable_union"] += union_count
        for key in paired:
            aggregate[key] += paired[key]

    if total_positions != accepted["cohort"]["position_count"]:
        parser.error(
            f"family records cover {total_positions} positions, expected "
            f"{accepted['cohort']['position_count']}"
        )

    total = aggregate["clean_wins"]
    aggregate["accepted_rate"] = rate(aggregate["accepted_converted"], total)
    aggregate["reference_rate"] = rate(aggregate["reference_converted"], total)
    aggregate["attained_single_engine_ceiling_rate"] = rate(
        aggregate["attained_single_engine_ceiling"], total
    )
    aggregate["demonstrated_solvable_union_rate"] = rate(
        aggregate["demonstrated_solvable_union"], total
    )

    output = {
        "schema": "basilisk-endgame-ceilings-v1",
        "definition": {
            "attained_single_engine_ceiling": (
                "best count attained by one complete engine run per family"
            ),
            "demonstrated_solvable_union": (
                "positions converted by either engine; stretch evidence only"
            ),
            "not_a_hard_maximum": True,
            "not_a_statistical_gate": True,
            "confidence_owner": "PLAN 6.0.d",
        },
        "contract": {field: accepted.get(field) for field in CONTRACT_FIELDS},
        "cohort": {
            field: accepted["cohort"][field]
            for field in ("manifest_sha256", "book_sha256", "position_count")
        },
        "inputs": {
            "accepted": {
                "path": str(args.accepted.resolve()),
                "report_sha256": sha256_file(args.accepted),
                "engine": accepted.get("engine_id", {}).get("name"),
                "engine_sha256": accepted["engine_sha256"],
            },
            "reference": {
                "path": str(args.reference.resolve()),
                "report_sha256": sha256_file(args.reference),
                "engine": reference.get("engine_id", {}).get("name"),
                "engine_sha256": reference["engine_sha256"],
            },
        },
        "aggregate": aggregate,
        "families": families,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(output, indent=2) + "\n", encoding="utf-8", newline="\n"
    )
    print(
        f"Wrote {args.output.resolve()}: {len(families)} families, "
        f"attained ceiling {aggregate['attained_single_engine_ceiling']}/{total}, "
        f"union {aggregate['demonstrated_solvable_union']}/{total}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
