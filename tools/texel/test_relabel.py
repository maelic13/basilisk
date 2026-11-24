"""Tests for relabel.py: the rule-50 verdict and corpus B's identity with A.

The verdict tests use a fake tablebase, so they need no tables. The build and
check tests need the local Syzygy set (BASILISK_SYZYGY, or the default path)
and are skipped without it.
"""
import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path

import chess
import chess.syzygy

import extract
import relabel

SYZYGY = os.environ.get("BASILISK_SYZYGY", "D:/chess/tablebases/syzygy3456")
KRK = "8/8/8/4k3/8/8/8/R3K3 w - - {clock} 60"


class FakeTb:
    def __init__(self, wdl, dtz, missing=False):
        self.wdl, self.dtz, self.missing = wdl, dtz, missing

    def probe_wdl(self, board):
        if self.missing:
            raise chess.syzygy.MissingTableError("no table")
        return self.wdl

    def probe_dtz(self, board):
        return self.dtz


def board(clock=0, turn="w"):
    return chess.Board(KRK.format(clock=clock).replace(" w ", f" {turn} "))


class Verdict(unittest.TestCase):
    def test_clean_win_inside_the_clock(self):
        self.assertEqual(relabel.verdict(board(95), FakeTb(2, 5))[:2], (1.0, "win"))

    def test_the_clock_turns_a_clean_win_into_a_draw(self):
        self.assertEqual(relabel.verdict(board(96), FakeTb(2, 5))[:2], (0.5, "clock"))

    def test_loss_inside_and_outside_the_clock(self):
        self.assertEqual(relabel.verdict(board(95), FakeTb(-2, -5))[:2], (0.0, "loss"))
        self.assertEqual(relabel.verdict(board(96), FakeTb(-2, -5))[:2], (0.5, "clock"))

    def test_cursed_and_drawn(self):
        self.assertEqual(relabel.verdict(board(0), FakeTb(1, 105))[:2], (0.5, "cursed"))
        self.assertEqual(relabel.verdict(board(0), FakeTb(-1, -103))[:2], (0.5, "cursed"))
        self.assertEqual(relabel.verdict(board(0), FakeTb(0, 0))[:2], (0.5, "draw"))

    def test_points_are_whites(self):
        # Black to move and winning is a White loss.
        self.assertEqual(relabel.verdict(board(0, "b"), FakeTb(2, 7))[:2], (0.0, "win"))
        self.assertEqual(relabel.verdict(board(0, "b"), FakeTb(-2, -7))[:2], (1.0, "loss"))

    def test_ineligible_rows(self):
        self.assertEqual(relabel.verdict(chess.Board(), FakeTb(2, 1))[:2], (None, "too_many_men"))
        castle = chess.Board("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1")
        self.assertEqual(relabel.verdict(castle, FakeTb(2, 1))[:2], (None, "castling"))
        self.assertEqual(relabel.verdict(board(0), FakeTb(2, 1, missing=True))[:2],
                         (None, "missing"))


@unittest.skipUnless(Path(SYZYGY).is_dir(), "local Syzygy tables not found")
class BuildAndCheck(unittest.TestCase):
    ROWS = [
        # (fen, A's label, phase): KRK, White to move, a clean win; labelled a draw in A.
        ("8/8/8/4k3/8/8/8/R3K3 w - - 0 60", "0.5", 2),
        # The same with the clock at 99: the win cannot be forced in one ply.
        ("8/8/8/4k3/8/8/8/R3K3 w - - 99 60", "1", 2),
        # KK-equivalent draw: KNK.
        ("8/8/8/4k3/8/8/8/N3K3 w - - 0 60", "1", 1),
        # More than six men: never relabelled.
        ("r3k3/pp6/8/8/8/8/PP6/R3K3 w - - 0 30", "0", 8),
        # A castling right: unknown, kept.
        ("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", "0", 2),
    ]

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.a = self.tmp / "A"
        self.a.mkdir()
        output = {}
        for split in extract.SPLITS:
            path = self.a / f"{split}.csv"
            with path.open("w", encoding="utf-8", newline="\n") as out:
                out.write(f"# {extract.SCHEMA} {split}\n")
                out.write(",".join(extract.COLUMNS) + "\n")
                for i, (fen, label, phase) in enumerate(self.ROWS):
                    out.write(f"{fen},{label},{i:016x},{i:016x},{10 + i},{phase}\n")
            output[split] = {"file": path.name, "rows": len(self.ROWS),
                             "sha256": extract.sha256_file(path)}
        manifest = {"schema": extract.SCHEMA, "columns": list(extract.COLUMNS),
                    "split_rule": "test", "dedup_fields": 5, "seed": 1, "parameters": {},
                    "games_read": 5, "games_by_split": {}, "games_rejected": {},
                    "positions": {}, "phase_quotas": {}, "inputs": [], "output": output}
        (self.a / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
        self.b = self.tmp / "B"

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def labels(self, split="train"):
        lines = (self.b / f"{split}.csv").read_text(encoding="utf-8").splitlines()[2:]
        return [line.split(",")[1] for line in lines]

    def test_build_relabels_only_eligible_rows(self):
        self.assertEqual(relabel.build(self.a, self.b, SYZYGY, 1), 0)
        labels = self.labels()
        self.assertEqual(labels[0], "1")      # clean KRK win
        self.assertEqual(labels[1], "0.5")    # the clock at 99 draws it
        self.assertEqual(labels[2], "0.5")    # KNK
        self.assertEqual(labels[3], "0")      # eight men: A's label
        self.assertEqual(labels[4], "0")      # castling: A's label
        manifest = json.loads((self.b / "manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["label_contract"], relabel.LABEL_CONTRACT)
        counts = manifest["relabel"]["train"]["counts"]
        self.assertEqual((counts["eligible"], counts["changed"], counts["castling"]), (3, 3, 1))
        self.assertEqual(relabel.build(self.a, self.tmp / "B2", SYZYGY, 1), 0)
        self.assertEqual((self.b / "train.csv").read_bytes(),
                         (self.tmp / "B2" / "train.csv").read_bytes())

    def test_refuses_to_overwrite(self):
        self.assertEqual(relabel.build(self.a, self.b, SYZYGY, 1), 0)
        with self.assertRaises(SystemExit):
            relabel.build(self.a, self.b, SYZYGY, 1)

    def tamper(self, row, column, value):
        path = self.b / "train.csv"
        lines = path.read_text(encoding="utf-8").split("\n")
        fields = lines[2 + row].split(",")
        fields[column] = value
        lines[2 + row] = ",".join(fields)
        path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
        manifest_path = self.b / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["output"]["train"]["sha256"] = extract.sha256_file(path)
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")

    def test_check_catches_a_changed_column(self):
        self.assertEqual(relabel.build(self.a, self.b, SYZYGY, 1), 0)
        self.tamper(2, 4, "99")
        self.assertEqual(relabel.check(self.a, self.b), 1)

    def test_check_catches_a_large_row_relabelled(self):
        self.assertEqual(relabel.build(self.a, self.b, SYZYGY, 1), 0)
        self.tamper(3, 1, "1")
        self.assertEqual(relabel.check(self.a, self.b), 1)

    def test_check_catches_a_wrong_verdict(self):
        self.assertEqual(relabel.build(self.a, self.b, SYZYGY, 1), 0)
        self.tamper(0, 1, "0")
        # Still a changed label, so the count agrees: the re-probe, which
        # samples every seventh row from the first, is what catches it.
        self.assertEqual(relabel.check(self.a, self.b, SYZYGY), 1)


if __name__ == "__main__":
    unittest.main()
