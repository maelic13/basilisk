#!/usr/bin/env python3
"""Tests for family_screen.py: the joins it refuses and the signal it finds."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import family_screen as fs  # noqa: E402

TUNER = str(Path(os.environ["BASILISK_TEXEL"]).resolve()) if os.environ.get("BASILISK_TEXEL") else ""

SF16_TRACE = """
|   Material |  ----  ---- |  ----  ---- |  3.87  4.23 |
|  Imbalance |  ----  ---- |  ----  ---- | -0.05 -0.04 |
|      Pawns |  0.09  0.00 |  0.09  0.00 |  0.00  0.00 |
|      Rooks |  0.15  0.08 |  0.00  0.00 |  0.15  0.08 |
|   Mobility |  0.20  0.53 |  0.00  0.00 |  0.20  0.53 |
|King safety |  0.14 -0.07 |  0.04 -0.07 |  0.10  0.00 |
|   Winnable |  ----  ---- |  ----  ---- | -0.30 -1.95 |
|      Total |  ----  ---- |  ----  ---- |  3.97  2.85 |
Classical evaluation   +2.83 (white side)
Final evaluation       +2.64 (white side)
"""


class Families(unittest.TestCase):
    def test_every_tracked_slot_group_has_a_family(self) -> None:
        manifest = HERE.parents[0] / "texel" / "surface_v1.tsv"
        groups = {line.split("\t")[1] for line in manifest.read_text(encoding="utf-8").splitlines()
                  if line and line[0].isdigit()}
        self.assertTrue(groups)
        self.assertEqual(sorted(g for g in groups if fs.family_of(g) is None), [])

    def test_the_prefixes_do_not_cross(self) -> None:
        self.assertEqual(fs.family_of("PassedMg"), "passed")
        self.assertEqual(fs.family_of("PawnIslandsEg"), "pawns")
        self.assertEqual(fs.family_of("RookKingRingMg"), "king")
        self.assertEqual(fs.family_of("RookOpenMg"), "pieces")
        self.assertEqual(fs.family_of("ThreatByRookMg"), "threats")
        self.assertEqual(fs.family_of("SpaceBlockedMg"), "space")


class Donor(unittest.TestCase):
    def test_a_trace_parses_with_its_final(self) -> None:
        terms, final = fs.parse_donor(SF16_TRACE)
        self.assertEqual(terms["Material"], (3.87, 4.23))
        self.assertEqual(terms["King safety"], (0.10, 0.00))
        self.assertEqual(final, 2.64)
        fs.check_donor_sum(terms)

    def test_a_table_that_does_not_sum_is_refused(self) -> None:
        terms, _ = fs.parse_donor(SF16_TRACE.replace("|  3.97  2.85 |", "|  4.97  2.85 |"))
        with self.assertRaises(fs.Refused):
            fs.check_donor_sum(terms)


class Join(unittest.TestCase):
    def sample(self, directory: Path) -> Path:
        path = directory / "sample.csv"
        path.write_text("# basilisk-texel-rows-v1 test\nfen,result,game_id,start_id,ply,phase\n"
                        "a,1,g1,s,10,20\nb,0,g1,s,11,20\nc,0.5,g2,s,12,4\n", encoding="utf-8")
        return path

    def test_lines_join_their_rows(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.sample(Path(tmp))
            used, score, index, lazy = fs.join_basilisk(
                path, 3, "3 40 0\n5 -7 1\n", "40 1\n0 0\n-9 0.5\n", 2, np.zeros((2, 2)))
        self.assertEqual(used.tolist(), [True, False, True])
        self.assertEqual(index.tolist(), [0, -1, 1])
        self.assertEqual(score.tolist(), [40, 0, -7])  # a lazy row keeps its played score
        self.assertEqual(lazy.tolist(), [False, False, True])

    def test_a_misaligned_row_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.sample(Path(tmp))
            with self.assertRaisesRegex(fs.Refused, "misaligned"):
                fs.join_basilisk(path, 3, "3 40 0\n4 5 0\n", "40 1\n0 0\n-9 0.5\n", 2, np.zeros((2, 2)))

    def test_a_line_outside_the_sample_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.sample(Path(tmp))
            with self.assertRaisesRegex(fs.Refused, "no sample row"):
                fs.join_basilisk(path, 3, "9 40 0\n", "40 1\n0 0\n-9 0.5\n", 2, np.zeros((1, 2)))


class Screen(unittest.TestCase):
    def data(self, directory: Path, results=None) -> Path:
        rng = np.random.default_rng(5)
        n = 6000
        base = rng.normal(0, 150, n)
        hidden = rng.normal(0, 1, n)  # signal neither whole score holds
        phase = rng.integers(0, 25, n)
        logit = 0.006 * base + 1.2 * hidden
        y = results if results is not None else np.round(2 / (1 + np.exp(-logit))) / 2
        terms = np.zeros((n, len(fs.DONOR_TERMS), 2))
        terms[:, 0] = np.column_stack([base / 100, base / 100])  # material: the score again
        terms[:, fs.DONOR_TERMS.index("Space")] = np.column_stack([hidden, hidden])
        terms[:, fs.DONOR_TERMS.index("Pawns")] = rng.normal(0, 1, (n, 2))  # noise
        families = np.column_stack([hidden * 50, rng.normal(0, 50, n)])
        path = directory / "dataset.npz"
        np.savez_compressed(path, result=y, game=np.array([f"g{i // 20}" for i in range(n)]), phase=phase,
                            served=base, used=np.ones(n, bool), lazy=np.zeros(n, bool), families=families,
                            family_names=np.array(["space", "noise"]), donor_labels=np.array(["D"]),
                            D_terms=terms, D_total=base / 100)
        return path

    def run_screen(self, path: Path, directory: Path) -> dict:
        import json
        out = directory / "report.json"
        status = fs.main(["screen", "--data", str(path), "--out", str(out), "--reps", "100"])
        self.assertEqual(status, 0)
        return json.loads(out.read_text(encoding="utf-8"))

    def test_a_planted_family_is_found_both_ways_and_noise_is_not(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            report = self.run_screen(self.data(Path(tmp)), Path(tmp))
        cell = report["donors"]["D"]["bands"]["all"]
        space = cell["donor_into_basilisk"]["Space"]
        pawns = cell["donor_into_basilisk"]["Pawns"]
        self.assertGreater(space["ci_e4"][0], 0)
        self.assertLessEqual(pawns["ci_e4"][0], 0)
        self.assertGreater(cell["basilisk_into_donor"]["space"]["ci_e4"][0], 0)
        self.assertLessEqual(cell["basilisk_into_donor"]["noise"]["ci_e4"][0], 0)

    def test_labels_that_are_not_results_are_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self.data(Path(tmp), results=np.full(6000, 0.3))
            status = fs.main(["screen", "--data", str(path), "--out", str(Path(tmp) / "r.json")])
        self.assertEqual(status, 2)


@unittest.skipUnless(TUNER, "set BASILISK_TEXEL to a basilisk-texel binary")
class WithTheTuner(unittest.TestCase):
    def test_row_lines_name_the_coefficient_rows_in_order(self) -> None:
        rows = HERE.parents[0] / "texel" / "test_positions.csv"
        with tempfile.TemporaryDirectory() as tmp:
            dump, lines = Path(tmp) / "c.f64", Path(tmp) / "l.txt"
            for mode, out in (("--dump-coeffs", dump), ("--dump-row-lines", lines)):
                done = subprocess.run([TUNER, mode, str(rows), str(out)], capture_output=True, text=True)
                self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
            width = len([l for l in (HERE.parents[0] / "texel" / "surface_v1.tsv").read_text(
                encoding="utf-8").splitlines() if l and l[0].isdigit()])
            count = np.fromfile(dump, dtype="<f8").size // width
            pairs = [l.split() for l in lines.read_text(encoding="utf-8").splitlines()]
            served = subprocess.run([TUNER, "--dump-eval", str(rows)], capture_output=True, text=True).stdout
        self.assertEqual(len(pairs), count)
        numbers = [int(p[0]) for p in pairs]
        self.assertEqual(numbers, sorted(numbers))
        scores = [int(l.split()[0]) for l in served.splitlines() if len(l.split()) == 2]
        for (line, played, lazy), score in zip(pairs, scores):
            if lazy == "0":
                self.assertEqual(int(played), score)


if __name__ == "__main__":
    unittest.main()
