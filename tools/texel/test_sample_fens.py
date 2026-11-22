#!/usr/bin/env python3
"""Focused tests for opening-book exclusion parsing."""

import unittest

import audit_starts
import sample_fens


class ExclusionTests(unittest.TestCase):
    def test_equal_five_phase_targets_are_exact(self):
        self.assertEqual(sample_fens.bucket_targets(12), [3, 3, 2, 2, 2])

    def test_weighted_targets_follow_the_registered_mix(self):
        self.assertEqual(sample_fens.bucket_targets(100, [5, 1, 1, 1, 2]), [50, 10, 10, 10, 20])

    def test_the_sampler_and_the_extractor_share_one_phase_rule(self):
        import random
        import chess
        import extract
        rng = random.Random(3)
        for _ in range(200):
            board = chess.Board()
            for _ in range(rng.randint(0, 120)):
                moves = list(board.legal_moves)
                if not moves:
                    break
                board.push(rng.choice(moves))
            epd = " ".join(board.fen().split()[:4])
            self.assertEqual(sample_fens.phase_bucket(epd),
                             extract.phase_bucket(extract.game_phase(board)))

    def test_phase_classifier(self):
        self.assertEqual(sample_fens.phase_bucket(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"), 0)
        self.assertEqual(sample_fens.phase_bucket(
            "4k3/8/8/8/8/8/P7/4K3 w - -"), 4)

    def test_fen_header_becomes_epd_key(self):
        line = '[FEN "8/8/8/8/8/8/K6k/8 w - - 17 42"]\n'
        self.assertEqual(
            sample_fens.parse_pgn_fen_header(line),
            "8/8/8/8/8/8/K6k/8 w - -",
        )

    def test_non_fen_header_is_ignored(self):
        self.assertIsNone(sample_fens.parse_pgn_fen_header('[Event "x"]\n'))

    def test_audit_header_and_pawn_family(self):
        line = '[FEN "8/8/8/8/8/8/P6p/4K2k w - - 17 42"]\n'
        fen = audit_starts.parse_fen_header(line)
        self.assertEqual(fen, "8/8/8/8/8/8/P6p/4K2k w - -")
        self.assertEqual(audit_starts.pawn_family(fen), ("8", "15", "w", "-"))


if __name__ == "__main__":
    unittest.main()
