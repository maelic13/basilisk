# BAS-P25

**BAS-P25 — the zero-weight evaluation terms no longer computed**

- **Registration.** Registered 2026-10-29, before any pool was built and
  before any reading; PLAN A.16. Builds agent-run; the read needs the host
  idle and is run when the maintainer frees it:
  `pwsh tools/run_a16_nps_read.ps1` (`-BuildOnly` builds the pools and
  stops).
- **Background.** A.16 deletes three computations whose every weight is 0:
  the winnable block, the hanging-piece loop, and the king-danger ring
  pressure, flank attack and defence, pawnless flank and open-file inputs
  (the flank zone's popcounts for both colours). Each added its weight times
  a feature, and the winnable step added `sgn * max(0, -|eg|)`, which is 0;
  `bench 13` stays 19,470,325. The question is only whether the saved work
  is measurable.
- **Instrument.** `tools/nps_read.ps1`, decided by `tools/diag/nps_read.py`
  (PROCESS *Build, bench and NPS*).
- **Builds**, four `build_test.ps1` Release builds a pool (the pext asset's
  recipe), clang 23.1.4, `bench 13` 19,470,325:
  - `a16-base`: `0680393`, master before A.16, built from a temporary
    worktree by `nps_pool.ps1 -Revision`;
  - `a16-cand`: this branch, `ba67ecc`'s engine source.
- **Contrast:** `zero`, `a16-cand` over `a16-base`, gain form.
- **Question.** Is the evaluator measurably faster without the zero-weight
  computations?

#### PRE-REGISTERED PREDICTION — freeze before exposure

- `zero`: +0.4%, 80% interval [−0.1%, +1.0%]. The three blocks are a few
  popcounts, a loop over the non-pawn pieces and a dozen operations on
  every full evaluation; evaluation is a minority of a node's time.

- **Decision rule (PLAN A.16).** The deletion is kept when the read is
  faster than BAS-P18's null interval (−0.19% [−0.66%, +0.27%]): its
  estimate at the final step above +0.27% with the 95% interval's lower
  bound above 0. Otherwise it is reverted, and the slots wait for their
  Phase C owners. The gain form's own verdict decides only the stop: step 2
  on EXTEND.
- **Stop rule:** step 1, step 2 only on EXTEND; a REPEAT repeats the read
  once into a fresh directory, a second stops for the maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, a
  `bench 8` warm-up per build, 60 s under 10% host load, no engine, harness
  or build process.
- **Reading time:** two pools, 8 readings a cycle: about 6 minutes at step
  1, about 15 with step 2.

#### RESULT — append after exposure

- **Result (agent-run 2026-10-29, host 0.6% and 0.9%, no cycle disturbed;
  `tools/results/a16-nps/p25/`).** Every build 19,470,325. Means of build
  medians: base 3.4371M, candidate 3.5180M. `zero`: **+2.35%**
  [+2.01%, +2.70%], ACCEPT at step 1.
- **Decision (the registered rule).** Above +0.27% with the lower bound
  above 0: the deletion is kept.
- **Calibration.** Missed: +2.35% against +0.4% [−0.1%, +1.0%]. The wrong
  part of the model was the cost per evaluation: the three blocks are about
  a dozen popcounts, a loop with two branches over every non-pawn piece and
  the winnable block's census, on every full evaluation of both colours,
  and the evaluation's share of a node is larger than the "minority" the
  prediction assumed. The A.17 survey measures that share directly.
