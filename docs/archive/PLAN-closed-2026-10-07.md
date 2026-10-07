# Basilisk plan — closed steps A.1–A.7

Moved verbatim out of `docs/PLAN.md` on 2026-10-07 by PLAN A.8.4 (maintainer
decision 2026-10-06: PLAN keeps open work). PLAN citations of A.1–A.7
resolve here. This is a record: never take a next step from it.

## Phase A — Reset: documents, harness, instruments, baselines

- [x] **A.1** Document reset
  CLOSED 2026-09-28.
  New PLAN, GUIDE and PROCESS; AGENTS merged with Rarog's rules; DESIGN,
  HISTORY and the ledger's live sections brought up to date; the Phase-15
  roadmap archived verbatim (`6543ccf`); `check_roadmap.py` adapted to
  lettered IDs, the `(ANY TIME)` exemption, the register and the fingerprint
  check (`01cc84a`); `dev` recreated from `master`. Amended the same day:
  everything the plan takes from Rarog pinned as a verbatim snapshot in
  `docs/reference/rarog/` (`e5ad70b`), with the checker verifying its
  manifest (`e948979`), so the plan needs no live Rarog repository.
- [x] **A.2** Repository and inventory
  CLOSED 2026-09-28.
    - [x] **A.2.1** `[M]` Tracked-file cleanup
      Every one-off or superseded tracked file named with its last commit and
      removed when nothing consumes it. — CLOSED 2026-09-28.
      A consumer is a build, CI, test or tool that reads the file, or a live
      document or open leaf that directs its use; a closed record's citation
      is provenance and resolves through the commit named here. Removed, each
      recoverable as `git show 23f5557:<path>`:
      `tools/run_5911_experiment.ps1` (BAS-E17 launcher);
      `tools/diag/kbnk_{sweep,upper,refinement,holdout}_summary.py`
      (archived 6.1.c/6.1.e KBNK screens);
      `tools/diag/damping_resolution_summary.py` (archived 6.4.a probe);
      `tools/diag/freeze_group_a_head.py` and `group_a_head_v1.json`
      (the retired Group A head; GUIDE's checkpoint now defines the head);
      `tools/diag/eval_term_firing.cpp` (archived 5.9.1 firing check; C.1's
      `EvalTrace` coverage owns activation). Kept because an open leaf owns
      them: the endgame instruments, cohorts and label tools (C.2.4, C.5),
      `passer_king_geometry.py` (C.6), `rook_ending_failure_profile.py` and
      `narrow_node_probe.py` (C.5.4), `tools/texel/phase911.ps1` (C.2.1),
      `tools/spsa_configs/config_*.json` (retired by A.3.2's generated
      surface), `docs/release_tiers.md` (A.4.2, E.3); `analysis/` records,
      logos and the UCI specification are not one-off files. The remnants of
      the withdrawn August Colosseum adoption (`3cbf90b`) were removed so A.3
      starts clean: the Texel README's datagen recipe and
      `phase911.ps1`'s header point to `datagen.ps1` again (the script
      always required its manifest), and `build_test.ps1`'s TUNE comment
      names weather-factory.
    - [x] **A.2.2** `[M]` Branch and tag disposition
      Each `archive/*` and `oracle/*` tag gets its citing document, reason and
      retirement condition. — CLOSED 2026-09-28.
      Tags whose condition has fired are proposed for removal; deleting a
      remote tag stays the maintainer's command. The register is HISTORY's
      *Preserved commits*: `oracle/hybrid`, `oracle/hybrid-diag` and
      `archive/nnue-local` are kept with their retirement conditions (F.4
      now cites the last); `archive/backup` and `archive/nnue-origin` are
      ancestors of kept tags and `archive/arm_fix` is BAS-P07's rejected
      diff, so all three are proposed for removal. Only `master` and `dev`
      exist as branches. The ledger's stale branch pointers (BAS-D03,
      BAS-P07) now name the tags.
    - [x] **A.2.3** `[R2]` Feature, option and parameter inventory
      Every `search_params.h` coordinate, evaluation parameter, TUNE-only
      option and `Diag` counter classified live, inert-with-owner or dead.
      The "exposed but inert" knobs in `search_params.h` are listed with
      their owners: capture futility, SEE-quiet pruning, qsearch quiet
      checks, the double-extension cap and `PostLmrHistScale`. The inventory
      is the input to B.0's survivor list and B.1's removals. — CLOSED
      2026-09-28: `analysis/parameter_inventory_v1.md`, BAS-D19. Four of the
      five were inert; the double-extension cap is a live bound (16). Capture
      futility is dead at the default LMR table but wakes at `LmrBase` >= 100.
      B.1 removes 7 coordinates and the drifted `KBNK Drive` option.
- [x] **A.3** Colosseum CLI as the main harness
  Harness: Colosseum CLI as the main path (maintainer decision 2026-09-28). —
  CLOSED 2026-09-29.
    - [x] **A.3.1** `[I1]` Run files, `colosseum.ps1` and the shared guards
      Run files and wrapper: port the snapshot's `tools/colosseum.ps1`, run
      files and guard suite, adapted to Basilisk's CMake manifests; the
      snapshot's `tools/colosseum/colosseum.pin.json` (`cli-v0.2.0`) is the
      starting pin.
      Every guard is shared with `sprt.ps1` and `spsa.ps1` through
      `harness_common.ps1`, so the two paths cannot drift. Guards: refuse a
      dirty or wrong-revision candidate; require the binary, hash, compiler,
      flavor and bench manifest (`-ExpectBench`); refuse an option the engine
      does not advertise; refuse a busy host or an unpinned runner (SHA-256
      pin); require natural termination. These absorb the archived 6.6.f.
      Checks: the resolved configuration matches a recorded `sprt.ps1`
      manifest field by field; a guard suite breaks one input per case and
      gets the refusal that names it; a short live match completes with zero
      faults and an independent recount from the PGN. — CLOSED 2026-09-29:
      the pinned `cli-v0.2.0` wrapper and committed run files share strict
      CMake-sidecar, option, host-idle and fault guards with the backup paths;
      15/15 negative/control guard cases behaved, fastchess and Colosseum
      agreed on 26/26 comparable resolved fields, and the two-game live smoke
      had zero faults with its `[0,0,1,0,0]` pentanomial reproduced from PGN.
      Ignored evidence: `tools/results/sprt_A31A_vs_A31B_20260929_095323.*`,
      `tools/results/colosseum_match_a31-parity_20260929_095349.*`, and
      `tools/results/a31-live-smoke-process/` with
      `tools/results/colosseum_match_a31-live-smoke-process_20260929_095951.*`;
      their manifests carry the runner, engine, book, configuration and
      artifact hashes.
    - [x] **A.3.2** `[I1]` SPSA tune path from the X-macro
      Tune path: the X-macro table in `src/search_params.h` generates the SPSA
      surface and the Colosseum tune file. — CLOSED 2026-09-29.
      The tune shape is 15 slots × 30 games per iteration (RAR-M62, 1.65×
      the old throughput). `-SeedFrom` chains the blocks of PLAN rule 7c; the snapshot's
      `tools/spsa_config_to_colosseum.py` and `tools/spsa_block_rule.py` are
      the models.
      `spsa.ps1`'s fixed config-group list gains the generated path, and its
      5,000-iteration floor, which cites the archived "PLAN gate 11", is
      reconciled with rule 7c's 2,000-iteration blocks. A one-iteration tune
      on a temporary surface is the smoke. The hand-written
      `tools/spsa_configs/config_*.json` and the README's warnings about them
      (which name `.STALE` files no longer in the tree) retire in the same
      change.
      `generate_spsa_surface.py` now emits the ordered 48-coordinate
      weather-factory JSON and the 2,000-iteration Colosseum tune/run files;
      perturbations are `max(2, round(range / 16))`, and `--check` rejects
      drift. `spsa_colosseum.ps1` fixes production blocks at 15 × 30 and
      accepts `-SeedFrom`; `spsa_block_rule.py` applies the registered
      three-mover rule against each block's own seeds. The backup
      `spsa.ps1` consumes the generated surface at 30 games/iteration and
      uses the rule-7c 2,000-iteration floor. Five hand-written historical
      vectors retired. A fresh TUNE build reproduced **14,978,465**; the
      one-iteration/30-game temporary-surface smoke completed with 30 normal
      terminations and zero engine, time or infrastructure faults, and a
      second dry run proved `-SeedFrom` bound the completed result. The
      registered 15-slot shape targets the maintainer's Ryzen 9 5950X; this
      hybrid host exposes five eligible P-cores, so its smoke deliberately
      used one slot. Ignored evidence:
      `tools/results/a32-smoke/` and
      `tools/results/colosseum_spsa_a32-smoke_20260929_100945.*`.
    - [x] **A.3.3** `[M]` PROCESS *Harness* section
      PROCESS *Harness* section finalised: Colosseum main, fastchess and
      weather-factory maintained as backup and second opinion until at least
      the classical release, with the cross-check triggers. — CLOSED
      2026-09-29. The procedure names both guarded Colosseum entry points,
      ownership of run conditions, pre/post-run proof, resume and tune-chain
      semantics, the three backup triggers, topology rules and main/backup
      commands.
- [x] **A.4** Build and toolchain
  CLOSED 2026-09-29
    - [x] **A.4.1** `[I1]` Toolchain refresh and freeze
      Toolchain refresh and freeze (archived 7.0): inventory the compiler, C++
      library, CMake, Ninja and profile tools on Windows, Linux CI and macOS.
      Compare the current and newest stable versions one axis at a time.
      Require CTest, sanitizers, exact search agreement, ISA checks and
      pooled release/PGO throughput before selecting the faster
      non-regressing line. — CLOSED 2026-09-29. Retained the deployed
      compiler/library lines because forward Linux variants had no native
      pooled result and Windows already used current LLVM 22. The audit found
      and repaired a false PEXT-tier contract, added an ISA verifier with a
      known-bad control, kept the startup launcher at baseline ISA, and pinned
      CI runner generations. Release and sanitizer CTest passed 13/13 with
      the exact **14,978,465** fingerprint; repaired final-PGO PEXT measured
      **+7.45%** [**+7.22%, +7.75%**] over the pre-repair tier. Evidence:
      `analysis/toolchain_refresh_v1.md`.
    - [x] **A.4.2** `[I1]` Build flavors and manifests for arms
      Build flavors for arms: `build_test.ps1` builds and manifests release,
      tune, diag and umbrella-switch arms.
      Each manifest carries the hash, source revision, compiler, flavor and
      bench. CI covers the CMake option and ISA combinations rather than
      single options, and distinguishes production from diagnostic builds
      (archived 6.6.g). The matrix includes the `TEXEL` target, repaired first:
      `tools/texel/tuner.cpp` does not compile at `aecbd93` (A.2.3). — CLOSED
      2026-09-29. TEXEL now uses public board reads plus an opaque coherent
      position snapshot and reconstructs the seven-position fixture exactly.
      Final-PGO release, tune, diagnostic and umbrella-probe builds each
      reproduced **14,978,465** with clean schema-2 manifests; their UCI
      surfaces carried 9, 60 and 11 options respectively, and both PGO phases
      retained the umbrella value. CI exercises TUNE across every existing
      ISA/platform row, diagnostic and TEXEL PEXT combinations, and known-bad
      overlapping flavor/ISA controls. Release CTest passed 13/13 and the
      final PGO PEXT ISA contract passed.
- [x] **A.5** Instruments for the search programme
  CLOSED 2026-09-30.
    - [x] **A.5.1** `[I1]` Fixed-budget probe
      WAC solved at 100k and 400k nodes and at a fixed PV depth, plus oracle
      best-move agreement on `suite_v1.epd` at 300k nodes, with per-position
      records. — CLOSED 2026-09-29.
      Ported from the snapshot's `tools/diag/fixed_budget_probe.py` onto
      Basilisk's `wac` command and `tools/diag/run_suite.py`. `wac nodes N`
      records completed and total work separately; `wac depthpv N` records
      every completed PV head plus first/stable solution depths. The oracle
      path rejects missing options, ignores incomplete aspiration bounds and
      records comparable best-move agreement per position. Parser falsifiers,
      three live WAC command forms, a live 300k same-binary agreement control,
      release CTest 13/13 and bench **14,978,465** passed.
    - [x] **A.5.2** `[I1]` Reference-anchored branching profile
      Branching profile: the reference-anchored geometric branching factor
      over depths 4–14, one fresh process per depth. — CLOSED 2026-09-29.
      Basilisk against the classical oracle and modern Stockfish, with
      per-position ratios and the median beside the aggregate: one position
      of forty once decided an endpoint measure (BAS-X13).
      `tools/diag/branching.py` is the starting point, and the snapshot's
      `tools/branching_profile.ps1` the model. The v2 JSON binds both reference
      binaries and the corpus by SHA-256, fixes Hash 64 and Threads 1, records
      cumulative and iteration growth plus every position, and compares all
      arms on one common completed-position set with explicit exclusions. Six
      parser, outlier, common-denominator and lifecycle tests plus a real
      Basilisk UCI option handshake passed. No profile was measured on the
      busy host; B.0 performs the first three-arm run on an idle host.
    - [x] **A.5.3** `[I1]` Counter summation and decision trace
      Counters and decision trace: `Diag` counters summed per position at
      sampling stride 1 with their units named, and a diag-only decision
      trace. — CLOSED 2026-09-30.
      The trace is bounded to plies 1–2 under `searchmoves` and prints every
      prune, reduction and extension with its inputs. Rarog's trace found
      two seed defects that no counter could see: a static margin overriding
      a mate in one, and a count-based skip dropping a mating quiet move.
      The snapshot holds the counter tools (`tools/diag/bench_counters.py`,
      `tools/diag/phase4_differential.py`, `analysis/phase4_counter_spec.md`)
      and the oracle's counters (`refs/oracle-hybrid-diag.patch`).
      Every counter becomes machine-readable: 16 of the 57 are printed only
      as prose today (A.2.3). The leaf also decides the 73 `diag_`
      increments that run in release builds with `Diag` off, priced by
      PROCESS's NPS method.
      All 57 core counters now have named units, strict per-position records
      and checked sum identities at stride 1. `DecisionTrace` is compiled only
      into diagnostic/tune builds, requires `Diag=true`, one thread and one
      `searchmoves` root, records plies 1–2 into fixed-capacity storage, and
      rejects overflow. The release-counter arm was bench-identical at
      **14,978,465**. Its pooled two-build PGO comparison found compiling the
      increments out worth **+0.90% median NPS**, 95% CI **[+0.54%, +1.02%]**,
      best-of **+0.75%**, faster in **16/16** alternating rounds; the preceding
      two-build self-pair passed at **−0.17%**, 95% CI **[−0.36%, +0.20%]**.
      Production therefore compiles the increments out; diagnostic and tune
      builds retain them, and `RELEASE_DIAG_COUNTERS` remains only as the
      reproducibility arm.
    - [x] **A.5.4** `[I1]` Matched ablation mask
      One bit order on Basilisk and on the `oracle/hybrid` build, compiled
      away in production, every bit proven live by a moved node count; the
      oracle side exists as the snapshot's `refs/oracle-hybrid-ablate.patch`,
      written for the same classical-Stockfish search. — CLOSED 2026-09-30
      `AblationMask` uses bits 0–7 for razoring, reverse futility, null move,
      ProbCut, IIR, shallow move pruning, extensions and LMR. It exists only
      in the `Ablate` flavor; the final-PGO builder propagates that flavor
      through both child builds, while production advertises no option.
      Mask 0 and production both reproduce **14,978,465** at `bench 13`.
      `tools/diag/ablation_liveness.py` ran eight suite-v1 positions at depth
      9 and proved every single bit live in both engines. Against mask 0,
      Basilisk's aggregate node deltas were **+78,530, +123,314, +5,496,
      −19,430, +40,431, +363,230, −112,186, +771,104**; the patched pinned
      oracle's were **+24,158, +26,339, −10,116, −24,741, +2,875, +557,562,
      −44,400, +115,520**. The hash-bound record is
      `tools/results/a54-ablation-liveness.json`.
    - [x] **A.5.5** `[I1]` PGN conversion instrument
      Conversion instrument over PGN: port the snapshot's
      `tools/diag/conversion_audit.py` and
      `tools/diag/export_tournament_pgn.py`. — CLOSED 2026-09-30
      The hash-bound `5e539523` export contains 39,600 games, skips none and
      reproduces the snapshot's `conversion_release_basilisk_1.9.3_v1.json`
      exactly apart from path and generation time: against the six anchors,
      **94 draws and 12 losses after a persistent piece-up in 3,600 games**
      (**26.1 / 3.3 per 1,000**), with all counts, parameters and termination
      classes identical. PGN SHA-256 is
      `fe0cf072acaeba95427a9fa549ca50a9662350380c664f9071dfa0ffe0b863d5`.
      Seven synthetic tests cover perspective, persistence, material
      signatures, aggregation, filtering, PGN round-trip and read-only SQLite
      export/header rewriting. The earlier **17.5 / 5.0** text belonged to the
      separate Super Rating Tournament rather than the named frozen JSON; it
      remains an A.7.1 result to reproduce from that tournament's PGN.
    - [x] **A.5.6** `[R2]` Reference-anchored canaries
      WAC positions, including quiet key moves and quiet mate threats,
      anchored at the depth classical Stockfish solves them. — CLOSED
      2026-09-30.
      A changed canary is recorded with its cause and never re-blessed.
      The hash-bound `canary_v1.json` freezes **126** local classical-oracle
      anchors: oracle stable depth at most 6 plus WAC.001, with an independent
      oracle solve at 100k nodes. Of these, **77** are required because the
      1.10.1 baseline also stays correct by oracle depth + 2 and solves at
      100k; **24** have quiet key moves. The other **49** remain named gaps:
      a later pass is reported but never silently added to the gate. WAC.001
      is the quiet mate-threat gap, anchored locally at oracle depth 10 with a
      depth-12 allowance; the baseline fails both depth and node conditions.
      The imported Rarog counts (116/242 and WAC.001 at depth 9) were not
      reused because they were produced under Rarog's evaluation. Two complete
      runs matched in every non-time field, the baseline check passed 77/77,
      eight unit tests passed, a missing UCI option was rejected live, and a
      known regression fails while a new pass is diagnostic only. A frozen
      manifest refuses overwrite; changing the cohort requires a new version.
- [x] **A.6** `[R2]` Codebase consolidation analysis
  `src/search.cpp` (3,125 lines) and `src/eval.cpp` (2,148 lines) mapped into
  target modules; the B.1 and C.1 move tables; dead code; the seams a cluster
  needs. Refactors nothing. — CLOSED 2026-10-01.
  `analysis/codebase_consolidation_v1.md` separates engine-shared,
  per-thread, per-search configuration and mutable search state; fixes the
  target modules and ordered move tables; and preserves the hot recursive
  worker and accumulator boundaries behind pooled-PGO floors. The earlier
  2,848-line search count predated A.5. `time_limit_` and the unconsumed
  `RootMoveStat` collection are dead; the latter also allocates from the root
  recursive path. B.1 removes those, A.2.3's seven inert coordinates and the
  drifted `KBNK Drive` option. C.1 retains the 23 zero evaluation groups for
  C.0 rather than mistaking traced, consumed zero weights for dead code.
- [x] **A.7** Baselines on the 1.10.1 binary
  CLOSED 2026-10-06. Phase A reopened the same day with A.8.
    - [x] **A.7.1** `[V]` 1T pool baseline from the Super Rating Tournament
      1T pool baseline, zero games: census of Basilisk 1.10.0's head-to-heads
      in the Super Rating Tournament PGN
      (`D:/chess/results/super_rating_tournament.pgn`, SHA-256 in BAS-X34),
      and its conversion rate with A.5.5 on the same file. — CLOSED
      2026-10-01.
      The exact 172,200-game PGN reproduced BAS-X34's hash. Direct records
      were Houdini 3 15-26-159, Critter 1.6a 25-35-140, Fritz 16 25-45-130
      and Rybka 4.1 44-38-118, 200 games per pair, balanced colours and zero
      unfinished. Against the six conversion anchors, 1.10.0 recorded 20
      draws and 8 losses after a persistent piece-up advantage in 1,200 games:
      **16.67 / 6.67 per 1,000**. BAS-M08 replaces HISTORY's unsourced
      2026-09-04 figures; raw PGN and JSON outputs remain ignored.
    - [x] **A.7.2** `[V]` 4T gauntlet against the targets and Rarog 2.4.0
      4T gauntlet against the four targets and Rarog 2.4.0, 400 games per
      pair, no adjudication, maintainer-run; a null pair first if the 4T setup
      changed since BAS-M02.
      BAS-M09's duplicate null was withdrawn before exposure by maintainer
      decision 2026-10-01: Basilisk uses the same pinned Colosseum 0.2.0 binary
      qualified in the Rarog snapshot, and no Rarog engine verdict transfers.
      BAS-M10 freezes the remaining 2,000-game gauntlet as 200 cycles of two
      colour-reversed games against each opponent. Its clean dry run resolves
      1,000 distinct openings, all five fixed ratings, heterogeneous 4T option
      names, zero permitted faults and symmetric four-core placement.
      Amendment 1 (2026-10-05, before any game) moves the run to the
      production 5950X: concurrency 3, the Basilisk and Rarog pins rebuilt
      for that host, everything else unchanged; its clean dry run is at
      `943f3f8`.
      BAS-M10 was voided at 659 games by two Houdini 3 crashes against its
      zero-fault rule. BAS-M11 re-runs it in the Colosseum desktop app with
      Hash 512 MB per engine and crashes left to maintainer judgement, read from
      Basilisk's performance against the five fixed ratings. Rarog 2.5.0 joins
      as an unrated sixth opponent for comparison (2,400 games), outside the
      verdict. — CLOSED 2026-10-05: 4T performance **3040** [3026, 3054]
      against the five fixed 1T ratings (1T: 2994); Houdini −203, Critter
      −129, Fritz −177, Rybka −56, Rarog 2.4.0 +14; Rarog 2.5.0 −312. Five
      Houdini setup crashes excluded (BAS-M12); deviations in BAS-M11.
    - [x] **A.7.3** `[V]` Oracle deficit meter G(0)
      Rebuild `oracle/hybrid` with the 1.10.1 evaluation, then 3,000 paired
      games at equal time, no adjudication, maintainer-run.
      A prediction is frozen first; BAS-O01's +322.7 is the prior, on a
      coarser estimator.
      Prepared 2026-10-05 as BAS-O05 while A.7.2's re-run was paused by the
      maintainer: the oracle rebuilt from tag `oracle/hybrid` with `v1.10.1`'s
      `src/` by `tools/oracle/build_oracle.ps1` (conformance 0 mismatches,
      known-bad control fails), 3,000 paired games by
      `tools/run_a73_oracle_g0.ps1`; clean dry run at `a7bc05b`. — CLOSED
      2026-10-05: **G(0) = +312.6 ± 17.8** over 3,000 paired games, 0 faults,
      inside the frozen +250 to +350; the deficit is unchanged from 1.9.3.
    - [x] **A.7.4** `[V]` Pooled-PGO NPS baseline
      Pooled-PGO NPS baseline with `nps_ab.ps1`: a self-pair validated first,
      at least two PGO builds per arm, interleaved, idle host.
      Prepared 2026-10-05 as BAS-P12: `tools/run_a74_nps_baseline.ps1`
      builds four final-PGO 1.10.1 binaries at `3e5294be`, runs a self pair
      over all four and builds 1–2 against 3–4; wiring smoke-tested. — CLOSED
      2026-10-06: **4.127M NPS** pooled; self pair +0.11%, build pools +0.04%;
      all three BAS-P12 predictions held.

