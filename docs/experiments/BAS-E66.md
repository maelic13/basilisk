# BAS-E66

**BAS-E66 — C.5.2's first cycle in C.3.6: the refit R against the shipped evaluation, SPRT `[0,3]` and the endgame bracket veto**

- Date / owner / category: registered 2026-11-26, before any game;
  agent-run overnight with the maintainer's leave; data/tuning.
- Base: the shipped evaluation, `bench 13` 3,880,333. Candidate: BAS-O19's
  R, baked and re-levelled, with its `bench 13` recorded here before the
  first game. Clean PGO PEXT builds of one commit, with the same compiler
  for both.
- Research question: does refitting on the current engine's games gain
  strength?
- Pre-game checks: no knob on a bound; played-hash identity through the
  bake; CTest green, with the value-bound files updated;
  `test_endgames --conversion` 24/24; the C-98 lazy audit; piece values
  within ±15% of the base in every phase band. A failure stops this
  line before any game.
- Conditions: BAS-E65's. Colosseum CLI `sprt`, `[0, 3]` nElo, `3+0.03`,
  margin 20 ms, 1T, Hash 64, `UHO_Lichess_4852_v1.epd` in random order,
  paired and colour-reversed, no adjudication, no tablebases, fault cap
  0, its own seed. **Cap 10,000 pairs.** Nothing else runs on the host
  while it plays.
- Then the C.2.7 endgame bracket veto at 21k, 64k and 190k nodes
  against the base.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- Research's prior: +4 Elo, 80% band −4 to +12. Rarog's first cycle read
  +11.0, and BAS-E64 shows that a first fit can lose 10.
- H1 inside the cap: 0.45. H0: 0.25. The cap is reached unresolved: 0.3.
- The bracket's two-of-three rule fires against the base: 0.7. A hard
  failure (`new_rule50_failure`, `engine_anomaly`, `malformed_record`):
  0.05.

#### Registered rule

- **The veto, the maintainer's ruling for this run only** (2026-11-26):
  "H1 alone is enough whatever the check says, because we have the C.4
  in plan. It should not be general rule, just rule for this C.3 work."
  Clean-win discards and conversion counts neither block nor hold. The
  bracket is recorded in full and its vetoed positions go to C.4.
- H1 with no hard failure: R is kept and baked, and corpus C becomes the
  base.
- H1 with a hard failure: held for the maintainer, not baked, and the
  base stays the shipped vector on corpus A.
- H0: not baked; the cycle stops (C.5.2's rule). The cap: not baked,
  recorded as unresolved, retried in C.5.2.
- The estimate at a stop is a first-crossing estimate. Never extended or
  re-run on the same pair.

#### RESULT — append after exposure
