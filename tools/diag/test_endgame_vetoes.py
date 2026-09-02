#!/usr/bin/env python3
"""Focused unit tests for the PLAN 6.0.e hard-veto policy."""

from __future__ import annotations

import unittest
from types import SimpleNamespace

import chess
import chess.engine

from endgame_truth import play_and_grade
from endgame_vetoes import evaluate_vetoes


def record(
    *,
    outcome: str = "mated",
    theory_wdl: int = 2,
    graded: int = 4,
    preserved: int = 4,
    first_discard=None,
) -> dict:
    return {
        "outcome": outcome,
        "theory_wdl": theory_wdl,
        "plies": 8,
        "graded_moves": graded,
        "win_preserving_moves": preserved,
        "dtz_checked_moves": preserved,
        "dtz_progress_moves": preserved // 2,
        "first_discard_ply": first_discard,
    }


def evaluate(baseline: dict, candidate: dict) -> dict[str, list[str]]:
    return evaluate_vetoes({"KBN-K": [("EG0001", baseline, candidate)]})


class EndgameVetoTests(unittest.TestCase):
    def assert_passes(self, vetoes: dict[str, list[str]]) -> None:
        self.assertFalse(any(vetoes.values()), vetoes)

    def test_identical_clean_result_passes(self) -> None:
        self.assert_passes(evaluate(record(), record()))

    def test_new_clean_win_discard_blocks(self) -> None:
        vetoes = evaluate(
            record(), record(outcome="ply_limit", preserved=3, first_discard=6)
        )
        self.assertEqual(vetoes["new_clean_win_discard"], ["KBN-K/EG0001"])

    def test_existing_discard_is_grandfathered(self) -> None:
        self.assert_passes(
            evaluate(
                record(outcome="ply_limit", preserved=3, first_discard=6),
                record(outcome="ply_limit", preserved=2, first_discard=2),
            )
        )

    def test_new_rule50_failure_blocks(self) -> None:
        vetoes = evaluate(record(), record(outcome="fifty_move"))
        self.assertEqual(vetoes["new_rule50_failure"], ["KBN-K/EG0001"])

    def test_engine_anomalies_are_absolute_on_draws(self) -> None:
        for outcome in ("engine_crash", "engine_error", "illegal_move", "no_move"):
            with self.subTest(outcome=outcome):
                vetoes = evaluate(
                    record(theory_wdl=0),
                    record(theory_wdl=0, outcome=outcome),
                )
                self.assertEqual(
                    vetoes["engine_anomaly"], [f"KBN-K/EG0001:{outcome}"]
                )

    def test_malformed_record_fails_closed(self) -> None:
        candidate = record()
        del candidate["outcome"]
        vetoes = evaluate(record(), candidate)
        self.assertEqual(
            vetoes["malformed_record"], ["KBN-K/EG0001:missing=outcome"]
        )


class FakeTablebase:
    def probe_wdl(self, board) -> int:
        return 0

    def probe_dtz(self, board) -> int:
        return 0


class IllegalEngine:
    def play(self, board, limit, game):
        return SimpleNamespace(move=chess.Move.from_uci("a1a8"))


class CrashedEngine:
    def play(self, board, limit, game):
        raise chess.engine.EngineTerminatedError("synthetic crash")


class EndgameRunnerAnomalyTests(unittest.TestCase):
    def test_illegal_move_is_recorded(self) -> None:
        result = play_and_grade(
            IllegalEngine(), FakeTablebase(), chess.Board(), 1, 1, object()
        )
        self.assertEqual(result["outcome"], "illegal_move")
        self.assertEqual(result["anomaly"], {"move": "a1a8"})

    def test_engine_crash_is_recorded(self) -> None:
        result = play_and_grade(
            CrashedEngine(), FakeTablebase(), chess.Board(), 1, 1, object()
        )
        self.assertEqual(result["outcome"], "engine_crash")
        self.assertEqual(result["anomaly"]["type"], "EngineTerminatedError")


if __name__ == "__main__":
    unittest.main()
