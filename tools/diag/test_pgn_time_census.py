#!/usr/bin/env python3
"""pgn_time_census.py, by hand on small games and against §A2.3's table.

The three games of test_replay_games.py give the time a move, with every figure
worked out here from their comments. A game written here gives the decided and
unconverted counts, which those three, unfinished, cannot reach. The last test
reads ignored storage and skips without it: the tool must reproduce R1, number
for number, on the two B.4.7 PGNs.
"""

import io
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import pgn_time_census
import test_replay_games as games

REPO = pathlib.Path(__file__).resolve().parents[2]
AMEND2 = REPO / "tools/results/b5-20261109/amend2"
ON, OFF = "basilisk-b47-on-pext-pgo", "basilisk-b47-off-pext-pgo"
HEADER = '[White "w"]\n[Black "b"]\n[Result "{result}"]\n[TimeControl "white 1000ms+10ms; black 1000ms+10ms"]\n\n'


def census(*texts):
    return pgn_time_census.census(io.StringIO("\n".join(texts)))


def played(result, moves):
    return HEADER.format(result=result) + moves + f" {result}\n"


class TimeAMove(unittest.TestCase):
    def setUp(self):
        self.report = census(games.GAME_BLACK_FIRST, games.GAME_SECOND, games.GAME_WHITE_FIRST)

    def test_three_games_and_both_sides(self):
        self.assertEqual(self.report["games"], 3)
        self.assertEqual(self.report["engines"][ON]["games"], 3)
        self.assertEqual(self.report["engines"][OFF]["games"], 3)

    def test_every_move_lands_in_the_even_band(self):
        self.assertEqual(self.report["engines"][ON]["moves"], {"even": 7})
        self.assertEqual(self.report["engines"][OFF]["moves"], {"even": 6})

    def test_the_mean_time_a_move_is_its_engines(self):
        # 57 53 37 96 55 233 315 and 101 69 44 111 182 687, in ms.
        self.assertEqual(self.report["engines"][ON]["mean_ms"], {"even": 120.9})
        self.assertEqual(self.report["engines"][OFF]["mean_ms"], {"even": 199.0})

    def test_no_move_of_these_spends_half_the_clock(self):
        self.assertEqual(self.report["engines"][ON]["half_clock_moves"], {"even": 0})

    def test_an_unfinished_game_decides_nothing(self):
        self.assertEqual(self.report["engines"][ON]["decided"], {300: 0, 800: 0, 1500: 0})
        self.assertEqual((self.report["engines"][ON]["wins"], self.report["engines"][ON]["draws"], self.report["engines"][ON]["losses"]), (0, 0, 0))


class Bands(unittest.TestCase):
    def test_the_bands_cut_at_300(self):
        self.assertEqual([pgn_time_census.band(s) for s in (300, 299, -299, -300)], ["won", "even", "even", "lost"])

    def test_half_the_clock_left_is_read_at_the_moves_own_clock(self):
        # White's clock: 1000, then 1000 - 490 + 10 = 520. 490 is under half of 1000;
        # 260 is exactly half of 520.
        report = census(played("*", "1. d4 {s=400 d=9 t=490ms} d5 {s=0 d=9 t=500ms} 2. c4 {s=400 d=9 t=260ms} e6 {s=0 d=9 t=1ms}"))
        self.assertEqual(report["engines"]["w"]["half_clock_moves"], {"won": 1})
        self.assertEqual(report["engines"]["b"]["half_clock_moves"], {"even": 1})

    def test_a_score_that_is_not_a_number_is_counted_and_breaks_nothing(self):
        report = census(played("*", "1. d4 {s=M3 d=9 t=1ms} d5 {s=0 d=9 t=1ms}"))
        self.assertEqual(report["engines"]["w"]["unparsed_scores"], 1)
        self.assertEqual(report["engines"]["w"]["moves"], {})


class Decided(unittest.TestCase):
    TWO = "1. d4 {s=900 d=9 t=1ms} d5 {s=-900 d=9 t=1ms} 2. c4 {s=850 d=9 t=1ms} e6 {s=-850 d=9 t=1ms}"
    BROKEN = "1. d4 {s=900 d=9 t=1ms} d5 {s=-900 d=9 t=1ms} 2. c4 {s=100 d=9 t=1ms} e6 {s=-100 d=9 t=1ms} 3. Nf3 {s=900 d=9 t=1ms}"

    def row(self, result, moves, side="w"):
        return census(played(result, moves))["engines"][side]

    def test_two_consecutive_own_moves_decide_a_game_at_each_threshold_reached(self):
        row = self.row("1-0", self.TWO)
        self.assertEqual(row["decided"], {300: 1, 800: 1, 1500: 0})
        self.assertEqual(row["unconverted"], {300: 0, 800: 0, 1500: 0})

    def test_a_broken_run_decides_nothing(self):
        self.assertEqual(self.row("1-0", self.BROKEN)["decided"], {300: 0, 800: 0, 1500: 0})

    def test_a_draw_and_a_loss_are_not_won(self):
        draw = self.row("1/2-1/2", self.TWO)
        self.assertEqual((draw["unconverted"][800], draw["unconverted_draw"][800]), (1, 1))
        loss = self.row("0-1", self.TWO)
        self.assertEqual((loss["unconverted"][800], loss["unconverted_draw"][800]), (1, 0))

    def test_the_other_side_answers_for_its_own_score(self):
        self.assertEqual(self.row("1-0", self.TWO, "b")["decided"], {300: 0, 800: 0, 1500: 0})
        self.assertEqual(self.row("1-0", self.TWO, "b")["losses"], 1)

    def test_a_game_counts_once_however_long_its_run(self):
        long = "1. d4 {s=900 d=9 t=1ms} d5 {s=0 d=9 t=1ms} 2. c4 {s=900 d=9 t=1ms} e6 {s=0 d=9 t=1ms} 3. Nf3 {s=900 d=9 t=1ms}"
        self.assertEqual(self.row("1-0", long)["decided"][300], 1)


class Control(unittest.TestCase):
    def test_a_game_without_a_readable_control_is_refused(self):
        with self.assertRaisesRegex(ValueError, "time control"):
            census('[White "w"]\n[Black "b"]\n[Result "*"]\n[TimeControl "40/120"]\n\n1. d4 {s=1 d=1 t=1ms} *\n')

    def test_the_tool_writes_its_report(self):
        with tempfile.TemporaryDirectory() as directory:
            pgn = pathlib.Path(directory) / "g.pgn"
            out = pathlib.Path(directory) / "o.json"
            pgn.write_text(games.GAME_SECOND, encoding="utf-8")
            done = subprocess.run([sys.executable, pgn_time_census.__file__, str(pgn), "--out", str(out)], capture_output=True, text=True, timeout=60)
            self.assertEqual(done.returncode, 0, done.stderr)
            self.assertEqual(json.loads(out.read_text(encoding="utf-8"))["games"], 1)


@unittest.skipUnless((AMEND2 / "r1_ltc.json").exists(), "ignored storage is absent")
class TableR1(unittest.TestCase):
    """§A2.3's R1 table, number for number, the shares to R1's own three places
    (the table's 13.39 is 13.385 rounded a second time), and R1's whole report."""

    TABLE = {
        "b47-direction-ltc": {
            "OFF": ((444, 42), (403, 1), (391, 0), 178.7, 1.617, 13.385),
            "ON": ((658, 38), (620, 1), (607, 0), 193.6, 1.139, 10.994),
        },
        "b47-gate-a": {
            "OFF": ((619, 58), (559, 1), (532, 0), 55.0, 0.770, 6.374),
            "ON": ((816, 37), (771, 1), (730, 0), 58.9, 0.673, 4.784),
        },
    }

    def test_the_table(self):
        for run, arms in self.TABLE.items():
            with (REPO / "tools/results" / run / "games.pgn").open(encoding="utf-8", errors="replace") as handle:
                report = pgn_time_census.census(handle)
            stored_name = "r1_ltc.json" if "ltc" in run else "r1_stc.json"
            stored = json.loads((AMEND2 / stored_name).read_text(encoding="utf-8"))
            self.assertEqual(json.loads(json.dumps(report))["engines"], stored["engines"], run)
            engines = report["engines"]
            for arm, (d300, d800, d1500, mean_won, half_won, half_lost) in arms.items():
                name = next(n for n in engines if f"-{arm.lower()}-" in n)
                row = engines[name]
                self.assertEqual((row["decided"][300], row["unconverted"][300]), d300, (run, arm))
                self.assertEqual((row["decided"][800], row["unconverted"][800]), d800, (run, arm))
                self.assertEqual((row["decided"][1500], row["unconverted"][1500]), d1500, (run, arm))
                self.assertEqual(row["mean_ms"]["won"], mean_won, (run, arm))
                self.assertEqual(round(100 * row["half_clock_moves"]["won"] / row["moves"]["won"], 3), half_won, (run, arm))
                self.assertEqual(round(100 * row["half_clock_moves"]["lost"] / row["moves"]["lost"], 3), half_lost, (run, arm))


if __name__ == "__main__":
    unittest.main()
