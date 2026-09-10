#!/usr/bin/env python3
"""Per-class evaluation bias on the DRAWN subset (PLAN 6.5.a, BAS-E32).

A scaling function exists to recognise that a materially-winning position is in
fact drawn. A per-class mean loss cannot see whether it is needed: if a class is
40% decisive and those positions are scored correctly, the mean looks healthy
while every drawn position is called a win. BAS-E27 made exactly that mistake
and concluded the rook endings needed nothing; BAS-E32 split each class by the
actual game result and found the opposite.

This rebuilds that measurement so 6.5.a can be gated on it rather than on a
number quoted from a retired script.

  metric   mean predicted score over the positions whose GAME WAS DRAWN,
           oriented to the stronger side, minus 0.5.

Orientation matters and is the whole point: without it, positions where White
is the stronger side and positions where Black is cancel, and every class reads
as unbiased. A draw scores 0.5 whoever is to move, so only the prediction needs
orienting.

*What the metric is not.* A position from a drawn game is not necessarily a
theoretical draw -- somebody may have thrown a win, and a position in a won game
may have been objectively drawn. This is a population statistic over 52,632
holdout rows, not per-position truth, and it says nothing about any single
position. It is the right instrument for a recogniser precisely because a
recogniser is a population claim: it fires on a class.

Inputs are the holdout CSV (`fen;result`) and the matching
`basilisk-texel --dump-eval` output (`white_pov_score result`), which the tuner
writes in input order. The results column appears in both, so the join is
checked line by line rather than assumed.
"""

from __future__ import annotations

import argparse
import json
import math
from collections import defaultdict
from pathlib import Path

# Fitted in BAS-E27 on this same holdout; kept so the numbers are comparable.
DEFAULT_K = 1.90906

PIECE_ORDER = "QRBNP"


def class_of(fen: str) -> tuple[str, bool]:
    """Canonical class name and whether WHITE is the stronger side.

    'Stronger' is by non-pawn material first, then pawns, then a stable
    tiebreak on the piece string so that symmetric classes are not orientated
    at random from row to row.
    """
    board = fen.split()[0]
    counts = {"w": defaultdict(int), "b": defaultdict(int)}
    for ch in board:
        if ch.isalpha() and ch.upper() != "K":
            counts["w" if ch.isupper() else "b"][ch.upper()] += 1

    def value(side):
        return (9 * counts[side]["Q"] + 5 * counts[side]["R"]
                + 3 * counts[side]["B"] + 3 * counts[side]["N"])

    def text(side):
        return "K" + "".join(p * counts[side][p] for p in PIECE_ORDER)

    wv, bv = value("w"), value("b")
    wp, bp = counts["w"]["P"], counts["b"]["P"]
    wt, bt = text("w"), text("b")
    if (wv, wp) != (bv, bp):
        white_strong = (wv, wp) > (bv, bp)
    else:
        white_strong = wt >= bt
    strong, weak = (wt, bt) if white_strong else (bt, wt)
    return f"{strong}-{weak}", white_strong


def expected(score_cp: float, k: float) -> float:
    return 1.0 / (1.0 + 10.0 ** (-k * score_cp / 400.0))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dataset", required=True, type=Path,
                        help="holdout CSV, 'fen;result' per line")
    parser.add_argument("--evals", required=True, type=Path,
                        help="basilisk-texel --dump-eval output for that dataset")
    parser.add_argument("--k", type=float, default=DEFAULT_K)
    parser.add_argument("--min-n", type=int, default=100,
                        help="classes with fewer rows are not reported")
    parser.add_argument("--only", action="append",
                        help="report only these classes (repeatable)")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    fens, results = [], []
    for line in args.dataset.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        fen, _, result = line.rpartition(";")
        fens.append(fen)
        results.append(float(result))

    scores, dump_results = [], []
    for line in args.evals.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        score, result = line.split()
        scores.append(int(score))
        dump_results.append(float(result))

    if not (len(fens) == len(scores) == len(dump_results)):
        parser.error("dataset and eval dump have different lengths: "
                     f"{len(fens)} vs {len(scores)}")
    # The dump repeats the result, so the join can be checked rather than
    # assumed. A silent off-by-one here would corrupt every number below.
    mismatch = sum(1 for a, b in zip(results, dump_results) if a != b)
    if mismatch:
        parser.error(f"{mismatch} rows disagree on the result column; "
                     "the eval dump does not correspond to this dataset")

    rows = defaultdict(list)
    for fen, result, score in zip(fens, results, scores):
        name, white_strong = class_of(fen)
        strong_score = score if white_strong else -score
        strong_result = result if white_strong else 1.0 - result
        rows[name].append((strong_score, strong_result))

    table = []
    for name, items in rows.items():
        if args.only and name not in args.only:
            continue
        if len(items) < args.min_n and not args.only:
            continue
        drawn = [s for s, r in items if r == 0.5]
        if not drawn:
            continue
        predicted_drawn = sum(expected(s, args.k) for s in drawn) / len(drawn)
        table.append({
            "class": name,
            "n": len(items),
            "drawn_n": len(drawn),
            "drawn_share": len(drawn) / len(items),
            "predicted_on_drawn": predicted_drawn,
            "bias": predicted_drawn - 0.5,
            "mean_eval_drawn": sum(drawn) / len(drawn),
        })
    table.sort(key=lambda r: -r["bias"])

    print("Evaluation bias on the DRAWN subset, oriented to the stronger side")
    print("K = %.5f, %d rows, %d classes reported"
          % (args.k, len(fens), len(table)))
    print()
    print("%-12s %7s %7s %11s %12s %8s %10s"
          % ("class", "n", "drawn", "drawn share", "we predict", "bias", "mean cp"))
    for row in table:
        print("%-12s %7d %7d %10.1f%% %12.3f %+8.3f %10.0f"
              % (row["class"], row["n"], row["drawn_n"],
                 100 * row["drawn_share"], row["predicted_on_drawn"],
                 row["bias"], row["mean_eval_drawn"]))

    print("\nA positive bias means we call drawn positions won. That is what a")
    print("scaling function exists to remove. It is a population statistic over")
    print("game outcomes, not per-position truth.")

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(
            {"k": args.k, "rows": len(fens), "dataset": str(args.dataset),
             "classes": table}, indent=2) + "\n",
            encoding="utf-8", newline="\n")
        print(f"\nWrote {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
