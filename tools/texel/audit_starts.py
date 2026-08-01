#!/usr/bin/env python3
"""Fast correlation/provenance proxy audit for self-play PGN start positions.

The Beast source does not retain source-game identifiers, so no tool can prove
that two sampled FENs came from different ICCF/MEGA games.  This audit reports
what *is* knowable from the PGN: exact start duplication, five-phase coverage,
and concentration by pawn-structure family (pawn squares + side + castling).
"""

from __future__ import annotations

import argparse
from collections import Counter

import extract


def parse_fen_header(line: str) -> str | None:
    prefix = '[FEN "'
    text = line.rstrip("\r\n")
    if not text.startswith(prefix) or not text.endswith('"]'):
        return None
    fields = text[len(prefix):-2].split()
    return " ".join(fields[:4]) if len(fields) >= 4 else None


def pawn_family(fen: str) -> tuple[str, str, str, str]:
    placement, side, castling, _ep = fen.split()[:4]
    white: list[str] = []
    black: list[str] = []
    square = 56
    for char in placement:
        if char == "/":
            square -= 16
        elif char.isdigit():
            square += int(char)
        else:
            if char == "P":
                white.append(str(square))
            elif char == "p":
                black.append(str(square))
            square += 1
    return (",".join(white), ",".join(black), side, castling)


def phase_from_fen(fen: str) -> int:
    weights = {"n": 1, "b": 1, "r": 2, "q": 4,
               "N": 1, "B": 1, "R": 2, "Q": 4}
    return min(sum(weights.get(char, 0) for char in fen.split()[0]), 24)


def effective_count(counter: Counter) -> float:
    total = sum(counter.values())
    denominator = sum(count * count for count in counter.values())
    return total * total / denominator if denominator else 0.0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pgn")
    args = parser.parse_args()

    exact = Counter()
    families = [Counter() for _ in extract.PHASE_BUCKETS]
    phase_counts = Counter()
    with open(args.pgn, encoding="utf-8", errors="replace") as pgn:
        for line in pgn:
            fen = parse_fen_header(line)
            if fen is None:
                continue
            bucket = extract.phase_bucket(phase_from_fen(fen))
            exact[fen] += 1
            families[bucket][pawn_family(fen)] += 1
            phase_counts[bucket] += 1

    total = sum(exact.values())
    print(f"PGN starts          : {total:,}")
    print(f"Exact unique starts : {len(exact):,}")
    print(f"Exact duplicates    : {total - len(exact):,}")
    print("\nStart-position phase and pawn-family concentration:")
    for index, name in enumerate(extract.BUCKET_NAMES):
        count = phase_counts[index]
        family = families[index]
        largest = max(family.values(), default=0)
        top100 = sum(sorted(family.values(), reverse=True)[:100])
        print(f"  {name:13}: starts={count:,} families={len(family):,} "
              f"effective={effective_count(family):,.0f} "
              f"largest={largest:,} top100={100.0 * top100 / max(count, 1):.2f}%")
    print("\nPawn-family is a correlation proxy, not source-game provenance; the Beast")
    print("position files do not contain ICCF/MEGA game identifiers.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
