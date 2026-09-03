#!/usr/bin/env python3
"""Census self-play labels against Syzygy on every <=6-man corpus row.

The corpus target is a White-perspective game result: 0, 0.5 or 1. Syzygy
WDL is first converted from side-to-move to White's point of view, then clean
wins/losses map to 1/0 and cursed wins, blessed losses and draws map to 0.5.

This is deliberately a census, not a relabeler. Syzygy WDL assumes a freshly
zeroed fifty-move clock. A later relabel experiment must separately decide how
to treat the corpus halfmove clock and may not propagate an ending verdict to
earlier, non-tablebase positions.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import Counter, defaultdict
from decimal import Decimal, InvalidOperation
from pathlib import Path
from typing import Any

import chess
import chess.syzygy


SCHEMA = "basilisk-endgame-label-census-v1"
LABELS = ("0", "0.5", "1")
PIECE_ORDER = (
    (chess.QUEEN, "Q"),
    (chess.ROOK, "R"),
    (chess.BISHOP, "B"),
    (chess.KNIGHT, "N"),
    (chess.PAWN, "P"),
)


def parse_label(text: str) -> str:
    try:
        value = Decimal(text.strip())
    except InvalidOperation as exc:
        raise ValueError(f"invalid label {text!r}") from exc
    if value == 0:
        return "0"
    if value == Decimal("0.5"):
        return "0.5"
    if value == 1:
        return "1"
    raise ValueError(f"label must be exactly 0, 0.5 or 1, got {text!r}")


def label_from_wdl(wdl_white: int) -> str:
    if wdl_white == 2:
        return "1"
    if wdl_white == -2:
        return "0"
    if wdl_white in (-1, 0, 1):
        return "0.5"
    raise ValueError(f"invalid Syzygy WDL {wdl_white}")


def material_signature(board: chess.Board) -> str:
    def side(color: chess.Color) -> str:
        return "K" + "".join(
            symbol * len(board.pieces(piece_type, color))
            for piece_type, symbol in PIECE_ORDER
        )

    return f"{side(chess.WHITE)}-{side(chess.BLACK)}"


def empty_matrix() -> dict[str, dict[str, int]]:
    return {source: {target: 0 for target in LABELS} for source in LABELS}


def summary_bucket() -> dict[str, Any]:
    return {
        "rows": 0,
        "self_play_labels": Counter(),
        "syzygy_wdl_white": Counter(),
        "syzygy_labels": Counter(),
        "disagreements": 0,
        "matrix": empty_matrix(),
    }


def add_eligible(bucket: dict[str, Any], label: str, wdl: int, truth: str) -> None:
    bucket["rows"] += 1
    bucket["self_play_labels"][label] += 1
    bucket["syzygy_wdl_white"][str(wdl)] += 1
    bucket["syzygy_labels"][truth] += 1
    bucket["matrix"][label][truth] += 1
    bucket["disagreements"] += label != truth


def finalize_bucket(bucket: dict[str, Any]) -> dict[str, Any]:
    rows = bucket["rows"]
    return {
        "rows": rows,
        "self_play_labels": {key: bucket["self_play_labels"][key] for key in LABELS},
        "syzygy_wdl_white": {
            key: bucket["syzygy_wdl_white"][key]
            for key in ("-2", "-1", "0", "1", "2")
        },
        "syzygy_labels": {key: bucket["syzygy_labels"][key] for key in LABELS},
        "disagreements": bucket["disagreements"],
        "disagreement_rate": bucket["disagreements"] / rows if rows else 0.0,
        "matrix_self_play_to_syzygy": bucket["matrix"],
    }


def display_path(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return str(path.resolve())


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


def manifest_fields(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if ":" in line:
            key, value = line.split(":", 1)
            fields[key.strip()] = value.strip()
    if fields.get("status") != "complete":
        raise ValueError(f"parent PGN manifest is not complete: {path}")
    if not fields.get("adjudication", "").upper().startswith("NONE"):
        raise ValueError(f"parent PGN was not generated without adjudication: {path}")
    required = (
        "output_pgn", "pgn_sha256", "revision", "engine_sha256",
        "engine_manifest_sha256", "label_engine_id", "nodes_per_move",
        "games_total", "book_sha256", "opening_seed", "fastchess",
    )
    missing = [key for key in required if key not in fields]
    if missing:
        raise ValueError(f"parent PGN manifest lacks {', '.join(missing)}: {path}")
    return fields


def census(
    sources: list[Path],
    tablebase: chess.syzygy.Tablebase,
    *,
    root: Path,
    max_pieces: int = 6,
    example_limit: int = 24,
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    overall = summary_bucket()
    per_piece: dict[int, dict[str, Any]] = defaultdict(summary_bucket)
    per_material: dict[str, dict[str, Any]] = defaultdict(summary_bucket)
    examples: list[dict[str, Any]] = []
    source_reports: list[dict[str, Any]] = []
    total_rows = 0
    piece_counts: Counter[int] = Counter()

    for source in sources:
        digest = hashlib.sha256()
        source_bucket = summary_bucket()
        source_rows = 0
        with source.open("rb") as stream:
            for line_number, raw in enumerate(stream, 1):
                digest.update(raw)
                source_rows += 1
                total_rows += 1
                try:
                    text = raw.decode("utf-8").rstrip("\r\n")
                    fen, raw_label = text.rsplit(";", 1)
                    label = parse_label(raw_label)
                    board = chess.Board(fen)
                except (UnicodeDecodeError, ValueError) as exc:
                    raise ValueError(
                        f"{source}:{line_number}: malformed corpus row: {exc}"
                    ) from exc
                if not board.is_valid():
                    raise ValueError(f"{source}:{line_number}: invalid position: {fen}")

                pieces = chess.popcount(board.occupied)
                piece_counts[pieces] += 1
                if pieces > max_pieces:
                    continue
                try:
                    wdl_stm = tablebase.probe_wdl(board)
                except (KeyError, ValueError) as exc:
                    raise ValueError(
                        f"{source}:{line_number}: cannot probe {pieces}-man FEN: {fen}"
                    ) from exc
                wdl_white = wdl_stm if board.turn == chess.WHITE else -wdl_stm
                truth = label_from_wdl(wdl_white)
                family = material_signature(board)
                for bucket in (overall, source_bucket, per_piece[pieces], per_material[family]):
                    add_eligible(bucket, label, wdl_white, truth)
                if label != truth and len(examples) < example_limit:
                    examples.append(
                        {
                            "source": display_path(source, root),
                            "line": line_number,
                            "fen": fen,
                            "pieces": pieces,
                            "material": family,
                            "self_play_label": label,
                            "syzygy_wdl_white": wdl_white,
                            "syzygy_label": truth,
                        }
                    )

        source_reports.append(
            {
                "path": display_path(source, root),
                "bytes": source.stat().st_size,
                "sha256": digest.hexdigest().upper(),
                "rows": source_rows,
                "eligible": finalize_bucket(source_bucket),
            }
        )

    report = {
        "total_rows": total_rows,
        "piece_counts_all_rows": {
            str(key): piece_counts[key] for key in sorted(piece_counts)
        },
        "eligible": finalize_bucket(overall),
        "by_source": source_reports,
        "by_piece_count": {
            str(key): finalize_bucket(per_piece[key]) for key in sorted(per_piece)
        },
        "by_material": {
            key: finalize_bucket(per_material[key])
            for key in sorted(per_material)
        },
    }
    return report, examples


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sources", nargs="+", type=Path, help="FEN;label corpus CSVs")
    parser.add_argument("--syzygy", required=True, type=Path)
    parser.add_argument("--pgn-manifest", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--max-pieces", default=6, type=int)
    parser.add_argument("--examples", default=24, type=int)
    args = parser.parse_args()

    if args.max_pieces != 6:
        parser.error("this frozen contract requires --max-pieces 6")
    for path in (*args.sources, args.syzygy, args.pgn_manifest):
        if not path.exists():
            parser.error(f"not found: {path}")
    if args.examples < 0:
        parser.error("--examples cannot be negative")

    try:
        parent = manifest_fields(args.pgn_manifest)
        corpus = Path(parent["output_pgn"]).stem
        expected_sources = {f"{corpus}_train.csv", f"{corpus}_holdout.csv"}
        actual_sources = {path.name for path in args.sources}
        if actual_sources != expected_sources or len(args.sources) != 2:
            raise ValueError(
                "sources must be exactly the train and holdout CSVs belonging "
                f"to {corpus}: {sorted(expected_sources)}"
            )
        with chess.syzygy.open_tablebase(str(args.syzygy)) as tablebase:
            measured, examples = census(
                args.sources,
                tablebase,
                root=root,
                max_pieces=args.max_pieces,
                example_limit=args.examples,
            )
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    manifest_hash = hashlib.sha256(args.pgn_manifest.read_bytes()).hexdigest().upper()
    artifact = {
        "schema": SCHEMA,
        "contract": {
            "corpus": corpus,
            "row_format": "FEN;white-perspective self-play game result",
            "legal_labels": list(LABELS),
            "domain": "every corpus row with at most 6 pieces",
            "syzygy_perspective": "converted from side-to-move to White",
            "syzygy_label_mapping": {
                "-2": "0",
                "-1": "0.5",
                "0": "0.5",
                "1": "0.5",
                "2": "1",
            },
            "rule_50_limit": (
                "WDL assumes a freshly zeroed clock; this census is not permission "
                "to relabel rows without the separate Phase 7.4 clock/domain audit"
            ),
            "duplicates": "rows are counted as stored; no census-time deduplication",
        },
        "parent_self_play": {
            "manifest": display_path(args.pgn_manifest, root),
            "manifest_sha256": manifest_hash,
            "pgn_sha256": parent["pgn_sha256"],
            "revision": parent["revision"],
            "label_engine_id": parent["label_engine_id"],
            "engine_sha256": parent["engine_sha256"],
            "engine_manifest_sha256": parent["engine_manifest_sha256"],
            "nodes_per_move": int(parent["nodes_per_move"]),
            "games_total": int(parent["games_total"]),
            "adjudication": parent["adjudication"],
            "book_sha256": parent["book_sha256"],
            "opening_seed": int(parent["opening_seed"]),
            "fastchess": parent["fastchess"],
        },
        "extraction": {
            "launcher": "tools/run_5911_experiment.ps1",
            "tool": "tools/texel/extract_parallel.py",
            "command_contract": (
                "python extract_parallel.py <PGN> --out-dir data "
                f"--train {corpus}_train.csv --holdout {corpus}_holdout.csv "
                "--target-train 1000000 --holdout-pct 5"
            ),
            "split": "stable SHA-256(start FEN + movetext), game-level 5% holdout",
            "extractor_defaults": {
                "phase_weights": [1, 1, 1, 1, 1],
                "max_per_phase_per_game": 8,
                "max_per_game": 0,
                "skip_start": 0,
                "skip_end": 6,
                "seed": 42,
                "quiet_filter": True,
            },
        },
        "tablebase": {
            "path": str(args.syzygy.resolve()),
            "max_pieces": args.max_pieces,
            "python_chess_version": chess.__version__,
            "inventory": syzygy_inventory(args.syzygy),
        },
        **measured,
        "disagreement_examples": examples,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(artifact, indent=2) + "\n", encoding="utf-8")
    eligible = artifact["eligible"]
    print(f"Rows scanned: {artifact['total_rows']:,}")
    print(f"<=6-man rows: {eligible['rows']:,}")
    print(
        f"Disagreements: {eligible['disagreements']:,} "
        f"({eligible['disagreement_rate']:.2%})"
    )
    print(f"Report: {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
