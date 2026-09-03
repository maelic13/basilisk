#!/usr/bin/env python3
"""Test whether king-to-passed-pawn distance explains Basilisk's pawn-family
truth failures, rather than assuming it does.

PLAN 6.3.a requires the feature to be DERIVED from Basilisk's own measured
failures, not copied from a reference engine's constants. A failure rate alone
does not do that: it says the engine loses these positions, not that king
approach is why. This reads an `endgame_truth.py` per-position report and asks
whether the geometry the proposed feature would act on actually separates the
positions Basilisk converts from the ones it does not.

For every clean-win root it computes, from the strong side's point of view:

  own_dist   Chebyshev distance from the strong king to its most advanced
             passed pawn (its own candidate for promotion)
  foe_dist   Chebyshev distance from the weak king to that same pawn
  race       own_dist - foe_dist, negative when the strong king is closer
  promo_gap  distance from that pawn to its promotion square

A feature is only justified if conversion separates along `race` or `own_dist`
AFTER holding the tablebase difficulty (`theory_dtz`) roughly constant --
otherwise the split just re-measures that hard positions are hard.
"""

from __future__ import annotations

import argparse
import collections
import json
import statistics
from pathlib import Path

import chess


def passed_pawns(board: chess.Board, color: chess.Color):
    """Pawns of `color` with no enemy pawn able to stop them on or beside their file."""
    out = []
    for square in board.pieces(chess.PAWN, color):
        file_index = chess.square_file(square)
        rank_index = chess.square_rank(square)
        blocked = False
        for enemy in board.pieces(chess.PAWN, not color):
            if abs(chess.square_file(enemy) - file_index) > 1:
                continue
            enemy_rank = chess.square_rank(enemy)
            ahead = enemy_rank > rank_index if color == chess.WHITE else enemy_rank < rank_index
            if ahead:
                blocked = True
                break
        if not blocked:
            out.append(square)
    return out


def geometry(fen: str):
    """Strong side is whoever has the pawns; None when that is ambiguous."""
    board = chess.Board(fen)
    white_pawns = len(board.pieces(chess.PAWN, chess.WHITE))
    black_pawns = len(board.pieces(chess.PAWN, chess.BLACK))
    if white_pawns == black_pawns:
        return None
    strong = chess.WHITE if white_pawns > black_pawns else chess.BLACK
    passers = passed_pawns(board, strong)
    if not passers:
        return None

    def advancement(square):
        rank = chess.square_rank(square)
        return rank if strong == chess.WHITE else 7 - rank

    pawn = max(passers, key=advancement)
    promo_rank = 7 if strong == chess.WHITE else 0
    promo = chess.square(chess.square_file(pawn), promo_rank)
    own = board.king(strong)
    foe = board.king(not strong)
    if own is None or foe is None:
        return None
    own_dist = chess.square_distance(own, pawn)
    foe_dist = chess.square_distance(foe, pawn)
    return {
        "own_dist": own_dist,
        "foe_dist": foe_dist,
        "race": own_dist - foe_dist,
        "promo_gap": chess.square_distance(pawn, promo),
        "passers": len(passers),
    }


def rate(rows):
    if not rows:
        return None
    return sum(r["converted"] for r in rows) / len(rows)


def bucket_report(title, rows, key, edges):
    print(f"\n  by {title}:")
    print("    %-12s %6s %10s %10s" % ("bucket", "n", "converted", "median dtz"))
    for low, high in edges:
        sel = [r for r in rows if low <= r["geom"][key] <= high]
        if not sel:
            continue
        label = f"{low}" if low == high else f"{low}..{high}"
        dtz = statistics.median([r["dtz"] for r in sel]) if sel else 0
        print("    %-12s %6d %9.1f%% %10.0f" % (
            label, len(sel), 100.0 * rate(sel), dtz))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, help="endgame_truth.py --per-position JSON")
    parser.add_argument("--dtz-band", type=int, nargs=2, default=(0, 200),
                        help="restrict to roots in this absolute DTZ range, to "
                             "hold tablebase difficulty roughly constant")
    args = parser.parse_args()

    data = json.loads(args.report.read_text(encoding="utf-8"))
    families = data["families"]

    rows = []
    skipped = collections.Counter()
    for name, family in families.items():
        for position in family.get("positions", []):
            if position.get("theory_wdl") != 2:
                skipped["not_clean_win"] += 1
                continue
            geom = geometry(position["fen"])
            if geom is None:
                skipped["no_strong_passer"] += 1
                continue
            rows.append({
                "family": name,
                "converted": position["outcome"] == "mated",
                "outcome": position["outcome"],
                "dtz": abs(position.get("theory_dtz") or 0),
                "geom": geom,
            })

    print("clean-win roots with an identifiable strong passer: %d" % len(rows))
    print("skipped: %s" % dict(skipped))
    if not rows:
        return 1

    print("\noverall conversion: %.1f%%" % (100.0 * rate(rows)))

    print("\n%-12s %6s %10s %12s %12s" % ("family", "n", "converted", "med own_dist", "med race"))
    by_family = collections.defaultdict(list)
    for r in rows:
        by_family[r["family"]].append(r)
    for name in sorted(by_family, key=lambda k: rate(by_family[k])):
        sel = by_family[name]
        print("%-12s %6d %9.1f%% %12.1f %12.1f" % (
            name, len(sel), 100.0 * rate(sel),
            statistics.median([r["geom"]["own_dist"] for r in sel]),
            statistics.median([r["geom"]["race"] for r in sel])))

    low, high = args.dtz_band
    band = [r for r in rows if low <= r["dtz"] <= high]
    print(f"\n--- all roots (n={len(rows)}) ---")
    bucket_report("strong king distance to its most advanced passer", rows,
                  "own_dist", [(0, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 7)])
    bucket_report("race (own_dist - foe_dist; negative = strong king closer)", rows,
                  "race", [(-7, -3), (-2, -1), (0, 0), (1, 2), (3, 7)])

    print(f"\n--- DTZ band {low}..{high} only (n={len(band)}), difficulty held down ---")
    bucket_report("strong king distance", band, "own_dist",
                  [(0, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 7)])
    bucket_report("race", band, "race",
                  [(-7, -3), (-2, -1), (0, 0), (1, 2), (3, 7)])

    print("\nfailures by outcome:")
    bad = collections.Counter(r["outcome"] for r in rows if not r["converted"])
    for outcome, count in bad.most_common():
        print("  %-22s %d" % (outcome, count))

    print("\nA king-approach feature is justified only if conversion separates")
    print("along these axes INSIDE the DTZ band. If it separates only across all")
    print("roots, the split is re-measuring that distant kings mean harder")
    print("positions, which a king-distance term cannot fix.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
