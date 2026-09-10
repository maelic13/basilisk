#!/usr/bin/env python3
"""Validate and summarize the PLAN 6.1.e held-out KBNK confirmation.

6.1.c selected `15600,1750,0,340,0` on cohort positions 1-60 after three
rounds and 42 arms, so that figure is a development estimate contaminated by
selection. This step re-runs the full frozen 198-position cohort and decides
on positions 61-198, which no arm has ever been chosen against. Positions
1-60 are reported only to quantify the shrinkage, and can never rescue a
failed held-out verdict.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

try:
    from .kbnk_sweep_summary import HARD_OUTCOMES
except ImportError:
    from kbnk_sweep_summary import HARD_OUTCOMES

BASELINE = "legacy-baseline"
VARIANTS = {
    BASELINE: "15600,800,900,220,220",
    "d1750-k340": "15600,1750,0,340,0",
    "d1900-k460": "15600,1900,0,460,0",
}

# Development split is the first 60 cohort records, exactly the rows 6.1.c
# selected on; the held-out split is everything after them.
DEVELOPMENT_POSITIONS = 60

# Control drift gates. The first-60 value is the published BAS-E39 legacy
# fingerprint, so matching it proves continuity with the screen this step
# confirms. The all-198 value was derived during 6.1.e preparation on commit
# 5a8da16 and verified to contain those first-60 records unchanged, which is
# what licenses splitting one run into development and held-out halves.
# Recomputed 2026-09-04 over FINGERPRINT_FIELDS from the same stored 6.1.e
# legacy-control report. The engine did not change: every shared field of every
# record still matches the pre-fix artifacts exactly, verified position by
# position. Only the hashing basis changed, from whole records to behavioural
# fields. The superseded whole-record values were
# first_60 A28D2843... and all_198 8CCB2C20...
CONTROL_FINGERPRINTS = {
    "first_60": "436CC064F3921E9E01473169D4A4014794AA1028E49BE30022EF5E071F327B34",
    "all_198": "E0A8D67321093315B73B6A9F4CE5A8843FC155572FB8BFA40ACFB5C4A65571E8",
}

# Pre-registered before any candidate result exists.
ACCEPT_Z = 2.0


# Hash BEHAVIOUR, not record layout. The first version hashed whole records, so
# adding one diagnostic field to the report schema -- `shed_material_ply`, when
# the material-abort rule was fixed -- made every frozen control fingerprint
# permanently unreachable even though the engine had not changed at all. These
# are the fields that describe what the engine did; a new diagnostic column must
# not void a control.
FINGERPRINT_FIELDS = (
    "id", "outcome", "plies", "graded_moves", "win_preserving_moves",
    "dtz_checked_moves", "dtz_progress_moves", "first_discard_ply", "anomaly",
)


def fingerprint(positions):
    reduced = [
        {k: p.get(k) for k in FINGERPRINT_FIELDS} for p in positions
    ]
    canonical = json.dumps(reduced, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(canonical).hexdigest().upper()


def paired_z(gained: int, lost: int) -> float:
    """McNemar-style z on the discordant pairs; 0.0 when nothing is discordant."""
    discordant = gained + lost
    return (gained - lost) / math.sqrt(discordant) if discordant else 0.0


def split_stats(positions, base):
    converted = sum(p["outcome"] == "mated" for p in positions)
    gained = sum(
        base[p["id"]]["outcome"] != "mated" and p["outcome"] == "mated"
        for p in positions
    )
    lost = sum(
        base[p["id"]]["outcome"] == "mated" and p["outcome"] != "mated"
        for p in positions
    )
    discards = sum(p.get("first_discard_ply") is not None for p in positions)
    # BAS-E35: historical discards occur only at plies 94-98, when the win is
    # already dying at the rule-50 boundary. Anything before ply 80 is a new
    # live truth failure and is a correctness veto, not a ranking input.
    live = sum(
        p.get("first_discard_ply") is not None and p["first_discard_ply"] < 80
        for p in positions
    )
    hard = sum(
        p["outcome"] in HARD_OUTCOMES or p.get("anomaly") is not None
        for p in positions
    )
    graded = sum(p["graded_moves"] for p in positions)
    preserved = sum(p["win_preserving_moves"] for p in positions)
    checked = sum(p["dtz_checked_moves"] for p in positions)
    progress = sum(p["dtz_progress_moves"] for p in positions)
    mate_plies = sorted(p["plies"] for p in positions if p["outcome"] == "mated")
    return {
        "n": len(positions),
        "converted": converted,
        "conversion_rate": converted / len(positions) if positions else 0.0,
        "paired_gained": gained,
        "paired_lost": lost,
        "paired_z": round(paired_z(gained, lost), 3),
        "discarded_clean_wins": discards,
        "live_truth_discards": live,
        "hard_anomalies": hard,
        "win_preserving_rate": preserved / graded if graded else 0.0,
        "dtz_progress_rate": progress / checked if checked else 0.0,
        "median_mate_plies": (
            float(mate_plies[len(mate_plies) // 2]) if mate_plies else None
        ),
        "fifty_move": sum(p["outcome"] == "fifty_move" for p in positions),
        "stalemate": sum(p["outcome"] == "stalemate" for p in positions),
    }


def load(result_dir: Path, allow_control_drift: bool = False):
    reports = {}
    for name, weights in VARIANTS.items():
        path = result_dir / (name + ".json")
        if not path.is_file():
            raise ValueError("missing variant report: " + str(path))
        report = json.loads(path.read_text(encoding="utf-8"))
        if report.get("engine_options", {}).get("KBNK Drive") != weights:
            raise ValueError(name + ": KBNK Drive provenance does not match registry")
        reports[name] = report

    base = reports[BASELINE]
    base_ids = [p["id"] for p in base["families"]["KBN-K"]["positions"]]
    if len(base_ids) <= DEVELOPMENT_POSITIONS:
        raise ValueError(
            "cohort has %d records; a held-out split needs more than the %d "
            "development rows" % (len(base_ids), DEVELOPMENT_POSITIONS)
        )
    for name, report in reports.items():
        for key in ("engine_sha256", "nodes_per_move", "max_plies", "hash_mb"):
            if report.get(key) != base.get(key):
                raise ValueError(name + ": mismatched " + key)
        if report.get("cohort", {}).get("book_sha256") != base["cohort"]["book_sha256"]:
            raise ValueError(name + ": mismatched frozen cohort")
        if [p["id"] for p in report["families"]["KBN-K"]["positions"]] != base_ids:
            raise ValueError(name + ": position pairing/order differs")

    base_positions = base["families"]["KBN-K"]["positions"]
    actual = {
        "first_60": fingerprint(base_positions[:DEVELOPMENT_POSITIONS]),
        "all_198": fingerprint(base_positions),
    }
    for key, expected in CONTROL_FINGERPRINTS.items():
        if actual[key] != expected:
            if allow_control_drift:
                print("control fingerprint %s differs (%s); permitted because "
                      "this run uses a different node budget, for which the "
                      "frozen 60,000-node fingerprints cannot apply"
                      % (key, actual[key][:16]))
                continue
            raise ValueError(
                "control drift on %s: %s != %s; the run is invalid and no "
                "candidate may be read from it" % (key, actual[key], expected)
            )
    return reports


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result_dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--allow-control-drift",
        action="store_true",
        help="skip the frozen 60,000-node control fingerprints. ONLY legitimate "
             "for a run at a different node budget, where those fingerprints "
             "cannot apply by construction. Never pass this to excuse an "
             "unexplained mismatch at the registered budget.",
    )
    args = parser.parse_args()
    try:
        reports = load(args.result_dir, args.allow_control_drift)
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        parser.error(str(exc))

    base_by_id = {
        p["id"]: p for p in reports[BASELINE]["families"]["KBN-K"]["positions"]
    }
    rows = []
    for name, weights in VARIANTS.items():
        positions = reports[name]["families"]["KBN-K"]["positions"]
        rows.append({
            "variant": name,
            "uci_kbnk_drive": weights,
            "development": split_stats(positions[:DEVELOPMENT_POSITIONS], base_by_id),
            "held_out": split_stats(positions[DEVELOPMENT_POSITIONS:], base_by_id),
            "all": split_stats(positions, base_by_id),
        })

    # Pre-registered verdict, evaluated on the held-out split only.
    verdicts = {}
    for row in rows:
        if row["variant"] == BASELINE:
            continue
        held = row["held_out"]
        blocking = []
        if held["hard_anomalies"] or row["all"]["hard_anomalies"]:
            blocking.append("hard anomaly")
        if held["live_truth_discards"] or row["all"]["live_truth_discards"]:
            blocking.append("live truth discard before ply 80")
        if blocking:
            verdicts[row["variant"]] = "REJECTED: " + "; ".join(blocking)
        elif held["paired_z"] >= ACCEPT_Z:
            verdicts[row["variant"]] = "CONFIRMED on held-out"
        else:
            verdicts[row["variant"]] = (
                "NOT CONFIRMED: held-out paired z %+.2f < %.1f"
                % (held["paired_z"], ACCEPT_Z)
            )

    summary = {
        "schema": "basilisk-kbnk-holdout-v1",
        "purpose": "6.1.e held-out confirmation; positions 61-198 decide",
        "engine_sha256": reports[BASELINE]["engine_sha256"],
        "cohort_book_sha256": reports[BASELINE]["cohort"]["book_sha256"],
        "development_positions": DEVELOPMENT_POSITIONS,
        "accept_paired_z": ACCEPT_Z,
        "control_fingerprints": CONTROL_FINGERPRINTS,
        "nodes_per_move": reports[BASELINE]["nodes_per_move"],
        "max_plies": reports[BASELINE]["max_plies"],
        "workers": reports[BASELINE]["workers"],
        "verdict_policy": [
            "controls must reproduce both frozen fingerprints or the run is void",
            "reject any arm with a hard anomaly",
            "reject any arm with a live truth discard before ply 80",
            "primary: paired conversion on held-out positions 61-198, accept at z >= 2.0",
            "candidates separated only at |z| >= 2.0; otherwise prefer fewer clean-win "
            "discards, then higher DTZ progress, then the simpler vector",
            "development rows 1-60 quantify shrinkage only and cannot rescue a failure",
            "all-198 aggregate, rule-50, stalemate, preservation and mate efficiency "
            "are diagnostic, not the verdict",
        ],
        "variants": rows,
        "verdicts": verdicts,
    }
    output = args.output or args.result_dir / "summary.json"
    output.write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8", newline="\n"
    )

    header = ("variant", "split", "conv", "gain/loss", "z", "disc", "live",
              "hard", "r50", "stale")
    print("%-17s%-12s%9s%11s%7s%6s%6s%6s%6s%6s" % header)
    for row in rows:
        for split in ("development", "held_out", "all"):
            s = row[split]
            print("%-17s%-12s%4d/%-4d%5d/%-5d%+7.2f%6d%6d%6d%6d%6d" % (
                row["variant"] if split == "development" else "",
                split, s["converted"], s["n"], s["paired_gained"],
                s["paired_lost"], s["paired_z"], s["discarded_clean_wins"],
                s["live_truth_discards"], s["hard_anomalies"],
                s["fifty_move"], s["stalemate"],
            ))

    print("\nHead-to-head on held-out positions 61-198:")
    candidates = [r for r in rows if r["variant"] != BASELINE]
    for i, first in enumerate(candidates):
        for second in candidates[i + 1:]:
            left = reports[first["variant"]]["families"]["KBN-K"]["positions"]
            right = reports[second["variant"]]["families"]["KBN-K"]["positions"]
            pairs = list(zip(left[DEVELOPMENT_POSITIONS:], right[DEVELOPMENT_POSITIONS:]))
            wins = sum(a["outcome"] == "mated" and b["outcome"] != "mated" for a, b in pairs)
            losses = sum(a["outcome"] != "mated" and b["outcome"] == "mated" for a, b in pairs)
            z = paired_z(wins, losses)
            verdict = "separated" if abs(z) >= ACCEPT_Z else "INDISTINGUISHABLE"
            print("  %s vs %s: %d/%d discordant, z %+.2f -> %s" % (
                first["variant"], second["variant"], wins, losses, z, verdict))

    print("\nShrinkage from the 6.1.c development rows to held-out:")
    for row in candidates:
        dev = row["development"]["conversion_rate"]
        held = row["held_out"]["conversion_rate"]
        print("  %-12s %6.1f%% -> %6.1f%%  (%+.1f pp)" % (
            row["variant"], dev * 100, held * 100, (held - dev) * 100))

    print("\nPre-registered verdicts:")
    for name, verdict in verdicts.items():
        print("  %s: %s" % (name, verdict))
    print("\nSummary: %s" % output.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
