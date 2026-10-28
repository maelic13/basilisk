# BAS-S39

**BAS-S39 — A.13.4: the tablebase probe after the TT cutoff, re-reading A.8.15**

- **Registration.** Registered 2026-10-27, before any game; maintainer-run;
  correctness repair S11 of the code audit's second pass (PLAN A.13.4). The
  change acts only with tablebases, so the gate is a re-read of A.8.15's
  tablebase-enabled gate (BAS-S19) at its registered size and conditions.
- **Defect.** The in-search WDL probe ran before the TT probe, and its store
  passed `ss->tt_pv` before the node assigned it, so the stored PV bit was
  whatever the previous visitor of that ply's record left. Measured with a
  diagnostic counter on the old order (six 7–8-man endgames, `go depth 18`,
  3–6-man Syzygy): 2,901 of 54,443 tablebase stores (5.3%) carried a bit
  other than the node's own. A TT hit that would cut also paid a tablebase
  read first.
- **Candidate.** `basilisk-a1134-fix-pext-pgo.exe`, revision `fb7e754`
  (engine commit `0c0571a`): the probe block, unchanged, now runs after the
  early TT cutoff and before the eval-difference training, Stockfish's
  Step 7 between Steps 6 and 8 (`sf_19` `edb0d9db`, `src/search.cpp:922`).
  - Release PEXT final PGO, clang 23.1.4, clean manifest, built from a
    temporary worktree at its revision.
  - `bench 13` 19,470,325; SHA-256
    `F34EE4F9FA063A5C3EA1E58506EFE67B0FB3D3867670B61B418D8520D083C3F8`.
- **Baseline.** `basilisk-a1134-base-pext-pgo.exe`, revision `128f6b2`, the
  head after A.13.3; `bench 13` 19,470,325; SHA-256
  `4440B8526987606D15745E64F85DB5FD7D60606448B10309D7FAE053E406B4DA`.
- **Mechanism.** A node the TT cuts pays no tablebase read (tablebase hits
  fell from 54,443 to 16,291 on the six positions); the store carries this
  node's PV flag (0 stale of 16,291).
- **Interactions.** At its new place the probe reads the node's depth after
  the check extension and the hindsight adjustment, as the donor's Step 7
  reads its depth after Step 5's adjustment: an in-check node entered at
  depth 0 is now probed at depth 1, and a depth-adjusted node is stored at
  its adjusted depth + 6. The static eval, and on a TT miss its eval-only
  store, now run before a tablebase return. The probe predicate, limits and
  probe depth are unchanged (D.4 owns them); the root probe is untouched.
  The decided-node `depth > 0` test is gone, since every node past the
  quiescence hand-off has depth ≥ 1.
- **Competing explanation for a neutral reading:** most probes that change
  are at nodes the TT would have cut to the same bound anyway, and the PV
  bit only steers later reductions at a re-visit.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +2 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S35, BAS-S37 and BAS-S38; report the LLR and
    the estimate, and the maintainer confirms;
  - H0: revert the engine commit `0c0571a` and return the leaf to
    `RESEARCH`.
  - Either way, an unconverted clean tablebase win or a time loss is an
    implementation defect, whatever the Elo (BAS-S19's reading).
- **Correctness reads, before the result is acted on:** CTest on the
  candidate; `tools/diag/tb_gate_audit.py conversion` on the run's PGN
  (every clean tablebase win converted, both sides).
- The fingerprint does not move (bench runs without tables), so no
  fingerprint record changes.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair-ltc.toml`: normalized `[-5,5]`,
  α = β = 0.05.
- `10+0.1`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  `SyzygyPath=D:/chess/tablebases/syzygy3456` on both engines with default
  probe options; seed 39001; concurrency 7, as BAS-S19.
- **Cap: 4,000 pairs**, BAS-S19's registered size. At BAS-S19's rate a run
  to the cap takes about 11 hours.
- Launcher `tools/run_a1134_tb_gate.ps1`; results in
  `tools/results/a1134-tb-gate/`.
- **Dry run accepted 2026-10-27**
  (`tools/results/colosseum_sprt_a1134-tb-gate_20261027_210726.*`, dry run
  `231b0079…`, manifest `d5e57561…`).

#### QUALIFICATION (before the gate)

- On `0c0571a`, Release PEXT without PGO: `bench 13` 19,470,325, exact;
  CTest 24/24, `test_search`'s tablebase fixtures included.
- The counter `tb_store_stale_pv` (DIAGNOSTIC builds, `diag kv`): on the
  six positions above, 2,901 on the old order (a measurement arm of
  `128f6b2` with the counter only), 0 on the candidate.

#### RESULT — append after exposure

- **Cap without a decision (maintainer-run 2026-10-27/28):** **+0.3 ± 3.2
  Elo** (nElo +0.7 ± 7.6), LLR +0.48 in [−2.94, 2.94] at the 4,000-pair cap
  (8,000 games); W/D/L 1960/4087/1953, pentanomial [21, 607, 2746, 596, 30].
  - 0 faults, 0 time losses; the independent PGN recount agrees.
  - The run took a fresh dry run with the registered hash
    (`colosseum_sprt_a1134-tb-gate_20261027_223046.dry-run.json`,
    `231b0079…`); manifest `ddd31fcb…`, run record `5fc93d87…`, PGN
    `e392f000…`.
- **Conversion** (`tools/diag/tb_gate_audit.py conversion`): 2,286 games
  reached a clean tablebase win, 2,285 were converted. The one miss is the
  baseline's (game 4965, Black: rook, knight and pawn against a queen,
  clean from move 67): it reported the tablebase win at every move and
  drew by threefold repetition (Kf4/Ke5 against Qc4/Qa4, moves 75–78).
  That is A-58, the root ranking that ignores repetitions already in the
  game (D.4), which the candidate shares and did not change. The candidate
  converted every clean win it held.
- **Verdict (registered rule):** the cap without a decision, so the repair
  stays as a correctness fix with no measured harm; the maintainer
  confirmed on 2026-10-28. The miss is not the candidate's. The fingerprint
  record does not move (19,470,325).
- Disposition: **cap, +0.3 ± 3.2; repair kept. CLOSED 2026-10-28.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +2 Elo, most likely the cap; not H0 at about
  85%.
- **Reading:** the cap at +0.3, at the bottom of the predicted range. As
  predicted.
- **What it teaches:** removing 70% of the tablebase reads did not show as
  a gain at `10+0.1`, so cold reads cost little once a TT cut would have
  answered the node; the repair's evidence is the stale-bit counter.
