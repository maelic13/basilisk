#!/usr/bin/env python3
"""Freeze or verify the exact 198 Syzygy wins from the BAS-E35 KBNK census."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import chess
import chess.syzygy

SEED = 0x5E9D18
MASK64 = (1 << 64) - 1


class Lcg:
    def __init__(self, seed: int) -> None:
        self.state = seed

    def below(self, limit: int) -> int:
        self.state = (
            self.state * 6364136223846793005 + 1442695040888963407
        ) & MASK64
        return (self.state >> 33) % limit


def random_kbnk(rng: Lcg) -> str | None:
    """Preserve kbnk_outcomes.py's generator byte-for-byte in intent."""
    for _ in range(400):
        squares = [rng.below(64) for _ in range(4)]
        if len(set(squares)) != 4:
            continue
        board = chess.Board.empty()
        board.turn = chess.WHITE
        board.set_piece_at(squares[0], chess.Piece(chess.KING, chess.WHITE))
        board.set_piece_at(squares[1], chess.Piece(chess.BISHOP, chess.WHITE))
        board.set_piece_at(squares[2], chess.Piece(chess.KNIGHT, chess.WHITE))
        board.set_piece_at(squares[3], chess.Piece(chess.KING, chess.BLACK))
        if not board.is_valid() or board.is_check() or not any(board.legal_moves):
            continue
        return board.fen()
    return None


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def inventory(path: Path) -> dict:
    files = sorted(
        (item.name, item.stat().st_size)
        for item in path.iterdir()
        if item.is_file() and item.suffix.lower() in {".rtbw", ".rtbz"}
    )
    rendered = "".join(f"{name}\t{size}\n" for name, size in files).encode()
    return {
        "file_count": len(files),
        "total_bytes": sum(size for _, size in files),
        "name_size_sha256": sha256(rendered),
    }


def render(syzygy: Path, epd_name: str) -> tuple[bytes, bytes]:
    rng = Lcg(SEED)
    generated = []
    while len(generated) < 200:
        fen = random_kbnk(rng)
        if fen is None:
            raise RuntimeError("legacy generator stalled before 200 positions")
        generated.append(fen)
    if len(set(generated)) != len(generated):
        raise RuntimeError("legacy 200-position source unexpectedly contains duplicates")

    records = []
    exclusions = []
    with chess.syzygy.open_tablebase(str(syzygy)) as tb:
        for source_index, fen in enumerate(generated, 1):
            board = chess.Board(fen)
            wdl = tb.probe_wdl(board)
            dtz = tb.probe_dtz(board)
            if wdl != 2:
                exclusions.append({"source_index": source_index, "fen": fen, "wdl": wdl, "dtz": dtz})
                continue
            records.append({
                "id": f"KBNK{len(records) + 1:04d}",
                "family": "KBN-K",
                "family_seed": SEED,
                "source_index": source_index,
                "fen": fen,
                "theory_wdl": wdl,
                "theory_verdict": "clean_win",
                "theory_dtz": dtz,
                "quota": "clean_win",
            })

    if len(records) != 198:
        raise RuntimeError(f"expected 198 clean wins from the legacy 200, got {len(records)}")
    epd = ("\n".join(
        f'{r["fen"]} ; c0 "id={r["id"]} family=KBN-K wdl=2 '
        f'verdict=clean_win dtz={r["theory_dtz"]} source={r["source_index"]}"'
        for r in records
    ) + "\n").encode()
    manifest = {
        "schema": "basilisk-endgame-cohort-v1",
        "purpose": "Exact Syzygy-clean-win subset from the historical BAS-E35 KBNK census",
        "generator": "tools/diag/kbnk_cohort.py",
        "legacy_generator": "tools/diag/kbnk_outcomes.py",
        "seed": SEED,
        "generated_position_count": len(generated),
        "excluded_position_count": len(exclusions),
        "exclusions": exclusions,
        "position_count": len(records),
        "unique_positions": len({r["fen"] for r in records}),
        "book": epd_name,
        "book_sha256": sha256(epd),
        "python_chess_version": chess.__version__,
        "syzygy_inventory": inventory(syzygy),
        "records": records,
    }
    return epd, (json.dumps(manifest, indent=2) + "\n").encode()


def main() -> int:
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--syzygy", type=Path, default=Path("D:/chess/tablebases/syzygy3456"))
    parser.add_argument("--epd", type=Path, default=here / "kbnk_cohort_v1.epd")
    parser.add_argument("--manifest", type=Path, default=here / "kbnk_cohort_v1.manifest.json")
    parser.add_argument("--write", action="store_true", help="create missing frozen artifacts")
    args = parser.parse_args()
    if not args.syzygy.is_dir():
        parser.error(f"Syzygy directory not found: {args.syzygy}")
    epd, manifest = render(args.syzygy.resolve(), args.epd.name)
    expected = ((args.epd, epd), (args.manifest, manifest))
    missing = [path for path, _ in expected if not path.is_file()]
    if missing and not args.write:
        parser.error("frozen artifacts are missing; pass --write to create them")
    for path, content in expected:
        if path.is_file():
            if path.read_bytes() != content:
                raise RuntimeError(f"frozen artifact drift: {path}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
            print(f"Wrote {path.resolve()}")
    print("Verified exact legacy KBNK cohort: 198 clean wins from 200 positions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
