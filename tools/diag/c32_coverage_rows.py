#!/usr/bin/env python3
"""Generated rows for the evaluation reference fixture (C.3.2, Amendment 1 point 7).

Every slot of the rebuilt evaluation must move the value on at least 20 of
the fixture's rows, or be listed as unreached with its reason. Some slots
are legal but rare in game positions; this writes deterministic positions
that reach them to tests/fixtures/eval_reference_generated.txt, which
tools/diag/c32_reference.py reads after the hand-built rows and mirrors in
the same way, so each position below gives two rows.

TARGETS is append-only and in order: each ticket adds its targets at the
end, so the rows earlier tickets produced keep their place. Each target
yields PER_TARGET positions (twenty rows with the mirrors).

  piece-square  a white piece of a type moved to a square, from game rows
  connected     a white pawn with a phalanx, opposition and supporters
  weak lever    an unsupported white pawn attacking two black pawns

Usage:
  python tools/diag/c32_coverage_rows.py
"""

from __future__ import annotations

import csv
import pathlib
import sys

try:
    import chess
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

REPO = pathlib.Path(__file__).resolve().parents[2]
FIXTURE = REPO / "tests" / "fixtures" / "eval_reference.tsv"
OUT = REPO / "tests" / "fixtures" / "eval_reference_generated.txt"
PER_TARGET = 10
PIECES = {"knight": chess.KNIGHT, "bishop": chess.BISHOP, "queen": chess.QUEEN, "king": chess.KING}

# Ticket 2: the table squares and connected cells game rows reach on fewer
# than 20 rows, and the weak lever.
TARGETS: list[tuple] = (
    [("psq", "knight", sq) for sq in (0, 7, 55, 56, 57, 58, 59, 60, 61, 62, 63)]
    + [("psq", "bishop", 63)]
    + [("psq", "queen", sq) for sq in (6, 7, 54, 55)]
    + [("psq", "king", sq) for sq in (48, 56, 57, 58, 59, 60, 61, 62, 63)]
    # (relative rank index 1-6, phalanx, opposed, supporters)
    + [("connected", 2, 1, 0, 2), ("connected", 2, 1, 1, 2), ("connected", 3, 1, 0, 2),
       ("connected", 3, 1, 1, 2), ("connected", 4, 1, 0, 2), ("connected", 4, 1, 1, 2),
       ("connected", 5, 0, 0, 2), ("connected", 5, 0, 1, 2), ("connected", 5, 1, 0, 1),
       ("connected", 5, 1, 0, 2), ("connected", 5, 1, 1, 0), ("connected", 5, 1, 1, 1),
       ("connected", 5, 1, 1, 2), ("connected", 6, 0, 0, 2), ("connected", 6, 1, 0, 0),
       ("connected", 6, 1, 0, 1), ("connected", 6, 1, 0, 2)]
    + [("weak lever",)]
)


def usable(board: chess.Board) -> bool:
    return board.is_valid() and not board.is_check()


def game_boards() -> list[chess.Board]:
    with FIXTURE.open(encoding="utf-8", newline="") as stream:
        return [chess.Board(r["fen"]) for r in csv.DictReader(stream, delimiter="\t") if r["source"] == "game"]


def psq_rows(games: list[chess.Board], name: str, square: int) -> list[str]:
    piece = PIECES[name]
    out = []
    for source in games:
        if source.piece_at(square) is not None or not source.pieces(piece, chess.WHITE):
            continue
        board = source.copy(stack=False)
        board.turn = chess.WHITE
        board.ep_square = None
        origin = min(board.pieces(piece, chess.WHITE))
        board.remove_piece_at(origin)
        board.set_piece_at(square, chess.Piece(piece, chess.WHITE))
        if piece == chess.KING:
            board.castling_rights &= ~chess.BB_RANK_1
        board.castling_rights = board.clean_castling_rights()
        if usable(board):
            out.append(board.fen())
        if len(out) == PER_TARGET:
            break
    return out


# Material around the pawn structures, so that the rows are ordinary
# middlegames rather than endings the reference evaluates specially.
SETTINGS = [
    {chess.G1: "K", chess.G8: "k", chess.D1: "Q", chess.D8: "q", chess.A1: "R", chess.A8: "r"},
    {chess.B1: "K", chess.B8: "k", chess.E1: "Q", chess.E8: "q", chess.H1: "R", chess.H8: "r"},
    {chess.H2: "K", chess.A7: "k", chess.C1: "R", chess.F8: "r", chess.B1: "N", chess.G8: "n"},
]


def connected_rows(r: int, phalanx: int, opposed: int, support: int) -> list[str]:
    out = []
    for setting in SETTINGS:
        for f in range(1, 7):
            board = chess.Board(None)
            for sq, symbol in setting.items():
                board.set_piece_at(sq, chess.Piece.from_symbol(symbol))
            pawns = [(f, r)]
            if phalanx:
                pawns.append((f + 1, r))
            pawns += [(f - 1, r - 1), (f + 1, r - 1)][:support]
            black = [(f, min(r + 2, 6))] if opposed else []
            squares = [chess.square(x, y) for x, y in pawns + black]
            if len(set(squares)) != len(squares) or any(board.piece_at(s) for s in squares):
                continue
            for x, y in pawns:
                board.set_piece_at(chess.square(x, y), chess.Piece(chess.PAWN, chess.WHITE))
            for x, y in black:
                board.set_piece_at(chess.square(x, y), chess.Piece(chess.PAWN, chess.BLACK))
            if usable(board):
                out.append(board.fen())
            if len(out) == PER_TARGET:
                return out
    return out


def weak_lever_rows() -> list[str]:
    out = []
    for setting in SETTINGS:
        for f in range(1, 7):
            for r in (2, 3, 4):
                board = chess.Board(None)
                for sq, symbol in setting.items():
                    board.set_piece_at(sq, chess.Piece.from_symbol(symbol))
                squares = [chess.square(f, r), chess.square(f - 1, r + 1), chess.square(f + 1, r + 1)]
                if any(board.piece_at(s) for s in squares):
                    continue
                board.set_piece_at(squares[0], chess.Piece(chess.PAWN, chess.WHITE))
                board.set_piece_at(squares[1], chess.Piece(chess.PAWN, chess.BLACK))
                board.set_piece_at(squares[2], chess.Piece(chess.PAWN, chess.BLACK))
                if usable(board):
                    out.append(board.fen())
                if len(out) == PER_TARGET:
                    return out
    return out


def main() -> int:
    games = game_boards()
    lines = ["# Generated by tools/diag/c32_coverage_rows.py; do not edit. One FEN per line,",
             "# then \" ; \" and the slot it reaches. The producer adds each row's mirror."]
    for target in TARGETS:
        kind = target[0]
        if kind == "psq":
            fens, tag = psq_rows(games, target[1], target[2]), f"{target[1]} on {chess.SQUARE_NAMES[target[2]]}"
        elif kind == "connected":
            fens, tag = connected_rows(*target[1:]), "connected rank {} phalanx {} opposed {} support {}".format(
                target[1] + 1, *target[2:])
        else:
            fens, tag = weak_lever_rows(), "weak lever"
        if len(fens) < PER_TARGET:
            print(f"FAIL: {tag}: only {len(fens)} positions", file=sys.stderr)
            return 1
        lines += [f"{fen} ; {tag}" for fen in fens]
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(f"Wrote {OUT} ({len(lines) - 2} positions for {len(TARGETS)} targets).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
