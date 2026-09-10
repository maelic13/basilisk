#!/usr/bin/env python3
"""Unit tests for the PLAN 6.0.f corpus-label census."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import chess

from endgame_label_census import (
    census,
    label_from_wdl,
    material_signature,
    parse_label,
)


class FakeTablebase:
    def probe_wdl(self, board: chess.Board) -> int:
        # Side-to-move result. The census must invert this for Black to move.
        return 2 if board.turn == chess.WHITE else -2


class LabelContractTests(unittest.TestCase):
    def test_only_exact_wdl_labels_are_accepted(self) -> None:
        for text, expected in (("0", "0"), ("0.0", "0"), (".5", "0.5"), ("1", "1")):
            with self.subTest(text=text):
                self.assertEqual(parse_label(text), expected)
        for text in ("-1", "0.25", "2", "nan", "win"):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    parse_label(text)

    def test_cursed_and_blessed_results_collapse_to_draw(self) -> None:
        self.assertEqual(
            [label_from_wdl(wdl) for wdl in (-2, -1, 0, 1, 2)],
            ["0", "0.5", "0.5", "0.5", "1"],
        )

    def test_material_signature_preserves_white_black_orientation(self) -> None:
        board = chess.Board("8/8/8/8/8/8/1k6/KQ5r w - - 0 1")
        self.assertEqual(material_signature(board), "KQ-KR")


class CensusTests(unittest.TestCase):
    def test_scans_every_row_and_inverts_black_to_move_wdl(self) -> None:
        rows = (
            "8/8/8/8/8/2k5/8/KQ6 w - - 0 1;1\n"
            "8/8/8/8/8/2k5/8/KQ6 b - - 0 1;0.5\n"
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1;0.5\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "tiny.csv"
            source.write_text(rows, encoding="utf-8")
            report, examples = census(
                [source], FakeTablebase(), root=Path(directory), example_limit=4
            )

        self.assertEqual(report["total_rows"], 3)
        self.assertEqual(report["eligible"]["rows"], 2)
        self.assertEqual(report["eligible"]["disagreements"], 1)
        self.assertEqual(report["eligible"]["syzygy_labels"], {"0": 0, "0.5": 0, "1": 2})
        self.assertEqual(report["piece_counts_all_rows"], {"3": 2, "32": 1})
        self.assertEqual(examples[0]["self_play_label"], "0.5")
        self.assertEqual(examples[0]["syzygy_label"], "1")

    def test_invalid_position_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "bad.csv"
            source.write_text("8/8/8/8/8/8/8/K7 w - - 0 1;0.5\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "invalid position"):
                census([source], FakeTablebase(), root=Path(directory))


if __name__ == "__main__":
    unittest.main()
