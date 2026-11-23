#!/usr/bin/env python3
"""Tests for piece_values.py, and for fit.py's seed, K and stage handling.

The tuner tests need a traced tuner:
    BASILISK_TEXEL=<basilisk-texel binary> python tools/texel/test_piece_values.py
"""

from __future__ import annotations

import os
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

import chess

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import bake  # noqa: E402
import fit  # noqa: E402
import piece_values as pv  # noqa: E402

TUNER = str(Path(os.environ["BASILISK_TEXEL"]).resolve()) if os.environ.get("BASILISK_TEXEL") else ""
HEADER_TEXT = (HERE.parents[1] / "src" / "eval_params.h").read_text(encoding="utf-8")

ROWS = [
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "r3k2r/ppp2ppp/2n1bn2/3pp3/3PP3/2N1BN2/PPP2PPP/R3K2R w KQkq - 0 9",
    "8/5pk1/6p1/8/3R4/6P1/5PK1/8 w - - 0 40",
    "8/8/4k3/8/2B5/4K3/5P2/8 w - - 0 50",
    "6k1/5ppp/8/8/8/8/1Q3PPP/6K1 w - - 0 30",
]


def rows_file(directory: Path) -> Path:
    path = directory / "rows.csv"
    lines = ["# basilisk-texel-rows-v1 test", "fen,result,game_id,start_id,ply,phase"]
    lines += [f"{fen},0.5,g{i},s{i},10,12" for i, fen in enumerate(ROWS * 20)]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return path


class Pairs(unittest.TestCase):
    def test_phase(self) -> None:
        self.assertEqual(pv.phase_of(chess.Board()), 24)
        self.assertEqual(pv.phase_of(chess.Board("8/8/4k3/8/8/4K3/5P2/8 w - - 0 1")), 0)

    def test_a_pair_removes_exactly_one_white_piece_of_its_kind(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            _, rows = pv.read_rows(rows_file(Path(tmp)))
        pairs = pv.build_pairs(rows, 50, 7)
        self.assertTrue(pairs)
        for pair in pairs:
            full, reduced = chess.Board(pair["full"]), chess.Board(pair["reduced"])
            kind = pv.PIECES[pair["piece"]]
            self.assertEqual(len(full.pieces(kind, chess.WHITE)) - 1,
                             len(reduced.pieces(kind, chess.WHITE)))
            self.assertEqual(len(full.piece_map()) - 1, len(reduced.piece_map()))
            self.assertTrue(reduced.is_valid() and not reduced.is_check())

    def test_pairs_are_seeded(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            _, rows = pv.read_rows(rows_file(Path(tmp)))
        self.assertEqual(pv.build_pairs(rows, 30, 3), pv.build_pairs(rows, 30, 3))

    def test_a_short_evaluation_list_is_refused(self) -> None:
        pairs = [{"piece": "pawn", "phase": 20}] * 3
        with self.assertRaises(ValueError):
            pv.medians(pairs, [1, 2, 3], [1, 2])

    def test_medians_by_cell(self) -> None:
        pairs = [{"piece": "rook", "phase": 2}] * pv.MIN_PER_CELL
        table = pv.medians(pairs, [600] * pv.MIN_PER_CELL, [100] * pv.MIN_PER_CELL)
        self.assertEqual(table, {"rook phase 0-7": {"median": 500, "n": pv.MIN_PER_CELL}})

    def test_vector_specs(self) -> None:
        self.assertEqual(pv.parse_vector("header"), ("header", None))
        self.assertEqual(pv.parse_vector("a=x.txt"), ("a", Path("x.txt")))
        with self.assertRaises(ValueError):
            pv.parse_vector("x.txt")


class FitDriver(unittest.TestCase):
    def seed_file(self, directory: Path, root: str | None = None, drop: int = 0) -> Path:
        params = bake.parse_param_list(HEADER_TEXT)
        vector = bake.header_vector(HEADER_TEXT, params)
        path = directory / "seed.txt"
        lines = [f"# {bake.VECTOR_SCHEMA}", f"# root {root or bake.vector_hash(vector)}"]
        lines += [f"{n} {i} {v}" for n, i, v in vector[drop:]]
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return path

    def test_a_seed_rooted_at_this_header_loads(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            vector = fit.load_seed(self.seed_file(Path(tmp)), HEADER_TEXT)
        self.assertEqual(len(vector), len(bake.header_vector(HEADER_TEXT, bake.parse_param_list(HEADER_TEXT))))

    def test_a_seed_from_another_header_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, self.assertRaisesRegex(fit.FitError, "root"):
            fit.load_seed(self.seed_file(Path(tmp), root="0" * 16), HEADER_TEXT)

    def test_an_incomplete_seed_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, self.assertRaisesRegex(fit.FitError, "missing"):
            fit.load_seed(self.seed_file(Path(tmp), drop=1), HEADER_TEXT)

    def test_the_written_seed_traces_back_to_this_header(self) -> None:
        # The dry run that found it: a seed written without its root made the
        # tuner root the chain at the seed, and bake.py refused the result.
        with tempfile.TemporaryDirectory() as tmp:
            vector = fit.load_seed(self.seed_file(Path(tmp)), HEADER_TEXT)
            written = Path(tmp) / "00-seed.txt"
            fit.write_seed(written, vector, HEADER_TEXT)
            text = written.read_text(encoding="utf-8")
        errors = bake.check_dump(text, bake.parse_param_list(HEADER_TEXT), HEADER_TEXT)
        self.assertEqual(errors, [])

    def args(self, **kw):
        base = dict(lr=0.3, patience=50, epochs=20000, min_delta=1e-6, max_positions=0,
                    l2=0.0, smooth=0.0, ks_positions=100000)
        return SimpleNamespace(**(base | kw))

    def command(self, stage: str, **kw):
        return fit.stage_command(stage, Path("tuner"), ["--init", "s"], Path("t"), Path("v"),
                                 Path("o"), 2.0, self.args(**kw))

    def test_the_registered_linear_stage(self) -> None:
        cmd, settings = self.command("linear")
        self.assertEqual(settings["lr"], 0.3)
        self.assertEqual(settings["patience"], 50)
        self.assertEqual(cmd[cmd.index("--min-delta") + 1], "1e-06")
        self.assertEqual(cmd[cmd.index("--epochs") + 1], "20000")

    def test_the_polish_runs_at_a_quarter_rate_and_twice_the_patience(self) -> None:
        cmd, settings = self.command("polish")
        self.assertEqual(settings["lr"], 0.075)
        self.assertEqual(settings["patience"], 100)
        self.assertIn("--min-delta", cmd)

    def test_no_tolerance_passes_none(self) -> None:
        cmd, _ = self.command("linear", min_delta=0.0)
        self.assertNotIn("--min-delta", cmd)

    def test_the_king_safety_stage(self) -> None:
        cmd, settings = self.command("kingsafety")
        self.assertIn("--tune-kingsafety", cmd)
        self.assertEqual(settings, {"max_positions": 100000})

    def test_rank_counts_support_on_the_linear_stages_rows(self) -> None:
        cmd = fit.rank_command(["surface.py"], Path("tuner"), Path("m.tsv"), Path("train.csv"),
                               Path("d.tsv"), self.args(max_positions=1234, rank_positions=300000,
                                                        min_support=200))
        self.assertEqual(cmd[cmd.index("--support-positions") + 1], "1234")
        self.assertEqual(cmd[cmd.index("--rank-positions") + 1], "300000")
        self.assertNotIn("--max-positions", cmd)

    def test_the_support_record(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            derived = Path(tmp) / "d.tsv"
            derived.write_text(
                "# basilisk-texel-surface-v1 test\n"
                "index\tgroup\tsub\tseed\tclass\tstage\tanchor\tlo\thi\tstep\treason\n"
                "0\tMobQMg\t0\t0\tfixed\t\t\t\t\t\tfrozen: 10 rows of support, under 200, "
                "on 3547892 rows of train.csv\n"
                "1\tMobQMg\t1\t0\tfree\tlinear\tmean:MobQMg\t\t\t\ta traced linear term\n",
                encoding="utf-8")
            record = fit.support_record(derived)
        self.assertEqual(record, {"rows_counted": 3547892, "min_support": 200,
                                  "frozen": [{"slot": "MobQMg[0]", "support": 10}], "free_linear": 1})

    def test_loss_records_carry_the_lazy_share(self) -> None:
        text = ("v.csv: 2000 rows used (227 at the lazy checkpoint), 115 overrides\n"
                "rows: 2000\nK: 2.009866\nloss: 0.09982406\n")
        loss = fit.parse_loss(text)
        self.assertEqual(loss["lazy_rows"], 227)
        self.assertAlmostEqual(loss["lazy_share"], 0.1135)
        self.assertEqual(loss["K"], 2.009866)


@unittest.skipUnless(TUNER, "set BASILISK_TEXEL to a basilisk-texel binary")
class WithTheTuner(unittest.TestCase):
    def setUp(self) -> None:
        # Five distinct positions cannot fill a cell of the production minimum.
        self.saved, pv.MIN_PER_CELL = pv.MIN_PER_CELL, 1

    def tearDown(self) -> None:
        pv.MIN_PER_CELL = self.saved

    def test_header_against_itself_and_a_queen_is_worth_a_lot(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            rows = rows_file(Path(tmp))
            out = Path(tmp) / "pv.json"
            self.assertEqual(pv.main(["--tuner", TUNER, "--rows", str(rows), "--vector", "header",
                                      "--vector", "again=" + str(self.header_vector(Path(tmp))),
                                      "--positions", "100", "--out", str(out)]), 0)
            import json
            report = json.loads(out.read_text(encoding="utf-8"))
        self.assertEqual(report["vectors"]["header"], report["vectors"]["again"])
        queens = [c["median"] for k, c in report["vectors"]["header"].items() if k.startswith("queen")]
        self.assertTrue(queens and min(queens) > 500, report["vectors"]["header"])

    def header_vector(self, directory: Path) -> Path:
        params = bake.parse_param_list(HEADER_TEXT)
        vector = bake.header_vector(HEADER_TEXT, params)
        path = directory / "header.txt"
        path.write_text(f"# {bake.VECTOR_SCHEMA}\n# root {bake.vector_hash(vector)}\n"
                        + "".join(f"{n} {i} {v}\n" for n, i, v in vector), encoding="utf-8")
        return path


if __name__ == "__main__":
    unittest.main()
