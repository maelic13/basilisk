#!/usr/bin/env python3
"""Tests for linear-in-phase K (--K-phase, --fit-k-phase) and fit.py's rebuild/k-mode wiring.

    BASILISK_TEXEL=<basilisk-texel binary> python tools/texel/test_k_phase.py
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
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import fit  # noqa: E402

TUNER = str(Path(os.environ["BASILISK_TEXEL"]).resolve()) if os.environ.get("BASILISK_TEXEL") else ""
TARGET = "PawnIslandsMg"
SHIFT = 50
TRUE_K_EG = 3.0
TRUE_K_MG = 1.0
COPIES = 40


def positions_by_phase(n_open: int, n_end: int, seed: int) -> list[str]:
    """Opening/middlegame (high phase) and endgame (low phase) positions, both sides with pawns."""
    import chess
    rng = random.Random(seed)
    opening, endgame = [], []
    while len(opening) < n_open:
        board = chess.Board()
        for _ in range(rng.randint(8, 22)):
            moves = list(board.legal_moves)
            if not moves or board.is_game_over():
                break
            board.push(rng.choice(moves))
        if board.is_game_over() or board.is_check():
            continue
        if len(board.piece_map()) >= 26 and board.pieces(chess.PAWN, chess.WHITE) and board.pieces(chess.PAWN, chess.BLACK):
            opening.append(board.fen())
    # Generate pawn + minor/rook endgame positions with phase <= 4
    files = list(range(8))
    while len(endgame) < n_end:
        board = chess.Board.empty()
        wk, bk = rng.sample(range(64), 2)
        if chess.square_distance(wk, bk) < 2:
            continue
        board.set_piece_at(wk, chess.Piece(chess.KING, chess.WHITE))
        board.set_piece_at(bk, chess.Piece(chess.KING, chess.BLACK))
        used = {wk, bk}
        # Give each side 1-2 pawns and 0-1 rook/knight so phase <= 4
        for col in (chess.WHITE, chess.BLACK):
            for _ in range(rng.randint(1, 2)):
                sq = rng.choice([s for s in range(8, 56) if s not in used])
                used.add(sq)
                board.set_piece_at(sq, chess.Piece(chess.PAWN, col))
        if rng.random() < 0.7:
            for col in (chess.WHITE, chess.BLACK):
                pt = rng.choice([chess.KNIGHT, chess.BISHOP, chess.ROOK])
                sq = rng.choice([s for s in range(64) if s not in used])
                used.add(sq)
                board.set_piece_at(sq, chess.Piece(pt, col))
        board.turn = rng.choice([chess.WHITE, chess.BLACK])
        if not board.is_valid() or board.is_check() or not any(board.legal_moves):
            continue
        endgame.append(board.fen())
    return opening + endgame


def write_rows(path: Path, rows: list[tuple[str, str]]) -> None:
    lines = ["# basilisk-texel-rows-v1 test", "fen,result,game_id,start_id,ply,phase"]
    lines += [f"{fen},{result},{k:016x},{k:016x},0,0" for k, (fen, result) in enumerate(rows)]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def tuner(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([TUNER, *args], capture_output=True, text=True, check=False)


class FitDriverKPhaseTests(unittest.TestCase):
    def args(self, **kw):
        base = dict(lr=0.5, patience=50, epochs=20000, min_delta=1e-6, max_positions=0,
                    l2=0.0, smooth=0.0, ks_positions=100000)
        return SimpleNamespace(**(base | kw))

    def test_stage_command_with_phase_tuple(self):
        cmd, _ = fit.stage_command("linear", Path("tuner"), ["--init", "s"], Path("t"), Path("v"),
                                   Path("o"), (3.0267, 0.9701), self.args())
        self.assertNotIn("--K", cmd)
        self.assertIn("--K-phase", cmd)
        self.assertEqual(cmd[cmd.index("--K-phase") + 1], "3.026700,0.970100")

        cmd_ks, _ = fit.stage_command("kingsafety", Path("tuner"), ["--init", "s"], Path("t"), Path("v"),
                                      Path("o"), (3.0267, 0.9701), self.args())
        self.assertIn("--K-phase", cmd_ks)
        self.assertEqual(cmd_ks[cmd_ks.index("--K-phase") + 1], "3.026700,0.970100")

    def test_parse_loss_phase_k(self):
        text = ("v.csv: 1000 rows used (100 at the lazy checkpoint), 10 overrides\n"
                "rows: 1000\nK_eg: 3.026700\nK_mg: 0.970100\nloss: 0.09466759\n")
        loss = fit.parse_loss(text)
        self.assertEqual(loss["K_eg"], 3.0267)
        self.assertEqual(loss["K_mg"], 0.9701)
        self.assertAlmostEqual(loss["loss"], 0.09466759)


@unittest.skipUnless(TUNER, "set BASILISK_TEXEL to a basilisk-texel binary")
class KPhaseTunerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import chess
        import numpy as np
        import surface
        cls.tmp = tempfile.TemporaryDirectory()
        root = Path(cls.tmp.name)
        registry = surface.registry(Path(TUNER))
        cls.slot = next(r.index for r in registry if r.group == TARGET)
        cls.seed_value = registry[cls.slot].seed

        rows = surface.read_manifest(surface.manifest_for(registry))
        for row in rows:
            if row.stage == "linear" and row.index != cls.slot:
                row.fixed("held for the k-phase test")
            row.anchor = ""
        cls.surface = root / "surface.tsv"
        surface.write_manifest(rows, cls.surface)

        weights = {chess.KNIGHT: 1, chess.BISHOP: 1, chess.ROOK: 2, chess.QUEEN: 4}

        def phase_of(fen: str) -> int:
            b = chess.Board(fen)
            return min(24, sum(weights.get(p.piece_type, 0) for p in b.piece_map().values()))

        def labelled(fens: list[str], name: str) -> Path:
            probe = root / f"{name}-probe.csv"
            write_rows(probe, [(fen, "0.5") for fen in fens])
            played_file, coeff_file = root / f"{name}.played", root / f"{name}.f64"
            assert tuner("--dump-played", str(probe), str(played_file)).returncode == 0
            assert tuner("--dump-coeffs", str(probe), str(coeff_file)).returncode == 0
            played = [int(x) for x in played_file.read_text().split()]
            coeff = np.fromfile(coeff_file, dtype="<f8").reshape(len(played), -1)[:, cls.slot]
            assert len(played) == len(fens)
            rows_out = []
            for fen, score, c in zip(fens, played, coeff):
                ph = phase_of(fen)
                k = TRUE_K_EG + (TRUE_K_MG - TRUE_K_EG) * (ph / 24.0)
                p = 1.0 / (1.0 + math.exp(-k * (score + SHIFT * c) / 400.0))
                wins = round(p * COPIES)
                rows_out += [(fen, "1")] * wins + [(fen, "0")] * (COPIES - wins)
            path = root / f"{name}.csv"
            write_rows(path, rows_out)
            return path

        cls.train = labelled(positions_by_phase(400, 400, 11), "train")
        cls.holdout = labelled(positions_by_phase(300, 300, 22), "holdout")
        cls.root = root

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_constant_k_phase_matches_scalar_k(self):
        a = tuner("--loss", str(self.holdout), "--K", "2.0")
        b = tuner("--loss", str(self.holdout), "--K-phase", "2.0,2.0")
        self.assertEqual(a.returncode, 0, a.stderr)
        self.assertEqual(b.returncode, 0, b.stderr)
        loss_a = fit.parse_loss(a.stdout)
        loss_b = fit.parse_loss(b.stdout)
        self.assertAlmostEqual(loss_a["loss"], loss_b["loss"], places=8)
        self.assertEqual(loss_a["bands"], loss_b["bands"])

    def test_fit_k_phase_recovers_slope_and_beats_single_k(self):
        # Generate rows with score at seed (shift 0) using TRUE_K_EG / TRUE_K_MG
        import chess
        probe = self.root / "kprobe.csv"
        fens = positions_by_phase(400, 400, 33)
        write_rows(probe, [(fen, "0.5") for fen in fens])
        played_file = self.root / "kprobe.played"
        self.assertEqual(tuner("--dump-played", str(probe), str(played_file)).returncode, 0)
        played = [int(x) for x in played_file.read_text().split()]
        weights = {chess.KNIGHT: 1, chess.BISHOP: 1, chess.ROOK: 2, chess.QUEEN: 4}
        rows_out = []
        for fen, score in zip(fens, played):
            ph = min(24, sum(weights.get(p.piece_type, 0) for p in chess.Board(fen).piece_map().values()))
            k = TRUE_K_EG + (TRUE_K_MG - TRUE_K_EG) * (ph / 24.0)
            p = 1.0 / (1.0 + math.exp(-k * score / 400.0))
            wins = round(p * COPIES)
            rows_out += [(fen, "1")] * wins + [(fen, "0")] * (COPIES - wins)
        kset = self.root / "kset.csv"
        write_rows(kset, rows_out)

        scalar = tuner("--loss", str(kset))
        phase = tuner("--loss", str(kset), "--fit-k-phase")
        self.assertEqual(scalar.returncode, 0, scalar.stderr)
        self.assertEqual(phase.returncode, 0, phase.stderr)
        ls = fit.parse_loss(scalar.stdout)
        lp = fit.parse_loss(phase.stdout)
        self.assertLess(lp["loss"], ls["loss"])
        self.assertAlmostEqual(lp["K_eg"], TRUE_K_EG, delta=0.15)
        self.assertAlmostEqual(lp["K_mg"], TRUE_K_MG, delta=0.15)

    def test_tune_surface_with_k_phase_reaches_optimum(self):
        out = self.root / "tuned.txt"
        done = tuner("--surface", str(self.surface), "--tune", "surface", str(self.train),
                     str(self.holdout), str(out), "--K-phase", f"{TRUE_K_EG},{TRUE_K_MG}",
                     "--lr", "1.0", "--epochs", "600", "--patience", "30", "--max-positions", "0")
        self.assertEqual(done.returncode, 0, done.stdout[-2000:] + done.stderr)
        value = next(int(line.split()[2]) for line in out.read_text().splitlines()
                     if line.startswith(f"{TARGET} 0 "))
        self.assertLessEqual(abs(value - (self.seed_value + SHIFT)), 4,
                             f"reached {value}, expected {self.seed_value + SHIFT}")

    def test_tune_kingsafety_accepts_k_phase(self):
        out = self.root / "ks.txt"
        done = tuner("--surface", str(self.surface), "--tune-kingsafety", str(self.train),
                     str(self.holdout), str(out), "--K-phase", "3.0,1.0",
                     "--epochs", "1", "--max-positions", "200")
        self.assertEqual(done.returncode, 0, done.stdout[-2000:] + done.stderr)
        self.assertTrue(out.exists())

    def test_invalid_k_phase_flags_fail(self):
        for bad in (["--loss", str(self.holdout), "--K-phase", "bad"],
                    ["--loss", str(self.holdout), "--K-phase", "1.0,2.0,3.0"],
                    ["--loss", str(self.holdout), "--K-phase", "0.1,1.0"],
                    ["--loss", str(self.holdout), "--K-phase", "6.0,1.0"],
                    ["--loss", str(self.holdout), "--K", "2.0", "--K-phase", "2.0,2.0"],
                    ["--loss", str(self.holdout), "--K", "2.0", "--fit-k-phase"]):
            with self.subTest(bad=bad):
                done = tuner(*bad)
                self.assertNotEqual(done.returncode, 0)


if __name__ == "__main__":
    unittest.main()
