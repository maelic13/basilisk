#!/usr/bin/env python3
"""Tests for c210_table.py: the estimate, the meter, the clamped draws and the refusals."""

from __future__ import annotations

import json
import math
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import c210_table as table  # noqa: E402

import chess  # noqa: E402

WIN = '1. e4 {s=10 d=5 t=10ms h=0ms n=1000} e5 {s=-10 d=5 t=20ms h=0ms n=4000} {white wins} 1-0'
DRAW = '1. e4 {s=0 d=5 t=10ms h=0ms n=1000} e5 {s=0 d=5 t=20ms h=0ms n=4000} 1/2-1/2'


def game(number: int, pair: int, white: str, black: str, result: str, moves: str, fen: str = "") -> str:
    tags = [("White", white), ("Black", black), ("Result", result), ("GameNumber", str(number)),
            ("PairNumber", str(pair)), ("PairGame", str(2 - number % 2))]
    if fen:
        tags += [("SetUp", "1"), ("FEN", fen)]
    head = "\n".join(f'[{name} "{value}"]' for name, value in tags)
    return f"{head}\n\n{moves}\n"


def write_run(directory: pathlib.Path, games: list[str], standings_wdl: list[int]) -> None:
    directory.mkdir(parents=True)
    (directory / "games.pgn").write_text("\n".join(games), encoding="utf-8")
    (directory / "run-record.json").write_text(json.dumps({
        "progress": {"fields": [{"label": "players", "value": "A vs. B"}]}, "official_sample": {}}),
        encoding="utf-8")
    w, d, l = standings_wdl
    (directory / "result.json").write_text(json.dumps({"results": {"standings": [
        {"name": "A", "wins": w, "draws": d, "losses": l}, {"name": "B", "wins": l, "draws": d, "losses": w}]}}),
        encoding="utf-8")


def four_games() -> list[str]:
    # Pair 1: A wins as white, draws as black; pair 2: both drawn. A scores 2.5 of 4.
    return [game(1, 1, "A", "B", "1-0", WIN), game(2, 1, "B", "A", "1/2-1/2", DRAW),
            game(3, 2, "A", "B", "1/2-1/2", DRAW), game(4, 2, "B", "A", "1/2-1/2", DRAW)]


class Estimate(unittest.TestCase):
    def test_bas_o08_is_reproduced(self) -> None:
        row = table.interval([5, 35, 299, 612, 549])
        self.assertEqual(round(row["elo"], 1), 217.3)
        self.assertEqual((round(row["low"], 1), round(row["high"], 1)), (207.1, 228.0))


class BareKing(unittest.TestCase):
    def test_won_material_against_a_lone_king(self) -> None:
        self.assertEqual(table.bare_king_win(chess.Board("8/8/8/4k3/8/8/8/R3K3 w - - 0 1")), chess.WHITE)
        self.assertEqual(table.bare_king_win(chess.Board("8/8/8/4k3/8/8/8/2B1KN2 w - - 0 1")), chess.WHITE)
        self.assertEqual(table.bare_king_win(chess.Board("4k3/8/8/8/8/8/8/1q2K3 w - - 0 1")), chess.BLACK)

    def test_drawn_material_is_not_counted(self) -> None:
        self.assertIsNone(table.bare_king_win(chess.Board("8/8/8/4k3/8/8/8/2B1K3 w - - 0 1")))
        self.assertIsNone(table.bare_king_win(chess.Board("8/8/8/4k3/8/B7/8/2B1K3 w - - 0 1")))  # same colour
        self.assertEqual(table.bare_king_win(chess.Board("8/8/8/4k3/8/8/8/2B1KB2 w - - 0 1")), chess.WHITE)
        self.assertIsNone(table.bare_king_win(chess.Board("8/8/8/4k3/8/8/3P4/4K3 w - - 0 1")))
        self.assertIsNone(table.bare_king_win(chess.Board("8/8/8/3pk3/8/8/8/R3K3 w - - 0 1")))


class Reads(unittest.TestCase):
    def test_a_read_is_counted_with_its_speed_and_clamped_draws(self) -> None:
        games = four_games()
        games[3] = game(4, 2, "B", "A", "1/2-1/2", "1. Kd2 {s=0 d=5 t=10ms h=0ms n=500} 1/2-1/2",
                        fen="8/8/8/4k3/8/8/8/4K2R w - - 0 1")  # B keeps K+R against a lone king
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c210-e0", games, [1, 3, 0])
            built, refused = table.build(pathlib.Path(tmp))
        self.assertEqual(refused, [])
        row = built["rows"]["e0"]
        self.assertEqual(row["wdl"], [1, 3, 0])
        self.assertEqual(row["pentanomial"], [0, 0, 1, 1, 0])
        self.assertEqual(row["clamped_draws"], {"B": 1})
        self.assertAlmostEqual(row["speed"]["A"]["nps"], (1000 + 4000 + 1000) / (10 + 20 + 10) * 1000)
        self.assertAlmostEqual(row["speed"]["A"]["nodes_per_move"], 2000)
        self.assertIn("m7", built["missing"])

    def test_a_meter_is_e0_less_its_read_with_errors_in_quadrature(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c210-e0", four_games(), [1, 3, 0])
            mirrored = [game(1, 1, "A", "B", "1/2-1/2", DRAW), game(2, 1, "B", "A", "1/2-1/2", DRAW),
                        game(3, 2, "A", "B", "1-0", WIN), game(4, 2, "B", "A", "1-0", WIN)]
            write_run(pathlib.Path(tmp) / "c210-m7", mirrored, [1, 2, 1])
            built, refused = table.build(pathlib.Path(tmp))
        self.assertEqual(refused, [])
        e0, m7 = built["rows"]["e0"], built["rows"]["m7"]
        self.assertAlmostEqual(m7["meter"], e0["elo"] - m7["elo"])
        self.assertAlmostEqual(m7["meter_error"], math.hypot(table.half_width(e0), table.half_width(m7)))

    def test_a_recount_that_disagrees_with_the_standings_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, mock.patch.object(table, "GAMES", 4):
            write_run(pathlib.Path(tmp) / "c210-e0", four_games(), [2, 2, 0])  # the PGN says 1-3-0
            status = table.main(["--results", tmp])
            built = json.loads((pathlib.Path(tmp) / "c210-table.json").read_text(encoding="utf-8"))
        self.assertEqual(status, 1)
        self.assertNotIn("e0", built["rows"])
        self.assertTrue(any("disagrees with the standings" in r for r in built["refused"]))

    def test_a_short_read_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            write_run(pathlib.Path(tmp) / "c210-e0", four_games(), [1, 3, 0])
            _, refused = table.build(pathlib.Path(tmp))
        self.assertTrue(any("not 3000 games" in r for r in refused))


if __name__ == "__main__":
    unittest.main()
