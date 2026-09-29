from __future__ import annotations

import importlib.util
import pathlib
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "branching", ROOT / "tools" / "diag" / "branching.py"
)
branching = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(branching)


class BranchingProfileTests(unittest.TestCase):
    def test_bound_iteration_does_not_count_as_completed(self):
        lines = [
            "info depth 8 score cp 1 nodes 100 pv a2a4",
            "info depth 9 score cp 5 lowerbound nodes 500 pv b2b4",
            "bestmove b2b4",
        ]
        self.assertEqual(branching.completed_nodes(lines, 8), 100)
        self.assertEqual(branching.completed_nodes(lines, 9), 0)

    def test_per_position_median_resists_endpoint_outlier(self):
        rows = [
            {"index": 1, "fen": "a", "nodes": {"4": 100, "5": 200, "6": 400}},
            {"index": 2, "fen": "b", "nodes": {"4": 100, "5": 200, "6": 400}},
            {"index": 3, "fen": "c", "nodes": {"4": 100, "5": 200, "6": 40000}},
        ]
        profile = branching.summarize_profile(rows, 4, 6)
        self.assertEqual(profile["per_position_span"]["median"], 2.0)
        self.assertGreater(profile["geometric_mean_ratio"], 10.0)

    def test_arm_spec_carries_reference_options(self):
        label, path, options = branching.parse_arm_spec(
            "oracle=oracle.exe|Use Basilisk HCE=true"
        )
        self.assertEqual(label, "oracle")
        self.assertEqual(path.name, "oracle.exe")
        self.assertEqual(options, [("Use Basilisk HCE", "true")])
        with self.assertRaisesRegex(ValueError, "fixed globally"):
            branching.parse_arm_spec("oracle=oracle.exe|Hash=32")

    def test_measurement_opens_one_session_per_depth(self):
        opened = []

        class FakeSession:
            def __init__(self, path, options, hash_mb, timeout):
                self.depth = None
                self.seen = []
                opened.append(self)

            def search(self, fen, depth):
                self.depth = depth
                self.seen.append(fen)
                return depth * 100 + len(self.seen)

            def close(self):
                pass

        engine = pathlib.Path(__file__)
        with mock.patch.object(branching, "EngineSession", FakeSession):
            raw = branching.measure_arm(
                "same", engine, [], ["fen-a", "fen-b"], 4, 5, 64, 10
            )
        self.assertEqual([session.depth for session in opened], [4, 5])
        self.assertEqual([session.seen for session in opened],
                         [["fen-a", "fen-b"], ["fen-a", "fen-b"]])
        included, excluded = branching.common_positions([raw], 4, 5)
        self.assertEqual(included, {1, 2})
        self.assertEqual(excluded, [])
        profile = branching.finalize_arm(raw, included, 4, 5)
        same = branching.comparison(profile, profile)
        self.assertEqual(same["branching_ratio"], 1.0)
        self.assertEqual(set(same["nodes_ratio_by_depth"].values()), {1.0})

    def test_all_arms_use_the_same_completed_position_set(self):
        base = {
            "label": "base",
            "position_rows": [
                {"index": 1, "fen": "a", "nodes": {"4": 10, "5": 20}},
                {"index": 2, "fen": "b", "nodes": {"4": 10, "5": 20}},
            ],
        }
        reference = {
            "label": "ref",
            "position_rows": [
                {"index": 1, "fen": "a", "nodes": {"4": 10, "5": 20}},
                {"index": 2, "fen": "b", "nodes": {"4": 0, "5": 0}},
            ],
        }
        included, excluded = branching.common_positions([base, reference], 4, 5)
        self.assertEqual(included, {1})
        self.assertEqual(excluded[0]["missing_depths"], {"ref": [4, 5]})


if __name__ == "__main__":
    unittest.main()
