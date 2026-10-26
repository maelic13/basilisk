#!/usr/bin/env python3
"""Build an opening book of the positions where two engines choose differently
and the side to move may promote to a rook or bishop.

Every such position in the given games is searched by both engines at a fixed
node count, one thread, Hash 64, from a fresh game; the positions where their
best moves differ are written as sorted, de-duplicated FENs. Fixed nodes make
the result reproducible.

    python tools/diag/promotion_book.py --candidate A.exe --baseline B.exe \\
        --out book.epd games1.pgn games2.pgn ...
"""

from __future__ import annotations

import argparse
import subprocess
from multiprocessing import Pool

import chess
import chess.pgn


def harvest(paths: list[str]) -> list[str]:
    seen: dict[str, str] = {}
    for path in paths:
        with open(path, encoding="utf-8") as f:
            while (game := chess.pgn.read_game(f)) is not None:
                board = game.board()
                for move in game.mainline_moves():
                    if any(m.promotion in (chess.ROOK, chess.BISHOP) for m in board.legal_moves):
                        seen.setdefault(" ".join(board.fen().split()[:4]), board.fen())
                    board.push(move)
    return list(seen.values())


class Engine:
    def __init__(self, exe: str, nodes: int):
        self.nodes = nodes
        self.proc = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     text=True, bufsize=1)
        self.send("uci\nsetoption name Threads value 1\nsetoption name Hash value 64\nisready\n")
        self.until("readyok")

    def send(self, text: str) -> None:
        self.proc.stdin.write(text)
        self.proc.stdin.flush()

    def until(self, prefix: str) -> str:
        for line in self.proc.stdout:
            if line.startswith(prefix):
                return line
        raise RuntimeError("engine exited")

    def bestmove(self, fen: str) -> str:
        self.send(f"ucinewgame\nposition fen {fen}\ngo nodes {self.nodes}\n")
        return self.until("bestmove").split()[1]


_engines: tuple[Engine, Engine] | None = None


def _init(candidate: str, baseline: str, nodes: int) -> None:
    global _engines
    _engines = (Engine(candidate, nodes), Engine(baseline, nodes))


def _differs(fen: str) -> tuple[str, bool]:
    assert _engines is not None
    return fen, _engines[0].bestmove(fen) != _engines[1].bestmove(fen)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--candidate", required=True)
    ap.add_argument("--baseline", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--nodes", type=int, default=100000)
    ap.add_argument("--workers", type=int, default=14)
    ap.add_argument("games", nargs="+")
    args = ap.parse_args()

    fens = harvest(args.games)
    with Pool(args.workers, initializer=_init,
              initargs=(args.candidate, args.baseline, args.nodes)) as pool:
        kept = sorted(fen for fen, differs in pool.imap_unordered(_differs, fens, chunksize=64)
                      if differs)
    with open(args.out, "w", newline="\n") as out:
        out.writelines(fen + "\n" for fen in kept)
    print(f"{len(fens)} positions with an underpromotion legal, {len(kept)} where the engines differ")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
