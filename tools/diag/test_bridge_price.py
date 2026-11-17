#!/usr/bin/env python3
"""Tests of tools/diag/bridge_price.py on synthetic bench rows."""
from __future__ import annotations

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import bridge_price  # noqa: E402

SUMMARY = """\
===========================
Total time (ms) : 6534
Nodes searched  : 12605878
Nodes/second    : 1929274
"""


def rows(ms1, ms3, nodes=12_605_878, exits=(0, 0)):
    out = []
    for cycle, (a, b) in enumerate(zip(ms1, ms3)):
        out.append({"cycle": cycle, "repeat": 1, "exit": exits[0], "nodes": nodes, "ms": a})
        out.append({"cycle": cycle, "repeat": 3, "exit": exits[1], "nodes": nodes, "ms": b})
    return out


class ParseTest(unittest.TestCase):
    def test_reads_stockfish_summary(self):
        self.assertEqual(bridge_price.parse_bench(SUMMARY), (12_605_878, 6534.0))

    def test_refuses_output_without_a_summary(self):
        with self.assertRaises(bridge_price.Refused):
            bridge_price.parse_bench("info depth 1\nbestmove e2e4\n")


class SummaryTest(unittest.TestCase):
    def test_reproduces_rarogs_classical_oracle_read(self):
        # RAR-O08's sf-classical medians: 6534 ms at repeat 1, 7641 at 3.
        s = bridge_price.summarise(rows([6534, 6534, 6600], [7641, 7600, 7700]), 3000, 30)
        self.assertAlmostEqual(s["bridge_share_at_1"], 0.0847, places=4)
        self.assertAlmostEqual(s["time_odds"], 1.0926, places=4)
        self.assertEqual((s["oracle_base_ms"], s["oracle_increment_ms"]), (3278, 33))

    def test_median_not_mean(self):
        s = bridge_price.summarise(rows([100, 100, 900], [120, 120, 120]), 3000, 30)
        self.assertEqual(s["median_ms"], {"1": 100, "3": 120})

    def test_changed_tree_is_refused(self):
        bad = rows([100], [120])
        bad[1]["nodes"] += 1
        with self.assertRaisesRegex(bridge_price.Refused, "node counts differ"):
            bridge_price.summarise(bad, 3000, 30)

    def test_dead_wire_is_refused(self):
        with self.assertRaisesRegex(bridge_price.Refused, "dead"):
            bridge_price.summarise(rows([100], [100]), 3000, 30)
        with self.assertRaisesRegex(bridge_price.Refused, "dead"):
            bridge_price.summarise(rows([100], [95]), 3000, 30)

    def test_failed_run_is_refused(self):
        with self.assertRaisesRegex(bridge_price.Refused, "non-zero"):
            bridge_price.summarise(rows([100], [120], exits=(0, 3)), 3000, 30)


if __name__ == "__main__":
    unittest.main()
