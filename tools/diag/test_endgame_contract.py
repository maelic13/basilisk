#!/usr/bin/env python3
"""Known-bad tests for the endgame truth contract and its consumers.

Each guard is shown to fail on a bad input and to pass a good one:
* a pre-fix v1 report, and a v1 report that cannot be shown post-fix;
* a changed cohort FEN, and a stored digest that does not describe its records;
* a candidate family the baseline lacks, and a subset that is comparable;
* a thin family, which may not block, and a real regression, which must;
* a zero-variance rate, which may not yield an infinite sigma;
* no default baseline, and `material_lost` no longer a known outcome;
* a sharded run against the serial one, with an engine that keeps state across
  `ucinewgame` (caught) and one that resets (identical). These run the real
  runner, so they need the local Syzygy set (BASILISK_SYZYGY, or the default
  path) and are skipped without it.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import endgame_compare  # noqa: E402
import endgame_contract as contract  # noqa: E402
from endgame_vetoes import KNOWN_OUTCOMES, malformed_record  # noqa: E402

ROOT = HERE.parents[1]
SYZYGY = os.environ.get("BASILISK_SYZYGY", "D:/chess/tablebases/syzygy3456")
COHORT = HERE / "endgame_cohort_v1.manifest.json"

FENS = [
    "8/8/8/4k3/8/8/8/4KQ2 w - - 0 1",
    "8/8/8/3k4/8/8/8/3KQ3 w - - 0 1",
    "8/8/2k5/8/8/8/8/4KQ2 w - - 0 1",
    "8/8/8/8/4k3/8/8/4KQ2 w - - 0 1",
    "8/8/8/8/3k4/8/8/2K1Q3 w - - 0 1",
    "8/8/8/8/8/5k2/8/1K1Q4 w - - 0 1",
    "8/8/8/5k2/8/8/8/1K2Q3 w - - 0 1",
    "8/8/8/8/2k5/8/8/1K3Q2 w - - 0 1",
    "8/8/6k1/8/8/8/8/1K3Q2 w - - 0 1",
    "8/8/8/8/6k1/8/8/1K3Q2 w - - 0 1",
    "8/8/8/8/8/2k5/8/1K4Q1 w - - 0 1",
    "8/8/1k6/8/8/8/8/1K4Q1 w - - 0 1",
]


def position(i: int, *, mated: bool = True, preserved: int = 4, moves: str = "e1e2") -> dict:
    return {
        "index": i,
        "id": f"EG{i:04d}",
        "fen": FENS[i],
        "theory_wdl": 2,
        "theory_dtz": 9,
        "outcome": "mated" if mated else "ply_limit",
        "plies": 9,
        "graded_moves": 4,
        "win_preserving_moves": preserved,
        "dtz_checked_moves": preserved,
        "dtz_progress_moves": preserved // 2,
        "first_discard_ply": None if preserved == 4 else 3,
        "shed_material_ply": None,
        "anomaly": None,
        "moves": moves,
    }


def report(families: dict[str, list[dict]], schema: str = contract.SCHEMA_V2) -> dict:
    out = {
        "schema": schema,
        "nodes_per_move": 60000, "max_plies": 100, "hash_mb": 16,
        "persistent_tt_per_game": True, "score_adjudication": False,
        "cohort": {"manifest_sha256": "M", "book_sha256": "B",
                   "position_count": sum(len(r) for r in families.values())},
        "families": {},
    }
    for name, records in families.items():
        outcomes = {}
        for r in records:
            outcomes[r["outcome"]] = outcomes.get(r["outcome"], 0) + 1
        out["families"][name] = {"outcomes": outcomes, "positions": records,
                                 "cohort_sha256": contract.records_digest(records)}
    return out


class Files(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def write(self, name: str, data: dict) -> Path:
        path = self.dir / name
        path.write_text(json.dumps(data), encoding="utf-8")
        return path

    def load(self, data: dict) -> dict:
        return endgame_compare.load_report(self.write("r.json", data))


class Digest(unittest.TestCase):
    def test_digest_follows_fen_order_and_id(self) -> None:
        a = contract.family_digest([("A", FENS[0]), ("B", FENS[1])])
        self.assertEqual(a, contract.family_digest([("A", FENS[0]), ("B", FENS[1])]))
        self.assertNotEqual(a, contract.family_digest([("B", FENS[1]), ("A", FENS[0])]))
        self.assertNotEqual(a, contract.family_digest([("A", FENS[0]), ("B", FENS[2])]))
        self.assertNotEqual(a, contract.family_digest([("A", FENS[0]), ("C", FENS[1])]))

    def test_overall_digest_folds_names(self) -> None:
        self.assertNotEqual(contract.overall_digest({"KQ-K": "00"}),
                            contract.overall_digest({"KR-K": "00"}))


class Contract(Files):
    def test_v2_is_accepted(self) -> None:
        self.assertEqual(self.load(report({"KQ-K": [position(0)]}))["_contract"], "v2")

    def test_postfix_v1_is_accepted(self) -> None:
        data = report({"KQ-K": [position(0)]}, contract.SCHEMA_V1)
        self.assertEqual(self.load(data)["_contract"], "v1-postfix")

    def test_prefix_v1_material_lost_is_refused(self) -> None:
        data = report({"KQ-K": [position(0)]}, contract.SCHEMA_V1)
        data["families"]["KQ-K"]["outcomes"] = {"material_lost": 1}
        with self.assertRaisesRegex(ValueError, "pre-fix"):
            self.load(data)

    def test_v1_without_the_shed_field_is_refused(self) -> None:
        record = position(0)
        del record["shed_material_ply"]
        with self.assertRaisesRegex(ValueError, "pre-fix"):
            self.load(report({"KQ-K": [record]}, contract.SCHEMA_V1))

    def test_unknown_schema_is_refused(self) -> None:
        with self.assertRaisesRegex(ValueError, "unsupported schema"):
            self.load(report({"KQ-K": [position(0)]}, "basilisk-endgame-truth-v9"))

    def test_a_stored_digest_must_describe_its_records(self) -> None:
        data = report({"KQ-K": [position(0)]})
        data["families"]["KQ-K"]["positions"][0]["fen"] = FENS[1]
        with self.assertRaisesRegex(ValueError, "does not describe"):
            self.load(data)


class Pairing(Files):
    def pair(self, base: dict, cand: dict):
        return endgame_compare.paired_families(
            endgame_compare.load_report(self.write("b.json", base)),
            endgame_compare.load_report(self.write("c.json", cand)))

    def test_a_subset_is_comparable(self) -> None:
        base = report({"KQ-K": [position(0)], "KR-K": [position(1)]})
        paired = self.pair(base, report({"KR-K": [position(1)]}))
        self.assertEqual(list(paired), ["KR-K"])

    def test_a_changed_fen_is_refused(self) -> None:
        base = report({"KQ-K": [position(0), position(1)]})
        moved = position(1)
        moved["fen"] = FENS[2]
        with self.assertRaisesRegex(ValueError, "cohort digests differ"):
            self.pair(base, report({"KQ-K": [position(0), moved]}))

    def test_a_family_the_baseline_lacks_is_refused(self) -> None:
        with self.assertRaisesRegex(ValueError, "absent from the baseline"):
            self.pair(report({"KQ-K": [position(0)]}), report({"KR-K": [position(0)]}))


def pairs(before: list[dict], after: list[dict]):
    return [(r["id"], a, b) for r, a, b in zip(before, before, after)]


class Statistics(unittest.TestCase):
    def conversion(self, lost: int, n: int = 12) -> str:
        base = [position(i) for i in range(n)]
        cand = [position(i, mated=i >= lost) for i in range(n)]
        stats = endgame_compare.paired_stats(pairs(base, cand), "conversion")
        return endgame_compare.classify(stats, family=True)

    def test_three_lost_conversions_do_not_block(self) -> None:
        # Before the repair this read -infinity sigma and blocked.
        self.assertEqual(self.conversion(3, n=3), "indeterminate")

    def test_five_discordant_are_indeterminate(self) -> None:
        self.assertEqual(self.conversion(5), "indeterminate")

    def test_six_lost_report(self) -> None:
        self.assertEqual(self.conversion(6), "report_regression")  # z = -2.45

    def test_ten_lost_block(self) -> None:
        self.assertEqual(self.conversion(10), "block")  # z = -3.16

    def test_zero_variance_rate_is_indeterminate_not_infinite(self) -> None:
        base = [position(i) for i in range(8)]
        cand = [position(i, preserved=3) for i in range(8)]
        stats = endgame_compare.paired_stats(pairs(base, cand), "win_preservation")
        self.assertIsNone(stats["sigma"])
        self.assertEqual(stats["indeterminate"], "zero variance")
        self.assertEqual(endgame_compare.classify(stats, family=True), "indeterminate")

    def test_an_identical_run_passes(self) -> None:
        base = [position(i) for i in range(12)]
        stats = endgame_compare.paired_stats(pairs(base, base), "conversion")
        self.assertEqual(endgame_compare.classify(stats, family=True), "indeterminate")
        self.assertEqual(stats["discordant"], 0)


class Commands(unittest.TestCase):
    def run_tool(self, *args: str) -> int:
        return subprocess.run([sys.executable, *args], capture_output=True).returncode

    def test_vetoes_need_a_named_baseline(self) -> None:
        self.assertEqual(self.run_tool(str(HERE / "endgame_vetoes.py"), "--candidate", "x.json"), 2)

    def test_compare_needs_a_named_baseline(self) -> None:
        self.assertEqual(self.run_tool(str(HERE / "endgame_compare.py"), "--candidate", "x.json"), 2)

    def test_material_lost_is_no_longer_an_outcome(self) -> None:
        self.assertNotIn("material_lost", KNOWN_OUTCOMES)
        record = position(0)
        record["outcome"] = "material_lost"
        self.assertEqual(malformed_record(record), "unknown_outcome=material_lost")


FAKE_ENGINE = r'''
import sys
import chess

KEEP_STATE = sys.argv[1] == "keep"
board = chess.Board()
searches = 0
for line in sys.stdin:
    words = line.split()
    if not words:
        continue
    if words[0] == "uci":
        print("id name fake-" + sys.argv[1])
        print("option name Hash type spin default 16 min 1 max 64")
        print("uciok", flush=True)
    elif words[0] == "isready":
        print("readyok", flush=True)
    elif words[0] == "ucinewgame":
        if not KEEP_STATE:
            searches = 0
    elif words[0] == "position":
        board = chess.Board(" ".join(words[2:8])) if words[1] == "fen" else chess.Board()
        if "moves" in words:
            for move in words[words.index("moves") + 1:]:
                board.push_uci(move)
    elif words[0] == "go":
        legal = sorted(m.uci() for m in board.legal_moves)
        print("bestmove " + legal[searches % len(legal)], flush=True)
        searches += 1
    elif words[0] == "quit":
        break
'''


def fake_engine(directory: Path, mode: str) -> Path:
    """A launchable fake UCI engine (a batch file on Windows, a script elsewhere)."""
    script = directory / "fake_engine.py"
    script.write_text(FAKE_ENGINE, encoding="utf-8")
    if os.name == "nt":
        launcher = directory / f"fake_{mode}.cmd"
        launcher.write_text(f'@"{sys.executable}" "{script}" {mode}\r\n', encoding="utf-8")
    else:
        launcher = directory / f"fake_{mode}"
        launcher.write_text(f'#!/bin/sh\nexec "{sys.executable}" "{script}" {mode}\n',
                            encoding="utf-8")
        launcher.chmod(0o755)
    return launcher


@unittest.skipUnless(Path(SYZYGY).is_dir(), "local Syzygy tables not found")
class ShardedAgainstSerial(Files):
    def run_truth(self, engine: Path, workers: int, name: str) -> Path:
        out = self.dir / name
        done = subprocess.run(
            [sys.executable, str(HERE / "endgame_truth.py"), "--engine", str(engine),
             "--syzygy", SYZYGY, "--cohort", str(COHORT), "--families", "KQ-K,KR-K",
             "--cohort-limit-per-family", "3", "--nodes", "1", "--max-plies", "8",
             "--workers", str(workers), "--output", str(out)],
            capture_output=True, text=True)
        self.assertEqual(done.returncode, 0, done.stderr[-2000:])
        return out

    def differences(self, mode: str) -> list[str]:
        engine = fake_engine(self.dir, mode)
        serial = contract.load_truth_report(self.run_truth(engine, 1, f"{mode}-1.json"))
        sharded = contract.load_truth_report(self.run_truth(engine, 3, f"{mode}-3.json"))
        self.assertEqual(serial["_contract"], "v2")
        self.assertEqual(serial["layers"], contract.LAYERS)
        self.assertEqual(serial["cohort_identity"]["family_sha256"],
                         serial["_family_digests"])
        return contract.identity_differences(serial, sharded)

    def test_an_engine_that_resets_reproduces_serially(self) -> None:
        self.assertEqual(self.differences("reset"), [])

    def test_an_engine_that_keeps_state_is_caught(self) -> None:
        self.assertTrue(self.differences("keep"))


if __name__ == "__main__":
    unittest.main()
