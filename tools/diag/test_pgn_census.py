from __future__ import annotations

import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "pgn_census", ROOT / "tools" / "diag" / "pgn_census.py"
)
assert SPEC and SPEC.loader
census = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(census)


class PgnCensusTests(unittest.TestCase):
    def test_result_uses_named_engine_perspective(self):
        self.assertEqual(census.result_for("1-0", True), "wins")
        self.assertEqual(census.result_for("1-0", False), "losses")
        self.assertEqual(census.result_for("0-1", False), "wins")
        self.assertEqual(census.result_for("1/2-1/2", True), "draws")
        self.assertEqual(census.result_for("*", True), "unfinished")

    def test_summary_filters_and_splits_colors(self):
        headers = [
            {"White": "Us", "Black": "A", "Result": "1-0"},
            {"White": "A", "Black": "Us", "Result": "1/2-1/2"},
            {"White": "Us", "Black": "A", "Result": "0-1"},
            {"White": "B", "Black": "Us", "Result": "0-1"},
            {"White": "Us", "Black": "B", "Result": "*"},
            {"White": "A", "Black": "B", "Result": "1-0"},
        ]
        report = census.summarize(iter(headers), "Us", {"A"})
        self.assertEqual(report["games_in_pgn"], 6)
        self.assertEqual(report["engine_games"], 3)
        self.assertEqual(set(report["records"]), {"A"})
        row = report["records"]["A"]
        self.assertEqual(row["white_games"], 2)
        self.assertEqual(row["black_games"], 1)
        self.assertEqual((row["wins"], row["draws"], row["losses"]), (1, 1, 1))
        self.assertEqual(row["points_twice"], 3)
        self.assertEqual(row["score_percent"], 50.0)
        self.assertEqual(row["elo_difference"], 0.0)

    def test_unfinished_is_excluded_from_score(self):
        headers = [
            {"White": "Us", "Black": "A", "Result": "1-0"},
            {"White": "A", "Black": "Us", "Result": "*"},
        ]
        row = census.summarize(iter(headers), "Us")["records"]["A"]
        self.assertEqual(row["unfinished"], 1)
        self.assertEqual(row["score_percent"], 100.0)
        self.assertIsNone(row["elo_difference"])


if __name__ == "__main__":
    unittest.main()
