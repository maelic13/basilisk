# BAS-P43

**BAS-P43 — the build: full LTO, IR-level PGO, both (B.8.9, item C)**

- **Registration.** 2026-11-15, B.8.9, before the reads. Agent-run on the idle host.
- **Change.** Two CMake options, both off by default (`a662fd9`): `FULL_LTO`
  (Clang's `-flto=full` for the IPO targets in place of ThinLTO) and `IR_PGO`
  (Clang's `-fprofile-generate` / `-fprofile-use` in place of the front end's
  `-fprofile-instr-generate` / `-fprofile-instr-use`); both forwarded to the PGO
  children. The training stays `bench 13` at one thread and the training build stays
  without LTO. The source is unchanged.
- **Arms.** `b89-head` (the head's pool, A1 to A4 reverted; the options' defaults
  build the same code) against three pools of four final-PGO builds each:
  `b89-c-lto` (`-ArmOption FULL_LTO -Arm On`), `b89-c-irpgo` (`-ArmOption IR_PGO
  -Arm On`), `b89-c-both` (a throwaway revision of `a662fd9` with both defaults on,
  built by `nps_pool.ps1 -Revision`; cited by that recipe). Every arm must hold
  `bench 13` 4,688,537 and the cohort fingerprint; one that moves is a compiler
  finding, not a candidate.
- **Prediction (packet §8, frozen).** The best arm: bench +0.8% [−0.5, +2.5] (80%);
  P(kept) 0.50.
- **Reads.**
  - One pooled `bench` read of the four arms (`nps_read.ps1`), three gain contrasts
    against `b89-head`.
  - The cohort read at one engine (`cohort_read.py run --engines 1`) of the four
    arms, three contrasts against `b89-head` (the held-out workload: the build
    trains on `bench`).
  - A second read, only on a winner (an arm kept by the rule below at +0.5% or
    more): the winner with `-mllvm -inline-threshold=500` at the compile and the
    link and ext-tsp block placement, against the winner, registered then.
- **Verdict rule (packet §6, frozen).** An arm is kept when its `bench` gain
  contrast ACCEPTs by the standing method (+0.9% at step 1, or +0.5% with the lower
  bound above 0 at step 2) and its one-engine cohort estimate is −0.5% or above. Of several kept arms the one with
  the largest `bench` estimate is the winner. A kept arm owes CI's and the release's
  flags, the universal binary's tiers and `check_universal.py`.
