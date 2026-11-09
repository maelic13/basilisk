#!/usr/bin/env python3
"""replay_games.py's clock reconstruction, against three games of a Colosseum PGN.

The expected clocks are written out by hand from each comment's `t=` and the
header's control, to the millisecond: a mover's clock falls by what the move spent
and gains the increment, and the other side's clock stands still.
"""

import io
import pathlib
import sys
import unittest

import chess
import chess.pgn

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import replay_games

# The first moves of games 1 and 2 of the B.4.7 gate (a) PGN, and of game 1 of
# its 10+0.1 direction run's shape: a set position with Black to move, the
# standard start, and unequal controls.
GAME_BLACK_FIRST = """[White "basilisk-b47-on-pext-pgo"]
[Black "basilisk-b47-off-pext-pgo"]
[Result "*"]
[TimeControl "white 3000ms+30ms; black 3000ms+30ms"]
[SetUp "1"]
[FEN "rnbqk1nr/pppp1ppp/3b4/8/1PPP4/8/P4PPP/RNBQKBNR b KQkq - 0 1"]

1... Bxb4+ {s=73 d=14 t=101ms h=1ms n=176775} 2. Bd2
{s=-91 d=15 t=57ms h=0ms n=102860} Bxd2+ {s=47 d=15 t=69ms h=1ms n=126447} 3.
Qxd2 {s=-57 d=16 t=53ms h=0ms n=94186} *
"""
GAME_WHITE_FIRST = """[White "other"]
[Black "basilisk-b47-on-pext-pgo"]
[Result "*"]
[TimeControl "white 10000ms+100ms; black 10000ms+100ms"]

1. e4 {s=20 d=10 t=250ms} e5 {s=-20 d=11 t=300ms} 2. Nf3 {s=25 d=12 t=125ms} *
"""
GAME_UNEQUAL = """[White "a"]
[Black "b"]
[Result "*"]
[TimeControl "white 5000ms+50ms; black 2000ms+0ms"]

1. d4 {t=1000ms} d5 {t=7ms} 2. c4 {t=3ms} e6 {t=1993ms} *
"""


def read(text: str) -> chess.pgn.Game:
    game = chess.pgn.read_game(io.StringIO(text))
    assert game is not None
    return game


class ClockReconstruction(unittest.TestCase):
    def test_a_set_position_with_black_to_move(self):
        rows = replay_games.rebuild_clocks(read(GAME_BLACK_FIRST))
        self.assertEqual([(r["san"], r["wtime"], r["btime"], r["spent"]) for r in rows], [
            ("Bxb4+", 3000, 3000, 101),
            ("Bd2", 3000, 2929, 57),
            ("Bxd2+", 2973, 2929, 69),
            ("Qxd2", 2973, 2890, 53),
        ])
        self.assertEqual(rows[0]["side"], chess.BLACK)
        self.assertEqual(rows[2]["moves"], ["d6b4", "c1d2"])

    def test_the_standard_start(self):
        rows = replay_games.rebuild_clocks(read(GAME_WHITE_FIRST))
        self.assertEqual([(r["wtime"], r["btime"]) for r in rows],
                         [(10000, 10000), (9850, 10000), (9850, 9800)])
        self.assertEqual({(r["winc"], r["binc"]) for r in rows}, {(100, 100)})

    def test_each_side_keeps_its_own_control(self):
        rows = replay_games.rebuild_clocks(read(GAME_UNEQUAL))
        self.assertEqual([(r["wtime"], r["btime"]) for r in rows],
                         [(5000, 2000), (4050, 2000), (4050, 1993), (4097, 1993)])
        self.assertEqual({(r["winc"], r["binc"]) for r in rows}, {(50, 0)})

    def test_an_unreadable_control_or_a_missing_time_is_refused(self):
        with self.assertRaisesRegex(ValueError, "time control"):
            replay_games.parse_control("40/120")
        no_time = GAME_UNEQUAL.replace("{t=7ms}", "{s=1}")
        with self.assertRaisesRegex(ValueError, "no t="):
            replay_games.rebuild_clocks(read(no_time))


if __name__ == "__main__":
    unittest.main()
