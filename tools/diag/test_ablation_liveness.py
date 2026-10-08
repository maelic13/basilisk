from __future__ import annotations

import importlib.util
import pathlib
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "ablation_liveness", ROOT / "tools" / "diag" / "ablation_liveness.py"
)
ablation = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(ablation)


class AblationLivenessTests(unittest.TestCase):
    def test_arm_spec_keeps_engine_options(self):
        label, path, options = ablation.parse_engine_spec(
            "oracle=oracle.exe|Use Basilisk HCE=true"
        )
        self.assertEqual(label, "oracle")
        self.assertEqual(path.name, "oracle.exe")
        self.assertEqual(options, [("Use Basilisk HCE", "true")])

    def test_bound_lines_do_not_count_as_completed_depth(self):
        lines = [
            "info depth 9 score cp 1 lowerbound nodes 99 pv e2e4",
            "info depth 9 score cp 0 nodes 123 pv e2e4",
            "bestmove e2e4",
        ]
        self.assertEqual(ablation.completed_nodes(lines, 9), 123)

    def test_every_bit_must_change_at_least_one_position(self):
        class FakeSession:
            def __init__(self, path, options, mask, hash_mb, timeout):
                self.mask = mask

            def search(self, fen, depth):
                return 100 + (self.mask if fen == "a" else 0)

            def close(self):
                pass

        with mock.patch.object(ablation, "EngineSession", FakeSession):
            result = ablation.measure(pathlib.Path("engine"), [], ["a", "b"], 9, 64, 5)
        self.assertTrue(all(check["live"] for check in result["checks"]))
        self.assertEqual(result["checks"][3]["changed_positions"], [1])

    def test_dead_bit_is_rejected(self):
        class FakeSession:
            def __init__(self, path, options, mask, hash_mb, timeout):
                self.mask = mask

            def search(self, fen, depth):
                return 100 if self.mask == 4 else 100 + self.mask

            def close(self):
                pass

        with mock.patch.object(ablation, "EngineSession", FakeSession):
            with self.assertRaisesRegex(RuntimeError, "null_move"):
                ablation.measure(pathlib.Path("engine"), [], ["a"], 9, 64, 5)


if __name__ == "__main__":
    unittest.main()
