"""Weather-factory runner — Basilisk overlay.  BASILISK_SPSA_RUNNER_V1

Upstream: https://github.com/jnlt3/weather-factory (MIT, Copyright (c) 2024
jnlt3).  This file REPLACES the clone's ``main.py``; see ``spsa.py`` in this
overlay for why the overlay exists at all.

Basilisk changes (Phase 9.1, 2026-07-29 — PLAN.md §5 step 9.1(c)), all three
hazards confirmed present in the clone before the fix:

1. **The loop never stopped.**  Upstream is ``while True:`` — the target
   iteration count lived only in the operator's head, so a tune ran until
   somebody remembered to Ctrl-C it.  The horizon now comes from
   ``WF_TARGET_ITERATIONS`` (set by ``spsa.ps1``) or, failing that, from the
   ``N`` recorded in the schedule, and the run stops itself there.

2. **``A`` is frozen at first launch.**  A resume restores ``spsa_params`` from
   ``tuner/state.json``, so re-passing ``-Iterations`` to ``spsa.ps1`` is
   silently ignored — the schedule keeps the horizon it was created with.  That
   is the correct behaviour (changing the schedule mid-run invalidates it), but
   it was invisible.  It is now printed, loudly, on every resumed launch.

3. **The trajectory was console-only and truncated on resume.**  The tail-mean
   bake (PLAN gate 11) reads the parameter trajectory, and the only copy of it
   was the piped console log, which ``watch.ps1`` used to reopen in truncate
   mode — Rarog lost 1,086 of 3,670 iterations exactly that way.  ``watch.ps1``
   now appends, and this file additionally appends every iteration to
   ``tuner/trajectory.csv``, which is what a tail-mean bake should read.
"""

import csv
import dataclasses
import json
import os
import pathlib
import time
from graph import Graph
from spsa import Param, SpsaParams, SpsaTuner
from cutechess import CutechessMan
import copy


STATE_PATH = pathlib.Path("./tuner/state.json")
TRAJECTORY_PATH = pathlib.Path("./tuner/trajectory.csv")


def cutechess_from_config(config_path: str) -> CutechessMan:
    with open(config_path) as config_file:
        config = json.load(config_file)
    return CutechessMan(**config)


def params_from_config(config_path: str) -> list[Param]:
    with open(config_path) as config_file:
        config = json.load(config_file)
    return [Param(name, **cfg) for name, cfg in config.items()]


def spsa_from_config(config_path: str):
    with open(config_path) as config_file:
        config = json.load(config_file)
    return SpsaParams(**config)


def save_state(spsa: SpsaTuner):
    save_file = "./tuner/state.json"
    spsa_params = spsa.spsa
    uci_params = spsa.uci_params
    t = spsa.t
    with open(save_file, "w") as save_file:
        spsa_params = dataclasses.asdict(spsa_params)
        uci_params = [dataclasses.asdict(
            uci_param) for uci_param in uci_params]

        json.dump({"t": t, "spsa_params": spsa_params,
                  "uci_params": uci_params}, save_file)


def append_trajectory(spsa: SpsaTuner):
    """Append this iteration's whole parameter vector to tuner/trajectory.csv.

    The tail-mean bake (PLAN gate 11: mean of the final ~500 iterations, whole
    vector) reads this file.  Appending - never truncating - is the point:
    a resumed run must extend the trajectory, not replace it.
    """
    new_file = not TRAJECTORY_PATH.is_file()
    with open(TRAJECTORY_PATH, "a", newline="") as handle:
        writer = csv.writer(handle)
        if new_file:
            writer.writerow(["iteration", "games"] +
                            [p.name for p in spsa.params])
        writer.writerow([f"{spsa.iteration:.0f}", spsa.t] +
                        [f"{p.value:.4f}" for p in spsa.params])


def target_iterations(spsa_params: SpsaParams) -> int:
    """Planned horizon, in iterations.

    A recorded `N` WINS over the environment: the schedule was solved for that
    horizon and frozen with it, so running further would anneal against a
    schedule that no longer describes the run.  The env var is the fallback for
    a pre-9.1 state (no N) and the deliberate override for extending a finished
    run, and a disagreement is always printed rather than silently resolved.
    """
    env_raw = os.environ.get("WF_TARGET_ITERATIONS", "").strip()
    env = 0
    if env_raw:
        try:
            env = int(env_raw)
        except ValueError:
            print(f"WARNING: WF_TARGET_ITERATIONS={env_raw!r} is not an integer; "
                  "ignoring it.")

    recorded = int(spsa_params.N)
    if recorded > 0:
        if env and env != recorded:
            print(f"WARNING: WF_TARGET_ITERATIONS={env} disagrees with the "
                  f"horizon this schedule was frozen with (N={recorded}). "
                  f"Using N={recorded}; the schedule is only valid for it. "
                  f"Start a fresh run to tune to {env}.")
        return recorded
    return env


def format_eta(seconds: float) -> str:
    seconds = max(0.0, seconds)
    hours, rem = divmod(int(seconds), 3600)
    minutes, secs = divmod(rem, 60)
    if hours:
        return f"{hours}h{minutes:02d}m"
    if minutes:
        return f"{minutes}m{secs:02d}s"
    return f"{secs}s"


def main():

    t = 0
    resumed = STATE_PATH.is_file()
    if resumed:
        with open(STATE_PATH) as state:
            state_dict = json.load(state)
            params = [Param(cfg["name"], cfg["value"], cfg["min_value"], cfg["max_value"], cfg["step"])
                      for cfg in state_dict["uci_params"]]
            spsa_params = SpsaParams(**state_dict["spsa_params"])
            t = state_dict["t"]
    else:
        params = params_from_config("config.json")
        spsa_params = spsa_from_config("spsa.json")
    cutechess = cutechess_from_config("cutechess.json")
    spsa = SpsaTuner(spsa_params, params, cutechess)
    spsa.t = t
    graph = Graph()

    avg_time = 0

    start_t = t
    target = target_iterations(spsa_params)
    done = spsa.iteration

    print("============================================================")
    if resumed:
        print(f"RESUMED from {STATE_PATH} at iteration {done:.0f} "
              f"({spsa.t} games).")
        print("  !! The schedule (a, c, A, N) was FROZEN when this run was")
        print("     first launched and is restored from state.json - passing")
        print("     -Iterations to spsa.ps1 again does NOT change it. To tune")
        print("     with a different horizon, start a fresh run (no -Resume).")
    else:
        print("FRESH run - the schedule below is frozen for its whole life.")
    print(f"  schedule: a={spsa_params.a:.6g} c={spsa_params.c:.6g} "
          f"A={spsa_params.A:.6g} alpha={spsa_params.alpha} "
          f"gamma={spsa_params.gamma}")
    if spsa_params.N:
        print(f"  horizon:  N={spsa_params.N} iterations, "
              f"r_end={spsa_params.r_end:.6g} "
              f"(realised {spsa_params.step_ratio():.6g})")
    if target > 0:
        print(f"  target:   {target} iterations "
              f"({target * cutechess.games} games); the run stops itself there.")
        if done >= target:
            print("  NOTE: this run has already reached its horizon; it will "
                  "stop immediately.")
            print("        Its schedule is only defined up to N, so continuing "
                  "means a FRESH run, not a longer one.")
    else:
        print("  target:   NONE recorded - this run will not stop by itself. "
              "Set WF_TARGET_ITERATIONS.")
    print(f"  games/iteration: {cutechess.games}   "
          f"state saved every {cutechess.save_rate} iterations")
    print("============================================================")
    print()

    print("Initial state: ")
    for param in spsa.params:
        print(param)
    print()

    def progress_report():
        iters = spsa.iteration
        elapsed_iters = iters - (start_t / cutechess.games)
        if elapsed_iters <= 0:
            return
        per_iter = avg_time / elapsed_iters
        line = f"iterations: {iters:.0f}"
        if target > 0:
            remaining = max(0.0, target - iters)
            line += (f"/{target} ({100.0 * iters / target:.1f}%, "
                     f"ETA {format_eta(remaining * per_iter)})")
        print(f"{line} ({per_iter:.2f}s per iter)")
        print(f"games: {spsa.t} ({(avg_time / elapsed_iters / cutechess.games):.2f}s per game)")

    try:
        while target <= 0 or spsa.iteration < target:
            start = time.time()
            spsa.step()
            avg_time += time.time() - start

            graph.update(spsa.t, copy.deepcopy(spsa.params))
            graph.save("graph.png")
            append_trajectory(spsa)

            if (spsa.iteration % cutechess.save_rate) == 0:
                print("Saving state...")
                save_state(spsa)

            progress_report()
            for param in spsa.params:
                print(param)
            print()

        print(f"Target of {target} iterations reached - stopping.")
    finally:
        print("Saving state...")
        save_state(spsa)
        print("Final results: ")
        progress_report()
        print("Final parameters: ")
        for param in spsa.params:
            print(param)
        print(f"Trajectory (bake the tail mean from this): {TRAJECTORY_PATH}")


if __name__ == "__main__":
    main()
