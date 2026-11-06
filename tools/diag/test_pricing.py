#!/usr/bin/env python3
"""Tests of tools/diag/pricing.py on synthetic engine output."""
from __future__ import annotations

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import pricing  # noqa: E402

SEARCH = """\
info depth 9 seldepth 14 multipv 1 score cp 31 nodes 9000 nps 900000 hashfull 1 tbhits 0 time 10 pv e2e4
info depth 10 seldepth 16 multipv 1 score cp 25 lowerbound nodes 11000 nps 900000 hashfull 1 tbhits 0 time 12 pv d2d4
info depth 10 seldepth 18 multipv 1 score cp 30 nodes 15000 nps 937500 hashfull 2 tbhits 0 time 16 pv e2e4 e7e5
info string price kv rule=rfp depth=1 node=cut moves=- decisions=80 probed=10 beats=1 changes=0 probe_nodes=30
info string price kv rule=quiet_futility depth=2 node=all moves=4-6 decisions=40 probed=5 beats=2 changes=1 probe_nodes=12
info string price total every=8 decisions=120 probed=15 probe_nodes=42
bestmove e2e4 ponder e7e5
"""


class ClassTest(unittest.TestCase):
    def test_a_class_decodes_to_its_victim_and_recapture(self):
        self.assertEqual(pricing.move_class(0), ("none", False))
        self.assertEqual(pricing.move_class(1), ("pawn", False))
        self.assertEqual(pricing.move_class(5), ("queen", False))
        self.assertEqual(pricing.move_class(6), ("none", True))
        self.assertEqual(pricing.move_class(7), ("pawn", True))
        self.assertEqual(pricing.move_class(11), ("queen", True))
        for bad in (-1, 12, 15):
            with self.assertRaises(ValueError):
                pricing.move_class(bad)

    def cells(self):
        def cell(decisions, beats, changes=0):
            return {"decisions": decisions, "probed": decisions, "beats": beats,
                    "changes": changes, "probe_nodes": decisions}
        return {("qs_futility", 1, "qs", "1-3"): cell(100, 1),      # a pawn
                ("qs_futility", 11, "qs", "1-3"): cell(10, 6),      # a queen, recaptured
                ("qs_futility", 3, "pv", "4-6"): cell(30, 3),       # a bishop
                ("qs_delta", 0, "qs", "-"): cell(5, 1),             # not a class rule
                ("rfp", 3, "cut", "-"): cell(7, 0)}

    def test_summary_splits_class_rules_and_leaves_others_by_depth(self):
        s = pricing.summarise(self.cells())
        self.assertEqual(s["qs_futility"]["decisions"], 140)
        self.assertEqual(s["qs_futility class recapture"]["decisions"], 10)
        self.assertEqual(s["qs_futility class other"]["decisions"], 130)
        self.assertEqual(s["qs_futility victim queen"]["beats"], 6)
        self.assertEqual(s["qs_futility victim queen recapture"]["beats"], 6)
        self.assertEqual(s["qs_futility victim bishop other"]["decisions"], 30)
        self.assertEqual(s["qs_futility moves 1-3 other"]["decisions"], 100)
        # The class is not a depth: no band is read from it.
        self.assertNotIn("qs_futility depth 1-3", s)
        self.assertEqual(s["rfp depth 1-3"]["decisions"], 7)
        self.assertEqual(s["qs_delta depth 0"]["decisions"], 5)

    def test_recaptures_share_of_a_rule(self):
        s = pricing.summarise(self.cells())
        shares = pricing.class_shares(s, "qs_futility")
        self.assertAlmostEqual(shares["decisions"], 10 / 140)
        self.assertAlmostEqual(shares["beats"], 6 / 10)
        self.assertIsNone(pricing.class_shares(s, "rfp"))

    def test_quiescence_rules_are_reported(self):
        for rule in pricing.CLASS_RULES:
            self.assertIn(rule, pricing.RULE_ORDER)


class ParseTest(unittest.TestCase):
    def test_cells_and_total(self):
        cells, total = pricing.parse_price(SEARCH)
        self.assertEqual(len(cells), 2)
        self.assertEqual(cells[1], {"rule": "quiet_futility", "depth": 2, "node": "all",
                                    "moves": "4-6", "decisions": 40, "probed": 5, "beats": 2,
                                    "changes": 1, "probe_nodes": 12})
        self.assertEqual(total, {"every": 8, "decisions": 120, "probed": 15, "probe_nodes": 42})
        pricing.check_cells(cells, total, 8)

    def test_refusal_is_an_error(self):
        with self.assertRaises(RuntimeError):
            pricing.parse_price("info string price error requires Threads=1 and no DecisionTrace\n")

    def test_invariants_are_checked(self):
        cells, total = pricing.parse_price(SEARCH)
        with self.assertRaises(RuntimeError):
            pricing.check_cells(cells, total, 4)          # another sampling rate
        with self.assertRaises(RuntimeError):
            pricing.check_cells(cells, None, 8)           # no total line
        bad = [dict(cells[0], beats=11)]                   # more beats than probes
        with self.assertRaises(RuntimeError):
            pricing.check_cells(bad, dict(total, decisions=80, probed=10, probe_nodes=30), 8)
        node_change = [dict(cells[0], changes=1)]          # a node rule has no best move
        with self.assertRaises(RuntimeError):
            pricing.check_cells(node_change, dict(total, decisions=80, probed=10,
                                                  probe_nodes=30), 8)
        with self.assertRaises(RuntimeError):              # cells must sum to the total
            pricing.check_cells(cells, dict(total, probed=16), 8)

    def test_iteration_lines_drop_only_the_clock(self):
        lines = pricing.search_lines(SEARCH)
        self.assertEqual(len(lines), 3)
        self.assertNotIn(" time ", lines[2])
        self.assertNotIn(" nps ", lines[2])
        self.assertIn("nodes 15000", lines[2])
        self.assertIn("pv e2e4 e7e5", lines[2])
        self.assertEqual(pricing.last_nodes(SEARCH), 15000)   # bound lines skipped


class SummaryTest(unittest.TestCase):
    def test_groups_and_shares(self):
        cells, _ = pricing.parse_price(SEARCH)
        total = {}
        pricing.add_cells(total, cells)
        pricing.add_cells(total, cells)
        s = pricing.summarise(total)
        self.assertEqual(s["quiet_futility"]["probed"], 10)
        self.assertAlmostEqual(s["quiet_futility"]["beat_share"], 0.4)
        self.assertAlmostEqual(s["quiet_futility"]["change_share"], 0.2)
        self.assertEqual(s["rfp depth 1-3"]["decisions"], 160)
        self.assertEqual(s["quiet_futility moves 4-6"]["beats"], 4)
        self.assertNotIn("rfp moves -", s)

    def test_screen(self):
        cells, _ = pricing.parse_price(SEARCH)
        total = {}
        pricing.add_cells(total, cells)
        s = pricing.summarise(total)
        # rfp: 80 decisions x 10% beats; quiet_futility: 40 x 20% changes,
        # both per 1,000 search nodes.
        self.assertAlmostEqual(pricing.hidden_per_mnode("rfp", s["rfp"], 1000), 8000.0)
        self.assertAlmostEqual(
            pricing.hidden_per_mnode("quiet_futility", s["quiet_futility"], 1000), 8000.0)
        out = pricing.screen(s, 1000)
        self.assertEqual(sorted(out["top_hidden"]), ["quiet_futility", "rfp"])
        self.assertEqual(out["beat_floor_above_5pct"], ["quiet_futility"])

    def test_wilson(self):
        lo, hi = pricing.wilson(0, 100)
        self.assertEqual(lo, 0.0)
        self.assertAlmostEqual(hi, 0.037, places=3)
        lo, hi = pricing.wilson(50, 100)
        self.assertAlmostEqual(lo, 0.404, places=3)
        self.assertAlmostEqual(hi, 0.596, places=3)


if __name__ == "__main__":
    unittest.main()
