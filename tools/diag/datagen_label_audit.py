#!/usr/bin/env python3
"""Measure how often datagen's WDL labels contradict tablebase truth.

Texel labels are game RESULTS. That is sound only if the games decide
theoretically won endings correctly. If 8,000-node self-play routinely fails to
convert a won ending, the corpus teaches the evaluator that won positions are
draws -- precisely in the families PLAN 6.1 just improved, and precisely where
Phase 7's post-endgame refit will read them.

This walks each game to the FIRST position both sides can still reach that the
tablebase can adjudicate, and compares the tablebase verdict with how the game
actually ended.

Two distinctions the count depends on, both easy to get wrong:

* Syzygy at `syzygy3456` covers up to SIX men, not seven. The 6.0.g occurrence
  census counts a <=7-man band, which is a different denominator; probing a
  7-man position here would raise MissingTableError, so 6 is the cutoff.
* WDL 2 is a clean win and WDL 1 is a CURSED win -- already unwinnable under the
  fifty-move rule. Only clean wins can be called mislabelled; cursed wins are
  counted separately and are not evidence of weak play.

Usage:
  python tools/diag/datagen_label_audit.py --pgn tools/texel/data/armA_basilisk8k.pgn \\
      --syzygy D:/chess/tablebases/syzygy3456 --max-games 20000
"""

from __future__ import annotations

import argparse
import collections
import json
import sys
from pathlib import Path

import chess
import chess.pgn
import chess.syzygy

MAX_TB_MEN = 6
RESULT_POINTS = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}


def material_key(board: chess.Board) -> str:
    """`KRP-K` style signature, strong side first by piece count."""
    def side(color):
        out = ""
        for piece in (chess.QUEEN, chess.ROOK, chess.BISHOP, chess.KNIGHT, chess.PAWN):
            out += chess.piece_symbol(piece).upper() * len(board.pieces(piece, color))
        return "K" + out
    white, black = side(chess.WHITE), side(chess.BLACK)
    return f"{white}-{black}" if len(white) >= len(black) else f"{black}-{white}"


def audit(pgn_path: Path, tb, max_games: int, stride: int) -> dict:
    stats = collections.Counter()
    families = collections.defaultdict(collections.Counter)
    seen = 0
    with pgn_path.open(encoding="utf-8", errors="replace") as stream:
        while stats["scanned"] < max_games:
            try:
                game = chess.pgn.read_game(stream)
            except (ValueError, RuntimeError):
                continue
            if game is None:
                break
            seen += 1
            if (seen - 1) % stride:
                continue
            result = game.headers.get("Result", "*")
            if result not in RESULT_POINTS:
                stats["unfinished"] += 1
                continue
            stats["scanned"] += 1

            board = game.board()
            verdict = None
            for move in game.mainline_moves():
                board.push(move)
                if chess.popcount(board.occupied) > MAX_TB_MEN:
                    continue
                try:
                    wdl = tb.probe_wdl(board)
                except (chess.syzygy.MissingTableError, KeyError, ValueError):
                    stats["probe_failed"] += 1
                    break
                # Convert to White's perspective so it can be compared with the
                # PGN result directly.
                white_wdl = wdl if board.turn == chess.WHITE else -wdl
                verdict = (white_wdl, material_key(board))
                break

            if verdict is None:
                stats["never_adjudicable"] += 1
                continue

            white_wdl, family = verdict
            points = RESULT_POINTS[result]
            stats["adjudicable"] += 1

            if white_wdl == 2 or white_wdl == -2:
                stats["clean_win_reached"] += 1
                won_by_white = white_wdl == 2
                expected = 1.0 if won_by_white else 0.0
                if points == expected:
                    stats["clean_win_converted"] += 1
                    families[family]["converted"] += 1
                elif points == 0.5:
                    stats["clean_win_drawn"] += 1
                    families[family]["drawn"] += 1
                else:
                    # The side that was winning went on to LOSE.
                    stats["clean_win_lost"] += 1
                    families[family]["lost"] += 1
                families[family]["reached"] += 1
            elif abs(white_wdl) == 1:
                stats["cursed_win_reached"] += 1
            else:
                stats["drawn_position"] += 1
                if points != 0.5:
                    stats["drawn_position_decided"] += 1

    reached = stats["clean_win_reached"]
    mislabelled = stats["clean_win_drawn"] + stats["clean_win_lost"]
    return {
        "pgn": str(pgn_path),
        "counts": dict(stats),
        "clean_win_reached": reached,
        "clean_win_mislabelled": mislabelled,
        "mislabel_rate": (mislabelled / reached) if reached else None,
        "families": {
            k: dict(v) for k, v in sorted(
                families.items(), key=lambda kv: -kv[1]["reached"])[:15]
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pgn", type=Path, action="append", required=True,
                        help="repeatable; one datagen PGN per corpus to compare")
    parser.add_argument("--syzygy", type=Path, required=True)
    parser.add_argument("--max-games", type=int, default=20000)
    parser.add_argument("--stride", type=int, default=1,
                        help="audit every Nth game, to spread the sample across "
                             "the whole file instead of taking a prefix")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()

    if args.max_games <= 0 or args.stride <= 0:
        parser.error("max-games and stride must be positive")
    if not args.syzygy.is_dir():
        parser.error(f"syzygy directory not found: {args.syzygy}")
    for pgn in args.pgn:
        if not pgn.is_file():
            parser.error(f"PGN not found: {pgn}")

    reports = []
    with chess.syzygy.open_tablebase(str(args.syzygy)) as tb:
        for pgn in args.pgn:
            print(f"auditing {pgn.name} ...", file=sys.stderr)
            reports.append(audit(pgn, tb, args.max_games, args.stride))

    print()
    print("%-26s %8s %10s %9s %8s %8s" % (
        "corpus", "games", "cleanwin", "converted", "drawn", "lost"))
    for r in reports:
        c = r["counts"]
        print("%-26s %8d %10d %9d %8d %8d" % (
            Path(r["pgn"]).name, c.get("scanned", 0), r["clean_win_reached"],
            c.get("clean_win_converted", 0), c.get("clean_win_drawn", 0),
            c.get("clean_win_lost", 0)))

    print()
    print("%-26s %14s %14s" % ("corpus", "mislabel rate", "cursed (excl.)"))
    for r in reports:
        rate = r["mislabel_rate"]
        print("%-26s %13s %14d" % (
            Path(r["pgn"]).name,
            "n/a" if rate is None else f"{rate:.2%}",
            r["counts"].get("cursed_win_reached", 0)))

    print()
    print("Mislabelled = the game reached a tablebase CLEAN win (WDL 2) and then")
    print("did not win it. Cursed wins (WDL 1) are excluded: they are already")
    print("drawn under the fifty-move rule and are not evidence of weak play.")
    print("A rate that barely moves between node budgets means a bigger datagen")
    print("budget buys no label quality and should not be paid for.")

    for r in reports:
        print()
        print(f"--- {Path(r['pgn']).name}: families by clean wins reached ---")
        for family, counts in r["families"].items():
            reached = counts.get("reached", 0)
            bad = counts.get("drawn", 0) + counts.get("lost", 0)
            print("  %-14s reached %6d  mislabelled %5d  (%.1f%%)" % (
                family, reached, bad, 100.0 * bad / reached if reached else 0.0))

    if args.out:
        args.out.write_text(json.dumps(reports, indent=2) + "\n",
                            encoding="utf-8", newline="\n")
        print(f"\nWrote {args.out.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
