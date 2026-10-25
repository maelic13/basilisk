# BAS-P18

**BAS-P18 — the pooled-build NPS read's null: two independent pools of the head**

- **Registration.** Registered 2026-10-25, before any reading; maintainer-run;
  instrument validation (PLAN A.9.4). It decides whether the new read may be
  used: nothing else is accepted on it.
- **Instrument.** `tools/nps_read.ps1`, decided by `tools/diag/nps_read.py`
  (PROCESS *Build, bench and NPS*; Rarog's RAR-P32 method).
  - Interleaved cycles of a pinned `bench 13 3 1`, each reading the best of
    its three runs.
  - Each build's value is the median of its cycle readings. The estimate is
    the delta of the arms' means of per-build medians, with a Welch
    t-interval on those medians.
  - Two steps: step 2 runs only when step 1 says EXTEND.
- **Arms**, both four final-PGO Release builds by `build_test.ps1`, clang
  23.1.3, `bench 13` 8,437,148:
  - **Base, `a94-head-a`:** built in this checkout at the branch HEAD, whose
    engine source is identical to `4fa68b4`. The launcher checks this before
    building.
  - **Cand, `a94-head-b`:** built from a temporary detached worktree of
    `4fa68b4`.
  - The hashes are recorded in the pool files at build time and appended
    below.
- **Question.** Does the read report zero for two independent pools of one
  engine source, and does building in a worktree instead of the checkout
  change speed?
- **Hypothesis.** The pools differ only by PGO luck, a few tenths of a
  percent per build, which four builds an arm average out. The build
  directory does not enter the release binary's hot code.
- **Competing explanations for a non-zero read:**
  - the worktree's path changes code layout (Rarog D-3: a plain build
    reproduces per directory only);
  - antivirus scanning of the fresh executables (RAR-M48), which the
    read's 120 s quiet wait is meant to absorb;
  - an ordering bias that the alternation fails to cancel.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Delta 0.00%**, 80% interval [−0.35%, +0.35%].
- **Verdict:** NO_CHANGE, at step 1 with probability about 0.75, otherwise
  after step 2.
- **Confidence:** moderate-high. BAS-P15 read builds 1–2 against 3–4 of one
  pool at −0.04% [−0.22, +0.15].
- **Falsifiers:** an ACCEPT verdict, a delta of 0.5% or more in either
  direction, or a step-2 interval that excludes 0. Any of them leaves A.9.4
  open and opens a diagnosis. The first test is a pool B rebuilt in this
  checkout: if that reads zero, the worktree path is the cause.
- **Stop rule:** step 1, then step 2 only on EXTEND. A REPEAT verdict
  repeats the read once into a fresh directory. A second REPEAT means the
  host is not quiet enough, and the read stops for the maintainer.
- **Conditions:** `tools/run_a94_nps_reads.ps1 -Read null`. One pinned
  physical core (the last) at High priority. One warm-up reading per build.
  Wait for 120 s of host load under 10%. Results in
  `tools/results/a94-nps-null/`.

#### RESULT — append after exposure

- Disposition: registered; maintainer-run.
