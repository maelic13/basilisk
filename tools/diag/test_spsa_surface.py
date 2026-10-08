from __future__ import annotations

import importlib.util
import json
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


def load(name: str, path: pathlib.Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(module)
    return module


surface = load("surface", ROOT / "tools" / "generate_spsa_surface.py")
block = load("block", ROOT / "tools" / "spsa_block_rule.py")


class SurfaceTests(unittest.TestCase):
    def test_source_table_is_complete_and_ordered(self):
        parameters = surface.parse_parameters(surface.HEADER.read_text(encoding="utf-8"))
        generated = json.loads(surface.CONFIG.read_text(encoding="utf-8"))
        self.assertEqual([item[0] for item in parameters], list(generated))
        self.assertEqual(len(parameters), 41)

    def test_steps_are_half_up_range_sixteenths_with_floor(self):
        self.assertEqual(surface.perturbation(0, 40), 3)
        self.assertEqual(surface.perturbation(0, 24), 2)
        self.assertEqual(surface.perturbation(2, 6), 2)

    def test_parser_rejects_default_outside_range(self):
        with self.assertRaisesRegex(ValueError, "outside"):
            surface.parse_parameters("X(field, Name, 11, 0, 10)")

    def test_block_rule_uses_block_seed_and_registered_step(self):
        config = {
            "A": {"step": 2},
            "B": {"step": 4},
            "C": {"step": 10},
        }
        result = {
            "driver": {"status": "completed"},
            "tuned_result": {
                "settings": {"iterations": 2000},
                "completed_iterations": 2000,
                "parameters": [
                    {"name": "A", "original": 5, "tuned": 7},
                    {"name": "B", "original": 10, "tuned": 6},
                    {"name": "C", "original": 20, "tuned": 25},
                ],
            },
        }
        verdict, movers = block.evaluate(result, config, 3)
        self.assertEqual(verdict, "STOP")
        self.assertEqual([item[0] for item in movers], ["A", "B"])

    def test_incomplete_block_is_rejected(self):
        result = {
            "driver": {"status": "stopped"},
            "tuned_result": {"settings": {"iterations": 2000}, "completed_iterations": 1999},
        }
        with self.assertRaisesRegex(ValueError, "not a completed block"):
            block.evaluate(result, {}, 3)


if __name__ == "__main__":
    unittest.main()
