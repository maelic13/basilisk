import contextlib
import io
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import nps_read

BASE = 4_000_000.0
# Per-build offsets of one source: about the 0.1-0.3% spread PGO luck gives.
OFFSETS = [1.0000, 1.0012, 0.9990, 1.0005]
# A small, deterministic per-cycle wobble shared by both arms.
WOBBLE = [1.0000, 0.9996, 1.0003, 0.9998, 1.0002, 0.9999]


def sha(arm, index):
    return f"{arm}{index}".ljust(64, "0")


def archive(delta=0.0, cycles=2, mode="gain", spread=1.0, cycle_scale=None, busy=None, extra=()):
    """Readings for two four-build arms; cand is base scaled by (1 + delta%)."""
    config = {
        "schema": nps_read.SCHEMA, "label": "t", "mode": mode, "busy_ceiling_percent": 5.0,
        "base": {"builds": [{"sha256": sha("b", i)} for i in range(4)]},
        "cand": {"builds": [{"sha256": sha("c", i)} for i in range(4)]},
    }
    records = []
    for cycle in range(1, cycles + 1):
        records.append({"kind": "cycle", "cycle": cycle, "busy_percent": (busy or {}).get(cycle, 1.0),
                        "busy_processes": []})
        for arm, factor in (("base", 1.0), ("cand", 1.0 + delta / 100.0)):
            scale = (cycle_scale or {}).get((arm, cycle), 1.0)
            for i, offset in enumerate(OFFSETS):
                spread_offset = 1.0 + (offset - 1.0) * spread
                nps = BASE * factor * spread_offset * WOBBLE[cycle - 1] * scale
                records.append({"kind": "reading", "arm": arm, "build": i, "cycle": cycle,
                                "sha256": sha(arm[0], i), "nodes": 8437148,
                                "nps": [nps * 0.98, nps, nps * 0.99]})
    records.extend(extra)
    return config, records


class QuantileTests(unittest.TestCase):
    def test_t_quantiles_match_tables(self):
        self.assertAlmostEqual(nps_read.t_quantile(0.975, 1), 12.7062, places=3)
        self.assertAlmostEqual(nps_read.t_quantile(0.975, 6), 2.4469, places=3)
        self.assertAlmostEqual(nps_read.t_quantile(0.975, 1e6), 1.9600, places=3)

    def test_welch_interval_by_hand(self):
        cand, base = [101.0, 103.0, 102.0], [100.0, 99.0, 101.0]
        delta, lower, upper, df = nps_read.welch_interval(cand, base)
        self.assertAlmostEqual(delta, 2.0)
        self.assertAlmostEqual(df, 4.0)
        half = nps_read.t_quantile(0.975, 4.0) * (2.0 / 3.0) ** 0.5
        self.assertAlmostEqual(lower, 2.0 - half, places=6)
        self.assertAlmostEqual(upper, 2.0 + half, places=6)


class DecisionTests(unittest.TestCase):
    def verdict(self, *args, **kwargs):
        return nps_read.analyse(*archive(*args, **kwargs))

    def test_estimate_is_the_delta_of_build_medians(self):
        report = self.verdict(delta=1.0)
        self.assertAlmostEqual(report["delta_percent"], 1.0, places=6)
        self.assertEqual(report["fingerprints"], {"base": [8437148], "cand": [8437148]})

    def test_a_reading_is_the_best_of_its_runs(self):
        report = self.verdict(delta=0.0)
        expected = sum(BASE * o * (WOBBLE[0] + WOBBLE[1]) / 2 for o in OFFSETS) / len(OFFSETS)
        self.assertAlmostEqual(report["arm_mean"]["base"], expected, delta=0.01)

    def test_a_build_is_the_median_of_its_cycles(self):
        config, records = archive(delta=0.6, cycles=6)
        outlier = next(r for r in records if r.get("kind") == "reading" and r["arm"] == "cand"
                       and r["build"] == 0 and r["cycle"] == 3)
        outlier["nps"] = [v * 1.05 for v in outlier["nps"]]
        report = nps_read.analyse(config, records)
        self.assertAlmostEqual(report["delta_percent"], 0.6, delta=0.01)

    def test_step_one_gain(self):
        # Each boundary is read just either side of it.
        self.assertEqual(self.verdict(delta=0.95)["verdict"], "ACCEPT")
        self.assertEqual(self.verdict(delta=0.85)["verdict"], "EXTEND")
        self.assertEqual(self.verdict(delta=0.15)["verdict"], "EXTEND")
        self.assertEqual(self.verdict(delta=0.05)["verdict"], "NO_CHANGE")
        self.assertEqual(self.verdict(delta=-2.0)["verdict"], "NO_CHANGE")

    def test_step_one_no_regression(self):
        self.assertEqual(self.verdict(delta=-0.05, mode="no-regression")["verdict"], "PASS")
        self.assertEqual(self.verdict(delta=-0.15, mode="no-regression")["verdict"], "EXTEND")
        self.assertEqual(self.verdict(delta=-0.85, mode="no-regression")["verdict"], "EXTEND")
        self.assertEqual(self.verdict(delta=-0.95, mode="no-regression")["verdict"], "FAIL")

    def test_step_one_cycles_must_agree(self):
        report = self.verdict(delta=1.0, cycle_scale={("cand", 2): 1.015})
        self.assertEqual(report["verdict"], "REPEAT_STEP_1")
        self.assertEqual(report["disturbed_cycles"]["cand"], [1, 2])
        self.assertEqual(self.verdict(delta=1.0, cycle_scale={("cand", 2): 1.008})["verdict"], "ACCEPT")

    def test_host_load_disturbs_a_cycle(self):
        report = self.verdict(delta=1.0, busy={2: 12.0})
        self.assertEqual(report["verdict"], "REPEAT_STEP_1")
        self.assertEqual(report["host_disturbed_cycles"], [2])

    def test_step_two_gain(self):
        self.assertEqual(self.verdict(delta=0.55, cycles=6)["verdict"], "ACCEPT")
        self.assertEqual(self.verdict(delta=0.45, cycles=6)["verdict"], "NO_CHANGE")
        wide = self.verdict(delta=0.6, cycles=6, spread=8.0)
        self.assertLessEqual(wide["interval_percent"][0], 0.0)
        self.assertEqual(wide["verdict"], "NO_CHANGE")

    def test_step_two_no_regression(self):
        self.assertEqual(self.verdict(delta=-0.6, cycles=6, mode="no-regression")["verdict"], "FAIL")
        wide = self.verdict(delta=-0.6, cycles=6, mode="no-regression", spread=8.0)
        self.assertGreaterEqual(wide["interval_percent"][1], 0.0)
        self.assertEqual(wide["verdict"], "PASS")
        self.assertEqual(self.verdict(delta=-0.3, cycles=6, mode="no-regression")["verdict"], "PASS")

    def test_six_cycles_absorb_two_disturbed(self):
        two = {("base", 2): 1.03, ("base", 5): 0.97}
        report = self.verdict(delta=0.6, cycles=6, cycle_scale=two)
        self.assertEqual(report["disturbed_cycles"]["base"], [2, 5])
        self.assertEqual(report["verdict"], "ACCEPT")
        three = {**two, ("base", 6): 1.03}
        self.assertEqual(self.verdict(delta=0.6, cycles=6, cycle_scale=three)["verdict"], "REPEAT_READ")

    def test_other_cycle_counts_decide_nothing(self):
        self.assertEqual(self.verdict(delta=1.0, cycles=3)["verdict"], "NONSTANDARD")

    def test_null_reads_no_change(self):
        report = self.verdict(delta=0.0, cycles=6)
        self.assertEqual(report["verdict"], "NO_CHANGE")
        self.assertLess(report["interval_percent"][0], 0.0)
        self.assertGreater(report["interval_percent"][1], 0.0)


class ArchiveTests(unittest.TestCase):
    def test_a_missing_reading_is_refused(self):
        config, records = archive()
        records = [r for r in records if not (r.get("kind") == "reading" and r["arm"] == "cand"
                                               and r["build"] == 3 and r["cycle"] == 2)]
        with self.assertRaisesRegex(nps_read.ArchiveError, "no reading in cycle 2"):
            nps_read.analyse(config, records)

    def test_a_reading_from_another_binary_is_refused(self):
        config, records = archive()
        next(r for r in records if r.get("kind") == "reading")["sha256"] = "f" * 64
        with self.assertRaisesRegex(nps_read.ArchiveError, "not its pool's binary"):
            nps_read.analyse(config, records)

    def test_a_duplicate_reading_is_refused(self):
        config, records = archive()
        records.append(dict(next(r for r in records if r.get("kind") == "reading")))
        with self.assertRaisesRegex(nps_read.ArchiveError, "two readings"):
            nps_read.analyse(config, records)

    def test_cli_reports_and_refuses(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            config, records = archive(delta=1.0)
            (root / "read.json").write_text(json.dumps(config), encoding="utf-8")
            (root / "runs.jsonl").write_text("\n".join(json.dumps(r) for r in records) + "\n", encoding="utf-8")
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                code = nps_read.main(["--report", str(root), "--json", str(root / "verdict.json")])
            self.assertEqual(code, 0)
            self.assertIn("verdict: ACCEPT", out.getvalue())
            self.assertEqual(json.loads((root / "verdict.json").read_text())["verdict"], "ACCEPT")
            (root / "read.json").write_text(json.dumps(dict(config, schema="other")), encoding="utf-8")
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(nps_read.main(["--report", str(root)]), 2)


if __name__ == "__main__":
    unittest.main()
