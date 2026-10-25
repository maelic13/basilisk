# BAS-P19

**BAS-P19 — what MSYS2's clang 22.1.8 to 23.1.3 step cost in speed, at identical source**

- **Registration.** Registered 2026-10-25, before any reading; maintainer-run;
  a toolchain speed read owed under `agents/measurement.md`'s compiler rule
  (PLAN A.9.3, A.9.4).
  - MSYS2 moved the Windows clang from 22.1.8 to 23.1.3 on 2026-10-23, and
    no read covers the step.
  - It is read only after BAS-P18 validates the instrument; the launcher's
    order is null first.
- **Arms**, A.8.20's head `9c93d15` (`bench 13` 14,978,465) on both sides,
  four final-PGO Release builds each:
  - **Base, `a94-a820-clang22`:** BAS-P15's pool, clang 22.1.8, built
    2026-10-08 in this checkout. SHA-256 `19326347…`, `3169742D…`,
    `84191797…`, `3C6CAB7E…`, all clean.
  - **Cand, `a94-a820-clang23`:** clang 23.1.3, built at run time from a
    temporary detached worktree of `9c93d15` with that revision's own
    `build_test.ps1`. The hashes are appended below.
- **Question.** At identical source and fingerprint, how much faster or
  slower is the clang 23.1.3 build?
- **Hypothesis.** LLVM 23's code generation changes the hot loops. Rarog's
  LLVM 22 to 23 step kept its fingerprint and cost 1.5–1.7% pooled with PGO,
  and 3.6% without it (RAR-P36 to RAR-P38).
- **Competing explanations:**
  - no change, because C++ and Rust stress different code generation;
  - a gain;
  - the worktree build path, which BAS-P18's null clears or convicts;
  - the base pool's age, which is not a factor: the bytes are the same,
    and the read interleaves the arms.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Delta −1.0%**, 80% interval [−2.5%, +0.5%].
- **Verdict:** FAIL (a regression) with probability about 0.55.
- **Confidence:** low; the only prior is Rarog's Rust step.
- **Falsifiers:** a delta outside [−2.5%, +0.5%] falsifies the size. A
  PASS with a non-negative estimate falsifies the prior that LLVM 23 costs
  Basilisk speed.
- **Stop rule:** no-regression mode against −0.5%. Step 1 passes at −0.1%
  or above and fails at −0.9% or below. Step 2 runs only in between, and
  fails at −0.5% or below with its upper bound under 0. A REPEAT verdict
  repeats the read once into a fresh directory.
- **Consequence:** speed only, no Elo claim. PROCESS's prior converts it at
  about 2 Elo per 1%. A FAIL is a measured cost of the toolchain step;
  whether to pin the Windows release compiler belongs to A.10.4 and the
  maintainer.
- **Conditions:** `tools/run_a94_nps_reads.ps1 -Read clang`, otherwise as
  BAS-P18. Results in `tools/results/a94-nps-clang/`.

#### RESULT — append after exposure

- **Result (maintainer-run 2026-10-25, 09:44–09:51 UTC).** **+0.89%**,
  95% t-interval [+0.23%, +1.54%] (Welch df 4.3), **PASS at step 1** in
  no-regression mode. Clang 23.1.3 is no slower.
  - Base, clang 22.1.8: mean of build medians 4.1863M. Builds 4.179M,
    4.187M, 4.181M and 4.198M.
  - Cand, clang 23.1.3: mean 4.2235M. Builds 4.212M, 4.206M, 4.231M and
    4.246M; SHA-256 `CB656940…`, `4C9484A1…`, `381E412E…`, `D934ED5A…`, all
    clean `9c93d15` at 14,978,465.
  - Cycle levels: base 4.176M and 4.196M, cand 4.214M and 4.233M; no cycle
    is disturbed.
  - Host load was 3.8% and 5.8%, with no engine, harness or build process.
    Fingerprints are identical. The archive is
    `tools/results/a94-nps-clang/`.
- **Reading.** The registered rule passes. The interval excludes 0, but
  this read was registered as a no-regression check, so it claims no gain.
  Under the gain rule the same +0.89% would sit just below step 1's +0.9%
  and would extend.
- **Calibration.** It fell outside the 80% interval [−2.5%, +0.5%] on the
  high side, and the sign was wrong.
  - The falsifier fires: a PASS with a non-negative estimate refutes the
    prior that LLVM 23 costs Basilisk speed.
  - Rarog's Rust step (−1.5% to −1.7%) did not transfer to Basilisk's C++.
  - BAS-P18 rules out the worktree path as the cause: if anything it read
    the worktree pool slower.
  - Confidence was registered as low, and that was right.
- **Consequence.** The clang 22.1.8 to 23.1.3 step owes nothing in speed.
  Any pinning of the Windows release compiler in A.10.4 is a
  reproducibility decision, not a speed one.
- **Lesson.** A sibling engine's compiler-step result is a weak prior
  across languages: read the step here, as `agents/measurement.md` now
  requires.
- Disposition: COMPLETE, +0.89% [+0.23%, +1.54%], no regression.
