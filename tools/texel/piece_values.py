#!/usr/bin/env python3
"""What a piece is worth to the evaluation, every term included, by phase.

For positions sampled from a rows-v1 file, remove one White piece of a type
(pawn, knight, bishop, rook or queen). Keep a position only if White is to
move and not in check before and after, and the reduced position is legal
with a move. Then read the drop in the evaluation, White's view:
eval(position) - eval(without the piece). The evaluation is the traced
tuner's (`basilisk-texel --dump-eval`), at the header or at a vector
(`--init`). Medians are by piece and phase band.

This is the marginal value a fit gives a piece in real positions. A table
level is not that value: BAS-O12's Correction 2 read a negative middlegame
pawn from a level, where this read measured 111 to 120 cp. It is one of the
magnitude contract's reads (C-97). The layer is evaluation only; it never
accepts a vector.

    python tools/texel/piece_values.py --tuner build/texel/basilisk-texel \\
        --rows tools/texel/data/corpusA/rows/validation.csv \\
        --vector header --vector fit=<run>/04-polish.txt --out piece_values.json
"""

from __future__ import annotations

import argparse
import csv
import json
import random
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path

import chess

PIECES = {"pawn": chess.PAWN, "knight": chess.KNIGHT, "bishop": chess.BISHOP,
          "rook": chess.ROOK, "queen": chess.QUEEN}
BANDS = [("phase 0-7", 0, 7), ("phase 8-15", 8, 15), ("phase 16-24", 16, 24)]
MIN_PER_CELL = 30
SCORE_LIMIT = 3000  # known wins and mates are not marginal values
PHASE_WEIGHT = {chess.KNIGHT: 1, chess.BISHOP: 1, chess.ROOK: 2, chess.QUEEN: 4}


def phase_of(board: chess.Board) -> int:
    """The evaluation's game phase: 24 with every piece on, 0 with none."""
    return min(24, sum(PHASE_WEIGHT.get(p.piece_type, 0) for p in board.piece_map().values()))


def read_rows(path: Path) -> tuple[list[str], list[list[str]]]:
    lines = [line for line in path.open(encoding="utf-8") if not line.startswith("#")]
    table = list(csv.reader(lines))
    return table[0], table[1:]


def build_pairs(rows: list[list[str]], count: int, seed: int) -> list[dict]:
    """Up to `count` (full, reduced) position pairs, seeded."""
    rng = random.Random(seed)
    order = list(range(len(rows)))
    rng.shuffle(order)
    pairs = []
    for i in order:
        row = rows[i]
        board = chess.Board(row[0])
        if board.turn != chess.WHITE or board.is_check():
            continue
        name = rng.choice(list(PIECES))
        squares = sorted(board.pieces(PIECES[name], chess.WHITE))
        if not squares:
            continue
        reduced = board.copy()
        reduced.remove_piece_at(rng.choice(squares))
        if not reduced.is_valid() or reduced.is_check() or not any(reduced.legal_moves):
            continue
        pairs.append({"piece": name, "phase": phase_of(board), "full": board.fen(),
                      "reduced": reduced.fen(), "rest": row[1:]})
        if len(pairs) >= count:
            break
    return pairs


def write_rows(path: Path, header: list[str], pairs: list[dict], which: str) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as out:
        out.write("# basilisk-texel-rows-v1 piece-values\n" + ",".join(header) + "\n")
        for pair in pairs:
            out.write(",".join([pair[which], *pair["rest"]]) + "\n")


def evaluations(tuner: str, rows: Path, vector: Path | None) -> list[int]:
    command = [tuner, "--dump-eval", str(rows)]
    if vector is not None:
        command += ["--init", str(vector)]
    out = subprocess.run(command, capture_output=True, text=True, check=True).stdout
    words = [line.split() for line in out.splitlines()]
    return [int(w[0]) for w in words if len(w) == 2 and w[0].lstrip("-").isdigit()]


def medians(pairs: list[dict], full: list[int], reduced: list[int]) -> dict:
    if not len(pairs) == len(full) == len(reduced):
        raise ValueError(f"{len(pairs)} pairs but {len(full)} and {len(reduced)} evaluations")
    table = {}
    for name in PIECES:
        for band, low, high in BANDS:
            drops = [f - r for p, f, r in zip(pairs, full, reduced)
                     if p["piece"] == name and low <= p["phase"] <= high
                     and abs(f) < SCORE_LIMIT and abs(r) < SCORE_LIMIT]
            if len(drops) >= MIN_PER_CELL:
                table[f"{name} {band}"] = {"median": round(statistics.median(drops)), "n": len(drops)}
    return table


def parse_vector(spec: str) -> tuple[str, Path | None]:
    if spec == "header":
        return "header", None
    if "=" not in spec:
        raise ValueError(f"a vector is `header` or LABEL=PATH, got {spec!r}")
    label, path = spec.split("=", 1)
    return label, Path(path)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tuner", required=True, type=Path)
    parser.add_argument("--rows", required=True, type=Path)
    parser.add_argument("--vector", action="append", required=True,
                        help="`header`, or LABEL=PATH to a complete vector; repeatable")
    parser.add_argument("--positions", type=int, default=6000)
    parser.add_argument("--seed", type=int, default=20261123)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args(argv)
    try:
        vectors = [parse_vector(v) for v in args.vector]
    except ValueError as exc:
        parser.error(str(exc))
    tuner = str(args.tuner.resolve())
    header, rows = read_rows(args.rows)
    pairs = build_pairs(rows, args.positions, args.seed)
    report = {"schema": "basilisk-piece-values-v1", "layer": "evaluation",
              "rows": str(args.rows), "seed": args.seed, "pairs": len(pairs), "vectors": {}}
    with tempfile.TemporaryDirectory() as tmp:
        full_rows, reduced_rows = Path(tmp) / "full.csv", Path(tmp) / "reduced.csv"
        write_rows(full_rows, header, pairs, "full")
        write_rows(reduced_rows, header, pairs, "reduced")
        for label, path in vectors:
            report["vectors"][label] = medians(pairs, evaluations(tuner, full_rows, path),
                                               evaluations(tuner, reduced_rows, path))
    labels = [label for label, _ in vectors]
    print(f"{'piece, phase':22}" + "".join(f"{label:>16}" for label in labels))
    for key in report["vectors"][labels[0]]:
        cells = [report["vectors"][label].get(key) for label in labels]
        print(f"{key:22}" + "".join(f"{c['median']:>9} ({c['n']:>4})" if c else f"{'-':>16}"
                                    for c in cells))
    if args.out:
        args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
