from __future__ import annotations

import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "deep_iteration_screen", ROOT / "tools" / "diag" / "deep_iteration_screen.py"
)
screen = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(screen)


class DeepIterationScreenTests(unittest.TestCase):
    def test_bound_lines_are_not_completed_iterations(self):
        lines = [
            "info depth 16 seldepth 30 score cp 10 nodes 1000 nps 1 pv e2e4",
            "info depth 17 seldepth 31 score cp 40 lowerbound nodes 1500 pv e2e4",
            "info depth 17 seldepth 31 score cp 30 nodes 2000 nps 1 pv e2e4",
            "bestmove e2e4",
        ]
        self.assertEqual(screen.completed_iterations(lines), {16: 1000, 17: 2000})

    def test_geomean_over_common_depths_and_missing_depths(self):
        base = {16: 100, 17: 200, 18: 400, 19: 800}
        arm = {16: 200, 17: 800, 18: 800}
        row = screen.compare(base, arm, 16, 19)
        self.assertEqual(row["depths_compared"], [16, 17, 18])
        self.assertAlmostEqual(row["geomean"], (2 * 4 * 2) ** (1 / 3))
        self.assertEqual(row["arm_missing_depths"], [19])
        self.assertFalse(screen.verdict(row, 3.0))

    def test_verdict_passes_under_the_limit_with_every_depth(self):
        row = screen.compare({16: 100, 17: 100}, {16: 250, 17: 300}, 16, 19)
        self.assertTrue(screen.verdict(row, 3.0))
        self.assertFalse(screen.verdict(row, 2.0))

    def test_no_common_depth_fails(self):
        row = screen.compare({}, {16: 1}, 16, 19)
        self.assertIsNone(row["geomean"])
        self.assertFalse(screen.verdict(row, 3.0))


if __name__ == "__main__":
    unittest.main()
