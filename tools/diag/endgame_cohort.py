#!/usr/bin/env python3
"""Generate and verify Basilisk's frozen Syzygy endgame cohort (PLAN 6.0.a).

The cohort is a measurement instrument, not training data. White owns the
nominally stronger material and moves first; a paired match must play every
start with both engines assigned to both sides.

Each family targets clean Syzygy wins and rule-compatible draws separately.
Cursed wins (WDL +1) belong to the draw quota because the fifty-move rule makes
them drawn, but their exact verdict remains recorded.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import random
import sys
from pathlib import Path

import chess
import chess.syzygy

SCHEMA = "basilisk-endgame-cohort-v1"
PIECE_OF = {
    "Q": chess.QUEEN,
    "R": chess.ROOK,
    "B": chess.BISHOP,
    "N": chess.KNIGHT,
    "P": chess.PAWN,
}
FAMILIES = [
    "KQ-K", "KR-K", "KBB-K", "KBN-K", "KNN-K",
    "KP-K", "KPP-K", "KBP-K",
    "KR-KP", "KR-KB", "KR-KN", "KQ-KP", "KQ-KR", "KNN-KP",
    "KRP-KR", "KRP-KB", "KBP-KB", "KBP-KN", "KP-KP",
    "KQ-KRP", "KBPP-KB",
]
UNVERIFIABLE_AT_6_MEN = ["KRPP-KRP"]
VERDICT = {
    2: "clean_win",
    1: "cursed_win",
    0: "draw",
    -1: "blessed_loss",
    -2: "loss",
}


def parse_int(value: str) -> int:
    return int(value, 0)


def parse_family(spec: str) -> tuple[tuple[int, ...], tuple[int, ...]]:
    try:
        strong_text, weak_text = spec.split("-")
    except ValueError:
        raise ValueError(f"family must look like KRP-KR, got {spec!r}") from None

    parsed: list[tuple[int, ...]] = []
    for side in (strong_text, weak_text):
        if not side.startswith("K"):
            raise ValueError(f"each side of {spec!r} must start with K")
        pieces: list[int] = []
        for symbol in side[1:]:
            if symbol not in PIECE_OF:
                raise ValueError(f"unknown piece {symbol!r} in {spec!r}")
            pieces.append(PIECE_OF[symbol])
        parsed.append(tuple(pieces))

    if 2 + len(parsed[0]) + len(parsed[1]) > 6:
        raise ValueError(f"{spec!r} exceeds the available 6-man tables")
    return parsed[0], parsed[1]


def stable_family_seed(seed: int, family: str) -> int:
    family_hash = int.from_bytes(
        hashlib.sha256(family.encode("ascii")).digest()[:8], "big"
    )
    return seed ^ family_hash


def random_position(
    rng: random.Random,
    strong: tuple[int, ...],
    weak: tuple[int, ...],
) -> chess.Board | None:
    count = 2 + len(strong) + len(weak)
    squares = rng.sample(range(64), count)
    board = chess.Board(None)
    board.turn = chess.WHITE
    board.set_piece_at(squares[0], chess.Piece(chess.KING, chess.WHITE))
    board.set_piece_at(squares[1], chess.Piece(chess.KING, chess.BLACK))

    index = 2
    for piece, color in (
        [(piece, chess.WHITE) for piece in strong]
        + [(piece, chess.BLACK) for piece in weak]
    ):
        square = squares[index]
        index += 1
        if piece == chess.PAWN and chess.square_rank(square) in (0, 7):
            return None
        board.set_piece_at(square, chess.Piece(piece, color))

    if not board.is_valid() or board.is_check():
        return None
    if board.is_game_over(claim_draw=True):
        return None
    return board


def syzygy_inventory(path: Path) -> dict[str, int | str]:
    entries = sorted(
        (item.name, item.stat().st_size)
        for item in path.iterdir()
        if item.is_file() and item.suffix.lower() in {".rtbw", ".rtbz"}
    )
    rendered = "".join(f"{name}\t{size}\n" for name, size in entries).encode()
    return {
        "file_count": len(entries),
        "total_bytes": sum(size for _, size in entries),
        "name_size_sha256": hashlib.sha256(rendered).hexdigest().upper(),
    }


def epd_line(record: dict) -> str:
    return (
        f'{record["fen"]} ; c0 "id={record["id"]} family={record["family"]} '
        f'wdl={record["theory_wdl"]} verdict={record["theory_verdict"]} '
        f'dtz={record["theory_dtz"]}"'
    )


def verify_manifest(manifest_path: Path, syzygy_path: Path) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != SCHEMA:
        raise ValueError(f"unsupported schema: {manifest.get('schema')!r}")

    records = manifest.get("records")
    if not isinstance(records, list) or not records:
        raise ValueError("manifest has no per-position records")
    fens = [record["fen"] for record in records]
    if len(fens) != len(set(fens)):
        raise ValueError("cohort contains duplicate FENs")
    if manifest.get("position_count") != len(records):
        raise ValueError("position_count does not match records")

    book_path = manifest_path.parent / manifest["book"]
    book_bytes = book_path.read_bytes()
    digest = hashlib.sha256(book_bytes).hexdigest().upper()
    if digest != manifest.get("book_sha256"):
        raise ValueError("EPD SHA-256 does not match the manifest")
    expected_lines = [epd_line(record) for record in records]
    actual_lines = book_path.read_text(encoding="utf-8").splitlines()
    if actual_lines != expected_lines:
        raise ValueError("EPD lines do not match manifest records")

    tb = chess.syzygy.open_tablebase(str(syzygy_path))
    try:
        for record in records:
            board = chess.Board(record["fen"])
            if not board.is_valid() or board.turn != chess.WHITE:
                raise ValueError(f'invalid cohort position {record["id"]}')
            if board.is_game_over(claim_draw=True):
                raise ValueError(f'terminal cohort position {record["id"]}')
            wdl = tb.probe_wdl(board)
            dtz = tb.probe_dtz(board)
            if wdl != record["theory_wdl"] or dtz != record["theory_dtz"]:
                raise ValueError(
                    f'Syzygy mismatch at {record["id"]}: '
                    f'manifest=({record["theory_wdl"]},{record["theory_dtz"]}) '
                    f'probe=({wdl},{dtz})'
                )
    finally:
        tb.close()

    return manifest


def generate(args: argparse.Namespace) -> tuple[Path, Path, dict]:
    families = [family.strip() for family in args.families.split(",") if family.strip()]
    specs: dict[str, tuple[tuple[int, ...], tuple[int, ...]]] = {}
    for family in families:
        if family in specs:
            raise ValueError(f"duplicate family: {family}")
        specs[family] = parse_family(family)

    wanted_wins = round(args.per_family * args.win_share)
    wanted_draws = args.per_family - wanted_wins
    records: list[dict] = []
    summaries: dict[str, dict] = {}
    seen: set[str] = set()

    tb = chess.syzygy.open_tablebase(str(args.syzygy))
    try:
        for family in families:
            strong, weak = specs[family]
            family_seed = stable_family_seed(args.seed, family)
            rng = random.Random(family_seed)
            selected: dict[str, list[dict]] = {"clean_win": [], "rule_draw": []}
            verdict_counts: collections.Counter[str] = collections.Counter()
            attempts = 0
            since_progress = 0

            while (
                attempts < args.max_attempts
                and since_progress < args.stall_attempts
                and (
                    len(selected["clean_win"]) < wanted_wins
                    or len(selected["rule_draw"]) < wanted_draws
                )
            ):
                attempts += 1
                since_progress += 1
                board = random_position(rng, strong, weak)
                if board is None:
                    continue
                fen = board.fen()
                if fen in seen:
                    continue
                try:
                    wdl = tb.probe_wdl(board)
                    dtz = tb.probe_dtz(board)
                except (chess.syzygy.MissingTableError, KeyError, ValueError):
                    continue

                if wdl == 2:
                    bucket = "clean_win"
                    if abs(dtz) < args.min_win_dtz:
                        continue
                elif wdl in (0, 1):
                    bucket = "rule_draw"
                else:
                    continue

                target = wanted_wins if bucket == "clean_win" else wanted_draws
                if len(selected[bucket]) >= target:
                    continue

                record = {
                    "family": family,
                    "family_seed": family_seed,
                    "fen": fen,
                    "theory_wdl": wdl,
                    "theory_verdict": VERDICT[wdl],
                    "theory_dtz": dtz,
                    "quota": bucket,
                }
                selected[bucket].append(record)
                verdict_counts[VERDICT[wdl]] += 1
                seen.add(fen)
                since_progress = 0

            family_records = selected["clean_win"] + selected["rule_draw"]
            for record in family_records:
                record["id"] = f"EG{len(records) + 1:04d}"
                records.append(record)

            summaries[family] = {
                "family_seed": family_seed,
                "requested": {
                    "clean_win": wanted_wins,
                    "rule_draw": wanted_draws,
                },
                "written": {
                    "clean_win": len(selected["clean_win"]),
                    "rule_draw": len(selected["rule_draw"]),
                },
                "verdicts": dict(sorted(verdict_counts.items())),
                "attempts": attempts,
            }
            written = summaries[family]["written"]
            short = (
                written["clean_win"] != wanted_wins
                or written["rule_draw"] != wanted_draws
            )
            marker = "  <-- SHORT" if short else ""
            print(
                f"{family:<10} clean-win {written['clean_win']:>3}/{wanted_wins}  "
                f"rule-draw {written['rule_draw']:>3}/{wanted_draws}  "
                f"({attempts} tries){marker}",
                flush=True,
            )
    finally:
        tb.close()

    lines = [epd_line(record) for record in records]
    rendered = "\n".join(lines) + "\n"
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(rendered, encoding="utf-8", newline="\n")
    digest = hashlib.sha256(rendered.encode("utf-8")).hexdigest().upper()

    manifest = {
        "schema": SCHEMA,
        "book": args.out.name,
        "book_sha256": digest,
        "position_count": len(records),
        "unique_positions": len(seen),
        "seed": args.seed,
        "per_family": args.per_family,
        "target_clean_win_share": args.win_share,
        "min_clean_win_abs_dtz": args.min_win_dtz,
        "syzygy": str(args.syzygy.resolve()),
        "syzygy_inventory": syzygy_inventory(args.syzygy),
        "python_chess": chess.__version__,
        "unverifiable_at_6_men": UNVERIFIABLE_AT_6_MEN,
        "families": summaries,
        "records": records,
    }
    manifest_path = args.out.with_suffix(".manifest.json")
    manifest_path.write_text(
        json.dumps(manifest, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    return args.out, manifest_path, manifest


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--syzygy", required=True, type=Path)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--out", type=Path)
    action.add_argument("--verify", type=Path, metavar="MANIFEST")
    parser.add_argument("--per-family", type=int, default=40)
    parser.add_argument("--win-share", type=float, default=0.6)
    parser.add_argument("--seed", type=parse_int, default=0x4E9A2)
    parser.add_argument("--families", default=",".join(FAMILIES))
    parser.add_argument("--min-win-dtz", type=int, default=2)
    parser.add_argument("--max-attempts", type=int, default=60_000)
    parser.add_argument("--stall-attempts", type=int, default=8_000)
    args = parser.parse_args()

    if not args.syzygy.is_dir():
        parser.error(f"Syzygy path is not a directory: {args.syzygy}")
    if args.per_family <= 0:
        parser.error("--per-family must be positive")
    if not 0.0 <= args.win_share <= 1.0:
        parser.error("--win-share must be between 0 and 1")
    if args.min_win_dtz < 1:
        parser.error("--min-win-dtz must be positive")
    if args.max_attempts <= 0 or args.stall_attempts <= 0:
        parser.error("attempt limits must be positive")

    try:
        if args.verify:
            manifest = verify_manifest(args.verify, args.syzygy)
            print(
                f"verified {manifest['position_count']} unique Syzygy positions; "
                f"EPD SHA-256 {manifest['book_sha256']}"
            )
            return 0

        book, manifest_path, manifest = generate(args)
        verified = verify_manifest(manifest_path, args.syzygy)
    except (ValueError, OSError, chess.syzygy.MissingTableError) as exc:
        parser.error(str(exc))

    print()
    print(
        f"positions : {verified['position_count']} "
        f"({verified['unique_positions']} unique)"
    )
    print(f"book      : {book}  SHA-256 {manifest['book_sha256']}")
    print(f"manifest  : {manifest_path}")
    if any(
        summary["written"] != summary["requested"]
        for summary in manifest["families"].values()
    ):
        print("SHORT families are explicit in the manifest; forced-win material")
        print("classes may have no non-terminal rule-draw subset.")
    print(f"not covered by 6-man tables: {', '.join(UNVERIFIABLE_AT_6_MEN)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
