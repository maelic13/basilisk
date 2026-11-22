#!/usr/bin/env python3
"""Tests of the fit's surface manifest (surface.py). Needs numpy.

    python tools/texel/test_surface.py

The tracked manifest is also checked against the tuner's registry and the
rules when BASILISK_TEXEL names a basilisk-texel binary.
"""

from __future__ import annotations

import os
import sys
import unittest
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import surface  # noqa: E402


def free_rows(n: int, anchor: str = "") -> list[surface.Row]:
    return [surface.Row(i, "G", i, 0, anchor=anchor) for i in range(n)]


class TrackedManifest(unittest.TestCase):
    def setUp(self):
        self.rows = surface.read_manifest(surface.DEFAULT_MANIFEST)
        self.by_name = {(row.group, row.sub): row for row in self.rows}

    def test_it_holds_every_invariant(self):
        self.assertEqual(surface.invariants(self.rows), [])

    def test_it_matches_the_rules(self):
        rebuilt = surface.classify([surface.Row(r.index, r.group, r.sub, r.seed) for r in self.rows])
        self.assertEqual([r.cells() for r in rebuilt], [r.cells() for r in self.rows])

    def test_the_named_decisions(self):
        cases = {
            ("MgVal", 3): ("fixed", ""), ("PstMgPawn", 0): ("fixed", ""),
            ("PstMgKnight", 27): ("free", "linear"), ("PstMgKing", 4): ("free", "linear"),
            ("ShelterClose1", 0): ("free", "linear"), ("SafetyTable", 0): ("fixed", ""),
            ("SafetyTable", 1): ("free", "kingsafety"), ("KsNoqueenDen", 0): ("fixed", ""),
            ("KsNoqueenNum", 0): ("free", "kingsafety"), ("Rook7thMg", 0): ("fixed", ""),
            ("PassedMg", 6): ("fixed", ""), ("ImbTheir", 16): ("fixed", ""),
            ("ImbOur", 6): ("fixed", ""), ("ImbOur", 5): ("free", "linear"),
            ("KsUnit", 1): ("fixed", ""), ("KsUnit", 2): ("free", "kingsafety"),
        }
        for (group, sub), (klass, stage) in cases.items():
            with self.subTest(slot=f"{group}[{sub}]"):
                row = self.by_name[group, sub]
                self.assertEqual((row.klass, row.stage), (klass, stage), row.reason)
        self.assertEqual(self.by_name["PstEgKing", 10].anchor, "mean:PstEgKing")
        self.assertEqual(self.by_name["MobRMg", 3].anchor, "mean:MobRMg")

    def test_king_safety_knobs_carry_their_own_step(self):
        steps = {row.group: int(row.step) for row in self.rows if row.stage == "kingsafety"}
        self.assertEqual(steps["SafetyTable"], 60)
        self.assertEqual(steps["KsNoqueenNum"], 1)


class Invariants(unittest.TestCase):
    def test_each_break_is_named(self):
        rows = free_rows(3)
        for row in rows:
            row.reason = "r"
        self.assertEqual(surface.invariants(rows), [])
        cases = []
        broken = free_rows(3); broken[0].stage = ""
        cases.append((broken, "needs a stage"))
        broken = free_rows(3, "mean:G"); broken[1].anchor, broken[2].anchor = "", ""
        cases.append((broken, "two or more"))
        broken = free_rows(3); broken[0].fixed("dead: never"); broken[0].seed = 5
        cases.append((broken, "a dead slot holds 5"))
        broken = free_rows(3); broken[0].stage = "kingsafety"
        broken[0].lo, broken[0].hi, broken[0].step, broken[0].seed = "0", "10", "1", 11
        cases.append((broken, "outside"))
        for rows, words in cases:
            for row in rows:
                row.reason = row.reason or "r"
            with self.subTest(words=words):
                self.assertTrue(any(words in e for e in surface.invariants(rows)),
                                surface.invariants(rows))


class Rank(unittest.TestCase):
    """The rank analysis on matrices whose null directions are known."""

    def setUp(self):
        self.rng = np.random.default_rng(7)

    def test_a_duplicated_column_is_a_null_direction(self):
        x = self.rng.normal(size=(400, 3))
        matrix = np.column_stack([x, x[:, 0] + x[:, 1]])
        report = surface.analyse(matrix, free_rows(4), 1, 1e-6, "synthetic")
        self.assertEqual(report.null, 1)
        weights = report.directions[0][1]
        largest = max(abs(weight) for _, weight in weights)
        names = {name for name, weight in weights if abs(weight) > 0.1 * largest}
        self.assertEqual(names, {"G[0]", "G[1]", "G[3]"})
        fixed = free_rows(4)
        fixed[3].fixed("collinear")
        self.assertEqual(surface.analyse(matrix, fixed, 1, 1e-6, "synthetic").null, 0)

    def test_a_mean_anchor_removes_a_level_the_data_cannot_see(self):
        # Like a king PST: each row reads one entry for White, +1, and one for
        # Black, -1, so moving every entry alike changes no row.
        matrix = np.zeros((500, 6))
        for r in range(500):
            white, black = self.rng.integers(0, 6, size=2)
            matrix[r, white] += 1.0
            matrix[r, black] -= 1.0
        self.assertEqual(surface.analyse(matrix, free_rows(6), 1, 1e-6, "synthetic").null, 1)
        self.assertEqual(surface.analyse(matrix, free_rows(6, "mean:G"), 1, 1e-6, "synthetic").null, 0)

    def test_support_freezing(self):
        matrix = self.rng.normal(size=(300, 3))
        matrix[:, 2] = 0.0
        matrix[:5, 2] = 1.0
        report = surface.analyse(matrix, free_rows(3), 10, 1e-6, "synthetic")
        self.assertEqual(report.frozen, [2])
        self.assertEqual(report.derived[2].klass, "fixed")
        self.assertIn("frozen: 5 rows", report.derived[2].reason)
        self.assertEqual(report.null, 0)

    def test_a_frozen_member_leaves_its_anchor(self):
        matrix = self.rng.normal(size=(300, 2))
        matrix[:, 1] = 0.0
        report = surface.analyse(matrix, free_rows(2, "mean:G"), 10, 1e-6, "synthetic")
        self.assertEqual(report.derived[0].anchor, "")  # a set of one is no anchor
        self.assertEqual(report.null, 0)


class Orientation(unittest.TestCase):
    def test_pesto_fixture_reads_whole(self):
        tables = surface.pesto_tables()
        self.assertEqual(len(tables), 12)
        self.assertEqual(tables[1][0], -167)  # mg knight a8, as published


@unittest.skipUnless(os.environ.get("BASILISK_TEXEL"), "set BASILISK_TEXEL to a basilisk-texel binary")
class AgainstTheTuner(unittest.TestCase):
    def test_the_tracked_manifest_matches_the_registry(self):
        import subprocess
        done = subprocess.run([sys.executable, str(HERE / "surface.py"), "check",
                               "--tuner", os.environ["BASILISK_TEXEL"]],
                              capture_output=True, text=True, check=False)
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)


if __name__ == "__main__":
    unittest.main()
