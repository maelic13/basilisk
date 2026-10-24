# BAS-S23

**BAS-S23 — B.2.4: Cluster 1 gates**

- Date / owner / calibration category: registered 2026-10-11 (Amendment 3,
  packet §22.7), before any game; maintainer-run; playing-strength gate.
- Baseline source revision / candidate source revision / dirty-diff identity:
  (a) the unfitted ON arm against the umbrella-off arm at the same revision,
  once its PGO binaries exist; (b) the fitted ON arm (BAS-S22's baked theta)
  against the unfitted ON arm.
- Binary / compiler / flags / PGO identity and hashes: recorded at run time;
  PGO pairs per arm.
- Research question: does Cluster 1 gain Elo on its own evidence, and does
  the fit add Elo on its own evidence, so that a weak tune cannot ride on the
  cluster's margin?
- Hypothesis / proposed mechanism: packet §13 and P2 (BAS-S21).
- Interacting mechanisms / consumers: the default flips to the accepted arm
  after (a) and (b); the legacy search stays compilable until B.8.
- Credible competing hypotheses: (a) passes and (b) fails (the fit is noise);
  (a) fails after B.2.2's screens held (the fixed-node layer does not carry
  Elo, as BAS-S17 showed for razoring alone).

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected diagnostic movement: none new; the screens are B.2.2's.
- Expected Elo sign/range: (a) + (b) together +40 at STC, 90% interval
  [+5, +90] (P2); each gate SPRT `[0,10]`.
- Probability candidate is positive/useful: 65% (P2).
- Confidence and basis: moderate.
- Most likely failure mode: (b) H0.
- Falsification criteria: H0 at `[0,10]` on either gate; a stop under +10
  refutes the coherent-core share (packet §11's stop rule).
- Cheapest prior falsifier: BAS-S21's paired run.
- Registered gate / stop rule: SPRT `[0,10]` at `3+0.03`, each gate accepting
  one thing on its own evidence (PLAN B.2.4).
- Full conditions / provenance: as BAS-S21; the paired-opening SPRT recipe of
  PROCESS; P1–P6 calibration appended here, as packet §11 says.

#### RUN DETAILS — registered 2026-10-16, before either gate's first game

- **Exposure since the prediction froze (2026-10-11).** Three readings of
  neighbouring questions have been seen; the prediction above stays as
  written and is calibrated as written:
  - B.2.2's paired run, unfitted ON against OFF at `84da91f`, +130.9 ±
    11.4 (BAS-S21);
  - SPSA block 1's centres against the seeds, +57.0 ± 9.8 (BAS-S24);
  - the current state against 1.10.2, +188.3 ± 17.5 (BAS-S25).
- **Bracket `[0,10]`.** The wide bracket is for a large prior, and the
  three readings above put both gates far above +10. `[0,3]` would spend
  games resolving a question already answered by size.
- **Gate (a): the cluster.**
  - Engine A `basilisk-b24-on-unfit-pext-pgo.exe`: B2_CORE on, revision
    `399c7ce`, `bench 13` 6,096,792, SHA-256
    `BF64DE180BDD2ABC169B47F5110E651E26D37807C526C90E9B3FB1F9B9D5C06C`.
  - Engine B `basilisk-b24-off-pext-pgo.exe`: B2_CORE off, the same
    revision, `bench 13` 14,978,465, SHA-256
    `C9FD51631E9A50701B8A7B83400EABC5AFCDFD709B33F41D871784FE57FB0D1C`.
  - Seed 24001, results in `tools/results/b24-gate-a/`.
- **Gate (b): the fit.**
  - Engine A `basilisk-b24-on-fit-pext-pgo.exe`: B2_CORE on, revision
    `2ab17c7` (the bake, `b777ba8`, plus the regenerated surface),
    `bench 13` 11,099,210, SHA-256
    `5D04A916445C41A744633C7026632B8AAD351B734B4AD0F54E38E4FE10EE322A`.
  - Engine B: gate (a)'s engine A.
  - The engine sources differ only in `src/search_params.h`'s 46 baked
    defaults. The two arms come from different commits, so the launcher
    pins both hashes and the wrapper checks each manifest's clean tree
    and fingerprint.
  - Seed 24002, results in `tools/results/b24-gate-b/`.
- **Both gates:**
  - Release PEXT final-PGO binaries from `tools/build_test.ps1`, clang,
    clean manifests;
  - Colosseum SPRT, `sprt-wide.toml` (normalized, α = β = 0.05);
  - `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
    tablebases off; concurrency as the wrapper resolves it;
  - launcher `tools/run_b24_gates.ps1 -Gate a|b`.
- **Cap: 2,500 pairs (5,000 games) per gate.** By the drift model the
  expected LLR reaches the upper bound ln 19 = 2.94 inside the cap for any
  true value above about +12. At the readings above both gates should end
  within a few hundred games.
- **Order and stop rule.**
  - Gate (a) runs first, then gate (b), never together.
  - H1 accepts the gate. H0 on either is this entry's falsifier (packet
    §11's stop rule).
  - The cap without a decision is inconclusive, not a pass: report the LLR
    and return to research.
- **Bake-time floors (PROCESS go/no-go step 7):**
  - Qualification of the baked tree (`tools/results/b21-r3-20261011/bake/`):
    - all 12 builds; ON `bench 13` 11,099,210 and `bench 10` 2,034,942 on
      every flavour; OFF 14,978,465 and 3,190,673, unchanged;
    - CTest 100% on both arms in Release (23 and 20 tests) and under the
      sanitizer (21 and 18);
    - WAC 181 at depth 6 (floor 130) and 241 at 100k nodes (from 237).
  - The endgame cohort under generous limits:
    `tools/diag/endgame_cohort_v1` (770 positions; an LF copy whose bytes
    match the manifest's pin), 600,000 nodes a move, 100 plies, paired
    position by position, unfitted (`BF64DE18…C06C`) against fitted (the
    qualification binary `F059C8F6…191E`, the same source as gate (b)'s
    engine A): `tools/results/b24-20261016/endgame-600k-{unfit,fit}.json`
    (`642f0477…604e`, `4ec94fd9…81c5`).
    - The CTest floor families hold: KQ-K, KR-K and KBB-K 24/24 on both;
      KBN-K 24/24 unfitted and 23/24 fitted, inside the floor's one
      position of slack. The fitted miss (EG0081) never discards the
      tablebase win but makes too little progress and draws by the
      fifty-move rule at ply 99.
    - Over the 480 clean wins: 458 converted unfitted and 452 fitted;
      paired, the fit loses 14 and gains 8 (sign test p ≈ 0.29), mostly
      100-ply limits in KNN-KP, KRP-KR and KQ-KRP. Win-preserving moves
      99.26% against 98.96%; 19 and 20 positions discard a win.
    - No floor breaks. The small tilt is consistent with the fitted
      tree's 82% larger fixed-depth size reaching less depth per node
      budget; it is recorded, not a gate.
- **Dry runs:** both accepted 2026-10-16:
  - gate (a) `tools/results/colosseum_sprt_b24-gate-a_20261016_082245.*`
    (dry run `0C71040F…4A8B`);
  - gate (b) `tools/results/colosseum_sprt_b24-gate-b_20261016_082250.*`
    (dry run `6FADF715…295E`);
  - each reads normalized `[0,10]`, α = β = 0.05, cap 2,500 pairs,
    concurrency 15, no adjudication, and the pinned hashes and benches.

#### RESULT — append after exposure

- **Gate (a), the cluster: H1 accepted.** Unfitted ON against OFF at
  `399c7ce`: **+136.2 ± 33.6 Elo** (nElo +192.8 ± 42.9), LLR +2.96 after
  126 pairs (252 games); W/D/L 131/84/37, pentanomial [1, 15, 26, 57, 27].
  - Run 2026-10-16 15:09–15:12 local by the agent, under the maintainer's
    one-time exception for this gate.
  - 0 faults; 16 post-terminal pairs kept outside the sample; the
    independent PGN recount agrees.
  - `colosseum_sprt_b24-gate-a_20261016_150939.*`, manifest
    `0e7ae11e…8778`, run record `1262eb3e…4079`, PGN `7b6b3351…e76e`.
  - It agrees with B.2.2's paired run (+130.9 ± 11.4 at `84da91f`), which
    preceded `CoreLmpExemptChecks` 0 and Amendment 5.
- **Gate (b), the fit: H1 accepted.** Fitted ON against unfitted ON:
  **+58.7 ± 20.9 Elo** (nElo +94.2 ± 32.8), LLR +3.00 after 215 pairs
  (430 games); W/D/L 152/198/80, pentanomial [2, 33, 90, 71, 19].
  - Maintainer-run 2026-10-16 08:54–08:59 local.
  - 0 faults; 16 post-terminal pairs kept outside the sample; the
    independent PGN recount agrees.
  - `colosseum_sprt_b24-gate-b_20261016_085419.*`, manifest
    `0065b6e8…b9c8`, run record `64ef6acb…a8ed`, PGN `91f00b29…d244`.
  - It agrees with BAS-S24's +57.0 ± 9.8 for block 1's centres: block 2
    added little, as its STOP said.
- **Order.** Gate (b) ran before gate (a), against the registered order.
  The two use different binary pairs and seeds and never ran together,
  so neither reading depends on the order.
- **Each gate accepted its own thing:** the cluster on its own evidence,
  and the fit on its own evidence. Chained, B.2 reads about +195 Elo over
  the B.1 head; BAS-S25's direct reading of the block-1 state against
  1.10.2 was +188.3 ± 17.5.
- **Default flipped:** `B2_CORE` defaults ON (`8a10d14`); the
  development fingerprint is now `bench 13` 11,099,210. The legacy
  kernel stays compilable with `-DB2_CORE=OFF` (14,978,465) until B.8,
  and CI builds and tests it (`2853699`).
- Disposition: **both gates H1; Cluster 1 accepted and the default
  flipped. CLOSED 2026-10-16.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **This entry's prediction (P2):** +40 Elo at STC, 90% interval [+5, +90],
  H1 probability 65%.
  - H1 came, on both gates.
  - The size missed high by about +105: the chained +195 sits far
    outside the interval. The falsifier (H0, or a stop under +10) did
    not fire.
- **P1–P6 of BAS-S21, read together:**
  - P1 (unfitted −15 ± 30): missed high by about 146 (B.2.2's +130.9;
    gate (a) +136.2).
  - P2: missed high, above.
  - P3 (nodes at depth 12 ≤ 2.5× the oracle): held at 1.86, with
    branching under its window by 0.0035.
  - P4 (WAC at 100k ≥ 225, LMR-off delta ≤ +5): WAC held (249, now 241
    fitted); the delta missed (+8).
  - P5 (NPS 0.92–0.98× B.1): missed above the band, at 1.038×.
  - P6 (LMR re-searches 3–5%, RFP cuts under 12%): held.
- **What it teaches:** the mechanism screens (P3, P6) held, and the misses
  all fell on the favourable side. The Elo predictions were anchored far
  too low: the packet priced the coherent core at +40, and it was
  nearer +195. The research for B.3–B.5 should not reuse that prior.
