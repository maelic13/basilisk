#!/usr/bin/env python3
"""The games census: where both sides of a Colosseum PGN spend their clock, and what they convert.

From the PGN's own `s=` and `t=` comments; no engine runs. Per engine:

- by score band (won at 300 or more, lost at -300 or less, even between, the mover's
  own score): the moves, the mean time a move, and the moves that spent half the
  clock left or more, that clock rebuilt from the header's control;
- per threshold of 300, 800 and 1,500: the games "decided for it", its own score at
  or above the threshold on two of its consecutive moves, and of those the games it
  did not win ("unconverted"), and how many of those were drawn.

An unfinished game counts as a game and decides nothing. A score that is not a
number counts as unparsed and is in no band; the clock still falls by its time.

Usage:
  python tools/diag/pgn_time_census.py games.pgn [--out census.json]
"""

from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re
import sys

import chess
import chess.pgn

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from replay_games import parse_control

THRESHOLDS = (300, 800, 1500)
BANDS = ("won", "even", "lost")
COMMENT = re.compile(r"s=(\S+) d=(\d+) t=(\d+)ms")


def band(score: int) -> str:
    return "won" if score >= 300 else "lost" if score <= -300 else "even"


def blank() -> dict:
    return {"games": 0, "wins": 0, "draws": 0, "losses": 0,
            "decided": {t: 0 for t in THRESHOLDS}, "unconverted": {t: 0 for t in THRESHOLDS},
            "unconverted_draw": {t: 0 for t in THRESHOLDS},
            "moves": collections.Counter(), "time": collections.Counter(), "half_clock": collections.Counter(),
            "unparsed_scores": 0}


def census(handle) -> dict:
    """The report of every game the handle holds."""
    stats: dict[str, dict] = collections.defaultdict(blank)
    games = 0
    while (game := chess.pgn.read_game(handle)) is not None:
        games += 1
        names = {chess.WHITE: game.headers["White"], chess.BLACK: game.headers["Black"]}
        result = game.headers.get("Result", "*")
        points = {chess.WHITE: {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}.get(result),
                  chess.BLACK: {"1-0": 0.0, "0-1": 1.0, "1/2-1/2": 0.5}.get(result)}
        clock, increment = parse_control(game.headers["TimeControl"])
        run = {side: {t: 0 for t in THRESHOLDS} for side in (chess.WHITE, chess.BLACK)}
        decided: dict[bool, set[int]] = {chess.WHITE: set(), chess.BLACK: set()}
        board = game.board()
        for node in game.mainline():
            side = board.turn
            s = stats[names[side]]
            match = COMMENT.search(node.comment or "")
            if match:
                spent = int(match.group(3))
                before = clock[side]
                try:
                    score = int(match.group(1))
                except ValueError:
                    score = None
                    s["unparsed_scores"] += 1
                if score is not None:
                    b = band(score)
                    s["moves"][b] += 1
                    s["time"][b] += spent
                    s["half_clock"][b] += spent >= 0.5 * max(before, 1)
                    for threshold in THRESHOLDS:
                        run[side][threshold] = run[side][threshold] + 1 if score >= threshold else 0
                        if run[side][threshold] >= 2:
                            decided[side].add(threshold)
                clock[side] = before - spent + increment[side]
            board.push(node.move)
        for side in (chess.WHITE, chess.BLACK):
            s = stats[names[side]]
            s["games"] += 1
            p = points[side]
            if p is None:
                continue
            s["wins" if p == 1.0 else "draws" if p == 0.5 else "losses"] += 1
            for threshold in decided[side]:
                s["decided"][threshold] += 1
                s["unconverted"][threshold] += p < 1.0
                s["unconverted_draw"][threshold] += p == 0.5
    report: dict = {"games": games, "engines": {}}
    for name, s in sorted(stats.items()):
        report["engines"][name] = {
            "games": s["games"], "wins": s["wins"], "draws": s["draws"], "losses": s["losses"],
            "unparsed_scores": s["unparsed_scores"], "decided": s["decided"], "unconverted": s["unconverted"],
            "unconverted_draw": s["unconverted_draw"], "moves": dict(s["moves"]),
            "mean_ms": {b: round(s["time"][b] / max(1, s["moves"][b]), 1) for b in s["moves"]},
            "half_clock_moves": dict(s["half_clock"])}
    return report


def print_report(report: dict) -> None:
    print(f"games {report['games']}")
    for name, row in report["engines"].items():
        print(f"{name}: games {row['games']} +{row['wins']} ={row['draws']} -{row['losses']}; unparsed scores {row['unparsed_scores']}")
        for threshold in THRESHOLDS:
            d, u = row["decided"][threshold], row["unconverted"][threshold]
            print(f"   T {threshold}: decided {d}; unconverted {u} ({100 * u / max(1, d):.2f}%), of them drawn {row['unconverted_draw'][threshold]}")
        for b in BANDS:
            n = row["moves"].get(b, 0)
            half = row["half_clock_moves"].get(b, 0)
            print(f"   {b}: moves {n}; mean {row['mean_ms'].get(b, 0.0)} ms; at half the clock or more {half} ({100 * half / max(1, n):.2f}%)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("pgn", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path)
    args = parser.parse_args()
    with args.pgn.open(encoding="utf-8", errors="replace") as handle:
        report = census(handle)
    report["pgn"] = str(args.pgn)
    print_report(report)
    if args.out:
        args.out.write_text(json.dumps(report, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
