#!/usr/bin/env python3
"""Tests of tools/diag/loss_by_cell.py without an engine."""
from __future__ import annotations

import io
import pathlib
import re
import sys
import unittest

import chess.pgn

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import loss_by_cell as lbc  # noqa: E402

GAME = """\
[Event "test"]
[White "Basilisk dev"]
[Black "Other 1.0"]
[Result "0-1"]
[TimeControl "white 3000ms+30ms; black 2000ms+20ms"]

1. e4 {s=20 d=10 t=100ms h=1ms n=1000} e5 {s=-10 d=10 t=50ms h=1ms n=900}
2. Qh5 {s=30 d=11 t=2700ms h=1ms n=2000} Nc6 {s=0 d=9 t=60ms h=1ms n=800} 0-1
"""


class ScoreTest(unittest.TestCase):
    def test_values(self):
        self.assertEqual(lbc.value({"cp": -35}), -35)
        self.assertEqual(lbc.value({"mate": 3}), lbc.MATE - 3)
        self.assertEqual(lbc.value({"mate": -2}), -lbc.MATE + 2)
        self.assertEqual(lbc.value({"mate": 0}), -lbc.MATE)

    def test_loss(self):
        # +30 before, the opponent +870 after: the mover lost 900, counted 300.
        self.assertEqual(lbc.move_loss({"cp": 30}, {"cp": 870}), 900)
        self.assertEqual(lbc.capped(900), lbc.CAP)
        # A move better than the judge expected is no loss.
        self.assertEqual(lbc.capped(lbc.move_loss({"cp": 30}, {"cp": -60})), 0)
        # Mating: the opponent is mated after it.
        self.assertLess(lbc.move_loss({"mate": 1}, {"mate": 0}), 0)

    def test_cells(self):
        self.assertEqual(lbc.position_class({"cp": -50}), "defending")
        self.assertEqual(lbc.position_class({"cp": -49}), "level")
        self.assertEqual(lbc.position_class({"cp": 50}), "ahead")
        self.assertEqual(lbc.position_class({"mate": -4}), "defending")
        self.assertEqual(lbc.clock_band(299, 3000), "<10%")
        self.assertEqual(lbc.clock_band(300, 3000), "10-30%")
        self.assertEqual(lbc.clock_band(900, 3000), "30%+")
        self.assertEqual(lbc.clock_band(None, 3000), "unknown")
        self.assertEqual([lbc.material_band(n) for n in (32, 24, 23, 16, 15, 10, 9)],
                         ["24+", "24+", "16-23", "16-23", "10-15", "10-15", "<=9"])


class GameTest(unittest.TestCase):
    def test_time_controls(self):
        self.assertEqual(lbc.time_control({"TimeControl": "3+0.03"}), (3000, 30, 3000, 30))
        self.assertEqual(lbc.time_control({"TimeControl": "white 3000ms+30ms; black 2000ms+20ms"}),
                         (3000, 30, 2000, 20))
        self.assertIsNone(lbc.time_control({}))

    def test_record_and_clock(self):
        game = chess.pgn.read_game(io.StringIO(GAME))
        rec = lbc.game_record("t", 7, game)
        self.assertEqual(rec["moves"], ["e2e4", "e7e5", "d1h5", "b8c6"])
        self.assertEqual(rec["men"], [32, 32, 32, 32])
        self.assertEqual(rec["meta"][2], {"s": 30, "d": 11, "t": 2700, "n": 2000})
        # White 3000, then 3000 + 30 - 100; Black 2000, then 2000 + 20 - 50.
        self.assertEqual(rec["clock_ms"], [3000, 2000, 2930, 1970])
        self.assertEqual(rec["base_ms"], [3000, 2000])

    def test_no_annotations_no_clock(self):
        text = GAME.replace("{s=20 d=10 t=100ms h=1ms n=1000}", "")
        rec = lbc.game_record("t", 1, chess.pgn.read_game(io.StringIO(text)))
        self.assertEqual(rec["clock_ms"], [None] * 4)

    def test_moves_and_tables(self):
        rec = lbc.game_record("t", 1, chess.pgn.read_game(io.StringIO(GAME)))
        # Judge scores of the five positions, side to move: Qh5 hangs nothing
        # here, but the test plants a 400 cp loss on it.
        rec["judge"] = [{"cp": 30}, {"cp": -25}, {"cp": 20}, {"cp": 380}, {"cp": -370}]
        moves = lbc.moves_of(rec, re.compile("^Basilisk"))
        self.assertEqual([m["side"] for m in moves], ["ours", "opponent", "ours", "opponent"])
        self.assertEqual([m["raw_loss"] for m in moves], [5, -5, 400, 10])
        self.assertEqual(moves[2]["loss"], lbc.CAP)
        self.assertEqual(moves[2]["clock"], "30%+")
        cells = lbc.tabulate(moves)
        ours = cells["ours | all | all | all"]
        self.assertEqual((ours["moves"], ours["loss"], ours["errors"]), (2, 305, 1))
        self.assertAlmostEqual(cells["ours | level | all | all"]["loss_share"], 1.0)
        self.assertEqual(cells["opponent | all | all | all"]["loss"], 10)
        # Level positions: ours 2 moves at 152.5 cp, the opponent's 0 cp
        # (its Nc6 move is in an ahead cell): 305 cp of excess.
        ranked = dict(lbc.excess(cells))
        self.assertAlmostEqual(ranked["level | all | all"], 2 * (152.5 - 0.0))
        self.assertNotIn("all | all | all", ranked)


class ResearchTest(unittest.TestCase):
    def test_selection_order(self):
        lost = {"game_raw_loss": 9000, "before": {"cp": -900}, "index": 1, "ply": 5}
        level = {"game_raw_loss": 400, "before": {"cp": 10}, "index": 2, "ply": 9}
        worse = {"game_raw_loss": 350, "before": {"cp": -200}, "index": 1, "ply": 7}
        small = {"game_raw_loss": 120, "before": {"cp": 80}, "index": 3, "ply": 3}
        self.assertFalse(lbc.is_open(lost["before"]))
        self.assertTrue(lbc.is_open(worse["before"]))
        ranked = sorted([small, worse, level], key=lbc.error_rank)
        self.assertEqual(ranked, [level, worse, small])

    def test_summary(self):
        rows = [{"game_move": "a", "x1": {"move": "a", "raw_loss": 200},
                 "x4": {"move": "b", "raw_loss": 10}, "x16": {"move": "b", "raw_loss": 10}},
                {"game_move": "c", "x1": {"move": "d", "raw_loss": 120},
                 "x4": {"move": "d", "raw_loss": 120}, "x16": {"move": "e", "raw_loss": 90}}]
        s = lbc.summarise_research(rows, [1, 4, 16])
        self.assertEqual(s, {"errors": 2, "still_errors_rejudged": 2,
                             "repeated_at_x1": 1, "mended_at_x1": 0,
                             "mended_at_x4": 1, "mended_at_x16": 1, "persist_at_deepest": 1})


if __name__ == "__main__":
    unittest.main()
