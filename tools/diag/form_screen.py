#!/usr/bin/env python3
"""The own-forms screen's measure (analysis/c36_own_forms_2026-11-26.md §6).

For two fitted arms read on the same rows, the paired per-row squared error
at one frozen K(phase), and a bootstrap over games:

    Δ = L(reference) − L(arm),   positive when the arm predicts better.

Each arm is a ``--dump-row-lines`` file from its own tuner and final vector:
one line per row the tuner uses, ``<file line> <played White score> <lazy>``,
in file order. The rows file gives each line's result, phase and game.

The tool refuses rather than reads when:

* the two arms do not use exactly the same rows (an override or a verdict
  that one arm serves and the other does not is a different row set);
* an arm's loss, recomputed here, differs from the tuner's own ``--loss`` for
  that arm by more than 1e-8 (``--expect-loss``): the instrument would not be
  measuring what the fit measured.

    python tools/diag/form_screen.py --rows validation.csv --k-phase 3.026687,0.970080 \\
        --reference N.lines --arm ARM.lines [--expect-loss-reference X] \\
        [--expect-loss-arm Y] [--resamples 2000] [--seed 360001] [--out result.json]

``selftest`` checks the arithmetic on a constructed case, a known-bad row set
and a known-bad loss.
"""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
from pathlib import Path

LOSS_TOLERANCE = 1e-8


def k_at(k_eg: float, k_mg: float, phase: int) -> float:
    """The tuner's linear-in-phase K: K_eg + (K_mg - K_eg) * phase / 24."""
    return k_eg + (k_mg - k_eg) * (phase / 24.0)


def squared_error(score: float, result: float, k: float) -> float:
    sig = 1.0 / (1.0 + math.exp(-k * score / 400.0))
    return (result - sig) ** 2


def read_rows(path: Path) -> dict[int, tuple[float, int, str]]:
    """File line (1-based) -> (result, phase, game_id), as the tuner numbers lines."""
    rows: dict[int, tuple[float, int, str]] = {}
    with path.open(encoding="utf-8") as f:
        header = None
        for number, line in enumerate(f, start=1):
            line = line.rstrip("\n").rstrip("\r")
            if not line or line.startswith("#"):
                continue
            fields = line.split(",")
            if header is None:
                header = fields
                col = {name: i for i, name in enumerate(header)}
                for need in ("result", "game_id", "phase"):
                    if need not in col:
                        raise SystemExit(f"error: {path} has no '{need}' column")
                continue
            rows[number] = (float(fields[col["result"]]), int(fields[col["phase"]]), fields[col["game_id"]])
    return rows


def read_lines(path: Path) -> list[tuple[int, int]]:
    """(file line, played White score) per used row, in order."""
    out = []
    with path.open(encoding="utf-8") as f:
        for text in f:
            parts = text.split()
            if len(parts) != 3:
                raise SystemExit(f"error: {path}: not a --dump-row-lines line: {text!r}")
            out.append((int(parts[0]), int(parts[1])))
    return out


def per_row_errors(lines: list[tuple[int, int]], rows: dict, k_eg: float, k_mg: float) -> list[float]:
    errors = []
    for number, score in lines:
        if number not in rows:
            raise SystemExit(f"error: row line {number} is not in the rows file")
        result, phase, _ = rows[number]
        errors.append(squared_error(score, result, k_at(k_eg, k_mg, phase)))
    return errors


def screen(rows: dict, ref: list[tuple[int, int]], arm: list[tuple[int, int]], k_eg: float, k_mg: float,
           resamples: int, seed: int, expect_ref: float | None = None,
           expect_arm: float | None = None) -> dict:
    if [n for n, _ in ref] != [n for n, _ in arm]:
        only_ref = len({n for n, _ in ref} - {n for n, _ in arm})
        only_arm = len({n for n, _ in arm} - {n for n, _ in ref})
        raise SystemExit(f"error: the arms use different rows ({only_ref} only in the reference, "
                         f"{only_arm} only in the arm)")
    e_ref = per_row_errors(ref, rows, k_eg, k_mg)
    e_arm = per_row_errors(arm, rows, k_eg, k_mg)
    n = len(e_ref)
    l_ref, l_arm = sum(e_ref) / n, sum(e_arm) / n
    for name, got, want in (("reference", l_ref, expect_ref), ("arm", l_arm, expect_arm)):
        if want is not None and abs(got - want) > LOSS_TOLERANCE:
            raise SystemExit(f"error: the {name}'s loss recomputes to {got:.8f}, the tuner read {want:.8f}")

    # By game: the sum of each game's paired differences and its row count.
    games: dict[str, list[float]] = {}
    for (number, _), a, b in zip(ref, e_ref, e_arm):
        g = games.setdefault(rows[number][2], [0.0, 0.0])
        g[0] += a - b
        g[1] += 1.0
    sums = [g[0] for g in games.values()]
    counts = [g[1] for g in games.values()]
    rng = random.Random(seed)
    m = len(sums)
    draws = []
    for _ in range(resamples):
        s = c = 0.0
        for _ in range(m):
            j = rng.randrange(m)
            s += sums[j]
            c += counts[j]
        draws.append(s / c)
    draws.sort()
    lo = draws[int(0.025 * resamples)]
    hi = draws[min(resamples - 1, int(0.975 * resamples))]
    return {"rows": n, "games": m, "loss_reference": l_ref, "loss_arm": l_arm,
            "delta": l_ref - l_arm, "ci95": [lo, hi], "resamples": resamples, "seed": seed,
            "k_eg": k_eg, "k_mg": k_mg}


def verdict(result: dict, delta_min: float) -> bool:
    return result["delta"] >= delta_min and result["ci95"][0] > 0.0


def selftest() -> int:
    rows = {3: (1.0, 24, "g1"), 4: (0.0, 0, "g1"), 5: (0.5, 12, "g2"), 6: (1.0, 6, "g3")}
    ref = [(3, 0), (4, 0), (5, 0), (6, 0)]
    better = [(3, 200), (4, -200), (5, 0), (6, 100)]
    r = screen(rows, ref, better, 3.0, 1.0, 200, 1)
    assert r["delta"] > 0 and r["loss_reference"] > r["loss_arm"], r
    expect = sum(squared_error(0, rows[n][0], k_at(3.0, 1.0, rows[n][1])) for n, _ in ref) / 4
    assert abs(r["loss_reference"] - expect) < 1e-15
    same = screen(rows, ref, ref, 3.0, 1.0, 50, 1)
    assert same["delta"] == 0.0 and same["ci95"] == [0.0, 0.0]
    for bad, why in (([(3, 0), (4, 0), (6, 0)], "a row set that differs"),):
        try:
            screen(rows, ref, bad, 3.0, 1.0, 10, 1)
        except SystemExit:
            pass
        else:
            raise AssertionError(f"accepted {why}")
    try:
        screen(rows, ref, better, 3.0, 1.0, 10, 1, expect_ref=expect + 1e-6)
    except SystemExit:
        pass
    else:
        raise AssertionError("accepted a loss that differs from the tuner's")
    print("selftest: PASS")
    return 0


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] == "selftest":
        return selftest()
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rows", type=Path, required=True)
    ap.add_argument("--k-phase", required=True, help="K_eg,K_mg")
    ap.add_argument("--reference", type=Path, required=True)
    ap.add_argument("--arm", type=Path, required=True)
    ap.add_argument("--expect-loss-reference", type=float)
    ap.add_argument("--expect-loss-arm", type=float)
    ap.add_argument("--resamples", type=int, default=2000)
    ap.add_argument("--seed", type=int, default=360001)
    ap.add_argument("--delta-min", type=float, default=2.0e-5)
    ap.add_argument("--out", type=Path)
    args = ap.parse_args()
    k_eg, k_mg = (float(x) for x in args.k_phase.split(","))
    rows = read_rows(args.rows)
    result = screen(rows, read_lines(args.reference), read_lines(args.arm), k_eg, k_mg,
                    args.resamples, args.seed, args.expect_loss_reference, args.expect_loss_arm)
    result["delta_min"] = args.delta_min
    result["passes"] = verdict(result, args.delta_min)
    text = json.dumps(result, indent=2)
    if args.out:
        args.out.write_text(text + "\n", encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
