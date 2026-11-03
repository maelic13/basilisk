# BAS-S45

**BAS-S45 — B.3.4: Basilisk's forms re-read on the fitted arm**

- **Registration.** Created 2026-11-03 at B.3.4's opening, before any
  game. Maintainer-run games, agent-run pricing; search cluster, Elo
  layer, with the binding set as vetoes.
  Assembled from PLAN B.3.4 (its 2026-11-01 audit clause and the
  2026-11-02 eval-gate clause) and B.3.3's fitted-constant analysis
  (`analysis/b33_constants_2026-11-03.md`, *For B.3.4*).
- **Subject.** The fitted ON arm, engine of `f02a9e4` (B.3.3's bake),
  `bench 13` **5,369,042**. Every run plays one ON Tune final-PGO build of
  that engine: arm A is the alternative by UCI option, arm B the defaults.
  The build's identity and hash are recorded below at build time.
- **Research question.** With Cluster 2 and the admitted Cluster 1
  constants fitted, does any categorical that B.3.2 read on the unfitted
  arm now read differently? This covers the donor's forms kept against
  Basilisk's and the forms whose mechanisms the fit moved.
- **Hypothesis.** A donor form B.3.2 read near zero can have lost only
  for want of its fit (PLAN B.3.4). The fit moved the null move, the
  singular search, IIR and ProbCut, so their categoricals are re-read.
- **Competing explanation.** The fit was mostly flat (one full-step mover
  in 29; analysis finding 4), so the re-reads repeat B.3.2's within
  noise and no default moves.

#### RUN LIST (frozen before the first game)

PLAN B.3.4's three clauses applied to B.3.2's readings (BAS-S41
*RESULT*) and B.3.3's moved mechanisms:

| # | Run (arm A) | Clause | B.3.2 read | Seed |
|---|---|---|---|---|
| 1 | `B3FailLowStore=1` | a: the donor's form, default Basilisk's | +3.3 ± 9.1 | 45001 |
| 2 | `CoreHistoryFills=1` | a | −0.5 ± 9.3 | 45002 |
| 3 | `B3ProbCutTt=1` | a (ProbCut also moved, clause b) | +0.9 ± 9.5 | 45003 |
| 4 | `B3NmpCutOnly=0` | b: inside ±5, the null move moved | −1.4 ± 9.2 | 45004 |
| 5 | `B3MultiCut=1` | b: the singular search moved | +3.3 ± 9.6 | 45005 |
| 6 | `B3SingNegative=1` | b: the singular search moved | 0.0 ± 9.2 | 45006 |
| 7 | `B3SingTtDepth=1` | b: the singular search moved | −1.0 ± 9.2 | 45007 |
| 8 | `B3IirStale=1` | b: IIR moved (`B3IirMinDepth` 6 → 5) | +2.8 ± 9.2 | 45008 |
| 9 | `B3NmpEvalGate=1` | the 2026-11-02 clause: re-read on the fitted arm | +11.3 ± 9.3 | 45009 |

Not re-read, with the reason:

- `B3NmpVerify=0` read −5.0 ± 8.8, which is at ±5, not inside it. B.3.2
  kept the default because its interval held 0.
- `B3PvQuietPrune=0` (+5.9), `B3CheckExt=1` (−22.4), the legacy check
  policy (−30.8) and `B3Iir=0` (−11.3) read outside ±5, so they keep
  B.3.2's verdict.
- `B3Cuckoo=0` (0.0): the upcoming-repetition check gates no fitted
  coordinate and no fitted coordinate gates it, so B.3.2's verdict stands
  (the audit's rule).
- `B3ExtBudget` (Basilisk's per-line budget; the donor has none) was
  settled without games in B.3.2 (amendment 2). It is not in PLAN
  B.3.4's list, so it goes to the departures table as kept, unread.
- Items 8 and 11 (`B3ProbCutPv`, `B3CorrExcluded`) had no B.3.2 run
  (§5.12's 10% rule), so there is nothing to re-read.

#### CONDITIONS

- **Every run:** Colosseum `match-fixed`, 2,000 games (1,000 paired
  reversed UHO openings), `3+0.03`, 1T, Hash 64, no adjudication,
  tablebases off, the PGN recounted. The runner is
  `tools/run_b34_games.ps1`, with every binary pinned by SHA-256 and each
  launch dry-run first.
- **Rule** (PLAN B.3.4, as amended by the audit): arm A is adopted at
  ≥ +5 Elo **and** a 95% interval that excludes 0; otherwise the default
  stays. A run never stops early and is never extended.
- **Vetoes on an adoption:** the conversion floors at the five budgets,
  the WAC floor, and release CTest, the promotion fixture's ON clause and
  `test_b3_proof`'s liveness sections included. They are read on a build
  with the adopted default (B.3.2's `adopt_price.py` recipe, copied to
  `tools/results/b34-20261103/price.py`).
- **Several adoptions** land together as B.3.5's candidate and are
  priced together. A veto of the combination prices each adoption alone,
  and only the vetoed ones drop.
- **Departures table:** at the close, every place the arm keeps
  Basilisk's form where the donor's differs is written to the packet
  (`analysis/b30_cluster2_research_2026-10-30.md`) for B.6.1. Each row
  gives Basilisk's form, the donor's, the reading that kept it, and the
  feature or fit it waits on. The eval gate's reading goes there kept or
  not.
- **No refit on adoption.** The fitted-constant analysis asked for none
  (its finding 1).

#### PRE-REGISTERED PREDICTION — freeze before exposure

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| Q1 | The eval gate (run 9) reads below B.3.2's +11.3: the fit already made the bare null move less aggressive on all four of its moving terms, which is part of what the gate does | 70% | run 9 ≥ +11.3 |
| Q2 | Run 9 adopts the gate (≥ +5, interval excluding 0) | 35% | — (scored either way) |
| Q3 | Of runs 1-8, none adopts | 70% | any of 1-8 adopts |
| Q4 | Every one of runs 1-8 reads within ±8 of its B.3.2 reading | 75% | a run more than 8 Elo from its B.3.2 read |

#### RESULT — append after exposure

- **2026-11-03, an interrupted start.** The agent started the nine-run
  queue at 22:33, which was not its to start. The maintainer stopped it
  during run 1 (`B3FailLowStore=1`), which checkpointed at 1,494 of 2,000
  games with 0 faults.
  - Its directory `tools/results/b34-cat-FailLowStore/` resumes at its
    recorded seed with the same command, to the registered 2,000 games.
  - That is not an extension. Nothing was read from the partial run, and
    nothing is decided on it.
  - Runs 2-9 had not started.
