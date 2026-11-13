# BAS-S60

**BAS-S60 — B.6.6: the two adopted donor forms against the head, SPRT `[0,3]`**

- **Registration.** Created 2026-11-13, before any game and before any
  census of this pair was read; maintainer-run games, agent-run census;
  search reconciliation, Elo layer. Owed by PLAN B.6.6 after BAS-S59 adopted
  two rows: the adoptions land together behind one switch and pass SPRT
  `[0,3]` against the head before B.6.7.
- **Baseline / candidate.** One clean revision, `65448d0`; the umbrella
  `B6_DONOR_FORMS` (`CMakeLists.txt`, `BASILISK_B6_CORE_DEFAULTS`) sets
  `CoreRazorDonor` and `CoreCorrNonPawnKings` to 1. Final-PGO PEXT release
  builds by `tools/build_test.ps1 -Flavor Release -ArmOption B6_DONOR_FORMS`,
  Clang 23.1.5, manifests clean:
  - ON `tools/test_engines/basilisk-b66s-on-pext-pgo.exe`, SHA-256
    `A2BF3537F6EBECFF761765AF9A10E73502E4E5DB528DA1982993318BE9B77F98`,
    `bench 13` **3,268,437**;
  - OFF `tools/test_engines/basilisk-b66s-off-pext-pgo.exe`, SHA-256
    `07BB73F32CBB7D6942C09A0CA93B1CD7646069BF40BD1AD6FD3560B6EA9FED79`,
    `bench 13` **3,783,135**, the development fingerprint.
- **Research question.** Do the two donor forms BAS-S59 adopted one at a
  time hold their gain together, against the head, on release builds?
- **Hypothesis / mechanism.** Packet §4.2 (razoring at every depth on a
  quadratic margin, now that quiescence is the donor's) and §4.4 (the
  non-pawn correction keyed with the kings, so bare kings no longer pool).
- **Interactions.** The corrected eval the kings key changes is the
  estimate razoring reads, so the two are not independent: the combined
  arm's `bench 13` (3,268,437) is not either arm's (3,422,264 razoring,
  3,019,185 kings).
- **Competing explanations.** Selection: each adoption cleared its rule
  just past 0, and with five runs on one rule a null row clears it about
  one time in nine. The SPRT is the guard.

#### CONDITIONS

- **Shape.** SPRT `[0,3]` nElo (`sprt-default.toml`, α = β = 0.05) at
  `3+0.03`, 1T, Hash 64, paired reversed UHO, no adjudication, tablebases
  off, Colosseum. Seed 66010. Arm A ON, arm B OFF. Runner
  `tools/run_b66_sprt.py`; results in `tools/results/b66-sprt`.
- **Cap 5,000 pairs.** σ ≈ 245 nElo a pair (BAS-S59: ±15.2 nElo at 1,000
  pairs on every run). The expectation is shrunk for selection: +10 Elo,
  about +16 nElo, against the two reads' +17.1 and +19.2 nElo. There the
  expected stop is 2.94 · 245² / (3 · 14.5) ≈ 4,060 pairs, and the cap is
  about 1.25× that. A smaller effect ends at the cap. At most about 1 h 40
  min (BAS-S59's 6,000 games an hour).
- **H1:** the two forms become the defaults. Before the flip commit, and
  each in its own commit with its measurement (`agents/implementation.md`):
  - the three ON-arm tests that bind the OFF tree are re-based (packet §9):
    the forgotten-mate fixture of `test_b5_root`, the depth-6 WAC floor,
    KQ-K's staleness guard;
  - the census condition below passes.
  The fingerprint record moves to 3,268,437 and `CHANGELOG.md` gains the
  entry. B.6.6 then closes; B.6.7's surface drops `CoreRazorMargin` and
  `CoreRazorDepthCap` and reads `CoreRazorQuadMargin`.
- **H0 or the cap:** neither form is adopted; an unresolved stop is not an
  adoption. Both rows wait in H.2 with BAS-S59's readings and this one, and
  B.6.7 fits the head's forms.
- **The census condition, on H1 only** (`tools/diag/pgn_time_census.py` on
  the SPRT's PGN, both sides): the flip is held when the ON arm's games
  decided for it at 300 and not won exceed the OFF arm's by more than the
  larger of 6 and 3·√(the OFF arm's count), B.5.5's L1 rule. It reads the
  one cost a quick gate can hide here: both forms delay mate proofs at
  fixed depth (packet §9). A hold goes to the maintainer with the counts.
- No arm, bound, cap, book or adjudication changes after a game is
  observed.

#### PRE-REGISTERED PREDICTION — freeze before exposure

| # | Prediction | Probability |
|---|---|---|
| G1 | H1 | 60% |
| G2 | the cap without a decision | 25% |
| G3 | H0 | 15% |
| G4 | the estimate at the stop (first-crossing) is **+10 Elo**, 80% interval [+3, +18] | low-moderate |
| G5 | the census condition passes | 85% |

- **Falsification:** H0, or an estimate under +3, says one or both
  adoptions were selection, or that the two do not add.

#### RESULT — append after exposure
