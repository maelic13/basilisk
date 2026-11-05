"""The wall-clock instrument recovers delays planted in a fake engine.

Two copies of tools/diag/wallclock_fake_engine.py run side by side, one
honest and one with known delays; the instrument's median gaps must equal
the planted values. A second check proves the gap is not a constant: the
honest copy against itself reads zero.
"""
from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "diag" / "wallclock.py"
FAKE = ROOT / "tools" / "diag" / "wallclock_fake_engine.py"
TOLERANCE_MS = 1.0


def run(out: pathlib.Path, planted: dict[str, float]) -> dict:
    argv = [sys.executable, str(TOOL), "--engine", f"honest={FAKE}",
            "--engine", f"planted={FAKE}", "--reference", "honest",
            "--threads", "1", "--positions", "4", "--nodes-rounds", "12",
            "--movetimes", "20", "--movetime-rounds", "6", "--ponder-rounds", "6",
            "--ponder-wait", "20", "--out", str(out)]
    for key, value in planted.items():
        argv += ["--option", f"planted:{key}={value}"]
    completed = subprocess.run(argv, capture_output=True, text=True, timeout=300)
    if completed.returncode != 0:
        raise AssertionError(completed.stdout + completed.stderr)
    return json.loads((out / "summary.json").read_text(encoding="utf-8"))


def gap(summary: dict, cell: str) -> float:
    return summary["cells"][cell]["planted"]["median_minus_reference"]


class PlantedDelays(unittest.TestCase):
    def test_planted_delays_are_recovered(self):
        with tempfile.TemporaryDirectory() as tmp:
            summary = run(pathlib.Path(tmp) / "a", {"PlantNodesMs": 5,
                                                    "PlantOvershootMs": 3,
                                                    "PlantPonderMs": 4})
        self.assertAlmostEqual(gap(summary, "1T nodes overhead_ms"), 5, delta=TOLERANCE_MS)
        self.assertAlmostEqual(gap(summary, "1T nodes wall_ms"), 5, delta=TOLERANCE_MS)
        self.assertAlmostEqual(gap(summary, "1T movetime 20 overshoot_ms"), 3,
                               delta=TOLERANCE_MS)
        self.assertAlmostEqual(gap(summary, "1T ponderhit latency_ms"), 4, delta=TOLERANCE_MS)
        self.assertEqual(summary["early_bestmoves"], {})
        self.assertEqual(summary["nodes_without_report"], 0)

    def test_no_plant_reads_zero(self):
        with tempfile.TemporaryDirectory() as tmp:
            summary = run(pathlib.Path(tmp) / "b", {})
        for cell in ("1T nodes overhead_ms", "1T movetime 20 overshoot_ms",
                     "1T ponderhit latency_ms"):
            self.assertAlmostEqual(gap(summary, cell), 0, delta=TOLERANCE_MS)


if __name__ == "__main__":
    unittest.main()
