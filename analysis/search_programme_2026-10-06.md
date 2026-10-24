# Search programme investigation — PLAN B.0

- State / class: research delivered; `READY_FOR_IMPLEMENTATION` for B.1 and
  B.2, B.3's handoff frozen contingent on the accepted B.2 head; two
  maintainer-run measurement sub-steps spawned (B.0.1, B.0.2). Class `R3`.
- Owner / date: B.0, 2026-10-06, on `dev` at `6c5ee63` (Phase A closed).
  **No engine source changed.** Tooling changed: `tools/diag/suite_v2.epd`,
  `tools/diag/scale_ratio.py`, `tools/oracle/build_oracle.ps1 -ExtraPatch`.
- Decision needed: the contents and seeds of cluster 1, the B.2.2 screen
  numbers, the frozen B.2 predictions, and whether the two cheap game tests
  registered below run before B.2.1.

Sources of record: Reckless `31d9cd6` (`src/search.rs`, `movepick.rs`,
`history.rs`, `transposition.rs`, `stack.rs`, `evaluation.rs`, read first
for mechanism and population as the maintainer asked); **modern Stockfish
pinned at the `sf_19` release tag `edb0d9db` (2026-09-05)**, the architecture
donor (PLAN rule 1; the `0a215d6c` default is superseded because the release
tag is reproducible and the official SF 19 binary used below is built from
it; the local checkout `59aae690` differs from it in `search.cpp` by three
small later patches, noted where they matter); classical Stockfish
`9587eeeb` through the `oracle/hybrid` package (the oracle and the HCE-era
seed column). Rarog's packet `docs/reference/rarog/analysis/search_programme_2026-09-13.md`
is the template; its verdicts are priors, never transferred.

Everything was measured today on the maintainer's idle Ryzen 9 5950X, one
thread, Hash 64, and archived under `tools/results/b0-20261006/` (ignored
storage; hashes in §15). Binaries: Basilisk 1.10.1 release
`basilisk-a74-nps-pgo1-pext-pgo.exe` (`3e5294be`, bench 14,978,465, the A.7.4
baseline binary; `dev` since differs only by compiled-out instrument code),
Tune `basilisk-b0-tune-pext-tune-pgo.exe` and Ablate
`basilisk-b0-ablate-pext-ablate-pgo.exe` (both `6c5ee63`, bench 14,978,465),
the A.7.3 oracle `oracle-1.10.1.exe`, a new `oracle-1.10.1-ablate.exe` (same
recipe plus the snapshot's `oracle-hybrid-ablate.patch`; conformance 471,519
positions, 0 mismatches; bench 967,078 = the unpatched oracle's), and the
official Stockfish 19 `x86-64-universal` binary.

## 1. Verdict

**B.1 and B.2 are `READY_FOR_IMPLEMENTATION`; B.3's handoff is frozen and
contingent on the accepted B.2 head.** PLAN's programme shape — one
Stockfish-shaped selectivity core adopted as a unit, seeded from three
columns, fitted, gated at `[0,10]` — survives, with five corrections that
change what B.2 builds, how it is seeded, and how B.2.2 screens it:

1. **The deficit is a constant ×4 node multiplier from depth 4 on, not
   per-ply growth.** Over depths 4–14 on 103 common positions Basilisk's
   geometric branching factor is **1.767** against the oracle's **1.757**
   (Stockfish 19: 1.549); its tree is **3.5× at depth 4, 4.0× at depth 8 and
   3.7× at depth 14**. BAS-D05–D08's "1.9× plateau" was measured on 16
   expensive positions; on the whole suite the multiplier is about 4.
2. **The multiplier is the shallow move-loop pruning family.** On the matched
   ablation builds, removing that family (mask bit 5: LMP, futility,
   history and SEE pruning in the move loop) makes the two trees **equal at
   depth 4 (0.98×) and 1.6× at depth 12** (per-position median 1.23×); with
   everything on they are 3.5× and 4.0×. The oracle's family carries a
   per-position median **4.5×** of its selectivity at depth 12, Basilisk's
   **2.2×**. LMR (3.4× against 4.5×), RFP (1.28× against 1.34×), NMP (1.08×
   against 1.22×) and the extension families (both about 1.9×) are equal or
   favour Basilisk. This is the measured form of BAS-S16's lesson: the
   oracle prunes on better information (history-adjusted `lmrDepth`,
   continuation-history pruning, quiet SEE pruning, capture futility), not
   harder on the same decisions.
3. **Basilisk's razoring is a measured tactical defect.** With razoring off
   alone, WAC at 100,000 nodes rises from **204 to 255** (59 gained, 8
   lost; 40 of the 50 positions only the oracle solved flip) for +13%
   nodes to depth 12; the oracle's razoring off changes its 241 by −5.
   `RazorCoeff=500` recovers 32 of the 59 at +5% nodes, so half the loss is
   the margin's reach to depth 3 and half is razoring at depth 1 into a
   captures-only quiescence. The HCE scores a sound sacrifice as material
   down with no compensation signal, `eval + 243·depth <= alpha` fires at
   depth 2–3, and the quiet follow-up is never searched. B.2 seeds razoring
   from the oracle column (depth 1, 256 cp), and B.0.1 registers the cheap
   Elo-layer test on the existing knob.
4. **Seeds cannot be converted by one scalar, more so than for Rarog.**
   Basilisk's static evaluation is **0.282** of Stockfish 19's search-facing
   value and **0.557** of the classical oracle's own HCE (10,039 positions),
   and its search-minus-static residual at 50,000 nodes averages **92 cp
   against a mean static magnitude of 165 cp** (the oracle's search on the
   same evaluation: 125 cp). Rarog's ratio was 0.457 with a residual of a
   third of its mean. Stockfish's NNUE-sized margins converted at 0.28 would
   fire at nearly every node; the three-column rule applies with the oracle
   column first for every evaluation-unit constant (§7).
5. **Basilisk's LMR is not timid; it is uninformed.** It removes more nodes
   than the oracle's (4.5× against 3.4× at depth 12) and costs **15 WAC
   positions at 100,000 nodes** when on (219 with it off), where the
   oracle's LMR costs none (236 off, 241 on). BAS-D02's "far too
   conservative" reading is withdrawn; the shape target is what is reduced
   and what a reduced result trains, which the donor's formula and its
   re-search rules provide.

Two cheap game tests are registered and spawned as B.0.1 and B.0.2 (§11);
neither blocks B.1. The expected size of B.2 is tens of Elo unfitted and
around +40 fitted (§11, P1–P2), with the balance of the 312.6-Elo deficit in
B.3's proof searches and check policy, B.4's quiescence (the quiet-check
screen below) and the evaluation programme.

## 2. The question, hypotheses and what B.0 measured

**Question.** At equal time the classical oracle, running Basilisk's own
evaluation, beats 1.10.1 by 312.6 ± 17.8 Elo (BAS-O05) while searching fewer
nodes per move at lower NPS. Which mechanisms, in which shapes, carry that
deficit, and how should the Stockfish architecture be cut into clusters for
this engine?

**Hypotheses, stated before the screens were run.**

- **H1 (PLAN's, leading).** The deficit is in the co-adapted selectivity
  core: the information each pruning and reduction decision is made on
  (TT-refined and corrected evaluation, history-adjusted depth, richer
  history training and ordering), so a coherent donor-shaped core adopted as
  a unit recovers a large share, where one-mechanism edits measured zero or
  worse (BAS-S13–S16).
- **H2 (competing).** The deficit is dominated by Basilisk's check policy
  (every in-check node extended one ply, no check ever pruned or reduced;
  15.6% of interior nodes) and its loose null-move entry — B.3's families —
  so B.2 alone reads near zero and the cluster order should change.
- **H3 (Phase 5's).** Width is a symptom of the evaluator. Refuted before
  B.0 by BAS-O04 (1.6% of the width follows the evaluator) and by the oracle
  playing +312.6 with the same evaluation; its surviving form is that the
  HCE's error, not its scale, decides which margins can be imported (§7).
- **H4 (instrument).** G(0) is at equal time with the oracle 30% slower
  (BAS-O05), which understates the gap; node counts on both sides include
  quiescence; nominal depth is not comparable across engines, so every
  depth number below is read beside a fixed-node quality number. One
  instrument defect was found and fixed (§14): `suite_v1.epd` carried an
  illegal position the 1.10.1 board rejects. Nothing here explains a
  300-Elo deficit.

**Evidence layers** (PLAN rule 8): branching and nodes-to-depth are tree
shape; WAC at fixed nodes is tactical move quality; counters are activation;
the scale ratio and residual are units; the matched ablation screens are
marginal selectivity and fixed-node quality per family. None is Elo. The only
strength evidence cited is the ledger's (BAS-O01–O05, BAS-S08, BAS-S16,
BAS-X30).

### 2.1 Reference-anchored branching profile (`tools/diag/branching.py`)

`suite_v2.epd` (106 positions; 103 complete every depth on every arm: two
trivial mates end early for Basilisk, one position for the references),
Hash 64, one thread, a fresh process per arm and depth, depths 4–14.

| Arm | Geometric branching 4→14 | Per-position median [min, max] | Nodes d4 | d8 | d14 | ms at d14 |
|---|---:|---:|---:|---:|---:|---:|
| Basilisk 1.10.1 | **1.767** | 1.697 [1.05, 2.27] | 164,885 | 2,457,982 | 48,966,427 | — |
| Oracle (`9587eeeb` + Basilisk HCE) | **1.757** | 1.773 [1.11, 2.14] | 47,069 | 616,869 | 13,220,509 | — |
| Stockfish 19 | **1.549** | 1.514 [1.12, 2.29] | 61,837 | 358,354 | 4,919,265 | — |

Node ratio Basilisk/oracle by depth: 3.50, 3.83, 4.55, 4.22, 3.98, 4.18,
4.41, 4.48, 3.97, 3.75, **3.70** (depths 4–14). Basilisk/Stockfish 19: 2.67 at
depth 4 rising to **9.95** at depth 14, because Stockfish's growth is lower.
Per-ply growth is therefore not the deficit against the oracle; a constant
multiplier of about 4 already present at depth 4 is. (Timing columns are
omitted: the three arms ran concurrently with other screens.)

### 2.2 Depth and agreement at a fixed budget (`run_suite.py --nodes 300000`)

| Cohort (n) | Basilisk median depth | Oracle | Agreement |
|---|---:|---:|---|
| openings (40) | 13 | 16 | — |
| WAC tactics (40) | 12 | 19 | — |
| endgames (17) | 20 | 30 | — |
| zugzwang (5) | 14 | 21 | — |
| in check (4) | 57 | 21.5 | mates found by Basilisk |
| all (106), median | **14** | **18** | **66 / 105** (62.9%) |

Mean gap 11.2 plies with the mate runaways, 5.5 without them (93 positions);
the median gap of 4 plies agrees with BAS-D07. A ×4 node multiplier at
branching 1.76 predicts about 2.5 plies, so about 1.5 plies remain
unexplained by the multiplier alone, as BAS-D07 recorded.

### 2.3 Tactical quality at fixed nodes and the anchors (`fixed_budget_probe.py`)

`src/wac.epd`, 300 positions.

| Budget | Basilisk | Oracle | Stockfish 19 | Only oracle / only Basilisk |
|---|---:|---:|---:|---|
| 100,000 nodes (median completed depth) | **204** (10) | **241** (16) | 261 (17) | 50 / 13 |
| 400,000 nodes | **242** (14) | **272** | 278 | 34 / 4 |
| depth 10, solved first by depth 10 | **224** | 212 | 218 | — |
| depth 10, stable from a depth ≤ 10 | 218 | 195 | 206 | — |
| solved first by depth 3 | 74 | 117 | 137 | — |

At equal nominal depth Basilisk is level or better; at equal nodes it is six
plies shallower and 37 positions behind. The 50 oracle-only positions at
100,000 nodes are dominated by quiet mate threats and checking sacrifices
the oracle answers from depth 2–3 (WAC.085 `Na6`, WAC.203 `Qh6`, WAC.253
`Qe8+`, WAC.007 `Ne3`, WAC.099 `Rh5`): 24 of them flip when Basilisk's inert
first-ply quiet-check loop is enabled (§2.7), and 40 when razoring is off.

**Canaries (A.5.6 contract).** `canary.py check` against `canary_v1.json`
with today's depth-12 PV and 100,000-node probes on the release binary:
**77 of 77 required pass, 0 regressions, 0 new passes**. WAC.001 (`Qg6`, the
quiet mate threat) is still unsolved through depth 10 and at 100,000 nodes.

**Decision traces** (A.5.3, plies 1–2 under `searchmoves`, depth 10) on
WAC.007, 085, 203 and 253 found no Rarog-type seed defect: under
`searchmoves` Basilisk proves the mates in WAC.203 and 253 (alpha in the
mate band at ply 2) and reduces or count-prunes late quiet replies in 007
and 085 exactly as its formulas say. The misses are budget and quiescence
misses, which the matched screens then localised.

### 2.4 Scale ratios and the residual (`tools/diag/scale_ratio.py`, new)

Corpus: the 40 bench FENs (one in check, skipped) plus 10,000 positions
sampled with seed 20261006 from `tools/texel/data/holdout_phase911.csv`.
Basilisk: `basilisk-texel --dump-eval` (full evaluation, lazy path off);
the oracle's bridge value agrees with it on every position except where the
lazy path fires (mean difference 0.146 cp, maximum 200 cp). Oracle: classical
Stockfish's own HCE in its internal units (`PawnValueEg` 206). Stockfish 19:
the raw network output in internal units and the final search-facing value,
inverted from its `to_cp` normalisation with the pinned `win_rate_params`.

| Set | n | mean \|Basilisk\| cp | \|oracle HCE\| | \|SF19 raw\| | \|SF19 final\| | ratio to SF19 final | ratio to oracle HCE |
|---|---:|---:|---:|---:|---:|---:|---:|
| all | 10,039 | 167.9 | 301.3 | 572.4 | 595.8 | **0.282** | **0.557** |
| bench 39 | 39 | 184.4 | 354.4 | 611.1 | 654.5 | 0.282 | 0.520 |
| opening (NPM ≥ 5800) | 263 | 101.1 | 182.2 | 224.6 | 276.0 | 0.366 | 0.555 |
| middlegame | 4,908 | 133.2 | 263.4 | 448.3 | 503.6 | 0.264 | 0.506 |
| endgame (NPM ≤ 2400) | 4,868 | 206.4 | 346.0 | 716.4 | 706.1 | 0.292 | 0.597 |

Per-position ratio to Stockfish 19's final value: median **0.293**, quartiles
0.185–0.605; sign agreement with Stockfish 78.7%. The oracle's unit
conversion is exactly `100 / 206 = 0.485` (the bridge multiplies Basilisk's
centipawns by 2.06); the measured 0.557 says Basilisk's evaluation is about
15% larger in pawn units than classical Stockfish's HCE, so an oracle
constant converted at 0.485 is slightly tighter here than it was there.

**Residual** (2,000 corpus positions, 1,997 usable, `go nodes 50000`, Hash 64):

| Quantity | Basilisk | Oracle search on Basilisk's evaluation |
|---|---:|---:|
| mean \|search − static\| | **92.3 cp** (median 55) | **124.9 cp** (median 71) |
| mean \|static\| on the same positions | 164.6 | 164.6 |
| mean \|Basilisk score − oracle score\| | 56.0 | — |

The residual is 56–76% of the evaluation's own magnitude. Reckless's and
Stockfish's margins are sized for an error far smaller than their scale;
converted by 0.28 alone, Stockfish 19's reverse-futility margin at depth 3
would be about 48 cp against a 92-cp average error. This is why the oracle
column leads for every evaluation-unit seed (§7).

**Units for SEE and material constants.** Basilisk's search values are
100/300/300/500/900; Stockfish 19's `PieceValue` 208/781/825/1276/2538
(ratio 0.48 for pawns, 0.35–0.39 for pieces: **0.38**); the classical oracle's
MG values 124/781/825/1276/2538 (**0.81** pawn, 0.38 pieces, its history
maxima differ as in §7).

### 2.5 Stride-1 counters (Tune build, `run_suite.py`)

Suite_v2 at depth 14 (106 positions; the depth-12 and bench-13 runs agree
within a point on every share):

| Quantity | Value | Reading |
|---|---:|---|
| interior / quiescence nodes | 31,549,396 / 17,340,089 | quiescence share **35.5%**, 0.55 qnodes per interior node (BAS-D09's 35.1%; oracle ~36%: matched, not a width source) |
| in-check interior nodes; check extensions | 15.9%; **15.6%** of interior | every in-check node is extended (BAS-S16's 15.84% stands) |
| `rfp_cuts` / interior | **14.2%** | razoring 6.2%; Rarog read 41.9%, the oracle about 32% at depth 8 |
| `null_cuts` / `null_tries` | **49.6%** | 753,669 of 1,518,153; half the null searches are wasted (Rarog 24%, oracle ~83%) |
| `probcut_cuts` / tries | 68.4% | 129,195 of 189,013 |
| move-loop prunes | LMP 49,058,753; futility 2,495,688; SEE 1,654,800; history **590** of 11,272,144 tested | history pruning is dead (BAS-D04); LMP fires 1.6 times per interior node, each candidate calling `gives_check` |
| LMR | eligible 17,317,985; applied **36.4%**; computed zero 15.8%; blocked by depth < 2 28.5%, second move 11.8%, in check 2.9%, gives check 3.6%, move type 1.0% | mean reduction **2.47** plies; **56.5%** of applied reductions hit the `new_depth − 1` ceiling (near-leaf population); re-search **1.67%** |
| cutoffs | 8,967,546; first move **89.1%**; mean index 0.210; sources TT 32.7%, good captures 42.7%, quiets 24.5%, bad captures 0.07% | ordering at the cutoff is strong (BAS-D01 stands) |
| history training | 8,967,546 cutoff updates, **52,782** exact-node reward updates (0.6%) | nothing trains on fail lows, eval swings or TT cutoffs |
| singular | fired 141,896 (**0.45%** of interior), double 50% of fired, in check 37%, triple 29%, negative 59,787 | sparse against Rarog's 4.3% attempts |
| TT | probes 39.4M, hits 35.6%, cutoffs 7.7% of interior, stores 24.8M (30.3% same key), **`tt_pv` nodes 0.57%** | the reconstructed `tt_pv` is nearly absent as a signal; no eval stored on a miss |
| aspiration | 1,058 windows, 1,595 re-searches (**1.51** per window), 16 give-ups | BAS-D16's 1.37 |
| cost telemetry | `eval_calls` 45% of nodes; `gives_check` **1.43 per node**; `see_ge` 0.57 per node | the per-move `!move_gives_check()` guards on every LMP/futility candidate are a throughput item for B.7, not a shape item |

### 2.6 Matched per-family ablation screens (new; the cheapest discriminator)

Both engines expose the same eight-bit `AblationMask` (A.5.4: 0 razoring,
1 RFP, 2 NMP, 3 ProbCut, 4 IIR (the oracle's IID), 5 shallow move-loop
pruning, 6 extensions, 7 LMR). Each bit alone on suite_v2 depths 4–12 and on
WAC at 100,000 nodes; predictions S1–S4 were frozen in
`frozen_predictions_screens.md` before any mask was read.

**Basilisk** (mask 0: 164,889 / 2,457,990 / 20,927,918 nodes at d4/d8/d12 on
104 positions; WAC 204):

| Bit off | nodes ×, d4 | d8 | d12 | per-position median [q1, q3] at d12 | WAC at 100k | Δ | gained / lost |
|---|---:|---:|---:|---|---:|---:|---|
| razoring | 1.035 | 1.146 | 1.132 | 1.00 [0.73, 1.31] | **255** | **+51** | 59 / 8 |
| RFP | 1.179 | 1.398 | 1.237 | 1.34 [1.00, 1.74] | 197 | −7 | 14 / 21 |
| NMP | 0.995 | 1.261 | 1.392 | 1.22 [1.00, 1.65] | 202 | −2 | 15 / 17 |
| ProbCut | 1.000 | 1.045 | 1.205 | 1.00 [0.88, 1.23] | 217 | +13 | 22 / 9 |
| IIR | 1.000 | 1.099 | 2.361 | 1.18 [0.97, 1.65] | 202 | −2 | 15 / 17 |
| shallow pruning | 1.430 | 2.171 | 2.284 | **2.20** [1.44, 3.24] | 179 | −25 | 6 / 31 |
| extensions | 0.878 | 0.430 | 0.366 | **0.58** [0.25, 1.00] | 206 | +2 | 23 / 21 |
| LMR | 1.437 | 2.767 | 12.398 | **4.54** [2.16, 7.95] | 219 | +15 | 26 / 11 |

**Oracle** (mask 0: 47,262 / 617,254 / 5,275,228 on 105 positions; WAC 241):

| Bit off | d4 | d8 | d12 | median [q1, q3] | WAC | Δ | gained / lost |
|---|---:|---:|---:|---|---:|---:|---|
| razoring | 0.997 | 1.032 | 1.035 | 1.00 [0.92, 1.28] | 236 | −5 | 11 / 16 |
| RFP | 1.237 | 1.327 | 1.413 | 1.28 [1.00, 1.86] | 243 | +2 | 17 / 15 |
| NMP | 1.002 | 1.101 | 1.081 | 1.08 [0.93, 1.40] | 233 | −8 | 8 / 16 |
| ProbCut | 1.000 | 1.011 | 1.076 | 1.01 [0.82, 1.29] | 247 | +6 | 19 / 13 |
| IID | 1.000 | 1.000 | 1.064 | 1.00 [0.98, 1.19] | 238 | −3 | 13 / 16 |
| shallow pruning | **5.078** | **6.129** | **5.514** | **4.55** [2.19, 7.07] | 245 | +4 | 21 / 17 |
| extensions | 0.947 | 0.689 | 0.455 | **0.52** [0.33, 0.90] | 248 | +7 | 21 / 14 |
| LMR | 1.073 | 2.106 | 4.448 | **3.41** [1.84, 5.32] | 236 | −5 | 22 / 27 |

**Cross-engine node ratio with a family removed on both sides** (Basilisk /
oracle; per-position median at depth 12 in brackets): all on 3.49 / 3.98 /
3.97 [2.77]; **shallow pruning off 0.98 / 1.41 / 1.64 [1.23]**; LMR off 4.67 /
5.23 / 11.06 [3.09]; extensions off 3.23 / 2.49 / 3.20 [2.92]; RFP off 3.32 /
4.20 / 3.47 [2.45]; razoring off 3.63 / 4.42 / 4.34 [2.56].

Readings, against the frozen predictions:

- S1 held for LMR, shallow pruning, RFP, NMP and razoring; missed for ProbCut
  (1.21 against ≤ 1.15) and IIR (2.36 against ≤ 1.25, but 87% of that excess
  is two positions; the median 1.18 is the honest number and is inside).
- S2 held: extensions off shrinks Basilisk's tree more than predicted (0.37,
  median 0.58) with WAC unchanged (+2); median completed depth at 100,000
  nodes rises from 10 to **14**. The same is true of the oracle (0.46, WAC
  +7, depth 16 → 20). **H2 is weakened:** both engines pay about 1.9× for
  their extension families and neither buys fixed-node tactics with them; the
  check policy is not the differential. It stays B.3's question.
- S3 held: the oracle's shallow pruning multiplies its nodes by 5.5 (≥ 3.0
  predicted) and is far larger than Basilisk's 2.3; its RFP reads 1.41
  (predicted ≥ 1.5, a small miss) and is not a differential. **H1's main
  support stands and is now specific:** the move-loop pruning family.
- S4 missed in the instructive direction: razoring off **+51**, LMR off
  +15, ProbCut off +13 are mechanisms that cost tactics at fixed nodes in
  Basilisk; shallow pruning off −25 (below the predicted floor) says
  Basilisk's count pruning is what buys it depth. On the oracle no single
  family moves WAC by more than 8. Basilisk's selectivity is tactically
  fragile; the oracle's is tactically free.

### 2.7 Two single-knob screens (Tune build)

| Knob | WAC at 100k | gained / lost | oracle-only flipped | solved first by depth 3 / 6 / 10 | nodes to d4 / d8 / d12 |
|---|---:|---|---:|---|---|
| baseline | 204 | — | — | 74 / 145 / 224 | — |
| `QsearchCheckCap=6` (first-ply quiet checks) | **214** | 24 / 14 | 18 of 50 | **106** / 161 / 237 | +17.5% / +9.3% / **+32.7%** |
| `RazorCoeff=500` | **228** | 33 / 9 | 24 of 50 | — | +1.4% / +2.0% / **+5.1%** |
| `RazorCoeff=400` | 223 | 28 / 9 | 18 of 50 | — | — |

S5 held on three of four counts (the WAC rise is +10 against ≥ 11 predicted;
the depth-3 count and the node cost held); the 14 losses say quiet checks
move tactics around at a fixed budget as much as they add, consistent with
the `hcefinal` SPSA pinning the cap at 0 and with B.4's card. S6 missed on
magnitude (32 of 59 recovered against ≥ 35 of 51 predicted): half of
razoring's tactical cost is its depth reach, half is razoring at depth 1
into a quiescence without checks.

## 3. Mechanism map: Basilisk against the donor

Verdicts: **ADOPT** the donor's form in the named cluster, written in
Basilisk's own structure; **KEEP** Basilisk's form with the local evidence
that earns it; **DROP**; **DEFER** to a later leaf. "SF" is Stockfish 19
(`edb0d9db`); "Reckless" is `31d9cd6`; "oracle" is classical `9587eeeb`.
Constants are seeds (§7).

### 3.1 Node entry, draws, mate bounds

| Mechanism | Basilisk | Donor | Verdict |
|---|---|---|---|
| Node typing | `is_pv`, `cut_node`, `allow_null` runtime flags; root by `ply == 0` | SF `NodeType {Root, PV, NonPV}` template, `cutNode` runtime, `allNode = !PV && !cutNode` | **ADOPT (B.1)** per A.6; `cut_node` stays runtime; `allow_null` is replaced by the donor's "no null after null" check in B.3 and kept as a flag until then |
| Draw score | 0 | SF `value_draw(nodes)` = ±1 jitter; Reckless `draw(td)` | **DEFER** to B.5 with the root cluster (never a standalone candidate) |
| Upcoming repetition | none (BAS-S12 closed) | SF/Reckless cuckoo `upcoming_repetition` raising alpha to the draw score | **DEFER**: BAS-S12's trigger (one named missing consumer) is not met by this investigation; D.3/D.4 territory |
| Mate-distance pruning, `MAX_PLY` 128 | present | present; SF 246 | KEEP |
| Rule 50 at TT probe | `score_from_tt` returns 0 at clock ≥ 100 | SF `value_from_tt` downgrades mate/TB scores by `100 − r50c`; no TT cutoff at clock ≥ 96 | **ADOPT (B.2)** the cutoff guard and the downgrade with the TT ticket |

### 3.2 Transposition table

| Mechanism | Basilisk | Donor | Verdict |
|---|---|---|---|
| Entry | 10 B in a 32 B cluster of 3: key16, score, eval, move, depth i8, flag(2)+age(6) (BAS-C05 accepted risk) | SF 10 B in 32 B/3, flag(2)+pv(1)+age(5); Reckless 8 B with 21-bit packed keys | **KEEP** the layout; **re-allocate one age bit to a persisted `tt_pv` bit** (age 5 bits, +4 per search becomes +8 of 256, so the age cycle is 32 searches as in SF). The reconstructed `tt_pv` reaches 0.57% of nodes (§2.5) and feeds only a 23/1024 LMR term; the donor's signal feeds RFP, NMP, singular, LMR and replacement (§4). BAS-D(8.5.7)'s "no operating point through the LMR route" was one consumer alone |
| Store on probe miss | never | SF/Reckless write the raw static eval with `DEPTH_UNSEARCHED` on every miss | **ADOPT (B.2)**; fingerprint-changing |
| Estimated score | `eval` = TT score when its bound proves it (BAS-S01, +7.18) | identical rule in both donors | **KEEP**; already the donor's |
| Early cutoff | `!is_pv && tt_depth >= depth`, bound matches | SF `ttDepth > depth − (ttValue ≤ beta)`, bound matches, `cutNode == (ttValue ≥ beta) || depth > 4`; graph-history check one ply down at depth ≥ 7; no cutoff at rule-50 ≥ 96; quiet TT move failing high trains `131·depth` and penalises the parent's early quiet −2210 | **ADOPT (B.2)** all four (PLAN lists them) |
| Replacement | same-key store refused when `depth < old − 3` unless exact, same age; quality `depth − age/2 + 2·exact` | SF refuses unless exact, different key, `depth + 2·pv > old − 4`, or older age; quality `depth − 8·relative age`; `penalize(1)` on a window-mismatched entry | KEEP ours as the seed (equivalent); `penalize` **DEFER** (post-SF-19 refinement) |

### 3.3 Static evaluation and correction

| Mechanism | Basilisk | Donor | Verdict |
|---|---|---|---|
| Corrected eval | `raw + (pawn + minor + own_np + opp_np + cont1) / 5`, clamp inside the mate band | SF `raw + (15341·pawn + 10569·minor + 12906·(npW + npB) + 8761·(cont2 + cont4)) / 131072` (sentinel 64049 without a prior move), clamped inside the TB band; Reckless `/64` with rule-50 buckets | **ADOPT (B.2)** SF's weighted form with continuation correction at plies **2 and 4** keyed by the sub-table pointer; the minor table stays (SF has it); no rule-50 buckets (Reckless only) |
| Correction update | at node exit when `best ≥ beta || best > alpha`, not in check, no exclusion, non-mate; slot ← blend with weight `min(depth+1, 16)/256`, clamp ±1024 | SF at node exit when `!inCheck && !(bestMove && capture) && (best > staticEval) == bool(bestMove)`; bonus `clamp(diff·depth·(bestMove ? 12 : 18)/128, ±256)·1061/1024` through gravity `<<` (limit 1024); also at a multi-cut from the singular value | **ADOPT (B.2)** admission, bonus and gravity; the capture exclusion is the graded form of a binary guard |
| `improving` | `eval > (ss−2)->eval`, false in check | SF `staticEval > (ss−2)->staticEval` with the in-check node inheriting `(ss−2)->staticEval`; `improving |= staticEval ≥ beta` after NMP | **ADOPT (B.2)** |
| Opponent worsening | absent | SF `staticEval > −(ss−1)->staticEval`, feeds RFP and hindsight | **ADOPT (B.2)** |
| Hindsight depth adjustment | absent (`ss->reduction` exists, unread) | SF `priorReduction ≥ 3 && !opponentWorsening → depth++`; `priorReduction ≥ 2 && depth ≥ 2 && staticEval + (ss−1)->staticEval > 166 → depth−−` | **ADOPT (B.2)**; the producer already exists |
| Eval-difference history training | absent | SF at node entry: parent's quiet move gets `(clamp(−(prev + this), −189, 194) + 60)·11` in main history and `·13` in pawn history when no TT hit | **ADOPT (B.2)** with the history policy |

### 3.4 Node-level pruning

| Mechanism | Basilisk | Donor (oracle where it seeds) | Verdict |
|---|---|---|---|
| Razoring | `depth ≤ 3`, `eval + 243·d ≤ alpha` → qsearch, return if `q ≤ alpha`; **+51 WAC at 100k when off** (§2.6) | SF `!PV && eval < alpha − 482·d²` → return qsearch (no cap; the later master made it linear); Reckless `est < alpha − 237 − 254·d²`, `alpha < 2048`, TT move not quiet, bound not Lower; **oracle `depth == 1`, margin 527 (256 cp)** | **ADOPT (B.2)** SF's return shape (always the qsearch value), **seeded from the oracle column: depth 1 only, 256 cp**, with Reckless's two guards (TT move not quiet, bound not Lower) as cheap admission; the depth cap is categorical and B.0.1's paired run prices the current reach before B.2.1 |
| Reverse futility | `!PV`, `depth ≤ 9`, `eval − (160·d − 72·improving) ≥ beta` → return `eval`; fires on 14.2% of interior nodes | SF `!ttPv`, `depth < 19`, `eval ≥ beta`, `(!ttMove || ttCapture)`, `!is_loss(beta)`, `!is_win(eval)`; `mult = min(45 + 4d, 85) − 20·!ttHit`; `margin = mult·d − (2789·improving + 335·oppWorsening)·mult/1024 + |corr|/198435`; returns `(661·beta + 363·eval)/1024`; **oracle `227·(d − improving)`, depth < 6**, returns eval | **ADOPT (B.2)** SF's shape (ttPv veto, TT-move condition, improving/worsening scaling, correction term, blended return) with §7 seeds: per-ply 133 cp, improving 89, depth cap 9 as a coordinate |
| Null move | `allow_null`, `depth ≥ 3`, `eval ≥ beta`, non-pawn material, no null after null; `R = 3 + d/4 + min((eval − beta)/192, 3)`; verification at depth ≥ 10 at the same ply; **49.6% conversion** | SF `cutNode` only, `staticEval ≥ beta − 13d − 47·improving + 365`, `ply ≥ nmpMinPly`, `beta ≥ −2000`; `R = 7 + d/3 + max((staticEval − beta)/256, 0)`; verification at depth ≥ 16 with the `nmpMinPly` region; oracle demands `staticEval ≥ beta − 33d − 33·improving + 112·ttPv + 311` and `eval ≥ beta`, `(ss−1)->statScore < 23824` | **KEEP in B.2, ADOPT in B.3**: the entry margin and the cut-node population are B.3's; B.2 re-wires `eval` to the new estimated score and `improving` to the new flag at their current values |
| ProbCut | `depth ≥ 5`, `pc_beta = beta + 189`, SEE filter `pc_beta − static_eval`, qsearch then `depth − 4`, store at `depth − 3`, return `pc_beta`; **+13 WAC when off** | SF `depth ≥ 3`, `probCutBeta = beta + 241 − 64·improving`, TT-value guard, picker threshold `probCutBeta − staticEval`, qsearch then `depth − (improving ? 5 : 3)`, store at `probCutDepth + 1`, return `value − (probCutBeta − beta)`; small TT ProbCut `beta + 428` at `ttDepth ≥ depth − 4` | **KEEP in B.2, ADOPT in B.3** (both the depth shape and the small TT ProbCut); B.2.2 reports `probcut_*` beside the WAC screen |
| IIR | `!PV`, `depth ≥ 4`, no TT move or `tt_depth < depth − 3` → `depth−−` | SF `!followPV && !allNode && depth ≥ 6 && !ttMove`; oracle IID at depth ≥ 7; Reckless none | **KEEP in B.2** (median marginal 1.18×); B.3 decides IIR against hindsight with both landed, as PLAN asks |

### 3.5 Singular and extensions (B.3; recorded for the boundary)

| Mechanism | Basilisk | Donor | Verdict |
|---|---|---|---|
| In-check extension | every in-check node `depth++` (15.6% of interior); checking moves never pruned or reduced | SF none; Reckless none; oracle extends a checking move by one at the parent when it is a discovered check or `SEE ≥ 0` | **KEEP in B.2** (BAS-S08 −10.17, BAS-S16 −3.48 both against this head's margins). §2.6: both engines' extension families cost about 1.9× nodes and buy no fixed-node tactics; the trade is B.3's, made on the accepted B.2 head where pruning is better informed (BAS-S16's retry trigger) |
| Singular | `depth ≥ 5`, TT bound ≥ Lower within 3 plies, `s_beta = tt − 4·d`, `(d−1)/2`; extend 1, or 2 when `!PV`, `s_val < s_beta − 4`, under `DoubleExtMax` 16; multi-cut returns `s_beta`; negative −1 when `tt ≥ beta`; `singular_quiet_lmr` (BAS-D18) | SF `depth ≥ 6 + ttPv`, `singularBeta = ttValue − (59 + 66·(ttPv && !PV))·depth/63`, `singularDepth = newDepth/2`; double/triple margins with PV, TT-capture, correction, `ttMoveHistory` and ply terms; `depth++` on extension; multi-cut trains correction and `ttMoveHistory`; negative **−3**; shuffling guard | **ADOPT (B.3)**; B.2 keeps Basilisk's form and wires its inputs (`tt_pv` from the persisted bit) |
| Other extensions | none | oracle: passed-pawn (killer), last capture, castling | not adopted (absent in SF 19) |

### 3.6 Move picker and histories

| Mechanism | Basilisk | Donor | Verdict |
|---|---|---|---|
| Stages | TT; good captures (`SEE ≥ 0`); all quiets; bad captures | SF: TT; good captures (`SEE ≥ −value/18`); good quiets (`> −14000`, partial sort limit `−3560·depth`); bad captures; bad quiets; `skip_quiet_moves()` ends quiet delivery; evasions, ProbCut and qsearch stage sets | **ADOPT (B.2)** |
| Capture scoring | `16·victim − attacker + capture history` | SF `capture history + 7·PieceValue[captured]` | **ADOPT (B.2)** |
| Quiet scoring | `main + cont1 + cont2 + cont4/2 + pawn + lowply + 32000·check`; killers 4M / 3.9M, countermove 3.8M tiers | SF `2·main + 2·pawn + cont1 + cont2 + cont3 + cont4 + cont6 + 16384·(check && SEE ≥ −75) + 20·PieceValue[pt]·(escape − threatened) + 8·lowply/(1 + ply)`; **no killers, no countermove** | **ADOPT (B.2)** and **DROP killers and countermove with the cluster**: the donors have neither, they rank above every graded history, and BAS-D01's ordering strength is TT- and capture-dominated. Needs the per-node threat producer (§5) |
| Tables | main `[colour][from][to]`; capture `[pt][to][cap]`; cont1/2/4 flat `[pt][to][pt][to]`; pawn 2048 buckets; low-ply 8 plies; killers; countermove; 5 correction tables | SF main `[colour][from-to with promotion]` (limit 7183); capture (10692); continuation `[inCheck][capture][piece][to] → [piece][to]` at plies 1–6 (30000); pawn 8192 buckets per thread (8192); low-ply 5 plies; `ttMoveHistory`; unified correction (1024) | **ADOPT (B.2)** the set and maxima; keep `HistoryTables` as the storage owner (A.6) |
| Update policy | bonus `min(62d²/64 + 120d, 1863)`, malus `min(62d²/64 + 143d, 1304)`, TT-move +29, surprise ×1.25 (BAS-S04); at cutoffs; exact nodes reward-only (BAS-S03); malus for searched quiets and bad captures; no fail-low, eval-swing, TT-cutoff or post-LMR training | SF bonus `min(133d − 81, 1487) + 364·(best == tt) + (ss−1)->statScore/28`, malus `min(968d − 235, 2244)`, non-PV bonus scaled by searched count; quiet bonus ×899/1024, malus decaying ×921/1024 per earlier move; capture bonus ×1427/1024, malus ×1489/1024; prev-ply early-quiet penalty ×713/1024; fail-low parent bonus with a six-term scale; eval-difference training; TT-cutoff bonus `131·d`; post-LMR continuation bonus 1334; continuation weights {520, 390, 145, 251, 66, 209} with positive-consistency multipliers; `ttMoveHistory << ±918/−747`; **updated at every node with a best move, exact nodes included** | **ADOPT (B.2)** the whole policy as a unit. BAS-S10's rejected exact-node malus returns inside the donor's index-scaled form; its retry trigger (history ownership and indexing change) fires with this cluster. BAS-S03 and BAS-S04 are subsumed by the donor's `statScore` and eval-difference terms |
| Ageing | halve every table every second search | SF main ×729/1024 per search, low-ply refilled to 102, nothing else aged | **ADOPT (B.2)** as a coordinate (decay per mille, 1024 = none) |

### 3.7 Move-loop pruning and LMR

| Mechanism | Basilisk | Donor (oracle where it seeds) | Verdict |
|---|---|---|---|
| Index | `searched` (pruned moves do not count) | SF `moveCount` counts every picked move; `ss->moveCount` read by the child | **ADOPT (B.2)** |
| Count pruning | quiets only, `depth ≤ 6`, `searched ≥ improving ? 3 + d² : 2 + d²/2`, never a checking move; **pruned one by one, 49M times at depth 14** | SF `moveCount ≥ (3 + d²)/(2 − improving)` → `skip_quiet_moves()`, no depth cap, quiet checks skipped too; Reckless exempts direct checks; oracle `futility_move_count` the same as SF | **ADOPT (B.2)** SF's form (the picker stops generating); the direct-check exemption is a B.2.2 categorical switch, pulled only if a quiet-mate canary fails (PLAN's Manta note) |
| Reduced depth for pruning | `lmr_depth = depth − base table` (never below 1: capture futility dead, BAS-D19) | SF `lmrDepth = newDepth − r/1024` from the full reduction, then `+= history/lmrDivisor[depth]` for quiets; oracle `newDepth − reduction()` | **ADOPT (B.2)**: the history-adjusted `lmrDepth` is the information the oracle's family prunes on (§2.6) |
| Capture / check branch | SEE `−73·d` at `depth ≤ 8`, no check pruned; capture futility dead | SF at `capture || givesCheck`: futility `lmrDepth < 8`, `staticEval + 234 + 247·lmrDepth + PieceValue + 134·captHist/1024 ≤ alpha`; SEE `−(177·d + 34·captHist/1024)` with the last-piece stalemate guard; oracle futility at `lmrDepth < 6`: `267 + 391·lmrDepth + value`, SEE `−202·d` | **ADOPT (B.2)**; seeds §7 |
| Quiet branch | futility `depth ≤ 6`, `eval + 180 + 128·d ≤ alpha`; history pruning `< −14004·d` (dead); SEE quiet inert | SF (`!followPV || !PV`): continuation-history pruning `cont1 + cont2 + pawn < −4136·d`; futility `lmrDepth < 12`, `staticEval + 119·lmrDepth + 90·(staticEval > alpha) + 164 ≤ alpha`, raising `bestValue` to it (fail-soft); SEE `−23·lmrDepth²`; oracle countermove pruning at `lmrDepth < 4`, futility `284 + 188·lmrDepth` at `lmrDepth < 6`, SEE `−(29 − min(l, 17))·l²` | **ADOPT (B.2)** all three with the fail-soft raise; seeds §7 |
| LMR eligibility and floor | `depth ≥ 2`, `searched ≥ 2`, quiet or bad capture, not in check, not a checking move; `reduction = clamp(r, 0, new_depth − 1)` | SF `depth ≥ 2 && moveCount > 1`, every move class; `d = max(1, min(newDepth − r/1024, newDepth + 2)) + PvNode`; a separate full-depth branch steps down by `(r > 5234) + (r > 5487 && newDepth > 2)` | **ADOPT (B.2)**; Basilisk already has the one-ply floor (0 reductions land in quiescence; the 56.5% ceiling hits are the near-leaf population); the negative-reduction allowance and the PV `+1` are new |
| LMR terms | table `0.60 + ln d·ln m / 2.09` plies; `!PV` +1024, cut +401, `tt_pv` −23, `!improving` +89, TT capture +301, singular −401, `−(stat/5683)` whole plies | SF `R[i] = 2872·ln(i)/128`; `r = R[d]·R[mn] − delta·577/rootDelta + !improving·R·197/512 + 982`; `ttPv` +929 then `−(3023 + 1004·PV + 885·(ttValue > alpha) + (ttDepth ≥ depth)·(816 + 940·cutNode))`; +697; `−65·moveCount`; `−|corr|/26310`; cut node `+4026 + 933·!ttMove`; TT capture +1079; `cutoffCnt > 1`: `+264 + 1095·(> 2) + 1138·allNode`, else TT move −2179; `statScore` (captures `873·PieceValue/128 + captHist`, quiets `(2252·main + 1126·cont1 + 1093·cont2)/1024`) `−statScore·439/4096`; quiets `+3·clamp(alpha − eval, −64, 96)`; all nodes `+r·276/(256d + 268)` | **ADOPT (B.2)** the formula as a whole in 1024ths at the donor's values (plies, counts and history units convert at 1; the `alpha − eval` clamp converts at 0.28); the singular-margin term is B.3's. BAS-S13's retry trigger (materially larger base and context reductions) is met by the donor table, so the continuous history response lands with it; BAS-S14's non-monotonic knob response was one term alone |
| Re-search | full depth when `score > alpha`; PV re-search; post-LMR nudge inert | SF `newDepth += (d < newDepth && value > best + 53) − (value < best + 8)`; re-search when `newDepth > d`; post-LMR continuation bonus 1334; PV re-search with the TT-move `max(newDepth, 1)` rule; `depth −= 3` at depth 4–11 after alpha rises; fail-high blend `(best·depth + beta)/(depth + 1)`; equal-score promotion `inc` | **ADOPT (B.2)** all; `cutoffCnt` producer `(ss+2)->cutoffCnt = 0` at entry and `ss->cutoffCnt += (extension < 2) || PvNode` at a cutoff |

### 3.8 Quiescence (B.4; the boundary)

Basilisk: TT cutoff at any bound; stand pat corrected and TT-tightened;
delta `Q + 200`; futility `stand_pat + gain + 150 ≤ alpha`; SEE threshold
`clamp(alpha − stand_pat − 200, −800, 200)`; late prune `i ≥ 6 && SEE < −50`;
quiet checks inert at cap 0; fail-hard store. SF 19: TT cutoff at
`DEPTH_QS`; stand pat `(441·v + 583·beta)/1024` stored on a miss;
`futilityBase = staticEval + 306` (86 cp); **moveCount > 2 prunes** unless
the move gives check or recaptures; SEE `−74` (28 cp); no quiet checks;
stalemate check; fail-high blend; TT write on exit. Oracle: quiet checks at
the first quiescence ply, `futilityBase + 141` (68 cp), losing SEE pruned.
**B.0's numbers for B.4's card:** first-ply quiet checks read +10 WAC at
100,000 nodes (24 gained, 14 lost), +32 positions solved first by depth 3,
+33% nodes to depth 12; razoring at depth 1 into the checkless quiescence
carries about half of razoring's tactical cost. The switch stays a measured
B.4 candidate against B.2's canaries, as PLAN records.

### 3.9 Root, aspiration and time (B.5, D.1)

SF 19: `delta = 5 + threadIdx % 8 + |meanSquaredScore|/10193` around
`averageScore` (an effort-weighted EMA), fail-high depth reduction
`failedHighCnt` (BAS-D17 refuted it standalone; it lands with the cluster or
not at all), `searchAgainCounter`, optimism `114·avg/(|avg| + 85)`,
`rootMoves[].effort`, forgotten-mate and aborted-loss guards; time:
`fallingEval`, `timeReduction`, `bestMoveInstability`, `highBestMoveEffort`.
Reckless matches in shape (§3.9 of Rarog's packet). Nothing changes in B.

## 4. Interaction map and shared signals

| Signal | Producer | Consumers |
|---|---|---|
| estimated score (TT-refined corrected eval) | B.2 (TT + correction) | razoring, RFP, quiet and capture futility, LMR `alpha − eval` term, NMP entry (B.3), ProbCut entry (B.3), singular margins (B.3), stand pat (B.4) |
| `improving`, opponent worsening | B.2 | RFP, count pruning, LMR table term, hindsight; NMP and ProbCut (B.3) |
| correction magnitude | B.2 | RFP, LMR; singular double/triple margins (B.3) |
| persisted `tt_pv` | B.2 (TT bit) | RFP veto, LMR terms, replacement, hindsight; NMP margin and singular depth (B.3) |
| `cutoffCnt[ply+1]` | B.2 | LMR; NMP (B.3) |
| `ss->reduction`, `ss->moveCount`, `statScore` | B.2 | hindsight, LMR, fail-low parent bonus, TT-cutoff penalty |
| histories (main, pawn, continuation 1–6, capture, low-ply, TT-move) | B.2 | picker order, count pruning, futility, history pruning, SEE thresholds, LMR; quiescence SEE (B.4) |
| history-adjusted `lmrDepth` | B.2 (LMR formula) | quiet futility, quiet SEE, capture futility, capture SEE |
| threat sets of the side not to move | board producer (B.2 adds it) | quiet scoring escape/threatened terms; RFP threat term is Reckless-only and not adopted |
| singular score, TT-move score | B.3 | LMR margin term (slot 0 until B.3) |
| optimism | B.5 | corrected eval (slot 0 until B.5) |

Feedback loops every registration names: history trains ordering which
sets `lmrDepth` which prunes (a history change is a pruning change); the
estimated score sets margins and the residual (§2.4) sets how far they can
be trusted; TT admission decides which nodes are searched at all, and the
TT-cutoff bonus trains on nodes never searched; the re-search rule trains
the post-LMR bonus which orders the next iteration; razoring and quiescence
decide what a shallow node can see (§2.7). B.2.2 reports activation and the
cutoff composition per mechanism, because a "hygiene" history change can be
a disguised selectivity change (Rarog RAR-S59).

## 5. Cluster contents, confirmed and changed, and the board producer

**B.1 (behaviour-neutral restructure)** — A.6's move table unchanged.
Removes `time_limit_`, `RootMoveStat`, the seven inert coordinates and
`KBNK Drive` (A.2.3); corrects the two stale comments; introduces `NodeType`;
keeps killers, countermove, low-ply history and every live mechanism exactly
(all fingerprint-bearing). Exact 14,978,465 at every commit; pooled-PGO NPS
within noise (A.6's floor).

**B.2 (selectivity core)** — PLAN's list stands, with these changes:
- razoring in the donor's return shape, seeded from the oracle column (depth
  1, 256 cp), the depth cap categorical;
- RFP in SF 19's shape with the TT-move condition and the blended return;
- the persisted `tt_pv` bit (one age bit) and the four TT-cutoff rules;
- SF's correction form with continuation correction at plies 2 and 4, its
  admission and gravity; `improving` with in-check inheritance; opponent
  worsening; hindsight; eval-difference training;
- the picker's good/bad quiet split and `skip_quiets`; capture scoring by
  victim plus history; threat-aware quiet scoring; **killers and countermove
  dropped**; low-ply history kept (SF has it);
- the history set, maxima and the whole update policy (fail-low parent
  bonus, TT-cutoff bonus, post-LMR bonus, decay, exact-node updates);
- `moveCount` semantics, count pruning as a picker skip, history-adjusted
  `lmrDepth`, continuation-history pruning, quiet SEE pruning, capture
  futility, fail-soft futility;
- the LMR formula, floor, negative-reduction allowance, `cutoffCnt`, the
  deeper/shallower re-search rule, `depth −= 3` after alpha rises, the
  fail-high blend;
- NMP, ProbCut, singular, the check extension, IIR, quiescence, root and
  time keep Basilisk's forms, re-wired to the new signals at their current
  values.

**B.3** — NMP (cut-node population, entry margin, dynamic reduction,
`nmpMinPly` verification), ProbCut (depth shape, small TT ProbCut, TT-value
guard), singular (margins, double/triple, negative −3, multi-cut training,
shuffling guard, `ttMoveHistory`), IIR against hindsight, the check policy.
**B.4** — as PLAN, with §3.8's numbers. **B.5** — as PLAN.

**The board producer B.2 needs.** SF's quiet scoring reads `attacks_by<PAWN>`,
`<KNIGHT>`, `<BISHOP>`, `<ROOK>` of the opponent and `check_squares(pt)`.
Basilisk's `Board` has `check_squares`; the evaluator computes attack maps
per evaluation but TT-hit nodes skip it. B.2.1 ticket 1 adds a lazily
computed per-node threat set from `attacks.cpp` primitives (no incremental
state, no board-footprint change), priced by B.2.2's pooled NPS; moving it
into `make_move` is a B.7 option. This is Rarog's §6.5 decision, re-derived.

## 6. Survivors with local evidence (operating rule 4)

| Mechanism | Evidence | Disposition |
|---|---|---|
| TT-bound pruning evaluation (BAS-S01, +7.18) | the donor's `estimated_score` | keep; identical |
| Exact-node reward-only history (BAS-S03), surprise scaling (BAS-S04) | +4.90, +2.50 in the 1.9.0 state | subsumed by the donor's policy (`statScore` term, eval-difference training, exact-node updates); not standalone coordinates |
| Dense 3×10 B TT (BAS-S05), publication incoherence (BAS-C05) | accepted | keep; one age bit re-allocated to `tt_pv` |
| Unconditional check extension (BAS-S08 −10.17 for removal; BAS-S16 −3.48 for a cap) | measured against this head's margins; §2.6 shows no fixed-node tactical value and a 1.9× node cost on both sides | keep through B.2; B.3 decides on the accepted B.2 head (BAS-S16's trigger fires there) |
| `singular_quiet_lmr` (BAS-D18, unmeasured +1.49) | no gate | subsumed by SF's `ttPv`/singular-margin LMR terms in B.3; B.2 keeps the flag at its current value |
| Aspiration refutations (BAS-D16, D17) | measured standalone | B.5 lands the donor's root cluster or nothing; no standalone retry |
| Killers, countermove | no standalone evidence; absent in the donor; ordering strength is TT/capture-driven (BAS-D01) | dropped with B.2 (rule 3: no standalone gate; a categorical paired run only if the review asks) |
| Capture futility, quiet SEE, qsearch quiet checks, post-LMR nudge (A.2.3) | dead or inert; the first two broke KBNK at their literature seeds because `lmr_depth` lacked the history term | B.1 removes; B.2 lands the first two and the fourth in the donor's form on the history-adjusted `lmrDepth`; quiet checks are B.4's switch |
| `DoubleExtMax` 16 | live bound, never binding on the suite | keep; B.3 |
| History pruning at `−14004·d` | dead (590 of 11.3M) | replaced by continuation-history pruning in B.2 |
| Razoring at 243·d, depth ≤ 3 | **−51 WAC at 100k against off** | re-seeded from the oracle column in B.2; B.0.1 prices it in Elo first |

## 7. Scale conversion and the three-column seed rule

### 7.1 Conversion factors

| Constant class | Stockfish 19 → Basilisk | Oracle → Basilisk | Notes |
|---|---:|---:|---|
| evaluation units (margins, deltas, correction bounds) | **0.282** (per-position median 0.293; phase 0.26–0.37) | **0.485** (unit conversion proven in play; measured magnitude ratio 0.557) | §2.4 |
| SEE / material units | **0.38** (pawn 0.48) | 0.38 pieces, 0.81 pawn | Basilisk 100/300/300/500/900 |
| plies, counts, 1024ths, lerp weights | 1 | 1 | |
| history units | SF maxima 7183 / 10692 / 30000 / 8192 are adopted with the tables, so 1 | — | Basilisk's 16384 maxima retire with the tables |

### 7.2 Seed rule (research decision; B.2.3's ranges span all three columns)

1. Plies, counts, 1024ths, lerp weights and history units: the Stockfish 19
   value unchanged.
2. Evaluation-unit constants: when the Stockfish-converted value lies more
   than 2× outside the range spanned by the oracle-converted and the
   Basilisk-fitted values (razoring, RFP, quiet and capture futility, the
   quiescence base, the NMP margin for B.3), seed the **geometric mean of the
   oracle-converted and Basilisk-fitted values**, or the oracle value alone
   where Basilisk has no fitted coordinate; this is Rarog's rule 2 with the
   oracle column first because of §2.4's residual.
3. Otherwise the Stockfish-converted value.
4. (Amendment 3, §22.2) An evaluation difference multiplied into another
   unit is scaled into the donor's units, × 1000 / 282, before the donor's
   constants; a compared constant converts by rule 3. (Amendment 4, §24.2) A
   scaled term fed by a bounded table keeps the donor's bound on the scaled
   product: scale, then clamp at the value the donor's own limit allows.
5. (Amendment 5, §27.1) A material value added into a history-unit sum is
   the donor's `PieceValue` (rule 1); a history-unit score turned into a SEE
   threshold converts to Basilisk's material units (× 0.38).

### 7.3 Principal seeds

| Constant | SF 19 | ×0.28 / ×0.38 | Oracle | ×0.485 / ×0.38 | Basilisk fitted | Seed (rule) |
|---|---|---:|---|---:|---|---|
| Razoring | `482·d²`, no cap, return qsearch | 136·d² | 527 at `d = 1` | **256, depth 1** | 243·d, depth ≤ 3, `q ≤ alpha` | **256 at depth 1** (oracle; the cap is categorical, B.0.1) |
| RFP per-ply, improving, worsening | `mult = min(45 + 4d, 85) − 20·!ttHit`; improving `2789/1024·mult`, worsening `335/1024·mult` | 13 + 1.1d; 35; 4 | `227·(d − improving)`, depth < 6 | 110·d; 110 | 160·d; 72; depth ≤ 9 | **133·d − 89·improving − 11·worsening**, cap 9 (2) |
| RFP correction term, blended return | `|corr|/198435`; `(661·beta + 363·eval)/1024` | shape | — | — | returns eval | correction coefficient seeded 0 (coordinate); SF's blend (1) |
| Capture futility | `234 + 247·lmrDepth + value + 134·captHist/1024`, `lmrDepth < 8` | 66 + 70·l | `267 + 391·lmrDepth + value`, `lmrDepth < 6` | 129 + 190·l | dead (198 + 283·l) | **92 + 115·lmrDepth + value + captHist/27**, cap 7 (2) |
| Capture / check SEE | `−(177·d + 34·captHist/1024)` | −67·d | `−202·d` | −81·d | −73·d, depth ≤ 8 | **−74·d − captHist/80**, no cap (2) |
| Continuation-history pruning | `cont1 + cont2 + pawn < −4136·d` (30000-scale) | −4136·d | countermove pruning at `lmrDepth < 4` | — | dead | −4136·d on the donor's tables (1) |
| Quiet futility | `119·lmrDepth + 90·(eval > alpha) + 164`, `lmrDepth < 12`, fail-soft | 34·l + 25 + 46 | `284 + 188·lmrDepth`, `lmrDepth < 6` | 138 + 91·l | 180 + 128·d, depth ≤ 6 | **158 + 108·lmrDepth + 25·(eval > alpha)**, cap 8 (2) |
| Quiet SEE | `−23·lmrDepth²` | −8.7·l² | `−(29 − min(l, 17))·l²` | −(11.6 − 0.4l)·l² | inert (25·l²) | **−10·lmrDepth²** (2) |
| Count pruning | `(3 + d²)/(2 − improving)`, skip quiets | same | same | same | `3 + d²` / `2 + d²/2`, depth ≤ 6 | SF (1), no cap |
| LMR table and terms | `2872/128·ln(i)` products, +982, the terms of §3.7 | unchanged | `(24.8 + ln threads)·ln(i)`, +570, ±1–2 ply terms | — | `0.60 + ln·ln/2.09` | SF (1); `alpha − eval` clamp bounds −18..27 cp |
| Re-search thresholds | `+53 / +8` | 15 / 2 cp | — | — | — | 15 / 2 (eval units, 3) |
| History bonus / malus | `min(133d − 81, 1487)`, `min(968d − 235, 2244)`, scalings of §3.6 | unchanged | `17d² + 133d − 134` | — | `min(62d²/64 + 120d, 1863)` / 1304 | SF (1) |
| Correction weights, bonus clamp | 15341 / 10569 / 12906 / 8761 over 131072; clamp ±256 at limit 1024 | unchanged (eval-unit output scales with the raw eval) | — | — | equal weights /5, ±1024 | SF (1); output bounded by the limit |
| TT replacement, age | `depth + 2·pv > old − 4`; 5-bit age | — | — | — | `depth < old − 3` refusal | keep ours; age 5 bits (B.2) |
| NMP, ProbCut, singular margins | §3.4–3.5 | — | §3.4–3.5 | — | current | B.3 |

## 8. SPSA surfaces

**B.2.3** (expected 46 live coordinates; ranges span the three columns;
generated from the X-macro so defaults cannot drift, A.3.2):

- node margins (9): razoring margin; RFP per-ply, improving, worsening,
  correction coefficient, depth cap, blend weight; IIR depth floor (kept
  mechanism); TT-cutoff bonus slope.
- move-loop pruning (13): count-pruning base and square term; quiet futility
  base, slope, `eval > alpha` term, cap; capture futility base, slope,
  history divisor, cap; continuation-history pruning slope; quiet and
  capture SEE coefficients.
- LMR (16): table scale and offset; `!improving` term; `ttPv` five; cut-node
  two; TT-capture; `cutoffCnt` three; `statScore` scale; `alpha − eval`
  scale; all-node scale; re-search deeper/shallower thresholds.
- histories (6): quiet bonus cap and slope, malus cap and slope, malus
  decay, main-history decay per search.
- correction (2): update slope, continuation weight.

**Categorical, settled by paired runs and never SPSA coordinates:** the
razoring depth cap (B.0.1 prices the current reach; B.2.2 settles the
cluster's), the direct-check exemption from count pruning, the LMR floor and
negative-reduction allowance, killer removal, the TT-cutoff admission rule,
the correction admission rule.

**Curvature sweep (PLAN rule 8) coordinates, classification frozen before the
first point:** RFP per-ply term, count-pruning square term, quiet futility
base, LMR table offset (982), correction update slope. Flat or monotone on
all five skips B.2's SPSA.

**B.3**: NMP entry margin (3), reduction (3), verification threshold; ProbCut
margin (2), depth shape (2), small TT ProbCut margin; singular margins (8);
IIR switch categorical. **B.4**: quiescence futility base, move-count limit,
SEE threshold, stand-pat and fail-high blend weights, quiet-check cap
categorical. **B.5**: aspiration (4), optimism (2), with D.1's multipliers.

## 9. Counters and the ablation mask

The 57 A.5.3 counters keep their names and units (the cross-engine contract
of `run_suite.py`). B.2.1 adds, with denominators: `rfp_cuts` and
`razor_cuts` by depth bucket (1–3, 4–7, 8+); `lmr_floor_hits`, `lmr_extended`
(negative reduction), `lmr_research_deeper`, `lmr_research_shallower`;
`hindsight_up`, `hindsight_down`; `skip_quiets_nodes`; `corr_cont2_updates`,
`corr_cont4_updates`; `tt_cutoff_quiet_bonus`, `tt_cutoff_graph_refused`;
`cont_hist_pruned`, `capture_futility_pruned`, `quiet_see_pruned`,
`capture_see_pruned` (the move-loop family split, with the moves tested as
denominator); `tt_pv_nodes` keeps its name and counts the persisted bit.
`hist_below_*` retire with history pruning. `cutoff_count` in the trace gets
its producer.

`AblationMask` stays through B.9. B.2.1 re-declares bit 0 on the new razoring,
bit 1 on the new RFP, bit 5 on the new move-loop family (count, futility,
continuation-history, SEE on both branches) and bit 7 on the new LMR; bits
2, 3, 4 and 6 are unchanged until B.3. The oracle's bits are fixed; the
liveness proof (A.5.4) is re-run on the on arm.

## 10. B.2.2 screens — registered numbers

Baselines are today's; B.1 must reproduce every zero-game number exactly
except NPS. Thresholds are screens, never acceptance (PLAN rule 8); the
paired run governs.

| Screen | Instrument | Baseline | Reference | Floor (ablate below) | Target (proceed above) |
|---|---|---:|---:|---:|---:|
| Geometric branching 4–14, suite_v2 | `branching.py` | 1.767 | oracle 1.757 | **window [1.65, 1.90]** | inside; outside either edge is a defect flag |
| Nodes at depth 12 relative to the oracle | same run | 3.97 (per-position median 2.77) | 1.0 | ≤ 3.5 (median ≤ 2.6) | **≤ 2.5** (median ≤ 1.8) |
| Shallow-pruning marginal factor at depth 12, Basilisk ablate | mask 32 | 2.20 | oracle 4.55 | ≥ 2.0 | ≥ 3.5 |
| WAC solved at 100,000 nodes | `fixed_budget_probe.py nodes 100000` | 204 | 241 | **≥ 200** | **≥ 225** |
| WAC solved at 400,000 nodes | same | 242 | 272 | ≥ 240 | ≥ 258 |
| LMR-off WAC delta at 100k | mask 128 | +15 | oracle −5 | ≤ +12 | ≤ +5 |
| Median depth at 300k, suite_v2 | `run_suite.py --nodes 300000` | 14 | 18 | monitored | monitored; must not rise while WAC falls |
| Oracle best-move agreement at 300k | same run | 66/105 | — | ≥ 60 | ≥ 75 |
| Time to depth, `bench 13` | bench nodes / pooled-PGO median NPS (interleaved) | **3.63 s** (14,978,465 / 4.127M) | — | ≤ 1.10× (4.0 s) | ≤ 0.85× (3.1 s); pooled NPS reported beside it |
| Canaries | `canary.py check` | 77/77 | oracle anchors | all 77 pass | all pass; new passes recorded, WAC.001 recorded |
| 2,000-game paired run against the B.1 head, seeds unfitted | Colosseum, no adjudication | 0 | — | **> −40 Elo** (below is a defect) | **≥ +10** proceeds to B.2.3; −40..+10 the review decides after the family screen |

**Decision trace** (B.2.1 ticket 0): the A.5.3 trace gains `cutoff_count`,
`lmr_depth`, `skip_quiets` and the family that pruned, and is run on
WAC.001, 085, 203 and 253 before B.2.2.

## 11. Frozen predictions and the two registered game tests

**B.0.1 — razoring depth reach, Elo layer** (registered BAS-S17,
maintainer-run, about 25 minutes). `RazorCoeff=500` against `243` on the
Tune binary `basilisk-b0-tune-pext-tune-pgo.exe`, one binary two option
sets, 2,000 paired games at `3+0.03`, no adjudication, the A.3.1 run-file
path. Prediction: **+8 Elo**, 80% interval [−4, +20]; probability the point
estimate is positive 70%; confidence moderate. Reading rule (frozen): a 95%
interval wholly above 0 confirms the reach as a defect and the oracle column
seeds razoring without a categorical re-test in B.2.2; an interval wholly
below 0 says the tactical cost is compensated in play, 243 stays a column and
B.2.2 settles the cap; otherwise the cluster decides. The `hcefinal` SPSA
fitted 243 with 500 as its range maximum, which is the main counter-argument.

**B.0.2 — the move-loop family in Elo** (registered BAS-S18, maintainer-run,
about 35 minutes). The oracle with mask 32 (its shallow move-loop pruning
off) against Basilisk 1.10.1 at equal time, the BAS-O05 recipe with 1,000
cycles (2,000 games), `Use Basilisk HCE=true`, Hash 64, no adjudication,
reading G(32) = 1500 − Basilisk's rating. Prediction: **G(32) = +110**, 80%
interval [+40, +180] (the family explains about 200 of 312.6); confidence
moderate. Falsifiers: G(32) ≥ +250 says the family explains under 60 Elo and
the node attribution does not carry Elo, which lowers P2 below +20 and
re-opens the cluster order; G(32) ≤ 0 is inconsistent with 2.6's node ratio
and points at the instrument.

**Calibration of the two game tests (2026-10-08, maintainer-run).**
B.0.1 (BAS-S17) read **+2.4 ± 9.9 Elo** (95% [−7.5, +12.3], 1,000 pairs, 0
faults): inside the 80% interval, 5.6 below centre. The interval holds 0, so
neither branch of the reading rule fires. Razoring's depth cap stays
categorical for B.2.2's own paired run, with the oracle column (depth 1,
256 cp) and 243·d at depth ≤ 3 both in range. The 32 WAC positions that
`RazorCoeff=500` recovers at 100k nodes read near zero in Elo at STC.
B.0.2 (BAS-S18) read **G(32) = +102.3 ± 15.9** (2,000 games, 0 faults):
inside [+40, +180], 7.7 below centre. G(0) − G(32) = **+210.3 ± 23.9**: the
oracle's shallow move-loop pruning family is worth about two thirds of
its 312.6 lead over Basilisk, which matches §2.6's node attribution. The
ablate oracle at mask 0 searches G(0)'s tree exactly (967,078 nodes) at the
same NPS within 1%, so the difference compares like with like. Neither
falsifier fires; P2 and the cluster order stand.

**B.2 (frozen before B.1 exists; calibration appended at B.2.4):**

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, §7 seeds) reads **−15 ± 30 Elo**; probability of the −40 defect stop 20% | moderate | below −40 is a defect or a wrong seed column |
| P2 | After B.2.3 and the `[0,10]` gates B.2 reads **+40 Elo at STC**, 90% interval [+5, +90]; H1 probability 65% | moderate | H0 or a stop under +10 refutes the coherent-core share; the residual then lies in B.3/B.4 |
| P3 | Nodes at depth 12 relative to the oracle fall from 3.97 to **≤ 2.5** (median ≤ 1.8) with branching inside [1.65, 1.90] | moderate-high | a ratio above 3.2 says the move-loop family did not land in the donor's population |
| P4 | WAC at 100k rises to **≥ 225** with razoring re-seeded and the LMR-off delta falls to **≤ +5** | moderate | WAC under 210 says the picker/LMR terms removed what razoring gave back |
| P5 | Pooled NPS lands at **0.92–0.98×** of the B.1 head (threat producer, larger histories) | moderate | below 0.90 sends the threat producer to B.7 before the gate |
| P6 | `lmr_researched / lmr_applied` rises from 1.7% to **3–5%**; `rfp_cuts` fall below 12% of interior nodes while the tree shrinks | high | neither moving means the floor or the estimated-score wiring did not land |

Programme stop rule: PLAN rule 6. B.0's own: if P2 fails and P3 held, the
coherent-core hypothesis is refuted for this evaluation and B.3–B.5 re-scope
to the smallest donor-shaped changes with measured activation; if P2 fails
and P3 failed, B.2 returns to `IMPLEMENTED` for a defect hunt.

## 12. Handoffs

### 12.1 B.1 — READY_FOR_IMPLEMENTATION (`I1`)

Scope: A.6's move table (tickets 1–8) with `NodeType`, `SearchConfig` /
`SearchShared` / `SearchState`, and the removals and comment corrections of
§5. Nothing in §3 lands here: killers, countermove, the LMR gate, histories
and every margin stay byte-identical in behaviour. Done criteria: exact
14,978,465 at every commit on PEXT and non-PEXT; release and sanitizer
CTest; search, threading, ponder, WAC and endgame tests; pooled-PGO NPS
within ±0.5% of BAS-P12; the §10 baselines reproduced exactly except NPS.
B.0.1 and B.0.2 use existing binaries and neither waits on B.1.

### 12.2 B.2 — READY_FOR_IMPLEMENTATION (`I2`), after B.1 and B.2.0

- Goal and semantics: §3.2–3.4, 3.6–3.7 and §5, seeds §7.3, behind one
  umbrella CMake option, off by default, the off arm at the exact fingerprint
  at every ticket.
- Why here: §2.6 (the move-loop family is the ×4), §2.4 (the oracle column
  seeds), §2.7 (razoring), §2.5 (nothing trains on fail lows, eval swings or
  TT cutoffs; `tt_pv` absent).
- Producer → state → consumer map: §4.
- Invariants: DESIGN §3 (mate band, TT ply adjustment and rule-50 semantics,
  TT-move validation at every consumer, one mutation seam); legal PV and
  best move; nothing pruned or reduced at the root, in check, or before one
  move is searched (Amendment 3 §22.1: except the donor's Step 19
  step-down of the first move at a non-PV node); the reduced depth at least
  one ply; direct quiet checks
  survive the count skip unless the categorical run says otherwise;
  correction never trains from in-check, capture-best or bound-disagreeing
  nodes; the TT stores the raw static eval only; the KBNK/KQK/KRK/KBBK and
  mate-band CTest floors bind the on arm.
- Ticket order: 0 trace fields and the §9 counters; 1 board threat producer
  and the stack fields (`moveCount`, `cutoffCnt`, `statScore`, the persisted
  `tt_pv`); 2 TT miss-store, node-typed cutoff, graph-history check, rule-50
  guard, cutoff bonus; 3 histories and picker (killers and countermove go
  here); 4 correction form and admission, `improving`, worsening, hindsight,
  eval-difference training; 5 razoring and RFP; 6 move-loop pruning; 7 LMR,
  floor, re-search rules, `depth −= 3`, fail-high blend; 8 update policy and
  decay. Each ticket keeps the off-arm fingerprint; unit tests cover table
  bounds and gravity, picker exhaustiveness and the quiet split, TT store and
  probe with the new bit, stack unwind, the threat producer against a slow
  reference.
- Cheap qualification: §10's zero-game rows on the on arm; the liveness
  proof on the re-declared bits; the decision trace on the four positions.
- Maintainer gate: the 2,000-game paired run, then B.2.3 and B.2.4 as PLAN.
- Non-goals: NMP, ProbCut, singular, the check extension, IIR, quiescence,
  root, time (re-wired, not changed); no adjacent heuristics; no constant
  outside §7.3 moved by hand after B.2.2's screen without a recorded reason.
- Register B.2.2–B.2.4 in `EXPERIMENTS.md` with P1–P6 copied verbatim before
  the first game.
- Amended by §17 (Amendment 1, 2026-10-10): ticket 3 opens with the `see_ge`
  quiet-move commit; razoring above depth 1 is linear; P5 re-registered.

### 12.3 B.3 — handoff frozen, contingent on the accepted B.2 head (`I2`)

Semantics §3.4 (NMP, ProbCut, IIR against hindsight) and §3.5; seeds §7.3's
last row at B.3.0, re-based on B.2.2's `null_*`, `probcut_*` and `sing_*`
readings; bracket `[0,3]` as PLAN registers; the check policy is decided
there with BAS-S16's trigger met.

### 12.4 Re-anchoring on the A.8 head (A.8.20, 2026-10-08)

Phase A's step A.8 landed between this packet and B.1.
- **Fingerprint:** unchanged, 14,978,465 at every A.8 commit.
- **Line ranges:** A.6's packet carries the re-anchored map.
- **§10's fixed-node baselines** (branching, WAC at fixed nodes, counters)
  are expected to reproduce: A.8's search changes act only with tablebases
  configured (A.8.14), under a clock (A.8.17), or at MultiPV > 1 (A.8.9), and
  none of those is in the fixed-node instruments. They were not re-run here,
  because the host was busy; B.1's done criteria re-run them.
- **Parsers:** the instruments' `info`-line parsers (`branching`,
  `ablation_liveness`, `run_suite`, `scale_ratio`, `fixed_budget_probe`) read
  fields by name. Checked on A.8.8's new line shape, including a bound line,
  which `fixed_budget_probe` excludes. Bound lines appear only after 3 s of
  search, beyond the instruments' budgets.
- **`cp`:** stays in internal units (BAS-C14), so `scale_ratio`'s 0.282 stands.
- **NPS:** BAS-P13 read the A.8.9 head +1.13% against BAS-P12, unexplained,
  so B.1's NPS criterion ("within ±0.5% of BAS-P12") is re-based on
  **BAS-P15**, the pooled baseline of the A.8 head.
- **B.0.1 and B.0.2:** their registered binaries are unchanged files
  (`basilisk-b0-tune-pext-tune-pgo.exe` `D99C8425…7563`,
  `oracle-1.10.1-ablate.exe` `0E5155CC…8644`, both re-hashed), so neither
  registration moves.

### 12.5 B.1's reproduction of §10 (2026-10-08)

On the B.1 PGO builds of `bf44834` (`tools/results/b1-qual/`) every §10
zero-game baseline reproduced, position by position, with one exception that
is A.8's: `suite_v2`'s mated root (Fool's mate) was searched once per
iteration by the B.0 binaries (1 node per depth) and since A.8.8 is reported
without a search. It accounts exactly for the differences in the counters
(12 nodes at depth 12, 14 at depth 14), the oracle differential and the
ablate profiles. Seven WAC positions whose final iteration is a found mate
report slightly different node totals than B.0's binaries (WAC.298 86,914
against 86,875); the A.8 head reports exactly B.1's on all 300 positions, so
that change is also A.8's. Solved counts, depths and best moves are
unchanged: WAC 204 / 242, branching 1.767, agreement 66/105, canaries
77/77. A.8.20's expectation that A.8 left these instruments untouched held
for every position but those two classes. NPS (BAS-P16): B.1 reads +1.21%
against the A.8 head, so B.2's prediction P5 is read against B.1's 4.216M
pooled median.

## 13. The four questions, answered for the programme

**Mechanism.** Basilisk's tree is a constant ×4 the oracle's from depth 4 on
at equal per-ply growth; the multiplier is the shallow move-loop pruning
family, which prunes on raw depth and dead history where the oracle prunes
on a history-adjusted depth with continuation-history, quiet SEE and capture
futility pruning; its razoring at depth 2–3 and its reductions cost tactics
at a fixed budget where the oracle's cost none. The strength property is
*better-informed pruning and reduction decisions*, not more of them.
**Interactions.** §4. **Invariants.** §12.2 and DESIGN §3. **Falsifier.**
P1–P6, the −40 stop, BAS-S17 and BAS-S18's reading rules, and PLAN rule 6.

## 14. Not done, corrections and tooling findings

- `tools/diag/suite_v1.epd` position 103 (`1r6/8/8/8/8/8/1K6/kR6 w - - 0 1`)
  is illegal (the black king is attacked with White to move); the 1.10.1
  board rejects it and exits under the BAS-C12 contract, which aborts
  `branching.py` and `run_suite.py`. `suite_v2.epd` is v1 without it (106
  positions); v1 stays frozen for its historical readings.
- `canary_v1.json` records the SHA-256 of a CRLF copy of `src/wac.epd`
  (built on the notebook); on an LF checkout `canary.py check` refuses the
  suite. Today's check ran against a CRLF copy. The next leaf that touches
  the checker should hash normalised content.
- `tools/build_test.ps1` invoked through Windows PowerShell 5.1 leaves
  `$PSScriptRoot` empty in its parameter default and writes to
  `D:\test_engines`; `pwsh` is correct. Both B.0 binaries were moved and their
  manifests corrected by hand.
- A.5.4's artifacts (`tools/results/a54-ablation-liveness.json`, the oracle
  ablate binary) are not on this host; the oracle ablate build was
  reproduced here with `build_oracle.ps1 -ExtraPatch`. The snapshot's
  `oracle-hybrid-diag.patch` conflicts with the ablate patch; the oracle's
  counters were not re-run and are not needed by this packet.
- BAS-D05–D08's 1.9× node ratio was a 16-position reading; the 103-position
  profile says about 4×. The ledger rows stand as recorded; the number used
  from now on is today's.
- BAS-D02's "reductions are far too conservative" is withdrawn by §2.6.
- The Stockfish pin is the `sf_19` tag `edb0d9db`; PLAN's `0a215d6c` default
  is superseded (PROCESS updated).
- Nothing here is Elo.

## 15. Artifacts (`tools/results/b0-20261006/`, ignored storage)

| File | Content |
|---|---|
| `branching.json/.log` | §2.1, three arms, suite_v2 (SHA-256 `33f3b603…`) |
| `suite_d14_diff300k.json`, `counters_d12.json`, `counters_bench13.json` | §2.5, §2.2 |
| `wac_nodes100k.json`, `wac_nodes400k.json`, `wac_depthpv10.json`, `wac_depthpv12_basilisk.json`, `canary_check_a74.json`, `wac_canary_suite.epd` | §2.3 |
| `scale_ratio.json`, `scale_corpus.csv` | §2.4 |
| `trace_wac{007,085,203,253}.json` | §2.3 traces |
| `ablate_branching_mask*.json`, `ablate_wac100k_mask*.json`, `oablate_*` | §2.6 |
| `qscheck6_*.json`, `razor500_*.json`, `razor400_wac100k.json` | §2.7 |
| `frozen_predictions_screens.md` | S1–S6 as frozen |

Binaries (`tools/test_engines/`, SHA-256): `basilisk-a74-nps-pgo1-pext-pgo.exe`
`d3abd5fb…0b0e48`; `basilisk-b0-tune-pext-tune-pgo.exe` `d99c8425…7563`;
`basilisk-b0-ablate-pext-ablate-pgo.exe` `63f96c73…1717`;
`oracle-1.10.1.exe` `5f77850a…0725`; `oracle-1.10.1-ablate.exe`
`0e5155cc…8644`; Stockfish 19 universal `45bc8e49…bcbd0`. Texel tool
`build/b0-texel/basilisk-texel.exe` from `6c5ee63`.

## 16. B.2.1 Return 1 (2026-10-10): tickets 0–2 built, the quiet-SEE premise false, leaf returned to `RESEARCH`

- State / class: B.2.1 (`I2`) stopped at the committed ticket-2 boundary and
  returned to `RESEARCH`. Research amendment 1 decides before ticket 3. No
  game, NPS or time-to-depth measurement was run.
- Head: `f179404` on `dev`, from `bef0e33`.

### 16.1 What landed (the working record)

| Ticket | Commits | Content |
|---|---|---|
| 0 | `1dbf0d9` tools, `61c15cb` engine | 22 counters after `lmr_blocked_gives_check` (79 in all) on two new `kv` lines and one prose line; trace version 2 (`lmr_depth`, `skip_quiets`, `family`, a `skip_quiets` event; the header states `cutoff_count` availability by arm); `decision_trace.py` reads versions 1 and 2; `run_suite.py` keeps the 57 as the cross-engine core and validates the 22 as a Basilisk-only group when present |
| 1 | `32e56ad` engine, `34a6d33` CI | `threats.h/.cpp` in the board library on both arms, with `test_threats` (10,000 random-walk positions against `attackers_to` and `is_square_attacked`, both sides, 0 mismatches); `STACK_SENTINELS` 7 on both arms; the ON arm's `SearchStack` with `move_count`, `cutoff_cnt` (produced as the donor's), `tt_hit` and `in_check`; `search_kernel_core.cpp` forked from the legacy kernel; the ON `id name` suffix `+b2core`; a CI step that builds and tests the ON arm |
| 2 | `54ff32d` tools, `e97a3cd` test, `f179404` engine | the ON TT encoding of review §5.6 (pv bit 2, five age bits, depth offset 2 with occupancy as a non-zero depth byte, evaluation-only entries with `NO_SCORE`, the rule-50 downgrade); the miss-store; the persisted `tt_pv` and its fail-low propagation; the node-typed cutoff (`CoreTtCutoffNodeTyped`), the graph-history check at depth ≥ 7, no cutoff at rule-50 ≥ 96; the quiet TT-move bonus (`CoreTtCutoffBonusSlope` 131) and the parent's early-quiet malus (−2210), on the legacy tables until ticket 3; the categorical-switch table (`S` lines: tune options, never on the SPSA surface); the SPSA generator skips the core table, so the registered surface is still 41 |

Fingerprints at every commit: the off arm's `bench 13` = 14,978,465 on
release PEXT and plain, Tune, Diag and Ablate, and its sanitizer `bench 10`
= 3,190,673. The ON arm's `bench 13` = 14,978,465 at ticket 1, which proves
the fork reproduces the legacy kernel exactly, and 13,488,940 at ticket 2 on
PEXT and plain; its release and sanitizer builds agree at `bench 10` =
3,445,675. Release CTest 20/20 on both arms, the WAC and endgame-conversion
floors included; sanitizer CTest 18/18 on both. Ticket 0 had no ON arm to
test (16.4, item 8).

`e97a3cd` fixed a test precondition in its own commit. The won-ending clock
test in `test_engine_threading` asserted the legacy search's move `f7d7`.
6-man Syzygy WDL scores 26 of the 28 legal moves as wins; only `f7f8` and
`f5h3` draw. The test now rejects those two. The ON arm plays `e5e6`, a win,
in 1.2 s.

### 16.2 The failed premise

The clauses are §3.6's quiet scoring term `16384·(check && SEE ≥ −75)`
(review §5.7, §5.8) and §3.7's quiet SEE pruning `−23·lmrDepth²`, seeded at
`−10·lmrDepth²` (§7.3; review §5.10, `CoreQuietSeeCoeff`). Both assume that
the SEE of a quiet move measures what the opponent wins by capturing the
moved piece, as the donor's `see_ge` does.

Basilisk's `Board::see_ge` returns `0 ≥ threshold` for a move that is
neither a capture nor a promotion (`src/board.cpp`: `if (!is_capture && mt
!= PROMOTION) return swap >= threshold;`). It never looks at the
destination. Every threshold the two clauses use is ≤ 0, so both always pass:
the quiet SEE pruning would be dead code and the check bonus ungated.
`Board::see()` does evaluate quiets (gain 0, then the exchange), but it is
the ordering-only approximation that truncates deliberately, and its own
comment forbids using it for pruning.

Probe: `tools/results/b21-20261010/see_quiet_probe.cpp`, with its output
beside it (SHA-256 `77a05d16…6730` and `14387fce…ba0e`). It ran 4,000
random walks of 0–29 plies from five seed positions, built with
`clang++ -O2` against the head's board sources.

- 120,534 quiet moves; `see_ge` is false for none of them at any threshold.
- The exchange `see()` falls below the threshold for 28.75% of them at
  `−10·l²` for l = 0–3, 24.43% at l = 4, 19.30% at l = 5 and 9.98% at l = 6.
- Of 2,351 quiet checks, 1,633 (69.5%) lose more than 75 by `see()`.

The legacy kernel calls `see_ge` only on captures and promotions (qsearch,
ProbCut, capture SEE pruning, the LMR bad-capture test, the picker's
bad-capture split), so the shortcut has never mattered. The legacy quiet SEE
pruning that B.1 removed called the same `see_ge` and could not have fired
either.

Stopped: the picker (ticket 3) is not written, since its check term is one
of the two clauses. Kept: everything in 16.1, and the probe.

### 16.3 Options

- (a) Give `see_ge` the donor's semantics for quiet moves: the moved piece
  becomes the first piece at risk on the destination, through the same exact,
  pin-aware exchange loop. The off arm never calls it on a quiet move, so it
  should keep 14,978,465 (to be verified), and `test_board`'s independent SEE
  oracle extends to quiets. The cost is SEE work on checking quiets in the
  picker and on quiets that reach SEE pruning; B.2.2's NPS screen prices it
  inside P5.
- (b) A core-only primitive (a `see_ge` for quiets, or `see()` despite its
  truncation) that leaves `see_ge` as it is.
- (c) Drop both uses on the ON arm: the check bonus ungated and no quiet SEE
  pruning. This departs from the donor's family population, on which §2.6
  and P3 rest.

Recommendation: (a). It is the donor's primitive and a change to one board
function that has an exact oracle; the off arm is untouched by
construction; and both clauses then mean what the contract wrote. The
counter-argument is cost: SEE on quiet moves is new per-node work, and P5
already budgets 0.92–0.98× for the threat producer and the larger tables.

### 16.4 For the same amendment (the implementation's reading, applied unless amended)

1. The TT-cutoff bonus `131·depth` (§3.2, seed 131) is the later Stockfish
   commit `e52ea9ac` ("Simplify quiet history bonus"); `sf_19` trains
   `min(112·depth, 695)`. Ticket 2 implements the contract: 131·depth,
   uncapped.
2. The review's stack table names RFP's `!ttHit` multiplier as a `tt_hit`
   consumer, but §3.4's adopted shape and §7.3's seed have no `ttHit` term
   and no coordinate. Ticket 5 implements §7.3's form without it.
3. The donor's quiet branch runs when `!followPV || !PvNode`, and `followPV`
   needs the previous iteration's PV per ply, which the record does not
   carry. Ticket 6 prunes quiets at non-PV nodes only.
4. Evaluation-unit constants that §7.3 does not list convert by rule 3 at
   ×0.282: hindsight's 166 becomes 47, and the fail-low bonus thresholds 106
   and 68 become 30 and 19. The eval-difference training constants stay at
   the donor's values, as the review fixes them.
5. Above depth 1 (`CoreRazorDepthCap` 2–3), razoring's margin takes
   `sf_19`'s `margin·d²`.
6. The donor's initial history fills (main −5, capture −742, pawn −1338,
   continuation −586, correction −5 and +5) are not adopted: the tables
   clear to 0, as review §5.11 tests.
7. Where the donor reads `PieceValue` (capture scoring, capture futility,
   the capture `statScore`, the threat term), the core reads Basilisk's
   search `PIECE_VALUE`.
8. A finding, not a decision: the ON arm did not compile at the B.2.0 head
   `bef0e33`. B.2.0 split the parameter tables, the ON arm selected the empty
   core table, and the legacy kernel it still compiled reads legacy
   coordinates. Review §2.1's ON-arm row was measured before the split.
   Ticket 1 aliases the legacy table into the core table until each
   mechanism is replaced.

Resume: on research amendment 1, at ticket 3 (histories and the picker), on
the amended contract.

## 17. B.2.1 Amendment 1 (2026-10-10): `see_ge` takes the donor's quiet-move semantics; decisions 2–13 confirmed, 6 amended, P5 re-registered

Answers §16. Research (`R3`) at head `3ee6128`; implementation resumes at
ticket 3 on this contract. Every call-site claim below was re-read at that
head.

### 17.1 Decision 1: option (a)

**Decision.** `Board::see_ge(m, t)` evaluates a quiet move as the donor's
does: the moved piece is the first piece at risk on `to`, through the
existing exact, pin-aware exchange loop. Castling alone keeps `0 ≥ t` (the
donor answers that for every non-`NORMAL` move; a legal castling
destination is unattacked and the king never enters an exchange). Captures,
en passant and promotions are untouched: their verdicts at every threshold,
the BAS-C09 fixtures and `see()` stay byte-identical.

Mechanically it is the one early return. `if (!is_capture && mt !=
PROMOTION) return swap >= threshold;` becomes a castling-only return, and
what follows already is the donor's quiet semantics: `swap = 0 − t` is
negative when `t > 0` (a quiet move never gains), `swap = value(mover) −
swap` is non-positive when `t ≤ −value(mover)` (a quiet move never loses
more than the mover), and the loop then runs with the opponent to move
first on an occupancy without the mover, so a slider behind the mover's
origin sees through it.

The four questions:

1. Mechanism. The check gate and the SEE tests need to know whether the
   moved piece hangs, and a quiet SEE measures exactly that: the net of the
   best capture sequence the opponent can start on the destination,
   declining allowed, so always ≤ 0. Without it three ON-arm consumers are
   inert, not the two §16.2 names: the picker's `16384·(check && SEE ≥
   −75)`, the quiet branch's `−CoreQuietSeeCoeff·lmrDepth²`, and the
   capture/check branch's `−74·d − captHist/80` (§3.7), which the donor
   applies to quiet checking moves as well. One primitive repairs all three.
2. Interactions. The off arm is untouched by construction: its `see_ge`
   callers are quiescence and ProbCut over `gen_legal_captures`, the
   capture SEE pruning under `is_cap`, the picker's bad-capture split under
   `is_cap`, and the memoised `see_score` under `is_cap && !is_promo`
   (`src/search_kernel.cpp`, `src/move_picker.h`). The ON arm's consumers
   are the three above, none of which exists before ticket 3, so the
   `see_ge` commit itself changes no fingerprint on either arm. The BAS-C09
   approximations (a pin created mid-exchange; a recapturing pawn that
   promotes) become reachable from quiet calls with the same geometry;
   their retry trigger is re-read once in B.2.2 (17.4).
3. Invariants. `see_ge(m, t)` is monotone non-increasing in `t`; for a
   quiet it is true at every `t ≤ −PIECE_VALUE[mover]` and false at every
   `t > 0`; a capture or promotion verdict equals the head's at every
   threshold; `see()` is unchanged; the king never enters the exchange as
   the mover, because only legal moves reach SEE (generation is legal and
   the picker validates the TT move); the off arm's `bench 13` stays
   14,978,465.
4. Falsifier. The oracle test and the probe re-run of 17.5: a quiet whose
   `see_ge` boundary differs from `oracle_see` outside the BAS-C09
   geometries, or an off-arm fingerprint that moves, refutes the commit.

(b) is rejected because `see()` truncates by design and the kernel would
carry two SEE semantics for one question; (c) because P3's family
population is the donor's with these gates in it.

**Cost, priced.** Quiet SEE calls arise on the ON arm at the picker, for
checking quiets only (the `&&` short-circuits: 2,351 of 120,534 quiets,
2.0%, in the probe corpus), and in the move loop for quiets that reach the
SEE test at non-PV nodes after the count skip, continuation-history and
futility pruning. Implementation may add a behaviour-identical fast path:
after the two early exits and before `see_pins`, `if (!(attackers_to(to,
occ) & occ & occupancy[~side_to_move])) return true;`. The pin filter only
removes attackers, so the loop would break at its first iteration with
`result = 1`; the path applies to captures too, and the off arm's
fingerprint is the proof it is identical. A quiet move to an unattacked
square then costs one `attackers_to`. Do not pre-gate with the threat
producer's `all` bitboard: it is computed on the pre-move occupancy and
misses a slider attack opened along the mover's own line. P5 is amended in
17.4.

### 17.2 Decisions 2–8 (§16.4)

2. Confirmed: `131·depth`, uncapped, `CoreTtCutoffBonusSlope` 131 in 0–300.
   The two donor forms differ only from depth 6 (786 against 695); a cap
   would be a coordinate B.2.3 does not need.
3. Confirmed: §7.3's RFP form with no `ttHit` term. The review's stack row
   maps a donor read, not a contract clause. The term is a donor-fitted
   offset (−20 on a 45–85 multiplier) whose absence P6's `rfp_cuts` will
   show; if `rfp_cuts` miss P6 it is the first candidate coordinate for
   B.2.3 (`CoreRfpNoTtHit`, eval per ply, seed 0).
4. Confirmed: the quiet branch (continuation-history, futility, SEE) at
   non-PV nodes only. The donor's population is a superset (PV nodes off
   the previous iteration's PV); the stack carries no per-ply previous PV,
   and the legacy quiet pruning is non-PV already, so this is the legacy
   population with the donor's tests. A `followPV` bit on the stack is a
   B.3 candidate; no switch now.
5. Confirmed: hindsight 166 → 47; fail-low bonus thresholds 106 → 30 and
   68 → 19 (rule 3, ×0.282); the eval-difference training constants at the
   donor's values. Amended by §22.2: the clamp bounds and the offset are
   evaluation units, so the sum is scaled into the donor's units first.
6. **Amended:** above depth 1 razoring's margin is linear,
   `CoreRazorMargin·d` (256, 512, 768), not `margin·d²`. The switch exists
   to price the legacy reach (243·d: 486 and 729 at depths 2–3, which
   §11's calibration keeps in range) against the oracle's depth-1 form.
   `256·d²` gives 1,024 and 2,304 cp, which no razoring population reaches,
   so `CoreRazorDepthCap = 3` would be the depth-1 arm again and the
   2,000-game run would compare an arm with itself. The donor's `482·d²` is
   a different fit whose depth-1 value converts to 136; pairing the
   oracle's 256 with the donor's square double-counts the depth.
7. Confirmed: the tables clear to 0; the donor's fills are not adopted in
   B.2. What that changes: a never-updated quiet reads 0 rather than about
   −5,600 in the picker sum and about −1,280 in the LMR `statScore`, so the
   donor reduces fresh quiets by about 0.13 ply more and prunes them a
   little earlier through `lmrDepth += history/divisor`; the −14000 and
   −4136·d thresholds are not crossed by the fills alone. One categorical
   candidate (`CoreHistoryFills` 0/1) for B.2.3's residual list if P3 or
   P6 miss; no switch now.
8. Confirmed: search `PIECE_VALUE` {100, 300, 300, 500, 900} wherever the
   donor reads `PieceValue`; −75 and −v/18 kept as written. On this vector
   every SEE value is a multiple of 100, so `see_ge(−75)` on a quiet and
   `see_ge(−v/18)` on a capture (−5 … −50) are both exactly `SEE ≥ 0`: the
   donor's fractional thresholds buy nothing here, which is why neither is
   a coordinate. Likewise the capture/check SEE at depth 1 (−74) is
   `SEE ≥ 0`, and `CoreQuietSeeCoeff` 10 prunes any loss at `lmrDepth ≤ 3`,
   a loss ≥ 200 at 4, ≥ 300 at 5 and ≥ 400 at 6; the probe's 28.75, 24.43,
   19.30 and 9.98% are that reach on its corpus.
   Amended by §27.1: the three history-unit sums that add material take the
   donor's `PieceValue`, and the split's `−score/18` is not `SEE ≥ 0`; it
   converts to `−score/47`.

### 17.3 Decisions 9–13

9. Confirmed. Ticket 3's ON-arm fingerprint is transient (legacy shapes
   and ageing on the donor's tables until ticket 8); record it like the
   others.
10. Confirmed: `tt_capture` is capture-stage membership, captures and every
    promotion. Departure recorded: the donor's `capture_stage` is captures
    plus queen promotions; Basilisk's SEE scores every promotion through the
    exchange, so all promotions stay in the capture stage and the
    `singular_quiet_lmr` gate follows.
11. Confirmed: `CorePawnHistBucketsLog2` 11–13, default 13.
12. Confirmed: RFP and razoring inside the legacy block gate (`!PV`,
    `!in_check`, no excluded move). The donor's RFP in excluded searches is
    coupled to its singular form, which is B.3's; revisit there.
13. Confirmed: in-check nodes through the main picker; nothing pruned or
    skipped in check (§12.2's invariant).

### 17.4 Predictions re-registered (before any game)

- P5, substantive: the band 0.92–0.98× of the B.1 head stands; its stated
  sources are now the threat producer, the larger histories **and quiet
  SEE calls**. Reading rule added: B.2.2 reports `see_ge_calls` per node on
  both arms beside the pooled NPS; if P5 reads below 0.90 and the ON arm's
  `see_ge_calls/node` is more than twice the off arm's, quiet SEE is the
  suspect, the 17.1 fast path (if not taken) is the first repair, and only
  then does the cost go to B.7 with the threat producer.
- P1–P4 and P6: unchanged. Clerical amendments: none.
- BAS-C09's retry trigger (production `see_ge` verdict changes under
  per-ply pins above 0.1% of calls on the bench corpus) is re-read once on
  the ON arm in B.2.2 with BAS-C09's recipe; quiet calls were outside the
  population it measured.

### 17.5 Cheap qualification of the `see_ge` commit

- Neither arm's fingerprint moves: off `bench 13` = 14,978,465 (release
  PEXT and plain) and sanitizer `bench 10` = 3,190,673; ON `bench 13` =
  13,488,940 and sanitizer `bench 10` = 3,445,675. No consumer of a quiet
  SEE exists yet, so an unchanged count on both arms is the proof.
- `test_board`: the BAS-C09 fixtures and the capture consistency check
  unchanged and passing. New, on the `test_see` FEN list and a fixed-seed
  random-walk corpus, for every legal quiet move: `see_ge_boundary(m) ==
  oracle_see(m)` (`oracle_see` already scores a quiet: gain 0, then the
  exchange on `to` after `make_move`), with any divergence pinned by count
  as BAS-C09 pins its approximations; and `see_ge(m, −PIECE_VALUE[mover])`
  true and `see_ge(m, 1)` false for every quiet.
- The probe rebuilt against the new board sources, its output beside the
  first: the `see_ge false` column agrees with the `see < t` column except
  where `see()`'s truncation differs (report the count), and the check
  row's `see_ge(−75) false` count is near 1,633.
- Once ticket 6 lands, `quiet_see_pruned` and `capture_see_pruned` are
  non-zero on the ON arm at `bench 13`.

### 17.6 Handoff, amended

Ticket 3 opens with one board commit on both arms: the `see_ge` change of
17.1 with its `test_board` additions and the probe re-run, qualified as
17.5. Then the tables, the picker and the update routing as §16.4 item 9.
Ticket 5 implements razoring above depth 1 as 17.2 item 6. Ticket 6
implements the quiet branch at non-PV nodes and the capture/check branch's
SEE test on quiet checks. §12.2's ticket order is otherwise unchanged.

## 18. B.2.1 Return 2 (2026-10-10): ticket 3's `see_ge` commit landed; the picker fails the ON arm's WAC floor; leaf returned to `RESEARCH`

- State / class: B.2.1 (`I2`) stopped after ticket 3's first commit and
  returned to `RESEARCH`. Research amendment 2 decides before the rest of
  ticket 3 lands. No game, NPS or time-to-depth measurement was run.
- Head: `60a610e` on `dev`, from `ae28411`.

### 18.1 What landed: the quiet-move `see_ge` (17.1, 17.5)

- `ea5c49f` (engine): castling-only early return; a quiet move goes through
  the exchange loop with the mover first at risk. The 17.1 fast path is
  taken: when no piece of the other side attacks `to`, return true before
  `see_pins`.
- `60a610e` (tests): every legal quiet on the SEE FEN list (97 quiets) and on
  a fixed-seed random-walk corpus (400 walks, 10,872 quiets) is checked:
  - the `see_ge` boundary against `oracle_see`;
  - `see_ge(m, −value(mover))` is true and `see_ge(m, 1)` is false.

  Corpus divergences are pinned by geometry, with no unexplained case
  allowed: 22 promotion recaptures, 8 discovered checks, 11 exchanges in
  which legality removes an attacker.

Neither arm's fingerprint moved, as 17.5 predicts. The off arm's `bench 13`
is 14,978,465 on release PEXT and plain, Tune, Diag and Ablate, with
sanitizer `bench 10` 3,190,673. The ON arm's is 13,488,940, with sanitizer
3,445,675. CTest passes on both arms: release 20/20, sanitizer 18/18.

Probe re-run (`tools/results/b21-20261010/see_quiet_probe_v2.cpp` and `.txt`,
SHA-256 `4d90f00e…2ead` and `6fba27b9…827f`):
- `see_ge(−75)` is false for exactly 1,633 of the 2,351 quiet checks, matching
  `see() < −75`.
- `see_ge` agrees with `see() ≥ t` at every threshold from 0 to −160. It
  disagrees on 1,575 quiets at −250 and 156 at −360, almost all of them
  `see()`'s deliberate truncation.

A classifier over 4,000 walks (`see_classify.cpp` and `.txt`, SHA-256
`88dbd16a…806d` and `b501819e…ddbade`) compares the `see_ge` boundary with the
legality oracle for every legal move. Quiet moves diverge on 482 of 119,732
(0.40%): 308 promotion recaptures, 45 discovered checks, 129
legality-filtered exchanges, 0 unexplained. Captures, whose verdicts are
unchanged, diverge on 41 of 21,971 (0.19%) in the same three geometries.

One point for review. BAS-C09 names created pins and promotion recaptures.
Discovered checks are a third geometry of the same kind (the kernel computes
pins once and does not model check). They already occur for captures at the
head, so quiet calls reach the existing approximation rather than add a new
one. The implementation read 17.1's falsifier ("outside the BAS-C09
geometries") as met in intent; the amendment should confirm that reading.

### 18.2 The conflict: ticket 3's picker against the ON arm's WAC floor

The rest of ticket 3 was built and is held as a patch, not committed:
`tools/results/b21-20261010/ticket3-wip/`, base `60a610e`, SHA-256
`f533f42f…25eb` for the tracked diff and `6d64c059…12db` for the new files;
restoring it was verified byte for byte. It contains:
- the core table types (`history_core.h/.cpp`): the donor's bounds and
  gravity, `clear()` to zero, legacy halving in the interim;
- the staged picker (`move_picker_core.h/.cpp`) of review §5.7 with the
  threat and SEE-gated check terms;
- the continuation row on the stack and in the move seam;
- the quiet- and continuation-update routing with the legacy shapes
  (decision 9);
- killers and countermove removed;
- `CorePawnHistBucketsLog2`;
- `test_history_core` (97/97) and `test_move_picker_core` (5/5). The picker
  test is proven live: a planted defect, bad quiets delivered after
  `skip_quiets()`, fails 4 of its 5 checks.

The ON arm with it reads `bench 13` = 12,815,895 (PEXT and plain) and
sanitizer `bench 10` = 3,063,719. Sanitizer CTest passes 20/20. Release CTest
fails 1 of 22: `test_wac_floor` solves **119/300 at depth 6, against a floor
of 130**. The off arm, and the ON arm at ticket 2, both solve 138.

Attribution: one change each to the held ticket-3 tree, ON arm, WAC at depth
6 (`test_wac --floor`):

| ON-arm tree | Solved | `bench 13` |
|---|---:|---:|
| Ticket 2 (`60a610e`) | 138 | 13,488,940 |
| Ticket 3 as contracted | **119** | 12,815,895 |
| … check bonus without the SEE gate (`16384·check`) | **132** | 13,425,573 |
| … legacy history pruning off | 119 | 12,517,289 |
| … legacy LMR history term off | 114 | 14,114,802 |
| … legacy MVV capture ordering | 121 | 13,251,220 |

The SEE gate on the picker's check bonus, which Amendment 1 made live, costs
13 of the 19 positions. A sacrificial quiet check now ranks by history
alone; the legacy picker boosted every quiet check (+32,000, above every
graded history). The remaining 6 come from the rest of the ticket: killers
and countermove gone, the new quiet formula and routing. Legacy history
pruning, which the new scales could have woken, changes no position. The
gate is the donor's term and is what §3.6 and 17.1 contract. The picker runs
here against the legacy kernel's pruning and LMR (tickets 5–7 not landed),
whose checks are never reduced or pruned, so this hybrid is not the
population the donor's term was fitted in.

Stopped: B.2.1 requires CTest, the WAC and endgame floors included, to pass
on both arms at every ticket. Implementation may not relax a floor or
rescue the picker by changing a neighbouring term.

### 18.3 Options

- (a) A categorical switch, `CoreCheckBonusSeeGate` (0/1), with the
  per-ticket floors read at the ungated form (132/300) and B.2.2's paired run
  deciding the default, as it settles the other switches. The quiet and
  check-branch SEE pruning of ticket 6 keep the gated `see_ge` either way.
- (b) Qualify the ON arm's strength floors at the coherent cluster rather
  than per ticket: tickets 3–7 together (picker, node pruning, move-loop
  pruning, LMR), the per-ticket gate being the off arm's fingerprint and the
  correctness tests. This amends §12.2's per-ticket clause for the WAC floor
  only; the mate-drive floors keep binding every ticket.
- (c) Keep the gate and land ticket 3 failing the WAC floor on the ON arm.
  This is not open to implementation: it relaxes a test.

Recommendation: (a). It keeps the per-ticket discipline, costs one switch
that B.2.2's categorical machinery already handles, and leaves both forms
measurable on one binary. The counter-arguments:
- a fixed-depth-6 WAC count is not Elo (B.0.1's 32 WAC positions read
  +2.4 ± 9.9);
- the ungated default departs from the donor until B.2.2 decides;
- 132 clears the floor by only 2, so the later tickets' drift could still
  cross it, which would bring (b) back.

Resume: on research amendment 2, at ticket 3's remainder (the held patch),
on the amended contract.

## 19. B.2.1 Amendment 2 (2026-10-10): the WAC floor binds the ON arm at the coherent cluster; the check gate stays as contracted; no switch

Answers §18. Research (`R3`) at head `fe68f34`; implementation resumes at
ticket 3's remainder from the held patch, on this contract.

### 19.1 Decision 1: option (b), with the instrument named

**Decision.** The picker keeps the donor's gated term,
`16384·(check && see_ge(m, −75))`, as §3.6 and 17.1 contract, with no
switch. The per-ticket gate on the ON arm is the off arm's fingerprint,
every correctness test, the unit tests, and the mate-drive and mate-band
floors (KBNK, KQK, KRK, KBBK, the band): those bind every ticket as §12.2
wrote them. The depth-6 WAC floor binds the ON arm from the commit that
lands ticket 7, when the picker, node pruning, move-loop pruning and LMR
are one population, and at every commit after it. Between ticket 3 and
ticket 7 the ON arm's WAC count is an instrument, recorded per ticket, not
a floor.

Why the floor does not bind a mid-cluster hybrid:

- It is the wrong instrument for the ON arm. A fixed-depth tactical count
  changes by construction when the selectivity stack changes, and the
  cluster is adopted as a whole precisely because its terms are fitted to
  each other (`agents/research.md`, *Before implementing a mechanism*). The
  programme registered the ON arm's tactical screens at a fixed node
  budget for that reason: §10's WAC at 100,000 nodes, floor ≥ 200 and
  target ≥ 225, and P4. Depth 6 under the legacy kernel's never-pruned,
  never-reduced, always-extended checks is a different population from
  depth 6 under the donor's, and 130 was calibrated on the former (138
  with headroom 8).
- The gate is more right for Basilisk's B.2 population than for the
  donor's, not less. Ticket 6 prunes a quiet check by SEE at `−74·d −
  captHist/80` and ticket 7 reduces every move class, so a losing check is
  pruned or reduced wherever the picker puts it; what an ungated bonus
  would buy in the coherent cluster is an early, extended subtree (B.2
  keeps the in-check extension) for a check that then loses material. The
  13 positions the gate costs at depth 6 are what the hybrid shows while
  the legacy kernel still searches every check at full depth plus one,
  and ordering is the only lever left.
- A switch whose default is chosen to clear a legacy-calibrated floor by
  two positions is a rescue by a neighbour, and its categorical run would
  decide a question about the hybrid in the population of the cluster.
  Nothing is lost by not having it now: both forms stay one line apart.

The remaining six positions (killers and countermove gone, the donor's
quiet formula and routing) are the cluster's population shift, not a
defect: the picker's exhaustiveness test is proven live, and legacy
history pruning, which the new scales could have woken, moves nothing.

**What binds instead, per ticket from ticket 3 to ticket 6.** The ON arm's
CTest keeps running the WAC suite each ticket, but as a count: a `--count`
mode of `test_wac` (depth 6, prints the solved count, exit 0), registered
on the ON arm in place of `--floor` until ticket 7 re-registers `--floor`
there; the off arm's `test_wac_floor` and its 130 are untouched. The
implementation record carries, per ticket, the ON arm's depth-6 count and
its 100,000-node count from `fixed_budget_probe.py` (§10's instrument). A
ticket that moves the depth-6 count by 10 or more positions attributes the
move one change at a time, as §18.2 did; an unexplained share above 5
positions returns the leaf. Ticket 3 records 119 at depth 6 with §18.2's
table as its attribution, and reads the 100,000-node count on the same
tree.

**At ticket 7 and after.** The ON arm's `--floor` at 130 is re-registered
in ticket 7's commit. If it fails there while §10's 100,000-node floor
(≥ 200) holds, that is a floor failure and returns the leaf as PLAN says;
research then decides the ON arm's depth-6 floor with both numbers in
hand. Implementation does not re-calibrate it.

**Falsifier and retry trigger for the switch.** If at the coherent cluster
the 100,000-node WAC reads below 200, or P4 (≥ 225 with razoring
re-seeded) misses, and a one-change attribution names the check gate, then
`CoreCheckBonusSeeGate` (0/1) becomes a categorical switch, decided by a
2,000-game paired run in the population it belongs to. Until then the
§18.2 table is the recorded evidence and nothing else is built for it.

### 19.2 Decision 2: discovered checks

Confirmed. Discovered checks are a third legality geometry of the kind
BAS-C09 names: the kernel computes pins once on the exchange occupancy and
does not model check, so a capture that is illegal because it discovers
check on its own king, or that is forced by one, is scored as if legal.
They already occur for captures at the head (the classifier's 41 of 21,971,
0.19%, in the same three geometries as the quiets' 482 of 119,732,
0.40%), so quiet calls reach an existing approximation and add none. 17.1's
falsifier reads "outside the three legality geometries the classifier
names: promotion recapture, discovered check, legality-filtered
exchange"; it is met, with 0 unexplained. BAS-C09's entry gains one line
naming the third geometry; its retry trigger (verdict changes under per-ply
pins on the bench corpus) is a different measure from the classifier's
random-walk divergence and is still re-read once in B.2.2 as 17.4 says.

### 19.3 Predictions

P1–P6 unchanged; clerical amendments none. §10's screens are unchanged:
the WAC floors there were always the cluster's.

### 19.4 Handoff, amended

- §12.2's "the KBNK/KQK/KRK/KBBK and mate-band CTest floors bind the on
  arm" stands as written; review §5.11 item 6's "strength floors (WAC …)"
  on both arms is read as: WAC binds the off arm every ticket and the ON
  arm from ticket 7.
- Ticket 3 resumes from the held patch (`tools/results/b21-20261010/
  ticket3-wip/`, base `60a610e`) unchanged in mechanism, plus the
  `test_wac --count` mode and its ON-arm CTest registration, in a test
  commit separate from the engine commit. Its record: ON `bench 13` =
  12,815,895, sanitizer `bench 10` = 3,063,719, depth-6 WAC 119 with the
  §18.2 attribution, the 100,000-node count, `test_history_core` 97/97,
  `test_move_picker_core` 5/5, off arm at 14,978,465.
- Tickets 4–6: the same record per ticket. Ticket 7: re-register
  `test_wac --floor` on the ON arm.

## 20. B.2.1 Implementation record (running; extended per ticket)

The record PROCESS asks for: commits per ticket, both arms' fingerprints,
tests added, deviations with their reasons, and what the reviewer must
check. Tickets 4–8 append rows here. Every fingerprint is `bench 13`
release, the same on PEXT and plain unless stated; the sanitizer column is
`bench 10` and equals the release `bench 10` of the same arm. The off arm
holds 14,978,465 (sanitizer 3,190,673) on PEXT, plain, Tune, Diag and Ablate
at every commit listed.

| Ticket | Commits | ON `bench 13` | ON sanitizer | CTest release / sanitizer (both arms) | ON WAC depth 6 | ON WAC 100k nodes |
|---|---|---:|---:|---|---:|---:|
| 0 counters, trace v2, parsers | `1dbf0d9` tools, `61c15cb` engine | — (§16.4 item 8) | — | off 19/19, 17/17 | — | — |
| 1 threats, stack record, fork, CI | `32e56ad` engine, `34a6d33` CI | 14,978,465 | 3,190,673 | 20/20, 18/18 | — | — |
| 2 TT encoding and cutoff rules | `54ff32d` tools, `e97a3cd` test, `f179404` engine | 13,488,940 | 3,445,675 | 20/20, 18/18 | 138 (§18.2) | — |
| 3a quiet-move `see_ge` | `ea5c49f` engine, `60a610e` tests | 13,488,940 | 3,445,675 | 20/20, 18/18 | — | — |
| 3b WAC count mode | `9a632e6` tests | 13,488,940 | — | 20/20 both arms | 138 (count) | — |
| 3c tables, picker, routing | `bcf63e8` engine | **12,815,895** (transient, decision 9) | 3,063,719 | ON 22/22, 20/20; off 20/20, 18/18 | **119** (count; §18.2 attribution) | **200** |
| 4 correction, improving, worsening, hindsight, eval-difference training | `221afdf` engine and test | **13,061,601** | 4,006,140 | ON 22/22, 20/20; off 20/20, 18/18 | **127** (count; +8, under the attribution trigger) | **193** |
| 5 razoring and RFP | `bb73fb5` engine | **25,182,694** (one position, below) | 3,093,931 | ON 22/22, 20/20; off 20/20, 18/18 | **167** (count; +40, attributed below) | **237** |
| 6 move-loop pruning | `573add0` engine, `8337944` test | **11,922,648** | 2,968,759 | ON 22/22, 20/20; off 20/20, 18/18 | **153** (count; −14, attributed below) | **235** |
| 7 LMR, floor, re-search rules, `depth −= 3`, fail-high blend | `b02fcb7` build, `802ed5c` engine | **10,739,013** | 2,176,534 | ON 22/22 (floor binds again), 20/20; off 20/20, 18/18 | **176** (floor 130 holds; +23, attributed below) | **241** |
| 8 update policy and decay | `4112db0` engine and test | **6,337,226** | 1,490,974 | ON 22/22, 20/20; off 20/20, 18/18 | **185** (floor holds; +9, under the attribution trigger) | **249** |

Ticket 3's instruments (`tools/results/b21-20261010/ticket3/`):
- `test_wac --count` → `wac_depth6_on.txt`, SHA-256 `e1854e6e…73b5`.
- `fixed_budget_probe.py nodes 100000 src/wac.epd` on the ON release
  binary (`basilisk-b21-t3-on-pext.exe`, `60a1971c…55f2`) →
  `wac_nodes100k_on.json`, `fe67970f…de15`: **200/300**.
- The same recipe on the off binary (`5705e8f3…dd2a`) reads **204**, §10's
  legacy baseline, which proves the instrument on this tree
  (`wac_nodes100k_off.json`, `b19e43cc…9b19`).
- Both counts are instruments until ticket 7 (§19.1); no action is taken
  on them.

Ticket 4's instruments (`tools/results/b21-20261010/ticket4/`):
- `wac_depth6_on.txt`, SHA-256 `f1e89eaf…30a7`: 127.
- `wac_nodes100k_on.json`, `6798fc0f…db48`: 193, on
  `basilisk-b21-t4-on-pext.exe` (`5dfa75cf…565b`).
- The depth-6 count moved +8, under the 10-position trigger, so no
  attribution is owed.
- Liveness, on an ON Tune build (Italian position, depth 13): the hindsight
  and continuation-correction counters fire (3,857 up, 10,055 down, 7,148
  updates); `CoreCorrAdmission` = 0 changes the search (453,734 against
  463,065 nodes).

Ticket 5's instruments (`tools/results/b21-20261010/ticket5/`):
- `wac_depth6_on.txt`, SHA-256 `ee16d14f…3560`: 167.
- `wac_nodes100k_on.json`, `3fddc775…0b65`: 237, on
  `basilisk-b21-t5-on-pext.exe` (`e52183fa…729e`). That is above §10's
  legacy baseline of 204 and its target of 225; at ticket 5 it is still an
  instrument (§19.1).

The depth-6 move of +40 is attributed one change at a time, in scratch
trees that restore a block to its ticket-4 code at the legacy constants:

| Ticket-5 tree with… | Depth 6 | 100k nodes | `bench 13` |
|---|---:|---:|---:|
| nothing restored | 167 | 237 | 25,182,694 |
| legacy razoring (depth ≤ 3, `243·d`, `q ≤ alpha`) | 124 | 202 | 12,551,164 |
| legacy RFP (`160·d − 72·improving`, returns eval) | 162 | 236 | 13,980,101 |
| both restored | 127 | — | 13,061,601 |

- Restoring both blocks reproduces ticket 4 exactly (127 and 13,061,601).
  So the parameter-table split is neutral, and the whole move belongs to
  the two blocks.
- Razoring accounts for +43 and RFP for +5. They interact for −8, the only
  other change being their joint presence, so 0 positions are unexplained.
- Razoring at depth 1 only is the B.0 finding at work: §2.6 measured the
  legacy razoring's reach to depth 3 costing 51 WAC positions at 100k nodes.

The `bench 13` total doubled because of one position. Bench position 39
(`1Q4R1/5k2/4rpp1/3K4/8/7p/8/8 b - - 2 9`) took 14,473,319 nodes (57.5% of
the total) against 756,321 at ticket 4, and reports a mated score (−31904)
where ticket 4's bench reported −1312. The position is a forced mate: from a
fresh search at depth 13, ticket 4 finds mate −36 in 2,310,540 nodes and
ticket 5 mate −30 in 3,508,950. Ticket 5 finds the mate inside the bench
sequence and expands its lines; ticket 4 did not. Over the 40 positions the
median node ratio to ticket 4 is 0.96 (`bench_t4.txt`, `bench_t5.txt`
beside the instruments). This is recorded, not acted on: the tree-size
screens are B.2.2's.

Liveness, on ON Tune and Ablate builds (Italian position):
- The razoring and RFP counters fill their depth buckets.
- `CoreRazorDepthCap` = 3 and `CoreRfpPerPly` = 60 change the search
  (293,683 against 393,875 and 199,505 nodes at depth 13).
- Ablation bits 0 and 1 change the tree (184,905 against 392,553 and
  369,019 nodes at depth 12).
- The off arm's Tune option list is byte-identical to ticket 4's. The ON
  arm's drops `RfpCoeff`, `RfpImproving` and `RazorCoeff` and gains the seven
  node-margin coordinates and `CoreRazorDepthCap`.

Ticket 6's instruments (`tools/results/b21-20261010/ticket6/`):
- `wac_depth6_on.txt`, SHA-256 `a9e6b4ee…d67c`: 153.
- `wac_nodes100k_on.json`, `ef1f84ea…3ade`: 235, on
  `basilisk-b21-t6-on-pext.exe` (`f367fa33…b7cd`).
- `bench13_counters_on.json`, `23c968f2…702c`: the Diag counters summed
  over the 40 bench positions at depth 13. Every member of the family fires:
  - quiet SEE 273,545 and capture/check SEE 1,390,651 (17.5 asked for both
    non-zero);
  - continuation history 91,892;
  - quiet futility 635,049 and capture futility 176,689;
  - the quiet skip at 759,788 nodes;
  - 5,953,069 moves tested.

  All 20 counter identities hold per position and summed.

The depth-6 move of −14 is attributed in scratch trees
(`attribution_depth6.txt`, `fb46b8f1…140b`; recipe `t6_variants.py`,
`c4809b77…23df`):

| Ticket-6 tree with… | Depth 6 | `bench 13` |
|---|---:|---:|
| nothing changed | 153 | 11,922,648 |
| the count skip off | 154 | 16,403,443 |
| capture futility off | 157 | 14,232,173 |
| capture/check SEE off | **176** | 17,142,433 |
| continuation-history pruning off | 153 | 11,158,009 |
| quiet futility off | 154 | 14,992,719 |
| quiet SEE off | 150 | 11,664,310 |
| every member off (no move-loop pruning) | 181 | 40,063,031 |
| the legacy family restored | 167 | 25,182,694 (= ticket 5) |

- Restoring the legacy block reproduces ticket 5 exactly. So the rest of
  the ticket (the picker's skip mode, the parameters) is neutral.
- The legacy family cost 14 positions (181 → 167) and the new family costs
  28 (181 → 153), which accounts for the −14 move exactly.
- The one-at-a-time effects sum to −26 against the joint −28: 2 positions
  of interaction, none unexplained.
- The capture/check SEE test carries the move (−23). It prunes sacrificial
  checks and captures that lose material by SEE, the population §19.1
  reasons about.
- At 100,000 nodes the count barely moves (237 → 235) while the tree
  halves, so the loss is a fixed-depth reading of the stronger pruning.

Liveness, on ON Tune and Ablate builds (Italian position):
- `CoreLmpExemptChecks` = 0 changes the search (352,187 against 250,539
  nodes at depth 13).
- Ablation bit 5 removes the family (277,289 against 752,158 nodes at
  depth 12).
- The picker test's new skip-keeping-checks section was proven live: a
  planted defect, delivering every quiet under the exemption, fails 5 of 6
  sections.

Ticket 7's instruments (`tools/results/b21-20261010/ticket7/`):
- `wac_depth6_on.txt`, SHA-256 `7894a6ce…099e`: 176. Ticket 7's commit
  re-registers `test_wac --floor` on the ON arm, and the 130 floor holds.
- `wac_nodes100k_on.json`, `fc18fd9f…aafa`: 241, on
  `basilisk-b21-t7-on-pext.exe` (`36ff79a2…c088`). That is above §10's
  100k floor of 200 and its target of 225.
- `bench13_counters_on.json`, `58cfa3e4…39d0`, summed over the 40 bench
  positions at depth 13:
  - `lmr_researched / lmr_applied` = 40,165 / 1,316,120 = **3.05%** (P6
    predicts 3–5%);
  - `rfp_cuts / interior_nodes` = **13.67%** (P6 predicts below 12%);
  - 211,290 negative reductions and 1,482,929 floor hits;
  - 22,475 re-searches made deeper and 57,107 shallower;
  - all 20 identities hold.

  These are P6's inputs, read here as instruments; B.2.2 reads P6.

The depth-6 move of +23 is attributed in scratch trees
(`attribution_depth6.txt`, `3e061336…0f79`; recipe `t7_variants.py`,
`d9c70e5f…4e9f`). Reverting every part reproduces ticket 6 exactly (153,
11,922,648), so the five parts below are the whole change. Adding them to
ticket 6 one at a time:

| Added to ticket 6 | Depth 6 | `bench 13` |
|---|---:|---:|
| (ticket 6) | 153 | 11,922,648 |
| the LMR section: formula, floor, negative reduction, deeper/shallower re-search, the step-down, the PV re-search rule | 182 (+29) | 15,890,487 |
| move-loop pruning reads the new reduction | 183 (+1) | 15,210,957 |
| `depth −= 3` after alpha rises | 175 (−8) | 10,843,169 |
| the fail-high blend | 177 (+2) | 8,981,867 |
| the equal-score promotion (= ticket 7) | 176 (−1) | 10,739,013 |

The steps sum to +23 exactly, so nothing is unexplained. Removing one part
at a time from ticket 7 reads differently, because the parts interact:
- `depth −= 3` off: 182; the blend off: 182; the promotion off: 177.
- The deeper/shallower adjustment off: 179; the step-down off: 179.
- The legacy `lmrDepth` for pruning: 179; ticket 6's whole LMR section:
  147.

The new LMR is the gain and `depth −= 3` the main cost. Depth 6 is a fixed
depth, and `depth −= 3` makes the later moves' searches shallower.

Liveness, on ON Tune and Ablate builds (Italian position):
- `CoreLmrNegativeAllowed` = 0 changes the search (136,721 against 146,072
  nodes at depth 13).
- Ablation bit 7 removes LMR (78,048 against 751,653 nodes at depth 12).

The sanitizer stack (`b02fcb7`):
- With ticket 7, the Windows sanitizer `test_endgames` overflowed the 1 MB
  main-thread stack in `KBNK0061`, whose game reaches selective depth 37
  (31 at ticket 6; `kbnk_replay.py`).
- `-fstack-usage` sizes a non-PV `negamax` at 5,336 bytes plus 9,464 for
  its move loop in the sanitizer build, against 1,224 plus 184 in release.
- Windows sanitizer builds now reserve 8 MB, as Linux (the CI sanitizer
  job) does. This is a precondition fixed in its own commit; the test is
  unchanged. A full 128-ply line in release needs about 0.3 MB.

Ticket 8's instruments (`tools/results/b21-20261010/ticket8/`):
- `wac_depth6_on.txt`, SHA-256 `f5080051…2ebf`: 185.
- `wac_nodes100k_on.json`, `b91514ae…22ee`: 249, on
  `basilisk-b21-t8-on-pext.exe` (`b3db77f7…42a3`).
- `bench13_counters_on.json`, `a649fe7f…b870`:
  - `lmr_researched / lmr_applied` = 24,431 / 834,061 = 2.93%;
  - `rfp_cuts / interior_nodes` = 717,147 / 4,589,805 = 15.62%;
  - 1,231,231 history updates at fail-high nodes and 39,584 at exact nodes;
  - all 20 identities hold.

Liveness, on an ON Tune build: `CoreMalusDecay` = 1024 and
`CoreQuietBonusSlope` = 60 change the search (104,200 against 94,043 and
95,086 nodes at depth 13). `CoreMainHistDecay` acts between searches. In
one session of three successive depth-12 searches the first is identical
(64,062 nodes, fresh tables) and the later ones differ: 30,832 / 120,209
at 1024, 48,268 / 47,998 at 729, 49,938 / 48,126 at 400.

§12.2's cheap qualification on the final ON arm:
- The ablation liveness proof (`ablation_liveness.py`, Ablate build
  `c9cd4a8f…be06`; `ablation_liveness_on.json`, `2749d8e5…1f4c`): all 8
  bits live, the re-declared 0, 1, 5 and 7 included.
- The decision trace on WAC.001, 085, 203 and 253 (`decision_trace.py`,
  depth 8, the WAC best move as root, Diag build `1a50951b…878c`;
  `trace_WAC.*.json`). Each is version 2 with `cutoff_count` available:
  - 466, 815, 1,132 and 138 records;
  - the move-loop families recorded at plies 1–2 (skip-quiets,
    capture futility and SEE, quiet futility and SEE, RFP);
  - `cutoff_count` produced on 331, 471, 939 and 109 records.
- §10's zero-game rows (branching, nodes at depth 12 against the oracle,
  the WAC screens at 100k and 400k nodes, the LMR-off delta, agreement,
  time to depth, canaries) are B.2.2's screens and were not run. Only the
  100k-node WAC count was read, as an instrument at each ticket.

#### Final state, both arms (head `4fd8600`)

| Build | Off arm | ON arm |
|---|---:|---:|
| `bench 13`, release PEXT | 14,978,465 | 6,337,226 |
| `bench 13`, release plain | 14,978,465 | 6,337,226 |
| `bench 13`, Tune / Diag / Ablate | 14,978,465 each | 6,337,226 each |
| `bench 10`, release and sanitizer | 3,190,673 | 1,490,974 |
| Release CTest | 20/20 | 22/22 |
| Sanitizer CTest (`-LE strength`) | 18/18 | 20/20 |
| WAC depth 6 (`test_wac --floor`, 130) | 138 | 185 |
| WAC 100,000 nodes (§10's instrument) | 204 | 249 |

PEXT and plain agree on both arms at every ticket, and the sanitizer
build's `bench 10` agrees with release on both arms at every ticket. The
off arm's Tune option list and the registered SPSA surface (41
parameters) are byte-identical to the B.2.0 head's. The ON arm's core
table carries the review's 50 cluster coordinates and 8 legacy ones still
read (null move, ProbCut, singular, `LmrSingularQuiet`). (Review §5.10
says 51, but its own table sums to 50.) `CoreIirMinDepth` (4) was exposed
last, in its own commit (`4fd8600`), behaviour-neutral: the ON arm's
`bench 13` is unchanged at 6,337,226. The ON arm also has 6 tune-build
switches: `CoreTtCutoffNodeTyped`, `CorePawnHistBucketsLog2` (the review's
pawn-history size, as a power of two), `CoreCorrAdmission`,
`CoreRazorDepthCap`, `CoreLmpExemptChecks` and `CoreLmrNegativeAllowed`.

Tests added so far: the 22 counters and trace v2 in the parsers' tests;
`test_threats` (both arms); the ON arm's TT tests (pv bit, age cycle,
evaluation-only entries, rule-50 downgrade) and the key-0 empty slot (both
arms); `test_board`'s quiet SEE against the legality oracle (both arms);
`test_history_core` (97 checks) and `test_move_picker_core` (5 sections:
exhaustive delivery, TT first and once, foreign TT move, excluded move,
skip-quiets at the start and after the first quiet, the partial sort),
both ON-only under `_ALL_TARGETS`. The picker test was proven live with a
planted defect (§18.2).

Deviations and implementation choices, with reasons:
1. Ticket 1: the ON parameter table aliases the legacy one, so the forked
   kernel reads every legacy coordinate until its mechanism is replaced
   (§16.4 item 8).
2. Ticket 2: the TT-cutoff bonus is `131·depth` uncapped (decision 2);
   `tt_capture` is capture-stage membership (decision 10); the clock test's
   move check was restated as keeping the win, in its own commit
   (`e97a3cd`, §16.1).
3. Ticket 3a: `see_ge` takes 17.1's optional fast path; captures'
   verdicts are byte-identical (both fingerprints unchanged).
4. Ticket 3c, placement choices inside the contract:
   - The main history is `[colour][from][to]`, as review §5.5 places it.
     Promotions share their from-to, which differs from the donor's
     whole-move key only for under-promotions.
   - The correction tables stay in their legacy form inside
     `CoreHistoryTables` until ticket 4.
   - Interim ageing halves every table but the TT-move history every
     second search, as the legacy kernel did, until ticket 8.
   - The legacy consumers still in place (history pruning, the LMR history
     term, the trace) read the new tables as cont1 + cont2 + cont4/2.
   - The legacy quiescence body (B.4's) keeps its capture order, reading
     the new capture table, and a copy of the legacy `pick_next`.
   - The picker no longer memoises `see_ge(m, 0)`: the legacy LMR's
     bad-capture test computes it where it reads it, until ticket 7.
   - In-check nodes use the main picker (decision 13).
   - `CorePawnHistBucketsLog2` is clamped to the table at use.
5. Ticket 3b: the ON arm registers `test_wac --count` as `test_wac_floor`
   until ticket 7 (Amendment 2); `FLOOR` stays 130. The off arm still
   asserts: a planted `FLOOR = 139` fails it.
6. Ticket 4, choices inside the contract:
   - The static evaluation, `improving`, worsening and hindsight now run
     before the TT cutoff, so the cutoff reads the hindsight-adjusted depth
     (the donor's order). The eval-difference training follows the cutoff.
   - The donor's `bool(bestMove)` is read as "a move raised alpha": the
     legacy loop sets `best_move` at fail-low nodes too.
   - Correction training keeps the legacy guards the review restates (no
     exclusion, no decisive score) beside the donor's admission. The
     multi-cut training is B.3's, with singular.
   - In check, the record inherits the eval from two plies up while the
     local static eval stays unset, so the legacy consumers still see none.
     Where that inherited eval is `VALUE_NONE`, hindsight's sum is large, as
     the donor's arithmetic is; this is kept literally.
   - The continuation-correction sentinel row is trained like the donor's;
     the continuation-history sentinel is never trained.
   - The correction tables are halved between searches with the rest,
     until ticket 8's decay.
   - `improving` also holds at or above beta, set after the node-pruning
     block, which is equivalent for every consumer, since none inside that
     block reads it after the null move.
   - `corr_cont2_updates` and `corr_cont4_updates` count the same events,
     because both rows train together.
   - `test_history_core` changed in the engine commit, since it names the
     new correction rows and neither commit would build alone.
7. Ticket 5, choices inside the contract:
   - The donor's order: razoring before RFP. Both sit inside the legacy
     block gate (decision 12); they cannot both fire at one node.
   - Razoring's guards are §3.4's two: no quiet TT move (an unvalidated TT
     move classified by capture-stage membership, like the legacy LMR's TT
     capture) and no lower-bound entry. Above depth 1 the margin is linear
     (Amendment 1, decision 6). The value is always returned.
   - RFP's decisive guards are the donor's: beta not a loss, the estimate
     not a win.
   - RFP's correction term divides the correction in evaluation units
     (`|raw| / 131072`) by `CoreRfpCorrDiv`; at its seed of 0 the term is
     off, so the unit only matters once B.2.3 moves it.
   - The legacy parameter table is split into six group macros spliced in
     their original order. The default arm's table, its Tune option list
     and the registered SPSA surface (41) are unchanged. The core table
     takes every group except the node margins.
8. Ticket 6, choices inside the contract:
   - The family is gated by §12.2's invariant: off at the root, in check and
     before a move was searched. The donor's own gates are kept as well: the
     best score not a proven loss, and non-pawn material for the side to
     move.
   - Count pruning reads `ss->move_count`, every picked move. The skip is
     called once per node; with `CoreLmpExemptChecks` on, the picker keeps
     delivering quiets whose moved piece checks from its destination
     (castling excluded).
   - `lmrDepth` reads the legacy base reduction table, in 1024ths, indexed by
     move count, until ticket 7 brings the cluster's reduction. The history
     adjustment uses the donor's 16 per-depth divisors, fixed constants in
     history units.
   - The capture/check branch covers captures, every promotion and quiet
     checks (Amendment 1). Its last-piece stalemate guard counts the side's
     knights, bishops, rooks and queens.
   - Quiet futility raises the best score to the futility value unless the
     best is decisive or the value a win. Since a move has always been
     searched first, the no-legal-move path is never reached through
     pruning.
   - The quiet branch is non-PV only (decision 4); the capture/check branch
     also runs at PV nodes, as the donor's does.
   - The core arm drops the legacy move-loop coordinates; the legacy
     reachability counters (`hist_below_*`) read 0 on this arm.
9. Ticket 7, choices inside the contract:
   - Nothing is reduced at the root or in check: neither LMR, nor the
     step-down when LMR is skipped, nor `depth −= 3` (§12.2's invariant;
     the donor does all three there). The first searched move is never
     reduced.
   - The reduction table has 256 entries, `int(scale/128 · ln i)`. The
     window term divides by the root window `root_search` records; the
     equal-score rule reads the iteration depth it records.
   - The fixed donor constants are kept as written, all in 1024ths of a
     ply or history units: +697, −65 per move, `|corr|/26310` (corrected by
     Amendment 3 §22.2: that one acts on evaluation units and becomes
     `|corr|/7419`), 940 for a
     cut node's deep TT entry, −2179 for the TT move, +1127 without a TT
     move, the step-down at 5234 and 5487, 577/1024 for the window, and
     the capture `statScore` 873/128.
   - A miss's TT score reads `VALUE_NONE`, so the tt_pv "above alpha"
     term counts it, as the donor's does.
   - The alpha term reads the TT-refined estimate, or in check the record's
     inherited eval. The kept B.3 term `LmrSingularQuiet` (401) still
     relaxes quiets after a singular TT move.
   - `best_move` keeps the legacy meaning (the best move so far, even below
     alpha); the equal-score promotion and the alpha raise follow the
     donor. The fail-high blend reads the alpha the cutting move had to
     beat.
   - The counters partition as before: eligible = applied + clamped_zero +
     blocked_depth + blocked_in_check, over non-root moves after the
     first. `lmr_clamped_high` counts the same floor hits as
     `lmr_floor_hits`.
   - The core arm drops the legacy LMR coordinates except
     `LmrSingularQuiet` (a split group, in the same order on the default
     arm).
10. Ticket 8, choices inside the contract:
    - Training happens at node exit for every node where a move raised
      alpha (fail highs and exact nodes), replacing both legacy update
      sites (the in-loop cutoff update with its surprise scale, and the
      reward-only exact update).
    - The donor's `bestMove` is tracked as `alpha_move`: the move that last
      raised alpha. The legacy `best_move` keeps its meaning for the TT
      store and the PV. The searched lists hold moves that did not raise
      alpha, from the node's first 32 picked moves, at most 32 each, split
      by capture stage.
    - The fail-low rule rewards the parent's move: a quiet one by the
      donor's scale, with this ticket's converted thresholds of 30 and 19
      (decision 5); a capture by +892 in the capture history.
    - The post-LMR continuation update (1334) runs whenever a reduced
      search fails high, re-searched or not, as the donor's does.
    - The TT-move history trains at non-PV nodes (+918 / −747). Its only
      donor consumer is the singular margin, which is B.3's.
    - Between searches the main history keeps `CoreMainHistDecay`/1024 and
      the low-ply history restarts at 102; the capture, continuation, pawn,
      TT-move and correction histories no longer age. A new game clears all
      of them to 0 (decision 7).
    - `hist_cutoff_updates` and `hist_reward_updates` now count updates at
      fail-high and at exact nodes.
    - The table test changed in the engine commit, since it calls the new
      `age()`.

What the reviewer must check (the complete list, at `4fd8600`):
- **Fingerprints.**
  - The off arm at 14,978,465 on release PEXT and plain, Tune, Diag and
    Ablate at every engine commit, with sanitizer `bench 10` = 3,190,673.
  - The ON arm at each ticket's row above, PEXT = plain, and sanitizer =
    release at `bench 10`.
  - CTest on both arms, release and sanitizer, the WAC and endgame floors
    included (the ON arm's WAC floor from ticket 7).
- **The handoff, clause by clause:** §12.2, Amendments 1 and 2, and review
  §5.4–5.11, against the deviations and choices listed above (items 1–10).
  Particular attention to:
  - **TT (§5.6):** the encoding, the miss-store's occupancy, the rule-50
    downgrade, the node-typed cutoff, the graph-history check;
  - **SEE (17.1):** the quiet `see_ge`, whose castling-only early return
    and fast path are exact; the oracle tests pin its divergences by
    geometry;
  - **Picker (§5.7):** stage order, the −v/18 and −14000 splits, the sort
    limit −3560·depth, the skip-quiets semantics and the direct-check
    exemption, the threat and check terms;
  - **Update routing (§3.6):** continuation weights, consistency
    multipliers, the in-check two-ply limit, +73 at one ply; the
    712/750/1104/459 factors; and `update_all_stats`, the fail-low rule
    and the post-LMR update against the donor's constants;
  - **Correction (§3.3):** the form and the admission;
  - **Node-entry order:** static eval, `improving` and worsening,
    hindsight, then the TT cutoff on the adjusted depth;
  - **Node pruning:** razoring and RFP (§3.4, Amendment 1 decision 6);
  - **Move-loop pruning (§3.7):** the family on the history-adjusted
    `lmrDepth`, its gates and the stalemate guard;
  - **LMR (§3.7):** the formula and terms, the floor and ceiling, the
    re-search rules, `depth −= 3`, the fail-high blend, the equal-score
    promotion.
- **The invariants (§12.2, review §5.12):**
  - nothing pruned or reduced at the root, in check or before a move was
    searched;
  - the reduced depth at least one ply;
  - TT moves validated at every consumer;
  - correction never trained in check, under exclusion, from a decisive
    score or after a capture best move;
  - the TT stores the raw static eval only;
  - the sentinel continuation row never updated;
  - no killers or countermove on the ON arm;
  - the trace and counters compiled out of release;
  - per-node code allocating nothing.
- **Instruments and registrations:**
  - the counter identities;
  - the ablation bits 0, 1, 5 and 7 re-declared;
  - the switches as tune options, never on the SPSA surface;
  - the count-mode registration (Amendment 2), withdrawn at ticket 7;
  - the Windows sanitizer stack (`b02fcb7`), with its measurement.

#### Return 3 and 4 fixes (Amendments 3 and 4), 2026-10-12

Items 1–5 are in §23.1 (`31c80db`..`90d7db7`); items 6–10 follow, in
§22.6's order with §24.1's item 6.
- Recipe: `tools/results/b21-r3-20261011/` (`qual.sh` `6813d6fc…d0b8`,
  `check.py` `b0555759…86ad`, `edit.py` `46faab4a…5e31`), on persistent
  build directories of the working tree, both arms, every flavour.
- Every row: the off arm reads 14,978,465 on release PEXT and plain, Tune,
  Diag and Ablate, and `bench 10` 3,190,673 on PEXT, plain and sanitizer.
- The ON arm's `bench 13` is equal on PEXT, plain, Tune, Diag and Ablate,
  and its `bench 10` is equal on release and sanitizer.

| Item | Commit | ON `bench 13` | ON `bench 10` | CTest release / sanitizer (ON; off) | ON WAC depth 6 | ON WAC 100k |
|---|---|---:|---:|---|---:|---:|
| 6 Q2.1, `min(abs(corr)/7419, 2695)` | `45f4827` | 7,644,681 | 2,337,269 | 22/22, 20/20; 20/20, 18/18 | 180 | 248 |
| 7 Q2.2, the alpha term on the scaled difference | `caee6ed` | 9,683,768 | 1,517,011 | 22/22, 20/20; 20/20, 18/18 | 177 | 250 |
| 8 Q2.3, eval-difference training on the scaled sum | `e0f7cc8` | 9,132,715 | 1,678,409 | 22/22, 20/20; 20/20, 18/18 | 185 | 249 |
| 9 `CoreRfpCorrMult` (neutral) | `e13ad68` | 9,132,715 | 1,678,409 | 22/22, 20/20; 20/20, 18/18 | 185 | 249 |
| 10 node checks (engine, neutral) | `c5ad7bb` | 9,132,715 | 1,678,409 | 22/22, 20/20; 20/20, 18/18 | 185 | 249 |
| 10 `test_stack_unwind` (test) | `7f1b5bb` | 9,132,715 | 1,678,409 | 23/23, 21/21; 20/20, 18/18 | 185 | 249 |

The 100k-node files are `i6`–`i10b/wac100k_on.json`: `a2092656…8463`,
`3d499a74…f6a0`, `53602c58…c9e9`, `edce84c7…f56c`, `c3b3fc9c…29e3`,
`21c25e33…cad0`.

- **Item 6** lands exactly on §23.3's scratch reading (7,644,681; 248).
- **Item 7** raises the median node count by 18% (110,508 → 130,462). Bench
  39 goes from 34.0% to 36.7% of the nodes, so the rise is broad, not one
  position running away. It is what the term is meant to do: its reach is
  now the donor's 288/1024, against 81 before. Recorded; the tree-size
  screens are B.2.2's.
- **Item 8** median: 120,520.
- **Item 9** is neutral at its seed. It is live on an ON Tune build (Italian
  position, depth 12): 61,746, 62,359 and 38,773 nodes at 0, 676 and 2048.

Item 10, choices inside the contract:
- **Where the checks live.** The checks (`CORE_NODE_CHECK`, after the TT
  probe) are live in the Diag flavour and in every build without `NDEBUG`,
  and compiled out of release.
  - §22.3 suggested `BASILISK_DIAGNOSTIC` alone, but CTest runs only release
    and sanitizer builds, so a Diag-only check would never run in either
    suite.
  - The sanitizer build is Debug without `NDEBUG`, so the sanitizer suite
    runs the checks in every core search it makes, the new test included.
    A failed check prints the condition and aborts (an `assert` would be
    compiled out of the Release-type Diag build).
- **What the checks verify:**
  - `in_check` is set;
  - `move_count`, `stat_score` and `reduction` are 0;
  - `(ss + 2)->cutoff_cnt` is 0;
  - the children's `excluded` is none, which reads "excluded none unless the
    singular search set it" at the slot where it can be checked;
  - `tt_pv` equals the node type or the table's flag outside an excluded
    search.
- **`test_stack_unwind`** is ON-only, like the other core tests, and under
  `_ALL_TARGETS`. It searches the first 20 bench positions at depth 8 with a
  fresh searcher and checks, at one thread:
  - the caller's board is unchanged (keys, occupancy, side, clock, history);
  - the best move, the line move by move and the ponder move are legal;
  - a fresh repeat is identical in nodes, score, best move and line.

  At `Threads 4` through `SearchThreadPool` it checks that the board is
  unchanged and the pool's answer is legal. It takes 1 s in release and 10 s
  in the sanitizer build.
- **Planted defects** (sanitizer build, `test_stack_unwind`, not
  committed):
  - Without the entry reset of `stat_score`, the test aborts with
    `node check failed: ss->stat_score == 0` (`i10/plant_stat_score.txt`
    `013a149d…0bbc`).
  - Without the parent's reset of `ss->reduction` after a reduced search,
    it aborts with `node check failed: ss->reduction == 0`
    (`plant_reduction.txt` `957bdb12…5aae`).
  - The restored tree passes.

#### Final state, both arms (engine `c5ad7bb`, tests `7f1b5bb`)

| Build | Off arm | ON arm |
|---|---:|---:|
| `bench 13`, release PEXT | 14,978,465 | 9,132,715 |
| `bench 13`, release plain | 14,978,465 | 9,132,715 |
| `bench 13`, Tune / Diag / Ablate | 14,978,465 each | 9,132,715 each |
| `bench 10`, release and sanitizer | 3,190,673 | 1,678,409 |
| Release CTest | 20/20 | 23/23 |
| Sanitizer CTest (`-LE strength`) | 18/18 | 21/21 |
| WAC depth 6 (`test_wac --floor`, 130) | 138 | 185 |
| WAC 100,000 nodes | 204 | 249 |

The ON arm's `bench 13` median is 120,520 per position; bench 39 holds
36.4% of the nodes (3,326,702).

What the delta review checks (§21.4):
- every fingerprint above and in §23.1, per commit, both arms;
- CTest on both arms;
- the changed lines of `31c80db`..`7f1b5bb` against §22.6 and §24.1;
- item 2's deviation, accepted in §24.3;
- item 10's placement of the checks;
- the planted-defect proof.

## 21. B.2.1 Review (2026-10-11): every fingerprint reproduced; not accepted; Return 3

- Reviewer: a separate `R2` session that did not implement the leaf, at head
  `c133876` (engine at `4fd8600`), against §12.2, Amendments 1 and 2, review
  §5.4–5.12 and the donor at `edb0d9db`.
- Verdict: **not accepted.** The leaf returns to `RESEARCH`. Two engine
  defects and one gap go back to implementation, and two contract questions
  need a research ruling first (21.4). B.2.2 does not start.

### 21.1 Reproduction

Recipe: one clean `git archive` per engine commit (`61c15cb`, `32e56ad`,
`f179404`, `ea5c49f`, `bcf63e8`, `221afdf`, `bb73fb5`, `573add0`, `b02fcb7`,
`802ed5c`, `4112db0`, `4fd8600`), configured with Ninja and the PATH's
`clang++` 22.1.8 (MSYS2 clang64; `MSYSTEM` unset so CMake resolves it), then
`basilisk bench 13` (sanitizer: `bench 10`). Scripts and output:
`tools/results/b21-review-20261011/` (`fp.sh` `db2245b3…ae2c`, `run_all.sh`
`6f869413…1a38`, `fingerprints.txt` `64a5f875…1749`).

- **All 149 readings equal the record.** The off arm reads 14,978,465 at
  every engine commit on release PEXT and plain, Tune, Diag and Ablate, and
  its sanitizer `bench 10` reads 3,190,673. The ON arm reads each ticket's
  row of §20 on PEXT and plain. Its release `bench 10` equals its sanitizer
  `bench 10` at every ticket. At the head its Tune, Diag and Ablate builds
  read 6,337,226.
- CTest at the head, built from the same sources:
  - release: off 20/20, ON 22/22;
  - sanitizer (`-LE strength`): off 18/18, ON 20/20;
  - `test_wac_floor` runs `--floor` on both arms: off 138, ON 185, against
    130.
- WAC at 100,000 nodes (`fixed_budget_probe.py`): off **204**, ON **249**,
  as recorded (`wac100k_off.json` `d5a027e5…455a`, `wac100k_final.json`
  `889aecb8…7fb2`; ON binary `28f957cf…c43c`).
- Ablation liveness on the ON Ablate build (`06791eab…c02d`): all 8 bits
  live (`ablation_liveness_on.json` `a9ed6c8e…1b4d`).
- The ON Tune build exposes 56 `Core*` options, the 50 coordinates and the
  6 switches. The switches stay off the SPSA surface: `test_spsa_surface`,
  `test_decision_trace` and `test_run_suite` pass (20/20).

### 21.2 What matches the handoff

Read against the donor line by line, all of the following match:

- **TT (§5.6):** the encoding, the occupancy byte and the miss-store, the
  rule-50 downgrade (equivalent to `value_from_tt`), the node-typed cutoff,
  the graph-history check, the cutoff bonus (decision 2) and the parent
  penalty.
- **SEE (17.1):** the quiet `see_ge` and its fast path.
- **Picker (§5.7):** the stage order, −v/18, −14000, the −3560·depth sort
  limit, the skip semantics with the direct-check exemption, and the
  threat-by-lesser array and quiet formula term for term.
- **Update routing:** continuation weights and consistency multipliers, the
  in-check two-ply limit, +73 at one ply, 712/750/1104/459, every constant
  of `update_all_stats`, the fail-low scale, eval-difference training and
  the post-LMR 1334.
- **Correction:** weights, sentinel, admission and bonus.
- **Node entry:** the order, hindsight (in check too, as the donor) and
  `improving` with in-check inheritance and the `≥ beta` rise.
- **Pruning:** razoring and RFP; the move-loop family on the
  history-adjusted `lmrDepth`, its gates and the stalemate guard.
- **LMR:** the formula and its terms, floor and ceiling, the
  deeper/shallower rule, `depth −= 3`, the fail-high blend and the
  equal-score promotion.
- **Ageing:** 729 and 102.

Deviations 1–10 of §20 are ordinary engineering choices inside the
contract, or decisions research already took, except where 21.3 says
otherwise. The invariants of §12.2 and review §5.12 hold as written:
- nothing is pruned or reduced at the root, in check or before a move was
  searched;
- the reduced depth is at least one ply;
- TT moves are validated at every consumer;
- correction training keeps its guards;
- the TT stores the raw static eval;
- the sentinel continuation row is never updated;
- no killers or countermove;
- counters and trace compile out of release;
- nothing allocates per node.

### 21.3 Findings

Each finding carries its evidence. A scratch tree changes only the line
named, from the head's sources (`variant_v1/v2/v3.patch`, `f6f5eb1a…3bb7`,
`3ea38dbb…e0e1`, `a71e4317…7ac1`), and reads `bench 13` on release PEXT.
The head reads 6,337,226 nodes, median 111,648 per position, and 249 WAC
solved at 100,000 nodes.

1. **Defect: `stat_score` is never reset at node entry.**
   - The donor zeroes `ss->statScore` in Step 1 (`search.cpp` line 809).
     Review §5.4 gives a default record zero counts. The core kernel writes
     `ss->stat_score` only in its move loop (`search_kernel_core.cpp`
     line 1020).
   - So a null-move child and a ProbCut child read, through `(ss - 1)->stat_score / 28` in `update_all_stats`, whatever move was last
     searched at that ply anywhere in the tree. The term can be about
     ±2,900 against a bonus cap of 1,487.
   - Scratch tree with `ss->stat_score = 0` after `ss->move_count = 0`
     (v1): 9,107,642 nodes. The total rises because one position takes
     55.9% of the nodes, but the median falls 20% to 88,871. WAC at 100k
     reads 246.
   - Not recorded. The contract is clear, so this goes back to
     implementation.
2. **Defect: the PV re-search's TT-move depth rule is inert.**
   - §3.7 adopts the donor's Step 20: at a PV node the TT move is searched
     at least one ply deep when its entry is deep or decisive, and the
     donor applies this to the first move (`moveCount == 1`, lines
     1409–1420).
   - The core kernel applies the rule only after a later move fails high
     (line 1137). The TT move is always the picker's first move, and the
     first-move branch (line 1064) never applies the rule. So at a PV node
     of depth 1 the TT move still drops into quiescence.
   - Scratch tree with the rule on the first-move branch (v3): 6,697,306
     nodes, median +11% (123,979); WAC 243.
   - The rule deepens and reduces nothing, so §12.2's "first move never
     reduced" does not forbid it. Not recorded; back to implementation.
3. **Contract gap: node typing of a non-PV node's first child.**
   - The core kernel searches every first move with `cut_node = false`
     (line 1065), kept from the legacy kernel. The donor passes `!cutNode`
     at non-PV nodes (Step 19, lines 1395–1404).
   - So the first child of an all node, the expected refutation and the
     donor's commonest cut node, is typed as an all node here. A chain of
     first moves below an all node never alternates.
   - The adopted consumers read the donor's population at the donor's
     values: the node-typed TT cutoff (`cut_node == (tt ≥ beta)` at
     depth ≤ 4), LMR's cut-node +4026 and +933, the all-node scaling
     ×276/(256d + 268), `cutoffCnt`'s all-node +1138 and the +940·cutNode
     TT-depth term.
   - Scratch tree with `!is_pv && !cut_node` there (v2): 6,260,593 nodes,
     median −4% (107,315); WAC 248.
   - The handoff never fixed this producer. §3.1 keeps `cut_node` runtime
     and leaves the null-move, ProbCut and singular children's typing to
     B.3, but the move loop is B.2's. Research rules (21.4).
4. **Contract question: two donor constants act on evaluation units
   unconverted.**
   - LMR's `r −= |corr|/26310` reads the correction in Basilisk's
     evaluation units, about 0.28× the donor's for the same error, so the
     term is about 0.28× the donor's. The divisor that keeps the donor's
     1024ths is about 7,420.
   - LMR's `alpha − eval` term converts its clamp to −18..27 (§3.7, §7.3)
     but keeps the slope 3 per unit. Its largest effect is 81/1024 against
     the donor's 288/1024. The converted slope, about 10.6, lies outside
     `CoreLmrAlphaEvalScale`'s 0–10.
   - §20 deviation 9 calls the fixed constants "all in 1024ths of a ply or
     history units"; `|corr|/26310` is not. The slope is a contract value
     (§7.3, review §5.10) that implementation followed, and the divisor is
     a fixed constant SPSA cannot reach. Research rules (21.4).
5. **Gap: the stack-unwind test.**
   - The leaf and review §5.11 item 5 ask for 20 positions at depth 8: the
     caller's board unchanged, legal best and ponder moves, a repeat search
     identical, and the same at `Threads 4`.
   - No such test was added, and §20's test list does not name one.
     Existing coverage is partial (`test_engine_threading`'s depth-10
     determinism and its 4T-then-1T check).
   - A determinism test would not have caught finding 1, whose stale read
     is deterministic, which is why the default-record clause needs a
     direct check. Back to implementation.
6. **Notes, no action required:**
   - At the root the picker adds the root-ordering score into the capture
     score that sets the `−score/18` SEE split, and into the quiet score
     the −14000 and −3560·depth limits read. Root only; ordering only.
   - The handoff's "register B.2.2–B.2.4 in `EXPERIMENTS.md` with P1–P6
     verbatim before the first game" is not done. No game has run; it is
     B.2.2's precondition, not a B.2.1 defect.

### 21.4 Return 3: what resumes, and on what

- **Research (Amendment 3) rules on 21.3 items 3 and 4.** The options for
  item 3:
  - (a) the donor's typing of the move loop's first child, with NMP,
    ProbCut and singular left to B.3;
  - (b) the legacy typing, with the consumers' constants re-seeded for its
    population;
  - (c) a categorical switch for B.2.2.

  For item 4, it decides whether the two constants convert as evaluation
  units: divisor about 7,420; slope about 10.6, with the range widened.
  - The reviewer's reading: (a) for item 3, because the consumers were
    adopted at the donor's values and the donor's population is what they
    assume. Convert both constants in item 4, by §7.2 rule 2's logic.
- **Implementation then lands, on the amended contract:** item 1 (zero the
  record's `stat_score` at node entry), item 2 (the TT-move rule on the
  first-move branch), item 5 (the stack-unwind test, with a direct check
  that the record's per-node fields start at their defaults), and what
  Amendment 3 decides. One commit per item, with the ON arm's `bench 13`
  and the 100,000-node WAC count recorded per commit, the off arm at
  14,978,465, and CTest on both arms.
- **A delta review then accepts those commits:** both arms' fingerprints
  reproduced, CTest, and the changed lines against the amended clauses.
  21.1 and 21.2 stand for everything else.

## 22. B.2.1 Amendment 3 (2026-10-11): the first child typed as the donor's, three evaluation-unit conversions, two more defects found; the review's findings confirmed

Answers §21. Research (`R3`) at head `b3d6e64`, against the donor
`edb0d9db` (`src/search.cpp` line numbers below are that file's).
Implementation lands the items of 22.6 in order; a delta review accepts
them.

### 22.1 Q1: the node type of a non-PV node's first child — (a)

**Decision.** The move loop's first child at a non-PV node is typed as the
donor's Step 19 types it: `!cut_node` (lines 1395–1404). At a PV node the
first child stays a PV child, untyped. Null-move, ProbCut and singular
children keep their legacy typing until B.3, as §3.1 and the review say.

Why. The adopted consumers read the donor's population at the donor's
values: the node-typed TT cutoff, LMR's +4026 and +933, the +940 TT-depth
term, the all-node scaling and `cutoffCnt`'s +1138 (§3.2, §3.7). Under the
legacy typing the first child of an all node is typed all, and so is the
first child of that child, so every leftmost chain below an all node is
all-typed when the donor expects it to alternate; the all-node scaling
and `cutoffCnt`'s all-node term then fire where the donor fires its
cut-node terms. (b) would re-seed the constants for a population whose
labels are wrong, which no fit repairs, and (c) would spend a 2,000-game
run on a question alpha-beta answers: the expected refutation of a move at
an all node is a cut node. The reviewer's evidence is the cheap
confirmation: −4% median nodes and one WAC position at 100,000 nodes
(249 → 248), a tree that got slightly narrower without losing tactics.
Counter-argument: Basilisk's legacy consumers (cut +401) were fitted to the
legacy typing; they are gone, so nothing is kept for them.

**With it, the donor's step-down on the first move (22.5, N2).** Step 19
also reduces the first move at a non-PV node by `(r > 5234) + (r > 5487 &&
newDepth > 2)`; the core kernel applies that step-down only to later moves
at depth 1, because §12.2's invariant "nothing reduced before one move is
searched" was read as forbidding it (deviation 9). The invariant meant the
donor's own gates (no LMR at `moveCount == 1`, no pruning before a move is
searched), not a mechanism the cluster adopted. Amended: at a non-PV node
that is not the root and not in check, the first move is searched as the
donor's Step 19, with the step-down and the `!cut_node` child. Evidence in
22.5.

### 22.2 Q2: constants that act on evaluation units — convert, by a stated rule

**Rule 4, added to §7.2.** An evaluation difference that is multiplied
into another unit (history, 1024ths of a ply) is scaled into the donor's
evaluation units, `× 1000 / 282` (§7.1), before the donor's constants,
which stay as written; a constant compared against an evaluation
difference converts by rule 3 (hindsight 47, the fail-low thresholds 30
and 19, the re-search thresholds 15 and 2, all already so). Where the
scaled product could overflow `int`, the constant converts instead.

Three terms fall under it, not two:

1. LMR `r −= |corr| / 26310` (line 1324). `corr` is Basilisk's correction
   in evaluation units × 131072, about 0.282× the donor's for the same
   error, so the term is 0.28× the donor's. Scaling `|corr|` would
   overflow, so the constant converts: **`|corr| / 7419`** (26310 × 0.282),
   fixed, as the donor's is. §20 deviation 9's "all in 1024ths of a ply or
   history units" is corrected in place.
2. LMR `3 · clamp(alpha − eval, −64, 96)` (lines 1354–1355). §7.3
   converted the clamp to −18..27 and left the slope at 3, so the term's
   largest effect is 81/1024 against the donor's 288/1024. Amended to
   **`CoreLmrAlphaEvalScale · clamp((alpha − eval) × 1000 / 282, −64, 96)`**:
   the donor's clamp on the scaled difference, the coordinate still the
   donor's slope 3 with its registered range 0–10 (review §5.10's label
   changes; its seed and range do not). The product is bounded by
   32,000 × 1000 and cannot overflow.
3. The eval-difference training (lines 981–985): `evalDiff =
   clamp(−(prev + cur), −189, 194) + 60`, then `× 11` into the main history
   and `× 13` into the pawn history. The clamp bounds and the offset are
   evaluation units, so Amendment 1's decision 5 was wrong to fix them at
   the donor's values on Basilisk's sum: the bonus is 0.28× the donor's and
   the clamp almost never binds. Amended to
   **`clamp(−(prev + cur) × 1000 / 282, −189, 194) + 60`**, slopes 11 and 13
   unchanged. The product is bounded by 64,000 × 1000.

Not under it: RFP's correction term is evaluation units on both sides
(`|raw| / 131072` against a margin), so it is unit-free; but its integer
divisor `CoreRfpCorrDiv` cannot express the donor's 131072/198435 = 0.66,
only 1.0 or 0.5. Before B.2.3 moves it, it is re-parametrised as
**`CoreRfpCorrMult`, 1024ths, seed 0, range 0–2048** (the donor 676), which
is behaviour-neutral at the seed (the ON fingerprint must not move).
The correction update's clamp and limit are correction units
(evaluation × depth) on both sides and stay; Basilisk saturates them at a
3.5× larger relative error than the donor, which is noted for B.2.3, not
changed.

Counter-argument: the three conversions were never measured, and the
donor's values may be no better here than the unconverted ones. True, and
it is why the seeds are seeds: B.2.3's curvature sweep spans all three
columns. What rule 4 fixes is a unit error, not a value.

### 22.3 F1, F2 and F5 — confirmed

- **F1, confirmed.** The donor zeroes `ss->statScore` at Step 1 (line 809)
  and the stack contract (review §5.4) gives a default record zero counts.
  The core kernel writes `ss->stat_score` only in the move loop, so a
  null-move or ProbCut child reads, through `(ss − 1)->stat_score / 28` in
  `update_all_stats` and `/ 98` in the fail-low scale, the last move
  searched at that ply anywhere in the tree. Fix: `ss->stat_score = 0` at
  node entry beside `ss->move_count = 0`. The same-slot re-entries (the
  singular search, the null-move verification) zero it again as the donor's
  do.
- **F2, confirmed.** §3.7 adopts Step 20 whole: at a PV node, when
  `moveCount == 1 || value > alpha`, the TT move's `newDepth` is raised to
  1 if its entry is deep (`ttData.depth > 1`) or decisive with depth > 0
  (lines 1409–1420). The core kernel applies the rule only on the
  re-search branch, and the TT move is first, so it never fires; at a
  depth-1 PV node the TT move drops into quiescence. Fix: apply the rule on
  the first-move branch at PV nodes. It deepens; §12.2's invariant is not
  touched. The reviewer's 243 at 100,000 nodes (from 249) is recorded as an
  instrument reading for B.2.2, not a decision input: the rule exists for
  mate finding and decisive scores, which the 100k count under-weights.
- **F5, confirmed.** The stack-unwind test of review §5.11 item 5 is owed:
  20 fixed positions at depth 8 on a `Board` passed by value, the caller's
  board unchanged, legal best and ponder moves, a repeat search identical
  in nodes and line, the same at `Threads 4`. Its direct check of the
  per-node defaults covers, at every node entry on the ON arm: `in_check`
  set, `move_count` 0, `stat_score` 0, `reduction` 0, `excluded` none
  unless the singular search set it, `(ss + 2)->cutoff_cnt` 0, and
  `tt_pv` false until the TT or the node type sets it. The cheapest form is
  a debug-only assertion block at node entry, compiled under
  `BASILISK_DIAGNOSTIC`, run by the test at depth 8 over the 20 positions;
  a planted stale field must fail it.

### 22.4 Discovered checks (Amendment 2 §19.2) and the review's notes

Nothing further: the root-ordering mix into the picker's split thresholds
is root-only and ordering-only, as the reviewer says; it is recorded here
as a B.5 (root) note. The B.2.2–B.2.4 registration is done in this change
(22.7).

### 22.5 Two defects the review did not name

Evidence: scratch trees from the head's sources, ON arm, release PEXT,
`bench 13` and the 100,000-node WAC count on `src/wac.epd` (recipe and
logs in `tools/results/b21-20261011-am3/`; the head reproduces 6,337,226
and 249 from the same recipe).

- **N1 (defect): the per-ply move count survives same-slot re-entries.**
  The donor counts moves in a local (`int moveCount = 0`, line 1116) and
  publishes it, `ss->moveCount = ++moveCount` (line 1137), so a search
  re-entered at the same stack slot (the singular search, the null-move
  verification) leaves the outer loop's count intact. The core kernel
  increments the record itself (`++ss->move_count`), and both re-entries
  run `ss->move_count = 0` and their own loop at the same slot. After a
  singular search the outer node's later moves count from the inner
  loop's final count, so count pruning (`move_count ≥ (3 + d²)/(2 −
  improving)`, then `skip_quiets`), the reduction table's index,
  `−65·moveCount`, the 32-slot searched lists and the children's reads of
  `(ss − 1)->move_count` all see an inflated count at exactly the nodes
  where a singular TT move exists. The null-move verification does the same
  for every node whose verification fails. Fix: the donor's local counter,
  `ss->move_count = ++move_count`. Evidence (vA): `bench 13` 5,960,990 (−5.9%),
  100k WAC 240 (−9).
- **N2 (gap): the donor's step-down on the first move at non-PV nodes is
  absent** (22.1). Evidence (vB, the step-down alone, typing unchanged):
  `bench 13` 5,499,163 (−13.2%), 100k WAC 244 (−5). The reviewer's typing-only tree
  (vC) reads 6,260,593 and 248.

Both are ordinary engineering against a clear donor clause; the leaf does
not return for them.

### 22.6 Handoff, amended: what implementation lands

One engine commit per item, in this order, each recording the ON arm's
`bench 13` (release PEXT = plain), its sanitizer `bench 10`, the
100,000-node WAC count, CTest on both arms (release and sanitizer) and the
off arm at 14,978,465 on every flavour; then the test commit; then the
records. The order is for attribution only.

1. F1: `ss->stat_score = 0` at node entry.
2. N1: the local move counter, `ss->move_count = ++move_count`.
3. Q1: the first child at a non-PV node typed `!cut_node`.
4. N2: the step-down on the first move at non-PV nodes (not root, not in
   check), `(r > 5234) + (r > 5487 && new_depth > 2)` with the no-TT-move
   +1127, as the later-move branch already computes it.
5. F2: Step 20's TT-move depth rule on the first-move branch at PV nodes.
6. Q2.1: `|corr| / 7419` in LMR.
7. Q2.2: the alpha term on the scaled difference with the donor's clamp.
8. Q2.3: the eval-difference training on the scaled sum.
9. `CoreRfpCorrMult` in place of `CoreRfpCorrDiv` (behaviour-neutral:
   the ON fingerprint of item 8 must hold; the SPSA generator and the
   parsers' tests follow the rename).
10. F5: the stack-unwind test and the per-node default assertions (test
    commit; the diagnostic block in the engine commit it needs, if any,
    fingerprint-neutral on both arms).
11. Records: §20's table gains one row per item and a new final-state
    table; deviation 9 corrected; PLAN's B.2.1 line; GUIDE. Then the delta
    review (§21.4).

Invariants amended: §12.2's "nothing pruned or reduced … before one move
is searched" now reads "… except the donor's Step 19 step-down of the
first move at a non-PV node (Amendment 3)". Everything else in §12.2 and
review §5.12 stands.

### 22.7 Predictions and screens

P1–P6 and §10's screens need no re-registration: none names a quantity
these items redefine, and all are read at B.2.2 on the accepted head. The
instrument readings per item (22.6) are recorded, not acted on. The
B.2.2–B.2.4 registration the handoff requires (§12.2) is made in this
change: BAS-S21 (B.2.2 screens and the unfitted paired run, P1–P6 verbatim
with Amendment 1's P5), BAS-S22 (B.2.3) and BAS-S23 (B.2.4), all
registered, no result.

## 23. B.2.1 Return 4 (2026-10-11): items 1–5 of 22.6 landed; item 6's converted divisor explodes the search in saturated positions; stopped

- State: implementation (`I2`) stopped before committing item 6, as 22.6's
  rules require when a fingerprint behaves in a way §22 did not anticipate.
  The leaf returns to `RESEARCH`. Items 1–5 are committed on `dev`
  (`31c80db`..`90d7db7`). Items 6–11 are not landed.
- Recipe: `tools/results/b21-r3-20261011/`.
  - `qual.sh` (`6813d6fc…d0b8`) builds both arms in every flavour from the
    working tree, reads every bench, and runs CTest on both arms (release,
    and sanitizer `-LE strength`) and the 100,000-node WAC probe on the ON
    release binary.
  - `check.py` asserts the off arm at 14,978,465 / 3,190,673, every ON
    flavour equal, every exit code 0 and every suite at 100%.
  - `edit.py` applies each edit with its anchor asserted once, keeping CRLF.
  - The unchanged head reproduces 6,337,226 and 249 through the same recipe
    (`base/`).

### 23.1 Items 1–5

Every row: the off arm reads 14,978,465 on release PEXT and plain, Tune,
Diag and Ablate, with `bench 10` 3,190,673 on PEXT, plain and sanitizer. The
ON arm's `bench 13` is equal on PEXT, plain, Tune, Diag and Ablate, and its
`bench 10` is equal on PEXT, plain and sanitizer. CTest passes on both arms:
release ON 22/22 and off 20/20, sanitizer ON 20/20 and off 18/18.

| Item | Commit | ON `bench 13` | ON `bench 10` (release = sanitizer) | ON WAC depth 6 (floor 130) | ON WAC 100k |
|---|---|---:|---:|---:|---:|
| head (`4fd8600` engine) | — | 6,337,226 | 1,490,974 | 185 | 249 |
| 1 F1, `stat_score` zeroed at entry | `31c80db` | 9,107,642 | 1,847,085 | 186 | 246 |
| 2 N1, local move counter | `008f8fc` | 7,618,212 | 1,779,010 | 185 | 244 |
| 3 Q1, first child `!cut_node` | `dc989af` | 6,842,239 | 1,525,428 | 186 | 247 |
| 4 N2, first-move step-down | `2c86329` | 6,276,484 | 1,369,386 | 172 | 245 |
| 5 F2, PV TT-move rule on the first move | `90d7db7` | 5,941,186 | 1,277,228 | 172 | 250 |

The 100k-node files are `i1`–`i5/wac100k_on.json` (`a2ff3aaa…87b5`,
`0e8bcecf…4792`, `8a1d0d39…9faa`, `8fc693ae…fef9`, `253bb89c…8f93`).
Item 1 reproduces the review's scratch reading exactly (9,107,642, 246).

**One deviation, item 2.** The named edit (vA) replaces only the increment.
The kernel's move loop also reads `ss->move_count` after the singular
search, in the same call: `r −= 65·moveCount` and the 32-slot searched-list
cap. There the singular search has overwritten the record, so those reads
would still see the inner loop's count. The donor reads its local
`moveCount` at both (lines 1323 and 1545), and 22.5 names both consumers.
The commit therefore reads the local at those two lines as well. At
`bench 13` it is identical to vA alone (7,618,212 nodes, median 110,733),
because no consumer of the first move's `r` existed until item 4. The trace
lines keep reading the record.

### 23.2 The failure: item 6, `|corr| / 7419`

- With item 6 applied to item 5, the ON arm's `bench 13` did not finish:
  1,205 CPU seconds against about 2 seconds, killed.
- `bench 8`: 511,700 → 1,361,570 nodes (`bench8_i5.txt` `160c5e4e…a43f`,
  `bench8_i6.txt` `a679961c…93b3`), median +9% (10,907 → 11,865).
- One position carries the blow-up: bench 39,
  `1Q4R1/5k2/4rpp1/3K4/8/7p/8/8 b - - 2 9`, a lost ending for the side to
  move. It takes 34,165 → 747,335 nodes at depth 8 (×21.9), and from
  depth 9 it does not finish within 120 s.
- Diag counters on that position, `go nodes 3000000` (ON Diag builds of
  items 5 and 6):

| | Item 5 | Item 6 |
|---|---:|---:|
| depth reached | 13 | 8 |
| `lmr_extended` (negative reductions) | 8,652 | 160,463 |
| `lmr_eligible` | 887,146 | 1,024,150 |
| extended / eligible | 1.0% | 15.7% |
| `lmr_applied` | 129,522 | 58,928 |

Cause:
- 22.2 converted the divisor on the premise that Basilisk's correction is
  about 0.282× the donor's for the same error. That holds below saturation.
- The correction tables keep the donor's limit, 1024, in Basilisk's units,
  so at saturation `|correction_raw|` reaches the donor's maximum:
  (15341 + 10569 + 2·12906)·1024 + 8761·2·1024 ≈ 70.9M.
- At that maximum, 7,419 subtracts up to 9,557/1024 ≈ 9.3 plies, against the
  donor's 2,695 (2.6 plies). 22.2 notes this 3.5× saturation difference for
  the update clamp but did not carry it into this conversion.
- In a lopsided ending where the correction saturates, most reductions turn
  negative. With `CoreLmrNegativeAllowed` the moves are searched up to two
  plies deeper, and the extensions compound.
- Item 7's alpha term and item 8's eval-difference term are clamped after
  scaling, so they stay inside the donor's bounds.

Kept: items 1–5 committed, the item-6 patch (`i6/item6.patch`
`44d4521d…1ce1`) and its two binaries. The working tree is at `90d7db7`.

### 23.3 Options

- (a) Keep the donor's 26310. The term then is 0.28× the donor's below
  saturation and equals the donor's cap at saturation.
- (b) Convert to 7,419 and cap the term at the donor's largest value
  (`min(|corr|/7419, 2695)`). That is the converted slope with the donor's
  bound: one more constant, but no new mechanism.
- (c) Rescale the correction tables' limit (or weights) so that saturation
  sits where the donor's does. This changes every consumer of the
  correction and the update clamp, beyond this item.
- (d) As (a), and make the divisor a B.2.3 coordinate spanning 7,419–26,310,
  with the run-away recorded as the reason its low end is bounded.

The implementation's reading is (b). It applies rule 4 where the correction
is unsaturated, which is where 22.2's premise holds, and keeps the donor's
bound where the premise fails. Scratch evidence (`scratch-i6b/`, item 5
plus the capped term, ON release PEXT `19102a39…`): `bench 13` terminates
at 7,644,681 (item 5: 5,941,186). The median is +8% (102,158 → 110,508)
and the top position carries 34.0% (item 5: 25.4%). WAC at 100k reads 248.
The main counter-argument is that it adds a fixed constant the donor does
not have.

Not affected: items 7–10 do not depend on this decision except through the
fingerprints, and they resume in 22.6's order once Amendment 4 fixes item 6.

## 24. B.2.1 Amendment 4 (2026-10-12): item 6 lands as the converted slope under the donor's bound; rule 4 gains a saturation clause

Answers §23. Research (`R3`) at head `17767a9`. One decision; nothing
else is reopened.

### 24.1 Decision: option (b)

**Item 6 lands as**

```
r -= std::min(std::abs(correction_raw) / 7419, 2695);
```

both constants fixed, as the donor's 26310 is. 7,419 is rule 4's
conversion of the slope (26310 × 0.282); 2,695 is the donor's own largest
value of the term, `(15341 + 10569 + 2·12906 + 2·8761) · 1024 / 26310`
(70,905,856 / 26310), not a new number. No coordinate: the slope is a
fixed donor constant (review §5.10) and the bound is derived from the
donor's table limit, so neither is B.2.3's to move.

Why (b). The failure is not the slope: below saturation 7,419 maps a
Basilisk correction of 100 cp to the same 1.7 plies the donor gives its
equivalent 355 units, which is what 22.2 asked for and what the item-5
tree shows elsewhere (median +8%). The failure is the domain: Basilisk's
correction tables keep the donor's limit 1024 and clamp ±256 in Basilisk's
units (`history_core.h` `HistoryBound<1024>`; the update at
`search_kernel_core.cpp` line 1297), so they saturate at a relative error
3.5× the donor's and `|correction_raw|` reaches the donor's maximum of
70.9M, which the converted slope turns into 9.3 plies of negative
reduction. Capping the term at the donor's largest value restores the
donor's range exactly where the premise of 22.2 fails, and leaves the
converted slope where it holds. The evidence is §23.3's scratch tree:
`bench 13` terminates at 7,644,681 on item 5's 5,941,186, median +8%,
WAC 248 at 100,000 nodes, one position at 34.0% of the nodes.

Why not the others.
- (a) keeps a term that is 0.28× the donor's wherever the correction is
  unsaturated, which is almost everywhere; it fixes the blow-up by
  keeping the unit error.
- (d) makes a coordinate of a fixed donor constant and asks SPSA to find
  the point where the search stops exploding; the bound is the right
  tool for a bound.
- (c) is the right long-term correction and the wrong change for this
  item. The root cause is an unconverted evaluation-unit constant of the
  Q2 kind: §7.1 lists "correction bounds" under evaluation units (0.282)
  and §7.3's row keeps the clamp and limit at the donor's values with the
  reasoning that the output scales with the raw eval, which is true only
  below saturation. Converting them (limit 1024 → 289, clamp ±256 → ±72)
  would make every consumer of the correction donor-equivalent, the
  static-eval correction included, whose reach is today 5.4 Basilisk
  pawns against the donor's 2.6 of its own. That changes the evaluation's
  behaviour in exactly the saturated positions (lost endings, large
  persistent eval errors) where B.0 never measured it, and nothing in this
  leaf's evidence says which bound is better for a classical evaluation
  whose errors are larger than the donor's. It is registered below as a
  B.2.3 question with its cheapest reading, not landed here.

Counter-argument to (b): a fixed constant the donor does not write. It is
the donor's constant made explicit, and the saturation clause of 24.2 is
what makes it a rule rather than a patch. If (c) is ever landed, 2,695
becomes unreachable and the `min` is dead but harmless.

Registered for B.2.3 (not a coordinate on the SPSA surface): a
categorical `CoreCorrLimit` of 1024 against 289, with the update clamp at
limit/4, decided by the 2,000-game paired run like the other switches,
only if B.2.2's per-position bench and time-to-depth screens show
saturated positions carrying the tree (bench 39 holds 25% of the nodes at
item 5 and 34% with item 6). Its cheapest prior reading is the Diag
counter of saturated correction reads per position, which B.2.2 may add.

### 24.2 Side question 1: rule 4 gains a saturation clause

Yes. §7.2 rule 4 now ends: "A scaled term whose source is a bounded table
(a history or correction entry) keeps the donor's bound on the scaled
product: scale, then clamp at the value the donor's own limit allows."
Items 7 and 8 already satisfy it, because their clamps are applied after
scaling and the clamp bounds are the donor's. Item 6 did not, because the
donor's bound there is implicit in its table limit; 24.1 makes it
explicit. Terms B.2.3 or B.3 may convert later: the singular margin's
`|corr| / 198368` converts to 55,940 with the cap 70,905,856 / 198368 =
357, and the RFP term is unit-free (22.2). History-fed terms are bounded
by the history limits, which are unconverted by rule 1, so the clause
binds nothing there.

### 24.3 Side question 2: the item-2 deviation is accepted

Reading the local counter at `r −= 65·moveCount` and the searched-list
cap, as well as at the increment, is the donor's reading (lines 1323 and
1545) and what 22.5 named; the vA edit was the minimum, not the contract.
The trace lines reading the record is fine: the trace reports the record.

### 24.4 Predictions and screens

None re-registered: P1–P6 (BAS-S21), BAS-S22, BAS-S23 and §10's screens are
read on the accepted head. The per-item instrument readings continue in
§20.

### 24.5 Handoff, resumed: items 6–11 of 22.6 as amended

Same per-commit qualification as 22.6 (ON `bench 13` PEXT = plain, ON
sanitizer `bench 10` = release, 100,000-node WAC, CTest on both arms
release and sanitizer, the off arm at 14,978,465 on every flavour).

6. Q2.1 as 24.1: `r -= std::min(std::abs(correction_raw) / 7419, 2695);`
   with a comment naming both derivations. Expected near §23.3's scratch
   reading on item 5 (7,644,681; 248).
7. Q2.2: the alpha term on the scaled difference with the donor's clamp
   (22.6 item 7), unchanged.
8. Q2.3: the eval-difference training on the scaled sum (22.6 item 8),
   unchanged.
9. `CoreRfpCorrMult` (22.6 item 9), behaviour-neutral at the seed,
   unchanged.
10. F5: the stack-unwind test and the per-node default assertions (22.6
    item 10), unchanged.
11. Records: §20's rows for items 6–10 and the final-state table; PLAN's
    B.2.1 line; GUIDE; the checkers. Then the delta review (§21.4).

A fingerprint that behaves in a way §22 or this amendment did not
anticipate still stops the work with a Return, as 22.6 says.

## 25. B.2.1 Delta review (2026-10-12): items 1–10 reproduced and checked; accepted with two named follow-ups

- Reviewer: a separate `R2` session that neither implemented nor researched
  items 1–10, at head `25d709a` (engine `c5ad7bb`, tests `7f1b5bb`),
  against §22.6, §24.1–24.3, §12.2 as amended, review §5.12 and the donor
  at `edb0d9db`. §21.1 and §21.2 stand for everything these commits do not
  touch.
- Verdict: **accepted**, with two follow-ups named in 25.4 that bind
  nothing in this leaf. B.2.1 closes; B.2.2 starts on engine `c5ad7bb`
  (tests `7f1b5bb`).

### 25.1 Reproduction

Recipe: `tools/results/b21-delta-review-20261012/repro.sh` (SHA-256
`f300b681…`), the reviewer's own: one clean `git archive` per commit into
`src/<label>`, configured with Ninja and the PATH's `clang++` with
`MSYSTEM` unset, built per flavour into `build/<label>-<arm>-<flavour>`,
`basilisk bench 13` (sanitizer `bench 10`) read from the binary, every
line appended to `fingerprints.txt` (`06260842…`). Logs under `logs/`.

- **Every fingerprint equals the record** (`fingerprints.txt`, 86
  readings). Per commit `31c80db`..`c5ad7bb`: the ON arm on release PEXT
  and plain reads §23.1's and the §20 addendum's `bench 13` and `bench 10`,
  its sanitizer `bench 10` equals its release `bench 10`, and the off arm on
  PEXT reads 14,978,465 / 3,190,673. At `7f1b5bb` all six flavours of both
  arms: ON 9,132,715 on PEXT, plain, Tune, Diag and Ablate with sanitizer
  1,678,409; off 14,978,465 on all five with sanitizer 3,190,673.
- CTest at `7f1b5bb`, built from the same sources: ON release 23/23 and sanitizer (`-LE strength`) 21/21; off release 20/20 and sanitizer 18/18, every exit code 0.
- WAC at 100,000 nodes at `7f1b5bb` (`fixed_budget_probe.py`,
  `wac100k-ON.json` `8b607e0a…`, `wac100k-OFF.json` `fd8e7b6f…`): ON
  **249/300**, off **204/300**, as recorded.
- The implementer's artifacts are where §20 says, with the prefixes it
  cites (`i6`–`i10b/wac100k_on.json`, `i10/plant_*.txt`).

### 25.2 Items 1–10 against their clauses

Each commit changes only the lines its clause names; no deviation beyond
the one §24.3 accepted.

| Item | Commit | Clause | Donor line | Finding |
|---|---|---|---|---|
| 1 | `31c80db` | §22.6 item 1 | 809 | `ss->stat_score = 0` beside `ss->move_count = 0` at entry. Exact. |
| 2 | `008f8fc` | §22.6 item 2, §24.3 | 1116, 1137, 1323, 1545 | Local `move_count`, published by `ss->move_count = ++move_count`; the local read at `−65·moveCount` and the searched-list cap (the accepted deviation). Complete: the only other parent-loop reads are `lmr_reduction(…, ss->move_count, …)` on the line after the publish, the pruning block's `const int move_count = ss->move_count` (before the singular search, so equal to the local) and the trace, which reports the record as §24.3 allows. The children's `(ss − 1)->move_count` after a singular search read the inner loop's count, as the donor's do. |
| 3 | `dc989af` | §22.1, §22.6 item 3 | 1395–1404 | The first-move branch passes `!is_pv && !cut_node`; PV children untyped; NMP, ProbCut and singular untouched (B.3). Exact. |
| 4 | `2c86329` | §22.1, §22.6 item 4 | 1395–1404 | `step = (rr > 5234) + (rr > 5487 && new_depth > 2)` with `rr = r + 1127·(no TT move)`, under `!is_pv && !ABLATED(7) && !is_root && !in_check`, the same gate and arithmetic as the later-move branch. The first move's `r` carries the TT-move −2179 and the −65, as the donor's. At `new_depth` 1 the step can reach quiescence, as the donor's `newDepth − 1` does; the "reduced depth at least one ply" invariant is LMR's floor, untouched. Exact. |
| 5 | `90d7db7` | §22.3 F2, §22.6 item 5 | 1409–1420 | On the first-move branch at PV nodes: `m == tt_move && ((tt_score valid && decisive && tt_depth > 0) || tt_depth > 1)` raises `new_depth` to 1. `tt_depth` is 0 on a miss and `tt_move` none, so the rule cannot fire on a foreign move; the move is the picker's validated TT move. The re-search copy stays. Exact. |
| 6 | `45f4827` | §24.1 | 1324 | `r −= std::min(std::abs(correction_raw) / 7419, 2695)` with the comment naming both derivations. Exact. The ON fingerprint is §23.3's scratch reading. |
| 7 | `caee6ed` | §22.2 item 2, §22.6 item 7 | 1354–1355 | `kp.lmr_alpha_eval_scale · clamp((alpha − lmr_eval) · 1000 / 282, −64, 96)`; seed 3, range 0–10 unchanged; the core-table comment updated. In check `lmr_eval` can be `VALUE_NONE`, giving the clamp's floor, which is the donor's own arithmetic. The product is bounded by about 64,000 × 1000. Exact, and bounded by the donor's clamp (rule 4's clause). |
| 8 | `e0f7cc8` | §22.2 item 3, §22.6 item 8 | 981–985 | `clamp(−((ss − 1)->eval + ss->eval) · 1000 / 282, −189, 194) + 60`, slopes 11 and 13 unchanged, the existing guards kept (not in check, parent not in check, no capture, a real previous move). Bounded as item 7. Exact. |
| 9 | `e13ad68` | §22.2, §22.6 item 9 | — | `CoreRfpCorrMult`, 1024ths, seed 0, range 0–2048; `margin += |corr| / 131072 · mult / 1024`, divided first. No `CoreRfpCorrDiv` remains in `src`, `tools` or `tests`; the three parser tests pass (20 passed); the core table stays off the SPSA surface. Neutral at the seed (the fingerprint of item 8 holds through `c5ad7bb`). Live: on the reviewer's `final-ON-tune` build (Italian position, depth 12) 63,391 nodes at 0, 49,793 at 676 and 52,949 at 2048 (`liveness.txt` `b1b9ddb4…`). |
| 10 | `c5ad7bb` | §22.3 F5, §22.6 item 10 | — | `CORE_NODE_CHECK` after the TT probe, under `BASILISK_DIAGNOSTIC || !NDEBUG`: live in the Diag flavour and every build without `NDEBUG` (the sanitizer suite), compiled out of release, Tune and Ablate, whose equal fingerprints agree. The placement is the right call: §22.3 named `BASILISK_DIAGNOSTIC` as the cheapest form, and a check that no CTest build runs would never run. |
| 10 | `7f1b5bb` | review §5.11 item 5 | — | `test_stack_unwind`: 20 bench positions at depth 8, the board image (keys, occupancy, side, clock, history) unchanged, best move, line and ponder legal where played, a fresh repeat identical in nodes, score, best move and line, and at `Threads 4` through the pool the board unchanged and the answer legal. ON-only, under `_ALL_TARGETS`. Covers the clause. |

**The seven node checks, judged.** Two state invariants of the whole
search: `ss->reduction == 0` (every reduced search at this slot restored
the field, including through early returns of the child) and
`(ss + 1)->excluded == MOVE_NONE` (every singular search below restored
its exclusion). The other five (`in_check`, `move_count`, `stat_score`,
`(ss + 2)->cutoff_cnt`, `tt_pv`) are evaluated a few lines after the
assignments that make them true, with no write between, so they can fail
only if one of those assignments is removed. They are regression guards on
the entry code, which is what §22.3 asked for ("a planted stale field must
fail it") and what the planted proof exercises; they are not tautologies in
the sense of being unfalsifiable, but they do not detect a stale field that
arrives from elsewhere. Recorded, not a defect (25.4).

**Planted-defect proof, re-run by the reviewer** (`plant.sh` `db488a22…`,
sanitizer builds of `7f1b5bb`'s sources with one line removed each):
- without `ss->stat_score = 0;` at entry: aborts with `node check failed: ss->stat_score == 0` (`plant-stat_score.txt` `8b1e5ad2…`);
- without the parent's `ss->reduction = 0;` after the reduced search:
  aborts with `node check failed: ss->reduction == 0` (`plant-reduction.txt` `775dd320…`);
- the clean `7f1b5bb` sanitizer `test_stack_unwind`: 5 / 5 sections pass (`clean-stack-unwind.txt` `7ee2f28b…`). The reviewer's first plant script failed to apply its edits and ran the test unplanted; it was corrected and both plants re-run, which is the record above.

### 25.3 Invariants on the changed code

§12.2 as amended by §22.6, and review §5.12:
- Nothing pruned or reduced at the root or in check: items 3–5 are gated
  by `!is_root && !in_check` where they reduce, and the typing does not
  reduce. The first-move step-down is the amended exception, as written.
- The reduced depth at least one ply: LMR's floor is untouched; the
  step-down reaches quiescence only where the donor's does.
- TT moves validated at every consumer: item 5 reads the picker's validated
  TT move; nothing else reads a TT move.
- Nothing allocated per node: the items add integers and a compiled-out
  macro; the test allocates in its own frames.
- Counters and trace out of release: `CORE_NODE_CHECK` is `((void)0)`
  under `NDEBUG` without `BASILISK_DIAGNOSTIC`; the release, Tune and
  Ablate fingerprints equal the Diag one.
- The off arm is untouched by every commit: 14,978,465 reproduced on all
  five flavours at `7f1b5bb` and on PEXT at every commit.

### 25.4 Information for B.2.2, and follow-ups that bind nothing here

- Item 7's growth (median +18%, bench 39 at 36.4% of `bench 13`) is the
  term reaching the donor's 288/1024 and is read by B.2.2's tree-size
  screens, not here. No cause outside the contract was found: the clamp
  bounds and slope are the donor's on the scaled difference.
- Follow-up 1 (B.2.3, already registered by §24.1): the correction bound's
  own conversion (`CoreCorrLimit` 1024 against 289); bench 39's share of the
  nodes is its cheapest reading.
- Follow-up 2 (any time, `I1`): if a later return ever needs the node
  checks to catch a stale field arriving from a sibling subtree, the five
  reset-guard checks should move before the resets and assert the parent's
  restores rather than this node's own writes; today's form satisfies
  §22.3 and is proven live.

### 25.5 Record

- Artifacts: `tools/results/b21-delta-review-20261012/` (ignored storage):
  `repro.sh`, `plant.sh`, `fingerprints.txt`, `wac100k-ON.json`,
  `wac100k-OFF.json`, `plant-stat_score.txt`, `plant-reduction.txt`,
  `clean-stack-unwind.txt`, `SHA256SUMS.txt`, `logs/`.
- PLAN: B.2.1 ticked and its register row removed (the checker refuses a
  row for a completed leaf). The next leaf is B.2.2 (`V`).

## 26. B.2.2 screens (2026-10-12): two floors flagged, P3, P4 and P5 partly missed, P6 held; the games prepared

- Leaf: B.2.2 (`V`), on the accepted head (engine `c5ad7bb`, tests
  `7f1b5bb`).
- Binaries: final-PGO builds via `build_test.ps1`, each read at `bench 13`
  before use, hashed and archived in
  `tools/results/b22-20261012/bin/` (`fingerprints.txt`).
  - Release ON ×2 and OFF ×2 at `84da91f`: ON pgo1 `9fed724f…`, pgo2
    `aba848c2…`; OFF pgo1 `ef503fc7…`, pgo2 `0e27f52d…`.
  - Tune, Diag and Ablate at `84fda58`, a tooling-only commit on the same
    engine: ON Tune `eb3ea560…`, ON Diag `ab65a50a…`, OFF Diag `19703e14…`,
    ON Ablate `634838e4…`.
  - Every manifest is clean: ON 9,132,715 and OFF 14,978,465 on every
    flavour.
- `84fda58` is a precondition fixed in its own commit: `build_test.ps1`
  recognised a tune build only by the legacy `RfpCoeff`, which the ON arm
  replaces, so an ON Tune PGO build could not be made.
- Recipes: `screens.sh`, `sweep.sh`, `nps_pool.ps1`; readers `analyze.py`
  and `analyze_sweep.py`; all in the same directory.
- Strength and NPS were measured only on Release builds, with counters from
  Diag builds.

### 26.1 The screens against §10

Each screen is read by its registered rule only.

| Screen | Instrument (artifact, SHA-256) | ON | off | Floor | Target | Verdict |
|---|---|---:|---:|---|---|---|
| Geometric branching 4–14, suite_v2 (103 positions) | `branching.py`, ON, off and oracle arms (`screens/branching.json` `4aa829ed…d706`) | **1.6465** | 1.767 | window [1.65, 1.90] | inside | **outside the lower edge by 0.0035: a defect flag** |
| Nodes at depth 12 relative to the oracle | same run | **1.86** (median **1.04**) | 3.97 (2.77) | ≤ 3.5 (≤ 2.6) | ≤ 2.5 (≤ 1.8) | target met |
| Shallow-pruning marginal factor at depth 12 | ON Ablate, mask 32 against 0 (`ablate_branching_mask32.json` `6381862b…f358`, `mask0` `8700bd10…2c29`) | **3.46** median (2.67 aggregate) | 2.20 | ≥ 2.0 | ≥ 3.5 | floor met, target missed by 0.04 |
| WAC at 100,000 nodes | `fixed_budget_probe.py` (`wac_nodes100k.json` `7d3a60c0…cdd7`) | **249** | 204 | ≥ 200 | ≥ 225 | target met |
| WAC at 400,000 nodes | same (`wac_nodes400k.json` `2e40a903…1ab8`) | **276** | 242 | ≥ 240 | ≥ 258 | target met |
| LMR-off WAC delta at 100k | ON Ablate, mask 128 against 0 (`ablate_wac100k.json` `94df0f16…8f34`) | **+8** (257 − 249) | +15 | ≤ +12 | ≤ +5 | floor met, target missed |
| Median depth at 300k | `run_suite.py --nodes 300000`, suite_v2 (`suite_d14_diff300k_on.json` `5728bc04…61ad`, `_off` `2d065b85…ec0e`) | **16** | 14 | monitored | must not rise while WAC falls | rises while WAC rises: as wanted |
| Oracle best-move agreement at 300k | same run | **66 / 105** | 66 / 105 | ≥ 60 | ≥ 75 | floor met, target missed |
| Time to depth, `bench 13` | bench nodes / pooled-PGO median NPS, interleaved (`nps/on-vs-off.txt` `b268688b…0f52`) | **2.13 s** (9,132,715 / 4.285M) | 3.59 s (14,978,465 / 4.171M) | ≤ 1.10× (4.0 s) | ≤ 0.85× (3.1 s) | **0.59×**, target met |
| Canaries | `canary.py check` with `canary_v1.json` (`canary_check.json` `446e52e3…b98e`) | **68 / 77**, 30 new passes | 77 / 77 | all 77 pass | all pass | **floor failed** |

Notes on the instruments:
- **Canary suite.** `canary_v1.json` pins its suite by SHA-256 `663d9da6…`.
  That is `src/wac.epd` with CRLF line endings: B.0's
  `wac_canary_suite.epd`, the same 300 positions byte for byte once
  normalised. The check therefore takes that copy as `--suite`, as B.0's
  did; the probes ran on `src/wac.epd`.
- **Self pair.** The NPS pool's self pair read **+0.28%**, 95% CI
  [−0.51%, +0.79%], SELF-PAIR OK (`nps/selfpair.txt` `b7e2e0e6…2c6a`).

### 26.2 The predictions read

- **P3 (nodes at depth 12 ≤ 2.5, median ≤ 1.8, with branching inside
  [1.65, 1.90]).**
  - The node ratio holds well inside the target: 1.86, median 1.04.
  - Branching misses the window's lower edge by 0.0035.
  - The falsifier (a ratio above 3.2) does not fire.
- **P4 (WAC at 100k ≥ 225 and the LMR-off delta ≤ +5).**
  - The WAC half holds: 249.
  - The LMR-off half misses: +8.
  - The falsifier (WAC under 210) does not fire.
- **P5 (pooled NPS 0.92–0.98× of the B.1 head).**
  - The ON pair against the B.1 pair, interleaved, reads **+3.81%**
    (1.038×), 95% CI [+3.42%, +4.49%]: median 4,284,644 against 4,127,440
    (`nps/on-vs-b1.txt` `dc676afe…c0b4`).
  - So the cluster is faster per node, not slower: P5 misses above its
    band, and its falsifier (below 0.90) does not fire.
  - The B.1 pair reads 2.1% below BAS-P16's 4.216M on today's host, which
    is why only the interleaved ratio is read.
  - The reading rule's inputs, from Diag `bench 13` summed by `run_suite.py`:
    `see_ge_calls` per node is 0.839 ON against 0.534 off (1.57×, under the
    2× trigger).
  - Against the off arm of the same revision the ON pair reads +2.69%, 95%
    CI [+2.40%, +3.18%]; the arms search different trees.
- **P6 (`lmr_researched / lmr_applied` 3–5%, `rfp_cuts` below 12% of
  interior nodes, while the tree shrinks): held.**
  - LMR re-searches 4.24% (31,264 / 737,738); off arm 1.91%.
  - RFP cuts 10.37% (572,537 / 5,522,985); off arm 12.86%.
  - Bench nodes 0.61× (`counters_bench13_on.json` `a3959b43…6935`,
    `_off.json` `96d5fbe8…d571`).
  - The LMR identity holds on both arms.
- **BAS-C09's retry trigger: not met.** BAS-C09's recipe was rebuilt in a
  scratch tree, recomputing pins on the current occupancy after every
  capture and comparing verdicts (`c09/c09_instrumentation.patch`
  `43a993d9…643e`, not committed).
  - The instrumented ON build still reads `bench 13` = 9,132,715.
  - Over 7,665,821 `see_ge` calls, 34 verdicts change (0.0004%), and 3,821
    calls (0.050%) reach a pawn recapture on a promotion rank, an upper
    bound on the promotion approximation (`c09/c09_counts.txt`
    `5c61befa…98e8`).
  - Both are well under the 0.1% trigger. The ON arm makes 22× BAS-C09's
    339,607 calls: quiet SEE and the picker's split.
- **§24.1's cheapest reading.** Bench 39 holds 36.4% of the ON arm's
  `bench 13` (3,326,702 of 9,132,715). Recorded, not acted on.

### 26.3 The floor failures, attributed (§2.6's sweep)

Each `AblationMask` bit alone on the ON Ablate build: suite_v2 depths
4–12, WAC at 100,000 nodes, and the depth-12 PV probe for a canary check per
mask (`sweep/summary.txt` `a0a76dab…854c`).

| Bit off | nodes ×, d4 | d8 | d12 | median d12 | branching 4–12 | WAC 100k | Δ | canaries |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| none (mask 0) | 1 | 1 | 1 | 1 | 1.692 | 249 | — | 68 |
| razoring | 0.990 | 1.019 | 0.959 | 1.00 | 1.685 | 256 | +7 | 68 |
| RFP | 1.346 | 1.247 | 1.193 | 1.25 | 1.666 | 234 | −15 | 67 |
| NMP | 1.001 | 1.200 | 1.480 | 1.00 | 1.776 | 247 | −2 | 68 |
| ProbCut | 0.994 | 1.095 | 0.784 | 1.00 | 1.642 | 253 | +4 | 66 |
| IIR | 1.074 | 1.170 | 1.169 | 1.17 | 1.710 | 244 | −5 | 69 |
| move-loop pruning | 4.807 | 3.703 | 2.673 | 3.46 | 1.572 | 216 | −33 | **73** |
| extensions | 0.839 | 0.546 | 0.362 | 0.71 | 1.523 | 231 | −18 | 64 |
| LMR | 0.859 | 1.092 | 1.595 | 1.66 | 1.828 | 257 | +8 | 69 |

**The canaries.**
- The nine required canaries that fail are WAC.031, 059, 067, 083, 151,
  201, 255, 273 and 279.
- Eight of the nine still solve within 100,000 nodes. They fail the depth
  half of the contract: the best move becomes stable some plies later than
  the oracle's anchor plus two (WAC.151 at depth 8 against a limit of 3,
  WAC.083 at 7 against 3). WAC.273 alone also misses at 100k.
- Removing the move-loop pruning family recovers eight of the nine (all but
  WAC.059), though it costs 33 WAC positions and loses three others
  (WAC.029, 124, 294).
- WAC.059 regresses under every mask, so no single family carries it.
- So the move-loop pruning family, the cluster's primary target (§5), is
  where the regressions live. It prunes the canaries' key moves at shallow
  depths and finds them later.
- The 30 new passes include WAC.001 (`Qg6`, the quiet mate threat B.0
  recorded as unsolved), now stable at depth 11 and solved at 100k.
- PROCESS says a changed canary is recorded with its cause and never
  re-blessed. This is that record.

**The branching flag.**
- ON's growth is 1.692 over depths 4–12, inside the window. It falls to
  1.478 over 12–14, against the oracle's 1.583 and the off arm's 1.530.
  Per depth, from 4→5 to 13→14: 1.86, 1.92, 1.70, 1.69, 1.61, 1.65, 1.56,
  1.57, 1.53, 1.43.
- Its node ratio to the oracle falls steadily from 3.12 at depth 4 to 1.63
  at depth 14.
- In the sweep, removing LMR raises branching to 1.828; every other bit
  stays inside or below 1.78.
- So the cluster's LMR, whose reduction grows with depth, carries the low
  edge.
- The window's lower edge is registered as a defect flag for over-pruning.
  The fixed-node screens read the other way: WAC +45 at 100k and +34 at
  400k, agreement unchanged, and depth at 300k up two plies.

### 26.4 The games, prepared (not started)

`tools/run_b22_games.ps1` (`526a68d`, SHA-256 `dee2afa4…`) wraps
`colosseum.ps1 -Mode match` on `match-fixed.toml`:
- 2,000 games, 1,000 paired UHO openings, `3+0.03`, one thread, Hash 64;
- no adjudication, tablebases off;
- concurrency 15, resolved by the wrapper on this host;
- every binary pinned by SHA-256 in the launcher (the wrapper checks
  manifest, revision and bench).

The runs, all seven dry runs accepted
(`tools/results/colosseum_match_b22-*_20261012_1507*.manifest.txt`):
- **`paired`, seed 22200.** ON release `9fed724f…` against OFF release
  `ef503fc7…`, revision `84da91f`. BAS-S21's rule: > −40 Elo proceeds,
  below is a defect; ≥ +10 proceeds to B.2.3; −40..+10 the review decides.
- **The six categorical runs, seeds 22201–22206.** One ON Tune build
  (`eb3ea560…`, `84fda58`); arm A plays the other value, arm B the
  default:
  - `CoreTtCutoffNodeTyped` 0 against 1;
  - `CorePawnHistBucketsLog2` 11 against 13;
  - `CoreCorrAdmission` 0 against 1;
  - `CoreRazorDepthCap` 3 against 1;
  - `CoreLmpExemptChecks` 0 against 1;
  - `CoreLmrNegativeAllowed` 0 against 1.
- Adopted at ≥ +5 Elo, rejected at ≤ −5, default kept between.
- Each switch is live on that binary (depth 13, bench position 1:
  71,514 nodes at the defaults; 104,672, 71,090, 96,616, 107,525, 62,766
  and 74,490 at the other values; `switch_liveness.txt` `006517d6…3314`).
- `CoreLmpExemptChecks` is the run §3.7 ties to a quiet-mate canary. Of
  the nine canary regressions, none is a quiet mate threat.

### 26.5 The games (maintainer-run 2026-10-13)

All seven runs: 2,000 games each, 0 time or other faults, natural
termination, the independent PGN recount agrees. Run records:
`tools/results/colosseum_match_b22-*_20261013_*`, hashes in BAS-S21.

| Run | A against B | Elo (95%) | W/D/L | Rule | Reading |
|---|---|---:|---|---|---|
| paired | ON against OFF, same revision | **+130.9 ± 11.4** | 1014/692/294 | ≥ +10 proceeds | **proceeds to B.2.3** |
| `CoreTtCutoffNodeTyped` | 0 against 1 | +4.9 ± 9.5 | 537/954/509 | ±5 | default 1 kept |
| `CorePawnHistBucketsLog2` | 11 against 13 | −4.5 ± 9.8 | 516/942/542 | ±5 | default 13 kept |
| `CoreCorrAdmission` | 0 against 1 | **−96.9 ± 10.7** | 320/816/864 | ±5 | rejected; default 1 kept |
| `CoreRazorDepthCap` | 3 against 1 | −7.1 ± 9.4 | 521/917/562 | ±5 | rejected; default 1 kept |
| `CoreLmpExemptChecks` | 0 against 1 | **+13.6 ± 9.7** | 574/930/496 | ±5 | **adopted: default becomes 0** |
| `CoreLmrNegativeAllowed` | 0 against 1 | −5.6 ± 9.8 | 518/932/550 | ±5 | rejected; default 1 kept |

What it means:
- **The cluster is a large gain.** The unfitted core already beats the
  legacy kernel by 131 Elo.
  - P1 (−15 ± 30) was wrong in size, not in kind. It priced the unfitted
    seeds as a cost; the oracle gap it was cut from (BAS-O05's +312.6, of
    which the move-loop family is about +210 by BAS-S18) says the
    architecture carries most of the value before fitting.
  - The two flagged floors (canaries 68/77, branching 1.6465) did not
    stop the game layer from reading strongly positive. They stay
    recorded with their attribution (26.3), as B.2.3's watch items.
- **Before it is acted on.** The magnitude is far outside the registered
  band, which is PROCESS *Harness*'s trigger for a backup-path
  cross-check: the same two binaries on fastchess. It is read as "the
  two instruments agree inside their intervals", never as a second gate.
- **Switches.**
  - `CoreLmpExemptChecks` moves to 0. The donor's count skip stops
    delivering quiet direct checks too, worth +13.6 ± 9.7 over the
    exemption, and the interval lies wholly above 0.
  - The other five stay at their defaults. Two of them sit just past the
    ±5 rule: `CoreRazorDepthCap` at −7.1 and `CoreLmrNegativeAllowed` at
    −5.6.
  - `CoreCorrAdmission` = 0 (the legacy `best ≥ beta || raised alpha`
    admission) costs 97 Elo inside this cluster. That is far larger than
    an admission rule's prior: training correction from nodes whose bound
    and error disagree corrupts the estimated score every node-level
    margin reads. It is information for B.2.3, not an action; the shipped
    default is the donor's rule.
- **The flip is an engine change.** `CoreLmpExemptChecks`'s default 1 → 0
  moves the ON fingerprint. It lands in its own commit, with the new
  `bench 13`, before B.2.3's sweep starts from the settled cluster.
- **Cross-check, 2026-10-13: the result stands.** The same two binaries on
  fastchess read **+136.2 ± 11.0** (1013/720/267, 0 faults, the PGN
  recount agrees; BAS-S21). The two instruments agree inside their
  intervals.
- **The flip, 2026-10-13 (`cd2e397`).** `CoreLmpExemptChecks` defaults
  to 0.
  - The ON arm's `bench 13` moves 9,132,715 → **6,199,439** on PEXT, plain,
    Tune, Diag and Ablate. `bench 10` reads 1,598,399 in release and
    sanitizer. The off arm stays at 14,978,465 (3,190,673).
  - CTest passes: ON 23/23 and 21/21, off 20/20 and 18/18.
  - ON WAC reads 173 at depth 6 (floor 130) and 249 at 100k nodes
    (`tools/results/b21-r3-20261011/lmp0/wac100k_on.json` `899be215…b5f9`).
  - The new default reproduces the categorical run's arm A exactly:
    62,766 nodes at depth 13 on bench position 1, as the old Tune binary
    gives with the option set to 0.
  - B.2.2 closes; B.2.3 starts from this head.

## 27. B.2.3 Amendment 5 (2026-10-13): piece values in history-unit sums take the donor's vector; the SPSA surface reviewed before block 1

Research (`R3`) at head `7322676`, before any game under BAS-S22. Two
deliverables: the unit ruling the maintainer held block 1 for (27.1), and
the review of the prepared tune (27.2). The donor is `edb0d9db`
(`src/movepick.cpp` lines 226, 240, 245, 307, 324; `src/search.cpp` line
1343; `src/types.h` lines 199–203).

### 27.1 Decision: the three sums use the donor's `PieceValue`, and the split's threshold converts back to SEE units

**Recommendation.** Land, before block 1, one `I1` engine commit that (i)
adds a history-unit material vector `HIST_PIECE_VALUE = {0, 208, 781, 825,
1276, 2538, 0}` (the donor's `PieceValue`, by rule 1) and reads it at the
three sites that add material into a history-unit sum; (ii) converts the
picker's good-capture split from the donor's material units to Basilisk's:
`see_ge(m, −score / 47)`; (iii) seeds `CoreRfpCorrMult` at the donor's
676. Then rebuild the ON Tune PGO binary, re-run the five-coordinate sweep
on it under the frozen rule, re-register block 1, and launch.

**The unit error.** The three sites are:

| Site | Donor | Basilisk today | Units of the sum |
|---|---|---|---|
| picker capture score (`move_picker_core.cpp` 62) | `captureHistory + 7·PieceValue[captured]` | `7·PIECE_VALUE[captured]` | capture history, limit 10,692 |
| picker quiet threat term (93) | `20·PieceValue[pt]·(escape − threatened)` | `20·PIECE_VALUE[pt]` | quiet score (main 7,183, pawn 8,192, continuations 30,000) |
| LMR capture `statScore` (`search_kernel_core.cpp` 1048) | `873·PieceValue[captured]/128 + captureHistory` | `873·PIECE_VALUE/128` | history, then 1024ths through `/4096·439` and the child's bonus `/28` |

The histories keep the donor's limits (rule 1), so their share of each sum
is the donor's; the material share is Basilisk's {100, 300, 300, 500, 900}
against the donor's {208, 781, 825, 1276, 2538}, about 0.38× (§7.1). The
effect is what the maintainer's note says: a pawn capture with good
history outranks a queen capture with bad history more often than in the
donor (7·900 = 6,300 against a history range of ±10,692; the donor's
17,766 against the same ±10,692), threatened-piece escapes are promoted
about a third as strongly, and LMR's capture `statScore` leans on history
by 2.6× the donor's proportion. Amendment 1 decision 8 chose Basilisk's
vector by reasoning about SEE thresholds, which are material units; these
three sums are not. The same class of error as rule 4 (§22.2): a quantity
in one unit added into another.

**Why the donor's vector rather than a scaled Basilisk one.** The sum is a
history-unit quantity and the donor's material term inside it is a donor
constant in those units; rule 1 says such constants are taken unchanged.
Scaling Basilisk's vector by 1000/380 gives {263, 789, 789, 1316, 2368},
the same thing within the measurement of 0.38 except that it loses the
donor's knight–bishop difference and lifts the pawn (§7.1 measured pawns
at 0.48). The donor's vector is the constant that the donor's histories
were fitted against. Nothing else in the engine reads `HIST_PIECE_VALUE`:
SEE, capture futility (`PIECE_VALUE[captured]` added to an evaluation,
unit-consistent) and the evaluation keep Basilisk's material.

**The split, corrected.** Decision 8 said the good-capture split
`see_ge(m, −score/18)` is "exactly SEE ≥ 0" on Basilisk's vector. That
holds only if `score` were the piece value; it is `7·material + capture
history`, so the threshold ranges over roughly a queen either way. In the
donor the threshold `−score/18` is in the donor's material units (a
zero-history queen capture: 7·2538/18 = 987, 0.39 queens). Once `score` is
in donor units, the threshold must come back to Basilisk's SEE units,
×0.38: **`see_ge(m, −score / 47)`** (18/0.38 = 47.4; a zero-history queen
capture then reads 378, 0.42 queens). The picker's check gate `see_ge(m,
−75)` is a material threshold the donor writes in its own units; its
Basilisk value is −28, and both are exactly `SEE ≥ 0` on a vector whose
values are multiples of 100, so it stays as written (fingerprint-neutral
either way).

**`CoreRfpCorrMult` seeded at 676.** §7.3 seeded the RFP correction
coefficient at 0 as a coordinate with no stated reason beyond caution,
and Amendment 3 fixed its unit (1024ths, unit-free). A tune cannot start a
coordinate at its range edge: half of every perturbation is clipped at 0
and the gradient estimate is biased toward staying there. The donor's
value is 131072/198435 = 0.66 → 676; liveness is shown (§25.2 item 9:
63,391 / 49,793 / 52,949 nodes at 0 / 676 / 2048). Seed it at 676 in the
same commit; B.2.4(a) gates it with the rest of the unfitted arm.

**Not changed.** Quiescence's capture order (`search_kernel_core.cpp`
242, `16·PIECE_VALUE[cap] − PIECE_VALUE[atk] + captureHistory`) mixes the
same units and is B.4's; recorded for B.4's handoff. The qsearch stand-pat
guard (221) is evaluation units. Capture SEE's `captHist/80` and capture
futility's `captHist/27` already convert history into their target units
(§7.3).

**Rule 5, added to §7.2.** A material value added into a history-unit sum
is the donor's `PieceValue` (rule 1); a history-unit score turned into a
SEE threshold converts to Basilisk's material units (×0.38).

**Counter-argument.** The cluster measured +130.9 Elo with the error in
place, and the three sums' material share is a fit question SPSA could in
principle absorb if `7`, `20` and `873/128` were coordinates. They are
fixed donor constants by rule 1, so the tune cannot repair the unit; and a
50-coordinate fit on top of a 0.38× material share would be partly stale
the moment the unit is corrected, which is why the correction precedes the
tune rather than follows it. The Elo of the correction itself is not
predicted here; it is inside B.2.4(a). Its cheap instruments are the
100k-node WAC count and the Diag ordering counters (`fail_high_first`,
`cutoff_src_good_tactical`) before and after, recorded, not gated.

**Qualification of the commit (the usual per-commit record).** ON `bench
13` on every flavour (it moves), sanitizer `bench 10` = release; the off
arm at 14,978,465 on every flavour and 3,190,673; CTest on both arms
(release and sanitizer), `test_move_picker_core` included; the 100k-node
WAC count and the depth-6 count; the two ordering counters at `bench 13`
on the Diag build. Then the ON Tune PGO binary is rebuilt with a clean
manifest and its `bench 13` recorded.

### 27.2 The prepared SPSA, reviewed

Read: BAS-S22's block registration, `core.tune.toml` and
`config_core.json` (generated, `--check` current: 41 default-arm and 50
core-arm parameters), `generate_spsa_surface.py`, `spsa_block_rule.py`,
`spsa-tune.toml`, the dry run, `coord_liveness.txt`, the sweep note, and
the ON kernel's reads of the parameter tables.

**What is right and stays.** The surface is generated from the X-macro
table, so defaults and ranges cannot drift. Every one of the 50
coordinates moves the tree at its range edge. The six categorical switches
are off the surface at their settled values, as the rule requires. Block
shape, gain, seeds, the movement rule, the three-block ceiling, the
estimator and the bake-time floor check are PROCESS's go/no-go procedure
as written, and the runner's dry run accepted the exact arguments. The
sweep was frozen before its first point and read by its rule.

**Changes before launch (through the engine commit of 27.1 and a
regeneration; the toml is never edited by hand):**

1. `CoreRfpCorrMult` starts at 676 (27.1). A coordinate at a range edge is
   one the tune cannot fit.
2. `CoreQuietSeeCoeff` leaves the surface, fixed at 10. Its response is a
   staircase the step cannot resolve: SEE values on Basilisk's vector are
   multiples of 100, so at `lmrDepth` 4–6 the thresholds for 9, 10 and 11
   (−144/−160/−176, −225/−250/−275, −324/−360/−396) prune the same set,
   and the ±1 perturbation (`c_end` 0.92, about 2 at the start) sees a
   gradient only from `lmrDepth ≥ 7`, a small population. The range-edge
   liveness (30) is real; the local gradient is not. Mechanically: the X
   line becomes an F line that the engine table still reads and the
   generator skips (an `F(...)` macro beside `S(...)`), so the surface
   regenerates to 49 coordinates with `--check` current. `CoreCaptSeeCoeff`
   (×depth, step 5) crosses a 100-unit boundary from depth 4 on captures
   and checks, a large population, and stays; `CoreCaptSeeHistDiv` moves
   the threshold by about 8 units per step and is weak but not stepped,
   and stays.
3. The surface, the config, the dry run and the binary hash in BAS-S22's
   block registration are re-recorded after the commit; the sweep is
   re-run on the new binary under the frozen rule (five coordinates, the
   same points, the same classification), because the registration's
   go/no-go input must be the binary the tune runs on. The rule's outcome
   is not expected to change; if all five read flat or monotone, the rule
   says no tune, and that is the record.

**Checked and kept as is.**
- Integer coordinates with small ranges (`CoreRfpDepthCap`,
  `CoreIirMinDepth`, `CoreLmpBase`, `CoreLmpSquareScale`,
  `CoreQuietFutDepthCap`, `CoreCaptFutDepthCap`, `CoreLmrAlphaEvalScale`,
  `CoreResearchDeeper`, `CoreResearchShallower`, `CoreCorrUpdateScale`):
  the registered step is 2 and `c_end` 0.92, so the late-schedule
  perturbation is ±1 after the runner's rounding, which is the smallest
  change these can take; each is live at ±1 (a depth cap of 8 against 9
  changes the node population it gates). The movement rule reads a full
  step of 2. The same shape fitted the A-phase surfaces.
- Small integer seeds near 0 (`CoreResearchShallower` 2, `CoreRfpWorsening`
  11, `CoreLmrAlphaEvalScale` 3): early perturbations clip at 0 on one
  side for a few hundred iterations; tolerable, and a start elsewhere would
  be a seed change without evidence.
- The eight legacy coordinates the core table splices in (`NullBase`,
  `NullEvalDiv`, `ProbCutMargin`, `SingularMinDepth`, `SingularBetaMult`,
  `SingularDoubleMargin`, `DoubleExtMax`, `LmrSingularQuiet`) stay off the
  surface. They run at values fitted to the legacy search inside the new
  cluster, which is a real misfit, but B.3 replaces their forms (§3.4–3.5,
  §12.3) and seeds the new forms from the oracle column; a fit of the
  legacy forms now would be discarded there, and eight more dimensions
  dilute a 50-coordinate gradient whose curvature the sweep already reads
  as shallow. The cost of leaving them is bookkeeping, not strength: B.3's
  attributed Elo will include repairing their fit. Counter-argument: Elo
  now rather than at B.3; if the maintainer prefers it, they enter only
  with the same range-edge liveness proof and a recorded reason, as a
  registration change before the first game.
- The fixed donor constants in history units and 1024ths stay fixed by
  rule 1 (§5.10): the 7, 20, 16384, −14000 and −3560·depth of the picker;
  LMR's 697, 65, 2179, 1127, 5234/5487, 940; the continuation weights and
  consistency multipliers, 899/1159/921, 1427/1489, 713, 1334, 918/−747,
  the fail-low scale's terms, 11/13 and 60/−189/194, the correction
  weights; 7419/2695 and 47 hindsight. A tune over them would be the
  undirected broad tune PROCESS forbids; B.6 exists for the case where the
  clusters' curvature justifies widening.
- Not on the surface and not the cluster's: root and time (B.5, D),
  quiescence (B.4), `CorePawnHistBucketsLog2` (a size).
- Nothing on the surface is inert at its seed except the two items above:
  every other coordinate's seed sits inside its range and its mechanism
  fires at `bench 13` (§20's counters).

**Prediction (BAS-S22, unchanged in substance):** curvature shallow, the
tune's gain mostly from interactions among the unswept coordinates; the
gate is B.2.4(b) at `[0,10]`. The unit correction is not part of the tune's
prediction; it is gated by B.2.4(a).

### 27.3 Handoff

1. `I1` engine commit on the ON arm (27.1): `HIST_PIECE_VALUE` at the
   three sites; `−score / 47`; `CoreRfpCorrMult` seed 676;
   `CoreQuietSeeCoeff` moved to an `F` line that the generator skips.
   The generator gains the `F` macro (skipped like `S`), in the same
   commit or a tooling commit before it. Qualification as 27.1.
2. Regenerate the surface (`generate_spsa_surface.py`; `--check`
   current; the parser tests pass); rebuild the ON Tune PGO binary;
   re-run the sweep (`sweep.sh` recipe, the frozen classification);
   re-validate block 1 (preflight and dry run); re-record BAS-S22's block
   registration (surface 49, hashes, bench) and the sweep note's result
   section; PLAN B.2.3 one line; GUIDE; the checkers. Commit docs
   separately. Stop: block 1 is maintainer-run.
3. B.4's handoff (§12.4 when written) gains the quiescence capture-order
   unit note.

## 28. B.2.3 unit commit (2026-10-13): Amendment 5 landed

- **Commits:**
  - `e24114a`, tooling: the generator finds the core table by its macro
    name and keeps `F` lines off the surface, with a test;
  - `3eea3c5`, engine, ON arm only: `HIST_PIECE_VALUE` at the three sums,
    `see_ge(m, −score / 47)`, `CoreRfpCorrMult` seed 676,
    `CoreQuietSeeCoeff` an `F` line, the table macro taking `(X, F)`;
  - `b5983c6`, tooling: the surface regenerated to 49 coordinates.
- **Recipe:** `tools/results/b21-r3-20261011/qual.sh` (label `b23unit`),
  the per-commit record of §20.

| Reading | Before (`cd2e397`) | After (`3eea3c5`) |
|---|---:|---:|
| ON `bench 13`, release PEXT = plain = Tune = Diag = Ablate | 6,199,439 | **6,096,792** |
| ON `bench 10`, release = sanitizer | 1,598,399 | 1,386,667 |
| Off arm `bench 13` (five flavours) / sanitizer `bench 10` | 14,978,465 / 3,190,673 | 14,978,465 / 3,190,673 |
| CTest ON release / sanitizer (`test_move_picker_core` included) | 23/23, 21/21 | 23/23, 21/21 |
| CTest off release / sanitizer | 20/20, 18/18 | 20/20, 18/18 |
| ON WAC depth 6 (floor 130) | 173 | 177 |
| ON WAC 100k nodes | 249 | **237** |
| Diag `bench 13`: first-move cutoffs / fail highs | 88.48% (941,970 / 1,064,606) | **89.09%** (1,009,571 / 1,133,236) |
| Diag `bench 13`: good-capture cutoffs / fail highs | 21.01% | **24.92%** |
| Diag `bench 13`: mean cutoff index | 0.196 | 0.182 |

Notes:
- **Item 2 is live.** The commit's tree with `/ 18` in place of `/ 47`
  (`scratch-div18`) reads `bench 13` 5,068,184, against 6,096,792.
- **WAC at 100k fell 12 positions; attribution, instruments only (27.1
  registers no prediction for them):**
  - without the split change (`/ 18`): 244;
  - with `CoreRfpCorrMult` at 0: 240;
  - so the split carries about 7, the 676 seed about 3, and the rest,
    about 2, the material vector (not isolated by its own run).
- **Ordering improved by both counters:** more first-move and
  good-capture cutoffs, a lower mean cutoff index.
- **The Elo of the correction is B.2.4(a)'s,** as 27.1 says.
- **Artifacts** (`tools/results/b23-20261013b/`):
  - `counters_bench13_before.json` `2fc4361d…7a45`,
    `counters_bench13_after.json` `1e8faf10…e44c`;
  - `wac100k_div18.json` `11fe721c…dcb6`, `wac100k_corrmult0.json`
    `a53518e0…c5f1`;
  - the commit's 100k probe `b23unit/wac100k_on.json` `4c1889e1…f62e`.
