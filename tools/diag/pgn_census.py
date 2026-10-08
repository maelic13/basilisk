#!/usr/bin/env python3
"""Summarize one engine's head-to-head records from PGN headers."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

try:
    import chess.pgn
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

SCHEMA = "basilisk-pgn-census-v1"


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_headers(path: Path):
    """Yield headers without constructing move trees."""
    with path.open(encoding="utf-8", errors="replace") as handle:
        while True:
            headers = chess.pgn.read_headers(handle)
            if headers is None:
                return
            yield headers


def result_for(result: str, played_white: bool) -> str:
    if result in ("1/2-1/2", "Draw"):
        return "draws"
    if result in ("1-0", "WhiteWin"):
        return "wins" if played_white else "losses"
    if result in ("0-1", "BlackWin"):
        return "losses" if played_white else "wins"
    return "unfinished"


def summarize(headers, engine: str, opponents: set[str] | None = None) -> dict:
    records: dict[str, Counter[str]] = {}
    games_in_pgn = 0
    engine_games = 0

    for game in headers:
        games_in_pgn += 1
        white = game.get("White", "")
        black = game.get("Black", "")
        if engine == white:
            opponent = black
            color = "white_games"
            played_white = True
        elif engine == black:
            opponent = white
            color = "black_games"
            played_white = False
        else:
            continue
        if opponents is not None and opponent not in opponents:
            continue

        engine_games += 1
        record = records.setdefault(opponent, Counter())
        record["games"] += 1
        record[color] += 1
        record[result_for(game.get("Result", "*"), played_white)] += 1

    rows = {}
    for opponent, record in sorted(records.items()):
        wins = record["wins"]
        draws = record["draws"]
        games = record["games"]
        finished = games - record["unfinished"]
        points_twice = 2 * wins + draws
        score = points_twice / (2 * finished) if finished else None
        if score is None or score <= 0.0 or score >= 1.0:
            elo = None
        else:
            elo = 400.0 * math.log10(score / (1.0 - score))
        rows[opponent] = {
            "games": games,
            "white_games": record["white_games"],
            "black_games": record["black_games"],
            "wins": wins,
            "draws": draws,
            "losses": record["losses"],
            "unfinished": record["unfinished"],
            "points_twice": points_twice,
            "score_percent": round(100.0 * score, 3) if score is not None else None,
            "elo_difference": round(elo, 2) if elo is not None else None,
        }

    return {
        "schema": SCHEMA,
        "engine": engine,
        "opponents": sorted(opponents) if opponents is not None else "all",
        "games_in_pgn": games_in_pgn,
        "engine_games": engine_games,
        "records": rows,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--pgn", required=True, type=Path)
    parser.add_argument("--engine", required=True,
                        help="exactly as it appears in the PGN")
    parser.add_argument("--opponents", nargs="*", default=None)
    parser.add_argument("--json", type=Path, help="write the compact summary")
    args = parser.parse_args()

    if not args.pgn.is_file():
        raise SystemExit(f"PGN not found: {args.pgn}")

    requested = set(args.opponents) if args.opponents else None
    report = summarize(read_headers(args.pgn), args.engine, requested)
    report["pgn"] = {
        "path": str(args.pgn).replace("\\", "/"),
        "bytes": args.pgn.stat().st_size,
        "sha256": sha256_of(args.pgn),
    }
    report["generated_utc"] = datetime.now(timezone.utc).isoformat(timespec="seconds")

    if report["engine_games"] == 0:
        raise SystemExit(f"engine not found, or no requested opponents found: {args.engine}")
    if requested is not None:
        missing = requested - set(report["records"])
        if missing:
            raise SystemExit("requested opponent(s) not found: " + ", ".join(sorted(missing)))

    print(f"{args.engine}: {report['engine_games']} of {report['games_in_pgn']} games")
    print("opponent | games | W-D-L-U | score | Elo")
    for opponent, row in report["records"].items():
        wdl = f"{row['wins']}-{row['draws']}-{row['losses']}-{row['unfinished']}"
        score_text = (f"{row['score_percent']:.3f}%"
                      if row["score_percent"] is not None else "n/a")
        elo_text = (f"{row['elo_difference']:+.2f}"
                    if row["elo_difference"] is not None else "n/a")
        print(f"{opponent} | {row['games']} | {wdl} | "
              f"{score_text} | {elo_text}")

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"\nsummary: {args.json}")


if __name__ == "__main__":
    main()
