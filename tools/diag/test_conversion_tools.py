from __future__ import annotations

import importlib.util
import io
import json
import pathlib
import sqlite3
import tempfile
import unittest

import chess
import chess.pgn

ROOT = pathlib.Path(__file__).resolve().parents[2]


def load_module(name: str, filename: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / "diag" / filename)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(module)
    return module


conversion = load_module("conversion_audit", "conversion_audit.py")
exporter = load_module("export_tournament_pgn", "export_tournament_pgn.py")

ROOK_UP = "8/8/4k3/8/8/4K3/8/R7 w - - 0 1"
LONE_BISHOP = "8/8/4k3/8/8/4K3/8/2B5 w - - 0 1"


def shuffle_moves(fen: str, plies: int) -> list[str]:
    board = chess.Board(fen)
    moves = []
    for _ in range(plies):
        move = next(iter(board.legal_moves))
        moves.append(move.uci())
        board.push(move)
    return moves


def make_game(fen: str, moves: list[str], white: str, black: str,
              result: str, termination: str = "normal"):
    board = chess.Board(fen)
    game = chess.pgn.Game()
    game.setup(board)
    node = game
    for uci in moves:
        move = chess.Move.from_uci(uci)
        if move not in board.legal_moves:
            raise AssertionError(f"{uci} illegal in {board.fen()}")
        board.push(move)
        node = node.add_variation(move)
    game.headers["White"] = white
    game.headers["Black"] = black
    game.headers["Result"] = result
    game.headers["Termination"] = termination
    return game


class ConversionAuditTests(unittest.TestCase):
    def test_material_helpers(self):
        board = chess.Board()
        self.assertEqual(conversion.material(board, chess.WHITE), 39)
        self.assertEqual(conversion.material(board, chess.BLACK), 39)
        self.assertEqual(conversion.non_pawn_material(board), 62)

    def test_lone_minor_signatures(self):
        cases = [
            ("8/8/8/4k3/8/8/8/4K1B1 w - - 0 1", True),
            ("8/8/8/4k3/8/8/8/4K1N1 w - - 0 1", True),
            ("8/8/8/4k3/8/8/8/4KBN1 w - - 0 1", False),
            ("8/8/8/4k3/8/8/8/4KBB1 w - - 0 1", False),
            ("8/8/8/4k3/8/8/8/4K1R1 w - - 0 1", False),
            ("8/8/8/4k3/8/8/4P3/4K1B1 w - - 0 1", False),
            ("8/8/8/3bk3/8/8/8/4K1B1 w - - 0 1", False),
        ]
        for fen, expected in cases:
            with self.subTest(fen=fen):
                self.assertEqual(conversion.is_lone_minor(chess.Board(fen), chess.WHITE),
                                 expected)

    def test_outcome_uses_named_engine_perspective(self):
        self.assertEqual(conversion.outcome_for("1-0", True), "W")
        self.assertEqual(conversion.outcome_for("1-0", False), "L")
        self.assertEqual(conversion.outcome_for("0-1", False), "W")
        self.assertEqual(conversion.outcome_for("1/2-1/2", True), "D")
        self.assertEqual(conversion.outcome_for("*", True), "?")

    def test_persistence_lone_minor_and_color(self):
        rook = make_game(ROOK_UP, shuffle_moves(ROOK_UP, 20),
                         "Us", "Them", "1/2-1/2")
        self.assertTrue(conversion.classify_game(rook, "Us")["persistent_advantage"])

        short = make_game(ROOK_UP, shuffle_moves(ROOK_UP, 6),
                          "Us", "Them", "1/2-1/2")
        self.assertFalse(conversion.classify_game(short, "Us")["persistent_advantage"])

        lone = make_game(LONE_BISHOP, shuffle_moves(LONE_BISHOP, 20),
                         "Us", "Them", "1/2-1/2")
        self.assertFalse(conversion.classify_game(lone, "Us", True)["persistent_advantage"])
        self.assertTrue(conversion.classify_game(lone, "Us", False)["persistent_advantage"])

        black = make_game(ROOK_UP, shuffle_moves(ROOK_UP, 20),
                          "Them", "Us", "1/2-1/2")
        verdict = conversion.classify_game(black, "Us")
        self.assertFalse(verdict["persistent_advantage"])
        self.assertTrue(verdict["persistent_deficit"])

    def test_audit_counts_and_filters(self):
        games = [
            make_game(ROOK_UP, shuffle_moves(ROOK_UP, 20),
                      "Us", "Strong", "1/2-1/2", "adjudication"),
            make_game(ROOK_UP, shuffle_moves(ROOK_UP, 20),
                      "Us", "Strong", "0-1"),
            make_game(ROOK_UP, shuffle_moves(ROOK_UP, 20),
                      "Us", "Weak", "1/2-1/2"),
        ]
        summary = conversion.audit(iter(games), "Us", None, True)
        self.assertEqual(summary["counts"]["games"], 3)
        self.assertEqual(summary["thrown_away"], 3)
        self.assertEqual(summary["draw_terminations"], {"adjudication": 1, "normal": 1})
        filtered = conversion.audit(iter(games), "Us", {"Strong"}, True)
        self.assertEqual(filtered["counts"]["games"], 2)
        self.assertEqual(filtered["thrown_away"], 2)

    def test_real_pgn_round_trip(self):
        game = make_game(ROOK_UP, shuffle_moves(ROOK_UP, 20),
                         "Us", "Them", "1/2-1/2")
        reparsed = chess.pgn.read_game(io.StringIO(str(game)))
        self.assertTrue(conversion.classify_game(reparsed, "Us")["persistent_advantage"])


class ExportTournamentTests(unittest.TestCase):
    def test_export_rewrites_headers_and_ignores_unfinished_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            db = root / "colosseum.sqlite"
            conn = sqlite3.connect(db)
            conn.executescript(
                "create table tournament_engines ("
                "tournament_id text, engine_id text, engine_config_json text);"
                "create table games (tournament_id text, status text, white_id text, "
                "black_id text, result text, termination text, pgn text);"
            )
            tournament = "test-tournament"
            conn.executemany(
                "insert into tournament_engines values (?, ?, ?)",
                [
                    (tournament, "a", json.dumps({"meta": {"name": "Alpha", "version": "1.0"}})),
                    (tournament, "b", json.dumps({"meta": {"name": "Beta", "version": "2.0"}})),
                ],
            )
            pgn = "[Result \"1-0\"]\n\n1. e4 e5 1-0\n"
            conn.executemany(
                "insert into games values (?, ?, ?, ?, ?, ?, ?)",
                [
                    (tournament, "finished", "a", "b", '"WhiteWin"', '"Checkmate"', pgn),
                    (tournament, "running", "b", "a", '"Draw"', '"Threefold"', pgn),
                ],
            )
            conn.commit()
            conn.close()

            out = root / "games.pgn"
            manifest = exporter.export(db, tournament, out)
            self.assertEqual(manifest["games"], 1)
            self.assertEqual(manifest["unparsable_games_skipped"], 0)
            self.assertEqual(manifest["engines"], ["Alpha 1.0", "Beta 2.0"])
            exported = chess.pgn.read_game(io.StringIO(out.read_text(encoding="utf-8")))
            self.assertEqual(exported.headers["White"], "Alpha 1.0")
            self.assertEqual(exported.headers["Black"], "Beta 2.0")
            self.assertEqual(exported.headers["Result"], "1-0")
            self.assertEqual(exported.headers["Termination"], "Checkmate")
            self.assertEqual(exported.headers["Event"], f"Colosseum {tournament}")


if __name__ == "__main__":
    unittest.main()
