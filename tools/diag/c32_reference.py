#!/usr/bin/env python3
"""The evaluation rebuild's reference fixture (analysis/c31_rebuild_2026-11-25.md §9).

The rebuilt evaluation is checked against classical Stockfish 9587eeeb's own
evaluation, integer for integer. This tool produces the reference those
checks read:

  1. The producer is the oracle built with tools/oracle/oracle-eval-reference.patch
     after the ablation and transplant patches, pinned by SHA-256. Its
     `reference` command prints, for the current position and with contempt
     zero, the start (tables and imbalance), the seven families and four piece
     types, winnable()'s adjustment, the phase, the strong side, the material
     entry's factor and whether a scaling function gave it, the general
     heuristics' factor, the factor applied, the value before the grain and
     the tempo from White's side, the final value, the lazy exit and whether a
     specialised evaluation answered.
  2. The patch changes no value: the oracle's bench counts equal the C.2.9
     oracle's at its four configurations (C.3.1's check 1), and every row's
     final value equals the stock 9587eeeb build's `evalraw`.
  3. Whether the material key has a registered scaling function for
     either side (`registered`): the reference's material entry then
     returns before it computes the imbalance (Amendment 3).
  4. The rows: every 21st of C.0's gate-game positions that is not in check
     (the 12,119 of BAS-O15's checks), then the hand-built rows of
     tests/fixtures/eval_reference_extra.txt and the generated ones of
     tests/fixtures/eval_reference_generated.txt, each followed by its
     colour-flipped mirror. Every halfmove clock is set to 0, so neither
     evaluation's rule-50 damping acts.

Writes tests/fixtures/eval_reference.tsv (one row per position: the FEN, its
source, then the record's integers) and tests/fixtures/eval_reference.json
(the producer's identity, the checks and the file's hash). The fixture is
append-only: a rerun must reproduce every existing row exactly, and refuses
otherwise.

Usage:
  python tools/diag/c32_reference.py [--oracle <exe>] [--sample 12000]
"""

from __future__ import annotations

import argparse
import csv
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import c31_checks as c31  # noqa: E402

try:
    import chess
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

REPO = c31.REPO
FIXTURES = REPO / "tests" / "fixtures"
EXTRA = FIXTURES / "eval_reference_extra.txt"
# Rows tools/diag/c32_coverage_rows.py generates for slots game rows rarely reach.
GENERATED = FIXTURES / "eval_reference_generated.txt"
TSV = FIXTURES / "eval_reference.tsv"
MANIFEST = FIXTURES / "eval_reference.json"
PATCHES = [REPO / "tools" / "oracle" / name for name in
           ("bridge-board-access.patch", "oracle-eval-ablate.patch",
            "oracle-eval-transplant.patch", "oracle-eval-reference.patch")]

# The producer: the oracle around src/ of the accepted head with the four
# patches above (tools/oracle/build_oracle.ps1).
ORACLE = {"path": c31.ENGINES / "oracle-c32-0fe7b13-reference.exe",
          "sha": "A14D76E4A960F6B4F69960873AB939D7F22255E97D3A7F6FF866951AB2883609"}

PAIRS = ["psq", "imbalance", *c31.FAMILIES, *c31.PIECES, "complexity", "adjusted"]
SCALARS = ["specialised", "lazy", "ph", "strong", "entrysf", "scalefn", "generalsf", "sf", "v", "final",
           "registered"]
COLUMNS = (["fen", "source", "specialised", "lazy"]
           + [f"{name}_{half}" for name in PAIRS for half in ("mg", "eg")]
           + [s for s in SCALARS if s not in ("specialised", "lazy")])


def clock_zero(board: chess.Board) -> str:
    board.halfmove_clock = 0
    return board.fen()


def checked(fen: str, where: str) -> chess.Board:
    board = chess.Board(fen)
    if not board.is_valid():
        raise c31.Failed(f"{where}: not a valid position: {fen} ({board.status()!r})")
    if board.is_check():
        raise c31.Failed(f"{where}: the side to move is in check: {fen}")
    return board


def rows_wanted(sample: int) -> tuple[list[tuple[str, str]], dict]:
    """(fen, source) in the fixture's order: the game sample, then each hand-built row and its mirror."""
    fens, census = c31.sample_positions(sample)
    rows = [(clock_zero(chess.Board(fen)), "game") for fen in fens]
    for path, kind in ((EXTRA, "extra"), (GENERATED, "generated")):
        added = 0
        lines = path.read_text(encoding="utf-8").splitlines() if path.exists() else []
        for number, line in enumerate(lines, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fen, _, note = line.partition(" ; ")
            board = checked(fen.strip(), f"{path.name}:{number}")
            tag = note.strip().replace("\t", " ") or f"line {number}"
            rows.append((clock_zero(board), f"{kind}: {tag}"))
            mirror = board.mirror()
            checked(mirror.fen(), f"{path.name}:{number} mirrored")
            rows.append((clock_zero(mirror), f"{kind} mirrored: {tag}"))
            added += 1
        census[kind] = added
    census["rows"] = len(rows)
    return rows, census


def parse_reference(line: str) -> dict:
    tokens = line.split()
    if not tokens or tokens[0] != "reference" or "refused" in tokens or "none" in tokens:
        raise c31.Failed(f"not a reference line: {line!r}")
    values, i = {}, 1
    while i < len(tokens):
        key = tokens[i]
        if key in PAIRS:
            values[f"{key}_mg"], values[f"{key}_eg"] = int(tokens[i + 1]), int(tokens[i + 2])
            i += 3
        else:
            values[key] = int(tokens[i + 1])
            i += 2
    missing = [c for c in COLUMNS[2:] if c not in values]
    if missing:
        raise c31.Failed(f"reference line lacks {missing}: {line!r}")
    return values


def reference_lines(oracle: pathlib.Path, fens: list[str]) -> list[dict]:
    lines = c31.setoptions({c31.ABLATE: 0})
    for fen in fens:
        lines += [f"position fen {fen}", "reference"]
    done = c31.run_uci(oracle, lines)
    out = [l for l in done.stdout.splitlines() if l.startswith("reference")]
    if done.returncode != 0 or len(out) != len(fens):
        raise c31.Failed(f"reference lines: {len(out)} of {len(fens)} (exit {done.returncode})")
    return [parse_reference(l) for l in out]


def evalraw(fens: list[str]) -> list[int]:
    donor = c31.pinned(c31.DONOR)
    lines = []
    for fen in fens:
        lines += [f"position fen {fen}", "evalraw"]
    done = c31.run_uci(donor, lines)
    out = [l.split()[1] for l in done.stdout.splitlines() if l.startswith("evalraw ")]
    if done.returncode != 0 or len(out) != len(fens):
        raise c31.Failed(f"evalraw: {len(out)} of {len(fens)} (exit {done.returncode})")
    return [int(v) for v in out]


def read_existing() -> list[dict]:
    if not TSV.exists():
        return []
    with TSV.open(encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--oracle", type=pathlib.Path, default=ORACLE["path"])
    parser.add_argument("--sample", type=int, default=12000)
    args = parser.parse_args(argv)
    try:
        oracle = c31.pinned({"path": args.oracle, "sha": ORACLE["sha"]})
        instrument = c31.check_old_instrument(oracle)
        print("Bench counts equal the C.2.9 oracle's at its four configurations.")

        rows, census = rows_wanted(args.sample)
        fens = [fen for fen, _ in rows]
        records = reference_lines(oracle, fens)
        raw = evalraw(fens)
        differ = [(fens[i], r["final"], raw[i]) for i, r in enumerate(records) if r["final"] != raw[i]]
        if differ:
            raise c31.Failed(f"{len(differ)} rows' final value is not the stock evalraw, first {differ[:3]}")
        print(f"{len(rows)} rows: every final value equals the stock build's evalraw.")

        table = [{"fen": fen, "source": source, **{c: str(r[c]) for c in COLUMNS[2:]}}
                 for (fen, source), r in zip(rows, records)]
        # Append-only: every existing row keeps its place and every value it
        # holds. A column may be added; rows may be appended.
        existing = read_existing()
        if len(existing) > len(table):
            raise c31.Failed(f"the fixture is append-only: {len(existing)} rows exist, {len(table)} produced")
        for i, old in enumerate(existing):
            if {k: table[i].get(k) for k in old} != old:
                raise c31.Failed(f"the fixture is append-only and row {i + 2} of {TSV.name} would change")

        with TSV.open("w", encoding="utf-8", newline="\n") as stream:
            writer = csv.DictWriter(stream, fieldnames=COLUMNS, delimiter="\t", lineterminator="\n")
            writer.writeheader()
            writer.writerows(table)
        manifest = {
            "schema": "basilisk-eval-reference-v1",
            "producer": {"oracle": oracle.name, "oracle_sha256": ORACLE["sha"],
                         "patches": {p.name: c31.sha256(p) for p in PATCHES},
                         "tool": "tools/diag/c32_reference.py"},
            "evalraw_donor": {"exe": c31.DONOR["path"].name, "sha256": c31.DONOR["sha"],
                              "rows_equal": len(rows)},
            "positions": {"source": "tools/results/c0-screen/positions.csv",
                          "sha256": c31.POSITIONS["sha"], **census},
            "bench_check": instrument["rows"],
            "counts": {"specialised": sum(r["specialised"] for r in records),
                       "lazy1": sum(r["lazy"] == 1 for r in records),
                       "lazy2": sum(r["lazy"] == 2 for r in records),
                       "scaling_function": sum(r["scalefn"] for r in records),
                       "entry_not_normal": sum(r["entrysf"] != 64 for r in records),
                       "registered": sum(r["registered"] for r in records)},
            "fixture": {"file": TSV.name, "rows": len(table), "sha256": c31.sha256(TSV)},
        }
        MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
        print(f"Wrote {TSV} ({len(table)} rows, sha256 {manifest['fixture']['sha256']}).")
        print(json.dumps(manifest["counts"]))
    except c31.Failed as failure:
        print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
