from __future__ import annotations

import copy
import importlib.util
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
DIAG = ROOT / "tools" / "diag"
sys.path.insert(0, str(DIAG))


def load_module(name: str, filename: str):
    spec = importlib.util.spec_from_file_location(name, DIAG / filename)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(module)
    return module


probe = load_module("fixed_budget_probe", "fixed_budget_probe.py")
canary = load_module("canary", "canary.py")


class FixedBudgetProbeTests(unittest.TestCase):
    def test_epd_parser_converts_san_and_ignores_comments(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "suite.epd"
            path.write_text(
                '2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 '
                'w - - bm Qg6; id "WAC.001";\n'
                '# comment\n'
                '8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 '
                'b - - bm Rxb2; id "WAC.002";\n',
                encoding="utf-8",
            )
            items = probe.parse_epd(path)
        self.assertEqual([item["id"] for item in items], ["WAC.001", "WAC.002"])
        self.assertEqual(items[0]["bm"], ["g3g6"])
        self.assertEqual(items[1]["bm"], ["b3b2"])

    def test_last_completed_rejects_bound_only_iteration(self):
        infos = [
            "info depth 13 seldepth 27 score cp 148 nodes 173821 time 106 pv d5d4",
            "info depth 14 seldepth 26 score cp 112 upperbound nodes 200255 time 116 pv d5d4",
        ]
        self.assertEqual(probe.last_completed(infos)["depth"], 13)
        self.assertEqual(probe.last_completed(infos[1:]), {})

    def test_engine_spec_and_rejection_detection(self):
        self.assertEqual(
            probe.parse_engine_spec("oracle=x.exe|Use Basilisk HCE=true"),
            ("oracle", "x.exe", [("Use Basilisk HCE", "true")]),
        )
        self.assertTrue(probe.option_rejected("info string No such option: Foo"))
        self.assertTrue(probe.option_rejected("Unknown option Bar"))
        with self.assertRaisesRegex(SystemExit, "fixed by the probe"):
            probe.parse_engine_spec("engine=x.exe|Hash=16")

    def test_missing_completed_depth_is_rejected(self):
        probe.require_complete_depths([1, 2, 3], 3, "engine", "WAC.001")
        with self.assertRaisesRegex(RuntimeError, "expected every depth"):
            probe.require_complete_depths([1, 3], 3, "engine", "WAC.001")


def fake_inputs():
    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    items = []
    depth_engines = {"baseline": {}, "oracle": {}}
    node_engines = {"baseline": {}, "oracle": {}}
    for number in range(1, 301):
        ident = f"WAC.{number:03d}"
        items.append({"id": ident, "fen": fen, "bm": ["e2e4"]})
        oracle_stable = 10 if ident == "WAC.001" else 2
        depth_engines["oracle"][ident] = {"stable": oracle_stable}
        depth_engines["baseline"][ident] = {"stable": oracle_stable + 1}
        node_engines["oracle"][ident] = {"solved": True}
        node_engines["baseline"][ident] = {"solved": True}
    node_engines["baseline"]["WAC.002"]["solved"] = False
    meta = {
        "baseline": {"path": "baseline.exe", "sha256": "a" * 64},
        "oracle": {"path": "oracle.exe", "sha256": "b" * 64},
    }
    depth = {
        "mode": "depthpv", "budget": 12, "hash_mb": 64, "threads": 1,
        "engines": depth_engines, "engine_meta": copy.deepcopy(meta),
    }
    nodes = {
        "mode": "nodes", "budget": 100000, "hash_mb": 64, "threads": 1,
        "engines": node_engines, "engine_meta": copy.deepcopy(meta),
    }
    return items, depth, nodes


class CanaryPolicyTests(unittest.TestCase):
    def test_manifest_admits_only_baseline_passes_and_keeps_quiet_threat(self):
        items, depth, nodes = fake_inputs()
        with tempfile.TemporaryDirectory() as directory:
            suite = pathlib.Path(directory) / "wac.epd"
            suite.write_text("fixture\n", encoding="utf-8")
            with mock.patch.object(canary.probe, "parse_epd", return_value=items):
                manifest = canary.build_manifest(depth, nodes, suite)
        self.assertEqual(manifest["summary"]["reference_anchors"], 300)
        self.assertEqual(manifest["summary"]["required_canaries"], 299)
        self.assertEqual(manifest["summary"]["baseline_gaps"], 1)
        threat = next(row for row in manifest["records"] if row["id"] == "WAC.001")
        self.assertTrue(threat["required"])
        self.assertTrue(threat["quiet_mate_threat"])

    def test_regression_fails_and_new_pass_is_only_reported(self):
        items, depth, nodes = fake_inputs()
        with tempfile.TemporaryDirectory() as directory:
            suite = pathlib.Path(directory) / "wac.epd"
            suite.write_text("fixture\n", encoding="utf-8")
            with mock.patch.object(canary.probe, "parse_epd", return_value=items):
                manifest = canary.build_manifest(depth, nodes, suite)
                candidate_depth = copy.deepcopy(depth)
                candidate_nodes = copy.deepcopy(nodes)
                for report in (candidate_depth, candidate_nodes):
                    report["engines"]["candidate"] = report["engines"].pop("baseline")
                    report["engine_meta"]["candidate"] = report["engine_meta"].pop("baseline")
                candidate_depth["engines"]["candidate"]["WAC.003"]["stable"] = None
                candidate_nodes["engines"]["candidate"]["WAC.002"]["solved"] = True
                result = canary.evaluate(
                    manifest, candidate_depth, candidate_nodes, suite, "candidate"
                )
        self.assertFalse(result["pass"])
        self.assertEqual([row["id"] for row in result["regressions"]], ["WAC.003"])
        self.assertEqual([row["id"] for row in result["new_passes"]], ["WAC.002"])

    def test_binary_identity_must_match_between_runs(self):
        items, depth, nodes = fake_inputs()
        nodes["engine_meta"]["baseline"]["sha256"] = "c" * 64
        with self.assertRaisesRegex(ValueError, "identity differs"):
            canary.validate_reports(depth, nodes, items, ("baseline",))

    def test_frozen_manifest_cannot_be_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "canary_v1.json"
            canary.write_result(path, {"version": 1}, exclusive=True)
            with self.assertRaisesRegex(SystemExit, "refusing to overwrite"):
                canary.write_result(path, {"version": 2}, exclusive=True)


if __name__ == "__main__":
    unittest.main()
