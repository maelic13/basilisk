#!/usr/bin/env python3
"""The piece-square seed from classical Stockfish 16 (PLAN C.2.5).

Writes a complete ``basilisk-texel-vector-v1`` vector: every slot is the
header's (``src/eval_params.h``), except the twelve PST tables, which take
the shape of ``sf_16``'s ``psqt.cpp`` (GPL-3, like Basilisk).

* **Source.** ``src/psqt.cpp`` at the pinned ``sf_16`` commit, read from a
  Stockfish checkout with ``git show``; the tag must resolve to the commit
  ``tools/donor/build_donor.py`` pins.
* **Squares.** Basilisk's tables are White's view, a1 = 0 to h8 = 63, rank 1
  first. ``Bonus[piece][rank][file]`` holds files a to d and mirrors them to
  h to e (Stockfish's ``edge_distance``); ``PBonus[rank][file]`` is full
  width, with ranks 1 and 8 empty.
* **Scale.** A Stockfish internal unit times ``--scale`` is a Basilisk
  centipawn: 0.557 by magnitude (B.0) or 0.613 by outcome (C.0).
* **Level, held.** Only each table's shape is transplanted:
  ``seed[s] = mean(current) + scale * (sf[s] - mean(sf))``, the means over
  the table's live squares (a pawn's are ranks 2 to 7). A table's level is
  its piece's effective value against the fixed material (C-19), and
  Stockfish 16's piece values belong with its own imbalance terms, which
  Basilisk does not have. Material stays the header's; dead pawn squares stay
  as they are.
* **Direction arm.** ``--flip`` mirrors every table by rank, the wrong
  orientation, for the two-arm direction test (C-22, C-23).
* **Orientation, checked at the import.** The parsed tables, in Stockfish's
  units before scale and level, must obey three chess facts that the parser
  did not produce: the middlegame king is worth more on its first rank than
  on its eighth, the middlegame knight more on the sixth rank than on the
  third, and the endgame pawn (``PBonus``, the separately indexed path) more
  on the sixth than on the third. An unflipped seed that breaks any is
  refused; a flipped seed must break all three. PeSTO is no reference here:
  its king table is not shaped like Stockfish 16's.

The vector's ``# root`` is the header's default vector, so a fit started from
it bakes into this header.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import bake  # noqa: E402

SF16_COMMIT = "68e1e9b3811e16cad014b590d7443b9063b3eb52"
PIECES = ("Pawn", "Knight", "Bishop", "Rook", "Queen", "King")
SCORE = re.compile(r"S\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)")


def read_psqt(stockfish: Path) -> str:
    resolved = subprocess.run(["git", "-C", str(stockfish), "rev-parse", "sf_16^{commit}"],
                              capture_output=True, text=True).stdout.strip()
    if resolved != SF16_COMMIT:
        raise SystemExit(f"{stockfish}: sf_16 resolves to {resolved or 'nothing'}, "
                         f"not the pinned {SF16_COMMIT}")
    done = subprocess.run(["git", "-C", str(stockfish), "show", f"{SF16_COMMIT}:src/psqt.cpp"],
                          capture_output=True, text=True)
    if done.returncode:
        raise SystemExit(f"git show psqt.cpp failed: {done.stderr.strip()}")
    return done.stdout


def sf16_tables(text: str) -> dict[str, list[list[int]]]:
    """{'mg'|'eg': six 64-square tables, pawn first}, White's view, a1 first."""
    bonus_at = text.index("Bonus[][RANK_NB]")
    pbonus_at = text.index("PBonus[RANK_NB][FILE_NB]")
    end = text.index("} // namespace", pbonus_at)
    bonus = [(int(a), int(b)) for a, b in SCORE.findall(text[bonus_at:pbonus_at])]
    pbonus = [(int(a), int(b)) for a, b in SCORE.findall(text[pbonus_at:end])]
    if len(bonus) != 5 * 8 * 4 or len(pbonus) != 6 * 8:
        raise SystemExit(f"psqt.cpp: read {len(bonus)} piece and {len(pbonus)} pawn scores, "
                         "expected 160 and 48")
    tables = {"mg": [[0] * 64 for _ in PIECES], "eg": [[0] * 64 for _ in PIECES]}
    for square in range(64):
        rank, file = divmod(square, 8)
        if 1 <= rank <= 6:
            mg, eg = pbonus[(rank - 1) * 8 + file]
            tables["mg"][0][square], tables["eg"][0][square] = mg, eg
        edge = min(file, 7 - file)
        for piece in range(1, 6):
            mg, eg = bonus[((piece - 1) * 8 + rank) * 4 + edge]
            tables["mg"][piece][square], tables["eg"][piece][square] = mg, eg
    return tables


ORIENTATION_FACTS = (
    # (phase, piece, higher rank, lower rank, what it says), ranks 1 to 8
    ("mg", 5, 1, 8, "the middlegame king is worth more on its first rank than on its eighth"),
    ("mg", 1, 6, 3, "the middlegame knight is worth more on the sixth rank than on the third"),
    ("eg", 0, 6, 3, "the endgame pawn is worth more on the sixth rank than on the third"),
)


def rank_mean(table: list[int], rank: int) -> float:
    return sum(table[(rank - 1) * 8 + f] for f in range(8)) / 8


def flipped(table: list[int]) -> list[int]:
    return [table[(7 - s // 8) * 8 + s % 8] for s in range(64)]


def orientation_violations(tables: dict[str, list[list[int]]]) -> list[str]:
    """The orientation facts a set of tables (White's view, a1 first) breaks."""
    return [fact for phase, piece, high, low, fact in ORIENTATION_FACTS
            if not rank_mean(tables[phase][piece], high) > rank_mean(tables[phase][piece], low)]


def check_orientation(tables: dict[str, list[list[int]]], flip: bool) -> None:
    """Refuse an unflipped seed that breaks a fact, and a flipped one that
    keeps any: the wrong arm must be wrong."""
    if flip:
        broken = orientation_violations({p: [flipped(t) for t in tables[p]] for p in tables})
        if len(broken) != len(ORIENTATION_FACTS):
            raise SystemExit("the flipped tables still keep an orientation fact; the parse is "
                             "not what --flip assumes")
    else:
        broken = orientation_violations(tables)
        if broken:
            raise SystemExit("the parsed tables are upside down: " + "; ".join(broken))


def live(piece: int) -> list[int]:
    return list(range(8, 56)) if piece == 0 else list(range(64))


def seed_vector(header_text: str, tables, scale: float, flip: bool):
    params = bake.parse_param_list(header_text)
    vector = bake.header_vector(header_text, params)
    current = {}
    for name, index, value in vector:
        current.setdefault(name, {})[index] = value
    replaced = {}
    for phase in ("mg", "eg"):
        for piece, piece_name in enumerate(PIECES):
            name = f"Pst{phase.capitalize()}{piece_name}"
            squares = live(piece)
            sf = tables[phase][piece]
            if flip:
                sf = flipped(sf)
            cur_mean = sum(current[name][s] for s in squares) / len(squares)
            sf_mean = sum(sf[s] for s in squares) / len(squares)
            table = dict(current[name])
            for s in squares:
                table[s] = round(cur_mean + scale * (sf[s] - sf_mean))
            replaced[name] = table
    out = [(name, index, replaced[name][index] if name in replaced else value)
           for name, index, value in vector]
    return out, bake.vector_hash(vector)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--stockfish", type=Path, default=Path("D:/code/stockfish"))
    parser.add_argument("--header", type=Path, default=ROOT / "src" / "eval_params.h")
    parser.add_argument("--scale", type=float, required=True)
    parser.add_argument("--flip", action="store_true")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args(argv)
    if args.out.exists():
        raise SystemExit(f"refusing to overwrite {args.out}")
    header_text = args.header.read_text(encoding="utf-8")
    tables = sf16_tables(read_psqt(args.stockfish))
    check_orientation(tables, args.flip)
    vector, root = seed_vector(header_text, tables, args.scale, args.flip)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8", newline="\n") as out:
        out.write(f"# {bake.VECTOR_SCHEMA}\n")
        out.write(f"# root {root}\n")
        out.write(f"# source sf_16 {SF16_COMMIT} src/psqt.cpp scale {args.scale}"
                  f"{' flipped' if args.flip else ''}, level held\n")
        for name, index, value in vector:
            out.write(f"{name} {index} {value}\n")
    print(f"wrote {args.out}: {len(vector)} slots, root {root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
