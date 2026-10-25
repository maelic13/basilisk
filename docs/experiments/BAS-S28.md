# BAS-S28

**BAS-S28 — A.11.1: rule-50 damping out of the TT static eval**

- **Registration.** Registered 2026-10-25, before any game; maintainer-run;
  correctness repair from the Rarog audit (PLAN A.11.1; audit B-1 and C-7,
  `analysis/rarog_audit_2026-10-24.md`).
- **Defect.** `evaluate()` damps the score by the halfmove clock, and the
  default kernel stored that damped value as the TT static eval, then
  reused it at any clock, though the TT key has no clock. A position
  stored at clock 0 and read at clock 80 was served about 1.7 times the
  eval a fresh call returns there. Rarog found the same defect in its own
  selectivity core (RAR-S73, `db92790`).
- **Candidate.** `basilisk-a111-fix-pext-pgo.exe`, revision `2c0cc76`: the TT
  stores `Evaluator::evaluate_undamped`, and the search damps it once,
  after the read, at the node's own clock (negamax and qsearch). Nothing
  else changes.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 9,260,429; SHA-256
    `ED147BAA0840D76F2A3612BCC4E707C653B13204CBEAC6C4712ADD2C03AC9B18`.
- **Baseline.** `basilisk-a111-base-pext-pgo.exe`, revision `a4f9944`
  (`master`), built the same way from its own worktree with the same
  compiler; `bench 13` 8,437,148; SHA-256
  `261C155ABA32EAC6E28E759D310F76E792DE395E48571E0B3D0FA7C237254898`.
- **Mechanism.** The static eval the search prunes and reduces on is the
  evaluation of the position it is at, clock included. A stale clock
  overstates the eval of transpositions reached later in a quiet phase
  (and understates it the other way), so RFP, futility, razoring, null
  move and the qsearch stand-pat act on the wrong margin there.
- **Interactions.**
  - Correction history (`correction_value`, `update_correction_history`)
    trains on `best_score - static_eval`. It now trains on a clock-true
    eval, and its fitted scale was set on the stale one.
  - Cluster 1's eval-valued margins were fitted with the defect present.
  - The `improving` and `opponent_worsening` comparisons read `ss->eval`,
    which now carries each node's own clock.
  - The TT persists across moves while the game clock advances, so the
    defect grew with every reversible move of a game; long shuffling
    phases are where the repair acts.
  - The legacy kernel (`-DB2_CORE=OFF`) is not repaired (PLAN A.11).
- **Competing explanations for a neutral reading:** the damping moves the
  eval by about 0.5% per ply of clock difference, so most reads, made at a
  clock within a few plies of their store, barely change; and the fitted
  margins may already be centred on the stale eval's average error.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +3 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Most likely failure mode:** a reading too small for the bracket.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix
    with no measured harm, on BAS-S20's precedent; report the LLR and
    the estimate, and the maintainer confirms;
  - H0: revert the engine commit and return the leaf to `RESEARCH` with
    the result.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the conversion floors included; the new test fails on the old
  kernel and passes on the new one.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 28001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs**, as BAS-S19 and BAS-S26 on this bracket.
- Launcher `tools/run_a111_gate.ps1`; results in `tools/results/a111-gate/`.
- **Dry run accepted 2026-10-25**
  (`tools/results/colosseum_sprt_a111-gate_20261025_165430.*`, dry run
  `66fc605c…fac6`, manifest `ad1fc6d4…2de2`).

#### QUALIFICATION (before the gate)

- On `2c0cc76`, the host idle, Release PEXT without PGO:
  - `bench 13`: ON 9,260,429 (from 8,437,148, +9.8%), deterministic on a
    repeat; the legacy OFF arm 15,887,213, unchanged, as it must be;
  - CTest: ON 23/23, OFF 20/20.
- Why the bench grows by 9.8% is not measured. A plausible reading: the
  eval-valued margins were fitted on the stale eval, and a clock-true eval
  moves which nodes clear them.
- The new test, `test_search` "rule-50 TT eval": on the old kernel the
  planted clock-0 entry is served undamped (root score -748 against the
  fresh -443); on the repair both read -443.

#### RESULT — append after exposure
