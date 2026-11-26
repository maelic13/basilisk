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

    def test_a_tall_sample_reads_the_same_rank(self):
        # The QR step for a tall sample keeps the singular values.
        x = self.rng.normal(size=(2000, 4))
        matrix = np.column_stack([x, x[:, 0] - x[:, 2]])
        report = surface.analyse(matrix, free_rows(5), 1, 1e-6, "synthetic")
        self.assertEqual(report.null, 1)
        self.assertEqual(len(report.relative), 5)


class SupportRows(unittest.TestCase):
    """Support counted on the support rows, apart from the rank sample
    (the C.2.8 registration's Amendment 1)."""

    def setUp(self):
        self.rng = np.random.default_rng(11)
        self.sample = self.rng.normal(size=(300, 3))
        self.sample[:, 2] = 0.0
        self.sample[:5, 2] = 1.0  # 5 rows of 300 in the sample

    def test_run_1s_defect_rare_in_the_sample_but_supported_stays_free(self):
        # Run 1 froze a slot by its count in a 30,000-row sample.
        report = surface.analyse(self.sample, free_rows(3), 10, 1e-6, "c", support=[300, 300, 500],
                                 support_rows=10_000)
        self.assertEqual(report.frozen, [])
        self.assertEqual(report.derived[2].klass, "free")

    def test_frequent_in_the_sample_but_rare_in_the_support_rows_freezes(self):
        sample = self.rng.normal(size=(300, 3))
        report = surface.analyse(sample, free_rows(3), 10, 1e-6, "c", support=[300, 300, 7],
                                 support_rows=10_000)
        self.assertEqual(report.frozen, [2])
        self.assertIn("frozen: 7 rows of support, under 10, on 10000 rows of c",
                      report.derived[2].reason)

    def test_a_free_slot_absent_from_the_sample_is_a_sample_too_small(self):
        sample = self.rng.normal(size=(300, 3))
        sample[:, 1] = 0.0
        report = surface.analyse(sample, free_rows(3), 10, 1e-6, "c", support=[300, 300, 300],
                                 support_rows=10_000)
        self.assertEqual(report.absent, [1])
        self.assertEqual(report.null, 0)  # named apart, not as an identity

    def test_a_duplicated_column_is_still_a_null_direction(self):
        x = self.rng.normal(size=(300, 2))
        sample = np.column_stack([x, x[:, 0]])
        report = surface.analyse(sample, free_rows(3), 10, 1e-6, "c", support=[300, 300, 300],
                                 support_rows=10_000)
        self.assertEqual(report.absent, [])
        self.assertEqual(report.null, 1)


class Guard(unittest.TestCase):
    """A support-frozen slot must not sit in a table whose level is free."""

    def report(self, rows, support):
        sample = np.random.default_rng(3).normal(size=(300, len(rows)))
        return surface.analyse(sample, rows, 10, 1e-6, "c", support=support, support_rows=10_000)

    def test_it_fails_by_name_in_an_unanchored_table_with_a_free_member(self):
        report = self.report(free_rows(4), [300, 300, 300, 5])
        self.assertEqual(report.stranded, [(3, "G")])
        self.assertFalse(report.anchored[3])

    def test_it_passes_in_a_mean_anchored_table(self):
        report = self.report(free_rows(4, "mean:G"), [300, 300, 300, 5])
        self.assertEqual(report.stranded, [])
        self.assertTrue(report.anchored[3])

    def test_it_passes_for_a_tables_only_slot(self):
        rows = [surface.Row(0, "A", 0, 0), surface.Row(1, "B", 0, 0)]
        self.assertEqual(self.report(rows, [300, 5]).stranded, [])

    def test_it_passes_where_every_other_member_is_fixed(self):
        rows = free_rows(3)
        rows[0].fixed("structural zero")
        rows[1].fixed("gauge anchor")
        self.assertEqual(self.report(rows, [300, 300, 5]).stranded, [])

    def test_an_anchor_left_with_one_member_holds_nothing(self):
        # Freezing one of two anchored entries frees the other's level.
        report = self.report(free_rows(2, "mean:G"), [300, 5])
        self.assertEqual(report.stranded, [(1, "G")])


class Orientation(unittest.TestCase):
    """seed_sf16.py's import check: chess facts, not a published table."""

    def tables(self):
        import seed_sf16
        tables = {"mg": [[0] * 64 for _ in range(6)], "eg": [[0] * 64 for _ in range(6)]}
        for sq in range(64):
            rank = sq // 8 + 1
            tables["mg"][5][sq] = 270 - 30 * rank  # a home-rank king
            tables["mg"][1][sq] = 10 * min(rank, 6)  # a knight that likes to advance
            tables["eg"][0][sq] = 4 * rank if 2 <= rank <= 7 else 0  # a pawn's march
        return seed_sf16, tables

    def test_a_home_rank_king_passes(self):
        seed_sf16, tables = self.tables()
        self.assertEqual(seed_sf16.orientation_violations(tables), [])
        seed_sf16.check_orientation(tables, flip=False)

    def test_its_rank_flip_fails_by_name(self):
        seed_sf16, tables = self.tables()
        tables["mg"][5] = seed_sf16.flipped(tables["mg"][5])
        broken = seed_sf16.orientation_violations(tables)
        self.assertEqual(len(broken), 1)
        self.assertIn("king", broken[0])
        with self.assertRaises(SystemExit):
            seed_sf16.check_orientation(tables, flip=False)

    def test_the_flipped_arm_must_break_every_fact(self):
        seed_sf16, tables = self.tables()
        seed_sf16.check_orientation(tables, flip=True)  # all three break once flipped
        # An upside-down pawn table keeps its fact once flipped.
        tables["eg"][0] = seed_sf16.flipped(tables["eg"][0])
        with self.assertRaises(SystemExit):
            seed_sf16.check_orientation(tables, flip=True)


@unittest.skipUnless(os.environ.get("BASILISK_TEXEL"), "set BASILISK_TEXEL to a basilisk-texel binary")
class AgainstTheTuner(unittest.TestCase):
    def test_the_tracked_manifest_matches_the_registry(self):
        import subprocess
        done = subprocess.run([sys.executable, str(HERE / "surface.py"), "check",
                               "--tuner", os.environ["BASILISK_TEXEL"]],
                              capture_output=True, text=True, check=False)
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)

    def test_the_streamed_count_is_the_dense_dumps(self):
        import subprocess
        import tempfile
        tuner = str(Path(os.environ["BASILISK_TEXEL"]).resolve())
        rows = HERE / "test_positions.csv"
        width = len(surface.read_manifest(surface.manifest_for(surface.registry(Path(tuner)))))
        for limit in ([], ["--max-positions", "4"]):
            with self.subTest(limit=limit), tempfile.TemporaryDirectory() as tmp:
                dump, counted = Path(tmp) / "c.f64", Path(tmp) / "s.txt"
                for mode, out in (("--dump-coeffs", dump), ("--count-support", counted)):
                    done = subprocess.run([tuner, mode, str(rows), str(out), *limit],
                                          capture_output=True, text=True, check=False)
                    self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
                matrix = np.fromfile(dump, dtype="<f8").reshape(-1, width)
                used, support = surface.read_support(counted, width)
                self.assertEqual(used, matrix.shape[0])
                self.assertGreater(used, 0)
                self.assertEqual(support, np.count_nonzero(matrix, axis=0).tolist())


if __name__ == "__main__":
    unittest.main()
