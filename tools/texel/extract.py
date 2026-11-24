#!/usr/bin/env python3
"""PGN -> phase-balanced Texel rows for Basilisk, contract basilisk-texel-rows-v1.

The contract (PLAN C.2.1.1):

* **Rows.** Each published file opens with ``# basilisk-texel-rows-v1 <split>``
  and the header ``fen,result,game_id,start_id,ply,phase``. ``result`` is the
  game's result from White's side, exactly ``1``, ``0.5`` or ``0``. ``game_id``
  and ``start_id`` are 16 hex digits of SHA-256 over the game (its start and
  moves) and over its start position; ``ply`` counts moves played from the
  start (0 is the start itself); ``phase`` is Basilisk's 0..24.
* **Labels.** A game is labelled only if it ended by the rules: its result is a
  game result, its PGN ``Termination`` (when present) is ``normal``, and its
  final position is a checkmate the result agrees with, or a stalemate,
  insufficient material, the fifty-move rule or a threefold repetition with a
  drawn result. Every other game is rejected and counted by its reason, and
  the run fails when the rejected share exceeds ``--max-reject-pct``.
* **Splits.** Train, validation and test are assigned by the start's hash, so
  every game from one start lands in one split and input order cannot move
  it. The test split is opened once, after model selection, through
  ``claim_test.py``.
* **Publication.** All three files and ``manifest.json`` are published together,
  only when every phase quota is met, and never over an existing file.

Positions are sampled per phase inside each game before they enter fixed-size
reservoirs, quiet positions only by default, de-duplicated over the whole run
on the position and its rule-50 clock (the served score's damping reads it).
Beast/database FENs are already positions rather than played book moves, so
the default is ``--skip-start 0``.

Requires python-chess (``pip install chess``).
"""

from __future__ import annotations

import argparse
import collections
import glob
import hashlib
import json
import math
import os
import random
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

try:
    import chess
    import chess.pgn
except ImportError:
    print("ERROR: python-chess not installed. Run: pip install chess", file=sys.stderr)
    sys.exit(1)


SCHEMA = "basilisk-texel-rows-v1"
COLUMNS = ("fen", "result", "game_id", "start_id", "ply", "phase")
SPLITS = ("train", "validation", "test")
RESULT_MAP = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}
RESULT_TEXT = {1.0: "1", 0.5: "0.5", 0.0: "0"}

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

# Reasons a whole game is not labelled. Each is counted in the manifest.
REJECT_REASONS = (
    "parse_error",
    "result_not_a_game_result",
    "termination_not_normal",
    "result_contradicts_mate",
    "result_contradicts_draw",
    "not_a_rules_ending",
    "no_candidate_position",
)


def game_phase(board: "chess.Board") -> int:
    return min(
        24,
        sum(
            PHASE_W[piece_type] * len(board.pieces(piece_type, color))
            for piece_type in PHASE_W
            for color in (chess.WHITE, chess.BLACK)
        ),
    )


def placement_phase(placement: str) -> int:
    """game_phase() from a FEN's piece placement alone, without a board: the
    one phase rule the book sampler and the extractor share."""
    weights = {"n": 1, "b": 1, "r": 2, "q": 4}
    return min(24, sum(weights.get(ch.lower(), 0) for ch in placement))


def phase_bucket(phase: int) -> int:
    for index, (_, lo, hi) in enumerate(PHASE_BUCKETS):
        if lo <= phase <= hi:
            return index
    raise ValueError(f"phase outside 0..24: {phase}")


def fen_key(fen: str) -> str:
    """The de-duplication key: position, side, castling, en passant and the
    rule-50 clock, which the served score reads through its damping. Two rows
    that differ only in the clock are two different labelled scores."""
    return " ".join(fen.split()[:5])


def start_key_of(fen: str) -> str:
    """A start's identity, for the split: the position without its clocks."""
    return " ".join(fen.split()[:4])


def start_key(game: "chess.pgn.Game") -> str:
    """Every game from one supplied start belongs to one split."""
    return start_key_of(game.board().fen())


def digest_hex(payload: str) -> str:
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def start_id(game: "chess.pgn.Game") -> str:
    return digest_hex(start_key(game))


def game_id(game: "chess.pgn.Game") -> str:
    """The game's identity: its start and the moves played from it."""
    moves = " ".join(move.uci() for move in game.mainline_moves())
    return digest_hex(f"{start_key(game)}|{moves}")


def split_for_start(start: str, validation_pct: float, test_pct: float) -> str:
    """The split of a start, by its hash: test first, then validation."""
    slot = int.from_bytes(hashlib.sha256(start.encode("utf-8")).digest()[:8], "big") % 1_000_000
    test_cut = round(test_pct * 10_000)
    validation_cut = test_cut + round(validation_pct * 10_000)
    if slot < test_cut:
        return "test"
    if slot < validation_cut:
        return "validation"
    return "train"


def header_reason(game: "chess.pgn.Game") -> str | None:
    """The reason the headers alone disqualify a game, or None."""
    if game.headers.get("Result", "*") not in RESULT_MAP:
        return "result_not_a_game_result"
    termination = game.headers.get("Termination")
    if termination is not None and termination.strip().lower() != "normal":
        return "termination_not_normal"
    return None


def ending_reason(result: str, board: "chess.Board") -> str | None:
    """None if the final position ends the game by the rules with the recorded
    result, else the reason the game is not labelled."""
    if board.is_checkmate():
        winner = "0-1" if board.turn == chess.WHITE else "1-0"
        return None if result == winner else "result_contradicts_mate"
    drawn = (board.is_stalemate()
             or board.is_insufficient_material()
             or board.halfmove_clock >= 100
             or board.is_repetition(3))
    if drawn:
        return None if result == "1/2-1/2" else "result_contradicts_draw"
    return "not_a_rules_ending"


def has_winning_capture(board: "chess.Board") -> bool:
    """Fast SEE>0 proxy: an immediately profitable legal capture, a more
    valuable victim or one on a square the opponent does not attack."""
    for move in board.generate_legal_captures():
        victim = board.piece_type_at(move.to_square) or chess.PAWN  # en passant
        attacker = board.piece_type_at(move.from_square)
        if PIECE_VALUE[victim] > PIECE_VALUE[attacker]:
            return True
        if not board.is_attacked_by(not board.turn, move.to_square):
            return True
    return False


@dataclass
class Candidate:
    fen: str
    ply: int
    phase: int

    @property
    def bucket(self) -> int:
        return phase_bucket(self.phase)


@dataclass
class GameRows:
    """One game's outcome: the reason it is not labelled, or its rows."""
    reject: str | None
    game_id: str = ""
    start_id: str = ""
    split: str = ""
    result: float = 0.5
    candidates: list[Candidate] = field(default_factory=list)
    quiet_rejected: int = 0


@dataclass(frozen=True)
class Options:
    skip_start: int
    skip_end: int
    max_per_phase_per_game: int
    max_per_game: int
    quiet_filter: bool
    validation_pct: float
    test_pct: float
    seed: int


def rules_ending(game: "chess.pgn.Game") -> str | None:
    """None if the game ended by the rules with the result it records, else the
    reason it is not labelled."""
    return header_reason(game) or ending_reason(game.headers["Result"], game.end().board())


def sample_positions(game: "chess.pgn.Game", opts: Options, rng: random.Random
                     ) -> tuple[list[Candidate], int, "chess.Board"]:
    """Phase-stratified candidates, the quiet-filter reject count, and the
    final position."""
    board = game.board()
    nodes = list(game.mainline())
    by_phase: list[list[Candidate]] = [[] for _ in PHASE_BUCKETS]

    for ply, node in enumerate(nodes):
        move = node.move
        if (
            ply >= opts.skip_start
            and ply < len(nodes) - opts.skip_end
            and not board.is_check()
            and not board.is_capture(move)
            and move.promotion is None
        ):
            phase = game_phase(board)
            by_phase[phase_bucket(phase)].append(Candidate(board.fen(), ply, phase))
        board.push(move)

    selected: list[Candidate] = []
    quiet_rejected = 0
    for candidates in by_phase:
        # Bound the expensive board rebuild and filter before checking quietness.
        check_cap = opts.max_per_phase_per_game * (2 if opts.quiet_filter else 1)
        if len(candidates) > check_cap:
            candidates = rng.sample(candidates, check_cap)
        if opts.quiet_filter:
            quiet: list[Candidate] = []
            for item in candidates:
                if has_winning_capture(chess.Board(item.fen)):
                    quiet_rejected += 1
                else:
                    quiet.append(item)
            candidates = quiet
        if len(candidates) > opts.max_per_phase_per_game:
            candidates = rng.sample(candidates, opts.max_per_phase_per_game)
        selected.extend(candidates)

    if opts.max_per_game > 0 and len(selected) > opts.max_per_game:
        selected = rng.sample(selected, opts.max_per_game)
    return selected, quiet_rejected, board


def game_rows(game: "chess.pgn.Game", opts: Options) -> GameRows:
    """Everything the contract decides about one game, in one place, so that
    the sequential and the parallel extractor cannot disagree."""
    reason = header_reason(game)
    if reason is not None:
        return GameRows(reason)
    gid, sid = game_id(game), start_id(game)
    rng = random.Random(opts.seed ^ int(gid, 16))
    candidates, quiet_rejected, final = sample_positions(game, opts, rng)
    reason = ending_reason(game.headers["Result"], final)
    if reason is not None:
        return GameRows(reason)
    if not candidates:
        return GameRows("no_candidate_position", gid, sid, quiet_rejected=quiet_rejected)
    return GameRows(
        None, gid, sid,
        split_for_start(start_key(game), opts.validation_pct, opts.test_pct),
        RESULT_MAP[game.headers["Result"]], candidates, quiet_rejected,
    )


@dataclass
class Reservoir:
    """Uniform fixed-size sample from a stream of unknown length."""

    capacity: int
    rng: random.Random

    def __post_init__(self) -> None:
        self.seen = 0
        self.items: list[tuple] = []

    def offer(self, item: tuple) -> None:
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


def split_counts(train: int, validation_pct: float, test_pct: float) -> dict[str, int]:
    """Rows per split: the train target, and validation and test in proportion
    to their shares of the starts."""
    train_fraction = 1.0 - (validation_pct + test_pct) / 100.0
    return {
        "train": train,
        "validation": round(train * validation_pct / 100.0 / train_fraction),
        "test": round(train * test_pct / 100.0 / train_fraction),
    }


class Collector:
    """The run's reservoirs, de-duplication and accounting."""

    def __init__(self, target_train: int, weights: list[float], validation_pct: float,
                 test_pct: float, seed: int) -> None:
        self.counts = split_counts(target_train, validation_pct, test_pct)
        self.quotas = {split: allocate(self.counts[split], weights) for split in SPLITS}
        self.reservoirs = {
            split: [
                Reservoir(quota, random.Random(
                    seed ^ (split_index + 1) * 0x9E3779B1 ^ (phase + 1) * 0x85EBCA77))
                for phase, quota in enumerate(self.quotas[split])
            ]
            for split_index, split in enumerate(SPLITS)
        }
        self.seen: set[str] = set()
        self.games_read = 0
        self.games_by_split = {split: 0 for split in SPLITS}
        self.rejected = collections.Counter({reason: 0 for reason in REJECT_REASONS})
        self.candidates = self.quiet_rejected = self.duplicates = 0
        self.unique_by_split = {split: [0] * len(PHASE_BUCKETS) for split in SPLITS}

    def parse_error(self) -> None:
        self.games_read += 1
        self.rejected["parse_error"] += 1

    def add(self, rows: GameRows) -> None:
        self.games_read += 1
        self.quiet_rejected += rows.quiet_rejected
        if rows.reject is not None:
            self.rejected[rows.reject] += 1
            return
        self.games_by_split[rows.split] += 1
        self.candidates += len(rows.candidates)
        for candidate in rows.candidates:
            key = fen_key(candidate.fen)
            if key in self.seen:
                self.duplicates += 1
                continue
            self.seen.add(key)
            bucket = candidate.bucket
            self.unique_by_split[rows.split][bucket] += 1
            self.reservoirs[rows.split][bucket].offer(
                (candidate.fen, rows.result, rows.game_id, rows.start_id,
                 candidate.ply, candidate.phase))

    def rejected_games(self) -> int:
        return sum(self.rejected.values()) - self.rejected["no_candidate_position"]

    def short(self) -> list[tuple[str, str]]:
        return [(split, BUCKET_NAMES[phase])
                for split in SPLITS
                for phase, quota in enumerate(self.quotas[split])
                if len(self.reservoirs[split][phase].items) < quota]


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


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def row_text(row: tuple) -> str:
    fen, result, gid, sid, ply, phase = row
    return f"{fen},{RESULT_TEXT[result]},{gid},{sid},{ply},{phase}\n"


def stage_rows(path: Path, split: str, rows: list[tuple]) -> Path:
    tmp = path.with_name(path.name + ".tmp")
    with tmp.open("w", encoding="utf-8", newline="\n") as out:
        out.write(f"# {SCHEMA} {split}\n")
        out.write(",".join(COLUMNS) + "\n")
        for row in rows:
            out.write(row_text(row))
    return tmp


def source_revision() -> str:
    """The checkout's commit, or 'unknown' outside a git checkout."""
    try:
        done = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True, text=True,
                              cwd=Path(__file__).resolve().parent, check=False)
    except OSError:
        return "unknown"
    return done.stdout.strip() if done.returncode == 0 else "unknown"


def publish(collector: Collector, out_dir: Path, paths: list[Path], params: dict,
            seed: int) -> int:
    """Write all three splits and the manifest, or nothing."""
    targets = {split: out_dir / f"{split}.csv" for split in SPLITS}
    manifest_path = out_dir / "manifest.json"
    for target in (*targets.values(), manifest_path):
        if target.exists():
            print(f"ERROR: refusing to overwrite a published dataset file: {target}",
                  file=sys.stderr)
            return 2
    out_dir.mkdir(parents=True, exist_ok=True)
    shuffle = random.Random(seed)
    staged: list[tuple[Path, Path]] = []
    output = {}
    for split in SPLITS:
        rows = [row for reservoir in collector.reservoirs[split] for row in reservoir.items]
        shuffle.shuffle(rows)
        temporary = stage_rows(targets[split], split, rows)
        staged.append((temporary, targets[split]))
        output[split] = {"file": targets[split].name, "rows": len(rows),
                         "sha256": sha256_file(temporary)}

    manifest = {
        "schema": SCHEMA,
        "columns": list(COLUMNS),
        "label": "the game's result from White's side, exactly 1, 0.5 or 0; "
                 "only games that ended by the rules",
        "split_rule": "SHA-256 of the start position (4 FEN fields), "
                      "test first, then validation",
        "dedup_fields": 5,
        "tool": "tools/texel/extract.py",
        "source_revision": source_revision(),
        "inputs": [{"path": str(path), "bytes": path.stat().st_size,
                    "sha256": sha256_file(path)} for path in paths],
        "seed": seed,
        "parameters": params,
        "games_read": collector.games_read,
        "games_by_split": collector.games_by_split,
        "games_rejected": dict(collector.rejected),
        "positions": {"candidates": collector.candidates,
                      "quiet_rejected": collector.quiet_rejected,
                      "duplicates": collector.duplicates},
        "phase_quotas": {split: dict(zip(BUCKET_NAMES, collector.quotas[split]))
                         for split in SPLITS},
        "output": output,
    }
    manifest_tmp = manifest_path.with_name(manifest_path.name + ".tmp")
    manifest_tmp.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
    for temporary, target in staged:
        os.replace(temporary, target)
    os.replace(manifest_tmp, manifest_path)
    print(f"Published {sum(item['rows'] for item in output.values()):,} rows under {out_dir}")
    return 0


def report(collector: Collector, max_reject_pct: float) -> int:
    """Print the accounting; 2 when the rejected share is past the ceiling."""
    print(f"\nGames read={collector.games_read:,} "
          f"by split={collector.games_by_split} "
          f"candidates={collector.candidates:,} duplicates={collector.duplicates:,} "
          f"quiet_rejected={collector.quiet_rejected:,}")
    for reason in REJECT_REASONS:
        if collector.rejected[reason]:
            print(f"  rejected {reason}: {collector.rejected[reason]:,}")
    share = 100.0 * collector.rejected_games() / max(collector.games_read, 1)
    print(f"Rejected games: {collector.rejected_games():,} ({share:.3f}%, ceiling {max_reject_pct}%)")
    if share > max_reject_pct:
        print("ERROR: the rejected share is past the ceiling; this is not the data "
              "the contract describes. Nothing is published.", file=sys.stderr)
        return 2
    return 0


def report_quotas(collector: Collector) -> None:
    for split in SPLITS:
        for phase, name in enumerate(BUCKET_NAMES):
            reservoir = collector.reservoirs[split][phase]
            print(f"  {split:10}/{name:13}: {len(reservoir.items):,}/"
                  f"{collector.quotas[split][phase]:,} eligible={reservoir.seen:,}")


def add_contract_arguments(parser: argparse.ArgumentParser) -> None:
    """The arguments both extractors share; they define the contract's run."""
    parser.add_argument("--out-dir", required=True, metavar="DIR",
                        help="directory for train.csv, validation.csv, test.csv and manifest.json")
    parser.add_argument("--target-train", default=3_500_000, type=int, metavar="N")
    parser.add_argument("--phase-weights", default=parse_phase_weights("1,1,1,1,1"),
                        type=parse_phase_weights, metavar="A,B,C,D,E")
    parser.add_argument("--validation-pct", default=5.0, type=float, metavar="N")
    parser.add_argument("--test-pct", default=5.0, type=float, metavar="N")
    parser.add_argument("--max-per-phase-per-game", default=8, type=int, metavar="N")
    parser.add_argument("--max-per-game", default=0, type=int, metavar="N")
    parser.add_argument("--skip-start", default=0, type=int, metavar="N")
    parser.add_argument("--skip-end", default=6, type=int, metavar="N")
    parser.add_argument("--seed", default=42, type=int, metavar="N")
    parser.add_argument("--max-reject-pct", default=1.0, type=float, metavar="X",
                        help="fail when more than X%% of the games read are rejected (default 1)")
    parser.add_argument("--no-quiet-filter", dest="quiet_filter", action="store_false")


def check_contract_arguments(args, parser: argparse.ArgumentParser) -> None:
    if args.target_train <= 0:
        parser.error("--target-train must be positive")
    if min(args.validation_pct, args.test_pct) <= 0 or args.validation_pct + args.test_pct >= 100:
        parser.error("--validation-pct and --test-pct must be positive and sum below 100")
    if args.max_per_phase_per_game <= 0:
        parser.error("--max-per-phase-per-game must be positive")
    if args.skip_start < 0 or args.skip_end < 0:
        parser.error("skip counts cannot be negative")
    if not 0.0 <= args.max_reject_pct <= 100.0:
        parser.error("--max-reject-pct must be in [0,100]")


def contract_options(args) -> Options:
    return Options(args.skip_start, args.skip_end, args.max_per_phase_per_game,
                   args.max_per_game, args.quiet_filter, args.validation_pct,
                   args.test_pct, args.seed)


def contract_parameters(args) -> dict:
    return {
        "target_train": args.target_train,
        "phase_weights": args.phase_weights,
        "validation_pct": args.validation_pct,
        "test_pct": args.test_pct,
        "max_per_phase_per_game": args.max_per_phase_per_game,
        "max_per_game": args.max_per_game,
        "skip_start": args.skip_start,
        "skip_end": args.skip_end,
        "quiet_filter": args.quiet_filter,
        "max_reject_pct": args.max_reject_pct,
    }


def finish(collector: Collector, args, paths: list[Path]) -> int:
    """The accounting, the quotas and the publication, shared by both tools."""
    status = report(collector, args.max_reject_pct)
    report_quotas(collector)
    if status:
        return status
    short = collector.short()
    if short:
        print(f"ERROR: phase quotas not met for {short}; nothing is published.", file=sys.stderr)
        return 2
    return publish(collector, Path(args.out_dir).resolve(), paths,
                   contract_parameters(args), args.seed)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("pgn", nargs="+", help="PGN files, globs, or directories")
    add_contract_arguments(parser)
    parser.add_argument("--preflight-games", default=0, type=int, metavar="N",
                        help="estimate required games from the first N games; write nothing")
    parser.add_argument("--preflight-safety", default=1.20, type=float, metavar="X")
    args = parser.parse_args()
    check_contract_arguments(args, parser)

    paths = iter_pgn_paths(args.pgn)
    opts = contract_options(args)
    collector = Collector(args.target_train, args.phase_weights, args.validation_pct,
                          args.test_pct, args.seed)
    print(f"Rows: {collector.counts} | skip_start={args.skip_start}, skip_end={args.skip_end}, "
          f"max/phase/game={args.max_per_phase_per_game}, "
          f"quiet_filter={'on' if args.quiet_filter else 'OFF'}")

    for path in paths:
        print(f"Reading {path} ...")
        with path.open(encoding="utf-8", errors="replace") as pgn_file:
            while not args.preflight_games or collector.games_read < args.preflight_games:
                try:
                    game = chess.pgn.read_game(pgn_file)
                except Exception as exc:  # python-chess raises several types
                    print(f"  WARNING: parse error, game rejected: {exc}", file=sys.stderr)
                    collector.parse_error()
                    continue
                if game is None:
                    break
                if game.errors:
                    collector.parse_error()
                    continue
                collector.add(game_rows(game, opts))
        if args.preflight_games and collector.games_read >= args.preflight_games:
            break

    if args.preflight_games:
        report(collector, args.max_reject_pct)
        print("Preflight estimate (including safety):")
        required = 0
        for split in SPLITS:
            for phase, name in enumerate(BUCKET_NAMES):
                rate = collector.unique_by_split[split][phase] / max(collector.games_read, 1)
                quota = collector.quotas[split][phase]
                estimate = math.ceil(quota / rate * args.preflight_safety) if rate else math.inf
                if estimate != math.inf:
                    required = max(required, int(estimate))
                print(f"  {split:10}/{name:13} rate={rate:7.4f}/game required={estimate:,}")
        if required:
            print(f"Recommended minimum: {required:,} games")
        return 0

    return finish(collector, args, paths)


if __name__ == "__main__":
    raise SystemExit(main())
