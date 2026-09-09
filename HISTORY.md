# Basilisk history

⚠ **THIS FILE IS HISTORY. It does not tell you what to do next.** Read
`GUIDE.md` for the current step and `PLAN.md` for what it involves.

## Numbering

Phases 1–14 belong to the archived roadmap
([docs/archive/PLAN-2026-09-09.md](docs/archive/PLAN-2026-09-09.md),
[docs/archive/GUIDE-2026-09-09.md](docs/archive/GUIDE-2026-09-09.md)). Every
`6.x`–`14.x` identifier in `EXPERIMENTS.md`, `analysis/` and commit messages
resolves there. Phase 15 in the current `PLAN.md` is the only active phase and
the only new numbering.

## Releases (Phases 1–4)

| Release | What it established |
|---|---|
| 1.0.0 – 1.8.0 | Board, move generation, UCI, PVS/qsearch, TT, histories, SEE, Syzygy, time management, Lazy SMP, reproducible tests, accepted HCE |
| 1.9.0 | State, repetition/rule-50, TT/mate and SEE/pin correctness; staged ordering, correction/history, root-instability timing, dense TT |
| 1.9.1 | Centralized parameters, invariants, fuzzing, CI, telemetry; behaviour-identical PGO speed pass at +4.34% NPS |
| 1.9.2 / 1.9.3 | SPSA/MT harness and helper clock/node/thread safety repaired; four-thread bundle accepted; PGO tool matching fixed |

## The 2026 development line (Phases 5–6), frozen at `d0f2627`

Between 1.9.3 (2026-08-01) and the freeze decision (2026-09-09) the `dev`
branch accumulated 170 commits, none released. What they established, each
with its ledger evidence:

| Result | Evidence |
|---|---|
| The KBNK mate drive completed and provably isolated; conversion measured on fixed-seed cohorts with deterministic CTest floors | BAS-E42 and the 6.1 record |
| Endgame Group A gated and accepted; the accepted HCE line about +12 Elo over 1.9.3 | 6.2 record, BAS-E4x |
| Rook-ending draw scaling (KRPKR, KRPPKRP) with a floored discount, accepted at +3.29 ± 4.61 Elo over frozen Group A; bench 12,568,898 | BAS-E54 |
| Passed-pawn king approach studied and dispositioned | 6.3 record |
| Magnitude and coverage audit of the endgame evaluator; reopened work recorded with its reasons | 6.4 record, "Reopened work, 2026-09-03" |
| The TT publication redesign: atomic whole-record word accepted for correctness, the coherence repair reverted on measured throughput with the risk recorded | BAS-X2x rows, commits `2bf43fb`, `c378706` |
| A peer audit of the shared board lineage found three real SEE defects in Rarog; the same kernel shape is in Basilisk and had no fixtures | BAS-X22 |
| Pool position on 2026-09-04 at `3+0.03` 1T: Houdini 1.5a −197, Critter 1.6a −187, Fritz 16 −178, Rybka 4 −84, Rarog 2.4.0-dev +26 | BAS-X11 |

Open at the freeze and **not continued**: 6.6 instrument and gate integrity,
6.7–6.11 remaining endgame families and closure, Phase 7 board correctness
(replaced by Phase 15's bounded repairs), Phase 8 corpus and complete HCE
refit, Phase 9 classical search consolidation, Phases 10–14 NNUE and
platforms. Their evidence, retry triggers and dispositions remain in
`EXPERIMENTS.md` (section 9 is the retry map) and in the archived roadmap.

## Why the freeze

The maintainer decided on 2026-09-09 to concentrate development on Rarog,
which shares Basilisk's evaluation lineage, sits at the same strength in the
pool, and carries the joint roadmap toward the strongest HCE-era engines and
then NNUE. Basilisk remains a pool member, the C++ reference for Rarog's
classical evaluation work, and a source of measured priors (its accepted and
rejected experiments transfer as hypotheses, never as verdicts).

## Freeze record

Filled in at 15.1.c: release revision, bench fingerprint, asset hashes, pool
position, and the reopening rule.
