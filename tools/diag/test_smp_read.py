import contextlib
import hashlib
import io
import json
import pathlib
import sys
import tempfile
import unittest
from datetime import datetime

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import smp_read

T0 = 1_800_000_000_000_000  # unix microseconds


def game_text(number, fen_side, comments):
    moves = []
    sans = ["e4", "e5", "Nf3", "Nc6", "Bb5", "a6"]
    for i, comment in enumerate(comments):
        prefix = f"{i // 2 + 1}. " if (i % 2 == 0) == (fen_side == "w") else ""
        moves.append(f"{prefix}{sans[i]} {{{comment}}}")
    fen = f"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR {fen_side} KQkq - 0 1"
    return "\n".join([f'[White "x"]', f'[Black "y"]', f'[FEN "{fen}"]', f'[GameNumber "{number}"]', "",
                      " ".join(moves) + " 1/2-1/2", "", ""])


def write_run(root, games):
    """games: (white arm, fen side, comments); game i ends at T0 + i seconds."""
    root = pathlib.Path(root)
    raw, lines = b"", []
    for i, (white, side, comments) in enumerate(games):
        text = game_text(i + 1, side, comments).encode("utf-8")
        lines.append(json.dumps({"game": {"number": i + 1, "white": white,
                                          "slot": {"index": 0, "started_unix_us": T0 + i * 1_000_000,
                                                   "ended_unix_us": T0 + (i + 1) * 1_000_000}},
                                 "pgn": {"offset": len(raw), "length": len(text),
                                         "sha256": hashlib.sha256(text).hexdigest()}}))
        raw += text
    (root / "games.pgn").write_bytes(raw)
    (root / "games.jsonl").write_text("\n".join(lines) + "\n", encoding="utf-8")


def run(argv):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        smp_read.main(argv)
    return out.getvalue()


class SidesTest(unittest.TestCase):
    def test_nps_ratio_and_non_mate_depth(self):
        # Arm a: 1M nodes in 1 s per move at depth 10; arm b: 4M in 1 s at
        # depth 12, and one mate-score move at depth 40 that the non-mate
        # column must leave out. h is subtracted from t.
        a = "s=10 d=10 t=1001ms h=1ms n=1000000"
        b = "s=-10 d=12 t=1002ms h=2ms n=4000000"
        mate = "s=#3 d=40 t=1001ms h=1ms n=4000000"
        games = [("a", "w", [a, b, a, b]), ("b", "w", [b, a, mate, a]), ("a", "b", [b, a, b, a])]
        with tempfile.TemporaryDirectory() as tmp:
            write_run(tmp, games)
            report_path = pathlib.Path(tmp) / "r.json"
            run(["sides", tmp, "--fifths", "3", "--json", str(report_path)])
            report = json.loads(report_path.read_text(encoding="utf-8"))
        whole = report["whole"]
        self.assertEqual(whole["a"]["moves"], 6)
        self.assertEqual(whole["b"]["moves"], 6)
        self.assertEqual(whole["b"]["mate_score_moves"], 1)
        self.assertAlmostEqual(whole["a"]["nps"], 1e6)
        self.assertAlmostEqual(whole["b"]["nps"], 4e6)
        self.assertAlmostEqual(whole["r"], 4.0)
        self.assertAlmostEqual(whole["b"]["depth_non_mate"], 12.0)
        self.assertAlmostEqual(whole["b"]["depth_all"], (5 * 12 + 40) / 6)
        self.assertEqual(len(report["fifths"]), 3)

    def test_tampered_pgn_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            write_run(tmp, [("a", "w", ["s=1 d=1 t=2ms h=1ms n=10", "s=1 d=1 t=2ms h=1ms n=10"])])
            pgn = pathlib.Path(tmp) / "games.pgn"
            pgn.write_bytes(pgn.read_bytes().replace(b"e4", b"d4"))
            with self.assertRaises(SystemExit):
                run(["sides", tmp])


class PlacementTest(unittest.TestCase):
    def test_packed_and_spread_turns(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            sampler = root / "sampler"
            sampler.mkdir()
            write_run(root, [("a", "w", ["s=1 d=1 t=2ms h=1ms n=10"] * 2)] * 4)
            (root / "resolved-config.json").write_text(json.dumps({"execution": {"slots": [
                {"slot_index": 0, "engine_a": {"allocation": {"cpus": [
                    {"group": 0, "number": c} for c in (2, 4, 6, 8)]}}}]}}), encoding="utf-8")
            (sampler / "topology.json").write_text(json.dumps(
                {"cores": [[0, 1], [2, 3], [4, 5], [6, 7], [8, 9]]}), encoding="utf-8")
            head = ['"(PDH-CSV 4.0)"'] + [f'"\\\\HOST\\Processor Information(0,{c})\\% Processor Time"' for c in range(10)]
            rows = [",".join(head)]
            packed = [0, 0, 95, 95, 95, 95, 0, 0, 0, 0]   # four threads on two cores' siblings
            spread = [0, 0, 95, 0, 95, 0, 95, 0, 95, 0]   # one thread per core
            for i, busy in enumerate([packed, packed, packed, spread]):
                stamp = datetime.fromtimestamp((T0 + i * 1_000_000 + 500_000) / 1e6)
                rows.append(",".join([f'"{stamp.strftime("%m/%d/%Y %H:%M:%S.%f")[:-3]}"'] + [f'"{v}"' for v in busy]))
            (sampler / "cpu-001.csv").write_text("\n".join(rows) + "\n", encoding="utf-8")
            report_path = root / "p.json"
            run(["placement", tmp, str(sampler), "--json", str(report_path)])
            report = json.loads(report_path.read_text(encoding="utf-8"))
        slot = report["slots"][0]
        self.assertEqual(slot["seconds"], 4)
        self.assertEqual(slot["cores_spanned"], {"2": 3, "4": 1})
        self.assertAlmostEqual(slot["mean_busy"]["3"], 95 * 3 / 4, delta=0.05)


if __name__ == "__main__":
    unittest.main()
