import copy
import hashlib
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import colosseum_parity


class ParityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        root = pathlib.Path(self.temp.name)
        self.a, self.b, self.book = root / "a.exe", root / "b.exe", root / "book.epd"
        self.a.write_bytes(b"a")
        self.b.write_bytes(b"b")
        self.book.write_bytes(b"book")
        digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest().upper()
        self.manifest = {
            "engineA": f"A = {self.a}", "engineA_sha256": digest(self.a),
            "engineB": f"B = {self.b}", "engineB_sha256": digest(self.b),
            "test_design": "SPRT elo0=0 elo1=3 alpha=0.05 beta=0.05 model=normalized",
            "game_budget": "200", "time_control": "tc=3+0.03 (clock) timemargin=20ms",
            "optionsA": "(none)", "optionsB": "(none)", "hash_mb": "64", "threads": "1",
            "book": str(self.book), "book_sha256": digest(self.book), "opening_order": "random",
            "opening_seed": "7", "concurrency": "2", "affinity_cpus": "2,4",
            "adjudication": "none (natural termination)",
        }
        control = {"control": {"Increment": {"base_ms": 3000, "inc_ms": 30}}, "margin_ms": 20}
        allocation = lambda cpu: {"allocation": {"cpus": [{"number": cpu}, {"number": cpu + 1}]}}
        self.dry = {"resolved_configuration": {
            "design": {"parameters": {"model": "normalized", "elo0": 0, "elo1": 3,
                                         "alpha": 0.05, "beta": 0.05}, "max_pairs": 100},
            "engine_a_time_control": copy.deepcopy(control), "engine_b_time_control": copy.deepcopy(control),
            "adjudication": {"draw": None, "resign": None, "max_moves": None},
            "engine_a": {"executable": str(self.a), "options": {"Hash": {"value": 64}, "Threads": {"value": 1}}},
            "engine_b": {"executable": str(self.b), "options": {"Hash": {"value": 64}, "Threads": {"value": 1}}},
            "openings": {"path": str(self.book), "order": "Random"}, "master_seed": 7,
            "execution": {"concurrency": 2, "slots": [
                {"engine_a": allocation(2), "engine_b": allocation(2)},
                {"engine_a": allocation(4), "engine_b": allocation(4)}]},
            "engine_processes": "per-slot",
        }}

    def tearDown(self):
        self.temp.cleanup()

    def rows(self, dry=None):
        return {row["field"]: row for row in colosseum_parity.compare(self.manifest, dry or self.dry)}

    def test_all_comparable_fields_agree(self):
        rows = self.rows()
        self.assertGreaterEqual(len(rows), 25)
        self.assertTrue(all(row["equal"] for row in rows.values()))

    def test_each_comparison_class_is_live(self):
        mutations = {
            "sprt.elo1": lambda d: d["resolved_configuration"]["design"]["parameters"].update(elo1=10),
            "game_budget": lambda d: d["resolved_configuration"]["design"].update(max_pairs=99),
            "engine_a_time_control.base_ms": lambda d: d["resolved_configuration"]["engine_a_time_control"]["control"]["Increment"].update(base_ms=10000),
            "adjudication": lambda d: d["resolved_configuration"]["adjudication"].update(resign={}),
            "engineA.Hash": lambda d: d["resolved_configuration"]["engine_a"]["options"]["Hash"].update(value=128),
            "opening_order": lambda d: d["resolved_configuration"]["openings"].update(order="Sequential"),
            "concurrency": lambda d: d["resolved_configuration"]["execution"].update(concurrency=3),
            "affinity_cpus": lambda d: d["resolved_configuration"]["execution"]["slots"][0]["engine_a"]["allocation"]["cpus"].append({"number": 6}),
        }
        for field, mutate in mutations.items():
            with self.subTest(field=field):
                changed = copy.deepcopy(self.dry)
                mutate(changed)
                self.assertFalse(self.rows(changed)[field]["equal"])


if __name__ == "__main__":
    unittest.main()
