#!/usr/bin/env python3
"""Tests of the Texel row contract, basilisk-texel-rows-v1 (extract.py,
extract_parallel.py and claim_test.py). Needs python-chess.

    python tools/texel/test_extract.py
"""

from __future__ import annotations

import io
import json
import random
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import chess
import chess.pgn

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import extract  # noqa: E402

OPTS = extract.Options(skip_start=0, skip_end=0, max_per_phase_per_game=8,
                       max_per_game=0, quiet_filter=False, validation_pct=20.0,
                       test_pct=20.0, seed=42)


def game_from(fen: str, moves: list[str], result: str,
              termination: str | None = "normal") -> chess.pgn.Game:
    game = chess.pgn.Game()
    board = chess.Board(fen)
    if fen != chess.STARTING_FEN:
        game.setup(board)
    node = game
    for uci in moves:
        move = board.parse_uci(uci)  # raises on an illegal move
        board.push(move)
        node = node.add_variation(move)
    game.headers["Result"] = result
    if termination is not None:
        game.headers["Termination"] = termination
    return game


def reparse(game: chess.pgn.Game) -> chess.pgn.Game:
    return chess.pgn.read_game(io.StringIO(str(game)))


def random_game(rng: random.Random) -> chess.pgn.Game | None:
    """A random legal game from a random start four plies in, played to a
    rules ending; None when it runs past 400 plies."""
    board = chess.Board()
    for _ in range(4):
        board.push(rng.choice(list(board.legal_moves)))
    start = board.copy(stack=False)
    game = chess.pgn.Game()
    game.setup(start)
    node = game
    # The game ends where the harness ends it: a mate, a stalemate, too little
    # material, or a position that has occurred three times or a clock at 100,
    # not where python-chess would let a draw be claimed one move early.
    while not (board.is_checkmate() or board.is_stalemate() or board.is_insufficient_material()
               or board.is_repetition(3) or board.halfmove_clock >= 100):
        if board.ply() > 400:
            return None
        move = rng.choice(list(board.legal_moves))
        board.push(move)
        node = node.add_variation(move)
    game.headers["Result"] = ("0-1" if board.turn == chess.WHITE else "1-0") \
        if board.is_checkmate() else "1/2-1/2"
    game.headers["Termination"] = "normal"
    return game


def write_corpus(path: Path, count: int, seed: int, extra: list[chess.pgn.Game] = ()) -> list:
    rng = random.Random(seed)
    games = []
    while len(games) < count:
        game = random_game(rng)
        if game is not None:
            games.append(game)
    games.extend(extra)
    path.write_text("\n\n".join(str(game) for game in games) + "\n", encoding="utf-8")
    return games


def run_tool(script: str, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(HERE / script), *args],
                          capture_output=True, text=True, check=False)


def read_rows(path: Path) -> tuple[list[str], list[list[str]]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    return lines[:2], [line.split(",") for line in lines[2:]]


class AllocationAndPhases(unittest.TestCase):
    def test_equal_five_phase_allocation_is_exact(self):
        self.assertEqual(extract.allocate(3_500_003, [1, 1, 1, 1, 1]),
                         [700_001, 700_001, 700_001, 700_000, 700_000])

    def test_five_phase_boundaries(self):
        expected = {24: 0, 20: 0, 19: 1, 14: 1, 13: 2, 8: 2, 7: 3, 3: 3, 2: 4, 0: 4}
        for phase, bucket in expected.items():
            with self.subTest(phase=phase):
                self.assertEqual(extract.phase_bucket(phase), bucket)

    def test_reservoir_is_bounded_and_reproducible(self):
        def sample():
            reservoir = extract.Reservoir(25, random.Random(42))
            for value in range(1_000):
                reservoir.offer((str(value),))
            return reservoir.items
        self.assertEqual(sample(), sample())
        self.assertEqual(len(sample()), 25)


class RulesEnding(unittest.TestCase):
    """A game is labelled only if it ended by the rules with its result."""

    FOOLS_MATE = ["f2f3", "e7e5", "g2g4", "d8h4"]

    def test_checkmate_with_its_result_is_labelled(self):
        game = game_from(chess.STARTING_FEN, self.FOOLS_MATE, "0-1")
        self.assertIsNone(extract.rules_ending(game))

    def test_checkmate_with_the_wrong_result_is_rejected(self):
        for result in ("1-0", "1/2-1/2"):
            with self.subTest(result=result):
                game = game_from(chess.STARTING_FEN, self.FOOLS_MATE, result)
                self.assertEqual(extract.rules_ending(game), "result_contradicts_mate")

    def test_stalemate_is_a_draw(self):
        fen = "7k/5Q2/6K1/8/8/8/8/8 w - - 0 1"
        game = game_from(fen, ["f7f6"], "1/2-1/2")  # not yet stalemate
        self.assertEqual(extract.rules_ending(game), "not_a_rules_ending")
        game = game_from("7k/8/5QK1/8/8/8/8/8 w - - 0 1", ["f6f7"], "1/2-1/2")
        self.assertIsNone(extract.rules_ending(game))
        game = game_from("7k/8/5QK1/8/8/8/8/8 w - - 0 1", ["f6f7"], "1-0")
        self.assertEqual(extract.rules_ending(game), "result_contradicts_draw")

    def test_threefold_repetition_counts_the_start(self):
        shuffle = ["g1f3", "g8f6", "f3g1", "f6g8"]
        game = game_from(chess.STARTING_FEN, shuffle * 2, "1/2-1/2")
        self.assertIsNone(extract.rules_ending(game))
        game = game_from(chess.STARTING_FEN, shuffle, "1/2-1/2")
        self.assertEqual(extract.rules_ending(game), "not_a_rules_ending")

    def test_fifty_moves_and_insufficient_material(self):
        game = game_from("8/8/4k3/8/8/4K3/8/7R w - - 99 80", ["h1h2"], "1/2-1/2")
        self.assertIsNone(extract.rules_ending(game))
        game = game_from("8/8/4k3/8/8/4K3/8/7n w - - 0 1", ["e3d4"], "1/2-1/2")
        self.assertIsNone(extract.rules_ending(game))

    def test_headers_reject_unfinished_and_unnatural_games(self):
        game = game_from(chess.STARTING_FEN, self.FOOLS_MATE, "*")
        self.assertEqual(extract.rules_ending(game), "result_not_a_game_result")
        for termination in ("time forfeit", "adjudication", "abandoned", "illegal move"):
            with self.subTest(termination=termination):
                game = game_from(chess.STARTING_FEN, self.FOOLS_MATE, "0-1", termination)
                self.assertEqual(extract.rules_ending(game), "termination_not_normal")
        game = game_from(chess.STARTING_FEN, self.FOOLS_MATE, "0-1", termination=None)
        self.assertIsNone(extract.rules_ending(game))

    def test_a_resignation_is_not_labelled(self):
        game = game_from(chess.STARTING_FEN, ["e2e4", "e7e5"], "1-0")
        self.assertEqual(extract.game_rows(game, OPTS).reject, "not_a_rules_ending")


class Dedup(unittest.TestCase):
    """C-8: the served score reads the rule-50 clock, so the key holds it."""

    def test_the_clock_separates_two_rows_and_the_move_number_does_not(self):
        a = "8/8/4k3/8/8/4K3/8/7R w - - 0 40"
        self.assertNotEqual(extract.fen_key(a), extract.fen_key("8/8/4k3/8/8/4K3/8/7R w - - 30 40"))
        self.assertEqual(extract.fen_key(a), extract.fen_key("8/8/4k3/8/8/4K3/8/7R w - - 0 77"))

    def test_the_split_still_keys_on_the_position_alone(self):
        self.assertEqual(extract.start_key_of("8/8/4k3/8/8/4K3/8/7R w - - 0 40"),
                         extract.start_key_of("8/8/4k3/8/8/4K3/8/7R w - - 30 40"))


class RowIdentity(unittest.TestCase):
    def test_rows_carry_ply_phase_and_stable_ids(self):
        moves = ["g1f3", "g8f6", "f3g1", "f6g8"] * 2
        rows = extract.game_rows(game_from(chess.STARTING_FEN, moves, "1/2-1/2"), OPTS)
        self.assertIsNone(rows.reject)
        board = chess.Board()
        fens = []
        for uci in moves:
            fens.append(board.fen())
            board.push_uci(uci)
        for candidate in rows.candidates:
            self.assertEqual(candidate.fen, fens[candidate.ply])
            self.assertEqual(candidate.phase, 24)
        again = extract.game_rows(reparse(game_from(chess.STARTING_FEN, moves, "1/2-1/2")), OPTS)
        self.assertEqual((rows.game_id, rows.start_id), (again.game_id, again.start_id))
        self.assertRegex(rows.game_id, r"^[0-9a-f]{16}$")

    def test_one_start_is_one_split_whatever_the_moves(self):
        # Forty games from one start, each a different first pair of moves and
        # then a knight shuffle to a threefold repetition. A split keyed on the
        # moves would scatter them (each lands in train with probability 0.6).
        firsts = ["b1c3", "b1a3", "h2h3", "a2a3", "e2e3", "d2d3", "c2c3", "b2b3"]
        replies = ["b8c6", "b8a6", "h7h6", "a7a6", "e7e6"]
        shuffle = ["g1f3", "g8f6", "f3g1", "f6g8"] * 2
        games = [extract.game_rows(game_from(chess.STARTING_FEN, [w, b] + shuffle, "1/2-1/2"), OPTS)
                 for w in firsts for b in replies]
        self.assertEqual(len({rows.game_id for rows in games}), 40)
        self.assertTrue(all(rows.reject is None for rows in games))
        self.assertEqual({rows.start_id for rows in games}, {games[0].start_id})
        self.assertEqual({rows.split for rows in games}, {games[0].split})

    def test_split_shares_follow_the_percentages(self):
        splits = [extract.split_for_start(f"start {index}", 5.0, 5.0) for index in range(40_000)]
        for split, share in (("test", 0.05), ("validation", 0.05), ("train", 0.90)):
            with self.subTest(split=split):
                self.assertAlmostEqual(splits.count(split) / len(splits), share, delta=0.006)


class Publication(unittest.TestCase):
    ARGS = ("--target-train", "60", "--validation-pct", "20", "--test-pct", "20",
            "--max-per-phase-per-game", "2", "--skip-end", "0", "--no-quiet-filter")

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.root = Path(cls.tmp.name)
        cls.pgn = cls.root / "games.pgn"
        cls.games = write_corpus(cls.pgn, 300, seed=7)
        cls.seq_dir = cls.root / "seq"
        cls.seq = run_tool("extract.py", str(cls.pgn), "--out-dir", str(cls.seq_dir), *cls.ARGS)
        cls.par_dir = cls.root / "par"
        cls.par = run_tool("extract_parallel.py", str(cls.pgn), "--out-dir", str(cls.par_dir),
                           "--jobs", "3", *cls.ARGS)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_both_extractors_publish(self):
        self.assertEqual(self.seq.returncode, 0, self.seq.stdout + self.seq.stderr)
        self.assertEqual(self.par.returncode, 0, self.par.stdout + self.par.stderr)

    def test_the_parallel_output_is_byte_identical(self):
        for name in ("train.csv", "validation.csv", "test.csv", "manifest.json"):
            with self.subTest(file=name):
                self.assertEqual((self.seq_dir / name).read_bytes(),
                                 (self.par_dir / name).read_bytes())

    def test_files_follow_the_schema(self):
        manifest = json.loads((self.seq_dir / "manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["schema"], extract.SCHEMA)
        starts: dict[str, str] = {}
        by_game = {extract.game_id(game): game for game in self.games}
        for split in extract.SPLITS:
            head, rows = read_rows(self.seq_dir / f"{split}.csv")
            self.assertEqual(head, [f"# {extract.SCHEMA} {split}", ",".join(extract.COLUMNS)])
            self.assertEqual(len(rows), manifest["output"][split]["rows"])
            self.assertEqual(len(rows), extract.split_counts(60, 20, 20)[split])
            for fen, result, gid, sid, ply, phase in rows:
                self.assertIn(result, ("1", "0.5", "0"))
                self.assertEqual(starts.setdefault(sid, split), split, "a start in two splits")
                game = by_game[gid]
                board = game.board()
                for move in list(game.mainline_moves())[: int(ply)]:
                    board.push(move)
                self.assertEqual(board.fen(), fen)
                self.assertEqual(int(phase), extract.game_phase(board))
                self.assertEqual(extract.RESULT_TEXT[extract.RESULT_MAP[game.headers["Result"]]],
                                 result)

    def test_a_published_corpus_is_never_overwritten(self):
        again = run_tool("extract.py", str(self.pgn), "--out-dir", str(self.seq_dir), *self.ARGS)
        self.assertEqual(again.returncode, 2)
        self.assertIn("refusing to overwrite", again.stderr)

    def test_too_many_rejected_games_publish_nothing(self):
        forfeits = [game_from(chess.STARTING_FEN, RulesEnding.FOOLS_MATE, "0-1", "time forfeit")
                    for _ in range(4)]
        pgn = self.root / "forfeits.pgn"
        write_corpus(pgn, 300, seed=7, extra=forfeits)  # 4 of 304 is past 1%
        out = self.root / "forfeits"
        done = run_tool("extract.py", str(pgn), "--out-dir", str(out), *self.ARGS)
        self.assertEqual(done.returncode, 2, done.stdout + done.stderr)
        self.assertIn("past the ceiling", done.stderr)
        self.assertFalse(out.exists() and any(out.iterdir()))
        loose = run_tool("extract.py", str(pgn), "--out-dir", str(out), "--max-reject-pct", "2",
                         *self.ARGS)
        self.assertEqual(loose.returncode, 0, loose.stdout + loose.stderr)
        manifest = json.loads((out / "manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["games_rejected"]["termination_not_normal"], 4)

    def test_the_test_split_is_claimed_once(self):
        corpus = self.root / "claimed"
        done = run_tool("extract.py", str(self.pgn), "--out-dir", str(corpus), *self.ARGS)
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
        self.assertEqual(run_tool("claim_test.py", "status", str(corpus)).returncode, 1)
        first = run_tool("claim_test.py", "claim", str(corpus), "--candidate", "a1",
                         "--purpose", "unit test")
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(Path(first.stdout.strip()), corpus / "test.csv")
        second = run_tool("claim_test.py", "claim", str(corpus), "--candidate", "b2",
                          "--purpose", "a second look")
        self.assertEqual(second.returncode, 1)
        self.assertIn("already claimed", second.stderr)
        status = run_tool("claim_test.py", "status", str(corpus))
        self.assertEqual(status.returncode, 0)
        self.assertEqual(json.loads(status.stdout)["candidate"], "a1")

    def test_a_changed_test_split_cannot_be_claimed(self):
        corpus = self.root / "tampered"
        done = run_tool("extract.py", str(self.pgn), "--out-dir", str(corpus), *self.ARGS)
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
        with (corpus / "test.csv").open("a", encoding="utf-8", newline="\n") as out:
            out.write("8/8/8/8/8/8/8/K6k w - - 0 1,0.5,0,0,0,0\n")
        claim = run_tool("claim_test.py", "claim", str(corpus), "--candidate", "a1",
                         "--purpose", "unit test")
        self.assertEqual(claim.returncode, 1)
        self.assertIn("its manifest published", claim.stderr)
        self.assertFalse((corpus / "test.claim.json").exists())


if __name__ == "__main__":
    unittest.main()
