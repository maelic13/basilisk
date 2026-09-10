#!/usr/bin/env python3
"""Profile WHY the engine throws won rook endings, with a control.

PLAN 6.5.a. The five first-move throws in KRPP-KRP suggested three candidate
causes -- an ignored enemy passer, a rook abandoning a rank or file it must
hold, and plain only-move precision. Listing features of the losing moves
cannot distinguish them: any feature common in the family will look common in
its failures too.

So this replays every clean-win game and classifies each White-to-move node
where the tablebase still says WIN into two groups:

    PRESERVED  the engine's move kept the win
    THREW      the engine's move gave it away

and reports each feature's rate in BOTH groups. A cause has to separate them.
That is the same control 6.3.a used to kill an apparent king-distance effect
that turned out to be family composition.

Features per node:

  win_moves        how many legal moves preserve the win (only-move pressure)
  foe_passer_dist  distance from the enemy's most advanced passed pawn to its
                   promotion square, or None when the enemy has no passer
  chose_pawn       the engine moved a pawn
  chose_king       the engine moved the king
  king_closes      the move reduced our king's distance to the enemy passer
  rook_to_file     the move placed our rook on the enemy passer's file
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path

import chess
import chess.engine
import chess.syzygy


def passed_pawns(board: chess.Board, color: chess.Color):
    out = []
    for square in board.pieces(chess.PAWN, color):
        file_index = chess.square_file(square)
        rank_index = chess.square_rank(square)
        if any(
            abs(chess.square_file(e) - file_index) <= 1
            and ((chess.square_rank(e) > rank_index) if color == chess.WHITE
                 else (chess.square_rank(e) < rank_index))
            for e in board.pieces(chess.PAWN, not color)
        ):
            continue
        out.append(square)
    return out


def foe_passer(board: chess.Board):
    """Enemy passer closest to promoting, and its distance to the queening square."""
    best, best_dist = None, None
    for square in passed_pawns(board, chess.BLACK):
        dist = chess.square_rank(square)          # Black promotes on rank 0
        if best_dist is None or dist < best_dist:
            best, best_dist = square, dist
    return best, best_dist


def features(board: chess.Board, move: chess.Move, win_moves: int) -> dict:
    passer, dist = foe_passer(board)
    piece = board.piece_at(move.from_square)
    own_king = board.king(chess.WHITE)
    row = {
        "win_moves": win_moves,
        "foe_passer_dist": dist,
        "chose_pawn": piece.piece_type == chess.PAWN,
        "chose_king": piece.piece_type == chess.KING,
        "king_closes": False,
        "rook_to_file": False,
    }
    if passer is not None:
        if piece.piece_type == chess.KING and own_king is not None:
            row["king_closes"] = (chess.square_distance(move.to_square, passer)
                                  < chess.square_distance(own_king, passer))
        if piece.piece_type == chess.ROOK:
            row["rook_to_file"] = (chess.square_file(move.to_square)
                                   == chess.square_file(passer))
    return row


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--syzygy", required=True, action="append", type=Path)
    parser.add_argument("--cohort", required=True, type=Path)
    parser.add_argument("--family", required=True)
    parser.add_argument("--nodes", type=int, default=60000)
    parser.add_argument("--max-plies", type=int, default=100)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    manifest = json.loads(args.cohort.read_text(encoding="utf-8"))
    records = [r for r in manifest["records"]
               if r["family"] == args.family and r["theory_wdl"] == 2]
    if not records:
        parser.error(f"no clean wins for {args.family} in {args.cohort}")

    tb = chess.syzygy.open_tablebase(str(args.syzygy[0]))
    for extra in args.syzygy[1:]:
        tb.add_directory(str(extra))
    engine = chess.engine.SimpleEngine.popen_uci(str(args.engine))
    engine.configure({"Hash": 16, "Threads": 1})

    groups = {"PRESERVED": [], "THREW": []}
    thrown_ids = []
    for record in records:
        board = chess.Board(record["fen"])
        token = object()
        for _ply in range(args.max_plies):
            if board.is_game_over() or chess.popcount(board.occupied) > 7:
                break
            white = board.turn == chess.WHITE
            try:
                still_won = (tb.probe_wdl(board) if white else -tb.probe_wdl(board)) == 2
            except (chess.syzygy.MissingTableError, KeyError, ValueError):
                break
            move = engine.play(board, chess.engine.Limit(nodes=args.nodes),
                               game=token).move
            if move is None:
                break
            if white and still_won:
                keep = []
                for candidate in board.legal_moves:
                    board.push(candidate)
                    try:
                        ok = -tb.probe_wdl(board) == 2
                    except (chess.syzygy.MissingTableError, KeyError, ValueError):
                        ok = False
                    board.pop()
                    if ok:
                        keep.append(candidate)
                row = features(board, move, len(keep))
                if move in keep:
                    groups["PRESERVED"].append(row)
                else:
                    groups["THREW"].append(row)
                    thrown_ids.append(record["id"])
                    board.push(move)
                    break
            board.push(move)
    engine.quit()
    tb.close()

    def rate(rows, key):
        vals = [r[key] for r in rows if r[key] is not None]
        if not vals:
            return None
        if isinstance(vals[0], bool):
            return sum(vals) / len(vals)
        return sum(vals) / len(vals)

    print("%-9s graded nodes: PRESERVED %d, THREW %d, games thrown %d"
          % (args.family, len(groups["PRESERVED"]), len(groups["THREW"]),
             len(set(thrown_ids))))
    print()
    print("%-18s %12s %12s %10s" % ("feature", "PRESERVED", "THREW", "separates?"))
    out = {"family": args.family, "nodes": args.nodes,
           "preserved": len(groups["PRESERVED"]), "threw": len(groups["THREW"]),
           "features": {}}
    for key in ("win_moves", "foe_passer_dist", "chose_pawn", "chose_king",
                "king_closes", "rook_to_file"):
        a, b = rate(groups["PRESERVED"], key), rate(groups["THREW"], key)
        if a is None or b is None:
            print("%-18s %12s %12s %10s" % (key, a, b, "n/a"))
            continue
        # A feature only matters if the two groups differ materially.
        sep = "yes" if abs(a - b) >= 0.15 * max(1.0, abs(a), abs(b)) else "no"
        print("%-18s %12.3f %12.3f %10s" % (key, a, b, sep))
        out["features"][key] = {"preserved": a, "threw": b, "separates": sep == "yes"}

    print("\nA cause must SEPARATE the groups. A feature that is common in both")
    print("is a property of the family, not an explanation of the failure.")
    if args.output:
        args.output.write_text(json.dumps(out, indent=2) + "\n",
                               encoding="utf-8", newline="\n")
        print(f"\nWrote {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
