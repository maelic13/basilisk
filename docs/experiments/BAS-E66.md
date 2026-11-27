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
  - The played hash fb1ed67763bb0dac is identical through the
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

#### RUN NOTE — 2026-11-27, before the result

Written by research's review (the packet's Amendment 2), from the run
directory `tools/results/c36-e66-sprt` and its three wrapper manifests.

- **Two interruptions, one sequential test.**
  - The run started at 07:48:36 UTC from the agent session.
  - At 08:16:30 UTC it was killed from outside, with 2,772 games
    committed.
  - It resumed from its checkpoint at 08:17:12 UTC (`run.log`'s
    `resumed` event). The games in flight were dropped and played again
    from the same seed.
  - The maintainer paused it at about 08:22 UTC for the review, at 1,649
    pairs with 0 faults, and resumed it at 08:43:58 UTC from the
    checkpoint at 3,306 games. It ran to its verdict at 09:02 UTC.
  The committed sample, the seed, the bounds and the cap are unchanged,
  so a resume from the checkpoint continues the registered test. It is
  not an extension or a restart.
- **The host was not empty for all of the last segment.** The
  registration says nothing else runs while games play. The review did
  not know the run had been resumed. Between about 08:53 and 09:00 UTC
  it ran about a minute of single-threaded Python (its phase and class
  tables, the document checks) and a few git commands.
  - Both engines of a game share one core pair, so a disturbance falls
    on either arm alike.
  - The run ended with no time loss and no fault. The arms read 1.95M
    and 1.96M nodes a second, at 69.7 ms a move each.
- **The `EngineCrash` record is the kill, not an engine fault.**
  `failed-games/` holds one record: the candidate, round 2789, "engine
  terminated unexpectedly (exit code: 1)", mid-search.
  - Exit code 1 in a search is an outside termination. Basilisk exits 1
    only at start-up or through `main`'s catch, which prints
    `info string FATAL` first, and the record's transcript has no such
    line. An abort or an access violation exits otherwise.
  - The record is stamped 3 seconds after the segment's last progress
    line, and the segment wrote nothing more. Its wrapper manifest has
    no closing line, which a soft stop writes.
  - The fault counters read 0-0 before and after.
  - The record's position, replayed on the candidate binary at depth 20
    three times, ends normally each time.
  The maintainer confirmed it on 2026-11-27: the launching session was
  asked to stop the run and did, and the maintainer then resumed it.

#### RESULT — append after exposure

Recorded 2026-11-27. Run directory `tools/results/c36-e66-sprt`, last
wrapper manifest
`tools/results/colosseum_sprt_c36-e66-sprt_20261127_094357.manifest.txt`
(exit 0, H1 accepted; PGN SHA-256
`C91AF237DFC45224B769F3202232AEB3AF4C150CB0DB375F039890A3EEF01783`).

- **SPRT `[0,3]`: H1 at 5,162 games (2,581 pairs), LLR +2.97** against
  the bound +2.944, inside the cap of 10,000 pairs.
  - W-D-L 1,500-2,374-1,288; pentanomial [44, 556, 1198, 710, 73]; the
    wrapper's independent PGN recount agrees.
  - **+14.3 Elo [+8.8, +19.8]** (95%, pentanomial), nElo +24.7 ± 9.5.
    This is a **first-crossing estimate**: it was read at the boundary,
    so it is not the effect size.
  - 0 engine faults, 0 time losses. Terminations: 2,788 mates, 1,935
    threefold, 374 insufficient material, 53 fifty-move, 12 stalemate.
  - Both arms 69.7 ms a move; 1.96M against 1.95M nodes a second; mean
    non-mate depth 16.57 against 16.56.
- **The C.2.7 endgame bracket** (`tools/results/c36-e66-veto/`, both
  arms' `bracket.json` and `veto.json`; run 2026-11-27 on an idle host
  by `tools/run_c36_reads.py --leg veto`). Converted of 480 won Cohort
  positions at 21,000 / 64,000 / 190,000 nodes:
  - candidate **443 / 448 / 457**, base **441 / 448 / 457**;
  - by family where the arms differ (base → candidate, the three
    budgets): KR-KP 23/24/24 → 24/24/24; KQ-KR 22/23/24 → 24/23/24;
    KNN-KP 4/5/7 → 4/4/7; KRP-KR 22/22/23 → 20/23/24; KRP-KB 23/24/23 →
    23/23/24; KBP-KB 22/23/23 → 23/23/24; KBP-KN 22/22/24 → 23/22/23;
    KP-KP 22/23/24 → 23/23/23; KQ-KRP 23/23/24 → 22/23/23; KBPP-KB
    22/21/22 → 21/22/22. Every other family is equal.
  - **No hard failure:** `new_rule50_failure`, `engine_anomaly` and
    `malformed_record` are empty, and no rule-50 failure is provisional.
  - **The two-of-three rule fires once** (the tool's verdict BLOCK, exit
    2): a new clean-win discard at KR-KN/EG0350, at 21,000 and 64,000.
  - Fifteen discards at one budget only (provisional): KBP-KN/EG0613
    (190k), KBP-KN/EG0623 (64k), KBPP-KB/EG0753 (21k), KNN-KP/EG0452
    (21k), KNN-KP/EG0457 (190k), KNN-KP/EG0469 (190k), KP-KP/EG0663
    (190k), KQ-KR/EG0417 (64k), KQ-KRP/EG0705 (190k), KR-KN/EG0336
    (21k), KR-KN/EG0347 (64k), KR-KN/EG0349 (190k), KRP-KR/EG0493 (21k),
    KRP-KR/EG0496 (21k), KRP-KR/EG0505 (21k).
- **Decision, by the registered rule:** H1 with no hard failure, so R
  is kept, and corpus C becomes the base. The veto does not hold it (the
  maintainer's ruling for this run). KR-KN/EG0350 goes to C.4 as a vetoed
  position, and the fifteen single-budget discards go with it as
  observations. R stays baked as `1f07c8e` baked it: `bench 13`
  3,525,148.

**Calibration against the frozen prediction.**
- The prior, +4 Elo with an 80% band of −4 to +12: the estimate,
  +14.3, lies above the band. It is a first-crossing estimate, and an
  early stop selects a large one, so this is weak evidence that the
  prior was low; it was not tested. The static read did not foresee
  it either: R moved the validation loss by 0.13%.
- H1 inside the cap, 0.5: happened, at 26% of the cap.
- The bracket's two-of-three rule fires against the base, 0.7: it fired,
  once. Research's review expected it to fire on the pawn-only and
  one-minor classes, where R's loss is worse. The pawn-only family
  converted as often (KP-KP 69 → 69, summed over the three budgets),
  and the one veto is in KR-KN.
- A hard failure, 0.05: none.
