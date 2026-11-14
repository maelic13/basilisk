#!/usr/bin/env python3
"""won_battery.py: its sample, its classes and an engine that never answers.

The sample tests run on games written here, so they run anywhere. Two read
ignored storage and skip without it: the review's 60 positions against the
direction run's PGN, and the review's rows against the amendment's counts.
"""

import hashlib
import io
import json
import pathlib
import subprocess
import sys
import tempfile
import textwrap
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import won_battery

REPO = pathlib.Path(__file__).resolve().parents[2]
PGN = REPO / "tools/results/b47-direction-ltc/games.pgn"
REVIEW = REPO / "tools/results/b5-20261109/review"
FEN = "8/8/3k4/8/8/8/8/R3K3 w - - 0 70"


def game(number, moves):
    return f'[GameNumber "{number}"]\n[Result "*"]\n[FEN "{FEN}"]\n[SetUp "1"]\n\n{moves} *\n\n'


# Rook against king: the mover's score decides which stratum a position joins.
THREE = (
    game(1, "1. Ra2 {s=2000 d=9 t=1ms} Kd5 {s=-2000 d=9 t=1ms} 2. Ra3 {s=2100 d=9 t=1ms}")
    + game(2, "1. Ra2 {s=100 d=9 t=1ms} Kd5 {s=-100 d=9 t=1ms} 2. Ra3 {s=400 d=9 t=1ms}"
              " Kd6 {s=-400 d=9 t=1ms} 3. Ra4 {s=500 d=9 t=1ms}")
    + game(3, "1. Ra2 {s=1600 d=9 t=1ms} Kd5 {s=-1600 d=9 t=1ms}")
    + game(4, "1. Ra2 {s=30000 d=9 t=1ms} Kd5 {s=-30000 d=9 t=1ms} 2. Ra3 {s=1700 d=9 t=1ms}")
)


class Sample(unittest.TestCase):
    def strata(self, skip=1, decided=2, better=2):
        return won_battery.sample(io.StringIO(THREE), skip=skip, decided=decided, better=better)

    def test_the_decided_stratum_is_each_game_first_position_after_the_skip(self):
        strata = self.strata()
        self.assertEqual([p["game"] for p in strata["R"]], ["1"])
        self.assertEqual([p["game"] for p in strata["E"]], ["3", "4"])

    def test_a_mate_score_is_not_decided(self):
        fourth = self.strata()["E"][1]
        self.assertEqual(fourth["s"], 1700)
        self.assertNotEqual(fourth["fen"], FEN)

    def test_the_better_stratum_needs_this_move_and_the_movers_previous_one(self):
        strata = self.strata()
        self.assertEqual([p["game"] for p in strata["W"]], ["2", "2"][:1])
        self.assertEqual(strata["W"][0]["s"], 500)
        self.assertEqual(strata["W"][0]["previous"], 400)

    def test_one_position_a_game_and_stratum(self):
        strata = self.strata(skip=0, decided=4)
        games = [p["game"] for p in strata["E"]]
        self.assertEqual(len(games), len(set(games)))

    def test_the_sample_is_the_same_on_every_run(self):
        self.assertEqual(json.dumps(self.strata()), json.dumps(self.strata()))

    def test_the_skip_is_honoured(self):
        self.assertEqual([p["game"] for p in self.strata(skip=2, decided=1)["E"]], ["4"])
        self.assertEqual(len(self.strata(skip=2, decided=1)["R"]), 2)

    def test_the_six_are_the_packets(self):
        six = {p["name"]: p["fen"] for p in won_battery.SIX}
        self.assertEqual(len(six), 6)
        self.assertEqual(six["rec1"], "7r/5R2/8/2k1PB2/8/4K3/8/8 w - - 0 86")
        self.assertEqual(six["kbnk"], "8/8/8/3k4/8/8/8/1NB1K3 w - - 0 70")

    @unittest.skipUnless(PGN.exists() and (REVIEW / "wide_positions_60.json").exists(), "ignored storage is absent")
    def test_the_sixty_skipped_are_the_reviews_and_the_sample_is_registered(self):
        review = REVIEW / "wide_positions_60.json"
        self.assertEqual(hashlib.sha256(review.read_bytes()).hexdigest()[:8], "581c8822")
        self.assertEqual(won_battery.sha256(PGN)[:8], "6931997c")
        with PGN.open(encoding="utf-8", errors="replace") as handle:
            strata = won_battery.sample(handle)
        self.assertEqual(strata["R"], json.loads(review.read_text(encoding="utf-8")))
        self.assertEqual((len(strata["E"]), len(strata["W"])), (120, 60))
        self.assertFalse({p["fen"] for p in strata["E"]} & {p["fen"] for p in strata["R"]})


def tm(soft=1000, hard=8000, elapsed=500, start=100, length=50):
    return (f"info string tm soft_ms={soft} hard_ms={hard} elapsed_ms={elapsed} dispatch_ms=0 "
            f"longest_window_start_ms={start} longest_window_ms={length}")


def line(depth, move, kind="", nodes=100):
    return f"info depth {depth} seldepth {depth} multipv 1 score cp 900{kind} nodes {nodes} nps 1 time 1 pv {move} a7a8"


class Parse(unittest.TestCase):
    def row(self, lines, wall=500.0):
        return won_battery.parse_search(lines, wall)

    def test_best_move_changes_count_completed_iterations_only(self):
        row = self.row([
            line(1, "e2e4"), line(2, "d2d4"), line(3, "d2d4"),
            line(4, "g1f3", " lowerbound"),
            line(4, "e2e4"),
            "info string diag kv fhstop_fired=0 interior_nodes=70 qs_nodes=30", tm(),
            "bestmove e2e4",
        ])
        self.assertEqual((row["iterations"], row["changes"]), (4, 2))
        self.assertEqual((row["depth"], row["bestmove"], row["nodes"]), (4, "e2e4", 100))

    def test_a_repeated_line_of_one_depth_is_one_iteration(self):
        row = self.row([line(1, "e2e4"), line(2, "d2d4"), line(2, "d2d4"), tm(), "bestmove d2d4"])
        self.assertEqual((row["iterations"], row["changes"]), (2, 1))

    def classify(self, start, length, fired=0, wall=None, soft=1000, hard=8000):
        wall = start + length if wall is None else wall
        return self.row([line(1, "e2e4"), f"info string diag kv fhstop_fired={fired} interior_nodes=1 qs_nodes=1",
                         tm(soft=soft, hard=hard, elapsed=int(wall), start=start, length=length), "bestmove e2e4"], wall)["hole"]

    def test_the_hole_ends_after_twice_the_optimum_and_splits_by_its_start(self):
        self.assertEqual(self.classify(900, 1101), "before")
        self.assertEqual(self.classify(1000, 1001), "after")
        self.assertEqual(self.classify(1500, 6500), "after")

    def test_the_hole_at_exactly_twice_the_optimum_is_not_one(self):
        self.assertIsNone(self.classify(900, 1100))

    def test_a_search_the_stop_ended_is_not_a_hole(self):
        self.assertIsNone(self.classify(900, 5000, fired=1))

    def test_at_the_maximum_is_a_hole_where_the_maximum_is_under_twice_the_optimum(self):
        self.assertEqual(self.classify(100, 1800, soft=1000, hard=1900), "before")
        self.assertIsNone(self.classify(100, 1700, soft=1000, hard=1900))

    def test_without_the_counter_a_late_search_is_still_classed_by_its_start(self):
        row = self.row([line(1, "e2e4"), tm(soft=1000, hard=8000, elapsed=5000, start=900, length=4100), "bestmove e2e4"], 5000.0)
        self.assertIsNone(row["fired"])
        self.assertEqual(row["hole"], "before")

    def test_a_late_search_whose_longest_ended_early_is_named_apart(self):
        self.assertEqual(self.classify(100, 1000, wall=2500), "other")

    @unittest.skipUnless((REVIEW / "wide_60.json").exists(), "ignored storage is absent")
    def test_the_reviews_rows_read_as_the_amendment_says(self):
        expected = {("wide_60.json", "on"): (4, 4), ("wide_60.json", "off"): (2, 0),
                    ("wide_10.json", "on"): (2, 5), ("wide_10.json", "off"): (2, 0)}
        for (name, arm), counts in expected.items():
            rows = json.loads((REVIEW / name).read_text(encoding="utf-8"))["rows"]
            classes = [won_battery.hole_class(r["soft_ms"], r["hard_ms"], r["longest_start_ms"], r["longest_ms"], r["elapsed_ms"], 0)
                       for r in rows if r["arm"] == arm]
            self.assertEqual((classes.count("before"), classes.count("after")), counts, (name, arm))


class Summary(unittest.TestCase):
    ROWS = [
        {"stratum": "E", "answered": True, "elapsed_ms": 100, "soft_ms": 50, "hard_ms": 1000, "iterations": 10, "changes": 1, "hole": None, "fired": 0},
        {"stratum": "E", "answered": True, "elapsed_ms": 501, "soft_ms": 50, "hard_ms": 1000, "iterations": 10, "changes": 3, "hole": "before", "fired": 0},
        {"stratum": "E", "answered": True, "elapsed_ms": 950, "soft_ms": 50, "hard_ms": 1000, "iterations": 20, "changes": 0, "hole": "after", "fired": 0},
        {"stratum": "E", "answered": True, "elapsed_ms": 500, "soft_ms": 600, "hard_ms": 1000, "iterations": 10, "changes": 0, "hole": None, "fired": 1},
        {"stratum": "E", "answered": False, "elapsed_ms": None, "soft_ms": None, "hard_ms": None, "iterations": 0, "changes": 0, "hole": None, "fired": None},
        {"stratum": "W", "answered": True, "elapsed_ms": 40, "soft_ms": 50, "hard_ms": 1000, "iterations": 4, "changes": 2, "hole": None, "fired": 0},
    ]

    def test_the_columns_of_a_stratum(self):
        table = won_battery.summarize(self.ROWS)
        self.assertEqual(table["E"], {
            "searches": 5, "unanswered": 1, "over_half": 2, "at_max": 1, "spend_ms": 2051, "past_optimum": 3,
            "changes_per_iteration": 0.08, "hole_before": 1, "hole_after": 1, "hole_other": 0, "fired": 1})
        self.assertEqual(table["W"]["changes_per_iteration"], 0.5)
        self.assertEqual(table["W"]["over_half"], 0)

    def test_a_read_without_diag_says_it_did_not_read_the_stop(self):
        rows = [{**row, "fired": None} for row in self.ROWS]
        self.assertIsNone(won_battery.summarize(rows)["E"]["fired"])


def column(over_half, spend):
    return {"E": {"over_half": over_half, "spend_ms": spend}, "W": {"over_half": 0, "spend_ms": 100}}


class Gate(unittest.TestCase):
    BASE = (column(5, 1000), column(3, 900))

    def test_an_option_within_both_tolerances_passes(self):
        self.assertEqual(won_battery.differential_gate(*self.BASE, column(7, 1100))["passes"], True)

    def test_the_count_is_read_against_the_larger_run(self):
        verdict = won_battery.differential_gate(*self.BASE, column(8, 1000))
        self.assertFalse(verdict["passes"])
        self.assertFalse(verdict["strata"]["E"]["count_ok"])
        self.assertTrue(verdict["strata"]["W"]["count_ok"])

    def test_the_spend_is_read_against_the_larger_run(self):
        verdict = won_battery.differential_gate(*self.BASE, column(5, 1101))
        self.assertFalse(verdict["passes"])
        self.assertFalse(verdict["strata"]["E"]["spend_ok"])

    def test_every_stratum_must_pass(self):
        worse = column(5, 1000)
        worse["W"]["over_half"] = 3
        self.assertFalse(won_battery.differential_gate(*self.BASE, worse)["passes"])

    def test_a_column_without_a_stratum_does_not_pass(self):
        partial = {"E": column(5, 1000)["E"]}
        self.assertFalse(won_battery.differential_gate(*self.BASE, partial)["passes"])


UCI_HEAD = textwrap.dedent("""\
    import sys
    for line in sys.stdin:
        if line.startswith('uci'):
            print('option name Diag type check default false')
            print('option name TM_Debug type check default false')
            print('option name Wanted type spin default 0 min 0 max 9')
            print('uciok', flush=True)
        elif line.startswith('isready'):
            print('readyok', flush=True)
""")
ANSWERS = UCI_HEAD + textwrap.indent(textwrap.dedent("""\
    elif line.startswith('go'):
        print('info depth 1 seldepth 1 multipv 1 score cp 900 nodes 7 nps 1 time 1 pv a1a2', flush=True)
        print('info string diag kv fhstop_fired=0 interior_nodes=5 qs_nodes=2', flush=True)
        print('info string tm soft_ms=10 hard_ms=80 elapsed_ms=1 dispatch_ms=0 longest_window_start_ms=0 longest_window_ms=1', flush=True)
        print('bestmove a1a2', flush=True)
    elif line.startswith('quit'):
        break
"""), "    ")
NEVER = UCI_HEAD + textwrap.indent(textwrap.dedent("""\
    elif line.startswith('go'):
        print('info depth 1 seldepth 1 multipv 1 score cp 900 nodes 7 nps 1 time 1 pv a1a2', flush=True)
"""), "    ")
# Answers only when Diag was not set, or only when it was: Diag slows a search, and a clock read must not carry it unasked.
NO_DIAG = UCI_HEAD + textwrap.indent(textwrap.dedent("""    elif line.startswith('setoption name Diag'):
        sys.exit(4)
    elif line.startswith('go'):
        print('info string tm soft_ms=10 hard_ms=80 elapsed_ms=1 dispatch_ms=0 longest_window_start_ms=0 longest_window_ms=1', flush=True)
        print('bestmove a1a2', flush=True)
    elif line.startswith('quit'):
        break
"""), "    ")
NEEDS_DIAG = UCI_HEAD + textwrap.indent(textwrap.dedent("""    elif line.startswith('setoption name Diag value true'):
        seen = True
    elif line.startswith('go') and 'seen' in globals():
        print('info string diag kv fhstop_fired=1 interior_nodes=5 qs_nodes=2', flush=True)
        print('info string tm soft_ms=10 hard_ms=80 elapsed_ms=1 dispatch_ms=0 longest_window_start_ms=0 longest_window_ms=1', flush=True)
        print('bestmove a1a2', flush=True)
    elif line.startswith('quit'):
        break
"""), "    ")
DIES_AT_GO = UCI_HEAD + textwrap.indent("elif line.startswith('go'):\n    sys.exit(3)\n", "    ")
SILENT = "import sys\nfor line in sys.stdin:\n    pass\n"
WANTS_OPTION = UCI_HEAD + textwrap.indent(textwrap.dedent("""\
    elif line.startswith('setoption name Wanted value 7'):
        seen = True
    elif line.startswith('go') and 'seen' in globals():
        print('info string diag kv fhstop_fired=0 interior_nodes=5 qs_nodes=2', flush=True)
        print('info string tm soft_ms=10 hard_ms=80 elapsed_ms=1 dispatch_ms=0 longest_window_start_ms=0 longest_window_ms=1', flush=True)
        print('bestmove a1a2', flush=True)
    elif line.startswith('quit'):
        break
"""), "    ")


class Engine(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.directory = pathlib.Path(directory.name)

    def search(self, source, watchdog=20.0, options=(), **keywords):
        script = self.directory / "engine.py"
        script.write_text(source, encoding="utf-8")
        return won_battery.search([sys.executable, str(script)], FEN, 60000, 600, list(options), 64, watchdog, **keywords)

    def test_an_engine_that_answers_gives_a_row_with_its_nodes(self):
        row = self.search(ANSWERS)
        self.assertTrue(row["answered"])
        self.assertEqual((row["bestmove"], row["nodes"], row["soft_ms"], row["exit"]), ("a1a2", 7, 10, 0))

    def test_a_clock_read_does_not_turn_diag_on(self):
        row = self.search(NO_DIAG, watchdog=5.0)
        self.assertTrue(row["answered"])
        self.assertIsNone(row["fired"])

    def test_diag_is_on_only_when_asked_for(self):
        self.assertFalse(self.search(NEEDS_DIAG, watchdog=1.0)["answered"])
        row = self.search(NEEDS_DIAG, watchdog=5.0, diag=True)
        self.assertTrue(row["answered"])
        self.assertEqual((row["fired"], row["nodes"]), (1, 7))

    def test_an_engine_that_never_answers_is_counted_and_does_not_hang(self):
        row = self.search(NEVER, watchdog=1.0)
        self.assertFalse(row["answered"])
        self.assertIsNone(row["bestmove"])

    def test_an_engine_silent_from_the_start_does_not_hang(self):
        self.assertFalse(self.search(SILENT, watchdog=1.0)["answered"])

    def test_an_engine_that_dies_is_counted(self):
        self.assertFalse(self.search(DIES_AT_GO)["answered"])

    def test_an_option_reaches_the_search(self):
        self.assertTrue(self.search(WANTS_OPTION, options=["Wanted=7"])["answered"])
        self.assertFalse(self.search(WANTS_OPTION, watchdog=1.0, options=["Wanted=8"])["answered"])

    def tool(self, source, *extra):
        script = self.directory / "engine.py"
        script.write_text(source, encoding="utf-8")
        launcher = self.directory / "launch.json"
        launcher.write_text(json.dumps([sys.executable, str(script)]), encoding="utf-8")
        positions = self.directory / "positions.json"
        positions.write_text(json.dumps({"pgn_sha256": "x", "strata": {"E": [{"game": "1", "fen": FEN, "s": 2000, "men": 3}], "W": [], "R": []}}), encoding="utf-8")
        out = self.directory / "out.json"
        code = subprocess.run([sys.executable, won_battery.__file__, "read", "--command-json", str(launcher), "--positions", str(positions),
                               "--clock", "60000+600", "--strata", "E", "--out", str(out), "--label", "t", *extra],
                              capture_output=True, text=True, timeout=120).returncode
        return code, json.loads(out.read_text(encoding="utf-8")) if out.exists() else None

    def test_the_tool_exits_one_and_keeps_the_rows_on_an_unanswered_search(self):
        code, document = self.tool(NEVER, "--watchdog", "1")
        self.assertEqual(code, 1)
        self.assertEqual(document["summary"]["E"]["unanswered"], 1)
        self.assertEqual(len(document["rows"]), 1)

    def test_the_tool_exits_zero_and_records_its_conditions(self):
        code, document = self.tool(ANSWERS)
        self.assertEqual(code, 0)
        self.assertEqual((document["wtime"], document["inc"], document["hash"], document["threads"]), (60000, 600, 64, 1))
        self.assertEqual(document["summary"]["E"]["searches"], 1)

    def test_the_tool_records_whether_diag_was_on(self):
        code, document = self.tool(NO_DIAG)
        self.assertEqual((code, document["diag"]), (0, False))
        self.assertIsNone(document["summary"]["E"]["fired"])
        code, document = self.tool(NEEDS_DIAG, "--diag")
        self.assertEqual((code, document["diag"]), (0, True))
        self.assertEqual(document["summary"]["E"]["fired"], 1)

    def test_an_option_the_engine_does_not_advertise_is_refused_before_any_search(self):
        code, document = self.tool(ANSWERS, "--option", "Absent=1")
        self.assertEqual(code, 2)
        self.assertIsNone(document)


if __name__ == "__main__":
    unittest.main()
