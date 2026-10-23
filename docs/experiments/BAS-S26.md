# BAS-S26

**BAS-S26 — B.10: the piece-square table orientation repair**

- **Registration.** Registered 2026-10-23, before any game; maintainer-run;
  correctness repair, admitted into the evaluation freeze (PLAN B.10).
- **Defect.** Seven piece-square tables (mg and eg knight, bishop and
  queen; eg king) have held PeSTO's rank-8-first rows on a1-first squares
  since v1.0.0 (`analysis/pst_orientation_2026-10-23.md`).
- **Candidate.** `basilisk-b10-flip-pext-pgo.exe`, revision `75381dc`: the
  seven tables rank-flipped and nothing else. The header reproduces the
  analysis's candidate byte for byte (sha256 `a535b968…e1ff`).
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest.
  - `bench 13` 8,437,148; SHA-256
    `AF8B1398A4722C4F0411199E09EBEB376809C6156E43BBEF8F9657AD5757BBAE`.
- **Baseline.** `basilisk-b10-base-pext-pgo.exe`, revision `2f958e0`, whose
  engine is the B.2.4 head's.
  - Rebuilt because MSYS2's clang moved from 22.1.8 to 23.1.3 after B.2.4,
    and the harness pairs only equal compilers. The rebuild reproduces
    `bench 13` 11,099,210 exactly.
  - SHA-256
    `41C7DA36636D083678310428EF80B95437BE3AF294AA0827803738B8D19E0F23`.
- **Mechanism.** Placement value on the squares PeSTO intended: advanced
  knight outposts, a queen kept off its own back rank in the endgame, an
  active endgame king.
- **Interactions.** `init_eval_tables` (`src/eval.cpp`) bakes the tables
  into `MG_TABLE`/`EG_TABLE`. Neighbours that may have absorbed the error:
  - outposts, `reachable_outpost`, mobility and `queen_infiltration`, all
    fitted around the mirrored tables;
  - the KBNK and KXK mate drives, which read the eg knight, bishop and
    king tables;
  - Cluster 1's eval-valued margins (RFP, futility, razoring), fitted on
    the old eval, which the flip moves by 31.7 cp mean.
- **Competing explanations for a neutral or negative reading:**
  - the neighbours above compensate, so fixing the tables double-counts;
  - the margins are mis-sized for the new eval shape until C.10.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** H1, **+3 to +10 Elo**; confidence about 70% (PLAN B.10,
  frozen 2026-10-23).
- **Most likely failure mode:** compensation by the neighbours, reading
  near zero.
- **Verdict rule:**
  - H1: the flip becomes the default, and the fingerprint record updates
    to 8,437,148;
  - H0: the old tables stay, and C.2.5 fits from the flipped values
    regardless;
  - the cap without a decision changes no default: report the LLR and
    return the leaf to the maintainer.
- **Correctness floors, read before the result is acted on:** the endgame
  cohort at 600k nodes keeps KQ-K, KR-K and KBB-K at 24/24 and KBN-K at
  23/24 or better, as the CTest conversion floors require.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 26001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs.** By the drift model the expected LLR reaches its
  bound inside the cap for a true value above about +5 nElo (roughly +3
  Elo, the low end of the prediction). BAS-S19 used the same cap on this
  bracket.
- Launcher `tools/run_b10_gate.ps1`; results in `tools/results/b10-gate/`.
- **Dry run accepted 2026-10-23**
  (`tools/results/colosseum_sprt_b10-gate_20261023_193230.*`, dry run
  `ee177d7e…58a0`, manifest `9bce8a30…f733`).

#### QUALIFICATION (before the gate)

- `tools/results/b10-qual/`, all on the candidate's source, the host idle:
  - `bench 13`: ON 8,437,148 (from 11,099,210); the legacy OFF arm
    15,887,213 (from 14,978,465), since the evaluation is shared;
  - `bench 10`: ON 2,219,596 in Release and under the sanitizer;
  - CTest: ON 23/23 Release, OFF 20/20 Release, ON 21/21 sanitizer;
  - the new orientation test fails exactly the seven mirrored tables on
    the old header and passes all twelve on the new one;
  - WAC at depth 6: ON 187 (from 181), OFF 141 (from 138); WAC at 100k
    nodes: ON 248 (from 241).
- **The endgame cohort at 600k nodes** (maintainer-run 2026-10-23,
  `tools/results/b10-qual/endgame-600k-flip.json`, `bc0354ee…5e736f`),
  paired position by position against the B.2.4 head's reading
  (`tools/results/b24-20261016/endgame-600k-fit.json`):
  - **The floors hold:** KQ-K, KR-K and KBB-K 24/24 and KBN-K 23/24, the
    same as the head.
  - Of the 480 clean wins, the flip converts 460 and the head 452. Paired,
    the flip gains 12 and loses 4 (sign test p ≈ 0.08), mostly 100-ply
    limits in KNN-KP, KRP-KR and KQ-KRP.
  - Win-preserving moves are 99.17% against 98.96%; 17 positions discard a
    win against 20.
  - The endgame tables now read in the intended direction, and nothing
    here argues against the gate.

#### RESULT — append after exposure

- Disposition: registered and dry-run; the endgame floors hold; the SPRT is
  maintainer-run.
