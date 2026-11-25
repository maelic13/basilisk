#!/usr/bin/env python3
"""Tests for c34_table.py: a read's row, the speed flag, and the refusals."""

from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
from unittest import mock

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import c210_table  # noqa: E402
import c34_table as table  # noqa: E402
from test_c210_table import four_games, game, write_run  # noqa: E402


class Reads(unittest.TestCase):
    def test_a_read_carries_its_entry_and_no_flag_at_equal_speed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(c210_table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c34-m25", four_games(), [1, 3, 0])
            built, refused = table.build(pathlib.Path(tmp))
        self.assertEqual(refused, [])
        row = built["rows"]["m25"]
        self.assertEqual(row["entry"], "BAS-M25")
        self.assertEqual(row["wdl"], [1, 3, 0])
        self.assertAlmostEqual(row["speed_ratio"], 1.0)  # the arms alternate colours in four_games
        self.assertFalse(row["arm_faster"])
        self.assertEqual(built["missing"], ["o16"])

    def test_an_arm_more_than_five_percent_faster_is_flagged(self) -> None:
        # A searches 1,000 nodes in 10 ms on every move, B 1,060: B is 6% faster.
        a_white = '1. e4 {s=0 d=5 t=10ms h=0ms n=1000} e5 {s=0 d=5 t=10ms h=0ms n=1060} 1/2-1/2'
        b_white = '1. e4 {s=0 d=5 t=10ms h=0ms n=1060} e5 {s=0 d=5 t=10ms h=0ms n=1000} 1/2-1/2'
        games = [game(1, 1, "A", "B", "1/2-1/2", a_white), game(2, 1, "B", "A", "1/2-1/2", b_white),
                 game(3, 2, "A", "B", "1/2-1/2", a_white), game(4, 2, "B", "A", "1/2-1/2", b_white)]
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(c210_table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c34-o16", games, [0, 4, 0])
            built, refused = table.build(pathlib.Path(tmp))
            text = table.markdown(built)
        self.assertEqual(refused, [])
        self.assertAlmostEqual(built["rows"]["o16"]["speed_ratio"], 1.06)
        self.assertTrue(built["rows"]["o16"]["arm_faster"])
        self.assertIn("an arm >5% faster", text)

    def test_a_short_read_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            write_run(pathlib.Path(tmp) / "c34-m25", four_games(), [1, 3, 0])
            status = table.main(["--results", tmp])
            text = (pathlib.Path(tmp) / "c34-table.md").read_text(encoding="utf-8")
        self.assertEqual(status, 1)
        self.assertIn("not 3000 games", text)


if __name__ == "__main__":
    unittest.main()
