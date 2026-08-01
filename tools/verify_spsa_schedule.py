"""Verification for the Phase 9.1 SPSA schedule repair.  No games are played.

PLAN.md §5 step 9.1 pre-registers exactly this evidence:

  1. reproduce the a_t / c_t table old-vs-new at iterations 1 / 100 / 600 / 3673
     (3673 = a real historical run length), showing what the units bug did;
  2. check that r_end is invariant across N = 1,000 / 2,500 / 5,000 / 10,000 —
     the property the end-state parameterization exists to guarantee;
  3. confirm a pre-9.1 tuner/state.json still loads and resumes.

It also checks the two internal-consistency properties the derivation rests on:
the probe equals each parameter's own `step` at the horizon, and the realised
step ratio at N equals the requested r_end.

Run from anywhere:

    python tools/verify_spsa_schedule.py

Exit code 0 = every check passed.  This is an instrument test, not a candidate:
nothing is re-tuned, and no historical bake is revisited by running it.
"""

import json
import pathlib
import sys
import tempfile

OVERLAY = pathlib.Path(__file__).resolve().parent / "weather-factory-overlay"
sys.path.insert(0, str(OVERLAY))

from spsa import SpsaParams  # noqa: E402  (path must be set up first)


GAMES_PER_ITER = 32          # cutechess.json "games" — the units-bug factor
ALPHA, GAMMA = 0.601, 0.102

failures: list[str] = []


def check(label: str, ok: bool, detail: str = "") -> None:
    # ASCII only: this output is piped into logs on a cp1252 console.
    print(f"  [{'PASS' if ok else 'FAIL'}] {label}{(' - ' + detail) if detail else ''}")
    if not ok:
        failures.append(label)


def old_schedule(a: float, c: float, A: float, iteration: int) -> tuple[float, float]:
    """The pre-9.1 behaviour: `t` (GAMES) fed straight into an iteration-scaled A.

    This is what tools/weather-factory/spsa.py did at line 68-70 before the fix,
    with A written by spsa.ps1 as Iterations/10.
    """
    t = iteration * GAMES_PER_ITER
    return a / (t + A) ** ALPHA, c / t ** GAMMA


def section(title: str) -> None:
    print()
    print(title)
    print("-" * len(title))


# ── 1. Old vs new schedule table ─────────────────────────────────────────────
section("1. Schedule old-vs-new (historical settings: a=1.0, c=1.0, A=N/10)")

N_HIST = 3673            # a real weather-factory run length
A_HIST = N_HIST / 10     # what spsa.ps1 wrote, in ITERATIONS
print(f"   horizon N={N_HIST}, A={A_HIST:g} iterations, {GAMES_PER_ITER} games/iteration")
print()
print(f"   {'iter':>6} | {'a_t old':>12} {'a_t new':>12} {'ratio':>8} |"
      f" {'c_t old':>10} {'c_t new':>10} {'ratio':>8}")
print(f"   {'-'*6}-+-{'-'*12}-{'-'*12}-{'-'*8}-+-{'-'*10}-{'-'*10}-{'-'*8}")

hist = SpsaParams(a=1.0, c=1.0, A=A_HIST, alpha=ALPHA, gamma=GAMMA)
ratios = []
for it in (1, 100, 600, 3673):
    a_old, c_old = old_schedule(1.0, 1.0, A_HIST, it)
    a_new, c_new = hist.schedule(it)
    ratios.append(a_new / a_old)
    print(f"   {it:>6} | {a_old:>12.6f} {a_new:>12.6f} {a_new/a_old:>8.2f}x |"
          f" {c_old:>10.6f} {c_new:>10.6f} {c_new/c_old:>8.2f}x")

# The damping term is the point: under the old units A was compared against a
# GAMES count, so it stopped mattering ~32x sooner than intended.
old_damp_frac = A_HIST / (1 * GAMES_PER_ITER + A_HIST)   # A's share at iteration 1
new_damp_frac = A_HIST / (1 + A_HIST)
check("A actually damps the start of the run (new)", new_damp_frac > 0.99,
      f"A's share of the denominator at iteration 1: "
      f"{old_damp_frac:.3f} old -> {new_damp_frac:.3f} new")
check("the fixed gain decays more slowly than the broken one",
      all(r > 1.0 for r in ratios) and ratios[-1] > ratios[0],
      f"a_t ratio grows {ratios[0]:.2f}x -> {ratios[-1]:.2f}x across the run")
check("c_t (probe size) is uniformly larger once units are right",
      all(old_schedule(1.0, 1.0, A_HIST, it)[1] < hist.schedule(it)[1]
          for it in (1, 100, 600, 3673)),
      f"{GAMES_PER_ITER}^gamma = {GAMES_PER_ITER ** GAMMA:.3f}x, constant in k")

# The reason (b) must ship with (a): restoring the shape at the old magnitude
# multiplies every step, which is what Rarog measured as worse than the bug.
mid = hist.schedule(600)[0] / old_schedule(1.0, 1.0, A_HIST, 600)[0]
check("fixing units WITHOUT rescaling a would inflate the step several-fold",
      mid > 4.0, f"a_t at iteration 600 grows {mid:.2f}x if a stays 1.0")


# ── 2. r_end invariance across horizons ──────────────────────────────────────
section("2. End-state parameterization: r_end is invariant in N")

R_END = 0.0031
print(f"   requested r_end = {R_END}")
print()
print(f"   {'N':>7} | {'A':>8} {'c':>9} {'a':>9} | {'probe@1':>9} {'probe@N':>9} "
      f"| {'r_end@N':>9}")
print(f"   {'-'*7}-+-{'-'*8}-{'-'*9}-{'-'*9}-+-{'-'*9}-{'-'*9}-+-{'-'*9}")

for n in (1000, 2500, 5000, 10000):
    p = SpsaParams.from_end_state(n, R_END)
    realised = p.step_ratio()
    print(f"   {n:>7} | {p.A:>8.1f} {p.c:>9.4f} {p.a:>9.4f} |"
          f" {p.schedule(1)[1]:>9.4f} {p.schedule(n)[1]:>9.4f} | {realised:>9.6f}")
    check(f"r_end at N={n} equals the request",
          abs(realised - R_END) < 1e-12, f"{realised:.9f}")
    check(f"probe at N={n} equals one parameter step",
          abs(p.schedule(n)[1] - 1.0) < 1e-12, f"{p.schedule(n)[1]:.9f}")
    check(f"A = 10% of N at N={n}", abs(p.A - 0.1 * n) < 1e-9, f"A={p.A:g}")

# The old form had no such invariance: with a=c=1.0 the end behaviour drifted
# with the horizon, which is how "a" silently went stale between runs.
old_r = {n: (1.0 / (n + n / 10) ** ALPHA) / (1.0 / n ** GAMMA)
         for n in (1000, 2500, 5000, 10000)}
spread = max(old_r.values()) / min(old_r.values())
print()
print("   old form (a=c=1.0), same reading: " +
      ", ".join(f"N={n}: {r:.5f}" for n, r in old_r.items()))
check("the old form's end behaviour DID drift with the horizon",
      spread > 2.0, f"{spread:.2f}x spread across N=1k..10k")
check("the 0.0031 default is ~10x tamer than the historical a=1.0 at N=1000",
      0.05 < R_END / old_r[1000] < 0.15,
      f"r_end 1.0-equivalent at N=1000 is {old_r[1000]:.5f}")


# ── 3. Backward compatibility of tuner/state.json ────────────────────────────
section("3. A pre-9.1 tuner/state.json still loads")

legacy = {
    "t": 32 * 1894,                      # hcefinal's length, in games
    "spsa_params": {"a": 1.0, "c": 1.0, "A": 189,
                    "alpha": 0.601, "gamma": 0.102},
    "uci_params": [{"name": "LmrBase", "value": 77.0, "min_value": 40,
                    "max_value": 120, "step": 4.0}],
}
with tempfile.TemporaryDirectory() as tmp:
    path = pathlib.Path(tmp) / "state.json"
    path.write_text(json.dumps(legacy))
    loaded = json.loads(path.read_text())
    try:
        restored = SpsaParams(**loaded["spsa_params"])
        ok, detail = True, f"N defaulted to {restored.N}, r_end to {restored.r_end}"
    except TypeError as exc:
        restored, ok, detail = None, False, str(exc)
    check("SpsaParams(**old_state) constructs", ok, detail)

    if restored is not None:
        it = loaded["t"] / GAMES_PER_ITER
        a_t, c_t = restored.schedule(it)
        check("a resumed pre-9.1 run still produces a finite schedule",
              a_t > 0 and c_t > 0,
              f"at iteration {it:.0f}: a_t={a_t:.6f} c_t={c_t:.6f}")
        check("a pre-9.1 state has no horizon, so the run cannot self-stop "
              "(spsa.ps1 warns and falls back to -Iterations)",
              restored.N == 0)

# A fresh state round-trips with the new fields present.
fresh = SpsaParams.from_end_state(5000, R_END)
round_tripped = SpsaParams(**json.loads(json.dumps(fresh.__dict__)))
check("a 9.1 schedule round-trips through json unchanged",
      round_tripped == fresh, f"N={round_tripped.N}, r_end={round_tripped.r_end}")


# ── 4. Guard rails ───────────────────────────────────────────────────────────
section("4. Input validation")

for bad, label in (((0, R_END), "N = 0"), ((5000, 0.0), "r_end = 0"),
                   ((5000, -1.0), "r_end < 0")):
    try:
        SpsaParams.from_end_state(*bad)
        check(f"{label} is rejected", False, "no exception raised")
    except ValueError:
        check(f"{label} is rejected", True)


# ── 5. The shell-facing state summary ────────────────────────────────────────
section("5. describe_state.py hands PowerShell an unambiguous summary")

import subprocess  # noqa: E402  (only needed for this section)

with tempfile.TemporaryDirectory() as tmp:
    path = pathlib.Path(tmp) / "state.json"
    path.write_text(json.dumps({
        "t": 32 * 4000,
        "spsa_params": {"a": 0.5487, "c": 2.3839, "A": 500.0,
                        "alpha": 0.601, "gamma": 0.102,
                        "N": 5000, "r_end": 0.0031},
        "uci_params": [],
    }))
    proc = subprocess.run(
        [sys.executable, str(OVERLAY / "describe_state.py"), str(path)],
        capture_output=True, text=True)
    check("describe_state.py succeeds", proc.returncode == 0, proc.stderr.strip())
    emitted = dict(line.split("=", 1) for line in proc.stdout.splitlines() if "=" in line)

    # The consumer is PowerShell, whose hashtable keys are case-insensitive, so
    # `a` and `A` in the same record silently merge — that is exactly why
    # ConvertFrom-Json refuses this file, and it bit this script's first draft.
    lowered = [k.lower() for k in emitted]
    check("no two emitted keys differ only by case",
          len(set(lowered)) == len(lowered), f"keys: {sorted(emitted)}")
    check("the gain and the damping term both survive",
          emitted.get("gain_a") == "0.5487" and emitted.get("damp_A") == "500.0",
          f"gain_a={emitted.get('gain_a')} damp_A={emitted.get('damp_A')}")
    check("the horizon is reported", emitted.get("N") == "5000")

    missing = subprocess.run(
        [sys.executable, str(OVERLAY / "describe_state.py"), str(path) + ".nope"],
        capture_output=True, text=True)
    check("a missing state file exits non-zero with empty stdout",
          missing.returncode != 0 and not missing.stdout.strip())


# ── 6. spsa.json read-back rejects a clobbered schedule ──────────────────────
section("6. The emitted spsa.json is read back and checked")

# Rarog, 2026-07-30: their spsa.ps1 wrote A = 0.0965 where 500 was required,
# because PowerShell variable names are case-insensitive and the `a` assignment
# clobbered `A` three lines below the comment explaining why damping matters.
# a_t = a/(t+A)^alpha with A ~ 0 is NO damping at all — exactly the defect the
# schedule repair exists to remove — and a dry run caught it, not the tune.
# Basilisk's back-solve has always lived in Python (case-sensitive), so we were
# never exposed; these checks make sure that stays true.
with tempfile.TemporaryDirectory() as tmp:
    good = pathlib.Path(tmp) / "spsa.json"
    emit = subprocess.run(
        [sys.executable, str(OVERLAY / "write_spsa_json.py"),
         "--iterations", "5000", "--r-end", "0.0031", "--out", str(good)],
        capture_output=True, text=True)
    check("emitting spsa.json succeeds and self-verifies",
          emit.returncode == 0 and "Verified" in emit.stdout, emit.stderr.strip())

    written = json.loads(good.read_text())
    check("both 'a' and 'A' survive as distinct keys",
          "a" in written and "A" in written and written["a"] != written["A"],
          f"a={written.get('a')} A={written.get('A')}")
    check("A is 10% of the horizon, not ~0",
          abs(written["A"] - 500.0) < 1e-9, f"A={written['A']}")

    def verify_rejects(mutate, label):
        bad = pathlib.Path(tmp) / "bad.json"
        d = json.loads(good.read_text())
        mutate(d)
        bad.write_text(json.dumps(d))
        proc = subprocess.run(
            [sys.executable, str(OVERLAY / "write_spsa_json.py"),
             "--verify-only", "--out", str(bad)],
            capture_output=True, text=True)
        check(label, proc.returncode != 0,
              (proc.stdout + proc.stderr).strip().splitlines()[0] if proc.stdout or proc.stderr else "")

    # The exact observed corruption.
    verify_rejects(lambda d: d.__setitem__("A", 0.0965),
                   "A clobbered to Rarog's observed 0.0965 is REFUSED")
    verify_rejects(lambda d: d.__setitem__("A", 0.0),
                   "A = 0 (no damping at all) is REFUSED")
    verify_rejects(lambda d: d.pop("A"),
                   "a case-folding writer that loses 'A' is REFUSED")
    verify_rejects(lambda d: d.__setitem__("c", 1.0),
                   "a stale c (not N^gamma) is REFUSED")
    verify_rejects(lambda d: d.__setitem__("a", 1.0),
                   "a stale gain (the pre-9.1 a=1.0) is REFUSED")

    ok = subprocess.run(
        [sys.executable, str(OVERLAY / "write_spsa_json.py"),
         "--verify-only", "--out", str(good)],
        capture_output=True, text=True)
    check("the untouched file still passes", ok.returncode == 0)


print()
if failures:
    print(f"FAILED: {len(failures)} check(s): " + "; ".join(failures))
    sys.exit(1)
print("All checks passed.")
