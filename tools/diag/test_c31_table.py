#!/usr/bin/env python3
"""Tests for c31_table.py: a read's row and rule 5's flags, and the refusals."""

from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
from unittest import mock

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import c210_table  # noqa: E402
import c31_table as table  # noqa: E402
from test_c210_table import DRAW, WIN, four_games, game, write_run  # noqa: E402


class Reads(unittest.TestCase):
    def test_a_read_carries_its_quantity_and_no_flag_at_equal_speed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(c210_table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c31-t40-b", four_games(), [1, 3, 0])
            built, refused = table.build(pathlib.Path(tmp))
        self.assertEqual(refused, [])
        row = built["rows"]["t40-b"]
        self.assertEqual(row["quantity"], "g")
        self.assertEqual(row["wdl"], [1, 3, 0])
        self.assertAlmostEqual(row["speed_ratio"], 1.0)  # the arms alternate colours in four_games
        self.assertFalse(row["arm_faster"])
        self.assertFalse(row["saturated"])  # 2.5 of 4
        self.assertEqual(built["missing"], ["d-t40", "d-t127"])

    def test_an_arm_more_than_five_percent_faster_is_flagged(self) -> None:
        # A searches 1,000 nodes in 10 ms on every move, B 1,060: B is 6% faster.
        a_white = '1. e4 {s=0 d=5 t=10ms h=0ms n=1000} e5 {s=0 d=5 t=10ms h=0ms n=1060} 1/2-1/2'
        b_white = '1. e4 {s=0 d=5 t=10ms h=0ms n=1060} e5 {s=0 d=5 t=10ms h=0ms n=1000} 1/2-1/2'
        games = [game(1, 1, "A", "B", "1/2-1/2", a_white), game(2, 1, "B", "A", "1/2-1/2", b_white),
                 game(3, 2, "A", "B", "1/2-1/2", a_white), game(4, 2, "B", "A", "1/2-1/2", b_white)]
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(c210_table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c31-d-t40", games, [0, 4, 0])
            built, refused = table.build(pathlib.Path(tmp))
        self.assertEqual(refused, [])
        self.assertAlmostEqual(built["rows"]["d-t40"]["speed_ratio"], 1.06)
        self.assertTrue(built["rows"]["d-t40"]["arm_faster"])

    def test_a_saturated_read_is_flagged(self) -> None:
        wins = [game(1, 1, "A", "B", "1-0", WIN), game(2, 1, "B", "A", "0-1", WIN.replace("1-0", "0-1")),
                game(3, 2, "A", "B", "1-0", WIN), game(4, 2, "B", "A", "1/2-1/2", DRAW)]
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(c210_table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c31-d-t127", wins, [3, 1, 0])
            built, refused = table.build(pathlib.Path(tmp))
        self.assertEqual(refused, [])
        self.assertTrue(built["rows"]["d-t127"]["saturated"])  # 3.5 of 4

    def test_a_disagreeing_or_short_read_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            write_run(pathlib.Path(tmp) / "c31-d-t40", four_games(), [1, 3, 0])
            status = table.main(["--results", tmp])
            text = (pathlib.Path(tmp) / "c31-table.md").read_text(encoding="utf-8")
        self.assertEqual(status, 1)
        self.assertIn("not 3000 games", text)


if __name__ == "__main__":
    unittest.main()
