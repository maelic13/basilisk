from __future__ import annotations

import importlib.util
import json
import math
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


class SurfaceTests(unittest.TestCase):
    def test_source_table_is_complete_and_ordered(self):
        parameters = surface.parse_parameters(surface.HEADER.read_text(encoding="utf-8"))
        generated = json.loads(surface.CONFIG.read_text(encoding="utf-8"))
        self.assertEqual([item[0] for item in parameters], list(generated))
        # The root table and the one inherited line.
        self.assertEqual(len(parameters), 12)
        self.assertEqual(parameters[-1][0], "LmrSingularQuiet")

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
        self.assertEqual(len(parameters), 48)
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

    def test_b4_surface_is_the_clusters_coordinates(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        parameters = surface.parse_parameters(text, b4=True)
        generated = json.loads(surface.B4_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual([item[0] for item in parameters], list(generated))
        self.assertEqual(list(generated),
                         ["B4QsFutBase", "B4QsStandPatBlend", "B4QsFailHighBlend"])
        # Its table stays off every other surface.
        for other in (surface.parse_parameters(text), surface.parse_parameters(text, core=True),
                      surface.parse_parameters(text, b3=True)):
            self.assertFalse({item[0] for item in other} & set(generated))

    def test_b5_surface_is_the_clusters_coordinates(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        parameters = surface.parse_parameters(text, b5=True)
        generated = json.loads(surface.B5_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual([item[0] for item in parameters], list(generated))
        self.assertEqual(list(generated), ["B5AspBase", "B5AspMsqDiv", "B5AspGrowth"])
        # The base counts thousandths of a unit: seeded at the donor's 5 x 282 and
        # fitted to 2202 (B.5.4), on a range whose every point the window's growth
        # can leave.
        base = generated["B5AspBase"]
        self.assertEqual((base["value"], base["min_value"], base["max_value"]), (2202, 500, 40000))
        # Its table stays off every other surface.
        for other in (surface.parse_parameters(text), surface.parse_parameters(text, core=True),
                      surface.parse_parameters(text, b3=True), surface.parse_parameters(text, b4=True)):
            self.assertFalse({item[0] for item in other} & set(generated))

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
        # start from Cluster 2's refit where it has one.
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
        # The core surface is the X lines' values, not the refit's.
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

    def test_b46_surface_is_the_b4_lines_and_the_admissions(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        b4 = [item[0] for item in surface.parse_parameters(text, b4=True)]
        generated = json.loads(surface.B46_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual(list(generated), b4 + list(surface.B46_ADMITTED))
        self.assertEqual(len(generated), 15)
        # The admitted coordinates keep their tables' ranges and steps and
        # start from the value the engine runs: Cluster 3's refit where it
        # has one, over Cluster 2's and the line's.
        self.assertEqual(generated["CoreRfpPerPly"]["value"], 107)
        self.assertEqual(generated["CoreRfpCorrMult"]["value"], 457)
        self.assertEqual(generated["B3NmpBase"]["value"], 172)
        self.assertEqual(generated["CoreRfpDepthCap"]["value"], 10)
        core = json.loads(surface.CORE_CONFIG.read_text(encoding="utf-8"))
        b3 = json.loads(surface.B3_CONFIG.read_text(encoding="utf-8"))
        for name in surface.B46_ADMITTED:
            source = core[name] if name in core else b3[name]
            self.assertEqual({k: generated[name][k] for k in ("min_value", "max_value", "step")},
                             {k: source[k] for k in ("min_value", "max_value", "step")})

    def test_b4_core_defaults_are_checked_against_the_earlier_tables(self):
        head = (
            "#define BASILISK_CORE_KERNEL_PARAMS(X, F) \\\n"
            "    X(b_field, Beta, 5, 0, 10) \\\n"
            "    F(e_field, Epsilon, 5, 0, 10)\n"
            "#define BASILISK_B3_PARAMS(X, F, S) \\\n"
            "    X(c_field, Gamma, 5, 0, 10) \\\n"
            "    S(s_field, Sigma, 0, 0, 1)\n"
            "#define BASILISK_B4_CORE_DEFAULTS(A) \\\n"
        )
        body = "    A(s_field, Sigma, 1) \\\n    A(b_field, Beta, 7) \\\n    A(c_field, Gamma, 8)\n"
        self.assertEqual(surface.b4_core_defaults(head + body), {"Beta": 7, "Gamma": 8})
        for body in ("    A(e_field, Epsilon, 7)\n",          # an F line
                     "    A(z_field, Zeta, 7)\n",             # no line
                     "    A(b_field, Beta, 5)\n",             # the default
                     "    A(c_field, Gamma, 11)\n",           # out of range
                     "    A(b_field, Beta, 7) \\\n    A(b_field, Beta, 8)\n"):  # twice
            with self.assertRaises(ValueError):
                surface.b4_core_defaults(head + body)

    def test_b46_surface_rejects_a_name_with_no_x_line(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        saved = surface.B46_ADMITTED
        try:
            surface.B46_ADMITTED = saved + ("CoreNoSuchCoordinate",)
            with self.assertRaises(ValueError):
                surface.b46_parameters(text)
        finally:
            surface.B46_ADMITTED = saved

    def test_b54_surface_is_the_b5_lines_and_the_joiners(self):
        # BAS-S54: Cluster 4's X lines, then the five coordinates section 5.8's
        # median rule admitted (analysis/b54_sweep_2026-11-10.md), each from the
        # value the arm runs: Cluster 4's refit, then Cluster 3's, Cluster 2's and
        # the line. Since the bake the arm runs B.5.4's theta.
        text = surface.HEADER.read_text(encoding="utf-8")
        b5 = [item[0] for item in surface.parse_parameters(text, b5=True)]
        generated = json.loads(surface.B54_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual(list(generated), b5 + ["CoreContHistPruneSlope", "CoreCorrUpdateScale",
                                                "B3NmpPerPly", "B3SingMargin", "B3IirMinDepth"])
        self.assertEqual({name: spec["value"] for name, spec in generated.items()},
                         {"B5AspBase": 2202, "B5AspMsqDiv": 2907, "B5AspGrowth": 46,
                          "CoreContHistPruneSlope": 3783, "CoreCorrUpdateScale": 18,
                          "B3NmpPerPly": 11, "B3SingMargin": 201, "B3IirMinDepth": 7})
        self.assertEqual(generated["B5AspBase"]["step"], 1101)
        core = json.loads(surface.CORE_CONFIG.read_text(encoding="utf-8"))
        b3 = json.loads(surface.B3_CONFIG.read_text(encoding="utf-8"))
        for name in surface.B54_ADMITTED:
            source = core[name] if name in core else b3[name]
            self.assertEqual({k: generated[name][k] for k in ("min_value", "max_value", "step")},
                             {k: source[k] for k in ("min_value", "max_value", "step")})

    def test_b5_core_defaults_are_the_fitted_joiners_without_the_switch(self):
        # The B5 table holds the arm's stop (a switch, never on a surface) and
        # Cluster 4's refit of the five joiners.
        text = surface.HEADER.read_text(encoding="utf-8")
        self.assertEqual(surface.b5_core_defaults(text),
                         {"CoreContHistPruneSlope": 3783, "CoreCorrUpdateScale": 18,
                          "B3NmpPerPly": 11, "B3SingMargin": 201, "B3IirMinDepth": 7})
        head = text.split(surface.B5_CORE_DEFAULTS_TABLE)[0] + surface.B5_CORE_DEFAULTS_TABLE + "A) \\\n"
        for body in ("    A(b3_nmp_per_ply, B3NmpPerPly, 99)\n",      # out of range
                     "    A(z_field, CoreNoSuchCoordinate, 7)\n"):    # no line
            with self.assertRaises(ValueError):
                surface.b5_core_defaults(head + body)

    def test_b54_surface_rejects_a_name_with_no_x_line(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        saved = surface.B54_ADMITTED
        try:
            surface.B54_ADMITTED = saved + ("CoreNoSuchCoordinate",)
            with self.assertRaises(ValueError):
                surface.b54_parameters(text)
        finally:
            surface.B54_ADMITTED = saved

    def test_the_window_base_steps_at_most_half_its_start(self):
        # Research amendment 2 (B.5 packet section A2.5): B5AspBase's step is
        # the smaller of the generator's and half the start value, so the low
        # perturbation does not clamp at the seed. The other two keep theirs.
        generated = json.loads(surface.B5_CONFIG.read_text(encoding="utf-8"))
        self.assertEqual(generated["B5AspBase"]["step"], 1101)
        self.assertEqual(generated["B5AspMsqDiv"]["step"], surface.perturbation(1000, 12000))
        self.assertEqual(generated["B5AspGrowth"]["step"], surface.perturbation(32, 96))
        for start, step in ((1410, 705), (2820, 1410), (4938, 2469), (19000, 2469), (1411, 706)):
            self.assertEqual(surface.step_of("B5AspBase", start, 500, 40000), step)
        self.assertEqual(surface.step_of("B5AspMsqDiv", 1000, 1000, 12000), 688)

    def test_steps_are_half_up_range_sixteenths_with_floor(self):
        self.assertEqual(surface.perturbation(0, 40), 3)
        self.assertEqual(surface.perturbation(0, 24), 2)
        self.assertEqual(surface.perturbation(2, 6), 2)

    def test_parser_rejects_default_outside_range(self):
        with self.assertRaisesRegex(ValueError, "outside"):
            surface.parse_parameters("X(field, Name, 11, 0, 10)")

    def test_rule7c_grades_the_perturbation_by_room(self):
        # Two steps where the seed has the room, else the largest from one to
        # two that stays inside the range, never under one step.
        self.assertEqual(surface.graded_perturbation(10, 0, 100, 4), (2.0, 2.5))
        self.assertEqual(surface.graded_perturbation(8, 0, 100, 4), (2.0, 2.0))
        self.assertEqual(surface.graded_perturbation(6, 0, 100, 4), (1.5, 1.5))
        self.assertEqual(surface.graded_perturbation(94, 0, 100, 4), (1.5, 1.5))
        self.assertEqual(surface.graded_perturbation(2, 0, 100, 4), (1.0, 0.5))

    def test_rule7c_tune_file_scales_c_end_by_the_graded_steps(self):
        parameters = [("Wide", 50, 0, 100), ("Edge", 9, 0, 100), ("Tight", 1, 0, 100)]
        tune, report = surface.render_rule7c(parameters, 6000, "test")
        decay = 6000 ** -surface.GAMMA
        values = {}
        for block_text in tune.split("[[parameters]]")[1:]:
            fields = dict(line.split(" = ", 1) for line in block_text.strip().splitlines()
                          if " = " in line and not line.startswith("#"))
            values[fields["name"].strip('"')] = float(fields["c_end"])
        step = surface.perturbation(0, 100)
        down = lambda value: math.floor(value * 1e4) / 1e4  # noqa: E731
        self.assertAlmostEqual(values["Wide"], down(2 * step * decay))
        self.assertAlmostEqual(values["Edge"], down(9 / step * step * decay))
        self.assertAlmostEqual(values["Tight"], down(step * decay))
        # The runner's c0 never exceeds the room, so no first arm is clamped.
        self.assertLessEqual(values["Edge"] * 6000 ** surface.GAMMA, 9.0)
        self.assertIn("N = 6000", tune)
        self.assertEqual(len(report), 2)
        self.assertIn("under one step of room", report[1])

    def test_b67_surface_reads_x_lines_and_literals(self):
        header = surface.HEADER.read_text(encoding="utf-8")
        literals = surface.LITERALS.read_text(encoding="utf-8")
        chosen = surface.b67_parameters(
            header, literals, "B3SingMargin\n# a comment\n\nLitLmrBase  # inline\nLitBoundCont 24000 32767\n"
                              "B5AspBase 500 6000\n")
        head = {p[0]: p for p in surface.head_parameters(header)}
        self.assertEqual(chosen, [head["B3SingMargin"], ("LitLmrBase", 697, 348, 1396),
                                  ("LitBoundCont", 30000, 24000, 32767),
                                  ("B5AspBase", head["B5AspBase"][1], 500, 6000)])
        self.assertEqual(len(surface.kernel_literals(literals)), 128)

    def test_b67_literal_ranges_are_the_screen_options(self):
        # src/kernel_literals.h's kernel_literal_min and kernel_literal_max, as
        # the screen build advertises them (C++ division truncates toward 0).
        cases = {697: (348, 1396), 1: (0, 4), 2: (1, 6), 3: (1, 8), -64: (-130, -32),
                 -4: (-10, -2), -2210: (-4422, -1105), -81: (-164, -41)}
        for value, (low, high) in cases.items():
            self.assertEqual((surface.literal_min(value), surface.literal_max(value)), (low, high), value)

    def test_b67_surface_rejects_what_it_cannot_place(self):
        header = surface.HEADER.read_text(encoding="utf-8")
        literals = surface.LITERALS.read_text(encoding="utf-8")
        for text in ("NoSuchName\n", "B3SingMargin 12 378\n", "B3SingMargin 250 378\n",
                     "B3SingMargin 13\n", "LitLmrBase 100 700\n",
                     "LitLmrBase 700 800\n", "LitLmrBase 1\n", "LitLmrBase\nLitLmrBase\n"):
            with self.assertRaises(ValueError, msg=text):
                surface.b67_parameters(header, literals, text)

    def test_rule7c_head_room_matches_the_recorded_count(self):
        # BAS-S57 counted 10 of the head's 74 coordinates short of two steps of
        # room at their seeds, 3 of them of one. B.6.7's rails (BAS-S61 section 5)
        # widened four ranges and floored two at their switch boundaries: 7
        # short, 4 of them of one.
        text = surface.HEADER.read_text(encoding="utf-8")
        parameters = surface.head_parameters(text)
        _, report = surface.render_rule7c(parameters, 6000, "head")
        self.assertEqual(len(parameters), 74)
        self.assertEqual(len(report), 7)
        self.assertEqual(sum("under one step" in line for line in report), 4)
        names = {line.split(":")[0] for line in report if "under one step" in line}
        self.assertEqual(names, {"CoreCaptFutHistDiv", "CoreLmrAlphaEvalScale", "CoreResearchShallower",
                                 "B3SingTripleQuiet"})

    def test_rule7c_seeds_are_the_values_the_head_runs(self):
        text = surface.HEADER.read_text(encoding="utf-8")
        overrides = {**surface.b3_core_defaults(text), **surface.b4_core_defaults(text),
                     **surface.b5_core_defaults(text)}
        seeds = {name: start for name, start, _, _ in surface.head_parameters(text)}
        for name, value in overrides.items():
            if name in seeds:
                self.assertEqual(seeds[name], value, name)


if __name__ == "__main__":
    unittest.main()
