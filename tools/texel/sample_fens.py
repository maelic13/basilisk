#!/usr/bin/env python3
"""
Sample FEN positions into a fastchess EPD opening book.

This is intended for the post-SF-label path: use Beast/database FENs only as
diverse start positions, then let Basilisk self-play produce the labels. By
default it fills five equal material-phase reservoirs. Use
``--natural-phase-mix`` only for an intentional distribution experiment.

Accepted input line shapes:
  FEN
  FEN<TAB>target
  FEN;target

Output line shape:
  <piece-placement> <side> <castling> <ep>

Use --exclude-pgn when extending a deterministic self-play corpus: every FEN
already used as a PGN start position is removed before reservoir sampling, so
the new EPD cannot replay an earlier opening.

Example:
  python tools/texel/sample_fens.py A:\\Chess\\Beast\\data\\txt\\positions.txt \
    --out tools\\texel\\data\\beast_seed.epd --count 100000
"""

from __future__ import annotations

import argparse
import glob
import os
import random
import sys
from typing import Iterable


PHASE_W = {"n": 1, "b": 1, "r": 2, "q": 4,
           "N": 1, "B": 1, "R": 2, "Q": 4}
PHASE_BUCKETS = (
    ("opening", 20, 24),
    ("early_mid", 14, 19),
    ("middlegame", 8, 13),
    ("endgame", 3, 7),
    ("deep_endgame", 0, 2),
)


def phase_bucket(epd: str) -> int:
    phase = min(sum(PHASE_W.get(char, 0) for char in epd.split()[0]), 24)
    for index, (_, lo, hi) in enumerate(PHASE_BUCKETS):
        if lo <= phase <= hi:
            return index
    raise ValueError(f"phase outside 0..24: {phase}")


def bucket_targets(total: int) -> list[int]:
    base, extra = divmod(total, len(PHASE_BUCKETS))
    return [base + (1 if index < extra else 0) for index in range(len(PHASE_BUCKETS))]


def iter_files(sources: list[str]) -> Iterable[str]:
    for source in sources:
        if os.path.isdir(source):
            patterns = [
                os.path.join(source, "evaluated_positions_*.txt"),
                os.path.join(source, "positions*.txt"),
                os.path.join(source, "*.csv"),
            ]
            seen: set[str] = set()
            files: list[str] = []
            for pattern in patterns:
                for path in sorted(glob.glob(pattern)):
                    if path not in seen:
                        seen.add(path)
                        files.append(path)
            if not files:
                raise SystemExit(f"No supported text/csv files found under {source}")
            yield from files
        else:
            if not os.path.isfile(source):
                raise SystemExit(f"Not found: {source}")
            yield source


def parse_line(line: str, target_min: float | None, target_max: float | None) -> str | None:
    line = line.strip()
    if not line:
        return None

    target = None
    if "\t" in line:
        fen, target = line.rsplit("\t", 1)
    elif ";" in line:
        fen, target = line.rsplit(";", 1)
    else:
        fen = line

    if target_min is not None or target_max is not None:
        if target is None:
            return None
        try:
            value = float(target)
        except ValueError:
            return None
        if target_min is not None and value < target_min:
            return None
        if target_max is not None and value > target_max:
            return None

    fields = fen.split()
    if len(fields) < 4:
        return None
    return " ".join(fields[:4])


def quick_piece_count(epd: str) -> int:
    placement = epd.split()[0]
    return sum(1 for ch in placement if ch.isalpha())


def parse_pgn_fen_header(line: str) -> str | None:
    prefix = '[FEN "'
    if not line.startswith(prefix):
        return None
    end = line.rfind('"]')
    if end <= len(prefix):
        return None
    fields = line[len(prefix):end].split()
    if len(fields) < 4:
        return None
    return " ".join(fields[:4])


def load_pgn_exclusions(paths: list[str]) -> set[str]:
    excluded: set[str] = set()
    for path in paths:
        if not os.path.isfile(path):
            raise SystemExit(f"Exclusion PGN not found: {path}")
        with open(path, "r", encoding="utf-8", errors="replace") as pgn:
            for line in pgn:
                epd = parse_pgn_fen_header(line)
                if epd is not None:
                    excluded.add(epd)
    return excluded


def validate_epds(epds: list[str], args, limit: int | None = None) -> list[str]:
    target = args.count if limit is None else limit
    if args.no_validate:
        return epds[:target]

    try:
        import chess
    except ImportError:
        print("WARNING: python-chess not installed; writing unvalidated EPD sample.", file=sys.stderr)
        return epds[:target]

    out: list[str] = []
    seen: set[str] = set()
    for epd in epds:
        if epd in seen:
            continue
        seen.add(epd)

        pieces = quick_piece_count(epd)
        if pieces < args.min_pieces or pieces > args.max_pieces:
            continue

        try:
            board = chess.Board(epd + " 0 1")
        except ValueError:
            continue

        if not board.is_valid() or board.is_game_over(claim_draw=False):
            continue
        if not args.allow_check and board.is_check():
            continue
        if args.quiet:
            tactical = False
            for move in board.legal_moves:
                if board.is_capture(move) or move.promotion is not None:
                    tactical = True
                    break
            if tactical:
                continue

        out.append(epd)
        if len(out) >= target:
            break

    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("sources", nargs="+", help="FEN/txt/csv source files or directories")
    parser.add_argument("--out", default="tools/texel/data/beast_seed.epd")
    parser.add_argument("--count", type=int, default=100_000)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--max-read", type=int, default=0,
                        help="stop after reading this many lines; 0 = no cap")
    parser.add_argument("--target-min", type=float, default=None,
                        help="optional filter when input has target column")
    parser.add_argument("--target-max", type=float, default=None,
                        help="optional filter when input has target column")
    parser.add_argument("--oversample", type=int, default=3,
                        help="sample this many times --count before validation")
    parser.add_argument("--natural-phase-mix", action="store_true",
                        help="use one global reservoir instead of five equal phase reservoirs")
    parser.add_argument("--min-pieces", type=int, default=6)
    parser.add_argument("--max-pieces", type=int, default=32)
    parser.add_argument("--allow-check", action="store_true")
    parser.add_argument("--quiet", action="store_true",
                        help="keep only final sampled positions with no legal capture/promotion")
    parser.add_argument("--exclude-pgn", action="append", default=[], metavar="PGN",
                        help="exclude every [FEN] start position found in PGN; "
                             "repeat the option for multiple prior corpora")
    parser.add_argument("--no-validate", action="store_true",
                        help="skip python-chess validation of the final sample")
    parser.add_argument("--progress-every", type=int, default=5_000_000)
    args = parser.parse_args()

    if args.count <= 0:
        raise SystemExit("--count must be positive")
    if args.oversample <= 0:
        raise SystemExit("--oversample must be positive")
    if args.target_min is not None and args.target_max is not None and args.target_min > args.target_max:
        raise SystemExit("--target-min cannot exceed --target-max")

    rng = random.Random(args.seed)
    multiplier = 1 if args.no_validate else args.oversample
    targets = bucket_targets(args.count)
    if args.natural_phase_mix:
        reservoirs: list[list[str]] = [[]]
        capacities = [args.count * multiplier]
        phase_seen = [0]
    else:
        reservoirs = [[] for _ in PHASE_BUCKETS]
        capacities = [target * multiplier for target in targets]
        phase_seen = [0 for _ in PHASE_BUCKETS]
    excluded = load_pgn_exclusions(args.exclude_pgn)

    read = candidates = excluded_hits = 0
    files = list(iter_files(args.sources))
    rng.shuffle(files)
    print(f"Sampling from {len(files)} file(s)")
    print(f"Target output: {args.count:,} EPD positions -> {args.out}")
    if excluded:
        print(f"Excluded prior PGN openings: {len(excluded):,}")

    for file_index, path in enumerate(files, start=1):
        print(f"[{file_index}/{len(files)}] {path}")
        with open(path, "r", encoding="utf-8", errors="replace") as source:
            for line in source:
                read += 1
                epd = parse_line(line, args.target_min, args.target_max)
                if epd is None:
                    if args.max_read > 0 and read >= args.max_read:
                        break
                    continue
                if epd in excluded:
                    excluded_hits += 1
                    if args.max_read > 0 and read >= args.max_read:
                        break
                    continue

                candidates += 1
                bucket = 0 if args.natural_phase_mix else phase_bucket(epd)
                phase_seen[bucket] += 1
                reservoir = reservoirs[bucket]
                capacity = capacities[bucket]
                if len(reservoir) < capacity:
                    reservoir.append(epd)
                else:
                    j = rng.randrange(phase_seen[bucket])
                    if j < capacity:
                        reservoir[j] = epd

                if args.progress_every > 0 and read % args.progress_every == 0:
                    print(f"  read={read:,} candidates={candidates:,}")
                if args.max_read > 0 and read >= args.max_read:
                    break
        if args.max_read > 0 and read >= args.max_read:
            break

    selected: list[str] = []
    if args.natural_phase_mix:
        rng.shuffle(reservoirs[0])
        selected = validate_epds(reservoirs[0], args)
    else:
        print("\nPhase reservoirs:")
        for index, (name, _, _) in enumerate(PHASE_BUCKETS):
            rng.shuffle(reservoirs[index])
            bucket_selected = validate_epds(reservoirs[index], args, targets[index])
            selected.extend(bucket_selected)
            print(f"  {name:13}: saw={phase_seen[index]:,} "
                  f"selected={len(bucket_selected):,}/{targets[index]:,}")
        rng.shuffle(selected)

    print()
    print("Summary:")
    print(f"  Lines read : {read:,}")
    print(f"  Excluded   : {excluded_hits:,}")
    print(f"  Candidates : {candidates:,}")
    print(f"  Reservoir : {sum(len(reservoir) for reservoir in reservoirs):,}")
    print(f"  Selected  : {len(selected):,}")
    if len(selected) < args.count:
        print(f"ERROR: requested {args.count:,}, selected only {len(selected):,}; "
              "existing output left untouched.", file=sys.stderr)
        return 2

    out_dir = os.path.dirname(os.path.abspath(args.out))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    tmp = args.out + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as out:
        for epd in selected:
            out.write(epd + "\n")
    os.replace(tmp, args.out)
    print(f"  Written   : {len(selected):,} -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
