#!/usr/bin/env python3
"""Tests for nodes_per_move.py and endgame_budget_bracket.py.

The bracket rule and the veto rule are registered (BAS-D23; C.2.7.1's audit,
§4), so each is tested on the inputs that decide it, and each refusal on a
known-bad input: a misnamed budget, a changed cohort, a duplicate budget.
"""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from types import SimpleNamespace

import chess

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import endgame_budget_bracket as bracket  # noqa: E402
import nodes_per_move as npm  # noqa: E402
from test_endgame_contract import FENS, position, report  # noqa: E402


class TimeControl(unittest.TestCase):
    def test_parse(self) -> None:
        self.assertEqual(npm.parse_tc("3+0.03"), (3.0, 0.03))
        for bad in ("3", "0+0.1", "3+-1"):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                npm.parse_tc(bad)


class BracketRule(unittest.TestCase):
    def test_round_to_two_significant_figures(self) -> None:
        self.assertEqual(npm.round_sig(137_412), 140_000)
        self.assertEqual(npm.round_sig(45_804), 46_000)
        self.assertEqual(npm.round_sig(1_234_567), 1_200_000)
        self.assertEqual(npm.round_sig(9), 9)

    def samples(self, endgame: int, other: int) -> list[dict]:
        return ([{"ply": 90, "men": 5, "nodes": 300_000} for _ in range(endgame)]
                + [{"ply": 10, "men": 30, "nodes": 90_000} for _ in range(other)])

    def test_enough_endgame_moves_set_the_bracket(self) -> None:
        b = npm.bracket_from(self.samples(npm.MIN_ENDGAME_MOVES, 1000))
        self.assertEqual(b["median"], 300_000)
        self.assertEqual(b["budgets"], [100_000, 300_000, 900_000])
        self.assertIn("at most 6 men", b["source"])

    def test_too_few_endgame_moves_fall_back_to_every_move(self) -> None:
        b = npm.bracket_from(self.samples(npm.MIN_ENDGAME_MOVES - 1, 1000))
        self.assertEqual(b["median"], 90_000)
        self.assertEqual(b["budgets"], [30_000, 90_000, 270_000])
        self.assertIn("every move", b["source"])


class Book(unittest.TestCase):
    def test_sampling_is_seeded_and_legal(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            book = Path(tmp) / "book.epd"
            book.write_text("\n".join(FENS) + "\n", encoding="ascii")
            first = npm.sample_book(book, 20, 7)
            self.assertEqual(first, npm.sample_book(book, 20, 7))
            self.assertNotEqual(first, npm.sample_book(book, 20, 8))
            for fen in first:
                self.assertTrue(chess.Board(fen).is_valid())


class SlowEngine:
    def __init__(self, nodes: int, delay: float) -> None:
        self.nodes, self.delay = nodes, delay

    def play(self, board, limit, info, game):
        time.sleep(self.delay)
        return SimpleNamespace(move=next(iter(board.legal_moves)), info={"nodes": self.nodes})


class Games(unittest.TestCase):
    def test_a_game_records_nodes_and_men(self) -> None:
        game = npm.play_game([SlowEngine(1000, 0), SlowEngine(2000, 0)], FENS[0], 10, 0, 4)
        self.assertEqual([s["nodes"] for s in game["samples"]], [1000, 2000, 1000, 2000])
        self.assertEqual(game["samples"][0]["men"], 3)
        self.assertEqual(game["ending"], "max_plies")

    def test_an_overspent_clock_is_a_forfeit(self) -> None:
        game = npm.play_game([SlowEngine(1, 0.02), SlowEngine(1, 0.02)], FENS[0], 0.01, 0, 10)
        self.assertEqual(game["ending"], "forfeit")
        self.assertEqual(len(game["samples"]), 1)


class Budgets(unittest.TestCase):
    def test_parse(self) -> None:
        self.assertEqual(bracket.parse_budgets("360000,40000,120000"), [40000, 120000, 360000])
        for bad in ("", "40000,40000", "0,1"):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                bracket.parse_budgets(bad)


def vetoes(discard=(), anomaly=()):
    out = {kind: [] for kind in bracket.MAJORITY_KINDS + bracket.ANY_BUDGET_KINDS}
    out["new_clean_win_discard"] = list(discard)
    out["engine_anomaly"] = list(anomaly)
    return out


class MajorityRule(unittest.TestCase):
    def test_one_of_three_is_provisional(self) -> None:
        r = bracket.bracket_vetoes({1: vetoes(["KBN-K/EG1"]), 2: vetoes(), 3: vetoes()})
        self.assertEqual(r["count"], 0)
        self.assertEqual(r["provisional"]["new_clean_win_discard"], {"KBN-K/EG1": [1]})

    def test_two_of_three_vetoes(self) -> None:
        r = bracket.bracket_vetoes({1: vetoes(["KBN-K/EG1"]), 2: vetoes(), 3: vetoes(["KBN-K/EG1"])})
        self.assertEqual(r["vetoes"]["new_clean_win_discard"], {"KBN-K/EG1": [1, 3]})
        self.assertEqual(r["count"], 1)

    def test_an_anomaly_vetoes_at_one_budget(self) -> None:
        r = bracket.bracket_vetoes({1: vetoes(), 2: vetoes(anomaly=["KQ-K/EG2:no_move"]), 3: vetoes()})
        self.assertEqual(r["count"], 1)


class Arms(unittest.TestCase):
    BUDGETS = [40000, 120000, 360000]

    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def arm(self, name: str, lost_at=(), *, fens=None, nodes=None) -> Path:
        directory = self.dir / name
        directory.mkdir()
        for budget in self.BUDGETS:
            records = [position(i, mated=not (i == 0 and budget in lost_at),
                                preserved=3 if (i == 0 and budget in lost_at) else 4)
                       for i in range(3)]
            if fens and budget in fens:
                records[1]["fen"] = FENS[5]
            data = report({"KQ-K": records})
            data["families"]["KQ-K"]["theoretically_won"] = len(records)
            data["families"]["KQ-K"]["converted"] = sum(r["outcome"] == "mated" for r in records)
            data["nodes_per_move"] = nodes or budget
            (directory / f"truth-{budget}.json").write_text(json.dumps(data), encoding="utf-8")
        return directory

    def veto(self, base: Path, cand: Path) -> int:
        return subprocess.run(
            [sys.executable, str(HERE / "endgame_budget_bracket.py"), "veto",
             "--baseline-dir", str(base), "--candidate-dir", str(cand),
             "--budgets", ",".join(map(str, self.BUDGETS))],
            capture_output=True, text=True).returncode

    def test_a_discard_at_one_budget_passes(self) -> None:
        self.assertEqual(self.veto(self.arm("b"), self.arm("c", lost_at={40000})), 0)

    def test_a_discard_at_two_budgets_blocks(self) -> None:
        self.assertEqual(self.veto(self.arm("b"), self.arm("c", lost_at={40000, 360000})), 2)

    def test_a_misnamed_budget_is_refused(self) -> None:
        with self.assertRaisesRegex(ValueError, "measured at 60000"):
            bracket.load_arm(self.arm("c", nodes=60000), self.BUDGETS)

    def test_a_changed_cohort_is_refused(self) -> None:
        reports = bracket.load_arm(self.arm("c", fens={120000}), self.BUDGETS)
        self.assertIn("different position set", bracket.cohorts_agree(reports))

    def test_matching_cohorts_tabulate(self) -> None:
        reports = bracket.load_arm(self.arm("c"), self.BUDGETS)
        self.assertIsNone(bracket.cohorts_agree(reports))
        table = bracket.conversion_table(reports)
        self.assertEqual(set(table["KQ-K"]), {str(b) for b in self.BUDGETS})


if __name__ == "__main__":
    unittest.main()
