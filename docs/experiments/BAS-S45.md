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

#### AMENDMENT 1 (2026-11-04, maintainer; before runs 2-9 played)

- **The run list is run 9 alone** (`B3NmpEvalGate=1`, seed 45009). Runs
  1-8 are withdrawn.
  - The fit (B.3.3) was made with Basilisk's forms in place. A donor
    form read on this arm plays unfitted against a form fitted for it,
    so the read is tilted toward the kept form: it could confirm a donor
    form, but never clear one.
  - Clause b's five runs already default to the donor's form, and B.3.2
    read them near zero.
  - The eval gate's tilt runs the other way, because the fit was made
    with the donor's bare null move. A gate that wins here is trusted.
- **Run 1's partial games** (1,494 of 2,000,
  `tools/results/b34-cat-FailLowStore/`) are dropped, unread.
- **The kept forms** (`B3FailLowStore` 0, `CoreHistoryFills` 0,
  `B3ProbCutTt` 0, `B3ExtBudget` 1) stay while they work.
  - A fair comparison refits both arms. That is fine tuning for a few
    Elo, so it waits in PLAN H.2.
  - The exception is `B3FailLowStore`, which goes to B.4 (same day):
    B.4's TT write on exit must choose the quiescence fail-low store
    anyway, so B.4 settles both searches' forms before its fit.
  - The departures table records each form with that reason, and the
    eval gate's reading, kept or not.
- **Predictions:** Q1 and Q2 stand. Q3 and Q4 are withdrawn with their
  runs, unscored.
- The rule, the vetoes and the conditions are unchanged.

#### RESULT — append after exposure

- **Run 9, `B3NmpEvalGate=1`** (maintainer-run 2026-11-04,
  `tools/results/b34-cat-NmpEvalGate/`, seed 45009):
  - **−6.1 ± 9.7 Elo** (nElo −9.6 ± 15.2).
  - W/D/L 518/929/553, recounted from the PGN; pentanomial [49, 239, 447,
    228, 37].
  - 0 engine faults, 0 time losses, 0 infrastructure faults. The binary
    is `D47013B9…`; the repository was clean at launch.
  - `result.json` `085a0557…`, `games.pgn` `8a5f80b6…`.
- **The rule:** −6.1 is below +5, so the gate is not adopted. The default
  stays 0, the donor's bare null move, and there is nothing to price.
  - The interval [−15.8, +3.6] holds 0.
  - The read is 17.4 Elo below B.3.2's +11.3 ± 9.3, about 2.5 standard
    errors apart. Two things account for it:
    - The fit was made with the bare form, and it made the null move
      less aggressive on all four of its moving terms (B.3.3's finding
      1), doing part of the gate's work with the constants.
    - The tilt runs against the gate.
  - A refit with the gate on was not asked for, and the gate's B.3.2 gain
    does not survive the fit.
- **Predictions:**
  - **Q1 hit:** the gate read −6.1, below +11.3.
  - **Q2:** the gate did not adopt; Q2 gave it 35%.
  - Q3 and Q4 were withdrawn by amendment 1.
- **Departures table:** written to the packet
  (`analysis/b30_cluster2_research_2026-10-30.md`, *Departures table
  (B.3.4)*).
  - Four categorical forms are kept, with their waits: `B3FailLowStore`
    goes to B.4, and `CoreHistoryFills`, `B3ProbCutTt` and `B3ExtBudget`
    go to H.2.
  - Two seed values, the singular margin and `seekMate`'s depth, belong
    to the fit.
  - The gate's reading is recorded too.
- **Disposition: CLOSED.** No default moved. B.3.5's candidate is the
  fitted arm itself (engine of `f02a9e4`, 5,369,042), so BAS-S43 (a) and
  (c) play the same ON build as (b)'s fitted arm.

- **2026-11-03, an interrupted start.** The agent started the nine-run
  queue at 22:33, which was not its to start. The maintainer stopped it
  during run 1 (`B3FailLowStore=1`), which checkpointed at 1,494 of 2,000
  games with 0 faults.
  - Its directory `tools/results/b34-cat-FailLowStore/` resumes at its
    recorded seed with the same command, to the registered 2,000 games.
  - That is not an extension. Nothing was read from the partial run, and
    nothing is decided on it.
  - Runs 2-9 had not started.
