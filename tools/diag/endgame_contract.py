#!/usr/bin/env python3
"""The endgame truth report's contract: schema, layers, cohort identity.

One module owns what every truth consumer must agree on, so the runner
(`endgame_truth.py`) and its readers (`endgame_compare.py`,
`endgame_vetoes.py`, `endgame_ceilings.py`) cannot drift apart.

* **Schema.** `basilisk-endgame-truth-v2` reports carry the four measurement
  layers, a per-family cohort digest and each game's move sequence. A
  `basilisk-endgame-truth-v1` report is read only when it is post-fix: v1
  first ended a game when the strong side shed material (BAS-E47), and the
  fix kept the v1 name. A post-fix record carries `shed_material_ply` and none
  ends `material_lost`; a pre-fix report is refused.
* **Cohort identity.** A family's digest is SHA-256 over `id<TAB>fen<LF>` for
  its positions in cohort order: the identity of a position set, with no
  engine, budget or result in it. It is recomputed from the records on every
  load, so a stored digest cannot vouch for records it does not describe.
* **Identity of two runs.** `identical` compares the per-position records of
  two reports family by family: a sharded run must reproduce the serial one,
  and a head must reproduce an earlier report before that report stands for
  it.

Usage:

  python tools/diag/endgame_contract.py identical A.json B.json
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

SCHEMA_V1 = "basilisk-endgame-truth-v1"
SCHEMA_V2 = "basilisk-endgame-truth-v2"

LAYERS = {
    "1_theory_truth": "first_discard_ply per graded move (Syzygy WDL)",
    "2_move_quality": "win_preserving_rate and dtz_progress_rate",
    "3_conversion": "converted / theoretically_won, per position",
    "4_game_strength": "NOT MEASURED HERE; requires a registered SPRT",
}


def family_digest(pairs) -> str:
    """SHA-256 over `id\\tfen\\n` for (id, fen) pairs in cohort order."""
    digest = hashlib.sha256()
    for position_id, fen in pairs:
        digest.update(f"{position_id}\t{fen}\n".encode("ascii"))
    return digest.hexdigest()


def overall_digest(family_digests: dict[str, str]) -> str:
    """Fold the per-family digests, in report order, into one cohort id."""
    digest = hashlib.sha256()
    for name, value in family_digests.items():
        digest.update(f"{name}\0{value}\0".encode("ascii"))
    return digest.hexdigest()


def record_id(record: dict):
    return record["id"] if "id" in record else record["index"]


def records_digest(records: list[dict]) -> str:
    return family_digest((record_id(r), r["fen"]) for r in records)


def contract_of(report: dict, label: str) -> str:
    """'v2' or 'v1-postfix'; raises ValueError for anything else."""
    schema = report.get("schema")
    families = report.get("families")
    if not isinstance(families, dict) or not families:
        raise ValueError(f"{label}: missing families")
    if schema == SCHEMA_V2:
        return "v2"
    if schema != SCHEMA_V1:
        raise ValueError(f"{label}: unsupported schema {schema!r}")
    for name, entry in families.items():
        if entry.get("outcomes", {}).get("material_lost"):
            raise ValueError(
                f"{label}: a pre-fix v1 report (family {name} ends games on a "
                "material shed); it is superseded and refused"
            )
        for record in entry.get("positions") or []:
            if "shed_material_ply" not in record or record.get("outcome") == "material_lost":
                raise ValueError(
                    f"{label}: a pre-fix v1 report (family {name} has records "
                    "without shed_material_ply); it is superseded and refused"
                )
        if not entry.get("positions"):
            raise ValueError(
                f"{label}: a v1 report without per-position records cannot be "
                "shown to be post-fix; refused"
            )
    return "v1-postfix"


def load_truth_report(path: Path, *, require_positions: bool = True) -> dict:
    """Load a truth report, check its contract and its cohort digests.

    Adds `_contract` and `_family_digests` (recomputed from the records).
    """
    try:
        report = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot read {path}: {exc}") from exc
    report["_contract"] = contract_of(report, str(path))
    digests = {}
    for name, entry in report["families"].items():
        records = entry.get("positions")
        if not isinstance(records, list) or not records:
            if require_positions:
                raise ValueError(f"{path}: {name} lacks per-position records")
            continue
        digests[name] = records_digest(records)
        stored = entry.get("cohort_sha256")
        if report["_contract"] == "v2" and stored != digests[name]:
            raise ValueError(
                f"{path}: {name}'s stored cohort digest does not describe its records"
            )
    report["_family_digests"] = digests
    return report


def identity_differences(left: dict, right: dict, limit: int = 10) -> list[str]:
    """Differences between the per-position records of left's families in right."""
    out = []
    for name, entry in left["families"].items():
        other = right["families"].get(name)
        if other is None:
            out.append(f"{name}: absent from the second report")
            continue
        a, b = entry.get("positions") or [], other.get("positions") or []
        if left["_family_digests"].get(name) != right["_family_digests"].get(name):
            out.append(f"{name}: cohort digests differ")
            continue
        for ra, rb in zip(a, b):
            keys = set(ra) | set(rb)
            # A v1 record has no move sequence; against one, the results decide.
            if "moves" not in ra or "moves" not in rb:
                keys.discard("moves")
            differing = sorted(k for k in keys if ra.get(k) != rb.get(k))
            if differing:
                out.append(f"{name}/{record_id(ra)}: {','.join(differing)} differ")
                if len(out) >= limit:
                    return out
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    same = sub.add_parser("identical", help="do A's families reproduce in B, record for record?")
    same.add_argument("first", type=Path)
    same.add_argument("second", type=Path)
    args = parser.parse_args()
    try:
        first = load_truth_report(args.first)
        second = load_truth_report(args.second)
    except ValueError as exc:
        parser.error(str(exc))
    differences = identity_differences(first, second)
    if differences:
        print("NOT IDENTICAL")
        for line in differences:
            print("  " + line)
        return 2
    count = sum(len(e["positions"]) for e in first["families"].values())
    print(f"IDENTICAL: {len(first['families'])} families, {count} positions "
          f"({first['_contract']} against {second['_contract']})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
