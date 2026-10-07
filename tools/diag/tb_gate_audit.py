#!/usr/bin/env python3
"""Audit a tablebase-enabled gate from its PGN.

  conversion  For every official game, the first position of at most the
              tables' size with a clean tablebase win for the side to move
              (WDL +2 and |DTZ| + rule-50 counter <= 100). The side holding it
              must win the game; every miss is listed.
  activation  Samples positions of 7-9 men from the games, searches each to a
              node budget with every engine given, tables configured, and
              reports the share of searches whose last info line shows a
              tablebase hit.

Usage:
  python tools/diag/tb_gate_audit.py conversion PGN --tables DIR
  python tools/diag/tb_gate_audit.py activation PGN --tables DIR --engine A --engine B
         [--sample 200] [--nodes 100000] [--seed 815]
"""
from __future__ import annotations

import argparse
import random
import sys
from pathlib import Path

import chess
import chess.pgn
import chess.syzygy

sys.path.insert(0, str(Path(__file__).resolve().parent))
import uci_probe  # noqa: E402


def official_games(pgn_path: str):
    with open(pgn_path, encoding="utf-8") as handle:
        while (game := chess.pgn.read_game(handle)) is not None:
            if game.headers.get("ColosseumSample", "official") == "official":
                yield game


def conversion(args) -> int:
    reached = converted = 0
    misses = []
    with chess.syzygy.open_tablebase(args.tables) as tables:
        for game in official_games(args.pgn):
            board = game.board()
            for move in game.mainline_moves():
                board.push(move)
                if chess.popcount(board.occupied) > 6 or board.castling_rights:
                    continue
                try:
                    wdl = tables.probe_wdl(board)
                    dtz = tables.probe_dtz(board)
                except (KeyError, chess.syzygy.MissingTableError):
                    continue
                if wdl != 2 or abs(dtz) + board.halfmove_clock > 100:
                    continue
                winner = "1-0" if board.turn == chess.WHITE else "0-1"
                holder = game.headers["White"] if board.turn == chess.WHITE else game.headers["Black"]
                reached += 1
                if game.headers["Result"] == winner:
                    converted += 1
                else:
                    misses.append((game.headers.get("GameNumber"), holder, board.fen(),
                                   game.headers["Result"]))
                break
    print(f"games reaching a clean tablebase win: {reached}; converted: {converted}")
    for number, holder, fen, result in misses:
        print(f"  NOT CONVERTED game {number}: {holder} held a clean win at {fen}; result {result}")
    return 1 if misses else 0


def activation(args) -> int:
    fens = set()
    for game in official_games(args.pgn):
        board = game.board()
        for move in game.mainline_moves():
            board.push(move)
            if 7 <= chess.popcount(board.occupied) <= 9 and not board.is_game_over():
                fens.add(board.fen())
    rng = random.Random(args.seed)
    sample = sorted(fens)
    rng.shuffle(sample)
    sample = sample[:args.sample]
    print(f"{len(fens)} distinct 7-9-man positions; sampled {len(sample)} (seed {args.seed})")
    for engine in args.engine:
        hits = 0
        for fen in sample:
            out, best, _ = uci_probe.session(str(Path(engine).resolve()), [f"SyzygyPath={args.tables}"],
                                             [f"position fen {fen}", f"go nodes {args.nodes}"], 60)
            lines = [l for l in out if l.startswith("info depth ") and " tbhits " in l]
            if lines and int(lines[-1].split(" tbhits ")[1].split()[0]) > 0:
                hits += 1
        print(f"{Path(engine).name}: {hits}/{len(sample)} searches with a tablebase hit "
              f"({100.0 * hits / max(1, len(sample)):.1f}%)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=("conversion", "activation"))
    parser.add_argument("pgn")
    parser.add_argument("--tables", required=True)
    parser.add_argument("--engine", action="append", default=[])
    parser.add_argument("--sample", type=int, default=200)
    parser.add_argument("--nodes", type=int, default=100000)
    parser.add_argument("--seed", type=int, default=815)
    args = parser.parse_args()
    return conversion(args) if args.mode == "conversion" else activation(args)


if __name__ == "__main__":
    sys.exit(main())
