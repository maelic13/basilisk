#!/usr/bin/env python3
"""Fit Stockfish's win-rate model to Basilisk's own games.

Model (Stockfish `uci.cpp`, `win_rate_params`): for an evaluation `v` from
the side to move and material `m = P + 3N + 3B + 5R + 9Q` (both sides),
clamped to [17, 78] and divided by 58,

    P(win)  = 1 / (1 + exp((a(m) - v) / b(m)))
    P(loss) = 1 / (1 + exp((a(m) + v) / b(m)))

with `a` and `b` cubics in m. The displayed score is then `100 * v / a(m)`:
`cp 100` means a 50% chance of winning at that material.

Data: every engine move of every official game in the PGNs, scored by its
comment `s=` (the engine's score from the side to move, in internal units),
the material before the move, and the game's result from that side. Decisive
and known-win scores (|s| >= 10,000) are left out.

Fit: per material bucket, maximum likelihood for (a, b); then cubics in m
fitted to the buckets, weighted by their positions (Stockfish's WDL_model
does the same two stages).

Usage: python tools/diag/winrate_fit.py PGN [PGN...] [--json OUT]
"""
from __future__ import annotations

import argparse
import json
import re
import sys

import chess
import chess.pgn
import numpy as np

SCORE = re.compile(r"\bs=(-?\d+)\b")


def idle_priority() -> None:
    """Measurement runs share the host: take only the cycles nobody wants."""
    if sys.platform == "win32":
        import ctypes
        ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x40)


def material(board: chess.Board) -> int:
    count = lambda kind: len(board.pieces(kind, chess.WHITE)) + len(board.pieces(kind, chess.BLACK))
    return (count(chess.PAWN) + 3 * count(chess.KNIGHT) + 3 * count(chess.BISHOP)
            + 5 * count(chess.ROOK) + 9 * count(chess.QUEEN))


def collect(paths):
    rows = []
    for path in paths:
        with open(path, encoding="utf-8") as handle:
            while (game := chess.pgn.read_game(handle)) is not None:
                if game.headers.get("ColosseumSample", "official") != "official":
                    continue
                result = {"1-0": 1, "0-1": -1, "1/2-1/2": 0}.get(game.headers.get("Result"))
                if result is None:
                    continue
                board = game.board()
                for node in game.mainline():
                    m = SCORE.search(node.comment or "")
                    if m:
                        v = int(m.group(1))
                        if abs(v) < 10000:
                            mover = 1 if board.turn == chess.WHITE else -1
                            rows.append((v, material(board), result * mover))
                    board.push(node.move)
    return np.array(rows, dtype=float)


def nll(a, b, v, o):
    pw = 1.0 / (1.0 + np.exp((a - v) / b))
    pl = 1.0 / (1.0 + np.exp((a + v) / b))
    pd = np.clip(1.0 - pw - pl, 1e-12, 1.0)
    p = np.where(o > 0, pw, np.where(o < 0, pl, pd))
    return -np.sum(np.log(np.clip(p, 1e-12, 1.0)))


def fit_bucket(v, o):
    """(a, b) by coarse-to-fine grid search on the likelihood."""
    best = (None, None, np.inf)
    a_lo, a_hi, b_lo, b_hi = 20.0, 1500.0, 10.0, 800.0
    for _ in range(6):
        for a in np.linspace(a_lo, a_hi, 31):
            for b in np.linspace(b_lo, b_hi, 31):
                value = nll(a, b, v, o)
                if value < best[2]:
                    best = (a, b, value)
        da, db = (a_hi - a_lo) / 10, (b_hi - b_lo) / 10
        a_lo, a_hi = max(1.0, best[0] - da), best[0] + da
        b_lo, b_hi = max(1.0, best[1] - db), best[1] + db
    return best[0], best[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("pgn", nargs="+")
    parser.add_argument("--json")
    parser.add_argument("--min-bucket", type=int, default=2000)
    args = parser.parse_args()
    idle_priority()

    data = collect(args.pgn)
    v, mat, o = data[:, 0], np.clip(data[:, 1], 17, 78), data[:, 2]
    print(f"{len(data)} scored positions from {len(args.pgn)} PGN file(s)")
    buckets = []
    for m in range(17, 79):
        sel = mat == m
        if sel.sum() < args.min_bucket:
            continue
        a, b = fit_bucket(v[sel], o[sel])
        buckets.append((m, a, b, int(sel.sum())))
    ms = np.array([x[0] for x in buckets]) / 58.0
    w = np.sqrt(np.array([x[3] for x in buckets], dtype=float))
    a_poly = np.polyfit(ms, [x[1] for x in buckets], 3, w=w)
    b_poly = np.polyfit(ms, [x[2] for x in buckets], 3, w=w)
    print("material  positions        a        b")
    for m, a, b, n in buckets:
        print(f"{m:8d} {n:10d} {a:8.1f} {b:8.1f}")
    print("a(m) cubic (m/58, highest power first):", ", ".join(f"{c:.8f}" for c in a_poly))
    print("b(m) cubic (m/58, highest power first):", ", ".join(f"{c:.8f}" for c in b_poly))
    a58 = np.polyval(a_poly, 1.0)
    print(f"a at material 58: {a58:.1f} internal units = cp 100")
    if args.json:
        with open(args.json, "w", encoding="utf-8") as out:
            json.dump({"positions": len(data), "buckets": buckets,
                       "a_poly": list(a_poly), "b_poly": list(b_poly)}, out, indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
