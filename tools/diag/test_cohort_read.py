import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import cohort_read  # noqa: E402

BASE = 1_900_000.0
OFFSETS = [1.0000, 1.0015, 0.9988, 1.0006]


def config(engines=14, mode="gain", arms=("a", "b")):
    return {
        "schema": cohort_read.SCHEMA, "label": "t", "engines": engines, "foreign_cpus": 1.6,
        "cpus": list(range(4, 32, 2))[:engines] if engines > 1 else [30],
        "arms": [{"name": n, "builds": [{"sha256": f"{n}{i}"} for i in range(4)]} for n in arms],
        "contrasts": [{"name": "read", "base": arms[0], "cand": arms[1], "mode": mode}],
    }


def records(cfg, delta=0.0, wobble=None, foreign=None, tree=None):
    out = []
    for cycle in range(1, cohort_read.CYCLES + 1):
        for p in cohort_read.cycle_plan(cfg, cycle):
            scale = 1.0 + (delta / 100.0 if p["arm"] == cfg["arms"][1]["name"] else 0.0)
            level = (wobble or {}).get((p["arm"], cycle), 1.0)
            out.append({"kind": "reading", "cycle": cycle, **p, "sha256": f"{p['arm']}{p['build']}",
                        "nps": BASE * OFFSETS[p["build"]] * scale * level, "nodes": 30_000_000,
                        "tree": (tree or {}).get(p["arm"], "t0"), "early_ends": 0})
        out.append({"kind": "cycle", "cycle": cycle, "foreign_cpus": (foreign or {}).get(cycle, 0.2),
                    "busy_processes": []})
    return out


class PlanTest(unittest.TestCase):
    def test_every_build_every_cycle_and_cores_swap(self):
        cfg = config()
        for cycle in range(1, 7):
            plan = cohort_read.cycle_plan(cfg, cycle)
            self.assertEqual(len(plan), 14)
            self.assertEqual(len({p["cpu"] for p in plan}), 14)
            for arm in ("a", "b"):
                mine = [p for p in plan if p["arm"] == arm]
                self.assertEqual(len(mine), 7)
                self.assertEqual({p["build"] for p in mine}, {0, 1, 2, 3})
            nxt = cohort_read.cycle_plan(cfg, cycle + 1)
            for p, q in zip(plan, nxt):
                self.assertEqual(p["cpu"], q["cpu"])
                self.assertNotEqual(p["arm"], q["arm"])

    def test_single_engine_reads_each_build_once(self):
        cfg = config(engines=1, arms=("a", "b", "c"))
        plan = cohort_read.cycle_plan(cfg, 1)
        self.assertEqual(len(plan), 12)
        self.assertEqual(sorted((p["arm"], p["build"]) for p in plan),
                         sorted((a, i) for a in "abc" for i in range(4)))


class DecisionTest(unittest.TestCase):
    def test_null_admitted_at_zero(self):
        cfg = config(mode="null")
        report = cohort_read.analyse(cfg, records(cfg))
        self.assertEqual(report["contrasts"][0]["verdict"], "ADMITTED")

    def test_null_refused_off_zero(self):
        cfg = config(mode="null")
        for delta in (-1.5, 1.5):
            report = cohort_read.analyse(cfg, records(cfg, delta=delta))
            self.assertEqual(report["contrasts"][0]["verdict"], "NOT_ADMITTED", delta)

    def test_gain_rules(self):
        cfg = config(mode="gain")
        big = cohort_read.analyse(cfg, records(cfg, delta=2.0))["contrasts"][0]
        self.assertEqual((big["verdict"], big["loaded_rule"]), ("ACCEPT", "MET"))
        small = cohort_read.analyse(cfg, records(cfg, delta=0.7))["contrasts"][0]
        self.assertEqual((small["verdict"], small["loaded_rule"]), ("ACCEPT", "NOT_MET"))
        loss = cohort_read.analyse(cfg, records(cfg, delta=-2.0))["contrasts"][0]
        self.assertEqual((loss["verdict"], loss["loaded_rule"]), ("NO_CHANGE", "NOT_MET"))
        self.assertAlmostEqual(big["delta_percent"], 2.0, places=6)

    def test_no_regression(self):
        cfg = config(mode="no-regression")
        self.assertEqual(cohort_read.analyse(cfg, records(cfg, delta=-2.0))["contrasts"][0]["verdict"], "FAIL")
        self.assertEqual(cohort_read.analyse(cfg, records(cfg, delta=0.0))["contrasts"][0]["verdict"], "PASS")

    def test_disturbance(self):
        cfg = config(mode="null")
        two = {("a", 2): 1.03, ("a", 5): 0.97}
        self.assertEqual(cohort_read.analyse(cfg, records(cfg, wobble=two))["contrasts"][0]["verdict"], "ADMITTED")
        three = {**two, ("b", 1): 1.02, ("b", 3): 1.02, ("b", 4): 0.98}
        self.assertEqual(cohort_read.analyse(cfg, records(cfg, wobble=three))["contrasts"][0]["verdict"],
                         "REPEAT_READ")
        host = cohort_read.analyse(cfg, records(cfg, foreign={1: 3.0, 2: 2.0, 3: 1.7}))
        self.assertEqual(host["host_disturbed_cycles"], [1, 2, 3])
        self.assertEqual(host["contrasts"][0]["verdict"], "REPEAT_READ")

    def test_tree_mismatch_is_reported(self):
        cfg = config()
        report = cohort_read.analyse(cfg, records(cfg, tree={"b": "t1"}))
        self.assertIn("TREES DIFFER", cohort_read.render(report))

    def test_wrong_binary_refused(self):
        cfg = config()
        recs = records(cfg)
        recs[0]["sha256"] = "zz"
        with self.assertRaises(cohort_read.nps_read.ArchiveError):
            cohort_read.analyse(cfg, recs)


class CountTest(unittest.TestCase):
    def test_counted_nodes(self):
        limit = ["info depth 13 seldepth 20 score cp 30 nodes 66681 nps 1 time 2 pv e2e4\n", "bestmove e2e4\n"]
        self.assertEqual(cohort_read.counted_nodes(limit, 150000, 100), (150000, False))
        mate = ["info depth 3 score mate 1 nodes 812 nps 1 time 1 pv h5f7\n", "bestmove h5f7\n"]
        self.assertEqual(cohort_read.counted_nodes(mate, 150000, 100), (812, True))
        cap = ["info depth 100 score cp 0 nodes 9000 nps 1 time 1 pv a1a2\n", "bestmove a1a2\n"]
        self.assertEqual(cohort_read.counted_nodes(cap, 150000, 100), (9000, True))


if __name__ == "__main__":
    unittest.main()
