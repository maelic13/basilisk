#!/usr/bin/env python3
"""PGN -> phase-balanced FEN;result data for Basilisk Texel tuning.

The dataset contract is explicit and deterministic:

* five equal material-phase reservoirs by default;
* positions sampled per phase inside each game before the global reservoirs;
* game-level train/holdout assignment and global FEN de-duplication;
* quiet positions only by default (no check, capture/promotion about to be
  played, or obvious winning capture for the side to move);
* atomic publication only when every requested quota is available.

Beast/database FENs are already positions rather than played book moves, so the
default is ``--skip-start 0``.  Use a positive value only for a conventional
opening-book corpus where early plies are intentionally excluded.
"""

from __future__ import annotations

import argparse
import glob
import hashlib
import math
import os
import random
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

try:
    import chess
    import chess.pgn
except ImportError:
    print("ERROR: python-chess not installed. Run: pip install chess", file=sys.stderr)
    sys.exit(1)


RESULT_MAP = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}

# Matches Basilisk's tapered evaluation: N=B=1, R=2, Q=4, capped at 24.
PHASE_W = {chess.KNIGHT: 1, chess.BISHOP: 1, chess.ROOK: 2, chess.QUEEN: 4}
PHASE_BUCKETS = (
    ("opening", 20, 24),
    ("early_mid", 14, 19),
    ("middlegame", 8, 13),
    ("endgame", 3, 7),
    ("deep_endgame", 0, 2),
)
BUCKET_NAMES = tuple(name for name, _, _ in PHASE_BUCKETS)

PIECE_VALUE = {
    chess.PAWN: 1,
    chess.KNIGHT: 3,
    chess.BISHOP: 3,
    chess.ROOK: 5,
    chess.QUEEN: 9,
    chess.KING: 20,
}


def game_phase(board: "chess.Board") -> int:
    return min(
        24,
        sum(
            PHASE_W[piece_type] * len(board.pieces(piece_type, color))
            for piece_type in PHASE_W
            for color in (chess.WHITE, chess.BLACK)
        ),
    )


def phase_bucket(phase: int) -> int:
    for index, (_, lo, hi) in enumerate(PHASE_BUCKETS):
        if lo <= phase <= hi:
            return index
    raise ValueError(f"phase outside 0..24: {phase}")


def fen_key(fen: str) -> str:
    """Position, side, castling and en-passant; clocks do not affect HCE."""
    return " ".join(fen.split()[:4])


def game_digest(game: "chess.pgn.Game") -> int:
    """Stable identity including the supplied start and the played moves."""
    start = fen_key(game.board().fen())
    moves = " ".join(move.uci() for move in game.mainline_moves())
    payload = f"{start}|{moves}".encode("utf-8")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "big")


def has_winning_capture(board: "chess.Board") -> bool:
    """Fast SEE>0 proxy, matching the upgraded Rarog extraction policy.

    It rejects an immediately profitable legal capture: a more valuable victim,
    or a victim on a square not attacked by the opponent.  Basilisk's played-move
    capture filter remains in force as an independent guard.
    """
    for move in board.generate_legal_captures():
        victim = board.piece_type_at(move.to_square) or chess.PAWN  # en passant
        attacker = board.piece_type_at(move.from_square)
        if PIECE_VALUE[victim] > PIECE_VALUE[attacker]:
            return True
        if not board.is_attacked_by(not board.turn, move.to_square):
            return True
    return False


@dataclass
class Reservoir:
    """Uniform fixed-size sample from a stream of unknown length."""

    capacity: int
    rng: random.Random

    def __post_init__(self) -> None:
        self.seen = 0
        self.items: list[tuple[str, float, int]] = []

    def offer(self, item: tuple[str, float, int]) -> None:
        self.seen += 1
        if len(self.items) < self.capacity:
            self.items.append(item)
            return
        pick = self.rng.randrange(self.seen)
        if pick < self.capacity:
            self.items[pick] = item


def allocate(total: int, weights: list[float]) -> list[int]:
    """Largest-remainder allocation whose entries sum exactly to total."""
    if total < 0 or len(weights) != len(PHASE_BUCKETS) or any(weight <= 0 for weight in weights):
        raise ValueError("total must be non-negative and five phase weights positive")
    weight_sum = sum(weights)
    raw = [total * weight / weight_sum for weight in weights]
    out = [math.floor(value) for value in raw]
    order = sorted(
        range(len(weights)),
        key=lambda index: (raw[index] - out[index], -index),
        reverse=True,
    )
    for index in order[: total - sum(out)]:
        out[index] += 1
    return out


def parse_phase_weights(value: str) -> list[float]:
    try:
        weights = [float(part) for part in value.split(",")]
    except ValueError as exc:
        raise argparse.ArgumentTypeError("phase weights must be comma-separated numbers") from exc
    if len(weights) != len(PHASE_BUCKETS) or any(weight <= 0 for weight in weights):
        raise argparse.ArgumentTypeError("phase weights must contain five positive numbers")
    return weights


def process_game(
    game: "chess.pgn.Game",
    skip_start: int,
    skip_end: int,
    max_per_phase_per_game: int,
    max_per_game: int,
    quiet_filter: bool,
    rng: random.Random,
) -> tuple[list[tuple[str, int]], int]:
    """Return phase-stratified candidates and quiet-filter reject count."""
    if game.headers.get("Result", "*") not in RESULT_MAP:
        return [], 0

    board = game.board()
    nodes = list(game.mainline())
    by_phase: list[list[tuple[str, int]]] = [[] for _ in PHASE_BUCKETS]

    for ply_index, node in enumerate(nodes):
        move = node.move
        if (
            ply_index >= skip_start
            and ply_index < len(nodes) - skip_end
            and not board.is_check()
            and not board.is_capture(move)
            and move.promotion is None
        ):
            bucket = phase_bucket(game_phase(board))
            by_phase[bucket].append((board.fen(), bucket))
        board.push(move)

    selected: list[tuple[str, int]] = []
    quiet_rejected = 0
    for candidates in by_phase:
        # Bound the expensive board rebuild/filter before checking quietness.
        check_cap = max_per_phase_per_game * (2 if quiet_filter else 1)
        if len(candidates) > check_cap:
            candidates = rng.sample(candidates, check_cap)
        if quiet_filter:
            quiet: list[tuple[str, int]] = []
            for item in candidates:
                if has_winning_capture(chess.Board(item[0])):
                    quiet_rejected += 1
                else:
                    quiet.append(item)
            candidates = quiet
        if len(candidates) > max_per_phase_per_game:
            candidates = rng.sample(candidates, max_per_phase_per_game)
        selected.extend(candidates)

    if max_per_game > 0 and len(selected) > max_per_game:
        selected = rng.sample(selected, max_per_game)
    return selected, quiet_rejected


def iter_pgn_paths(inputs: list[str]) -> list[Path]:
    paths: list[Path] = []
    seen: set[Path] = set()
    for source in inputs:
        matches = sorted(glob.glob(source))
        if not matches and os.path.exists(source):
            matches = [source]
        for match in matches:
            path = Path(match).resolve()
            candidates = sorted(path.glob("*.pgn")) if path.is_dir() else [path]
            for candidate in candidates:
                if candidate not in seen:
                    seen.add(candidate)
                    paths.append(candidate)
    if not paths or any(not path.is_file() for path in paths):
        raise SystemExit("No readable PGN inputs found")
    return paths


def stage_rows(path: Path, rows: Iterable[tuple[str, float]]) -> Path:
    tmp = path.with_name(path.name + ".tmp")
    with tmp.open("w", encoding="utf-8", newline="\n") as out:
        for fen, target in rows:
            out.write(f"{fen};{target:g}\n")
    return tmp


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("pgn", nargs="+", help="PGN files, globs, or directories")
    parser.add_argument("--out-dir", default="", metavar="DIR")
    parser.add_argument("--train", default="train.csv", metavar="FILE")
    parser.add_argument("--holdout", default="holdout.csv", metavar="FILE")
    parser.add_argument("--target-train", default=3_500_000, type=int, metavar="N")
    parser.add_argument("--phase-weights", default=parse_phase_weights("1,1,1,1,1"),
                        type=parse_phase_weights, metavar="A,B,C,D,E")
    parser.add_argument("--holdout-pct", default=5.0, type=float, metavar="N")
    parser.add_argument("--max-per-phase-per-game", default=8, type=int, metavar="N")
    parser.add_argument("--max-per-game", default=0, type=int, metavar="N")
    parser.add_argument("--skip-start", default=0, type=int, metavar="N")
    parser.add_argument("--skip-end", default=6, type=int, metavar="N")
    parser.add_argument("--seed", default=42, type=int, metavar="N")
    parser.add_argument("--preflight-games", default=0, type=int, metavar="N",
                        help="estimate required games from the first N games; write nothing")
    parser.add_argument("--preflight-safety", default=1.20, type=float, metavar="X")
    parser.add_argument("--no-quiet-filter", dest="quiet_filter", action="store_false")
    args = parser.parse_args()

    if args.target_train <= 0:
        parser.error("--target-train must be positive")
    if not 0.0 <= args.holdout_pct < 100.0:
        parser.error("--holdout-pct must be in [0,100)")
    if args.max_per_phase_per_game <= 0:
        parser.error("--max-per-phase-per-game must be positive")
    if args.skip_start < 0 or args.skip_end < 0:
        parser.error("skip counts cannot be negative")

    paths = iter_pgn_paths(args.pgn)
    out_dir = Path(args.out_dir).resolve() if args.out_dir else paths[0].parent
    quotas = allocate(args.target_train, args.phase_weights)
    holdout_total = round(args.target_train * args.holdout_pct / (100.0 - args.holdout_pct))
    holdout_quotas = allocate(holdout_total, args.phase_weights)
    train = [Reservoir(quota, random.Random(args.seed ^ (index + 1) * 0x9E3779B1))
             for index, quota in enumerate(quotas)]
    holdout = [Reservoir(quota, random.Random(args.seed ^ (index + 11) * 0x85EBCA77))
               for index, quota in enumerate(holdout_quotas)]

    print(f"Target train: {args.target_train:,} | holdout: {holdout_total:,}")
    print("Phase quotas: " + ", ".join(
        f"{BUCKET_NAMES[index]}={quota:,}" for index, quota in enumerate(quotas)))
    print(f"skip_start={args.skip_start}, skip_end={args.skip_end}, "
          f"max/phase/game={args.max_per_phase_per_game}, "
          f"quiet_filter={'on' if args.quiet_filter else 'OFF'}")

    seen: set[str] = set()
    games_total = games_skipped = raw_candidates = quiet_rejected = 0
    unique_by_phase = [0] * len(PHASE_BUCKETS)

    for path in paths:
        print(f"Reading {path} ...")
        with path.open(encoding="utf-8", errors="replace") as pgn_file:
            while not args.preflight_games or games_total < args.preflight_games:
                try:
                    game = chess.pgn.read_game(pgn_file)
                except Exception as exc:
                    print(f"  WARNING: parse error, skipping game: {exc}", file=sys.stderr)
                    games_skipped += 1
                    continue
                if game is None:
                    break
                games_total += 1
                digest = game_digest(game)
                rng = random.Random(args.seed ^ digest)
                candidates, rejected = process_game(
                    game, args.skip_start, args.skip_end,
                    args.max_per_phase_per_game, args.max_per_game,
                    args.quiet_filter, rng,
                )
                quiet_rejected += rejected
                if not candidates:
                    games_skipped += 1
                    continue
                raw_candidates += len(candidates)
                result = RESULT_MAP[game.headers["Result"]]
                is_holdout = digest % 10_000 < round(args.holdout_pct * 100)
                reservoirs = holdout if is_holdout else train
                for fen, bucket in candidates:
                    key = fen_key(fen)
                    if key in seen:
                        continue
                    seen.add(key)
                    unique_by_phase[bucket] += 1
                    reservoirs[bucket].offer((fen, result, bucket))

        if args.preflight_games and games_total >= args.preflight_games:
            break

    print(f"\nGames={games_total:,} raw={raw_candidates:,} unique={len(seen):,} "
          f"quiet_rejected={quiet_rejected:,}")
    if args.preflight_games:
        print("Preflight estimate (including safety):")
        required = 0
        train_fraction = 1.0 - args.holdout_pct / 100.0
        for index, quota in enumerate(quotas):
            rate = unique_by_phase[index] / max(games_total, 1) * train_fraction
            estimate = math.ceil(quota / rate * args.preflight_safety) if rate else math.inf
            if estimate != math.inf:
                required = max(required, int(estimate))
            print(f"  {BUCKET_NAMES[index]:13} rate={rate:6.3f}/game required={estimate:,}")
        if required:
            print(f"Recommended minimum: {required:,} independent games")
        return 0

    train_counts = [len(reservoir.items) for reservoir in train]
    holdout_counts = [len(reservoir.items) for reservoir in holdout]
    for index, name in enumerate(BUCKET_NAMES):
        print(f"  {name:13}: train {train_counts[index]:,}/{quotas[index]:,} "
              f"holdout {holdout_counts[index]:,}/{holdout_quotas[index]:,} "
              f"eligible train={train[index].seen:,}")

    short = [index for index in range(len(quotas))
             if train_counts[index] < quotas[index] or holdout_counts[index] < holdout_quotas[index]]
    if short:
        print("ERROR: phase quotas not met; existing outputs left untouched.", file=sys.stderr)
        return 2

    rng = random.Random(args.seed)
    train_rows = [(fen, result) for reservoir in train for fen, result, _ in reservoir.items]
    holdout_rows = [(fen, result) for reservoir in holdout for fen, result, _ in reservoir.items]
    rng.shuffle(train_rows)
    rng.shuffle(holdout_rows)
    out_dir.mkdir(parents=True, exist_ok=True)
    train_path = out_dir / args.train
    holdout_path = out_dir / args.holdout
    train_tmp = stage_rows(train_path, train_rows)
    holdout_tmp = stage_rows(holdout_path, holdout_rows)
    os.replace(train_tmp, train_path)
    os.replace(holdout_tmp, holdout_path)
    print(f"Wrote {len(train_rows):,} train and {len(holdout_rows):,} holdout rows.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
