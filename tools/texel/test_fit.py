#!/usr/bin/env python3
"""The fit's convergence and travel rules, on data whose optimum is known (T1).

    BASILISK_TEXEL=<basilisk-texel binary> python tools/texel/test_fit.py

One coordinate is free, ``PawnIslandsMg``, which no clamp touches. Each
position's label is drawn so that the loss is least with that coordinate 60
centipawns above its seed: a run with room to travel must land there, and a
schedule too short to carry it 60 (lr 0.3 for 100 epochs) must exit 3 rather
than report the point it stopped at. Needs python-chess and numpy; the driver's
end-to-end run is recorded in PLAN, not repeated here.
"""

from __future__ import annotations

import math
import os
import random
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

TUNER = str(Path(os.environ["BASILISK_TEXEL"]).resolve()) if os.environ.get("BASILISK_TEXEL") else ""
TARGET = "PawnIslandsMg"
SHIFT = 60
K = 1.0
COPIES = 40


def positions(count: int, seed: int) -> list[str]:
    """Middlegame positions from random games, both sides with pawns, so that
    no override or verdict can take a row out of the fit."""
    import chess
    rng = random.Random(seed)
    out: list[str] = []
    while len(out) < count:
        board = chess.Board()
        for _ in range(rng.randint(10, 40)):
            moves = list(board.legal_moves)
            if not moves or board.is_game_over():
                break
            board.push(rng.choice(moves))
        if board.is_game_over() or board.is_check():
            continue
        if not board.pieces(chess.PAWN, chess.WHITE) or not board.pieces(chess.PAWN, chess.BLACK):
            continue
        if len(board.piece_map()) < 16:
            continue
        out.append(board.fen())
    return out


def write_rows(path: Path, rows: list[tuple[str, str]]) -> None:
    lines = ["# basilisk-texel-rows-v1 test", "fen,result,game_id,start_id,ply,phase"]
    lines += [f"{fen},{result},{k:016x},{k:016x},0,0" for k, (fen, result) in enumerate(rows)]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def tuner(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([TUNER, *args], capture_output=True, text=True, check=False)


@unittest.skipUnless(TUNER, "set BASILISK_TEXEL to a basilisk-texel binary")
class Convergence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import numpy as np
        import surface
        cls.tmp = tempfile.TemporaryDirectory()
        root = Path(cls.tmp.name)
        registry = surface.registry(Path(TUNER))
        cls.slot = next(r.index for r in registry if r.group == TARGET)
        cls.seed_value = registry[cls.slot].seed

        # One free linear slot, every other slot held.
        rows = surface.read_manifest(surface.DEFAULT_MANIFEST)
        for row in rows:
            if row.stage == "linear" and row.index != cls.slot:
                row.fixed("held for the convergence test")
            row.anchor = ""
        cls.surface = root / "surface.tsv"
        surface.write_manifest(rows, cls.surface)

        def labelled(fens: list[str], name: str) -> Path:
            probe = root / f"{name}-probe.csv"
            write_rows(probe, [(fen, "0.5") for fen in fens])
            played_file, coeff_file = root / f"{name}.played", root / f"{name}.f64"
            assert tuner("--dump-played", str(probe), str(played_file)).returncode == 0
            assert tuner("--dump-coeffs", str(probe), str(coeff_file)).returncode == 0
            played = [int(x) for x in played_file.read_text().split()]
            coeff = np.fromfile(coeff_file, dtype="<f8").reshape(len(played), -1)[:, cls.slot]
            assert len(played) == len(fens), "a probe row was left out of the fit"
            rows_out = []
            for fen, score, c in zip(fens, played, coeff):
                p = 1.0 / (1.0 + math.exp(-K * (score + SHIFT * c) / 400.0))
                wins = round(p * COPIES)
                rows_out += [(fen, "1")] * wins + [(fen, "0")] * (COPIES - wins)
            path = root / f"{name}.csv"
            write_rows(path, rows_out)
            return path

        cls.train = labelled(positions(1500, 1), "train")
        cls.holdout = labelled(positions(800, 2), "holdout")
        cls.root = root

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def fit(self, lr: str, epochs: str, name: str) -> tuple[subprocess.CompletedProcess, Path]:
        out = self.root / f"{name}.txt"
        done = tuner("--surface", str(self.surface), "--tune", "surface", str(self.train),
                     str(self.holdout), str(out), "--K", str(K), "--lr", lr, "--epochs", epochs,
                     "--patience", "30", "--max-positions", "0")
        return done, out

    def test_a_run_with_room_reaches_the_optimum(self):
        done, out = self.fit("1.0", "600", "room")
        self.assertEqual(done.returncode, 0, done.stdout[-2000:] + done.stderr)
        value = next(int(line.split()[2]) for line in out.read_text().splitlines()
                     if line.startswith(f"{TARGET} 0 "))
        self.assertLessEqual(abs(value - (self.seed_value + SHIFT)), 4,
                             f"reached {value}, the optimum is {self.seed_value + SHIFT}")

    def test_a_schedule_that_cannot_reach_it_exits_3(self):
        done, out = self.fit("0.3", "100", "short")
        self.assertEqual(done.returncode, 3, done.stdout[-2000:] + done.stderr)
        self.assertIn("FATAL", done.stderr)
        self.assertFalse(out.exists(), "an unconverged run must not write a vector")


if __name__ == "__main__":
    unittest.main()
