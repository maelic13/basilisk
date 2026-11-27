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
- H1 inside the cap: 0.5. H0: 0.15. The cap is reached unresolved: 0.35.
  (Clerical correction, 2026-11-26, before any game: first written as
  0.45 / 0.25 / 0.3, which did not follow from the prior above and the
  drift model at 10,000 pairs; research's arithmetic.)
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
- H1 with a hard failure (`new_rule50_failure`, `engine_anomaly`,
  `malformed_record`): not baked; it returns to research as an R3
  return, and the cause decides what is redone, by the packet's
  Amendment 1 (the maintainer's ruling of 2026-11-26: "rather than held
  for me, passed as R3 to you … If you cannot fix it, then hold for
  me."). Work that depends on the base waits. If research cannot fix
  it: held for the maintainer, and the base stays the shipped vector on
  corpus A.
- H0: not baked; the cycle stops (C.5.2's rule). The cap: not baked,
  recorded as unresolved, retried in C.5.2.
- The estimate at a stop is a first-crossing estimate. Never extended or
  re-run on the same pair.

#### PRE-GAME RECORD — 2026-11-27, before the first game

- **Candidate** `Basilisk refit C`: `tools/test_engines/basilisk-c36-e66-cand-pext-pgo.exe`.
  - Built at `1f07c8e`, with R baked and re-levelled.
  - SHA-256 `F7953286EB4C12A2029FD81BB939087626035995FDC039439BAACD2F9E493F28`.
  - **`bench 13` = 3,525,148.**
- **Base** `Basilisk shipped`: `tools/test_engines/basilisk-c36-e66-base-pext-pgo.exe`.
  - Built at `22ec9c5`, with the shipped vector.
  - SHA-256 `3017C49A44B5CB7722387187422069BBD76BCDACCD5029A3E60B076C6C63C05B`.
  - `bench 13` = 3,880,333.
- Both arms are clean final-PGO PEXT builds, with Clang 23.1.5 for both.
  The two commits differ only in `src/eval_rb_params.h` and the fixture
  golden file.
- **Pre-game checks**, all passed:
  - No fitted knob on a bound. No king-safety slot was fitted (R8), and
    KdBlockers is at 0, its shipped value.
  - The played hash `fb1ed67763bb0dac` is identical through the
    re-level and the bake, and `--verify` is exact at the bake.
  - CTest is 59/59. The one value-bound file,
    `tests/data/endgame_verdicts_golden_rebuild.txt`, was regenerated:
    KRvKR wrong went from 297 to 294, and one KRvKB position moved from
    the scale-4 site to the estimate.
  - `test_endgames --conversion` is 24/24 in all four families.
  - The C-98 lazy audit (`eval_hash --lazy-audit`, a Diag build) leaves
    every served score unchanged:

    | | fires | sign flips | margin crossings | max \|Δ\| |
    |---|---|---|---|---|
    | C's validation | 36,257 | 3 | 702 (1.94%) | 1,889 |
    | A's validation | 37,853 | 4 | 800 (2.11%) | 2,332 |

    At C.3.4, the shipped vector on A's validation crossed the margin
    at 2.03%.
  - Marginal piece values are within ±3% of the base in every phase
    band (BAS-O19).

#### RESULT — append after exposure
