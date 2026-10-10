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
  move is searched; the reduced depth at least one ply; direct quiet checks
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
beside it (SHA-256 `77a05d16…0730` and `14387fce…ba0e`). It ran 4,000
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
