from __future__ import annotations

import importlib.util
import pathlib
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "run_suite", ROOT / "tools" / "diag" / "run_suite.py"
)
run_suite = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(run_suite)


class FixedBudgetRecordTests(unittest.TestCase):
    def test_counter_schema_is_complete_and_units_are_named(self):
        self.assertEqual(len(run_suite.CORE_COUNTER_UNITS), 57)
        self.assertNotIn(None, run_suite.CORE_COUNTER_UNITS.values())
        counters = {name: 0 for name in run_suite.CORE_COUNTER_UNITS}
        checks = run_suite.validate_diag(counters)
        self.assertTrue(all(checks.values()))

    def test_counter_parser_rejects_duplicates_and_broken_identity(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            run_suite.parse_diag(
                "info string diag kv tt_hits=1\n"
                "info string diag kv tt_hits=2\n"
            )
        counters = {name: 0 for name in run_suite.CORE_COUNTER_UNITS}
        counters["lmr_eligible"] = 1
        with self.assertRaisesRegex(ValueError, "eligibility"):
            run_suite.validate_diag(counters)

    def test_last_completed_iteration_ignores_later_bound(self):
        output = "\n".join([
            "info depth 7 seldepth 10 score cp 1 nodes 700 pv a2a4",
            "info depth 8 seldepth 12 score cp 5 lowerbound nodes 850 pv b2b4",
            "bestmove b2b4",
        ])
        self.assertEqual(run_suite.last_depth_nodes(output), (7, 700))
        self.assertEqual(run_suite.bestmove(output), "b2b4")

    def test_differential_records_disagreement_and_excludes_null_moves(self):
        outputs = iter([
            "info depth 8 nodes 100 pv a2a4\nbestmove a2a4\n",
            "info depth 9 nodes 101 pv b2b4\nbestmove b2b4\n",
            "bestmove 0000\n",
            "bestmove 0000\n",
        ])
        with mock.patch.object(run_suite, "uci_run", side_effect=lambda *args: next(outputs)):
            rows = run_suite.run_differential(
                pathlib.Path("basilisk"), pathlib.Path("oracle"),
                ["fen one", "fen two"], 100, ["setoption name Hash value 64"]
            )
        self.assertEqual(rows[0]["position"], 1)
        self.assertFalse(rows[0]["bestmove_agreement"])
        self.assertTrue(rows[0]["bestmove_comparable"])
        self.assertFalse(rows[1]["bestmove_comparable"])

    def test_missing_bestmove_is_rejected(self):
        outputs = iter(["info depth 1 nodes 1 pv a2a4\n", "bestmove a2a4\n"])
        with mock.patch.object(run_suite, "uci_run", side_effect=lambda *args: next(outputs)):
            with self.assertRaisesRegex(RuntimeError, "no bestmove"):
                run_suite.run_differential(
                    pathlib.Path("basilisk"), pathlib.Path("oracle"),
                    ["fen"], 1, []
                )


if __name__ == "__main__":
    unittest.main()
