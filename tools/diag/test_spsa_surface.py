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
        self.assertEqual(len(parameters), 19)

    def test_core_arm_table_is_not_on_the_registered_surface(self):
        text = (
            "#define BASILISK_LEGACY_KERNEL_PARAMS(X) \\\n"
            "    X(a_field, Alpha, 5, 0, 10)\n"
            "#define BASILISK_CORE_KERNEL_PARAMS(X) \\\n"
            "    BASILISK_LEGACY_KERNEL_PARAMS(X) \\\n"
            "    X(b_field, Beta, 5, 0, 10)\n"
            "#define BASILISK_ROOT_PARAMS(X) \\\n"
            "    X(c_field, Gamma, 5, 0, 10)\n"
            "    S(d_field, Delta, 1, 0, 1)\n"
        )
        names = [item[0] for item in surface.parse_parameters(text)]
        self.assertEqual(names, ["Alpha", "Gamma"])
        # The core surface is the core table's own lines: neither the legacy
        # group it splices in, nor the tables around it, nor a switch.
        core = [item[0] for item in surface.parse_parameters(text, core=True)]
        self.assertEqual(core, ["Beta"])

    def test_fixed_lines_stay_off_the_core_surface(self):
        text = (
            "#define BASILISK_CORE_KERNEL_PARAMS(X, F) \\\n"
            "    X(b_field, Beta, 5, 0, 10) \\\n"
            "    F(e_field, Epsilon, 5, 0, 10) \\\n"
            "    X(g_field, Eta, 5, 0, 10)\n"
            "#define BASILISK_ROOT_PARAMS(X) \\\n"
            "    X(c_field, Gamma, 5, 0, 10)\n"
        )
        core = [item[0] for item in surface.parse_parameters(text, core=True)]
        self.assertEqual(core, ["Beta", "Eta"])
        self.assertEqual([item[0] for item in surface.parse_parameters(text)], ["Gamma"])

    def test_core_surface_is_the_clusters_coordinates(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        parameters = surface.parse_parameters(text, core=True)
        generated = json.loads(surface.CORE_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual([item[0] for item in parameters], list(generated))
        # The core table's X lines; its F line (CoreQuietSeeCoeff) stays off.
        self.assertEqual(len(parameters), 49)
        self.assertNotIn("CoreQuietSeeCoeff", generated)
        self.assertTrue(all(name.startswith("Core") for name in generated))
        self.assertFalse(set(generated) & {"NullBase", "ProbCutMargin", "LmrSingularQuiet"})

    def test_b3_table_is_its_own_surface(self):
        text = (
            "#define BASILISK_CORE_KERNEL_PARAMS(X, F) \\\n"
            "    X(b_field, Beta, 5, 0, 10)\n"
            "#define BASILISK_B3_PARAMS(X, F, S) \\\n"
            "    S(s_field, Sigma, 1, 0, 1) \\\n"
            "    X(t_field, Tau, 5, 0, 10) \\\n"
            "    F(u_field, Upsilon, 5, 0, 10)\n"
            "#define BASILISK_ROOT_PARAMS(X) \\\n"
            "    X(c_field, Gamma, 5, 0, 10)\n"
        )
        self.assertEqual([item[0] for item in surface.parse_parameters(text)], ["Gamma"])
        self.assertEqual([item[0] for item in surface.parse_parameters(text, core=True)], ["Beta"])
        self.assertEqual([item[0] for item in surface.parse_parameters(text, b3=True)], ["Tau"])

    def test_b3_surface_is_the_clusters_coordinates(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        parameters = surface.parse_parameters(text, b3=True)
        generated = json.loads(surface.B3_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual([item[0] for item in parameters], list(generated))
        self.assertTrue(all(name.startswith("B3") for name in generated))
        # F lines and switches stay off it.
        self.assertFalse(set(generated) & {"B3NmpBetaFloor", "B3NmpVerify", "B3CheckExt"})

    def test_b3_sweep_notes_are_checked_against_the_table(self):
        text = (
            "#define BASILISK_B3_PARAMS(X, F, S) \\\n"
            "    X(t_field, Tau, 5, 0, 10) \\\n"
            "    F(u_field, Upsilon, 3, 0, 4) \\\n"
            "    S(s_field, Sigma, 1, 0, 1)\n"
        )
        notes = surface.sweep_notes(text, {"Upsilon": (0, 1, 3)}, {"Tau": (2, 9)})
        self.assertEqual(notes, [
            "# sweep coordinate Upsilon: 0, 1, 3 (default 3; never fitted by SPSA)",
            "# sweep extra points Tau: 2, 9 (beside the default 5)",
        ])
        for coordinates, extra in (({"Tau": (0, 5)}, {}),          # an X line
                                   ({"Sigma": (0, 1)}, {}),        # a switch
                                   ({"Upsilon": (0, 1)}, {}),      # default missing
                                   ({"Upsilon": (3, 5)}, {}),      # out of range
                                   ({}, {"Upsilon": (1,)}),        # an F line
                                   ({}, {"Tau": (5, 9)}),          # the default
                                   ({}, {"Tau": (11,)})):          # out of range
            with self.assertRaises(ValueError):
                surface.sweep_notes(text, coordinates, extra)

    def test_b3_tune_file_carries_the_registered_sweep(self):
        tune = surface.B3_TUNE.read_text(encoding="utf-8")
        self.assertIn("# sweep coordinate B3SeekMateDepth: 8, 12, 16", tune)
        self.assertIn("# sweep coordinate B3SingNegative: 0, 1, 3", tune)
        self.assertIn("# sweep extra points B3SingMargin: 17, 126", tune)

    def test_b33_surface_is_the_b3_lines_and_the_admissions(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        b3 = [item[0] for item in surface.parse_parameters(text, b3=True)]
        generated = json.loads(surface.B33_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual(list(generated),
                         [name for name in b3 if name != "B3ProbCutTtMargin"]
                         + list(surface.B33_ADMITTED_CORE))
        self.assertEqual(len(generated), 29)
        # The core coordinates keep the core table's ranges and steps and
        # start from the B3_PROOF arm's own default where it has one.
        core = json.loads(surface.CORE_CONFIG.read_text(encoding="utf-8"))
        overrides = surface.b3_core_defaults(text)
        self.assertTrue(overrides)
        for name in surface.B33_ADMITTED_CORE:
            wanted = dict(core[name], value=overrides.get(name, core[name]["value"]))
            self.assertEqual(generated[name], wanted)

    def test_b3_core_defaults_are_checked_against_the_core_table(self):
        head = (
            "#define BASILISK_CORE_KERNEL_PARAMS(X, F) \\\n"
            "    X(b_field, Beta, 5, 0, 10) \\\n"
            "    F(e_field, Epsilon, 5, 0, 10)\n"
            "#define BASILISK_B3_CORE_DEFAULTS(A) \\\n"
        )
        self.assertEqual(surface.b3_core_defaults(head + "    A(b_field, Beta, 7)\n"),
                         {"Beta": 7})
        # The core surface is the X lines' values, not the arm's.
        text = head + "    A(b_field, Beta, 7)\n"
        self.assertEqual(surface.parse_parameters(text, core=True), [("Beta", 5, 0, 10)])
        for body in ("    A(e_field, Epsilon, 7)\n",          # an F line
                     "    A(z_field, Zeta, 7)\n",             # no line
                     "    A(b_field, Beta, 5)\n",             # the default
                     "    A(b_field, Beta, 11)\n",            # out of range
                     "    A(b_field, Beta, 7) \\\n    A(b_field, Beta, 8)\n"):  # twice
            with self.assertRaises(ValueError):
                surface.b3_core_defaults(head + body)

    def test_b33_surface_rejects_a_name_with_no_x_line(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        saved = surface.B33_ADMITTED_CORE
        try:
            surface.B33_ADMITTED_CORE = saved + ("CoreNoSuchCoordinate",)
            with self.assertRaises(ValueError):
                surface.b33_parameters(text)
        finally:
            surface.B33_ADMITTED_CORE = saved

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
