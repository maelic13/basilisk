#!/usr/bin/env python3
"""How often two engine builds choose different moves in real game positions.

A repair's exposure read (agents/measurement.md, *Gating*): positions are
sampled from a PGN with the whole game history before them, so repetition
and rule-50 state are the game's, and each engine searches the same
`position ... moves ...` at a fixed node budget from a fresh game. The
budget is in nodes, so the result does not depend on host load.

    python tools/diag/decision_diff.py games.pgn A.exe B.exe \
        [--nodes 50000] [--every 10] [--max-positions 2000] [--json out.json]

Reported: positions compared, how many chose different moves, the share,
and the same split by the halfmove clock (a repetition needs at least four
reversible plies) and by game phase (men on the board).
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import chess
import chess.pgn


class Engine:
    def __init__(self, path: Path) -> None:
        self.proc = subprocess.Popen([str(path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     text=True, bufsize=1)
        self.send("uci")
        self.wait("uciok")
        self.send("setoption name Hash value 16")
        self.send("setoption name Threads value 1")
        self.send("isready")
        self.wait("readyok")

    def send(self, line: str) -> None:
        assert self.proc.stdin is not None
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()

    def wait(self, token: str) -> str:
        assert self.proc.stdout is not None
        for line in self.proc.stdout:
            if line.startswith(token):
                return line.strip()
        raise RuntimeError(f"engine exited before {token!r}")

    def bestmove(self, position: str, nodes: int) -> str:
        self.send("ucinewgame")
        self.send("isready")
        self.wait("readyok")
        self.send(position)
        self.send(f"go nodes {nodes}")
        return self.wait("bestmove").split()[1]

    def close(self) -> None:
        self.send("quit")
        self.proc.wait(timeout=10)


def positions(pgn_path: Path, every: int, limit: int):
    """(position command, board) every `every` plies of every game, with history."""
    count = 0
    with pgn_path.open(encoding="utf-8", errors="replace") as handle:
        while count < limit:
            game = chess.pgn.read_game(handle)
            if game is None:
                return
            start = game.board()
            prefix = "position startpos" if start.fen() == chess.STARTING_FEN \
                else f"position fen {start.fen()}"
            board = start.copy()
            moves: list[str] = []
            for ply, move in enumerate(game.mainline_moves(), 1):
                board.push(move)
                moves.append(move.uci())
                if ply % every or board.is_game_over(claim_draw=False):
                    continue
                yield f"{prefix} moves {' '.join(moves)}", board.copy(stack=False)
                count += 1
                if count >= limit:
                    return


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("pgn", type=Path)
    parser.add_argument("engine_a", type=Path)
    parser.add_argument("engine_b", type=Path)
    parser.add_argument("--nodes", type=int, default=50000)
    parser.add_argument("--every", type=int, default=10, help="sample every N plies")
    parser.add_argument("--max-positions", type=int, default=2000)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    a, b = Engine(args.engine_a), Engine(args.engine_b)
    cells: dict[str, list[int]] = {}

    def tally(key: str, differs: bool) -> None:
        cell = cells.setdefault(key, [0, 0])
        cell[0] += 1
        cell[1] += differs

    try:
        for position, board in positions(args.pgn, args.every, args.max_positions):
            differs = a.bestmove(position, args.nodes) != b.bestmove(position, args.nodes)
            tally("all", differs)
            tally("halfmove>=4" if board.halfmove_clock >= 4 else "halfmove<4", differs)
            men = len(board.piece_map())
            tally("men<=10" if men <= 10 else "men11-20" if men <= 20 else "men>20", differs)
    finally:
        a.close()
        b.close()

    for key, (n, d) in sorted(cells.items()):
        print(f"{key:12s} {n:6d} positions, {d:5d} differ ({100.0 * d / max(n, 1):.1f}%)")
    if args.json:
        args.json.write_text(json.dumps({"nodes": args.nodes, "every": args.every,
                                         "cells": cells}, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
