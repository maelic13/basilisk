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

## The 2026 development line (Phases 5–6)

Between 1.9.3 (2026-08-01) and 1.10.0 the `dev` branch accumulated the endgame
and hand-crafted-evaluation work below. What it established, each with its
ledger evidence:

| Result | Evidence |
|---|---|
| The KBNK mate drive completed and provably isolated; conversion measured on fixed-seed cohorts with deterministic CTest floors | BAS-E42 and the 6.1 record |
| Endgame Group A gated and accepted; the accepted HCE line about +12 Elo over 1.9.3 | 6.2 record, BAS-E4x |
| Rook-ending draw scaling (KRPKR, KRPPKRP) with a floored discount, accepted at +3.29 ± 4.61 Elo over frozen Group A; bench 12,568,898 | BAS-E54 |
| Passed-pawn king approach studied and dispositioned | 6.3 record |
| Magnitude and coverage audit of the endgame evaluator; reopened work recorded with its reasons | 6.4 record, "Reopened work, 2026-09-03" |
| The TT publication redesign: atomic whole-record word accepted for correctness, the coherence repair reverted on measured throughput with the risk recorded | BAS-X2x rows, commits `2bf43fb`, `c378706` |
| A peer audit of the shared board lineage found three SEE defects of the same kernel shape; Basilisk had no fixtures for them | BAS-X22 |
| Pool position on 2026-09-04 at `3+0.03` 1T: Houdini 1.5a −197, Critter 1.6a −187, Fritz 16 −178, Rybka 4 −84 | BAS-X11 |

## Phase 15 (2026-09-09 → 2026-09-10)

Board correctness, then the 1.10.0 release. Summarised in `PLAN.md`; the
measured detail is in `EXPERIMENTS.md` (BAS-C08, BAS-C09, BAS-E55, BAS-E56,
BAS-E57).

| Result | Evidence |
|---|---|
| SEE king legality repaired: the exchange now ends before an illegal king recapture, reading the unfiltered attacker set | BAS-C08, 6,481–0 and 301–0 against an independent legality oracle |
| Created pins and promotion recaptures kept as documented approximations | BAS-C09, 0 verdict changes in 339,607 production calls; repair costs +16.8% of the SEE column |
| Malformed UCI input hardened; unknown `setoption` names now diagnosed | 15.0.d, 17 test sections |
| Deterministic qualification clean | 15.0.e: CTest 12/12 release and sanitizer, 6/6 perft exact, invariants across four seeds |
| 1.10.0 accepted against 1.9.3 at +19.18 ± 6.76 Elo, 1T `3+0.03` | BAS-E55 |

## Open work

Not continued during Phase 15, and the natural place to resume: 6.6 instrument
and gate integrity, 6.7–6.11 remaining endgame families and closure, Phase 7
board correctness beyond Phase 15's bounded repairs, Phase 8 corpus and
complete HCE refit, Phase 9 classical search consolidation, Phases 10–14 NNUE
and platforms. Their evidence, retry triggers and dispositions remain in
`EXPERIMENTS.md` (section 9 is the retry map) and in the archived roadmap.

## Release record — 1.10.0

| Item | Value |
|---|---|
| Version | **1.10.0** (2026-09-10) |
| Bench-13 fingerprint | **14,978,465** |
| Previous release | 1.9.3, bench 11,941,440 |
| Strength | **+19.18 ± 6.76 Elo** over 1.9.3, `3+0.03` 1T, H1 accepted at 4,224 games, LOS 100% (BAS-E55) |
| 4T smoke gate | Clean: zero crashes, zero time forfeits, 95% lower bound −1.17 Elo |
| Qualification | CTest 12/12 release and 12/12 under ASan/UBSan; perft exact on all six standard positions; `test_invariants` 18/18 across four seeds |
| Pool position, `3+0.03` 1T (2026-09-04) | Houdini 1.5a −197, Critter 1.6a −187, Fritz 16 −178, Rybka 4 −84 |
| Accepted risks carried | TT publication coherence (BAS-C05); SEE created-pin and promotion-recapture approximations (BAS-C09) |

The release revision is the `Version 1.10.0` commit on `master`, tagged
**`v1.10.0`** — the durable identifier, since the squash SHA is not stable
across a re-merge. Published binaries are the nine assets built by
`release.yml` and attached to that release; per-asset hashes are not recorded
here, following the 8.6.5 local-only-manifest decision that keeps per-asset
data out of the repository (see `docs/release_tiers.md`). The locally built
reference asset used for the tier smoke tests was
`basilisk-v1.10.0-windows-x86_64-pext-pgo.exe`, sha256
`542247ca44a90d74abc793eb6cb121171e2b1224a78310c0cbb4e9915b522913`; the
published assets are CI-built and will differ.
