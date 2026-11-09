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

# The first moves of games 1 and 2 of the B.4.7 gate (a) PGN and of game 1 of
# its 10+0.1 direction run, then a game written here for unequal controls, which
# the gates never play.
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
GAME_SECOND = """[White "basilisk-b47-off-pext-pgo"]
[Black "basilisk-b47-on-pext-pgo"]
[Result "*"]
[TimeControl "white 3000ms+30ms; black 3000ms+30ms"]
[SetUp "1"]
[FEN "rnbqk1nr/pppp1ppp/3b4/8/1PPP4/8/P4PPP/RNBQKBNR b KQkq - 0 1"]

1... Bxb4+ {s=95 d=14 t=37ms h=1ms n=70102} 2. Bd2
{s=-106 d=16 t=44ms h=0ms n=79592} Bxd2+ {s=58 d=16 t=96ms h=0ms n=174312} 3.
Qxd2 {s=-63 d=14 t=111ms h=1ms n=198882} d5 {s=66 d=16 t=55ms h=1ms n=101552} *
"""
GAME_WHITE_FIRST = """[White "basilisk-b47-off-pext-pgo"]
[Black "basilisk-b47-on-pext-pgo"]
[Result "*"]
[TimeControl "white 10000ms+100ms; black 10000ms+100ms"]
[SetUp "1"]
[FEN "r1bqkb1r/p4ppp/2p5/2ppN3/4nP2/8/PPPP2PP/RNBQK2R w KQkq - 0 1"]

1. Qf3 {s=-84 d=17 t=182ms h=1ms n=285025} c4 {s=82 d=15 t=233ms h=1ms n=384758}
2. Nxc6 {s=-107 d=15 t=687ms h=0ms n=1250189} Qc7
{s=60 d=15 t=315ms h=1ms n=538363} *
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

    def test_the_other_side_replays_the_same_opening(self):
        rows = replay_games.rebuild_clocks(read(GAME_SECOND))
        self.assertEqual([(r["wtime"], r["btime"]) for r in rows],
                         [(3000, 3000), (3000, 2993), (2986, 2993), (2986, 2927), (2905, 2927)])
        self.assertEqual(rows[4]["moves"], ["d6b4", "c1d2", "b4d2", "d1d2"])

    def test_white_to_move_at_the_longer_control(self):
        rows = replay_games.rebuild_clocks(read(GAME_WHITE_FIRST))
        self.assertEqual([(r["wtime"], r["btime"]) for r in rows],
                         [(10000, 10000), (9918, 10000), (9918, 9867), (9331, 9867)])
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
