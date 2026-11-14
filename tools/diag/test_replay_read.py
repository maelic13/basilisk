#!/usr/bin/env python3
"""replay_read.py: M1's outputs from a replay's rows, by hand and against the review's replays.

The last class reads ignored storage and skips without it: twenty games of the
review, whose spend, firing rate and removed share are in the packet.
"""

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import replay_read

REPO = pathlib.Path(__file__).resolve().parents[2]
REVIEW = REPO / "tools/results/b5-20261109/review"


def row(game=1, ply=0, score="cp 0", elapsed=100, optimum=100, maximum=800, start=0, window=10, fired=0, **extra):
    base = {"game": game, "ply": ply, "score": score, "elapsed": elapsed, "wall_ms": elapsed + 1.0, "optimum": optimum, "maximum": maximum,
            "longest_window_start": start, "longest_window": window, "fhstop_fired": fired, "iterations": 10, "changes": 1,
            "root_effort_hi": 1, "root_effort_lo": 2, "asp_windows": 10, "ply_cap_hits": 0, "ply_cap_in_check": 0,
            "qs_ply_cap_hits": 0, "qs_ply_cap_in_check": 0}
    base.update(extra)
    return base


class Bands(unittest.TestCase):
    def test_the_score_bands(self):
        cases = {"cp 300": "won", "cp 299": "even", "cp -299": "even", "cp -300": "lost", "mate 4": "won", "mate -4": "lost", None: "unscored"}
        for score, band in cases.items():
            self.assertEqual(replay_read.band_of(score), band, score)


class Column(unittest.TestCase):
    ROWS = [
        row(ply=0, score="cp 500", elapsed=150, fired=1),
        row(ply=2, score="cp 500", elapsed=100),
        row(ply=4, score="cp 0", elapsed=110, fired=1),
        row(ply=6, score="cp 0", elapsed=90),
        row(ply=8, score="cp 0", elapsed=80),
        row(ply=10, score="cp -500", elapsed=70),
    ]

    def test_the_firing_rate_overall_and_by_band(self):
        column = replay_read.read_column(self.ROWS)
        self.assertEqual((column["searches"], column["fired"]), (6, 2))
        self.assertAlmostEqual(column["fired_share"], 2 / 6)
        self.assertEqual({b: (column["bands"][b]["searches"], column["bands"][b]["fired"]) for b in ("won", "even", "lost")},
                         {"won": (2, 1), "even": (3, 1), "lost": (1, 0)})

    def test_the_spend_and_the_clock_counts(self):
        rows = [row(elapsed=400, maximum=800), row(ply=2, elapsed=399, maximum=800), row(ply=4, elapsed=760, maximum=800), row(ply=6, elapsed=50, optimum=100)]
        column = replay_read.read_column(rows)
        self.assertEqual((column["spend_ms"], column["over_half"], column["at_max"], column["past_optimum"]), (1609, 2, 1, 3))

    def test_the_rest_of_a_row_is_read_by_sum(self):
        column = replay_read.read_column(self.ROWS)
        self.assertAlmostEqual(column["changes_per_iteration"], 0.1)
        self.assertAlmostEqual(column["effort_hi_share"], 0.1)
        self.assertAlmostEqual(column["effort_lo_share"], 0.2)

    def test_the_ply_cap_is_read_per_game(self):
        rows = [row(game=1, ply_cap_hits=2, ply_cap_in_check=1, qs_ply_cap_hits=1), row(game=2)]
        column = replay_read.read_column(rows)
        self.assertEqual((column["ply_cap_per_game"], column["ply_cap_in_check_share"]), (1.5, 1 / 3))


class Hole(unittest.TestCase):
    def test_the_hole_by_band_where_it_began_and_the_time_past_the_optimum(self):
        rows = [
            row(ply=0, score="cp 500", elapsed=300, start=50, window=250),
            row(ply=2, score="cp 500", elapsed=500, start=100, window=400),
            row(ply=4, score="cp 0", elapsed=250, start=150, window=100),
            row(ply=6, score="cp 0", elapsed=250, start=50, window=100),
            row(ply=8, score="cp 0", elapsed=500, start=50, window=400, fired=1),
            row(ply=10, score="cp 0", elapsed=90),
        ]
        column = replay_read.read_column(rows)
        won = column["bands"]["won"]["hole"]
        self.assertEqual((won["before"], won["after"], won["other"], won["past_optimum_ms"]), (1, 1, 0, 600))
        even = column["bands"]["even"]["hole"]
        self.assertEqual((even["before"], even["after"], even["other"], even["past_optimum_ms"]), (0, 1, 1, 300))
        self.assertAlmostEqual(column["hole_share"], 4 / 6)


class Removed(unittest.TestCase):
    STOP = [row(ply=0, score="cp 500", elapsed=100), row(ply=2, score="cp 0", elapsed=100), row(ply=4, score="cp 0", elapsed=100)]
    FREE = [row(ply=0, score="cp 500", elapsed=200), row(ply=2, score="cp 0", elapsed=150), row(ply=4, score="cp 0", elapsed=50)]

    def test_the_time_removed_is_the_paired_difference_over_the_free_spend(self):
        removed = replay_read.time_removed(self.STOP, self.FREE)
        self.assertAlmostEqual(removed["share"], 100 / 400)
        self.assertEqual(removed["removed_ms"], 100)

    def test_each_band_is_the_free_rows_band_and_its_share_of_the_whole(self):
        removed = replay_read.time_removed(self.STOP, self.FREE)["bands"]
        self.assertEqual((removed["won"]["removed_ms"], removed["even"]["removed_ms"]), (100, 0))
        self.assertAlmostEqual(removed["won"]["share_of_spend"], 100 / 400)
        self.assertAlmostEqual(removed["won"]["share_of_band"], 100 / 200)

    def test_unequal_positions_are_refused(self):
        with self.assertRaisesRegex(ValueError, "same positions"):
            replay_read.time_removed(self.STOP, self.FREE[:2])


class Rule(unittest.TestCase):
    """The stop packet's §5 rule: keep 1x under 10% of searches and 3% of spend."""

    def test_both_under_keeps(self):
        self.assertEqual(replay_read.m1_rule(0.0999, 0.0299), "keep 1x")

    def test_a_rate_at_the_bound_moves(self):
        self.assertEqual(replay_read.m1_rule(0.10, 0.01), "2x against 1x")

    def test_a_spend_at_the_bound_moves(self):
        self.assertEqual(replay_read.m1_rule(0.01, 0.03), "2x against 1x")


class Games(unittest.TestCase):
    def test_a_split_reads_the_first_games_against_the_rest(self):
        rows = [row(game=9), row(game=9, ply=2), row(game=3), row(game=5)]
        first, rest = replay_read.split_games(rows, 2)
        self.assertEqual([r["game"] for r in first], [9, 9, 3])
        self.assertEqual([r["game"] for r in rest], [5])


class Tool(unittest.TestCase):
    def test_columns_references_and_the_rule_reach_the_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            stop, free, out = root / "stop.jsonl", root / "free.jsonl", root / "o.json"
            stop.write_text("\n".join(json.dumps(r) for r in Removed.STOP) + "\n", encoding="utf-8")
            free.write_text("\n".join(json.dumps(r) for r in Removed.FREE) + "\n", encoding="utf-8")
            done = subprocess.run([sys.executable, replay_read.__file__, "--column", f"arm-1x={stop}", "--column", f"arm-0={free}",
                                   "--removed-against", "arm-1x=arm-0", "--rule", "arm-1x", "--out", str(out)],
                                  capture_output=True, text=True, timeout=60)
            self.assertEqual(done.returncode, 0, done.stderr)
            report = json.loads(out.read_text(encoding="utf-8"))
            self.assertEqual(report["columns"]["arm-1x"]["removed"]["removed_ms"], 100)
            self.assertEqual(report["rule"]["verdict"], "2x against 1x")
            self.assertIn("arm-1x", done.stdout)

    def test_a_reference_to_no_column_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            stop = pathlib.Path(directory) / "stop.jsonl"
            stop.write_text(json.dumps(row()) + "\n", encoding="utf-8")
            done = subprocess.run([sys.executable, replay_read.__file__, "--column", f"arm-1x={stop}", "--removed-against", "arm-1x=none"],
                                  capture_output=True, text=True, timeout=60)
            self.assertEqual(done.returncode, 2)


@unittest.skipUnless((REVIEW / "replay-on-100.jsonl").exists(), "ignored storage is absent")
class TheReviewsReplays(unittest.TestCase):
    def rows(self, name):
        # These rows predate the best-move changes; nothing here reads them.
        text = (REVIEW / f"replay-{name}.jsonl").read_text(encoding="utf-8")
        return [{"changes": 0, "iterations": 1, **json.loads(line)} for line in text.splitlines()]

    def test_the_packets_figures(self):
        expected = {"on-100": (90282, 10, 0.251), "on-0": (100385, 17, 0.0), "off-100": (89018, 13, 0.108), "off-0": (95289, 17, 0.0)}
        for name, (spend, at_max, share) in expected.items():
            column = replay_read.read_column(self.rows(name))
            self.assertEqual((column["searches"], column["spend_ms"], column["at_max"], round(column["fired_share"], 3)),
                             (1226, spend, at_max, share), name)

    def test_the_removed_shares(self):
        self.assertAlmostEqual(replay_read.time_removed(self.rows("on-100"), self.rows("on-0"))["share"], 0.101, places=3)
        self.assertAlmostEqual(replay_read.time_removed(self.rows("off-100"), self.rows("off-0"))["share"], 0.066, places=3)


if __name__ == "__main__":
    unittest.main()
