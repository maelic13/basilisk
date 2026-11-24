# BAS-E58

**BAS-E58 — C.0 static screens: the head's evaluation against the two donor candidates on its own games**

- Date / owner / calibration category: registered 2026-11-18, before any
  result; agent-run, agent-read; diagnostic.
- Source revision: the 1.11.0 head, `bench 13` 4,777,412; the oracle
  `oracle-b111-4d2742f.exe` (`8ADEE9AE…`) for `9587eeeb`'s evaluation and
  `stockfish-16-x64-bmi2-classical-pgo.exe` for `sf_16`'s.
- Research question: is the same-search evaluation deficit visible to a
  WDL loss on quiet game positions, and which donor families carry outcome
  information Basilisk's whole score lacks?
- Registration, predictions P1 to P7, falsifiers and the meaning of each
  outcome: `analysis/c0_evaluation_programme_2026-11-18.md` §4.1, frozen
  in the commit that adds this file.
- Evidence layer: fit loss and regression on game results. It ranks and
  explains; it accepts nothing (BAS-X02).

#### RESULT — append after exposure

- Run 2026-11-18 on 254,486 positions from 25,950 games
  (`tools/results/c0-screen/`, `dataset.csv` `cfee7385…`). The registered
  single K was set by Basilisk's known-win override rows, so the result is
  read on the 243,108 regular rows with each evaluator's own K; the packet's
  §5.2 records the change and why.
- Basilisk's evaluation trails `9587eeeb`'s by 0.68% of cross-entropy
  (36.0e-4 nats, 95% interval 28.2 to 44.5) and `sf_16`'s by 0.45%.
  `9587eeeb`'s leads `sf_16`'s by 12.3e-4 [9.5, 15.1].
- By phase the two are equal in the middlegame and the donor leads by 1.5
  to 3.4% at both ends; the ensemble beats either alone everywhere.
- Donor families that add outcome information to Basilisk's whole score:
  space and mobility on the full board, king safety in the middlegame,
  winnable in endings, pawn structure throughout. Passers, threats, rooks
  and queens add nothing.
- A travel-bounded refit of the existing terms improves held-out Brier by
  2.58% (0.055492 to 0.054062) and closes 56% of the static gap to the
  donor.
- Disposition: COMPLETE, an observation. It ranks and explains; C.2.8's
  gate and the family gates decide.

#### PREDICTION CALIBRATION — preserve the original prediction above

- Sixteen predictions, eleven held; the table and what the model had wrong
  are in the packet's §5.3.
- The misses that matter: the deficit *is* visible to a static loss on
  quiet game positions (P1a, P1b); the values are not at their static
  optimum (P6a, 2.58% where under 0.5% was given 0.70); the donor's
  information sits in space, pawns and mobility, not in passers and threats
  (P3h).
- Instrument lesson: a logistic K fitted over rows that carry override
  scores measures the overrides. Separate them before any loss is read.
- Conditional lesson: BAS-E04's wash measured the old search's positions
  and the mirrored tables, not this surface. Its retry trigger fired.
- Objective retry trigger: re-read after each evaluation cluster (Rarog
  audit C-84), on that head's own games.
