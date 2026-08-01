#!/usr/bin/env python3
"""Fast unit tests for the upgraded Texel extraction contracts."""

import random
import unittest

import chess
import chess.pgn

import extract


class ExtractTests(unittest.TestCase):
    def test_equal_five_phase_allocation_is_exact(self):
        self.assertEqual(
            extract.allocate(3_500_003, [1, 1, 1, 1, 1]),
            [700_001, 700_001, 700_001, 700_000, 700_000],
        )

    def test_five_phase_boundaries(self):
        expected = {24: 0, 20: 0, 19: 1, 14: 1, 13: 2,
                    8: 2, 7: 3, 3: 3, 2: 4, 0: 4}
        for phase, bucket in expected.items():
            with self.subTest(phase=phase):
                self.assertEqual(extract.phase_bucket(phase), bucket)

    def test_reservoir_is_bounded_and_reproducible(self):
        def sample():
            reservoir = extract.Reservoir(25, random.Random(42))
            for value in range(1_000):
                reservoir.offer((str(value), 0.5, 0))
            return reservoir.items

        self.assertEqual(sample(), sample())
        self.assertEqual(len(sample()), 25)

    def test_beast_start_position_is_available_with_skip_zero(self):
        game = chess.pgn.Game()
        game.headers["Result"] = "1/2-1/2"
        game.add_variation(chess.Move.from_uci("e2e4"))
        rows, rejected = extract.process_game(
            game, 0, 0, 1, 0, False, random.Random(1)
        )
        self.assertEqual(rejected, 0)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0][1], 0)

    def test_process_game_classifies_all_five_phases(self):
        cases = (
            (chess.STARTING_FEN, "e2e4", 0),
            ("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1", "e2e4", 1),
            ("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "a1a2", 2),
            ("4k2r/8/8/8/8/8/8/R3K3 w - - 0 1", "a1a2", 3),
            ("4k3/8/8/8/8/8/P7/4K3 w - - 0 1", "a2a3", 4),
        )
        for fen, move_uci, expected_bucket in cases:
            with self.subTest(bucket=expected_bucket):
                game = chess.pgn.Game()
                game.setup(chess.Board(fen))
                game.headers["Result"] = "1/2-1/2"
                game.add_variation(chess.Move.from_uci(move_uci))
                rows, _ = extract.process_game(
                    game, 0, 0, 1, 0, False, random.Random(1)
                )
                self.assertEqual(rows[0][1], expected_bucket)

    def test_game_digest_includes_start_position(self):
        def make_game(fen):
            game = chess.pgn.Game()
            game.setup(chess.Board(fen))
            game.headers["Result"] = "1/2-1/2"
            game.add_variation(next(iter(game.board().legal_moves)))
            return game

        first = make_game(chess.STARTING_FEN)
        second = make_game("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1")
        self.assertNotEqual(extract.game_digest(first), extract.game_digest(second))


if __name__ == "__main__":
    unittest.main()
