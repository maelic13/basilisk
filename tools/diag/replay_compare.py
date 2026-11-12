#!/usr/bin/env python3
"""Compare two Colosseum runs of one engine pair that replayed the same
openings (B.5.7, BAS-S58): the same binaries, book and master seed under two
conditions, such as 15 games on 15 cores against 28 games on 28 SMT siblings.

    python tools/diag/replay_compare.py <reference-run> <replay-run> [--json]

For each run: the pentanomial and Elo of engine A from its official sample,
time losses and faults from run-record.json, and the telemetry Colosseum
records per engine (nodes and non-mate depth per move, node rate).

For the pair of runs: the paired difference by opening. Pairs are matched by
their opening (games.jsonl `opening.label`), and the difference of engine A's
pair score is read with its own spread, so the openings' share of the
variance, which both runs carry, cancels. The Elo difference is the
difference of the two runs' Elo on the matched pairs; its interval is the
delta method on the paired score difference. The unpaired interval is printed
beside it to show what the pairing buys.

A stopped SPRT's sample was selected at its boundary; the reference's Elo is
then a first-crossing estimate, and so is the difference. The registration
states that bias; this tool reports the numbers.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

Z95 = 1.959963984540054


def pair_scores(run: Path) -> dict[str, float]:
    """Engine A's score in each complete official pair, keyed by opening."""
    games: dict[int, list[dict]] = {}
    for line in (run / "games.jsonl").read_text(encoding="utf-8").splitlines():
        if not line:
            continue
        game = json.loads(line)["game"]
        if game.get("sample") != "official" or not game.get("scorable"):
            continue
        games.setdefault(int(game["pair_number"]), []).append(game)
    scores: dict[str, float] = {}
    for number, pair in sorted(games.items()):
        if len(pair) != 2:
            continue
        label = pair[0]["opening"]["label"]
        if pair[1]["opening"]["label"] != label:
            raise SystemExit(f"{run}: pair {number} mixes two openings")
        if {pair[0]["white"], pair[1]["white"]} != {"a", "b"}:
            raise SystemExit(f"{run}: pair {number} does not swap colours")
        score = 0.0
        for game in pair:
            result = game["result"]
            if result == "Draw":
                score += 0.5
            elif (result == "WhiteWin") == (game["white"] == "a"):
                score += 1.0
            elif result not in ("WhiteWin", "BlackWin"):
                raise SystemExit(f"{run}: pair {number} has result {result!r}")
        if label in scores:
            raise SystemExit(f"{run}: opening {label!r} is played by two pairs")
        scores[label] = score
    return scores


def elo(score: float) -> float:
    score = min(max(score, 1e-9), 1 - 1e-9)
    return -400.0 * math.log10(1.0 / score - 1.0)


def elo_slope(score: float) -> float:
    return 400.0 / (math.log(10.0) * score * (1.0 - score))


def summary(values: list[float]) -> dict:
    n = len(values)
    mean = sum(values) / n
    variance = sum((v - mean) ** 2 for v in values) / (n - 1)
    score = mean / 2.0
    half = Z95 * math.sqrt(variance / n) / 2.0 * elo_slope(score)
    pentanomial = [sum(1 for v in values if v == k / 2.0) for k in range(5)]
    return {"pairs": n, "pentanomial": pentanomial, "score": round(score, 5),
            "elo": round(elo(score), 2), "elo_ci95": round(half, 2), "_variance": variance}


def telemetry(run: Path) -> dict:
    record = json.loads((run / "run-record.json").read_text(encoding="utf-8"))
    results = record["results"]
    out = {"time_losses": {t["engine"]: t["games"] for t in results["faults"]["time_losses"]},
           "faults": len(results["faults"]["faults"]),
           "infrastructure_faults": results["faults"]["infrastructure_faults"],
           "concurrency": record["workflow"]["execution"]["concurrency"],
           "allocation": record["workflow"]["execution"]["allocation"].get("mode"),
           "engines": {}}
    for engine in results["telemetry"]["engines"]:
        out["engines"][engine["engine"]] = {
            "nodes_per_move": round(engine["nodes"]["mean"]),
            "non_mate_depth": round(engine["non_mate_depth"]["mean"], 3),
            "nodes_per_second": round(engine["nodes_per_second"]),
            "ms_per_move": round(1000 * engine["elapsed_seconds"]["mean"], 2),
            "overhead_p99_ms": engine["harness_overhead_ms"].get("p99"),
        }
    return out


def compare(reference: Path, replay: Path) -> dict:
    ref_scores, rep_scores = pair_scores(reference), pair_scores(replay)
    common = [label for label in ref_scores if label in rep_scores]
    if len(common) < 100:
        raise SystemExit(f"only {len(common)} openings are common to both runs")
    ref_common = [ref_scores[label] for label in common]
    rep_common = [rep_scores[label] for label in common]
    diffs = [b - a for a, b in zip(ref_common, rep_common)]
    n = len(diffs)
    mean_diff = sum(diffs) / n
    var_diff = sum((d - mean_diff) ** 2 for d in diffs) / (n - 1)
    ref_s, rep_s = summary(ref_common), summary(rep_common)
    slope = elo_slope((ref_s["score"] + rep_s["score"]) / 2.0)
    paired_half = Z95 * math.sqrt(var_diff / n) / 2.0 * slope
    unpaired_half = Z95 * math.sqrt((ref_s["_variance"] + rep_s["_variance"]) / n) / 2.0 * slope
    ref_mean = sum(ref_common) / n
    rep_mean = sum(rep_common) / n
    covariance = sum((a - ref_mean) * (b - rep_mean) for a, b in zip(ref_common, rep_common)) / (n - 1)
    correlation = covariance / math.sqrt(ref_s["_variance"] * rep_s["_variance"])
    for s in (ref_s, rep_s):
        s.pop("_variance")
    ref_all, rep_all = summary(list(ref_scores.values())), summary(list(rep_scores.values()))
    for s in (ref_all, rep_all):
        s.pop("_variance")
    ref_t, rep_t = telemetry(reference), telemetry(replay)
    ratios = {}
    for name, values in rep_t["engines"].items():
        if name in ref_t["engines"]:
            base = ref_t["engines"][name]
            ratios[name] = {
                "nodes_per_move": round(values["nodes_per_move"] / base["nodes_per_move"], 4),
                "nodes_per_second": round(values["nodes_per_second"] / base["nodes_per_second"], 4),
                "non_mate_depth_delta": round(values["non_mate_depth"] - base["non_mate_depth"], 3),
            }
    return {
        "reference": {"run": str(reference), "all": ref_all, "matched": ref_s, "telemetry": ref_t},
        "replay": {"run": str(replay), "all": rep_all, "matched": rep_s, "telemetry": rep_t},
        "matched_openings": n,
        "elo_difference": round(rep_s["elo"] - ref_s["elo"], 2),
        "paired_ci95": round(paired_half, 2),
        "unpaired_ci95": round(unpaired_half, 2),
        "pair_score_correlation": round(correlation, 4),
        "telemetry_ratio_replay_over_reference": ratios,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("reference", type=Path)
    parser.add_argument("replay", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    result = compare(args.reference, args.replay)
    if args.json:
        print(json.dumps(result, indent=2))
        return 0
    for side in ("reference", "replay"):
        r = result[side]
        t = r["telemetry"]
        print(f"{side}: {r['run']}")
        print(f"  concurrency {t['concurrency']} ({t['allocation']}); time losses {t['time_losses']}; "
              f"faults {t['faults']}; infrastructure {t['infrastructure_faults']}")
        print(f"  all pairs {r['all']['pairs']}: {r['all']['elo']:+.1f} +/- {r['all']['elo_ci95']:.1f} Elo, "
              f"pentanomial {r['all']['pentanomial']}")
        for name, e in t["engines"].items():
            print(f"  {name}: {e['nodes_per_move']} nodes/move, depth {e['non_mate_depth']}, "
                  f"{e['nodes_per_second'] / 1e6:.3f}M nps, {e['ms_per_move']} ms/move, overhead p99 {e['overhead_p99_ms']} ms")
    print(f"matched openings: {result['matched_openings']}; pair-score correlation {result['pair_score_correlation']:+.3f}")
    print(f"Elo difference (replay - reference): {result['elo_difference']:+.2f} +/- {result['paired_ci95']:.2f} paired "
          f"(+/- {result['unpaired_ci95']:.2f} unpaired)")
    for name, ratio in result["telemetry_ratio_replay_over_reference"].items():
        print(f"  {name}: nodes/move x{ratio['nodes_per_move']}, nps x{ratio['nodes_per_second']}, "
              f"depth {ratio['non_mate_depth_delta']:+.3f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
