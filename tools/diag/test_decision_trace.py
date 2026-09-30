from __future__ import annotations

import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "decision_trace", ROOT / "tools" / "diag" / "decision_trace.py"
)
decision_trace = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(decision_trace)


def valid_lines() -> list[str]:
    fields = {
        "seq": 0, "event": "rfp_prune", "ply": 1, "depth": 4,
        "move": "none", "alpha": -10, "beta": 5,
        "estimated_score": 20, "improving": 1, "correction": 3,
        "history": 32002, "move_count": 0, "reduction": 0,
        "cutoff_count": -1, "window_alpha": -10, "window_beta": 5,
        "margin": 12, "result": 20,
    }
    record = " ".join(f"{key}={value}" for key, value in fields.items())
    return [
        "info string trace begin version=1 plies=1-2 root=e2e4 "
        "cutoff_count=unavailable value_none=32002 bool_unknown=-1 capacity=32768",
        "info string trace record " + record,
        "info string trace end status=ok records=1",
    ]


class DecisionTraceTests(unittest.TestCase):
    def test_valid_trace_is_typed_and_counted(self):
        parsed = decision_trace.parse_trace(valid_lines())
        self.assertEqual(parsed["records"][0]["event"], "rfp_prune")
        self.assertEqual(parsed["records"][0]["estimated_score"], 20)

    def test_overflow_is_rejected(self):
        lines = valid_lines()
        lines[-1] = "info string trace end status=overflow records=1"
        with self.assertRaisesRegex(ValueError, "incomplete"):
            decision_trace.parse_trace(lines)

    def test_missing_field_is_rejected(self):
        lines = [line.replace(" history=32002", "") for line in valid_lines()]
        with self.assertRaisesRegex(ValueError, "schema mismatch"):
            decision_trace.parse_trace(lines)

    def test_out_of_range_ply_is_rejected(self):
        lines = [line.replace(" ply=1", " ply=3") for line in valid_lines()]
        with self.assertRaisesRegex(ValueError, "out-of-contract"):
            decision_trace.parse_trace(lines)


if __name__ == "__main__":
    unittest.main()
