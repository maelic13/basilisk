"""Emit (and verify) weather-factory's spsa.json from the end-state parameters.

BASILISK_SPSA_SCHEDULE_V1 — part of the Basilisk overlay (see spsa.py).

`tools/spsa.ps1` calls this instead of hand-writing the json, so the
`a`/`c`/`A` back-solve exists exactly once, in `SpsaParams.from_end_state`.

    python write_spsa_json.py --iterations 5000 --r-end 0.0031 --out spsa.json
    python write_spsa_json.py --verify-only --out spsa.json

**Why this is Python and not PowerShell, and why it now reads itself back.**
PowerShell cannot safely handle this schema at all: it has both `a` and `A`,
and PowerShell variable names AND hashtable keys are case-insensitive, so `$a`
and `$A` are one variable and ConvertTo-Json/ConvertFrom-Json collapse or reject
the pair. Rarog wrote this file from PowerShell and the gain assignment silently
clobbered the damping term — `A` came out ~0.0965 instead of 500, i.e. **no
damping at all**, which is precisely the defect their schedule fix existed to
remove. It was caught by a dry run before the tune, not by the tune.

We were never exposed to that (the back-solve has always lived here), but the
lesson generalises: a schedule constant that is silently wrong costs a 40-hour
run that looks completely normal while it burns. So every emit is now read back
off disk and checked against the invariants, and `--verify-only` re-checks the
file the tuner is actually about to consume.
"""

import argparse
import dataclasses
import json
import sys

from spsa import SpsaParams

# Relative tolerance for the read-back. Generous enough for float formatting
# through json, tight enough that a clobbered constant cannot pass.
TOL = 1e-9


def _close(actual: float, expected: float) -> bool:
    if expected == 0:
        return abs(actual) <= TOL
    return abs(actual - expected) / abs(expected) <= TOL


def verify(path: str) -> SpsaParams:
    """Read spsa.json back off disk and assert the schedule it actually holds.

    Raises SystemExit(1) with a specific message on any violation — never a
    bare assert, because this runs unattended right before a multi-night run.
    """
    with open(path) as handle:
        raw = json.load(handle)

    missing = {"a", "c", "A", "alpha", "gamma"} - set(raw)
    if missing:
        sys.exit(f"spsa.json is missing required keys: {sorted(missing)}")
    # The pair that PowerShell cannot represent. If both survived the round
    # trip as distinct keys, no case-folding writer touched this file.
    if "a" not in raw or "A" not in raw:
        sys.exit("spsa.json lost the distinction between 'a' and 'A' — it was "
                 "written by something that folds key case. Only "
                 "write_spsa_json.py may write this file.")

    params = SpsaParams(**raw)
    n, alpha, gamma = params.N, params.alpha, params.gamma

    for name, value in (("a", params.a), ("c", params.c), ("A", params.A)):
        if not (value > 0) or value != value or value in (float("inf"), float("-inf")):
            sys.exit(f"spsa.json has a non-positive or non-finite {name}={value!r}. "
                     f"A=0 in particular means NO damping at all, which silently "
                     f"invalidates the whole run.")

    if n <= 0:
        # A pre-9.1 file, or hand-edited. The schedule may be fine but nothing
        # below can be checked, and the run cannot self-stop.
        print(f"WARNING: spsa.json records no horizon (N={n}); the invariants "
              f"below cannot be checked and the run will not stop by itself.")
        return params

    expected_A = max(1.0, 0.1 * n)
    expected_c = float(n) ** gamma
    expected_a = params.r_end * (expected_A + n) ** alpha

    if not _close(params.A, expected_A):
        sys.exit(f"spsa.json A={params.A!r} but the horizon N={n} requires "
                 f"A={expected_A} (10% of N). An A far below that is the "
                 f"'no damping' failure: a_t = a/(t+A)^alpha stops being damped "
                 f"at the start of the run.")
    if not _close(params.c, expected_c):
        sys.exit(f"spsa.json c={params.c!r} but N={n}, gamma={gamma} requires "
                 f"c={expected_c} (probe = one param step at the horizon).")
    if params.r_end > 0 and not _close(params.a, expected_a):
        sys.exit(f"spsa.json a={params.a!r} but r_end={params.r_end}, N={n} "
                 f"requires a={expected_a}.")
    if params.r_end > 0 and not _close(params.step_ratio(), params.r_end):
        sys.exit(f"spsa.json does not realise its own r_end: requested "
                 f"{params.r_end}, realised {params.step_ratio()}.")

    return params


def report(params: SpsaParams, path: str) -> None:
    print(f"Verified {path}: A = {params.A:g} "
          f"({'10% of horizon' if params.N else 'no horizon recorded'}), "
          f"a = {params.a:.6g}, c = {params.c:.6g}, "
          f"alpha = {params.alpha}, gamma = {params.gamma}, N = {params.N}")
    if params.N:
        print(f"  probe at iteration 1 / N: {params.schedule(1)[1]:.4f} / "
              f"{params.schedule(params.N)[1]:.4f} x each parameter's step")
        print(f"  realised r_end at N: {params.step_ratio():.6g} "
              f"(requested {params.r_end:.6g})")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iterations", type=int,
                        help="planned horizon N, in iterations")
    parser.add_argument("--r-end", type=float,
                        help="end-of-run step ratio (fishtest default ~0.002)")
    parser.add_argument("--alpha", type=float, default=0.601)
    parser.add_argument("--gamma", type=float, default=0.102)
    parser.add_argument("--out", default="spsa.json")
    parser.add_argument("--verify-only", action="store_true",
                        help="check an existing spsa.json instead of writing one")
    args = parser.parse_args()

    if args.verify_only:
        report(verify(args.out), args.out)
        return

    if args.iterations is None or args.r_end is None:
        parser.error("--iterations and --r-end are required unless --verify-only")

    params = SpsaParams.from_end_state(
        n_iterations=args.iterations,
        r_end=args.r_end,
        alpha=args.alpha,
        gamma=args.gamma,
    )

    with open(args.out, "w") as handle:
        json.dump(dataclasses.asdict(params), handle, indent=4)

    # Read it back off disk rather than trusting what we just computed: the
    # failure this guards against is the FILE being wrong, not the maths.
    report(verify(args.out), args.out)


if __name__ == "__main__":
    main()
