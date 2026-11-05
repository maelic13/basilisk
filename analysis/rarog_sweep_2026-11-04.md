# Rarog sweep, 2026-11-04: steps Basilisk had neither done nor planned, and the 2026-11-05 restructure

- State / class: delivered; the source of the PLAN restructure of 2026-11-05, `R2`
- Owner / date: agent, 2026-11-04 (sweep) and 2026-11-05 (restructure), at
  maintainer request
- Decision taken: maintainer, 2026-11-05. Every gap below goes where the
  table says; the correctness items go before the next release; the open
  steps are renumbered in the order they run, every measurement in the step
  that reads it, and no step depends on a later one.

## Scope and method

- **Rarog sources**, read from the live repository at `75f1c9b` (2026-11-04)
  under A.12's exception, at the maintainer's request: every step of the
  current `docs/PLAN.md`, closed ones included; the five archived roadmaps
  (`PLAN-legacy-2026-07-29.md`, `PLAN-closed-leaves-2026-09-14.md`,
  `PLAN-phase4-2026-09-09.md`, `PLAN-closed-2026-10-05.md`,
  `PLAN-closed-2026-10-22.md`); commits since 2026-10-20; the experiment
  records and analyses those steps cite. The thread-scaling legs were
  imported later, from `019646a`, as BAS-X41.
- **Basilisk** at `bdf09e4` (2026-11-04), against PLAN, the closed-step
  archives, HISTORY, EXPERIMENTS, PROCESS and the earlier imports that
  already mapped Rarog items (`analysis/rarog_audit_2026-10-24.md` and its
  appendices, `analysis/rarog_parity_review_2026-10-06.md`,
  `analysis/triumviratus_review_2026-11-03.md`). An item one of those mapped
  or refused with a reason counts as covered.
- **Two read-only sweeps** (current plan; archives), each grepping
  Basilisk's documents under several names before calling an item absent.
  The coordinator re-read in code the claims marked ✔, and re-checked on
  2026-11-05 that every gap was still absent from PLAN. No engine was built
  or run.

## The placement finding (done 2026-11-05)

Rarog's D.2 found that Colosseum's automatic slot holds both SMT siblings of
its cores and Windows packs a multi-thread engine onto them (RAR-P54,
RAR-M82); its "worse scaling than Stockfish and Reckless" verdicts were the
placement. Basilisk's wrapper had the same automatic slot; BAS-M11's 4T pool
was unpinned and BAS-R09's 4T gate ran packed. Ported as `84f03b8` (every
multi-thread run sibling-free, self-relative legs, the host sampler and
`tools/diag/smp_read.py`); the sibling-free legs of Stockfish 18 and
Reckless are BAS-X41.

## Gaps and where they went

| # | Rarog source | Gap in Basilisk | Placement |
|---|---|---|---|
| 1 | D.5.3 (DRIVER-12) | ✔ Under `go depth N` the helpers drop the depth limit and `merge_results` takes the deepest result, so at Threads > 1 the engine can report depth N+1 and play a helper's move; Stockfish answers from the main thread under a depth limit | B.9.1 |
| 2 | D.6.7 (NODE-8) | ✔ At `ply >= MAX_PLY` (128) search and quiescence return the static evaluation even in check, before the draw test; Stockfish 19 returns a draw in check and its cap is 246; Basilisk lines reach seldepth 112 | B.5 |
| 3 | legacy 7.3 | ✔ No rule-50 bucket in the TT key (Stockfish's `adjust_key50`); parked in Rarog while games were adjudicated, a reason both engines have dropped | B.6.1 row |
| 4 | D.6.14 (BOARD-9) | ✔ The non-pawn correction key leaves the kings out (`board.cpp`); Stockfish's includes them; a bare king's key is 0, one entry for every such position (B.3.0's packet §1.4) | B.6.1 row |
| 5 | D.9 (PROTO-11, PROTO-24) | ✔ Fathom calls `exit(EXIT_FAILURE)` when a table fails to map, mid-search with no `bestmove`; a non-ASCII `SyzygyPath` cannot load on Windows (narrow `CreateFile`) | B.9.1 |
| 6 | D.1.4 instrument finding | ✔ The wrapper checked option names, not values, and the engine clamps silently | done, `84f03b8` |
| 7 | phase 4.5c (RAR-S40) | ✔ The RFP margin adds the correction magnitude when `eval` is a TT bound without it; Rarog's arm without it read −4.50% nodes on 9.0% of nodes | B.4 (B.16.2 by eval source) |
| 8 | Phase F rule (2026-10-22) | No rule for the HCE as a selectable evaluator after F.6 refits the search for the network | Phase F rules, F.6, F.9 |
| 9 | E.1 (H30) | No release read with Syzygy loaded | E.1 |
| 10 | legacy 10.0(c) (RAR-S54) | A blind uniform 15% shift toward less pruning, +4.06 ± 3.71 over 14,196 games, which led to Rarog's +15.33 refit | B.6.2 prior |
| 11 | D.2.3 (RAR-P52) | PGO trains Threads 1 only; Rarog's multi-thread copies were cold (+2.4% at 4T once trained) | B.8.5 |

Lessons missing from steps Basilisk had:

- **D.2**: Rarog's legacy SMP negatives (RAR-R04 a symmetric early stop
  vote −15.85; RAR-R06 helper-history blending −0.52, with the advice not to
  share continuation or pawn histories on that prior; RAR-R07–R10 helper
  staggering flat once read with enough samples, and the method: one process
  per cell, arms alternating, 40–80 pairs, a null under the same protocol)
  and its zero-game time-to-depth and hit-rate reads before any games.
- **D.1**: pooled best-move instability lost −5.54 (RAR-R05); it pays only
  from a completed root (+6.46, RAR-X06). The thread-instability factor moves
  to D.2, after the helper design it depends on.
- **B.11.1**: judge Basilisk by the pool score, not the Rarog pairing
  (Rarog legacy 10.0: the sibling head-to-head differed from the pool by
  35–45 Elo).
- **B.8.4**: a `LargePages` UCI option so one binary reads both arms (Rarog
  RAR-M77, +2.7% NPS, +10.3 ± 5.9 Elo at 4T); macOS ships arm64 only, where
  no userspace superpage request exists (Rarog D.7, 2026-11-02).
- **B.9.1**: Hash floored to a power of two without a notice (TT-7).
- **C.3**: the donor's king-danger attack maps x-ray (bishops through
  queens, rooks through queens and own rooks, pinned pieces cut to the king
  line); Rarog's C.3 fixture came from a donor patched to plain maps.
- **C.7.1**: the colour-mirror test does not cover the endgame fixtures
  (Rarog EVAL-2, D.5.17).

## The restructure of 2026-11-05

Dependency errors the old order carried, each fixed by the move named:

1. B.4 fitted quiescence's SEE pruning on SEE's created-pin and
   promotion-recapture defects, repaired only in B.15.1: the repairs open
   B.4.
2. B.9's floor-legality patch was "done between clusters, never inside
   one", yet scheduled after B.4 and B.5: it opens B.4, the next point
   between clusters.
3. C.3's eg king danger had to change with C.7's phase: phase and tempo
   move into C.3, before the other families' refits.
4. C.12 audited the lazy thresholds, a behaviour change, after C.11's
   freeze and C.10's `LAZY_MARGIN` refit: the audit moves to C.10.
5. D.1 tuned best-move instability across threads before D.2's helper
   design: the factor moves to D.2.

Measurements moved to the step that reads them:

- D.1's loss analysis reads C.11's pool gauntlet (C.12 is bit-exact, so its
  games are D.1's entry engine), not B.9's, which Phase C makes stale; D.1
  re-reads the wall-clock overhead on its own head.
- B.5's S6 counter is read at 1T; the 4T read is D.2's.
- C.5.2's E7 measurement (the correction at scaled-ending returns; B.14
  already took the draw verdicts) moves to C.10, which reads its switch.

Order changes without a dependency error: the endgame cluster follows pawns
and material, which its scalers and recognisers act on (C.0 still orders by
expected value within that constraint); the lifecycle and protocol work
(D.3.2) and the release-visible defects run before the checkpoint and the
1.11.0 release (maintainer: every correctness fix before the next release);
the cleanup precedes large pages and the speed pass. B.6.2 runs
unconditionally (maintainer). HISTORY's number map resolves every moved
identifier.
