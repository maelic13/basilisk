"""Freeze a cohort of game positions from a PGN as an EPD file.

One position from every `--game-step`-th game, at a point that moves through
the game with the game's index (the golden-ratio sequence), so the cohort
spans openings, middlegames and endings without favouring any. Positions
before `--min-ply` and positions where the game is over are skipped. The
output is deterministic for a given PGN and options, so the EPD can be
regenerated and its hash checked; each line names its game and ply.

    python tools/diag/pgn_cohort.py tools/results/b87-gate/games.pgn out.epd \
        --game-step 8 --limit 1000
"""
from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

import chess
import chess.pgn

GOLDEN = 0.6180339887498949


def cohort(pgn_path: Path, game_step: int, limit: int, min_ply: int) -> list[str]:
    lines: list[str] = []
    with pgn_path.open(encoding="utf-8", errors="replace") as handle:
        index = 0
        while len(lines) < limit:
            game = chess.pgn.read_game(handle)
            if game is None:
                break
            index += 1
            if (index - 1) % game_step:
                continue
            moves = list(game.mainline_moves())
            if len(moves) <= min_ply:
                continue
            fraction = (index * GOLDEN) % 1.0
            target = min_ply + int(fraction * (len(moves) - min_ply))
            board = game.board()
            for move in moves[:target]:
                board.push(move)
            if board.is_game_over(claim_draw=False):
                continue
            lines.append(f'{board.epd()} id "game {index} ply {target}";')
    return lines


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("pgn", type=Path)
    ap.add_argument("out", type=Path)
    ap.add_argument("--game-step", type=int, default=8)
    ap.add_argument("--limit", type=int, default=1000)
    ap.add_argument("--min-ply", type=int, default=8)
    args = ap.parse_args(argv)
    lines = cohort(args.pgn, args.game_step, args.limit, args.min_ply)
    if not lines:
        print("no position sampled", file=sys.stderr)
        return 1
    text = "\n".join(lines) + "\n"
    args.out.write_text(text, encoding="utf-8", newline="\n")
    digest = hashlib.sha256(text.encode("utf-8")).hexdigest()
    print(f"{len(lines)} positions -> {args.out}  sha256 {digest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
