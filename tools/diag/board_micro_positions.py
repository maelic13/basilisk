#!/usr/bin/env python3
"""Freeze the board microbenchmark's positions from a gate's games.

A board primitive's speed depends on the positions it runs on, so the
microbenchmark times real game positions rather than the five perft roots:
every `--every`-th ply of the PGN's games in file order, from ply `--every`
on, until `--count` positions, skipping finished games. The output is a
FEN list with a header naming the PGN's SHA-256 and the recipe, and an
oracle line of python-chess counts that every engine's harness checks
before timing. The file can be regenerated and checked:

    python tools/diag/board_micro_positions.py tools/results/b71-gate/games.pgn
    python tools/diag/board_micro_positions.py tools/results/b71-gate/games.pgn --check
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

import chess
import chess.pgn

REPO = Path(__file__).resolve().parents[2]
OUTPUT = REPO / "tests" / "data" / "board_micro_positions.fen"
PYTHON_CHESS = "1.11.2"


def build(pgn: Path, every: int, count: int) -> bytes:
    digest = hashlib.sha256(pgn.read_bytes()).hexdigest().upper()
    fens: list[str] = []
    with pgn.open(encoding="utf-8", errors="replace") as handle:
        while len(fens) < count:
            game = chess.pgn.read_game(handle)
            if game is None:
                break
            board = game.board()
            for ply, move in enumerate(game.mainline_moves(), 1):
                board.push(move)
                if ply % every or board.is_game_over(claim_draw=False):
                    continue
                fens.append(board.fen(en_passant="legal"))
                if len(fens) >= count:
                    break
    if len(fens) < count:
        raise SystemExit(f"{pgn} gave {len(fens)} positions, not {count}")
    # Engine-independent counts every timing harness must reproduce before it
    # times anything: the legal moves, the capture stage (captures, en passant
    # and every promotion) and the moves that give check.
    legal = tactical = checking = 0
    for fen in fens:
        board = chess.Board(fen)
        for move in board.legal_moves:
            legal += 1
            tactical += board.is_capture(move) or move.promotion is not None
            checking += board.gives_check(move)
    header = [
        "# Board microbenchmark positions (tests/board_micro.cpp).",
        f"# Source: {pgn.name}, SHA-256 {digest}",
        f"# Recipe: board_micro_positions.py --every {every} --count {count}; python-chess {PYTHON_CHESS}",
        f"# Oracle: positions {count} legal {legal} tactical {tactical} checking {checking}",
    ]
    return ("\n".join(header + fens) + "\n").encode("utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("pgn", type=Path)
    parser.add_argument("--every", type=int, default=9)
    parser.add_argument("--count", type=int, default=512)
    parser.add_argument("--check", action="store_true", help="fail if the committed file differs")
    args = parser.parse_args()
    if chess.__version__ != PYTHON_CHESS:
        print(f"python-chess {chess.__version__}, not {PYTHON_CHESS}", file=sys.stderr)
        return 2
    data = build(args.pgn, args.every, args.count)
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_bytes().replace(b"\r\n", b"\n") != data:
            print(f"{OUTPUT} is stale or missing", file=sys.stderr)
            return 1
        print(f"{OUTPUT.name}: up to date")
        return 0
    OUTPUT.write_bytes(data)
    print(f"wrote {OUTPUT} ({args.count} positions)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
