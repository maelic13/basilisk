# BAS-S62

**BAS-S62 — B.6's gate: the B.6.7 bake against the head before it, SPRT `[0,3]`**

- **Registration.** Created 2026-11-14, before any game; maintainer-run games;
  parameter fit, Elo layer. Owed by PLAN B.6 ("registered surface; PGO bake; SPRT
  `[0,3]`") and B.6.7's bake record. The tune and its analysis are BAS-S61 and
  `analysis/b67_constants_2026-11-14.md`.
- **Baseline / candidate.** Two revisions, the bake alone between them:
  - candidate `6a9f81f` (the bake `25c570f` and its records):
    `tools/test_engines/basilisk-b67s-on-pext-pgo.exe`, SHA-256
    `F43338F99B3693BACC9A18F7F2BE97C21BCF9C132964791FE5618151294DD1E7`,
    `bench 13` **4,278,307**;
  - baseline `0f30bf1`, the commit before the bake:
    `tools/test_engines/basilisk-b67s-off-pext-pgo.exe`, SHA-256
    `8C93E4F1DFDB687E7F0CA39FE9FF5E96169427BBC3C87818FD2FBDF82BB4D3B6`,
    `bench 13` **3,783,135**, the accepted fingerprint.
  Both final-PGO PEXT release builds by `tools/build_test.ps1 -Flavor Release`,
  Clang 23.1.5, manifests clean (the baseline built in a temporary worktree at its
  revision). The wrapper's single `-ExpectRevision` cannot name two revisions; the
  runner checks each manifest's revision, cleanliness and bench itself, and pins
  both binaries by SHA-256.
- **Research question.** Does the joint fit's theta, baked, play stronger than the
  head it was fitted on?
- **Hypothesis.** BAS-S61's theta: 154 coordinates moved, 8 by a full step; the
  tune's own gain read is +22.9 Elo (null SD 11.6), taken by iteration 1,000.
- **Competing explanations.** The gain read is the tune's own and noisy; most of
  the movement is sub-step noise (the analysis's finding 6); the bake grows the
  `bench 13` tree by 13.1%, a cost at equal time if the extra plies buy nothing.

#### CONDITIONS

- **Shape.** SPRT `[0,3]` nElo (`sprt-default.toml`, α = β = 0.05) at `3+0.03`, 1T,
  Hash 64, paired reversed UHO, no adjudication, tablebases off, Colosseum. Seed
  67010. Arm A the candidate, arm B the baseline. Runner `tools/run_b67_sprt.py`;
  results in `tools/results/b67-sprt`.
- **Cap 6,400 pairs.** σ ≈ 245 nElo a pair (BAS-S59, BAS-S60). The expectation is
  shrunk for selection, as BAS-S60 taught (two single reads of +10.6 and +11.6
  read +2.2 together): **+8 Elo, about +13 nElo**, against the tune's +22.9. There
  the expected stop is 2.94 · 245² / (3 · 11.5) ≈ 5,100 pairs, and the cap is about
  1.25× that. A smaller effect ends at the cap. About 2 h 10 min at BAS-S60's pace.
- **H1:** the bake is accepted. The fingerprint record moves to 4,278,307 (GUIDE,
  `AGENTS.md`, DESIGN), `CHANGELOG.md` gains the entry with this result, and B.6.7
  and B.6 close.
- **H0:** the bake is reverted on the branch (`25c570f` and the generator commit
  that reads it; the screen fix `0f30bf1` stays). The analysis's findings keep
  their owners.
- **The cap without a decision:** the maintainer decides between the bake and the
  head with the estimate in hand; an unresolved stop is not an adoption, and none
  is assumed here.
- No arm, bound, cap, book or adjudication changes after a game is observed.

#### PRE-REGISTERED PREDICTION — freeze before exposure

| # | Prediction | Probability |
|---|---|---|
| G1 | H1 | 50% |
| G2 | the cap without a decision | 30% |
| G3 | H0 | 20% |
| G4 | the estimate at the stop is **+7 Elo**, 80% interval [+1, +14] | low-moderate |

- **Falsification:** H0, or an estimate under +2, says the tune's gain read was
  mostly its own noise and selection.

#### RESULT — append after exposure
