# Cluster 2 research and handoff — PLAN B.3.0

Started 2026-10-30 on `master` `43b4c35` (`bench 13` 19,470,325), the
accepted B.2 head with A.11, A.13, B.10 and A.16 on top. Donor: Stockfish
`sf_19` `edb0d9db` (`src/search.cpp` line numbers below are that file's);
kernel line numbers are `src/search_kernel_core.cpp` at `43b4c35`. This is
the working packet: sections are added as the research proceeds, and the
leaf closes `READY_FOR_IMPLEMENTATION` only when §8's conditions are met.
Resumed 2026-10-30 on `b3-pruning-extensions` at `1414039` (B.12's repair
`d8d30e9`, `bench 13` 16,615,757, and the counter seam `1414039` on top);
§2.1 and §5–§13 read that head. The seams moved §3's kernel lines by at
most twenty; §10 cites symbols, not lines.

**Status: `READY_FOR_IMPLEMENTATION`, 2026-10-30 (draft 2).** §2 re-bases
B.0's handoff, §3 compares every Cluster 2 mechanism with the donor, §4
disposes of every input PLAN B.3 lists, §5 maps the interactions with
Cluster 1 and the later clusters with the head's numbers, §7 seeds, §8
decides, §9 registers the switches and their brackets, §10 is the B.3.1
contract, §11 the frozen predictions and the B.3.2–B.3.4 registrations,
§12 the four questions, §13 the artifacts.

## 1. Verdict

- B.0's Cluster 2 contents stand, re-based on the repaired B.2 head: NMP,
  ProbCut with the small TT ProbCut, singular in the donor's shape, IIR off
  the previous PV, and the check policy; plus the couplings the code audit
  found (S1, S5, S7, S8), the upcoming-repetition check (BAS-S12's trigger
  fires), the TT bound-mismatch penalty (S10) and the four defect repairs
  PLAN §2 assigns to B.3 (B-12, B-14, B-45, B-109). No item's direction
  changed; three seeds and the extension bound did.
- The largest semantic gap is still the **singular margin**: 4 cp per ply
  against the donor's 0.26 (B-45). The exclusion search already runs at
  2.41% of interior nodes and extends 28.5% of its tries (§2.1), so what
  changes is the question it asks, not how often it is asked (§5.5).
- The **check policy** is the cluster's hardest decision and is taken
  (§8): the donor's form is the ON arm (no in-check extension; reductions
  and move-loop pruning in check), the current policy readable as the first
  two categoricals of §9 and read before anything else. In-check nodes are
  26.4% of interior nodes on this head, none pruned or reduced, a
  population Cluster 1's fit never saw (§5.1).
- The **child node types** of the three proof searches (S1) and NMP's
  estimate (S7) are population changes inside the ON arm, not categoricals;
  through the measured rule of §5.12 they pull the Cluster 1 LMR
  coordinates they move onto B.3.3's surface, which is how the cluster
  stays one search rather than a second one bolted on.
- The **extension budget** (B-7) replaces the double-extension cap, which
  bound 5 times in 51.5M nodes; IIR against hindsight is closed (0.3%
  overlap, both stay) and IIR's stale-entry clause (56% of its firings) is
  a categorical.
- Decision: `READY_FOR_IMPLEMENTATION` (§8). The contract is §10, the
  predictions and registrations §11.

## 2. Re-basing B.0's handoff

What changed under Cluster 2 since B.0 measured it (B.0 §2.5, the
1.10.1 head, `bench 13` 14,978,465):

- B.2's core is on: RFP in the donor's shape, razoring at depth 1, the
  donor's LMR and histories, the persisted `tt_pv` bit, `cutoffCnt`,
  hindsight. NMP, ProbCut, singular, IIR and the check extension kept
  Basilisk's forms, re-wired to the new estimated score, `improving` and
  `tt_pv` (B.0 §5).
- B.2.3 fitted Cluster 1's coordinates around Cluster 2 as it stands, so
  every Cluster 2 change moves the population Cluster 1 was fitted on
  (§5).
- A.11, A.13 and B.10 changed evaluation and endgame behaviour; the
  fingerprint moved 11,099,210 → 19,470,325.

The fresh counters (§2.1) replace B.0 §2.5's Cluster 2 rows as the seeds'
and predictions' base.

### 2.1 Stride-1 counters on the repaired head

Done 2026-10-30: `run_suite.py --depth 14 --stride 1 --hash 64` on
`suite_v2` (106 positions), the Diag build of `1414039` (`bench 13`
16,615,757; §13), `tools/results/b30-20261030/suite_d14.json`. The first
run, on `ba31f28`, stopped at its 600 s limit on position 89 (§2.3); on
the repaired head that position takes 12,986 nodes. Shares are of interior
nodes unless a row says otherwise; B.0 §2.5's column is the 1.10.1 head.

| Quantity | This head | B.0 §2.5 | Reading |
|---|---:|---:|---|
| interior / quiescence nodes | 51,511,198 / 29,274,358 (quiescence 36.2%) | 31.5M / 17.3M (35.5%) | the tree at depth 14 is 1.6× the 1.10.1 tree on this suite; the quiescence share is unchanged |
| in-check interior nodes; check extensions | **26.4%**; 26.1% | 15.9%; 15.6% | the forcing share rose by two thirds under Cluster 1 (§5.1) |
| `rfp_cuts`; razoring | 14.9%; 5.1% | 14.2%; 6.2% | RFP owns the shallow band (§5.3) |
| `null_tries`; conversion | 7.48%; **41.5%** (by depth 1–3 / 4–7 / 8+: 37.5 / 42.5 / 46.8%) | —; 49.6% | more than half the null searches are wasted, worse than B.0's read; the oracle about 83% |
| `probcut_tries`; conversion | 1.40%; 54.0% | —; 68.4% | |
| IIR; its stale clause; on a node hindsight reduced | 0.81%; 56.4% of IIR; 0.3% of IIR | — | no overlap with hindsight; the stale clause is half of IIR (§5.6) |
| hindsight up / down | 5.20% / 1.02% | — | |
| singular tries; extended / multi-cut / negative per try | **2.41%**; 28.5 / 11.8 / 8.3% | extensions 0.45% | extensions 0.69%: doubles 84.3% of them, in check 23.5%, triples 16.1%; the cap bound 5 times; lone-move exclusions 2.0% of tries |
| LMR | eligible 29.2M; applied 41.7%; mean 2.48 plies; floor hits 34.7%; extended 7.1%; re-search 6.5%; blocked in check 10.8%, by depth 20.7% | applied 36.4%; 2.47; re-search 1.67% | B.2's P6 asked for 3–5% re-search: 6.5% now |
| cutoffs | 20.8M; first move 87.0%; mean index 0.194; TT 38.1%, good captures 27.9%, quiets 33.8% | 89.1%; 0.210; 32.7 / 42.7 / 24.5% | ordering at the cutoff holds; quiet cutoffs took the share captures lost |
| move loop | 42.6M moves tested; quiet futility 20.5%, count-skip nodes 9.9%, continuation 5.5%, capture futility 7.1%, quiet SEE 7.0%, capture SEE 7.4% | LMP 1.6 per node; history pruning dead | the donor's family is live on every branch |
| TT | probes 80.7M, hits 45.3%, cutoffs 7.9%, stores 68.8M; `tt_pv` nodes **2.26%**; graph-history refusals 1,499 | hits 35.6%; 7.7%; `tt_pv` 0.57% | the persisted bit is a live signal |
| histories | 20.8M cutoff updates, 65k exact-node rewards, 3.66M continuation-correction updates at ply 2 | 8.97M; 52.8k | |
| aspiration | 1,031 windows, 1,515 re-searches (1.47 per window), 22 give-ups | 1.51 | |

The largest trees: positions 50 (31.2M nodes), 42 (10.8M) and 60 (8.9M);
they anchor the deep-iteration cost screen (§10).

### 2.2 The conversion floors at five budgets (B-3)

`test_endgames --conversion --nodes N` (`ebc7623`: the floors replay at
another budget and report; their bounds are set at 60k), release build of
`ba31f28` (behaviour as `43b4c35`), the 24 seeded positions per family, a
cold search per move (`tools/results/b30-20261030/conversion/`):

| Family | 30k | 45k | 60k | 90k | 120k | Floor (60k) |
|---|---:|---:|---:|---:|---:|---:|
| KQ-K | 24 (14.8) | 24 (14.2) | 24 (12.7) | 24 (12.4) | 24 (11.3) | 24 |
| KR-K | 24 (27.2) | 24 (25.4) | 24 (23.6) | 24 (21.6) | 24 (21.2) | 24 |
| KBB-K | 24 (26.2) | 24 (24.2) | 24 (25.2) | 24 (24.5) | 24 (24.6) | 23 |
| KBN-K | **21** (55.2) | 24 (58.6) | 23 (55.0) | 23 (51.8) | 22 (53.0) | 22 |

Conversions of 24, mean plies to mate in brackets. The major-piece and
two-bishop mates are budget-stable; KBN-K is not monotone in budget
(24 at 45k, 22 at 120k) and falls below its floor at 30k. A single-budget
KBN-K verdict is therefore one draw from a spread of about ±1.5
conversions on the head itself. The rule the cluster reads (registered
here, before any ON-arm build): **a KBN-K change counts only as a majority
over the five budgets against this table, the sum over budgets moving by
more than 3 (the head's 113 of 120)**; the other families hold 24 at every
budget.

The repaired head (`d8d30e9`, BAS-S40's candidate and the engine B.3.1
starts from; `tools/results/b12-20261030/conversion/`), same recipe:

| Family | 30k | 45k | 60k | 90k | 120k | Sum |
|---|---:|---:|---:|---:|---:|---:|
| KQ-K | 24 (15.7) | 24 (12.6) | 24 (12.5) | 24 (12.1) | 24 (10.8) | 120 |
| KR-K | 24 (28.9) | 24 (22.2) | 24 (23.3) | 24 (23.6) | 24 (21.2) | 120 |
| KBB-K | 24 (24.2) | 24 (23.8) | 24 (24.3) | 24 (23.3) | 24 (23.5) | 120 |
| KBN-K | 22 (56.2) | 23 (55.9) | 24 (52.8) | 23 (50.9) | 23 (53.3) | **115** |

The KQ-K line at 120k trips the floors' own staleness guard (a mean of
10.8 plies, under two thirds of the 16.3-ply ceiling set at 60k), not a
floor: the bounds judge at 60k alone and only report elsewhere. The
rule's base is therefore **115**: a KBN-K change counts only as a
majority over the five budgets, the sum moving by more than 3.

### 2.3 A blocker found by §2.1's run: a false win in a drawn KBP-K

§2.1's suite stopped at its 600 s limit on `suite_v2` position 89,
`7k/8/6KP/8/8/8/8/5B2 w - - 0 1`: an h-pawn with the wrong (light) bishop
and the defending king in the corner, a textbook draw.

| Build | Depth 8 | Depth 10 | Depth 14 |
|---|---|---|---|
| 1.10.2 (`tools/test_engines/basilisk-1.10.2-rc-pext-pgo.exe`) | 0, 1,328 nodes | 0, 2,408 | 0, 11,704 |
| head `ba31f28` (release) | **+150.94**, 5,182 nodes, seldepth 17 | **+112.08**, 13,553,601 nodes, seldepth 112 | does not finish in 60 s |

- The static evaluation is right along the false line (depth-1 search of
  every position on it reads 0, and +7.89 only with the black king out on
  f7). 15094 lies in the `KNOWN_WIN` class (`eval.cpp:132`, 10000 plus
  material), so the search reaches positions it rates as known wins: a won
  KPK after the bishop is lost, or a promoted bare-king win. A minimax line
  can only reach them from this root if black is compelled out of the
  corner, which in this ending happens only by zugzwang.
- Bisect over `master`'s squash commits since `v1.10.2` (depth 10, good
  under 200,000 nodes): first bad **`75d7fce`** (A.13: a won KPK now
  scores `KNOWN_WIN + 206 + rank` instead of its tapered score). Before
  it the position already read +3.94 (`6e66c5d`) and +5.57 (`057797d`)
  on small trees, where 1.10.2 reads 0: an earlier, smaller drift.
- Leading hypothesis, untested: null-move pruning in a zugzwang ending.
  NMP is allowed here because White has a bishop (non-pawn material), has
  no verification below depth 10, and its cutoffs are now fed known-win
  magnitudes, which are not "decisive" by the band check (`is_decisive`
  covers the tablebase and mate bands only). NMP's verification region is
  Cluster 2's own item (B-31).
- Consequence for this leaf: §2.1's counters cannot complete at depth 14
  on the head, and any Cluster 2 reading that includes this ending is
  distorted by the explosion. The defect also misplays real games (a
  drawn KBP-K scored as a win invites trading into it).

B.12 working record (2026-10-30, interrupted at the maintainer's request).
Ablation build of `a08d40f` (`AblationMask`; bits: 0 razoring, 1 RFP,
2 NMP, 3 ProbCut, 4 IIR, 5 move-loop pruning, 6 singular and the check
extension, 7 LMR), depth 10 unless noted:

| Mask | Score | Nodes | Seldepth |
|---:|---:|---:|---:|
| 0 | +112.08 | 13,553,601 | 112 |
| 1 | +95.13 | 21,599 | 45 |
| 2 | +26.82 | 49,277 | 46 |
| 4 (NMP off) | +1.36 | 24,319 | 45 |
| 8 | +4.08 | 19,664,420 | 114 |
| 16 | +33.33 | 9,995 | 60 |
| 32 | +11.69 | 6,328,450 | 97 |
| 64 (singular, check ext. off) | +95.39 | 4,750 | 26 |
| 128 | +1.49 | 191,410 | 45 |
| 68, depth 14 | +88.75 | 25,391 | 23 |
| 255, depth 10 / 14 | +198.71 / +198.71 | 69,457 / 467,418 | 11 / 16 |
| 5, depth 14 | +145.88 (upper bound) | 54,531,100 | 128 |
| 12, depth 14 | +167.78 (upper bound) | 76,137,796 | 116 |

Reading so far: no single family is the cause. With every family off
(255) the score is still +198.71, so the false win is not a pruning
artefact at all: plain alpha-beta plus quiescence reaches positions scored
as known wins. NMP and LMR amplify it, and the extensions turn it into the
explosion. Next: walk the all-off PV (`f1d3 h8g8 h6h7 g8h8 g6h5 h8g7
h5g5 g7h8 g5h4 h8g7`) for the leaf that scores a known win, most likely a
quiescence or evaluation path, which `go depth 1` along the line did not
show.

Found 2026-10-30. Along the all-off line, each white-to-move position with
the pawn on h7 and the king in the corner reads the known-win class at
depth 2 and 0 from its own root, so the score is made inside quiescence.
The leaf is an underpromotion beside the defending king: after `h8=B` (or
`=N`) the defender's stand-pat is a won KBB-K (KBN-K) for the opponent,
and delta pruning, capture futility and the alpha-relative SEE threshold
read `Kxh8`, which leaves a dead draw, as a gain of one minor against
it. The cut returns the known win as a bound. That is why no ablation bit
removed it, and why NMP and the extensions only scaled it. The repair
(`d8d30e9`) is the oracle's guard: the three prunes stand down when the
futility base is below `-KNOWN_WIN`. The record and the gate are BAS-S40.
Two consequences for this packet:

- §2.1's counters and §2.2's floors were read on the head; the counter
  suite reruns on the repaired head, and the floors there are BAS-S40's
  (KBN-K 115/120, the same under the rule).
- The main search's move-loop pruning is safe here: like the oracle's
  (`9587eeeb` `search.cpp:1007`) it runs only for a side with non-pawn
  material, which a bare defending king never has. RFP's
  `eval < tablebaseWinInMaxPly` bound and NMP's beta test are not: the
  oracle bounds both by `VALUE_KNOWN_WIN` (`:821`, `:855`). NMP's is
  §3.1's to dispose; RFP's is B.4's input.

## 3. Mechanisms against the donor

Units: Basilisk evaluation units; donor values converted by B.0 §7.1
(evaluation × 0.282, material × 0.38, plies and counts × 1).

### 3.1 Null-move pruning (kernel `:666-699`; donor `:1010-1044`)

| Aspect | Basilisk | Donor | Converted | Disposition |
|---|---|---|---|---|
| Population | any non-PV node, not in check, `allow_null`, previous move not null | `cutNode` only | — | **ADOPT** cut-node-only; B-33's LTC sign (−10.4 at `10+0.1` in Rarog) makes this the one Cluster 2 item read at LTC as well |
| Entry estimate | `eval` (TT-refined) `≥ beta` | `staticEval ≥ beta − 13d − 47·improving + 365` | `≥ beta − 3.7d − 13·improving + 103` | **ADOPT** static eval (S7) and the margin; seed by rule 2 against the oracle's `−33d − 33·improving + 112·ttPv + 311` (×0.485: 16d, 16, 54, 151) |
| Other guards | non-pawn material | non-pawn material, `ply ≥ nmpMinPly`, `beta ≥ −2000` (≈ −564) | — | ADOPT both |
| Reduction | `3 + d/4 + min((eval − beta)/192, 3)` | `7 + d/3 + max((staticEval − beta)/256, 0)` | `7 + d/3 + max(Δ/72, 0)` | ADOPT shape; base and slope coordinates (§7) |
| Child | `NonPV`, `allow_null = false`, **`cut_node = true`** | `NonPV`, **`cutNode = false`** | — | ADOPT (S1) |
| Mate scores | a decisive null score returns `beta` | a win does not return (`!is_win`) | — | ADOPT the donor's refusal; it removes a fail-soft path that returns `beta` from an unproven mate |
| Verification | depth ≥ 10, same ply, `depth − r`, no region | depth ≥ 16, `nmpMinPly = ply + 3·(d − R)/4`, NMP off below it | — | ADOPT the region (B-31); the same-ply form is the `[-5,0]` deletion arm's base (PLAN simplification read) |
| Stack record | the verification re-enters `ss` and overwrites it (B-37) | the donor's re-entry is guarded by `excludedMove`/`nmpMinPly` only | — | B.3.1 saves and restores the fields the verification overwrites, or proves none is read after it |

### 3.2 ProbCut (kernel `:701-733`; donor `:1054-1099`, small `:1101-1104`)

| Aspect | Basilisk | Donor | Converted | Disposition |
|---|---|---|---|---|
| Population | non-PV, not in check, depth ≥ 5 | not in check (steps 7–12 skip check), **PV nodes included**, depth ≥ 3 | — | ADOPT depth ≥ 3; PV admission is a categorical (the donor admits it, Basilisk's whole non-PV block excludes it) |
| Margin | `beta + 189` | `beta + 241 − 64·improving` | `beta + 68 − 18·improving` | seed by rule 2 against the oracle's (×0.485) and Basilisk's fitted 189 |
| TT-value guard | none | skip when a valid TT value `< probCutBeta` | — | ADOPT (B-42 asks for a depth condition on it: §6) |
| Move source | `gen_legal_captures`, generation order, SEE `pc_beta − static_eval` | picker over captures by capture history, threshold `probCutBeta − staticEval`, TT move first | — | ADOPT the picker (B-39: order and cap) |
| Search | qsearch, then `depth − 4` | qsearch, then `depth − (improving ? 5 : 3)` when `> 0` | — | ADOPT |
| Child | **`cut_node = true`**, `allow_null = true` | `!cutNode` | — | ADOPT (S1) |
| Store, return | depth `− 3`, value `pc_beta`, returns `pc_beta` | depth `probCutDepth + 1`, value `value`, returns `value − (probCutBeta − beta)` unless decisive | — | ADOPT |
| Small TT ProbCut | none | at the move loop (in check too): lower bound, `ttDepth ≥ depth − 4`, `ttValue ≥ beta + 428`, nothing decisive → return `beta + 428` | `beta + 121` | ADOPT (B-43: "small" means the TT-served form) |

### 3.3 Internal iterative reduction (kernel `:736-744`; donor `:1048-1052`)

| Aspect | Basilisk | Donor | Disposition |
|---|---|---|---|
| Population | non-PV (cut and all nodes) | PV and cut nodes not on the previous PV (`!followPV && !allNode`) | ADOPT with `followPV` (S8) |
| Trigger | no TT move, or a TT entry `< depth − 3` deep | no TT move | ADOPT; the stale-entry clause is a categorical only if §6's read shows it carries the effect |
| Depth | `CoreIirMinDepth` 8, its range's rail (B-55) | 6 | seed 6; range widened 3..10 |
| Against hindsight | both reduce the same nodes' depth | the donor carries both | B.0's "IIR against hindsight": §6 reads the overlap before the categorical is registered |
| Correctness | B-5 and B-6 in Rarog: IIR in the donor's form exploded the search where a ply bound did not bound depth | — | the per-line extension budget, the deep-iteration cost screen and the watchdog of PLAN B.3 gate every IIR arm (A.11.2 has landed) |

### 3.4 Singular extension (kernel `:975-1051`; donor `:1234-1305`)

| Aspect | Basilisk | Donor | Converted | Disposition |
|---|---|---|---|---|
| Entry | depth ≥ 5, TT depth ≥ `depth − 3`, bound Lower or Exact, not decisive | depth ≥ `6 + ttPv`, same TT conditions, not shuffling, not `seekMate` | — | ADOPT; `is_shuffling` is the donor's non-regression guard, ported as written and never tuned |
| Margin | `ttValue − 4·d` | `ttValue − (59 + 66·(ttPv && !PV))·d/63` | `− (0.26 + 0.30·(ttPv && !PV))·d` | ADOPT the donor's; B-45: the current range cannot express it; the coordinate is in 1/63 cp per ply |
| Depth | `(depth − 1)/2` | `newDepth/2` | same | keep |
| Verification child | **`cut_node = true`** | `cutNode` | — | ADOPT (S1) |
| Extension | +1, or +2 when non-PV and `s_val < s_beta − 4` under `DoubleExtMax` | `1 + (v < sb − doubleMargin) + (v < sb − tripleMargin)`, margins in PV, TT capture, correction, `ttMoveHistory` and ply terms; the node's `depth++` | §7 | ADOPT; the double-extension counter is the deletion arm of PLAN's simplification read |
| Multi-cut | `s_beta ≥ beta` → return `s_beta` | `value ≥ beta`, not decisive → train `ttMoveHistory` and correction, return `value` | — | ADOPT; Rarog's donor multi-cut failed its KBNK drive test (PLAN B.3 risk): §6's KBNK multi-budget majority (B-3) decides before the categorical is dropped |
| Negative | −1 when `ttValue ≥ beta` | −3 when `ttValue ≥ beta` or `cutNode` | — | ADOPT (B-50); BAS-D13's WAC-only refusal of the donor's second verification is re-read by games (B-89) |
| `ttMoveHistory` | present since B.2: one scalar, trained at the node's end (918 / −747), consumed nowhere | the same scalar, trained also at multi-cut (−421 − 110·d), consumed by the double margin | — | ADOPT the update and the consumer; it is not an LMR term in `edb0d9db`, so B-51 is moot (§5.5) |
| `singular_quiet_lmr` | relaxes LMR for the node's quiets after a singular TT move (BAS-D18, unmeasured) | the donor's `ttPv`/singular-margin LMR terms | — | subsumed (B.0 §6); removed with the ON arm |

### 3.5 Check policy (kernel `:357-373`, `:854`, `:1121`, `:1137`, `:1268`)

| Aspect | Basilisk | Donor | Disposition |
|---|---|---|---|
| Extension | every in-check node `depth++` (15.6% of interior, B.0) | none | the cluster's ON arm takes the donor's (none); the current extension is a categorical read by games at STC and LTC |
| Reduction and pruning in check | none: LMR, LMP, futility, SEE and history pruning all skip in-check nodes | LMR, the step-down, `depth −= 3`, LMP, capture SEE, history pruning and quiet SEE all run; only quiet futility is gated | ADOPT with the extension decision (S3): the two are one policy, BAS-S16's trigger |
| Composition with singular | a checking node can take 3 plies (BAS-D11 kept it on WAC) | — | follows the extension decision; BAS-D11 re-read by games (B-90) |

### 3.6 Couplings the donor has and Basilisk lacks

- **`seekMate`** (S5): at root depth ≥ 16 with `|root score| ≥ 2000`
  (≈ 564 cp; donor `:727`, consumed at `:996` and `:1248`) the donor
  caps RFP at depth 6 and disables singular extensions. The singular half
  is B.3's; RFP's is B.4's. Read on the KBNK and KQK fixtures at the five
  budgets (§2.2) and on BAS-R07's rec1 timeline in B.3.2; §5.9.
- **`followPV`** (S8): a stack bit true along the previous iteration's PV;
  gates IIR and quiet pruning at PV nodes.
- **Upcoming repetition** (cuckoo): BAS-S12's retry trigger fires (PLAN B.3:
  the consumer is the TT probe's decisive-bound guard). The donor's form
  after `is_draw` and in quiescence; one pre-registered terminal test.

## 4. PLAN B.3's inputs, disposed

| Input | Disposition |
|---|---|
| B.2.3 `CoreHistoryFills` 0/1 | a categorical of the ON arm, read with the cluster; settled before B.6 |
| B.2.3 correction units | the donor's units replace `CoreCorrLimit`'s categorical (Amendment 4's rule 4) |
| S1 child node types | ON arm (§3.1, §3.2, §3.4) |
| S2 colour-less history indexing | a separate ticket with its own read (shadow counter of cross-colour hits, colour-indexed build on first-move cutoffs, mean cutoff index and bench, then SPRT `[0,3]`); not part of the cluster's ON arm, so its memory cost is priced alone |
| S3 check policy | §3.5 |
| S5 `seekMate` | §3.6, singular half |
| S7 NMP on the static eval | ON arm (§3.1) |
| S8 `followPV` | ON arm (§3.3, §3.6) |
| S10 TT bound-mismatch penalty | present in `edb0d9db` (`search.cpp:914-919`, depth > 5): ticket 9 of the ON arm behind the attribution switch `B3TtPenalty` |
| S12 `double_exts` inheritance | moot: the per-line budget replaces the counter and is written for every child (§5.10, ticket 1) |
| B-3 KBNK multi-budget majority | §2.2 (the rule, base 115); decides `B3MultiCut` before any game (§9) |
| B-5, B-6, B-7, B-97, B-98 | the budget (ticket 1), the cost screen and its driver (ticket 0, §10), `B3Iir` read only after the screen passes with IIR off (§9) |
| B-12 best move stored at fail-low | reclassified as a gap on 2026-11-01 (*Roadmap audit*): a donor-form difference, not a wrong computation, since the store's own product is 7 of the 83 IIR firings on the fixture line (research amendment 6 §1); the donor's form is built in the ON arm behind `B3FailLowStore` (ticket 9) at default 0 and read by §9 item 16's paired run. Before the audit this row read it as a defect by PLAN §2's table whose default waited on item 16 |
| B-14, B-15 correction training at decisive results and singular-excluded nodes | B-14 a defect: repaired (ticket 7, `B3CorrDecisive`) and read on the conversion floors first (§5.5); B-15 the categorical `B3CorrExcluded` |
| B-31, B-32, B-37 NMP verification | §3.1 |
| B-33 cut-node-only NMP at LTC | §3.1 |
| B-34 margin terms against their entry guards | a B.3.1 review item |
| B-35, B-54, B-94, B-112 instruments | §6 |
| B-39, B-42, B-43 ProbCut | §3.2 |
| B-45, B-46, B-50 singular | §3.4; B-46's TT-depth margin 2 against 3 is a categorical |
| B-49, B-53 extension composition | §3.5 |
| B-51 new LMR term bases on the surface | moot: `ttMoveHistory` feeds the double margin, not LMR, in `edb0d9db`; the margin's bases are on the surface (§7) |
| B-55, B-103 IIR range | §3.3 |
| B-89, B-90 zero-game reads mispredicting games | BAS-D11 and BAS-D13 re-read by games |
| B-109 ablation bit 6 also removes the check extension | resolved by construction: the ON arm has no check extension, so bit 6 is the singular block alone and stays matched to the oracle's extension family; the categorical's extension is tune-build only (ticket 10) |
| B-114 CI covers the cluster's switch arm | B.3.1 adds the OFF arm to CI, as B.2 did |
| Double-extension counter and cap | replaced by the per-line budget (B-7); the cap bound 5 times in 51.5M nodes, so no deletion games are owed (§5.10) |
| Same-ply NMP verification | kept readable as `B3NmpVerify=2`; counter-decided before any `[-5,0]` is owed (§9) |
| Upcoming repetition | §3.6 |

## 5. Interactions with Cluster 1's fitted core

Numbers are §2.1's (the repaired head, `suite_v2` at depth 14, stride 1).
Each item runs producer → stored state → consumer and names what the ON
arm moves and which fitted Cluster 1 coordinate sees it. The cluster is
read as one unit: no mechanism is judged alone, and the surface B.3.3
fits is composed from these couplings by a measured rule (5.12), not by
cluster membership.

### 5.1 The in-check population: the check policy against every move-loop rule, LMR and hindsight

- 26.4% of interior nodes are in check (13.6M of 51.5M; B.0 read 15.9%
  on the 1.10.1 head: Cluster 1 prunes the quiet population harder, so
  the share of forcing nodes rose by two thirds). Every one is extended;
  none is reduced or move-loop pruned: `lmr_blocked_in_check` is 10.8% of
  LMR-eligible moves (3.15M of 29.2M). The ON arm removes the extension
  and admits these nodes to count pruning, capture futility, SEE pruning
  on both branches, continuation-history pruning, quiet SEE, LMR, the
  first-move step-down and the `depth −= 3` rule, gating only quiet
  futility: the donor's form. Cluster 1's margins were fitted (B.2.3) on
  the out-of-check population; the donor's constants were fitted with
  in-check nodes inside it. The two are reconciled by reading which rules
  the in-check nodes actually reach (5.12), not by reasoning.
- In check the record's estimate is the inherited `(ss − 2)->eval`, so
  capture futility and LMR's `alpha − eval` term read a two-ply-old value
  there, `improving` is false by construction and `opponent_worsening`
  compares the inherited value — all the donor's form. Count pruning in
  check can skip only quiet evasions past the threshold and only while
  `best_score > −tablebaseWinInMaxPly` (the donor's `!is_loss`): once a
  loss is the best, nothing is pruned, so the lone saving evasion is never
  count-skipped.
- Singular and the check extension compose today on 23.5% of singular
  extensions (83,457 of 354,448 fire at in-check nodes) and 16.1% stack a
  double with the check extension (three plies; BAS-D11's 24.6% on the
  old head). On the ON arm the stack is the donor's graded 1/2/3 alone,
  bounded by the budget (5.10).
- Where it bites first: forcing lines got their depth from the check
  extension; on the ON arm they get it from singularity. The conversion
  floors and the KBN-K majority rule (§2.2) bind every ticket of the ON
  arm, and the check-policy categoricals (§9) are read before anything
  else, so a loss there is not misattributed to the proof searches.

### 5.2 Proof-search children and the cut-node LMR term (S1)

NMP 3.85M, ProbCut 0.72M and singular 1.24M child searches per 51.5M
interior nodes, every child typed cut. On the ON arm the null child is an
all node (the donor's `cutNode = false`), ProbCut's child `!cut_node`, the
exclusion search's `cut_node`. `CoreLmrCutNode` (3997, about 3.9 plies)
and `CoreLmrCutNodeNoTt` (864) were fitted with those children cut-typed,
and the typed TT-cutoff rule (`CoreTtCutoffNodeTyped` 1) reads the type
too. The two LMR coordinates join B.3.3's surface (5.12); the TT-cutoff
categorical is not re-read, its rule being symmetric in the type.

### 5.3 RFP against NMP: the crossover band (B-35)

| Remaining depth | RFP cuts / interior | NMP tries / interior | NMP conversion |
|---|---:|---:|---:|
| 1–3 | 14.07% | 2.29% | 37.5% |
| 4–7 | 0.81% | 4.19% | 42.5% |
| 8+ | 0.05% | 1.00% | 46.8% |

RFP owns the shallow band (margin 101·d, cap 10) and NMP the deeper one;
the crossover is at depth 4. NMP converts 41.5% overall (B.0: 49.6% on
the 1.10.1 head; the oracle about 83%): Cluster 1's RFP takes the easy
cuts first and leaves NMP a residual that converts worse. That residual is
what the cut-node population and the static margin `151 − 16·d −
16·improving` shrink (P3). RFP runs before NMP, so NMP's change does not
move RFP's population; RFP's coordinates stay off B.3.3's surface unless
5.12's rule pulls `CoreRfpCorrMult` in through 5.5.

### 5.4 Static against TT-refined estimate (S7)

NMP's entry reads the TT-refined `eval` today, as RFP does. On the ON
arm NMP reads the corrected static eval with the margin and sizes R on
`static_eval − beta`, while RFP keeps `eval`: a TT lower bound RFP has
already consumed no longer inflates NMP's population or its reduction.

### 5.5 Singular: the margin, `tt_pv`, `ttMoveHistory`, correction, ProbCut's store

- 1.24M exclusion searches (2.41% of interior): 28.5% extend, 11.8%
  multi-cut, 8.3% take the negative extension, 51.4% do nothing. 84.3% of
  extensions are doubles (a double margin of 4 under a singular margin of
  4·d: BAS-D11's finding, now stronger), the double cap bound 5 times
  (0.001%), and 2.0% of tries are lone-move exclusions (A.11.2). With the
  margin at 2/ply (seed) and the donor's 0.26/ply reachable, the question
  changes from "every other move 4·d below the table's score" to "below by
  a fraction of a pawn": tries stay (entry unchanged but for `6 + tt_pv`,
  shuffling and `seek_mate`), extensions rise, multi-cuts under the
  donor's `value ≥ beta` rule rise (Rarog: 32% of attempts under that rule,
  19% under its own), negatives fall.
- `tt_pv` (persisted; 2.26% of interior) raises the singular depth floor by
  one and the non-PV margin by 0.30/ply; the five `tt_pv` LMR terms see the
  same nodes and are not refitted.
- `ttMoveHistory` is one scalar per thread, trained since B.2 at every
  non-PV node's end (918 / −747) and consumed nowhere today. The ON arm
  adds the multi-cut update (−421 − 110·d) and its one consumer, the
  double margin (−331·tmh/114178, at most ±24 cp). It is not an LMR term
  in `edb0d9db`; B-51 is moot and the margin's bases are on the surface
  instead.
- Correction: the multi-cut trains it (a new producer; the bonus clamped
  ±256 as every update) and the double and triple margins read |corr|
  (a new consumer, capped at 101 cp by rule 4). `CoreRfpCorrMult` 538
  and LMR's `|corr|/7419` term were fitted on today's producers. B-14's
  repair (decisive results train) widens the producer population in won
  positions, where RFP's term can grow by up to 284 cp at saturation and
  LMR's by up to 2.6 plies less reduction: read on the conversion floors
  at five budgets with the attribution switch before any game (§9), not
  left for the paired run to misattribute.
- B-44: ProbCut's store (lower bound at `probCutDepth + 1` with the
  capture) admits a later singular search on that capture (`tt_depth ≥
  depth − 3`, bound lower); a capture TT move lowers the double and triple
  bars by 43 and 53 cp (the `!ttCapture` terms) and raises LMR by
  `CoreLmrTtCapture` 964. The donor carries the same loop.

### 5.6 IIR against hindsight, the fail-low store and `followPV`

- IIR fires at 0.81% of interior nodes; 56.4% of its firings are the
  stale-entry clause (a TT move present, the entry shallower than
  `depth − 3`) and 0.3% land on a node hindsight already reduced. B.0's
  "IIR against hindsight" is answered: they do not overlap, both stay.
  The stale clause is half of today's IIR and becomes the categorical
  `B3IirStale`.
- B-12's repair (no move stored after a fail low) enlarges the no-TT-move
  population, which IIR, razoring's and RFP's `tt_move == NONE ||
  tt_capture` admission, `CoreLmrCutNodeNoTt` and the first-move
  step-down's +1127 all read. `CoreLmrCutNodeNoTt` joins the surface
  (5.12); the step-down constant is the donor's. B.3.2 reads the share of
  interior nodes without a TT move on both arms (`tt_move_absent`).
  The population effect is measured (research amendment 6):
  `tt_move_absent` 65.4% → 72.7% of interior nodes; each of its separable
  consumers alone flips the promotion fixture and the KR-K position at
  30,000 nodes. The store is read as §9 item 16, and B.3.3's sweep of
  `B3IirMinDepth` and `CoreLmrCutNodeNoTt` runs on whichever population
  that run chooses.
- `followPV` gates IIR at PV nodes and quiet pruning at PV nodes off the
  previous iteration's PV (S8): at iteration 1 and below the first
  deviation from the previous PV the bit is false, so PV nodes there are
  reduced and pruned as the donor's are. Producer: the driver's previous
  PV; MultiPV lines after the first carry an empty previous PV.

### 5.7 ProbCut against quiescence (B.4), `improving` and the TT cutoff

- ProbCut tries 1.40% of interior and converts 54.0% (B.0: 68.4%). Its
  pre-verification is quiescence, which B.4 changes (TT cutoff, corrected
  stand-pat, futility base, SEE). B.3 lands ProbCut on today's quiescence;
  B.4.0 re-reads `probcut_cuts/tries` and NMP's depth-0 nulls (`depth − R
  ≤ 0` lands in quiescence) as inputs of its own.
- ProbCut reads `improving` after `improving |= static_eval ≥ beta` (the
  donor's order; today it reads the flag before the OR). Count pruning,
  the LMR table term and hindsight read the OR'd flag on both arms.
- The small TT ProbCut returns `beta + 121` from a lower-bound entry with
  `tt_depth ≥ depth − 4` and `tt_score ≥ beta + 121`, in check too: a
  depth-discounted cutoff the TT-cutoff rule (`tt_depth > depth − 1`, the
  graph-history check at depth ≥ 7) does not take, and without the
  graph-history guard (the donor's form).

### 5.8 Upcoming repetition against draws, the TT and mate distance

The cuckoo check runs after `is_draw(ply)` and before mate-distance
pruning and the TT probe, in negamax and in quiescence: when `alpha < 0`
and a reversible move reaches a position already on the path (at the
root's children, a repeated pre-root position), alpha rises to 0 and the
node returns 0 on `alpha ≥ beta`. It pre-empts a TT cutoff from a stored
decisive bound (A-13's consumer: a losing bound stored along a path
without the repetition), uses the fixed draw value 0 (B-84), reads the
board's key history and `plies_since_null`, and never touches a node with
`alpha ≥ 0`. Mate-distance pruning sees the raised alpha.

### 5.9 `seekMate` against the mate drives (S5)

At root depth ≥ 16 with |root score| ≥ 564 the ON arm disables singular
extensions (B.4 caps RFP at depth 6). Every `KNOWN_WIN`-class position
qualifies, so in a won ending at depth ≥ 16 the whole singular block,
multi-cut included, is inert — the opposite of Rarog's KBNK failure mode.
The floors at 30k–120k nodes reach root depths of about 12–20 in KBN-K,
so the five-budget read spans both regimes. The producer is the driver's
last completed iteration score (raw, not the tablebase-corrected one).

### 5.10 The extension budget against hindsight and negative reductions (B-7)

The budget counts positive singular (and, under `B3CheckExt=1`, check)
extensions along a line against `root_depth_`; hindsight's +1 and LMR's
negative reductions (`new_depth + 2`) are not extensions and stay outside
it (Rarog amendment 6). Today's double cap bound 5 times in 51.5M nodes;
the budget is expected never to bind at depth ≤ 14 (P8) and exists for
the deep iterations the cost screen reads. It replaces `double_exts` and
`DoubleExtMax`, which resolves S12 (the null and ProbCut children inherit
the budget by the same write) and retires the simplification read's
deletion arm: a guard that never binds costs no games.

### 5.11 What B.3 hands on

- B.4: `probcut_*` and `null_*` after the quiescence change; the small TT
  ProbCut beside the quiescence TT cutoff; RFP's `seekMate` cap and its
  `VALUE_KNOWN_WIN` bound (BAS-S40's note).
- B.5: the two root producers (`root_score_`, the previous PV) belong to
  the driver; optimism enters the corrected static eval and therefore
  every B.3 margin; aspiration puts beta in the mate range more often,
  where NMP (`!is_win`), ProbCut (`!is_decisive(beta)`), the small
  ProbCut and multi-cut (`!is_decisive`) stand down by construction.
- B.6: the Cluster 1 coordinates 5.12 pulls onto B.3.3's surface are the
  measured case for the joint tune; if none crosses the rule, B.6's skip is
  justified by that read.

### 5.12 The surface rule: cohesion by measured activation

B.3.3 fits Cluster 2's own coordinates (§7) plus every Cluster 1
coordinate whose consumer population the ON arm moves: a coordinate joins
when, on B.3.2's depth-14 read of ON against OFF, its mechanism's firing
share of interior nodes (or of moves tested, for the move-loop rules)
changes by 10% or more of itself, or its in-check share of firings exceeds
10% (B-102's rule, applied across the cluster boundary). The candidates
named by 5.1–5.6, read in that order: `CoreLmrCutNode`,
`CoreLmrCutNodeNoTt` (5.2, 5.6), `CoreLmpBase`, `CoreQuietFutBase`,
`CoreCaptFutBase`, `CoreCaptSeeCoeff`, `CoreContHistPruneSlope`,
`CoreLmrTableScale`, `CoreLmrOffset` (5.1), `CoreRfpCorrMult` and
`CoreCorrUpdateScale` (5.5). The list is frozen here; B.3.2 records each
candidate's two shares and the verdict, and B.3.3's registration names
the surface that results.

## 6. Zero-game reads, done

1. §2.1's counters on the repaired head: done,
   `tools/results/b30-20261030/suite_d14.json` (106 positions, depth 14,
   Hash 64, the Diag binary of §13).
2. The double-extension cap binds 5 times in 354,448 extensions (0.001%)
   at depth 14; the ON arm does not carry it (5.10).
3. IIR against hindsight: 0.3% overlap; the stale clause 56.4% of IIR (5.6).
4. The floors: §2.2, the repaired head's row (BAS-S40's read).
5. B-35's crossover band: 5.3. B-54: the in-check share is 26.4% on the
   head; the oracle build `oracle-1.10.1.exe` exposes no `Diag` option, so
   the oracle's own share stays unread and B-54 is read as ON against OFF.
6. WAC at 100,000 nodes on the head: **240/300** (20,450,883 nodes,
   `tools/results/b30-20261030/wac100k_head.json`, the release PGO
   `basilisk-b12-fix-pext-pgo.exe` at `d8d30e9`, engine-identical to the
   head), the base of the ON arm's floor (§11).
7. B-94 (canary calibration under neutral perturbations) and B-112
   (counter units) are B.3.2's first reads: the canary set at Hash 32 and
   128 against 64 on the OFF arm fixes how many canaries flip under a
   neutral change, and the new counters' units are declared in
   `run_suite.py` at ticket 0.

## 7. Seeds

By B.0 §7.2's rules: plies, counts and history units take the donor's
value (rule 1); an evaluation-unit constant takes the donor's converted
value (rule 3) unless it lies more than 2× outside the range of the
oracle-converted and Basilisk-fitted values, when it takes their geometric
mean, or the oracle's alone where Basilisk has no fitted coordinate
(rule 2); an evaluation difference multiplied into another unit is scaled
into the donor's units first and a scaled term fed by a bounded table
keeps the donor's bound (rule 4). The oracle is classical Stockfish
`9587eeeb` (×0.485); the donor's evaluation units convert by ×0.282.
Every seed is an integer in the unit named; `d` is the remaining depth.

| Constant | Donor (SF 19) | ×0.282 | Oracle | ×0.485 | Basilisk fitted | Rule | Seed |
|---|---|---:|---|---:|---|---|---|
| NMP entry: per ply | 13 | 3.7 | 33 | 16 | none (`eval ≥ beta`) | 2, oracle alone (B.0 §7.2 names the NMP margin) | **16** |
| NMP entry: improving | 47 | 13 | 33 | 16 | none | 2, oracle alone | **16** |
| NMP entry: constant | 365 | 103 | 311 (+112·ttPv) | 151 (+54) | none | 2, oracle alone; the donor's shape has no `ttPv` term | **151** |
| NMP beta floor | −2000 | −564 | — | — | none | 3; off the surface | **−564** |
| NMP reduction base, depth divisor | `7 + d/3` | plies | `(737 + 77d)/246` | plies | `3 + d/4` | 1 | **7, 3** |
| NMP reduction, eval divisor | 256, uncapped | 72 | 192, capped at 3 | 93 | 192, capped at 3 | 3 (72 within 2× of 93); uncapped as the donor | **72** |
| NMP verification | depth ≥ 16, region `ply + 3·(d − R)/4` | plies | — | — | depth ≥ 10, same ply | 1; off the surface (B-32) | **16, 3/4** |
| ProbCut margin: base, improving | 241, 64 | 68, 18 | 176, 49 | 85, 24 | 189 | 3 | **68, 18** |
| ProbCut depth: minimum; search depth | 3; `d − (improving ? 5 : 3)` | plies | — | — | 5; `d − 4` | 1 | **3; 3 / 5** |
| ProbCut TT-value guard, store depth | skip when `ttValue < probCutBeta`; store at `probCutDepth + 1` | — | — | — | none; store at `d − 3` | 1 | as the donor |
| Small TT ProbCut | 428, `ttDepth ≥ d − 4` | 121 | — | — | none | 3 | **121, 4** |
| IIR minimum depth | 6 | plies | IID at 7 | plies | 8 (rail) | 1; range 3..10 (B-55, B-103) | **6** |
| Singular minimum depth | `6 + ttPv` | plies | 6 | plies | 5 | 1 | **6 + ttPv** |
| Singular TT-depth margin | 3 | plies | 3 | plies | 3 | 1; 2 is the categorical `B3SingTtDepth` | **3** |
| Singular margin, per ply (1/63 cp) | 59 (0.94/ply) | 0.26/ply | `(4 + formerPv)·d/2` | 0.97/ply | 4/ply | 2: 0.26 is more than 2× below 0.97; geometric mean of 0.97 and 4 | **126** (2.0/ply); range 13..378 so the donor's 17 is reachable (B-45) |
| Singular margin, `ttPv && !PV` per ply (1/63 cp) | 66 | 0.30/ply | `formerPv·d/2` | 0.24/ply | none | 3 | **19** (0.30/ply); range 0..126 |
| Double margin: base, PV, `!ttCapture`, deep (`ply > root depth`) | −2, 204, −152, −38 | −0.6, 57.5, −42.9, −10.7 | — | — | 4, non-PV only | 3 | **−1, 58, −43, −11** |
| Double margin: correction term | `−|corr|/198368` | rule 4 | — | — | none | 4: `−min(|correction_raw|/198368, 101)` in cp | fixed (F line) |
| Double margin: `ttMoveHistory` term | `−1175·tmh/114178` | ×0.282 | — | — | none | 1 (history units), 3 | `−331·tmh/114178` (F line) |
| Triple margin: base, PV, `!ttCapture`, ttPv, deep | 70, 279, −188, 81, −43 | 19.7, 78.7, −53.0, 22.8, −12.1 | — | — | none | 3 | **20, 79, −53, 23, −12**; correction term as the double's |
| Extension | `1 + (v < sb − double) + (v < sb − triple)`, then `depth++` at the node | plies | — | — | 1 or 2 | 1 | as the donor |
| Negative extension | −3 when `ttValue ≥ beta` or `cutNode` | plies | — | — | −1 when `ttValue ≥ beta` | 1 | **−3** |
| Multi-cut | `value ≥ beta`, not decisive: return `value` | — | — | — | `s_beta ≥ beta`: return `s_beta` | 1; Rarog's rule is `B3MultiCut=1` | the donor's |
| `ttMoveHistory` updates | multi-cut `−421 − 110·d`; node end `918 / −747` | history units | — | — | node end only (B.2) | 1 | as the donor |
| Multi-cut correction bonus | `(v − se)·sd·177/1024`, clamp ±LIMIT/4 | rule 4 | — | — | none | 4 | `clamp((v − se)·1000/282·sd·177/1024, ±256)` |
| `seekMate` | root depth ≥ 16, `|root score| ≥ 2000` | 564 | — | — | none | 1, 3 | **16, 564** (F lines) |
| `is_shuffling` | not a capture; rule50 ≥ 10; plies from null ≥ 6; ply ≥ 20; the from-to chain over `ss − 2`, `ss − 4` | counts | — | — | none | 1; never tuned (the donor's non-regression guard) | as the donor |
| Extension budget per line | none (Rarog: the root depth) | plies | — | — | `DoubleExtMax` 16 on doubles | B-7 | `root_depth_` of positive extension plies; no coordinate |
| Upcoming repetition | after `is_draw`, `alpha < draw` → `alpha = draw` | — | — | — | absent | 1 | the donor's, with the fixed draw 0 |
| TT bound-mismatch penalty (S10) | `penalize(1)` at depth > 5 | plies | — | — | absent | 1 | as the donor |

Three seeds move far from Basilisk's fitted values and are flagged for
B.3.2's paired run and B.3.3's sweep: the NMP reduction base (3 → 7), the
ProbCut margin (189 → 68) and the singular margin (4/ply → 2/ply with
0.26/ply in range). B.3.3's ranges span all three columns, as B.2.3's did;
the generated surface is Cluster 2's X lines plus the Cluster 1
coordinates 5.12 admits.

## 8. Decision: `READY_FOR_IMPLEMENTATION` (2026-10-30)

§8's conditions of draft 1 are met: §2.1, §5, §6 and §7 are complete;
every categorical, attribution switch and deletion arm has a registered
rule and bracket (§9); the B.3.1 contract lists its tickets, each with its
fingerprint rule and tests (§10); the predictions are frozen and the
B.3.2–B.3.4 registrations written (§11). The one decision draft 1 left to
the maintainer, the check policy, is taken here as recommended: the ON arm
carries the donor's policy (no in-check extension; reductions and
move-loop pruning in check), and the current policy stays readable as the
first two categoricals of §9, so overturning the choice before B.3.1 costs
two defaults, not a return. The evidence: B.0 §2.6 (both engines pay about
1.9× nodes for their extension families and buy no fixed-node tactics
with them), 5.1 (26.4% of interior nodes in check on this head, none
pruned or reduced, a population Cluster 1's fit never saw), BAS-S16's
trigger (the information pruning acts on has changed), and the donor's
constants being fitted with in-check nodes inside the population. The
counter-argument is BAS-S08's −10.17 ± 6.52 for the bare removal on the
old head and the mate-drive fragility; both are answered by reads that
precede any game (the floors at five budgets, the WAC floors) and by the
categorical order of §9, which reads the check policy first.

Not implemented, and why: an oracle-side counter build for B-54 (no
owner leaf; the read is ON against OFF); a per-move check extension in
the oracle's form (absent in the donor; B.0 §3.5); Rarog's per-line budget
as a tuned coordinate (the budget has none; B-7 asks for a bound, not a
knob).

## 9. Categoricals, attribution switches and deletion arms

Tune-build UCI options, each settled by one 2,000-game paired run on one
tune build at `3+0.03` (PROCESS *Cluster delivery shape*: adopted at
≥ +5 Elo, rejected at ≤ −5, left at the default between; never an SPSA
coordinate), in the order below, before B.3.3's sweep. Items 1-3 are read
before the whole-arm paired run, which then runs on the defaults they
leave (item 4's zero-game read is done: the default stays 0); items 4-16
follow it. Items 6, 7, 8 and 11 run only if B.3.2's depth-14 counters
show the alternative moving its mechanism's firing share by 10% or more
of itself (5.12's rule); otherwise the default stays and the read is
recorded. Adoption and rejection need the 95% interval to exclude 0 as
well as the ±5 estimate (PROCESS, 2026-11-01). Roadmap audit of
2026-11-01; the original sentence stands in that section.

| # | Option | Values | Default | What decides | Bracket |
|---|---|---|---|---|---|
| 1 | `B3CheckExt` | 0 no in-check extension; 1 the unconditional extension | 0 | paired run at `3+0.03`, then at `10+0.1` (BAS-X01's re-read) | ±5 at both; adopt 1 only if neither read is ≤ −5 and one is ≥ +5 |
| 2 | `B3CheckPrune` | 1 the donor's reductions and move-loop pruning at in-check nodes; 0 none in check | 1 | read A: the default (0, 1) against the legacy policy (1, 0), BAS-S08's re-read; read B: the default against (1, 1) | ±5; read A ≤ −5 flips both defaults to the legacy policy and reopens 5.1 |
| 3 | `B3NmpCutOnly` | 1 cut nodes only; 0 any non-PV node | 1 | paired runs at `3+0.03` and `10+0.1` (B-33's LTC sign) | ±5 each; keep 1 unless the LTC read is ≤ −5 |
| 4 | `B3MultiCut` | 0 the donor's `value ≥ beta`; 1 Rarog's `s_beta ≥ beta` | 0 | the KBN-K majority rule first (zero-game, §2.2): if the ON arm fails it and passes with 1, the default flips to 1 without games; else a paired run | ±5 |
| 5 | `B3NmpVerify` | 1 the region; 0 none; 2 the legacy same-ply form at depth ≥ 10 | 1 | counter first: `null_verify_failed / null_verifications` on the OFF arm (depth 14; the depth-20 wire check); a paired run 1 against 0; 1 against 2 only if the same-ply form changes more than 1% of null cuts, else PLAN's `[-5,0]` is retired as behaviour-identical | ±5 |
| 6 | `B3SingTtDepth` | 0 margin 3; 1 margin 2 (B-46) | 0 | paired run (Rarog +12.9 ± 9.4, a prior only) | ±5 |
| 7 | `B3IirStale` | 0 no TT move only; 1 also an entry shallower than `depth − 3` | 0 | paired run (the clause is 56.4% of today's IIR) | ±5 |
| 8 | `B3ProbCutPv` | 1 PV nodes admitted; 0 non-PV only | 1 | paired run | ±5 |
| 9 | `B3Cuckoo` | 1 on; 0 off | 1 | the terminal test (§10); paired run | ±5 |
| 10 | `CoreHistoryFills` | 0 fresh tables cleared to 0; 1 the donor's fills (−5, −742, −1338, −586, +5) | 0 | paired run (B.2.3's residual; before B.6) | ±5 |
| 11 | `B3CorrExcluded` | 1 correction trains at singular-excluded nodes (the donor); 0 refused | 1 | paired run, last (B-15, low stakes) | ±5 |
| 12 | `B3Iir` | 1 on; 0 off | 1 | only after the deep-iteration cost screen passes with 0 (B-5); then a paired run | ±5 |
| 13 | `B3ExtBudget` | 1 on; 0 off | 1 | no games unless `ext_budget_bound > 0` at a B.3.2 read (identical trees otherwise) | ±5 |
| 16 | `B3FailLowStore` | 0 the best-scored move stored after a fail low (the OFF arm's form); 1 no move stored (the donor's form, B-12's repair) | 0 | one paired run, last in the order, with `tt_move_absent` read on both settings at depth 14 before it (research amendment 6) | ±5 |

Items 14 (`B3SingNegative`) and 15 (`B3PvQuietPrune`) are research
amendment 3's (§5).

Attribution switches: defaults are the repair or the donor's form and a
paired run never moves them (PLAN §2: a correct repair that loses is
reworked, not reverted). They exist so a negative paired run of the arm is
ablated by switch before anything is blamed, and so the conversion floors
can read one repair alone.

| Option | Values | Default | Read |
|---|---|---|---|
| `B3CorrDecisive` | 1 decisive results train the correction (B-14 repaired); 0 refused | 1 | the conversion floors at five budgets with 0 and 1 before any game (5.5); ablation on a negative paired run |
| `B3SingularQuietLmr` | 0 BAS-D18's sibling relaxation removed; 1 kept at 401 | 0 | ablation on a negative paired run or when the fitted-constant analysis asks |
| `B3TtPenalty` | 1 the bound-mismatch penalty (S10); 0 off | 1 | ablation only |

Deletion arms: the double-extension counter and cap are replaced by the
budget (5.10; no games, the cap binds 0.001%); the same-ply NMP
verification is `B3NmpVerify=2` above, counter-decided. PLAN's two
`[-5,0]` brackets are owed only if the counter read shows the deleted form
live (more than 1% of its mechanism's cuts); the packet records the rule
so B.3.2 can apply it without a return.

## 10. The B.3.1 contract (`I2`)

- **Goal and exact semantics.** §3's "Disposition" column, read with §7's
  seeds and §9's defaults, behind one CMake `option(B3_PROOF ... OFF)`
  (compile definition `BASILISK_B3_PROOF`; flipped ON in one commit after
  B.3.4). Both arms build at every commit; the OFF arm reproduces
  `bench 13` **16,615,757** at every commit; the ON arm's fingerprint is
  recorded per ticket. Engineering is the implementer's: the preferred
  form is one `constexpr bool` from the definition with `if constexpr`
  branches inside the kernel's existing blocks, so both arms compile on
  every build and the dead arm is eliminated, rather than a second kernel
  file. The OFF arm's code is not edited except for seams both arms share
  (counters, trace, new stack fields the OFF arm leaves at their
  defaults); the OFF arm is neither fixed nor maintained after B.3.4.
- **Why here.** §1, §5, §8.
- **Producer → stored state → consumer.** §5. New producers: `ss->follow_pv`
  (the driver's previous-iteration PV; MultiPV lines after the first carry
  an empty one), `ss->ext_plies` (written for every child by the parent:
  the move loop adds the positive extension, the null and ProbCut children
  inherit, a same-ply re-entry keeps the node's), `root_score_` (the
  driver, after each completed iteration, the raw score; 0 at `go`),
  `nmp_min_ply_` (the NMP verification), `Board::upcoming_repetition(ply)`
  (the cuckoo tables over the key history, bounded by
  `min(rule50, plies_from_null)`).
- **Files and subsystems.** `src/search_kernel_core.cpp` (every
  mechanism); `src/search_worker.h`, `src/search_worker.cpp` (stack fields,
  the driver producers); `src/search_params.h` (a `BASILISK_B3_PARAMS(X, F, S)`
  group: X the coordinates, F the fixed donor terms, S the switches of §9);
  `src/search_diagnostics.h`, `.cpp` (counters, trace events);
  `src/move_picker_core.h`, `.cpp` (the ProbCut stage); `src/board.h`,
  `src/board.cpp`, `src/zobrist.*` (cuckoo); `src/tt.h` (the penalty
  seam); `CMakeLists.txt`; `.github/workflows/ci.yml`;
  `tools/diag/run_suite.py` (units), `tools/diag/decision_trace.py`
  (events), `tools/generate_spsa_surface.py` (the B3 surface); `tests/`.
- **Invariants that must not change.** DESIGN §3: no static score in the
  mate band, and no mechanism admits a decisive value where the donor
  refuses one (NMP returns no win, ProbCut and the small ProbCut refuse a
  decisive `beta` or `tt_score`, the singular entry and multi-cut refuse a
  decisive `tt_score`/`value`, the cuckoo check never touches `alpha ≥ 0`);
  TT scores ply-adjusted on store and probe; every TT move validated at
  every consumer (the ProbCut stage's TT move included); the stored static
  eval is the raw one; the fail-high blend unchanged; a legal PV and best
  move; nothing at the root is pruned, reduced or extended by any B.3
  mechanism; the exclusion search keeps the node's `tt_pv`; the node
  checks hold at every entry (the same-ply re-entries of the verification
  and the exclusion included); the per-ply stack unwinds (`test_stack_unwind`);
  `ply < MAX_PLY − 2` bounds every extension; a line's positive singular
  extensions never exceed `root_depth_` (the budget's own invariant); the
  draw value stays 0; the mate-drive floors, the wrong-bishop draws (B.12)
  and the mate band bind both arms at every ticket.
- **Required instrumentation.** Ticket 0.
- **Deterministic tests.** Per ticket below; all bind both arms.
- **Cheap local qualification.** The list after the tickets.
- **Maintainer-owned expensive gate.** B.3.2's paired run (BAS-S41),
  then B.3.4's SPRT (BAS-S43), §11.
- **Acceptance/rejection rule.** §11.
- **Non-goals and adjacent mechanisms not to alter.** Razoring, RFP, the
  move-loop margins, the LMR terms, the histories, the correction form and
  weights, quiescence, root, aspiration and time are re-wired where a B.3
  mechanism reads them and otherwise untouched; no constant outside §7 is
  moved by hand; no adjacent heuristic (no per-move check extension in the
  oracle's form, no passed-pawn or recapture extension, no Rarog-only
  device such as LDSE or a demotion rule); no tuning before B.3.3; a
  Cluster 1 coordinate that 5.12's read pulls onto the surface is fitted
  there, never adjusted in B.3.1.

### Tickets

Ordered; one mechanism each; every ticket keeps the OFF arm's exact
fingerprint and records the ON arm's. From ticket 2 on, each ticket ends
with the ON arm's `bench 13` counters at stride 1 (Diag build) against the
previous ticket's, read for both clusters' rows: a Cluster 1 row
(`rfp_cuts`, `razor_cuts`, the move-loop family, `lmr_applied`,
`lmr_researched`, `lmr_floor_hits`, the cutoff sources, `tt_cutoffs`,
hindsight, the correction updates) whose share of interior nodes moves by
more than 20% of itself is reported in the implementation record with its
cause. The implementer does not repair such a move (a Cluster 1 constant
is never touched here); the record is B.3.2's and 5.12's input.

0. **Instruments and the switch.** The `B3_PROOF` option and definition;
   the CI `flavors` job gains a cell that configures `-DB3_PROOF=ON`,
   builds `all`, runs CTest and prints `bench 13` (B-114). Counters,
   appended, never renamed: `null_verifications`, `null_verify_failed`,
   `null_region_refusals`, `probcut_tt_cuts`, `probcut_pv_tries`,
   `sing_ext3`, `sing_negative_cut`, `sing_shuffle_refused`,
   `sing_seekmate_refused`, `ext_budget_bound`, `follow_pv_nodes`,
   `iir_pv_nodes`, `cuckoo_draws`, `tt_move_absent`, `tt_penalized`,
   `in_check_lmr_applied`, `in_check_skip_quiets`, `in_check_capt_fut`,
   `in_check_capt_see`, `in_check_cont_hist`, `in_check_quiet_see`, each
   with its denominator in the table's comment and its unit in
   `run_suite.py` (`B3_COUNTER_UNITS`, validated when present). Trace
   events: `NullVerify`, `NullRegionRefused`, `ProbcutTtCutoff`,
   `ExtBudgetTruncate`, `CuckooDraw`, `TtPenalty`; `SingularExtension`'s
   `reduction` field carries the plies (1, 2 or 3); `decision_trace.py`
   learns the names. `tools/diag/deep_iteration_screen.py`: two engines,
   a position list, `go depth N` for N in a range under a per-depth
   watchdog (default 60 s), cumulative nodes per depth, the per-position
   geometric mean of the arm/baseline ratio over the depths both finish,
   JSON out (B-97, B-98). Both arms 16,615,757.
1. **Producers and the budget.** `ss->follow_pv`, `ss->ext_plies`,
   `root_score_`, the previous PV kept by the driver; `ext_plies` written
   for every child and bounded by `root_depth_` for the singular
   extension on the ON arm (truncated, not refused; `ext_budget_bound`
   counts truncations); `double_exts` and `DoubleExtMax` are dead on the
   ON arm. Node check: a child's `ext_plies` equals its parent's plus the
   parent's positive extension. Tests: the budget's edge arithmetic (full,
   truncated, spent); `follow_pv` true at the root, true at a child only
   when the parent's is and the move matches the previous PV's ply, false
   otherwise (a kernel-driving test as `test_stack_unwind`). Both arms
   16,615,757: the budget never binds at bench depth, and a test forcing
   `root_depth_` small proves the bound live.
2. **Check policy.** No in-check extension on the ON arm (`check_exts`
   reads 0 at `B3CheckExt=0`); in-check nodes admitted to count pruning,
   capture futility, SEE pruning on both branches, continuation-history
   pruning and quiet SEE (quiet futility gated), to LMR, the first-move
   step-down and `depth −= 3`; switches `B3CheckExt`, `B3CheckPrune`;
   `sing_in_check` and `sing_triple` count the categorical's extension
   only. The floors on both arms; WAC `--count` on the ON arm.
3. **NMP.** Population `cut_node` only (`B3NmpCutOnly=0` restores any
   non-PV node), not in check, no excluded move, non-pawn material,
   `static_eval ≥ beta − 16·d − 16·improving + 151`, `ply ≥ nmp_min_ply_`,
   `beta ≥ −564`; `R = 7 + d/3 + max((static_eval − beta)/72, 0)`; the null
   child `NonPV` with `cut_node = false`; a null score `≥ beta` that is a
   win is not returned; at depth ≥ 16 outside a verification, the region
   `nmp_min_ply_ = ply + 3·(d − R)/4`, a same-ply verification at
   `d − R` with window `(beta − 1, beta)` and `cut_node = false`, the
   region cleared after it, the null score returned only if the
   verification reaches beta; inside a verification (`nmp_min_ply_ != 0`)
   the null score returns without verifying, as the donor's
   `nmpMinPly || depth < 16`. `B3NmpVerify` 0/1/2 (2 keeps the legacy
   same-ply form at depth ≥ 10 for the counter read). B-37: the
   implementation record lists every stack field the same-ply re-entry
   writes (`move_count`, `stat_score`, `(ss+2)->cutoff_cnt` reset,
   `(ss+1)->cutoff_cnt` through its children, `tt_pv` and `follow_pv`
   recomputed equal) and shows by the node checks and `test_stack_unwind`
   that the outer node reads none of them except `(ss+1)->cutoff_cnt`,
   which the donor shares. Wire check: `go depth 20` on bench positions
   11 and 15 reads `null_verifications ≥ 1` on the ON arm.
4. **IIR.** `!follow_pv && !all_node && depth ≥ 6 && tt_move == NONE`
   (`B3IirStale=1` adds the stale-entry clause; `B3Iir=0` removes IIR);
   `iir_pv_nodes` counts PV-node firings.
5. **ProbCut.** After IIR (the donor's order), with `improving` already
   OR'd with `static_eval ≥ beta`; `depth ≥ 3`, `!is_decisive(beta)`, not
   in check, the TT-value guard (`tt_score` valid and `< probcut_beta`
   skips), PV nodes admitted (`B3ProbCutPv=0` restores non-PV only);
   `probcut_beta = beta + 68 − 18·improving`; a picker stage over captures
   and queen promotions in the capture stage's order, the TT move first
   when it is one, each admitted by `see_ge(m, probcut_beta −
   static_eval)` and the excluded move skipped; `probcut_depth = depth −
   (improving ? 5 : 3)`; quiescence at `(−probcut_beta, −probcut_beta + 1)`,
   then, when it holds and `probcut_depth > 0`, the search at
   `probcut_depth` with `cut_node = !cut_node`; on `value ≥ probcut_beta`
   store `(probcut_depth + 1, value, LOWER, move, raw static eval, tt_pv)`
   and return `value − (probcut_beta − beta)` unless decisive. The small TT
   ProbCut at the move loop's entry, in check too: a lower bound with
   `tt_depth ≥ depth − 4` and `tt_score ≥ beta + 121`, neither `beta` nor
   `tt_score` decisive, returns `beta + 121` (`probcut_tt_cuts`).
6. **Singular.** Entry: not root, `m == tt_move`, no excluded move,
   `depth ≥ 6 + ss->tt_pv`, `tt_score` valid and not decisive, the bound
   proves lower, `tt_depth ≥ depth − 3` (`B3SingTtDepth=1`: 2),
   `!is_shuffling`, `!seek_mate` (root depth ≥ 16 and `|root_score_| ≥
   564`); `s_beta = tt_score − (126 + 19·(ss->tt_pv && !is_pv))·depth/63`
   (integer division as the donor's); `s_depth = (depth − 1)/2`; the
   exclusion search `NonPV` at `(s_beta − 1, s_beta)` with the node's own
   `cut_node`. On `s_val < s_beta`: the double and triple margins of §7
   (the correction term `min(|correction_raw|/198368, 101)`, the
   `ttMoveHistory` term `331·tmh/114178`, `deep = ply > root_depth_`),
   `extension = 1 + (s_val < s_beta − double) + (s_val < s_beta − triple)`
   under the budget, then `depth++` at the node. Else on `s_val ≥ beta`
   and not decisive (`B3MultiCut=1`: `s_beta ≥ beta` instead): the
   `ttMoveHistory` update `−421 − 110·depth`, the correction bonus when
   not in check and `s_val > static_eval`, and return `s_val`. Else on
   `tt_score ≥ beta || cut_node`: `extension = −3`. `singular_quiet_lmr`
   and `lmr_singular_quiet` are dead on the ON arm (`B3SingularQuietLmr=1`
   restores them for ablation); the double cap is gone. Tests: the margin
   at depths 6, 10, 20 with and without the `tt_pv` term; the extension
   in [1, 3] and monotone in the gap; the negative at a cut node with
   `tt_score < beta`; multi-cut admission under both rules; the
   `is_shuffling` truth table; the `seek_mate` gate with forced
   `root_depth_` and `root_score_`. At this ticket's end the KBN-K
   majority rule at five budgets with `B3MultiCut` 0 and 1 (§9 item 4).
7. **Correction and the fills.** `B3CorrDecisive` (B-14's repair: the
   `!is_decisive(best_score)` refusal removed on the ON arm, the bonus
   clamped as every update), `B3CorrExcluded` (the exclusion refusal
   removed), `CoreHistoryFills` (fresh tables filled with the donor's
   constants when 1); the floors at five budgets with `B3CorrDecisive`
   0 and 1 (5.5), recorded.
8. **Upcoming repetition.** The cuckoo tables (for every reversible move
   of every piece between two squares, the key `piece ^ from ^ to ^
   side`, two hashes into 8192 slots as the donor's `H1`/`H2`, built once
   at start-up and asserted complete), `Board::upcoming_repetition(ply)`
   over the key history with `min(rule50, plies_from_null)` as the bound
   and the between-squares occupancy test, the two consumers (negamax
   after `is_draw(ply)` and before mate-distance pruning; quiescence after
   its draw check), `B3Cuckoo`. Tests: every reversible move's key is
   found; no false positive on 10,000 random legal positions against a
   slow reference that enumerates the reversible moves; the pre-registered
   terminal tests, FENs recorded in the test (BAS-S12's trigger): (a) a
   position reached by a pre-root move list in which the side to move is
   lost by evaluation and by a depth-2 search but can play a reversible
   move completing a repetition reads **0** at `go depth 2` on the ON arm
   and the loss on the OFF arm; (b) a losing bound stored in the TT from
   a path without the repetition is pre-empted by the draw on a path with
   it, read as 0 at `go depth 1` on the ON arm.
9. **TT.** The fail-low store (`B3FailLowStore=1`: the move stored is the
   one that raised alpha, none after a fail low); the bound-mismatch
   penalty (S10: at a non-PV node whose entry is deep enough and valid
   but whose bound lies on the wrong side of the window at depth > 5, the
   entry's depth is lowered by one through a `tt.h` seam that rewrites
   the same payload with `depth − 1` and nothing else; BAS-C05's known
   tolerance applies as to any store); `B3TtPenalty`.
10. **Mask, floors, CI and the final state.** Bits 2, 3, 4 and 6
    re-declared on the ON arm's mechanisms (bit 6 is the singular block:
    positive, multi-cut and negative; the check extension exists only
    under the tune-build categorical and never in the Ablate flavour, so
    B-109 is resolved by construction and bit 6 stays matched to the
    oracle's extension family); the liveness proof
    (`tools/diag/ablation_liveness.py`) on the ON arm; `test_wac --floor`
    (175) re-registered on the ON arm (Amendment 2's rule; `--count`
    recorded per ticket until then); the sanitizer suite on both arms;
    the final two-arm table in B.2's §20 shape (`bench 13` on Release,
    Tune, Diag and Ablate, `bench 10` sanitizer, CTest, WAC at depth 6 and
    at 100,000 nodes, both arms); CI green on both arms.

### Cheap qualification (end of B.3.1, before the review)

- The two-arm table of ticket 10, every fingerprint reproduced from a
  clean tree.
- The ON arm's WAC at 100,000 nodes **≥ 232** (240 − 8: the cluster may
  cost at most what one oracle family costs, B.0 §2.6) and at depth 6
  ≥ 175; the 400,000-node and `depthpv 10` reads recorded.
- The conversion floors at the five budgets on the ON arm: KQ-K, KR-K and
  KBB-K 24 at every budget; KBN-K's sum over budgets **≥ 112** (the head's
  115 less 3); the wrong-bishop draws; `B3CorrDecisive` 0 and 1 read.
- The depth-20 wire check (ticket 3) and `ext_budget_bound` at depth 20.
- The deep-iteration cost screen on five positions — bench positions 11
  and 15 and `suite_v2` positions 50, 42 and 60 (the three largest trees
  at depth 14: 31.2M, 10.8M and 8.9M nodes) — at depths 16–19, ON against
  OFF on the same idle host under the 60 s watchdog: per position the
  geometric mean of the cumulative-node ratios over the depths both finish
  is **≤ 3**, and the ON arm finishes every depth the OFF arm finishes.
- The liveness proof on the ON arm's mask, every bit moving nodes.
- The decision trace on B.2's four positions, both arms, archived.
- The KBN-K multi-cut read (ticket 6) and its verdict on `B3MultiCut`.

### Review

A separate session reproduces every fingerprint and test on both arms,
reads each ticket's diff against this section and §7, checks the B-37
field list and the budget's invariant, and records its acceptance (or
return) in the packet before B.3.2 starts.

## 11. Frozen predictions and the B.3.2–B.3.4 registrations

Frozen 2026-10-30 before any ON-arm build exists. Calibration is appended
in the experiment entries, never here. IDs reserved: **BAS-S41** (B.3.2),
**BAS-S42** (B.3.3), **BAS-S43** (B.3.4); the entry files are created at
each leaf's opening with the text below copied verbatim, before any game.
Priors: Rarog's unfitted Cluster 2 read +18.4 ± 9.4 (RAR-S79), its
in-range singular margin +24.2 ± 9.4 (RAR-S80) and its fit added nothing
measurable over the margin (RAR-S84); Rarog's verdicts do not transfer
(BAS-X01), only its designs.

Amendments 1-6 and the *Roadmap audit* of 2026-11-01 replace clauses of
this section through their *clauses replaced* lists; an entry file is
assembled from this section plus every such list, never from this
section alone (today: the sweep's singular margin is 252 with 17 and 126
as extra points, BAS-S43 is PLAN's B.3.5, and BAS-S41's order is the
audit's).

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, ON against OFF at one revision, §7 seeds, §9 defaults) reads **+10 Elo**, 80% interval [−15, +35]; probability positive 65%; probability of the −40 defect stop 15% | moderate | below −40 is a defect or a wrong seed column; the switches of §9 ablate it before any mechanism is blamed, in the order check policy, `B3NmpCutOnly`, the singular margin at 378 (today's 4/ply) |
| P2 | After B.3.3 and the gate, B.3 reads **+20 Elo at STC**, 90% interval [0, +45]; H1 probability 55% | moderate | H0 at `[0,3]` or an estimate under +5 refutes the coherent-cluster share for proof searches; the residual then lies in B.4/B.5 |
| P3 | NMP: `null_tries/interior` falls to **≤ 50%** of the OFF arm's 7.48%; conversion rises from 41.5% to **≥ 65%**; the region fires at depth ≥ 16 and refuses under 5% of its null cuts there | moderate-high | conversion under 55% says the margin or the population did not land |
| P4 | Singular: tries stay within 1.5–3.5% of interior; extensions rise from 0.69% to **≥ 1.2%** of interior; multi-cuts 10–30% of tries; triples ≤ 15% of extensions; negatives 5–25% of tries | moderate | extensions under 0.9% say the margin did not reach the population; triples above 25% say the bars are mis-scaled |
| P5 | Nodes: the ON arm's `bench 13` within **0.6–1.4×** of the OFF arm's; `suite_v2` at depth 14 within 0.5–1.5× | moderate | outside the band on both reads says a mechanism exploded or died; the cost screen and the liveness proof say which |
| P6 | Tactics: WAC at 100,000 nodes in **[232, 262]**; the depth-6 count ≥ 175 | moderate | under 232 is a floor failure and returns the leaf |
| P7 | Floors: KQ-K, KR-K, KBB-K 24 at every budget; KBN-K's five-budget sum **≥ 112** | moderate-high | a miss attributes to the multi-cut rule, `B3CorrDecisive` or the check policy by the switches, zero-game |
| P8 | The budget: `ext_budget_bound` = 0 on `suite_v2` at depth 14 and on `bench 13`; under 0.1% of extensions at depth 20 | high | binding at depth ≤ 14 says the graded extension is mis-scaled, not that the budget is wrong |
| P9 | Speed: pooled NPS **0.98–1.01×** the OFF arm (the cuckoo loop and `follow_pv` are the only per-node additions) | moderate-high | below 0.97 sends the cuckoo loop to B.7 before the gate |
| P10 | Categoricals: `B3CheckExt` 1 against 0 within ±8 (the trigger's premise: with informed pruning the extension's value is gone); `B3NmpCutOnly` 0 within ±5 at STC and ≤ 0 at `10+0.1`; `B3MultiCut` 1 within ±5 if the donor's rule survives KBN-K | low-moderate | `B3CheckExt` ≥ +5 at both controls says the policy decision was wrong and flips the default |

Stop rules: P1 below −40 (the defect stop, then the switch ablation
above); a floor failure at any ticket returns the leaf; PLAN rule 6 for
the programme (two failed coherent clusters). B.3.0's own: if P2 fails
with P3 and P4 held, proof-search architecture does not carry Elo in this
engine at STC and B.4/B.5 re-scope to the smallest donor-shaped changes
with measured activation; if P2 fails and P3 failed, B.3 returns to
`IMPLEMENTED` for a defect hunt.

### BAS-S41 — B.3.2: Cluster 2 diagnostics, the unfitted paired run and the categoricals

- Baseline / candidate: one revision, the accepted B.3.1 head; the
  umbrella-off arm against the umbrella-on arm (`-DB3_PROOF=ON`), seeds
  unfitted (§7), defaults §9; PGO pairs per arm (PROCESS *Cluster delivery
  shape*); the categoricals on one tune build of the same revision, the
  alternative as arm A by UCI option against the defaults as arm B.
- Zero-game screens, all recorded before the first game: §2.1's counter
  table on both arms at depth 14 (the 5.12 candidates' shares and verdicts
  included); the fixed-node differential against the oracle at 300,000
  nodes (`run_suite.py --oracle`), the B.0 recipe; WAC at 100,000 and
  400,000 nodes and `depthpv 10`; the crossover band (5.3) and the
  in-check share (B-54) on both arms; the canary calibration (B-94: the
  OFF arm at Hash 32 and 128 against 64, flips counted) before the ON
  arm's canaries are read; the cost screen (§10) if B.3.1 ran it on an
  earlier revision; pooled NPS (`tools/nps_pool.ps1`, ≥ 2 PGO builds per
  arm, an idle host); the `B3NmpVerify` counter read.
- The paired run: 2,000 games at `3+0.03`, 1T, Hash 64, paired reversed
  UHO openings, no adjudication, tablebases off, Colosseum
  `match-fixed`, the recount from the PGN; it governs: P1's interval, the
  −40 stop.
- The categorical runs: §9's table in its order, 2,000 games each, one
  tune build, ±5; items 1 and 3 also at `10+0.1` 1T; the KBN-K and counter
  reads that precede items 4, 5, 12 and 13 recorded with their verdicts.
- Expected: P1, P3–P10. Falsification: P1's stop; the floors.

### BAS-S42 — B.3.3: the curvature sweep, then SPSA in blocks

- Surface: Cluster 2's X lines (§7: NMP entry 3 and reduction 2, ProbCut
  margin 2, the small ProbCut margin, the singular margins 2 and the
  double and triple bars 8, IIR minimum depth) plus the Cluster 1
  coordinates 5.12 admits; the F lines (verification depth, `seek_mate`,
  the correction and `ttMoveHistory` terms, the depth shapes) and every S
  switch stay off it. Ranges span the three columns of §7; the generated
  configuration from `tools/generate_spsa_surface.py` (a B3 table beside
  `config_core.json`) is cited by hash.
- Curvature sweep (PLAN rule 8), classification frozen before the first
  point: NMP entry constant (151), NMP reduction eval divisor (72), ProbCut
  margin base (68), the singular margin (126) with the donor's 17 as a
  sixth, read-only point because the geometric grid does not reach it,
  and the triple margin base (20); each at 0.5×, 0.75×, 1×, 1.5× and 2×,
  `bench 13` and WAC at 100,000 nodes per point, paired depth at 300,000
  nodes (`sweep.py`). Flat or monotone on all five skips the SPSA, and
  the seeds ship as the fitted values.
- SPSA: Colosseum blocks on the tune build at `3+0.03`, the registered
  estimator and block rule (PROCESS *SPSA go/no-go*); theta is the last
  completed block's rounded centres, never a selected checkpoint; the
  fitted-constant analysis (rails, full-step movers, reversals, unit and
  cohesion problems, each with an owner) is written before the bake;
  the bake is one commit with the ON arm's new fingerprint.
- A categorical is never on the surface; a coordinate the sweep finds
  flat is reported, not dropped.

### BAS-S43 — B.3.4: the Cluster 2 gates

- (a) The fitted ON arm against the OFF arm at the bake revision, SPRT
  `[0,3]` nElo at `3+0.03`, 1T, Hash 64, paired reversed UHO, no
  adjudication, tablebases off, clean final-PGO builds and manifests, the
  registered command; the default flips ON on H1.
- (b) The fitted ON arm against the unfitted ON arm, SPRT `[0,3]`, only if
  B.3.3 ran its SPSA (a flat sweep ships the seeds and skips (b)).
- (c) A direction read, not a gate: 2,000 games at `10+0.1` 1T of (a)'s
  pair, recorded with its interval beside `B3NmpCutOnly`'s LTC read (B-33).
- Reject: H0 on (a) reverts the default, keeps the evidence and restores
  the fingerprint record; the categoricals adopted in B.3.2 stay recorded
  with the arm. No candidate, bound, cap, book or adjudication changes
  after a game is observed.

## 12. The four questions

1. **Mechanism.** Proof searches that ask sharper questions on
   better-informed estimates: null move only where a cut is expected and
   the static estimate clears a depth-scaled margin (fewer wasted nulls:
   41.5% conversion today against the oracle's 83%), with a verification
   region instead of a same-ply check that refuses 0.3% of cuts; ProbCut
   from depth 3 on ordered candidates behind a TT guard, returning the
   proven gap, plus the TT-served form; singular extensions that fire on a
   margin near the table's score, so the only-move question is actually
   asked (today it is asked at 4·d below and answered yes for 28% of
   tries), graded one to three plies under a per-line budget, with a
   multi-cut that returns the proven bound and trains the TT-move
   history; IIR off the previous PV; and forcing lines that earn depth
   from singularity rather than from an unconditional check extension on
   26% of interior nodes that no pruning rule may touch. The chess
   property is the same as B.0's: better-informed decisions, not more of
   them, now at the proof-search layer the selectivity core left
   untouched.
2. **Interactions.** §5, with numbers; the cross-cluster surface rule 5.12.
3. **Invariants.** §10's list, from DESIGN §3 and the cluster's own.
4. **Falsifiers.** P1–P10 and the stop rules (§11); the floors at every
   ticket; the cost screen; the terminal tests of ticket 8; BAS-S41's
   categorical reads, each adopted or rejected on its own ±5.

## 13. Artifacts (`tools/results/b30-20261030/`, ignored storage)

| Artifact | Identity |
|---|---|
| Diag binary, the counters' source | `tools/test_engines/basilisk-b30-diag-pext-diag-pgo.exe`, revision `1414039` clean, `bench 13` 16,615,757, SHA-256 `73B7A29D499FDA2F8478E754CEABFFDCC9F409212B379BCFCD8DCC8A4C922501` (manifest beside it) |
| Depth-14 counter suite | `suite_d14.json`, `suite_d14.log` (106 positions, Hash 64, stride 1, the LMR identity holds) |
| WAC at 100,000 nodes | `wac100k_head.json`, `wac100k_head.log` on `basilisk-b12-fix-pext-pgo.exe` (`d8d30e9`, SHA-256 `F1C3D0BC…14ED6E3`, BAS-S40's candidate; engine-identical to the head) |
| The first suite run (stopped on position 89) and the bisect | `suite_d14_head.log`, `bisect_log.txt`, `probe_76_100_rel.txt` (§2.3) |
| Conversion floors, pre-repair head | `conversion/conv_*.txt` (§2.2's first table, `ba31f28`) |
| Conversion floors, repaired head | `../b12-20261030/conversion/conv_*.txt` (§2.2's second table, `d8d30e9`) |
| The donor's `search.cpp` | `git -C D:/code/stockfish show sf_19:src/search.cpp` (`edb0d9db`), the line numbers of §3 |

## Return 1 — B.3.1 ticket 1: the extension budget binds at bench depth (2026-10-30)

**Premise returned.** §10 ticket 1: "Both arms 16,615,757: the budget
never binds at bench depth"; §5.10: "the budget is expected never to bind
at depth ≤ 14 (P8) ... a guard that never binds costs no games"; §4 and
§9: the double-extension cap it replaces "binds 0.001%", so dropping it is
free. Both halves are false at `bench 13`.

**Evidence.** Branch `b3-pruning-extensions`: ticket 0 (`dafe92d`,
`a676317`) and ticket 1 as contracted (`823d087`). Release-preset builds,
PEXT portable, Clang 23.1.4, `bench 13` from the release binary; the
counters from the Diag binary of the same source, one fresh process per
bench position at `go depth 13`, Hash 64
(`tools/results/b31-20261030/bench_fens.epd`, the 40 `BENCH_FENS`). Rows 2
to 5 are temporary edits of `Searcher::ext_budget()` (row 3 also restores
the cap in the singular block's `allow_double` condition and skips the
budget; rows 4 and 5's counters ran with the `ext_plies <= root_depth_`
node check removed), built, read and reverted, never committed.

| ON-arm variant | `bench 13` | `ext_budget_bound` / `sing_fired` |
|---|---:|---:|
| 1. ticket 1 as contracted: budget = `root_depth_`, no cap | **13,315,120** (−19.9%) | **12,800 / 89,121 (14.4%)** |
| 2. no budget, no cap | 18,212,126 (+9.6%) | — |
| 3. no budget, the cap kept (the producers alone) | **16,615,757** | — |
| 4. budget 2 × `root_depth_`, no cap | 11,060,918 (−33.4%) | 346 / 73,894 (0.47%) |
| 5. budget 3 × `root_depth_`, no cap | 13,932,846 (−16.1%) | 398 / 69,226 (0.57%) |

The OFF arm reads 16,615,757, and on the same counter read its double cap
binds 40 times in 68,385 singular firings (`sing_cap_bound`). Bench
position 39 (`1Q4R1/5k2/4rpp1/3K4/8/7p/8/8 b - - 2 9`, queen and rook
against rook with checks) carries 10,913 of row 1's 12,800 truncations,
position 38 808. The bookkeeping is not the cause: on the Diag build the
node checks (a child's `ext_plies` is its parent's plus the parent's
positive extension; never above `root_depth_`) hold through
`test_stack_unwind`, and rows 4 and 5 tripped the second check as they
must. Row 3 shows the ticket's producers (`follow_pv`, `ext_plies`, the
previous PV and root score) are behaviour-neutral.

**Why the premise failed.** The cap counts doubles along a line (16 of
them, up to 32 plies) and was read at depth 14 on `suite_v2`; the budget
counts plies against the iteration's depth, which every search passes
through at 5 to 12 before it reaches 13. On this head 84% of singular
extensions are doubles (§2.1) and the unconditional check extension
lengthens forcing lines so that singular fires deep inside them: a bound of
`root_depth_` plies is tighter than 16 doubles, not looser. And the cap
binds at bench depth itself, so its removal alone moves the fingerprint
(row 2).

**Consequences.** Ticket 1 cannot keep the ON arm at 16,615,757 with the
budget as written. On today's extension the budget is an active
selectivity rule, truncating one firing in seven, not a dormant bound; P8
fails on `bench 13` before the graded extension exists; and §9's
`B3ExtBudget` rule ("no games unless `ext_budget_bound > 0`") would owe
games.

**Options.**

1. *Land the budget with ticket 6 and keep the double cap on the ON arm
   until then.* Ticket 1 becomes the producers alone, which row 3 shows
   reproduce 16,615,757 on both arms. The budget then bounds the graded
   extension it was designed for, after ticket 2 has removed the check
   extension that lengthens the lines it binds on, and `ext_budget_bound`
   is read against P8 at ticket 6 with the counter already in place; a
   budget that still binds at bench depth there returns again. The
   per-ticket counter tables keep ticket 1 neutral, so tickets 2 to 5 are
   read without an unregistered rule under them.
2. *Keep ticket 1 as committed* and record its ON fingerprint
   (13,315,120); P8 is read at ticket 6 instead of ticket 1. The least
   code, but the budget acts as an unregistered pruning rule through
   tickets 2 to 5, and their counter tables carry it.
3. *Loosen the bound* (rows 4 and 5). A change of mechanism, which is
   research's: 2× still truncates 0.47% of firings, and the fingerprints
   are not monotone in the multiple (−33%, −16%), so the bound reaches the
   tree through the TT and ordering rather than locally.

Recommendation: option 1. It keeps ticket 1 behaviour-neutral as the
contract intends, and reads the budget where its premise is defined: on the
donor's graded extension without the check extension.

**State at the return.** Tickets 0 and 1 committed as contracted; the
OFF arm 16,615,757 on Release and Diag; release CTest 25/25 on both arms
(`test_b3_proof` added: the budget's edge arithmetic, the `follow_pv`
predicate, a forced cap that moves the ON tree and its counter and leaves
the OFF tree alone, and in counter builds `follow_pv_nodes` 1 at iteration
1); WAC at depth 6 180/300 on the ON arm (OFF 182). A draft Diag-only
assertion that `ext_budget_bound` is 0 at depth 10 failed on three of
eight bench positions (35, 6 and 142 truncations) and was not committed;
the amendment decides the expectation. Implementation resumes on research
amendment 1 in this packet.

## Research amendment 1 — the extension budget lands with the graded extension (2026-10-30)

Answers Return 1. Class `R2`; no engine code changed; the temporary edits
below were reverted (`git status` clean after the run) and their recipes
are the patches in `tools/results/b31-20261030/amend1/`. The maintainer's
confirmed decisions (§7 seeds, §9 defaults, the donor's check policy as
the ON arm) are not reopened.

### 1. Why the premise failed, plainly

The researcher compared the budget (`root_depth_` plies, 13 at bench
depth) with the double cap (16 doubles, 32 plies) and called the budget
the looser bound. It is the tighter one by a factor of two and a half, and
on a tree whose forcing lines are lengthened by an unconditional check
extension it binds one singular firing in seven. Rarog's own budget arm
was never dormant either: it truncated 44,914 extensions at `bench 13`
(its final implementation record) and shipped at +4.1% bench nodes, so
"a guard that never binds" was this packet's claim, not Rarog's. The
error was arithmetic, and the dormant-guard premise is withdrawn.

### 2. The registered read (question 1)

Registered before the run: three temporary variants of the ticket-1 ON
arm (`823d087`, `B3_PROOF=ON`, `DIAGNOSTIC=ON`, `build/b3-on-diag`), each
with the in-check extension disabled on the ON arm (`!kB3Proof &&` in its
condition), `bench 13` from the same Diag binary (the seam proved the
Diag flavour bench-identical) and the 40 bench positions one fresh
process each at `go depth 13`, Hash 64, `Diag true`, summed by
`run_suite.py --suite tools/results/b31-20261030/bench_fens.epd --depth 13`.
Verdict rule, frozen: `ext_budget_bound / sing_fired` under 1% says the
budget is a guard on an extension shape without the check extension and
ticket 6 asserts a band around that rate; above 5% says it is an active
selectivity rule on any extension shape and `B3ExtBudget` must be read by
games unconditionally; between, both: the band is registered and the
games are owed. What the read cannot show: the legacy singular block is
still in place (84% doubles under a 4-cp double margin), so the rate under
the donor's graded 1/2/3 extension is not this number.

| Variant (check extension off on the ON arm) | `bench 13` | interior nodes | `sing_fired` | `ext_budget_bound` | `sing_cap_bound` |
|---|---:|---:|---:|---:|---:|
| A. the budget as ticket 1, no cap | 5,313,412 | 3,022,493 | 28,399 | **347 (1.22%)** | — |
| B. the double cap restored, no budget | 5,464,579 | 2,944,772 | 27,070 | — | 2 (0.007%) |
| C. no cap, no budget | 5,173,931 | 2,921,302 | 26,701 | — | — |

Against Return 1's rows with the check extension on: the budget's rate
falls from 14.4% to 1.22% of firings, its bench effect from −19.9% to
+2.7% (A against C), and the cap's rate from 0.06% to 0.007%. The check
extension is the amplifier: it lengthens the forcing lines on which
singular doubles chain, and the budget was reading that composition, not
the singular mechanism. One more reading, recorded for P5 and not acted
on: removing the check extension alone takes the ON arm's `bench 13` from
18,212,126 (Return 1 row 2) to 5,173,931, 0.28× — B.0 §2.6's 1.9× was
measured on the 1.10.1 head, and Cluster 1's tree carries far more of its
depth in check (26.4% of interior nodes, §2.1).

**Decision: Return 1's option 1.** Ticket 1 is the producers alone and
keeps the double cap on the ON arm; the budget lands with ticket 6, after
ticket 2 has removed the check extension, on the graded extension it was
designed for. The 1.22% read sits in the rule's middle band, so both
halves apply: ticket 6 asserts a band and `B3ExtBudget` is read by games.
Options 2 and 3 are declined: 2 carries an unregistered pruning rule
through tickets 2–5 and their counter tables; 3 changes the mechanism on
a bench curve that is not monotone in the multiple (−33%, −16%), which
DESIGN §5 forbids as a selection method.

### 3. The double cap (question 2)

The cap binds 40 of 68,385 firings (0.06%) at `bench 13` with the check
extension and 2 of 27,070 (0.007%) without it; §2.1's 0.001% was read at
depth 14 on `suite_v2`, a different suite and depth, and is superseded
by the bench-13 figures for this question. All three are under §9's 1%
threshold: the deleted form is not live, no `[-5,0]` games are owed, and
the cap's removal is priced inside the arm's paired run. The cap leaves
the ON arm at ticket 6, in the commit that replaces the singular block
and lands the budget, so that at no ticket is the ON arm's singular
extension unbounded.

### 4. P8 and `B3ExtBudget` (question 3)

P8 is frozen and is not edited. Its calibration entry, to be copied into
BAS-S41 when that entry is created: *"P8's `bench 13` leg was falsified
on 2026-10-30 before any ON-arm mechanism existed, on the legacy
extension at ticket 1 (12,800 truncations of 89,121 firings). Cause: the
mechanism assumption that a bound of `root_depth_` plies is looser than a
cap of 16 doubles, which is false; the check extension's lengthening of
forcing lines was the missed interaction. Sign and magnitude wrong;
confidence over-calibrated (it was marked high). The prediction is scored
a miss on the mechanism, and the budget's expectation is re-registered as
P8a below, before any ON-arm build carrying the budget exists."*

P8a (new, frozen here): at ticket 6, on the 40 bench positions at depth
13 as above, `ext_budget_bound / sing_fired` lies in **[0.5%, 10%]**
(prior: 1.22% on the legacy extension without the check extension; the
donor's bars make doubles the rule off the PV and triples common, Rarog
21%, so the rate rises); at depth 20 on bench positions 11 and 15 it is
above 0 and **under 25%** of firings; and the cost screen passes. A rate
above 10% at depth 13 or 25% at depth 20 says the graded extension's bars
are mis-scaled for this evaluation (triples too common), and the bars,
not the budget, return to research. Confidence moderate.

§9 item 13 is amended: `B3ExtBudget` (1 on; 0 off; default 1) is read by
a 2,000-game paired run unconditionally, in §9's order after item 12;
0 may be adopted only if the deep-iteration cost screen also passes with
it at 0 (B-5, B-7: PLAN requires a bound on the IIR arms, and the screen
is the only other one).

### 5. The budget's contract (question 4)

- **Definition.** `ss->ext_plies` is the sum of positive singular
  extension plies taken along the line from the root; a singular
  extension of 1, 2 or 3 is granted up to `root_depth_ − ss->ext_plies`
  and truncated at the edge, never refused (`ext_budget_bound` counts
  truncations, `ExtBudgetTruncate` traces them). The check extension
  under `B3CheckExt=1` does **not** consume the budget: it is a tune-build
  categorical, and with in-check nodes at a quarter of the tree a budget
  that counted it would bind at every forcing line and the categorical
  would measure the budget, not the extension. Negative extensions,
  hindsight's +1 and LMR's negative reductions neither consume nor refund
  it (as §5.10).
- **Invariant and node checks** (unchanged from ticket 1, which proved
  them through `test_stack_unwind`): a child's `ext_plies` equals its
  parent's plus the parent's positive singular extension (`ext_taken`),
  the null and ProbCut children and the same-ply re-entries inherit the
  node's value, and `ext_plies ≤ root_depth_` at every entry. Under
  `B3CheckExt=1` the checks read the same, since the check extension is
  not in `ext_taken`.
- **Expected values.** No test asserts `ext_budget_bound = 0` anywhere.
  `test_b3_proof` keeps the edge arithmetic and the forced-cap test
  (`ext_budget_cap`) that proves the bound live. The cheap qualification
  records the depth-13 and depth-20 rates against P8a's bands, and the
  cost screen is the binding test.
- **Fingerprint rules.** Ticket 1: both arms 16,615,757 (the producers
  alone; Return 1's row 3 proved it). Tickets 2–5: OFF 16,615,757, ON
  recorded, the cap still bounding the ON arm's legacy singular block.
  Ticket 6: OFF 16,615,757, ON recorded with the graded extension, the
  budget and no cap; the ticket's counter table includes
  `ext_budget_bound`, `sing_fired`, `sing_ext3` and `sing_double`.
  Tickets 7–10: as §10.

### 6. The quiet-pruning consumer of `follow_pv` (question 5)

A gap in §10: §3.6 and §5.6 give `follow_pv` two consumers and the
tickets place one. The second is assigned to **ticket 2**, which already
rewrites the move-loop admission gates: the quiet branch of move-loop
pruning (continuation-history pruning, quiet futility, quiet SEE), today
`else if (!is_pv)`, becomes `else if (!is_pv || !ss->follow_pv)` — the
donor's `!ss->followPV || !PvNode` — so a PV node off the previous
iteration's PV prunes its quiets as a non-PV node does, and a PV node on
it does not. The capture branch is unchanged (the donor prunes captures
at every node type). A counter `pv_quiet_pruned` (moves pruned by the
quiet branch at PV nodes; denominator `moveloop_tested`) is appended at
ticket 2, extending the table as ticket 0 did; the trace's `FutilityPrune`,
`HistoryPrune` and `QuietSeePrune` records already carry the ply. Cohesion
note for §5.12: quiet futility's and continuation-history pruning's
populations gain PV nodes off the PV, a small share at depth 13 (PV nodes
are under 0.1% of interior nodes); the per-ticket counter table shows it.

### 7. The per-ticket counter method (question 6)

Confirmed: `bench` clears the table once per run and prints no counters,
so "the ON arm's `bench 13` counters at stride 1" means, everywhere in
§10, the 40 bench positions (`tools/results/b31-20261030/bench_fens.epd`,
regenerated from `src/bench.cpp`'s `BENCH_FENS` if missing) one fresh
process each with Hash 64, `Diag true`, `ucinewgame` and `go depth 13`,
summed by `run_suite.py --suite ... --depth 13 --stride 1 --hash 64` on
the Diag build of the same source. Its node sum differs from `bench 13`
(the table is cleared per position); the fingerprint stays the release
binary's `bench 13`, the counters this read. Return 1 used exactly this
method, and so did §2's read.

### 8. Other premises reading the cap, the budget or "never binds" (question 7)

Handoff clauses replaced (old → new); the original sections stand as
written and are read with this list:

- §5.10 "Today's double cap bound 5 times in 51.5M nodes; the budget is
  expected never to bind at depth ≤ 14 (P8) and exists for the deep
  iterations the cost screen reads. ... retires the simplification read's
  deletion arm: a guard that never binds costs no games." → "The budget is
  an active bound on the graded extension (P8a); it replaces the cap, whose
  removal owes no games because the cap binds under 1% (amendment 1 §3)."
- §6 item 2 "the ON arm does not carry it (5.10)" → "the ON arm carries
  it until ticket 6".
- §9 item 13 → amendment 1 §4.
- §10 ticket 1, "`ext_plies` written for every child and bounded by
  `root_depth_` for the singular extension on the ON arm (truncated, not
  refused; `ext_budget_bound` counts truncations); `double_exts` and
  `DoubleExtMax` are dead on the ON arm. ... Both arms 16,615,757: the
  budget never binds at bench depth, and a test forcing `root_depth_`
  small proves the bound live." → "`ext_plies` written for every child;
  the budget's arithmetic and its forced-cap test land here but the bound
  is not applied to the singular extension until ticket 6; `double_exts`
  and `DoubleExtMax` stay live on the ON arm until ticket 6. Both arms
  16,615,757." The committed `823d087` is amended by a ticket-1b commit
  that lifts the bound from the singular block (the cap condition back to
  `ss->double_exts < double_ext_max` on both arms) and keeps everything
  else; its ON fingerprint is 16,615,757 (Return 1 row 3).
- §10 ticket 6, "under the budget, then `depth++` at the node" and "the
  double cap is gone" → unchanged in words, now the ticket where the
  budget is applied and the cap removed; its tests gain the budget's
  application (a forced `ext_budget_cap` moves the ON tree and
  `ext_budget_bound`, as `test_b3_proof` already checks) and its end reads
  P8a's depth-13 band.
- §10 cheap qualification, "The depth-20 wire check (ticket 3) and
  `ext_budget_bound` at depth 20." → "... and `ext_budget_bound` at depth
  20 against P8a".
- §4 rows "Double-extension counter and cap" and "S12" → replaced by the
  cap's removal at ticket 6 (no games) and the budget's inheritance write
  (unchanged).
- §11 P5 is not amended (frozen); amendment 1 §2's 0.28× reading is its
  early calibration input, recorded in BAS-S41 with P8's.

### 9. Condition for resuming

B.3.1 resumes `READY_FOR_IMPLEMENTATION` on this amendment at ticket 1b
(the bound lifted from the singular block, both arms 16,615,757), then
ticket 2 with §6's consumer added, and so on in §10's order with §5's
fingerprint rules. No further research is owed before ticket 6's P8a
read; a rate outside P8a's bands at ticket 6 returns the leaf with the
counters.

## Return 2 — B.3.1 ticket 6: every singular firing extends three plies, and the budget truncates most of them (2026-10-30)

**Premise returned.** Research amendment 1 §4's P8a, frozen before any
ON-arm build carried the budget: at ticket 6, on the 40 bench positions at
depth 13, `ext_budget_bound / sing_fired` lies in [0.5%, 10%]; at depth 20
on bench positions 11 and 15 it is above 0 and under 25%. Its stated
consequence outside the band: the graded extension's bars are mis-scaled
for this evaluation (triples too common), and the bars, not the budget,
return to research. Its prior read "the donor's bars make doubles the rule
off the PV and triples common, Rarog 21%".

**Evidence.** Ticket 6 as contracted (`13cdda0`; surface `03c703e`),
release and Diag builds of the release preset, PEXT portable, Clang
23.1.4; the counters by amendment 1 §7's method (`run_suite.py --suite
tools/results/b31-20261030/bench_fens.epd --depth 13 --stride 1 --hash
64`, `tools/results/b31-20261030/t6/counters_on.json`).

| Read | Value | P8a |
|---|---:|---|
| depth 13, 40 bench positions: `ext_budget_bound / sing_fired` | **107,076 / 139,667 = 76.7%** | band [0.5%, 10%]: **outside** |
| depth 20, bench position 15: the same | **13,216 / 16,037 = 82.4%** (finished, 1.68M interior nodes) | under 25%: **outside** |
| depth 20, bench position 11: the same | **700,551 / 725,009 = 96.6%** (finished in 462 s, 405M interior nodes) | under 25%: **outside** |
| depth 13: `sing_ext3 / sing_fired` | **137,342 / 139,667 = 98.3%** | prior "triples common", Rarog 21% |
| depth 13: `sing_double / sing_fired` (2 or 3 plies) | 139,302 / 139,667 = 99.7% | |
| depth 13: tries, multi-cuts, negatives | 458,890; 97,727 (21.3% of tries); 200,667 (43.7%; 164,191 at cut nodes below beta) | |

The tree: the counter read's interior nodes rose from 2,470,896 at ticket 5
to 58,788,438 (23.8×), `bench 13` from 10,754,654 to 18,049,092 (1.68×),
and WAC fell to 103/300 at depth 6 and 217/300 at 100,000 nodes.

**Why every firing is a triple.** For a quiet TT move at a non-PV node
(not past the root depth), the seeds give a double bar of −1 − 43 = −44
and a triple bar of 20 − 53 = −33 (−10 near a PV), before the correction
term lowers both and the TT-move history term moves the double by at most
±23. A value below the singular bound is below the bound plus 33 or 44, so
both comparisons succeed whenever the exclusion search fails low: the
extension is 3 on every such firing. Only a capture TT move (−1 and +20)
or a PV node (+58 and +79) can see 1 or 2. The donor's bars have the same
sign structure in its own units (−2 − 152 = −154, 70 − 188 = −118), so the
conversion is not the cause: in the donor's form, a singular quiet TT move
at a non-PV node always takes three plies, and P8a's prior of "doubles the
rule, triples common" did not follow from the formula. The budget then
binds after about four firings along a line at root depth 13, which is the
76.7%.

**Reads by UCI option on the Tune build of `13cdda0`** (no code changed;
`tools/results/b31-20261030/t6/on-tune.exe`), for the options below:

| Variant (options on the Tune build) | WAC depth 6 | WAC 100,000 nodes |
|---|---:|---:|
| ticket 6's defaults | 103 | 217 |
| `B3ExtBudget=0` (no budget) | 103 | 207 |
| triple bars at their range tops (`B3SingTripleBase=60`, `B3SingTripleQuiet=0`, `B3SingTripleDeep=0`) | 97 | 216 |
| both bars at their range tops (the above and `B3SingDoubleBase=40`, `B3SingDoubleQuiet=0`, `B3SingDoubleDeep=0`) | 97 | 216 |
| `B3SingMargin=252` (the legacy 4 per ply) | 101 | 217 |
| `B3SingMargin=378` (6 per ply, the range top) | 103 | 230 |
| `B3MultiCut=1` | 101 | 211 |

The engine's `bench` command searches with the default parameters and
ignores `setoption`, so it gives no per-variant fingerprint here. Two
readings: removing the budget makes 100,000-node WAC worse (207), so the
budget is containing the triples, not causing the loss; and no single
singular option restores the ticket-5 arm's 227, let alone 232. P1's
ablation order names "the singular margin at 378 (today's 4/ply)": 378 is
6 per ply at 1/63 per ply; today's 4 per ply is 252, read above.

**The other open findings of this run, which a researcher should read with
this one** (the implementation record below carries their evidence):

- WAC: depth 6 182 → 158 (ticket 2) → 157 → 156 → 109 (ticket 5) → 103;
  100,000 nodes 240 → 238 → 245 → 234 → 227 → 217. Each move of 10 or more
  is attributed with nothing unexplained (ticket 2: the check extension
  −20, the PV quiet pruning −5, pruning in check +1; ticket 5: the small TT
  ProbCut −38 and −18). Both floors (≥ 175 and ≥ 232) bind at the final
  qualification and fail on today's arm.
- The deep-iteration cost screen failed on bench position 11 at ticket 3
  (geometric mean 23.5× at depths 16–17, depths 18–19 unfinished): the
  donor's null move tries at 3.3% of interior nodes against 11.9% there, a
  won middlegame (+20 pawns) where the null move is the main shortcut; no
  single NMP coordinate or switch, the child's type or the legacy
  `eval ≥ beta` gate restores it (the table in the record).
- The small TT ProbCut costs 38 WAC positions at depth 6 and 18 at 100,000
  nodes; restricting it to search-proven entries, non-PV nodes or the
  widest coordinates does not recover them; off, or out of check, does.

**Options.**

1. *Re-scale the bars* (P8a's own consequence). A researcher decides what
   the donor's negative bars mean in a tree whose singular search fires on
   a margin of 2 per ply rather than 0.26, and seeds them so that the
   graded extension separates 1, 2 and 3 plies here; ticket 6 is then
   re-read against P8a.
2. *Keep the donor's bars and re-register P8a* around the budget as the
   binding control (Rarog's budget truncated 44,914 extensions at
   `bench 13`). The tree and WAC reads above say the arm would then fail
   its final floors and cost screen.
3. *Return the whole cluster to research* with this run's evidence: four of
   the six landed mechanisms (the check policy, NMP, the small ProbCut and
   the graded singular) each move a floor or the cost screen on their own,
   which says the cluster's seeds were chosen mechanism by mechanism against
   a population §5.12 meant to read only after the arm was coherent.

Recommendation: option 1 for the singular bars, with option 3's findings
read by the same researcher before ticket 6 resumes: the WAC floors and the
cost screen will not pass on today's arm whatever the bars do, and the
researcher should decide whether the small ProbCut and the donor's null-move
population fit this engine before more tickets land on them.

**State at the return.** Tickets 0–6 committed (the list in the
implementation record below); the OFF arm 16,615,757 at every commit;
release CTest 25/25 on both arms at every ticket (the ON arm's WAC test a
count since ticket 2, by B.2.1 Amendment 2's rule, which ticket 10's
re-registration ends); tickets 7–10 not started. Implementation resumes on
research amendment 2 in this packet.

## Implementation record — tickets 0 to 6, at Return 2 (2026-10-30)

Branch `b3-pruning-extensions`. Builds: the release preset, PEXT portable,
Clang 23.1.4, non-PGO `all` trees for both arms in Release and Diag
(`build/b3-off`, `build/b3-on`, `build/b3-off-diag`, `build/b3-on-diag`)
and a Tune tree of the ON arm for option reads. `bench 13` is the release
binary's; the Diag binary of the same source reads the same count at every
ticket (checked each time). The PGO two-arm table of ticket 10 was not
reached. Per-ticket logs, counter JSON and comparison tables:
`tools/results/b31-20261030/<ticket>/`.

### Per ticket

| Ticket | Commits | OFF `bench 13` | ON `bench 13` | ON WAC depth 6 / 100k | Bench 11, depth 15, ON |
|---|---|---:|---:|---:|---:|
| 0 | `dafe92d`, `a676317` | 16,615,757 | 16,615,757 | 182 / 240 | 1,363,091 (as OFF) |
| 1 | `823d087` (Return 1) | 16,615,757 | 13,315,120 | 180 / — | — |
| 1b | `4f7119d` | 16,615,757 | 16,615,757 | 182 / 240 | 1,363,091 |
| 2 | `64439dd`, `0929527` | 16,615,757 | 5,568,900 | 158 / 238 | 1,303,387 |
| 3 | `b0ff158`, `e5b49ca` | 16,615,757 | 6,765,087 | 157 / 245 | 26,065,865 |
| 4 | `1ba0fc7`, `dcf060f` | 16,615,757 | 8,747,261 | 156 / 234 | 18,087,547 |
| 5 | `2df2732`, `d2df385` | 16,615,757 | 10,754,654 | 109 / 227 | 30,464,545 |
| 6 | `13cdda0`, `03c703e` | 16,615,757 | 18,049,092 | 103 / 217 | — |

Release CTest passed on both arms at every ticket (24 tests at ticket 0,
25 from ticket 1). The ON arm's Diag build ran `test_stack_unwind`,
`test_b3_proof` and `test_search` with node checks live at every ticket,
and passed.

### Cluster 1 rows that moved by more than 20% (amendment 1 §7's read)

Shares of interior nodes, ON arm, against the previous ticket; never
repaired. Full tables: `tools/results/b31-20261030/t<N>/compare.md`.

- **Ticket 2.** `skip_quiets_nodes` +34%, `fut_prunes` +42%,
  `cont_hist_pruned` +81%, `capture_futility_pruned` +47%,
  `quiet_see_pruned` +73%, `capture_see_pruned` +26%, `moveloop_tested`
  +45%, `lmr_applied` +46%, `lmr_researched` +34%, `lmr_floor_hits` +34%,
  `cutoff_src_goodcap` +23%, `hindsight_up` +49%, `hist_reward_updates`
  +41%. Cause: the tree is 0.37×, and its in-check share falls from 19.9%
  to 11.9% of interior nodes once the check extension is gone. In-check
  nodes now enter LMR and the move-loop rules (the `in_check_*` rows), and
  PV nodes off the previous PV prune quiets: `pv_quiet_pruned` is 1.56% of
  interior nodes, about 1.2% of moves tested, more than amendment 1 §6's
  "small share".
- **Ticket 3.** `rfp_cuts` −37%, `razor_cuts` +24%, `fut_prunes` −36%,
  `cont_hist_pruned` +232%, `lmr_researched` +78%, `lmr_floor_hits` −21%,
  `cutoff_src_quiet` +44%, `cutoff_src_badcap` +115%, `hindsight_up` and
  `hindsight_down` −31%, the continuation-correction updates +76%,
  `hist_cutoff_updates` +21%, `hist_reward_updates` −39%. Cause: the null
  move's population narrows (`null_tries` −64%, `null_cuts` −48% of
  interior), so subtrees it used to cut are searched; the counter read's
  tree is 2.1× ticket 2's.
- **Ticket 4.** `capture_futility_pruned` −24%, `hindsight_down` +39%.
  Cause: IIR fires 29% less (the stale-entry clause and the in-check
  firings are gone; PV nodes off the PV are added), so nodes keep a ply,
  which raises `lmr_depth` and the children's remaining depth.
- **Ticket 5.** `razor_cuts` −35%, `fut_prunes` +31%, `quiet_see_pruned`
  +23%, `lmr_researched` −22%, `cutoff_src_badcap` −38%, `hindsight_up`
  +23%, `hindsight_down` +43%, `hist_reward_updates` +73%. Cause: ProbCut
  cuts 5.4% of interior nodes (4×) and the small ProbCut 2.1%, mostly at
  depth 3 and 4, before the node and move-loop rules see those nodes; the
  tree is 0.46×.
- **Ticket 6.** Seventeen of the twenty-three rows. Cause: the counter
  read's tree is 23.8× ticket 5's (Return 2).

### WAC moves of ten or more, attributed (B.2.1 Amendment 2's rule)

- **Ticket 2**, depth 6 182 → 158, by the switches on the Tune build:
  - the PV quiet pruning: −5 (182 → 177);
  - reductions and pruning in check: +1 (→ 178);
  - no check extension: −20 (→ 158).
  At 100,000 nodes: 240 → 244 → 242 → 238. Nothing unexplained.
- **Ticket 5**, depth 6 156 → 109, reverted cumulatively (a temporary build
  without the small ProbCut, then options):
  - the small TT ProbCut: −38 at depth 6 and −18 at 100,000 nodes;
  - PV admission: +3;
  - minimum depth 3 against 5: −9;
  - the margin 68/18 against 189: −6;
  - the rest of the donor's form: +3.
  Nothing unexplained. The small ProbCut's variants (temporary builds, read
  at depth 6 / 100,000 nodes):
  - search-proven entries only (depth ≥ 1): 106 / 225, which refutes
    quiescence's stand-pat stores as the cause;
  - non-PV only: 105 / 226;
  - not in check: 132 / 237;
  - the widest coordinates (margin 240, `tt_depth ≥ depth − 2`): 141 / 234;
  - off: 147 / 245.

### The deep-iteration screen at ticket 3

`deep_iteration_screen.py`, bench positions 11 and 15, depths 16–19, 60 s
watchdog, the ticket-3 release binaries
(`tools/results/b31-20261030/t3/screen.json`):

| Position | Geometric mean of ON/OFF cumulative nodes | Depths compared | ON unfinished | Verdict |
|---:|---:|---|---|---|
| 11 | 23.5 | 16–17 | 18, 19 | fail |
| 15 | 0.64 | 16–19 | none | pass |

The deep iterations do not cause it. Position 11 is a won middlegame
(Re7xb7 wins the queen, the score passes +20 pawns), and its ON tree
exceeds OFF's from depth 7. Ticket 2's arm is clean there (depth 15:
1,303,387 against OFF's 1,363,091).

The depth-15 reads on the ticket-3 Tune build all stay several times OFF's:

| Change | Depth 15 |
|---|---:|
| `B3NmpCutOnly=0` | 11.1M |
| `B3NmpVerify=0` | 16.2M |
| `B3NmpVerify=2` | 15.6M |
| R as 3 + d/4 | 8.8M |
| `B3NmpRedEvalDiv=192` | 37.9M |
| `B3NmpBase=300` | 10.9M |
| a flat margin | 6.5M |
| `B3NmpBetaFloor=-2000` | 11.5M |
| the null child typed cut (temporary build) | 127M |
| the legacy `eval ≥ beta` gate added (temporary build) | 10.7M |

At depth 13 the null move is tried at 3.3% of interior nodes against OFF's
11.9%, and the tree is 15×. The screen is the final qualification's
binding test; this read is its early warning.

### Tests added

- `test_b3_proof`, both arms:
  - the budget's edge arithmetic;
  - the `follow_pv` predicate;
  - a forced `ext_budget_cap` that moves the ON tree from ticket 6 (none
    before) and leaves the OFF tree alone, with its counter in Diag builds;
  - `follow_pv_nodes` 1 at iteration 1 in Diag builds;
  - the singular bound at depths 6, 10 and 20, with and without the PV
    term;
  - the graded extension in [1, 3] and monotone in the gap;
  - multi-cut admission under both rules, never decisive;
  - the negative extension;
  - `is_shuffling`'s truth table;
  - the `seek_mate` gate.
- `test_move_picker_core`: the ProbCut picker's contract over 3,000 random
  positions:
  - exactly the candidates clearing SEE, in descending capture order;
  - a qualifying TT move first;
  - a foreign TT move and the excluded move never delivered.
- Python:
  - `test_deep_iteration_screen.py`;
  - the Cluster 2 counter checks in `test_run_suite.py`;
  - the event names in `test_decision_trace.py`;
  - the B3 surface in `test_spsa_surface.py`.

### Deviations and interpretations, each with its reason

- Ticket 1b. The `ext_plies ≤ root_depth_` node check and the forced-cap
  test's ON expectation move to ticket 6: without the bound neither can
  hold, and Return 1's rows 4 and 5 tripped the check. From ticket 6 the
  check reads `!B3ExtBudget || ext_plies ≤ root_depth_`.
- Ticket 2. The ON arm's `test_wac_floor` runs `test_wac --count` (CMake,
  `B3_PROOF` only) until ticket 10 re-registers the floor. This follows
  B.2.1 Amendment 2's rule, which §10 ticket 10 cites; the OFF arm's floor
  is untouched.
- Ticket 3.
  - The ON condition list is the contract's, which is the donor's: no
    `depth ≥ 3` and no TT-refined `eval ≥ beta`. Basilisk's `allow_null`
    and previous-null guards are kept.
  - The region verification passes `allow_null = true`, so the region
    governs, as in the donor; the legacy form (`B3NmpVerify=2`) keeps
    `false`.
  - B-37: the verification re-enters the node's record and is followed by
    ProbCut, whose children read `(ss-1)->move_count` and
    `(ss-1)->stat_score` (the TT-cutoff penalty and the fail-low parent
    bonus). So the ON arm saves and restores those two, packet §3.1's first
    branch, rather than the ticket's "reads none of them".
  - The other fields the re-entry writes:
    - `tt_pv` and `follow_pv` are recomputed to equal values;
    - `ext_taken` is reset at entry, where it is already 0;
    - `(ss+2)->cutoff_cnt` is reset;
    - `(ss+1)->cutoff_cnt` accumulates, which the donor shares;
    - `move` is reset by undo;
    - `cont_hist` and `cont_corr` are rewritten by the next `do_move`
      before any read.
- Ticket 4. IIR skips in-check nodes on the ON arm (the donor reaches it
  only out of check); the ticket's list is silent on check.
- Ticket 5.
  - The picker delivers queen promotions and captures only, and no
    underpromotion (the donor's capture generation).
  - The store's static eval at an exclusion node is the entry's own raw
    eval, because the node computed none and a store of `INF_SCORE` would
    erase it.
- Ticket 6.
  - The node's `depth++` deepens the moves after the TT move but not the TT
    move itself (`depth_bump`), as the donor's `newDepth` is fixed before
    it.
  - Under `B3MultiCut=1` the admission is the singular bound clearing beta
    and the return is the value, as the ticket says. Neither rule admits a
    decisive value.
  - `ply < MAX_PLY − 2` is added to the entry.
- The X ranges are my provisional choices spanning §7's columns; B.3.3
  registers the surface.
- The packet says "the double and triple bars 8"; the code has nine `X`
  lines (four double, five triple).

### What the reviewer must check

- Each ticket's diff against §10 as amended, and the deviations above.
- The OFF arm's code: compile-time `!kB3Proof &&` prefixes on the legacy
  NMP, ProbCut, IIR and singular blocks, and shared seams only (counters,
  stack fields, the driver's previous PV and score, a `!in_check` on quiet
  futility that the OFF arm never reaches in check).
- The B-37 field list above.
- The budget's invariant under `test_stack_unwind` on a Diag ON build.
- The fingerprints, reproduced from a clean tree.

## Research amendment 2 — the bars, the small ProbCut, the null move's hand-off, the floors (2026-10-30)

Answers Return 2. Class `R2`; no engine code changed; every read below is a
fixed-depth node count or a WAC count on existing binaries and tune
options (deterministic, no idle host needed), except the one temporary
Diag edit of read G, reverted (`git status` clean) with its patch kept.
Recipes, option lists and JSON: `tools/results/b31-20261030/amend2/`
(`sing_summary.json`, `sing2_summary.json`, `proposed_summary.json`,
`wac_summary.json`, `gap_summary.json`, `gap_counters.patch`,
`nmp_pos11.json`, `nmp_pos11_t3.json`, `nmp_pos11_t3_base.json`); logs
beside them as `amend2_*.log`. Binaries: the Tune ON build of `2698b35`
(`build/b3-on-tune`, `bench 13` 18,049,092), the release OFF build
(`build/b3-off`, 16,615,757), the ticket-3 Tune ON build
(`tools/results/b31-20261030/t3/on-tune.exe`, 6,765,087) where the legacy
singular block isolates the null move. Counter reads are amendment 1 §7's
(40 bench positions, one process each, `go depth 13`, Hash 64). A tune
option set outside its range is clamped by `parameters.cpp`, which the
reads below account for.

### 0. What this run says about the contract, plainly

Four of the six landed mechanisms each cost a floor or the screen on
their own, and in each case the cause is the same: a seed or a form was
taken from the donor's column while its partner in the search stayed at
Basilisk's fitted value. The graded extension's quiet-move bars were
converted from the donor, where they sit under a margin eight times
smaller; the small ProbCut's margin was converted from the donor, whose
quiescence stores stand-pat cutoffs at a depth the rule cannot read; the
null move's margin was seeded from the oracle's column without the
oracle's own gate beside it and against a reverse-futility margin fitted
three times less aggressive than the donor's at shallow depth. The
researcher wrote §5.12 to catch exactly this and then exempted the
reverse-futility coordinates from it (§5.3, "RFP's coordinates stay off
B.3.3's surface"); that exemption is withdrawn here. None of this says
the donor's shape does not fit: with the three pairs re-matched the arm
reads the registered 100,000-node floor (read P), and the proof-search
contents stand.

### 1. The singular bars (question 1)

**What the donor's bars mean.** For a quiet TT move at a non-PV node the
donor's double bar is `−2 − 152 = −154` and its triple bar `70 − 188 =
−118` in its units, both negative before the correction and TT-move
history terms, so `value < singularBeta − bar` holds whenever the
exclusion search fails low: the donor's grading is not by the gap at
all for that class. It grades by class: a quiet non-PV TT move that is
singular takes three plies, a capture TT move two (three when the gap
clears about 20 cp), a PV quiet move one (two at about 14 cp, three at 46).
Where the donor spends its triples is therefore wherever its singular
search fires on a quiet move off the PV, and its firing is governed by
the margin alone: `(59 + 66·ttPv)·d/63` in its units, about 0.26 cp per
ply here, so the exclusion search asks whether every other move is a
hair worse than the table's, and the gap distribution at its firings is
whatever the alternatives happen to score. The node's `depth++` then
searches the siblings a ply deeper if the extended move fails low.
Rarog's 21% triple rate came from its own positive quiet bar (its packet:
"the three-ply bar is 16 or less"), not from the donor's.

**What they do here.** Under a margin of 2 cp per ply (§7's seed,
eight times the donor's) the firings are the strongly singular ones, and
their gaps are large: read G (temporary counters at firings, defaults,
depth 13; `gap_summary.json`) gives `s_beta − s_val` under 10 cp at 24%
of firings, 10–30 at 22%, 30–60 at 17%, 60–120 at 16% and 120 or more at
21%; half the firings have a capture TT move (70,008 of 139,667), 44% are
`tt_pv` non-PV nodes, 1.7% PV nodes. With the donor's negative quiet
bars every quiet firing is a triple and every capture firing with a gap
above 20 cp is one: 98.3% (Return 2). Read S (the parts, one at a time,
`sing_summary.json`, `wac_summary.json`):

| Variant (Tune ON build, options) | Interior nodes, depth-13 read | Triples / fired | Budget truncations / fired | WAC depth 6 / 100k |
|---|---:|---:|---:|---:|
| S0 ticket 6's defaults | 58,788,438 | 98.3% | 76.7% | 103 / 217 |
| S1 triple bars at their range tops (base 60, quiet 0, deep 0) | 6,261,283 | 77.8% | 42.1% | 97 / 216 |
| S2 S1 and the double bars at their tops (base 40, quiet 0, deep 0) | 17,332,884 | 90.0% | 67.3% | 97 / 216 |
| S3 negative extension off (`B3SingNegative=0`) | 84,096,359 | 99.1% | 79.7% | 103 / 210 |
| S4 the budget off (`B3ExtBudget=0`) | 398,432,076 | 99.8% | — | 103 / 207 |
| S5 the donor's margin (`B3SingMargin=17`) | 10,179,441 | 95.2% | 55.5% | 103 / 208 |
| S6 S5 with S1's triple bars | 170,816,906 | 96.0% | 85.6% | — |
| S7 Rarog's multi-cut rule (`B3MultiCut=1`) | 89,021,881 | 99.0% | 82.7% | 101 / 211 |
| S10 the donor's bars with the quiet triple term dropped (`B3SingTripleQuiet=0`) | 9,171,009 | 91.9% | 52.2% | 102 / 220 |
| S11 S10 and the quiet double term dropped | 9,775,841 | 92.1% | 55.0% | 102 / 220 |
| S12 S11 and the double base at Basilisk's fitted 4 | 10,920,167 | 95.2% | 57.9% | 102 / 220 |

Ticket 5's read is 2,470,896 interior nodes and the OFF arm's about
8.0M (ticket 2's 2,967,802 at 0.37×). Readings: the negative quiet
triple term alone is the 24×: dropping it (S10) brings the tree to 1.15×
OFF's and the 100,000-node WAC up by 3; the tree is not monotone in the
bars (S2 is larger than S1 with stricter doubles, DESIGN §5's chaos), so
no bar is chosen by node count; the negative extension is load-bearing
(S3 grows the tree and costs 7 at 100k); the donor's margin alone costs
9 at 100k here (S5) and with graded triples explodes (S6, 171M), so §7's
seed of 2 cp per ply stands; the budget is containing an explosion, not
causing one (S4: 398M interior nodes without it, 68× ticket 5), which
settles amendment 1's `B3ExtBudget` question in the budget's favour
before any game; Rarog's multi-cut rule is worse here (S7). The rate of
triples stays above 90% under S10 because the gaps are large: at this
margin the third ply is earned by the gap on most firings, which is the
donor's own logic, not a defect.

**Seeds.** §7's rules give no seed for the quiet terms: the oracle has no
graded extension and Basilisk no fitted triple, so rule 3 converted the
donor's terms, which encode "quiet non-PV equals three plies" and explode
this tree. Research decision, by read S' (`sing2_summary.json`; verdict
rule frozen before it: admissible if under 4× ticket 5's nodes and
within 5 of S1's 216 at 100k; among admissible candidates the one with
the fewest terms changed from the donor, never the best WAC): the triple
bar keeps the donor's base (20), PV (79), `tt_pv` (23) and deep (−12)
terms and drops the quiet term (**`B3SingTripleQuiet` seed 0**, range
−110..0 so B.3.3 can fit it back); every other bar term stays at §7's
value, the double bar included (doubles by default off the PV, which
Basilisk's own fitted 4-cp margin already gave, BAS-D11). S11 and S12
change more for the same reads and are not taken. This is a §7 seed
and the maintainer confirmed §7: the amendment recommends the change
and the maintainer decides it (§7 of this amendment).

### 2. The small TT ProbCut (question 2)

**The donor's surroundings.** The donor stores a quiescence stand-pat
cutoff at `DEPTH_UNSEARCHED` (−2) with `BOUND_LOWER`, a searched
quiescence result at `DEPTH_QS` (0), and its rule reads
`ttData.depth >= depth − 4`: a stand-pat entry can serve it only at depth
1 and 2, a searched quiescence entry at depth up to 4, and an interior
entry otherwise. Basilisk stores its stand-pat cutoff at depth 0
(`search_kernel_core.cpp`, the `tt_store(hash, 0, stand_pat, TT_BETA, …)`
line), so a static evaluation alone can serve the rule at depth 3 and
4. The record's "search-proven entries only: 106/225" says that is not
the main cost, and "not in check: 132/237" says the in-check application
is; both are read at the seed margin of 121.

**What decides it here.** Read W's last row: with the margin at its
range top, 240, beside S1's bars, the arm reads **108 / 233** against
S1's 97 / 216, and the record's "widest coordinates" row (240 with the
depth margin 2) read 141 / 234. The cost is the margin, 428 × 0.282 =
121 cp: at that margin a lower bound four plies shallower cuts far more
nodes here (3.99% of interior at ticket 6) than the donor's rule cuts in
its own tree, and the lost positions are tactical. The donor's eval units
convert at 0.282 on the per-position median and 0.26–0.37 by phase (B.0
§2.4); a margin on a TT bound is a comparison of two search values, not
of two static evaluations, and the conversion may not apply to it at
all.

**Decision: the mechanism stays in the arm behind `B3ProbCutTt` (1 on,
0 off), default 0, read by games in B.3.2 directly after §9's item 5;
its switched-on form carries the margin at 240 (range 100..400) and the
donor's in-check application.** Off by default because every local read
of it is a tactical cost (−38/−18 at the seed, and the proposed arm with
it at 240 still sits at the floor's edge, read P: 229 at 100k against
233 without the quiet-bar change), because the donor's rule here reads
entries the donor's own stores would not serve it (the stand-pat depth),
and because the margin's conversion rests on an evaluation-unit factor
that a comparison of two search bounds may not obey; its prior is small
(+6.4 alone in Rarog, declined there). The seed 240 for the on-form is
the one setting that held the floor, flagged as floor-driven: the sweep
and B.3.3 fit it over the range if the paired run adopts the switch.
Evidence that would change the choice: `B3ProbCutTt` 1 against 0 at or
above +5 adopts it (and widens the range down for the fit); at or below
−5 it leaves until B.4 realigns the quiescence stores. B.4.0 inherits
the note on stand-pat store depth (−2 against 0).

### 3. The null move at bench position 11 (question 3)

Read N on the ticket-3 arm (`nmp_pos11_t3.json`, `nmp_pos11_t3_base.json`;
the ticket-6 arm's reads in `nmp_pos11.json` are dominated by the singular
explosion and are not used): bench position 11, `go depth 13` and `15`,
Hash 64, one process each, OFF 431,430 / 1,363,091.

| Setting on the ticket-3 Tune ON build | Depth 13 | Depth 15 |
|---|---:|---:|
| defaults | 8,260,302 | 26,065,865 |
| N1 every null-move coordinate at its legacy value (any non-PV node, same-ply verification at depth 10, margin `beta − 2d + 50` (the floors of the ranges), R `3 + d/4 + (Δ/192)`, beta floor −2000) | 637,576 | 3,744,714 |
| N2 any non-PV node (`B3NmpCutOnly=0`) | 1,077,697 | 11,096,013 |
| N3 the margin alone at the floors (`beta − 2d + 50`) | 584,978 | 6,511,939 |
| N4 the legacy R alone | 1,712,263 | 8,862,522 |
| N5 N1 but cut nodes only | 2,092,322 | 11,354,853 |
| N6 the base at its floor (`beta − 16d − 16i + 50`) | 3,145,702 | 17,163,504 |
| N8 the base at 100 | 3,352,196 | 15,245,588 |

Verdict rule, frozen before the run: N1 within 1.5× of OFF says the
donor's population, margin and reduction together are the cost and no
single coordinate carries it; above 3× says the ON block differs beyond
its settings. N1 reads 1.48× at depth 13 and 2.75× at depth 15: the
form as a whole, with the residual in what no option reaches (the
TT-refined `eval ≥ beta` gate, the null child's type, the eval term's
cap of 3). The margin is the largest part (N3 against defaults, 14× at
depth 13), and its shape is what matters: a margin that is permissive at
depth and restrictive at shallow depth (the donor's and the oracle's:
`151 − 16d` is +103 at depth 3 and −57 at depth 13) is five times the
tree of a flat margin near beta (N6 against N3) on this position. At
shallow depth the donor's margin asks for a static eval about a pawn
above beta before a null move, because its reverse futility already cuts
from 48 cp above beta at depth 3 (`min(45 + 4d, 85)·d` less the
improving term, converted); Basilisk's fitted RFP cuts from 205–303 cp
above beta at depth 3 (`101·d − 98·improving`), so on the ON arm the band
between the two margins, which the legacy null move covered at every
non-PV node, is searched in full. Position 11 is a won middlegame whose
proof runs through exactly that band, and the OFF arm's legacy null move
collapses it; the ON arm searches it. This is a property of the pair, not
of the null move alone, and not something a later ticket absorbs (ticket
5 read 30.5M there).

**Decision.** Three things, none a change of population:
- `CoreRfpPerPly`, `CoreRfpImproving` and `CoreRfpDepthCap` join B.3.3's
  surface by §5.12's rule, on this evidence; §5.3's exemption is
  withdrawn. The hand-off band between RFP and NMP is a joint property,
  and the fit must see both ends.
- The oracle's own gate goes beside the oracle's constants as a
  categorical, **`B3NmpEvalGate`** (1: the null move also requires the
  TT-refined `eval ≥ beta`, the oracle's and the legacy kernel's gate; 0:
  the donor's form), **default 1**: the record read the gate at 10.7M
  against 26.1M at depth 15, the seed rule took the oracle's constants
  (B.0 §7.2 rule 2) and the oracle never ran them without that gate. The
  default is a change to the ON arm's form the maintainer has not
  confirmed: the amendment recommends it (§7).
- The deep-iteration cost screen is re-scoped to what it was registered
  for, an explosion at deep iterations: on each position the rule reads
  the growth of cumulative nodes from depth 16 to 19 on the ON arm
  against the OFF arm's growth over the same depths, geometric mean of
  the per-depth growth ratios **≤ 1.5**, plus the ON arm finishing every
  depth the OFF arm finishes within the watchdog; the absolute ratio is
  recorded, not judged. A position where the ON arm's tree exceeds the
  OFF arm's from depth 7 (position 11 from ticket 3) is a shape finding
  for the paired run and the fit, not an explosion. This re-registration
  is made before the screen is re-run; the screen needs an idle host and
  was not run here.

### 4. The floors (question 4)

With §1's seed and the small ProbCut at its range top (read P,
`proposed_summary.json`; the switch of §2 and the gate of §3 cannot be
set on ticket 6's build): P1 (`B3SingTripleQuiet=0`,
`B3ProbCutTtMargin=240`) reads **110 / 229** at depth 6 and
100,000 nodes, interior nodes 6,575,559 on the depth-13 read (0.82× the
OFF arm's); read W's S1 bars with the same margin read 108 / 233. With
the small ProbCut off, the record's ticket-5 read is 147 / 245, and the
singular ticket under §1's seed costs about 7 at 100k on top of ticket
5's 227 (S10 220), so the resumed arm is expected at about **235 ± 5** at
100,000 nodes and 100–115 at depth 6; the gate read of ticket 3 had no
WAC leg and is read at ticket 10. Against the floors:

- **100,000 nodes, 232 (240 − 8): stands and binds.** It is the fixed-node
  tactical floor, the one instrument B.0 §2.6 and BAS-D11 showed to be
  robust to the depth-for-width trade, and the ON arm reads it. A read
  under 232 at ticket 10 returns the leaf.
- **Depth 6, 175: re-based on the ON arm** under B.2.1 Amendment 2's rule
  (research decides the ON arm's depth-6 floor with both numbers in hand).
  The fixed-depth count is not comparable across the arms: the ON arm
  turns nodes into depth (B.0 §2.6: extensions off took the median
  completed depth at 100,000 nodes from 10 to 14 on both engines), so its
  depth-6 tree is a different budget. The ON arm's `test_wac --floor`
  is re-registered at ticket 10 from its own count less 13, the headroom
  the OFF floor was calibrated with (188 → 175), and from then on guards
  the ON arm's own drift. The OFF arm's 175 is untouched.

### 5. Whether the cluster as contracted still fits (question 5)

It fits with three pairs re-matched, and it does not return as a whole:
the contents, the order and the switches stand, the OFF arm is exact at
every commit, and the reads above locate each cost in a seed or a gate,
not in a mechanism. **B.3.1 is `READY_FOR_IMPLEMENTATION` on this
amendment once the maintainer takes the two decisions of §7**; the leaf
stays returned until then. What resumes:

- Ticket 5 is amended in place: the `B3ProbCutTt` switch added, default
  0; `B3ProbCutTtMargin` seed 240, range 100..400, for its on-form.
- Ticket 3 is amended in place: `B3NmpEvalGate` (default 1 if confirmed).
- Ticket 6 is re-done on §1's seed: `B3SingTripleQuiet` seed 0; everything
  else as built. Its end reads P8b (§6) and the arm's WAC at 100,000
  nodes ≥ 232.
- Then tickets 7–10 as §10, with the per-ticket counter tables, the
  cost screen as re-scoped (idle host), and ticket 10's depth-6 floor
  re-based.

The condition for resuming: the maintainer's two decisions. The
condition for passing ticket 6: the interior nodes of the depth-13 read
within 2× the OFF arm's (about 16M) and WAC at 100,000 nodes ≥ 232 with
§2's default; outside either, the leaf returns with the counters and no
further seed is moved by hand. The 100k read is expected near the floor
(§4), so a return there is a real possibility and is the method working:
the next lever is then the paired run of the whole arm, which the floors
exist to protect, not a further seed.

### 6. P8a, P1 and the clauses this amendment changes (question 6)

**P8a** is frozen and is not edited. Calibration, to be copied into
BAS-S41: *"P8a was falsified at ticket 6 on 2026-10-30 (76.7% at depth
13, 82.4% and 96.6% at depth 20). Its band [0.5%, 10%] came from one read
on the legacy extension (1.22%) and ignored the prior the same amendment
cited: Rarog's budget truncated 44,914 extensions at bench 13, about half
of its extensions. Second miss of the same kind as P8's; confidence
(moderate) was over-calibrated and should have been low."* **P8b**, frozen
here for ticket 6 on §1's seed: on the 40 bench positions at depth 13,
`ext_budget_bound / sing_fired` in **[25%, 70%]** (S10 read 52%, S1 42%,
Rarog about 49%); at depth 20 on bench positions 11 and 15 the share is
recorded, not judged; the binding tests are §5's node bound, the 100k
floor and the re-scoped screen. Above 70% at depth 13 says the bars are
mis-scaled again and returns the leaf.

**P1's ablation note** (clerical, the frozen text stands): "the singular
margin at 378 (today's 4/ply)" should read "at 252 (today's 4 cp per
ply; 378 is 6 per ply, the range top)". The ablation order is unchanged.

**Clauses replaced** (old → new; the originals stand as written):
- §7 "Triple margin: base, PV, `!ttCapture`, ttPv, deep … 20, 79, −53, 23,
  −12" → "20, 79, **0**, 23, −12; the quiet term's range −110..0"
  (maintainer decision, §7 below).
- §7 "Small TT ProbCut … **121, 4**" → "off by default (`B3ProbCutTt`
  0); **240**, 4 for the on-form, range 100..400".
- §9 item 8's neighbourhood gains `B3ProbCutTt` (1/0, default 0) and
  `B3NmpEvalGate` (1/0, default 1 if confirmed), both read by paired runs
  after item 5; item 13 (`B3ExtBudget`) is settled by S4 before any game:
  the budget stays, and its paired run is dropped.
- §5.3 "RFP's coordinates stay off B.3.3's surface unless 5.12's rule
  pulls `CoreRfpCorrMult` in" → "`CoreRfpPerPly`, `CoreRfpImproving` and
  `CoreRfpDepthCap` join the surface (amendment 2 §3)".
- §5.12's candidate list gains those three.
- §10 ticket 3 → the gate; ticket 5 → the margin and switch; ticket 6 →
  the seed; the cheap qualification's cost screen → the re-scoped rule;
  ticket 10's `--floor` → re-based on the ON arm (§4).
- §11 P6 "WAC at 100,000 nodes in [232, 262]; the depth-6 count ≥ 175" is
  frozen; calibration note: the depth-6 leg is retired by §4 (the floor is
  re-based), the 100k leg stands.
- Amendment 1 §4's P8a → §6 above; amendment 1 §5's fingerprint rule for
  ticket 6 → unchanged in form, the ON fingerprint re-recorded on the new
  seed.

### 7. Two decisions for the maintainer

Both change something the maintainer confirmed, so neither is taken here:

1. **The quiet triple term seeded at 0** instead of the donor's converted
   −53 (§1). Recommended. The alternative, keeping −53, leaves every
   quiet firing a triple and the tree at 7× the OFF arm's (ticket 6 as
   built), which fails every floor; the fit could not rescue it from there.
2. **The oracle's `eval ≥ beta` gate on the null move by default** (§3).
   Recommended. The alternative, the donor's bare form as default with
   the gate as a categorical at 0, keeps the ON arm closer to the donor
   and reads the gate by games either way; it leaves position 11's tree
   at about 2× what the gate gives until the fit moves RFP and the margin
   together.

The small ProbCut's margin (§2) and the surface additions (§3) are
research's own and are decided here.

**Maintainer decisions, 2026-10-30, after the amendment:**

1. **Taken, with a read added.** `B3SingTripleQuiet` is seeded at 0 and
   is an SPSA coordinate (range −110..0). The maintainer asked whether a
   seed this far from the donor's means the implementation or an
   interaction is wrong. Read S5 answers it: the donor's bars with the
   donor's margin (17) read 10.2M interior nodes, 1.3× the OFF arm, so
   the implementation of the bars behaves as the donor's when given the
   donor's pair; the 24× came from pairing the donor's bars with §7's
   margin of 2 cp per ply (B.0 §7.2's rule-2 seed), eight times the
   donor's — the researcher's pairing error, of the same kind as §3's.
   The donor's whole pair (margin 17 with the quiet term at −53) joins
   B.3.3's curvature sweep as a registered point beside the margin's
   extra point, so the fit starts from both pairs; it is not the seed
   because it reads about 208 at 100,000 nodes (about 226 with the small
   ProbCut off) against the registered floor of 232, and a floor is not
   waived by research after a read.
2. **Not taken; the donor's form stays the default.** `B3NmpEvalGate`
   defaults to 0 (the donor's bare null move) and is a categorical read
   by one 2,000-game paired run in B.3.2 before the sweep, as every
   categorical is. The cost accepted: bench position 11's tree at about
   twice the gated form's until the fit moves the RFP coordinates and the
   margin together, which the re-scoped screen tolerates.

B.3.1 resumes `READY_FOR_IMPLEMENTATION` on these two decisions.

## Return 3 — B.3.1 ticket 6 on amendment 2's seed: one position carries the node gate, and the arm throws a won promotion (2026-10-30)

**What resumed.** Amendment 2's three changes landed in its order, each
with the per-ticket checks:

- Ticket 3, `B3NmpEvalGate` (default 0): `e6c120a`. ON is unchanged at
  18,049,092. The switch is live: bench position 11 at depth 11 reads
  2,033,267 nodes, and 1,670,826 with the gate on.
- Ticket 5, `B3ProbCutTt` (default 0; its on-form margin 240 in 100..400):
  `80d8261` and surface `51563d9`. ON reads 167,919,223, the hybrid with
  the old quiet triple term still in place.
- Ticket 6 re-done, `B3SingTripleQuiet` 0 in −110..0: `1e77ec9` and
  surface `9264fc8`. OFF reads 16,615,757 at every commit.

**Premise returned.** Amendment 2 §5: ticket 6 passes when "the interior
nodes of the depth-13 read [are] within 2× the OFF arm's (about 16M) and
WAC at 100,000 nodes ≥ 232 with §2's default; outside either, the leaf
returns with the counters and no further seed is moved by hand". With it,
§3 and the maintainer's decision 2: bench position 11's tree "at about
twice the gated form's ... which the re-scoped screen tolerates".

**The three gates at `1e77ec9`** (amendment 1 §7's read; the OFF arm's own
read beside it; `tools/results/b31-20261030/t6b/counters_on.json` and
`counters_off.json`):

| Gate | Read | Rule | Verdict |
|---|---:|---|---|
| P8b: `ext_budget_bound / sing_fired`, 40 bench positions, depth 13 | 30,255 / 56,570 = **53.5%** | [25%, 70%] | pass |
| WAC at 100,000 nodes | **234/300** | ≥ 232 | pass |
| Interior nodes of the depth-13 read, ON / OFF | 25,792,530 / 8,008,320 = **3.22×** | ≤ 2× | **fail** |

Recorded, not judged:

- `bench 13` is **427,471,707**.
- WAC at depth 6 is 134.
- Triples are 92.7% of firings and doubles 99.4%.
- At depth 20 on bench position 15, the share is 58.3% (2,883 of 4,944).

**One position carries the node gate.** Bench position 11 (the won
middlegame of Return 2's cost-screen finding) is 22,660,108 interior nodes
of the ON arm's 25,792,530 (87.9%), against the OFF arm's 277,125: 81.8×.
The other 39 positions read 3,132,422 against 7,731,195, which is
**0.41×**, and the geometric mean of the per-position ratios over all 40 is
0.57. In `bench 13`, where the table carries over from the earlier
positions, position 11 takes 413,713,714 of the 427M. On this position the
switches move the tree chaotically. Total nodes at depth 13, fresh, Hash 64,
Tune build of `1e77ec9`:

| Setting | Nodes |
|---|---:|
| the defaults | 57.2M |
| `B3NmpEvalGate=1` | 161.7M |
| `B3ProbCutTt=1` | 6.2M |
| both | 12.4M |

The maintainer's "about twice" was read on the ticket-3 arm (amendment 2
§3, read N); on the ticket-6 arm the same position is 82× and dominates a
gate written as a sum over 40 positions.

**A won promotion thrown.** The ON arm's `test_search` fails one section
that passed at ticket 5b: "promotion: a queen where a rook only
transposes" (`3r4/P7/2K1p3/R7/5p2/6kP/8/8 w - - 2 66`, 100,000 nodes, Hash
64) plays **h3h4** where the test requires **a7a8q**. This is a
theory-truth miss, not a style choice. The OFF arm at depth 22 scores a7a8q
+198.71 (the known-win class) and h3h4 0. The ON arm's 100,000-node search
prefers a7a8q through depth 9, reads the queen line as a draw from depth 10
and plays the draw at depth 16.

Every single switch flip recovers a7a8q on the Tune build of `1e77ec9`, at
depth 10–11 where the defaults reach 16:

| Switch flipped | Score |
|---|---:|
| `B3SingTripleQuiet=-53` | +837 |
| `B3ProbCutTt=1` | 0, still a7a8q |
| `B3ExtBudget=0` | +118 |
| `B3SingNegative=0` | +319 |
| `B3MultiCut=1` | +510 |
| `B3NmpEvalGate=1` | +864 |
| `B3CheckExt=1` | +317 |

So the defaults' greater selectivity in this ending, not one mechanism,
loses the win. DESIGN §4 makes theory truth an absolute veto. The OFF
arm's `test_search` passes at every commit, and the ON arm's failure is in
the committed tree (`1e77ec9`) so the reviewer can reproduce it.

**The counter table against ticket 5b** (`t6b/compare.md`): the tree is
0.29× ticket 5b's hybrid. Cluster 1 rows over 20%: `razor_cuts`,
`fut_prunes`, `hindsight_down` and `hist_reward_updates`, all following
the singular tree's shrinkage. Against the OFF arm on the same read:

| Rule | ON | OFF |
|---|---:|---:|
| null-move tries (share of interior nodes) | 2.6% | 9.1% |
| ProbCut cuts | 4.6% | 1.0% |
| RFP cuts | 9.3% | 23.9% |
| LMR applied | 36.2% | 26.2% |
| quiescence nodes (per interior node) | 1.52 | 0.67 |

**Options.**

1. *Re-state the node gate per position*: the geometric mean of ON/OFF
   over the 40 positions ≤ 2 (it reads 0.57), with position 11 recorded,
   as the screen was re-scoped. That honours the maintainer's acceptance of
   position 11's shape, but it is a gate changed after a read, so it is the
   maintainer's to make or refuse.
2. *Take the small ProbCut's on-form as the default* (`B3ProbCutTt=1`): on
   position 11 it reads 6.2M against 57.2M, and amendment 2's read P had
   the whole arm at 6.58M interior nodes with it on. It is a default the
   maintainer set at 0 on WAC evidence (read P: 229 at 100k with it on),
   and it would need its own read of the floor and of the promotion test.
3. *Hand the throw to research before anything else.* The promotion miss
   is a theory-truth veto independent of the node gate, and no single
   switch carries it. It says the default arm's selectivity in won pawn
   endings needs a reason before ticket 7 lands on top of it. B.12's
   KBP-K repair and BAS-R07 are the nearest precedents.

Recommendation: option 3 first, because a veto outranks a node gate; then
the maintainer chooses between 1 and 2 with that research in hand. No seed
or default has been moved.

**State at the return.** Tickets 0–6 as amended are committed (`1e77ec9`,
`9264fc8`). The OFF arm is 16,615,757 with release CTest 25/25 at every
commit. The ON arm's release CTest passes through ticket 5b; at `1e77ec9`
it fails one `test_search` section, the promotion above, which was not
relaxed. Tickets 7–10 are not started. The amendment-2 surface additions
(the three RFP coordinates and the donor's sweep pair) belong to the
regeneration at ticket 10 and are not made. The `test_b3_proof` forced-cap
assertion was corrected at `80d8261`: it compared two different trees'
truncation counts, and it now asserts the capped run truncates. No
idle-host read was run.

## Research amendment 3 — the thrown promotion, bench position 11's carrier, and whether the coherent arm continues (2026-10-30)

Answers Return 3. Class `R2`; no engine code is changed in the committed
tree and no seed or default is moved here; every read is a deterministic
node-count, counter or WAC read on the five trees of `45d4e6d` (`build/b3-on`
427,471,707 and `build/b3-off` 16,615,757, both re-read here; the Diag tree
of each arm; the Tune ON tree) or on a temporary build whose edit was
reverted (`git status` clean after each) with its patch kept. No timed
read was run; the re-scoped cost screen still needs the idle host (§5).
Recipes, option lists, logs and JSON: `tools/results/b31-20261030/amend3/`
(the read scripts under `scripts/`, `a3_*.py`, one per read; the patches
`linetrace.patch`, `linetrace2.patch`, `v1_pv_quiet_prune_off.patch`,
`v2_negative_ttbeta_only.patch`, `v3_both.patch`,
`v4_sing_negative_1.patch`, `v5_sing_margin_252.patch`,
`v6_margin252_seekmate12.patch`). Each read's verdict rule was written in
its script's header before it ran and is quoted where the read is judged.
The 6-man tablebases (`D:/chess/tablebases/syzygy3456`, python-chess
1.11.2) give the theory truth; the engine's searches never saw them. A
caution for the recipes: under `go nodes N` the last `info` line carries
the last completed iteration's node count, not the budget spent.

### 0. What this run says, plainly

The ON arm throws a won promotion because, in a won rook-and-pawn ending
that the static evaluation reads as level, three "only moves" in a row are
what the arm's selectivity removes: the king move that wins is pruned by
quiet futility on a history-driven negative reduced depth (at non-PV
nodes, and on this arm also at PV nodes off the previous PV), the one
forced reply gets the donor's three-ply negative extension because at
shallow depth the exclusion search finds a second move that looks as good,
and the last king move is futility-pruned at the horizon. The OFF arm
keeps the win only because it finds the first king move at depth 4,
stores it, and then extends every only-move singularly. No single switch
names the mechanism: at fixed depth every flip that recovers the move does
so with a two- to ten-fold tree, bar IIR off at 1.3× (§1.4); at the test's
100,000 nodes three settings recover it with the WAC floor held, and only
one of them, the singular margin at Basilisk's own 4 cp per ply, does so
without failing a read of its own (§1.5). Bench position 11 is a different
question with the same block at its centre: in a decisively won middlegame
the donor's singular block, negative extension included, turns cheap cut
nodes into wide ones in one iteration (1.6M nodes at depth 12, 57M at 13),
and the tree's response to the block's constants is chaotic (the negative
extension at 0: 0.7M, at 1: 236M, at 3: 57M), so a gate summed over 40
positions is decided by this one and nothing the ticket may move can
satisfy it, except the donor's own guard for won positions, `seekMate`,
fired from root depth 12 instead of 16 (§2). With the margin at 252 and
the guard at 12 every registered gate passes as written: the fixture at
six budgets, WAC 237, the suite at 0.26× the OFF arm's, position 11 at
0.97×, the conversion floors at five budgets (118), both test binaries;
the one miss is P8b's lower edge (§4). The recommendation is those two
seeds, the sweep carrying the donor's values beside them, and B.3.1
continuing to tickets 7–10 (§3, §5). Three of the four returns of this
leaf traced to a donor constant paired with a Basilisk partner; this one
traces to the donor's selectivity paired with an evaluation that does not
see the ending, which the fit cannot repair and the floors exist to catch.

### 1. The thrown win (question 1)

**1.1 Theory truth (read T0, `t0_tablebase.txt`).** The root is 7 men and
unprobeable; every White root move was walked three plies to the first
6-man position. After a8=Q Rxa8 Rxa8 the position is a tablebase win for
White against every Black reply (WDL +2; DTZ 3 after ...f3, 9 after ...Kxh3),
and the win is narrow: after ...f3 only Rf8, Rh8 and Ra3 keep it (Ra1, the
ON arm's own line, draws); after ...Kxh3 only Rf8; after ...Kxh3 Rf8 Kg3
only Kc5 (DTZ 6: Kc5 f3 Kd4 f2 Ke3 f1=Q Rxf1). After h4 Kxh4 a8=Q Rxa8 Rxa8
the 5-man position is a draw (WDL 0). The rook promotion a8=R also wins,
so the fixture is a theory-truth test of the queen line and a move-quality
test against the rook. The test's premise holds and the veto stands.

**1.2 Where the draw score lives (reads T1–T6).** Fresh searches of the
line's positions on the ON arm are correct: after Rxa8 it reads +712 at
depth 10 and the known-win band at 13; after ...f3 +884 at depth 8 (the
OFF arm reads 0 from depth 7 to 11 after Rxa8 and +857 after ...f3). With
`searchmoves a7a8q` the ON arm reads +788 to +1,406 at depths 10–12 while
the OFF arm reads 0 at 11 and 12 on a rook-check shuffle (`t34_d11.log`):
the fixture is knife-edge on both arms, and the OFF arm passes it by the
shape of its full root tree. The draw score is a property of the root
tree's table state: after `go depth 11` at the root, without clearing the
table, the position after ...Kxh3 reads 0 with Kd6 as best and Rf8 at +18
(the only winning move; OFF +599), and under Rf8 the reply Kg3 reads +33 on
a Rg8+ repetition (OFF +387 on Kc5). Read as a root in that state the Kg3
node finds Kc5 at +254 (`t5_d11.log`, `t6_d11.log`): the winning move is
there, the tree does not reach it.

**1.3 The mechanism (reads T7, T11, T11b: a temporary Diag-only edit that
prints every interior node whose path is a prefix of the winning line,
its depth at entry and at the move loop, node type, `follow_pv`, the TT
entry and window, and every decision-trace event at that node, both arms,
`go depth 11`; `linetrace2.patch`, `t11b_*_d11.log`, condensed in
`t11b_on_it10_line.txt` and `t11b_off_it10_line.txt`).** Iteration 10 is
the first that reads the draw. Its aspiration window fails low six times
(alpha 356, 337, 307, 265, 202, 108, then 0), and along the line a8=Q
Rxa8 Rxa8 Kxh3 Rf8 Kg3 the ON tree does this at the only-move node (ply 6,
White to move; the winning move is Kc5, the TT move is the drawing check
Rg8+ from the first pass on):

- Passes 1–3: Kc5 is pruned by **quiet futility** (`futility c6c5`,
  continuation history −14,377 to −17,977, reduced depth −3 to −5, margin
  −155 to −369) at depths 4, 9 and 6. Pass 1 is a PV node off the previous
  PV (`pv=1 fpv=0`), which on this arm prunes quiets (amendment 1 §6's
  consumer, ticket 2); passes 2–3 are non-PV nodes, where the OFF arm's
  rule is the same.
- Pass 4 (alpha 265, depth 13 at the node): Kc5 is searched three plies
  shallower (LMR, history −9,302), the f3 node has 7, Kd4 is reduced two
  more (history −85 at move 2), f2 has 3, and Ke3 is **futility-pruned**
  at depth 3 (static eval +78 against alpha 265, margin 59).
- Passes 5–6 (PV, alpha 202 and 108): Kc5 is searched unreduced; at the
  Kc5 node (a cut node) the exclusion search at depth 4–5 finds ...e5
  holding the singular bound (`beta_cutoff e6e5` at −206/−202 against a
  bound of −283/−223), so Black's only reply ...f3 takes the **negative
  extension −3** (`sing_negative f4f3 red=-3`): the f3 node has depth 5
  from 9 (pass 5) and 7 from 11 (pass 6); Kd4 is LMR'd three plies (history
  −9,050 / −10,707) to depth 1 and 3; the line ends in a horizon cutoff
  (pass 5: `beta_cutoff f3f2 result -21`) or Ke3's futility (pass 6: static
  −64 against alpha 108, margin 166).
- Final pass (alpha 0): the Kg3 node is non-PV at depth 10 and its
  corrected static eval has fallen from +327 to **−13** across the passes
  (the correction history trained on the fail-lows), so Kc5 is
  futility-pruned (history −20,719, reduced depth −2, margin −48). The
  a8=Q subtree returns ≤ 0 and the root prefers h4's exact 0.

The OFF arm's iteration 10 on the same line: the Kg3 node's TT move is
Kc5 from the first pass (found at depth 4, static eval +343), PV nodes
never prune quiets, the negative extension is −1 and only on
`tt_score ≥ beta`, and the three only-moves are **extended singularly**:
Kc5 +1 at depth 6 (value 51 against a bound of 288), Kd4 +1 at depth 5,
Ke3 +2 at depth 7 (175 against 222). The same singular machinery that
inflates position 11 (§2) is what holds this win on the OFF arm.

Two rules that are not Cluster 2's carry the pruning: quiet futility on a
history-driven reduced depth (the donor has it in the same form,
`search.cpp:1211-1218`: `lmrDepth += history / …`, then `lmrDepth < 12 &&
futilityValue <= alpha`) and LMR by history. What Cluster 2 adds is the
negative extension's cut-node clause and the PV-node quiet pruning, and
what the arm lacks against the OFF arm is the shallow singular firing
(entry at depth 5 and a margin of 4 cp per ply there, against 6 and 2 cp
here). The base cause is the evaluation: along a tablebase-won line the
corrected static eval reads −13 (Kg3 node), −80/+78 (Kd4, f2) and −64 (f2,
pass 6), so every pruning rule that compares the static eval with alpha
is working as designed on a wrong number, and only depth corrects it.

**1.4 Not one mechanism: the switches at fixed depth (read T8, Tune ON
build, `go depth 12`, Hash 64; `t8_switches_depth12.json`; rule frozen
before the run: a setting repairs the position at depth D when its
completed-iteration score at D is ≥ +150 with best move a7a8q; a repair at
depth 11 within 1.5× the defaults' depth-11 nodes names a mechanism, a
repair only with a larger tree is a tree-shape effect).**

| Setting | Depth-11 nodes (defaults 42,440) | Depth 10 / 11 / 12 (White view, best) |
|---|---:|---|
| defaults | 1.00× | +28 h4 / 0 h4 / 0 h4 |
| `B3Iir=0` | **1.29×** | +103 / **+275** / +1,108, a8=Q |
| `B3CheckExt=1` | 1.98× | +81 / +317 / +1,246 |
| `B3NmpEvalGate=1` | 2.30× | +144 / +864 / +1,068 |
| `B3SingNegative=0` | 3.04× | +319 / +1,118 / +1,113 |
| `B3MultiCut=1` | 3.00× | +510 / +1,005 / +1,105 |
| `B3SingMargin=252` | 3.05× | +956 / +1,095 / +1,153 |
| `B3SingMargin=17` | 4.88× | +1,166 / +1,205 / +1,532 |
| `B3SingTripleQuiet=-53` | 4.57× | +837 / +1,165 / +1,150 |
| `B3NmpCutOnly=0` | 5.33× | +816 / +1,128 / +1,152 |
| `B3ProbCutMinDepth=5` | 4.54× | +601 / +1,018 / +1,402 |
| `B3IirStale=1` | 1.84× | +265 / +703 / +1,187 |
| `B3SingularQuietLmr=1` | 2.96× | +553 / +869 / +1,132 |
| `B3ExtBudget=0` | 0.58× | +128 / +118 / +1,126 (4.3× at 12) |
| `B3CheckPrune=0` | 0.71× | 0 / 0 / 0, a8=Q by order only |
| `B3ProbCutPv=0` | 0.88× | +4 h4 / 0 / 0 |
| `B3ProbCutTt=1` | 1.17× | 0 / 0 / +745 (2.9× at 12) |
| legacy check policy (`Ext=1`, `Prune=0`) | 7.56× | +582 / +1,066 / +1,683 (432× at 12) |
| `B3NmpVerify=0` | identical | identical |

By the rule, IIR is the one mechanism named at depth 11: the donor's IIR
at PV nodes off the previous PV loses a ply at the line's nodes (the PV
alternates between a8=Q and h4 across iterations, so the line is off the
previous PV on every other iteration). Every other recovery is a wider
tree. Return 3's "every single switch flip recovers a7a8q" at 100,000 nodes
is therefore correct and uninformative: those arms reach depth 10–11 in
the budget where the defaults reach 16, and a wider tree at 10 beats a
narrow one at 16 in this ending.

**1.5 The repair (reads T13–T19; rules frozen before each: a setting
repairs the fixture when its 100,000-node best move is a7a8q with a score
≥ +150; it is admissible as a contract change only with WAC at 100,000
nodes ≥ 232 and, for a seed, with ticket 6's own reads: P8b in [25%, 70%],
the depth-13 interior nodes within 2× the OFF arm's 8,008,320, the
floors).** Three temporary builds isolated the two Cluster 2 consumers
named in 1.3 (`t13.log`):

| Variant (release ON, reverted) | Fixture at 100k | WAC 100k | `bench 10` |
|---|---|---:|---:|
| defaults (`build/b3-on`) | h4, 0, depth 16 | 234 | 1,654,548 |
| V1 PV nodes exempt from quiet pruning (the OFF arm's rule; `follow_pv` kept for IIR) | a8=Q, 0, depth 13 (by order) | **231** | 1,068,836 |
| V2 negative extension only on `tt_score ≥ beta` (the OFF arm's condition, −3) | a8=Q, 0, depth 12 (by order) | 232 | 2,683,473 |
| V3 both | **a8=R**, +491, depth 9 | 235 | 1,240,650 |

Neither consumer alone repairs it (the futility pruning at non-PV nodes
and the LMR on the king walk remain), both together make the tree three
to six times wider at fixed depth (+1,043 at 10) and at 100,000 nodes stop
at depth 9 on the rook. The Tune build's settings at the test's budget,
with bench position 11 beside them (`t14.log`, `t14b.log`, `t17.log`;
position 11 at depth 13, defaults 57.2M):

| Setting | Fixture at 100k (White view, best, depth) | WAC 100k | Position 11 |
|---|---|---:|---:|
| `B3SingMargin=252` (4 cp per ply, Basilisk's fitted value) | **+956 a8=Q, 10** | **237** | 52.8M |
| `B3SingMargin=252` + `B3SeekMateDepth=12` | **+956 a8=Q, 10** | **237** | **0.51M** |
| `B3SingNegative=1` (the OFF arm's magnitude at the donor's population) | **+578 a8=Q, 10** | **239** | 235.6M |
| `B3NmpEvalGate=1` | **+864 a8=Q, 11** | **238** | 161.7M |
| `B3SingMargin=252` + `B3SingNegative=0` | +1,097 a8=Q, 10 | 233 | 280.7M |
| `B3SingMargin=252` + `B3SingNegative=1` | a8=Q, 0, 12 (by order) | 235 | 60.0M |
| `B3SingNegative=0` | +319 a8=Q, 10 | 230 | 0.72M |
| `B3Iir=0` | +275 a8=Q, 11 | 230 | not read |
| `B3SeekMateDepth=12` | h4, 0, 16 | 233 | 0.70M |
| `B3SeekMateDepth=8` | h4, 0, 16 | 228 | 0.30M |
| `B3SingNegative=0` + `B3SeekMateDepth=8` | +319 a8=Q, 10 | 231 | not read |
| `B3ProbCutTt=1` | a8=Q, 0, 11 (by order) | 229 | 6.2M |
| `B3ProbCutTt=1` + `B3NmpEvalGate=1` | +314 a8=Q, 10 | 230 | 12.4M (Return 3) |
| `B3SingMinDepth=5` | a8=Q, 0, 15 (by order) | 241 | not read |

Four settings repair the fixture with the floor held. Their price on the
whole suite (read T17: Tune build, `go depth 13` per position, total nodes
against the OFF release build read the same way, 13,126,021; and the Diag
seed builds V4, V5 and V6 for the counters against the OFF arm's ticket-6
read, `t15_v4.json`, `t18_v5.json`, `t19_v6.json`):

| Setting | 40-position total (OFF 1.00) | Without position 11 | Geometric mean of per-position ratios | Interior nodes, Diag (OFF 8,008,320) | P8b | WAC 6 |
|---|---:|---:|---:|---:|---:|---:|
| defaults (`1e77ec9`) | 4.79× | 0.45× | 0.65 | 3.22× | 53.5% | 134 |
| `B3SingMargin=252` (V5) | 4.39× | 0.38× | 0.47 | 2.99× | 50.6% | 136 |
| `B3SingMargin=252` + `B3SeekMateDepth=12` (V6) | **0.29×** | 0.26× | 0.35 | **0.26×** | **14.9%** | 136 |
| `B3SingNegative=1` (V4) | 18.56× | 0.63× | 0.67 | 12.55× | 72.7% | 132 |
| `B3NmpEvalGate=1` | 12.90× | 0.60× | 0.68 | not read | not read | — |

`B3SingNegative=1` is the repair closest to the mechanism (it is the
clause 1.3 names, at the OFF arm's magnitude), it reads WAC 239 and the
fixture at +208 to +1,321 from 33,000 to 1,000,000 nodes (read T16,
`t16_budgets.log`; the defaults read 0 from 60,000 up, the OFF arm a8=Q at
every budget), and `test_search` passes 554/554 on it; but it fails ticket
6's own reads on the suite (P8b 72.7%, interior nodes 12.5×: position 11
at 236M with 97% of firings triples) and its `bench 13` is 184,706,542.
`B3NmpEvalGate=1` is the maintainer's decision 2 of amendment 2, not
reopened by this evidence (position 11 reads 162M with it). **The margin
at 252** repairs the fixture with the largest margin (+956), holds WAC at
237 (+3) and 136 at depth 6 (+2), P8b at 50.6%, and shrinks the other 39
positions to 0.38× the OFF arm's; it fails only the summed node gate,
through position 11 (21.3M interior nodes, 77× the OFF arm's 277,125),
which §2 shows no margin value moves. It is Basilisk's own fitted value
(P1's ablation note, corrected in amendment 2 §6: "252, today's 4 cp per
ply"), already a registered point of B.3.3's sweep, and it acts through
the mechanism the OFF arm's trace shows: a margin of 4 cp per ply fires
the singular search on the only-moves (the exclusion search must hold a
bound 4·depth below the TT score) and refuses it on the ambiguous ones,
so the negative extension reaches fewer of them. Amendment 2 §1 kept 126
on reads S5/S6 made under the old triple term (the donor's 17 explodes
with graded triples; 252 was not read); this read does not contradict
them. With the donor's guard for won positions fired from root depth 12
beside it (V6; §2), every registered gate passes as written: the fixture
at all six budgets (+282 at 33,000 and 60,000, +956 at 100,000, +1,203,
+1,273, +1,556), WAC 237 and 136, `test_search` 554/554, the endgame suite
121/121, the suite at 0.26× with position 11 at 267,873 interior nodes
(0.97× the OFF arm's) and a geometric mean of 0.31, the conversion floors
(below), and `bench 13` at **6,263,268** (0.38× the OFF arm's); P8b reads
14.9%, below its band, scored in §4.

**The guard with precedent, and what has none.** The donor carries no
guard for this position class: its quiet futility and history reductions
are the same rules, and its evaluation sees the ending. The oracle's
known-win bounds (packet §2.3) do not reach KRP–KPP, which is no known-win
class; a static bound for it would be new evaluation knowledge, not a
search guard, and belongs to the evaluation programme (§5). The donor's
`seekMate` (root depth ≥ 16, |score| ≥ 2000, its `search.cpp:727`, `:996`,
`:1248`) is the donor's own guard against singular machinery in won
positions and is position 11's lever, not this fixture's: at root depth 8
or 12 it does not recover the fixture (the defaults' root score never
reaches 564 there, so the guard never fires). The repair a contract can carry is
therefore the margin, with the fixture and the floors as its test, and
the guard beside it for the position class of §2; the negative
extension's magnitude (1 against 3) and the PV quiet pruning are
questions for games, registered in §4 as categoricals so a negative
paired run can ablate them.

**The test, pre-registered.** The fixture is the existing `test_search`
section "promotion: a queen where a rook only transposes" at 100,000 nodes
and Hash 64, unchanged, binding on both arms at every ticket as release
CTest already makes it (Return 3 reported it rather than relaxing it; this
amendment names it a theory-truth fixture of the ON arm so no later ticket
reads it as style). Beside it: the budget series of read T16 (33,000,
60,000, 200,000, 400,000 and 1,000,000 nodes), recorded at ticket 10 for
the ON arm (a8=Q with a positive score at each is the expectation; a
budget where it reads 0 is reported, not a floor: DESIGN §4's budget
rule); the endgame suite (`test_endgames`, the KBP-K wrong-bishop draws
included: 121/121 on the ON arm, `endgames_on.log`, and on V6); the
conversion floors at five budgets; and the other promotion fixtures.

**The other won-ending fixtures do not show the failure (reads T9, T10,
T19).** Conversion floors (`test_endgames --conversion --nodes N`,
release builds; `conv_on_*.log`, `conv_v6_*.log`; the OFF arm beside them
reproduces §2.2's repaired-head table exactly, 115, `conv_off_*.log`):

| Family | ON arm `45d4e6d`: 30k / 45k / 60k / 90k / 120k (sum) | V6: 30k / 45k / 60k / 90k / 120k (sum) |
|---|---|---|
| KQ-K | 24 (14.6) / 24 (12.7) / 24 (11.2) / 24 (11.1) / 24 (11.0) (120) | 24 (13.5) / 24 (14.3) / 24 (12.6) / 24 (11.2) / 24 (11.0) (120) |
| KR-K | 24 (**32.7**) / 24 (27.4) / 24 (22.8) / 24 (21.8) / 24 (22.2) (120) | 24 (28.2) / 24 (24.9) / 24 (23.3) / 24 (21.8) / 24 (20.5) (120) |
| KBB-K | 24 (24.5) / 24 (25.0) / 24 (24.9) / 24 (24.4) / 24 (24.6) (120) | 24 (24.4) / 24 (23.6) / 24 (23.7) / 24 (24.7) / 24 (24.4) (120) |
| KBN-K | 24 (52.0) / 24 (53.6) / 23 (50.1) / 24 (52.2) / 24 (54.1) (**119**) | 24 (57.7) / 24 (57.8) / 22 (53.7) / 24 (52.5) / 24 (51.2) (**118**) |

Both arms hold every floor (KBN-K ≥ 112: 119 and 118 against the head's
115). The one mark on the ticket-6 arm is KR-K's mean at 30,000 nodes,
32.7 plies against the 29.5-ply ceiling the floors judge at 60,000 only
(28.9 on the OFF arm, 28.2 on V6): slower, not thrown, reported for
B.3.2. BAS-R07's rec1 (`7r/5R2/8/2k1PB2/8/4K3/8/8 w`, a tablebase win,
DTZ 1) is won on both arms at 60,000 to 600,000 nodes (ON Rd7 +1,002, a
tablebase win with DTZ 2 after it; OFF e6 +900 to +1,388, DTZ 6), the ON
arm needing more than 600,000 nodes for depth 11 where the OFF arm needs
398,000 (slower in a won ending, consistent with KR-K above). The
rook-first promotion (g8=Q on both arms at every budget), Saavedra (c8=R,
mate in 7 on both) and the stalemate-avoiding rook (f8=R, both) pass
(`t10.log`). The failure is this one fixture.

### 2. Bench position 11 and the node gate (question 2)

**The counters at the ticket-6 arm (read T12, Diag builds, `run_suite.py`
on position 11 alone, depth 13, Hash 64, `pos11_counters_*.json`).** The
OFF arm searches 277,125 interior nodes, the ON arm 22,660,108 (81.8×).
Per interior node, the ON arm tries the null move at 1.75% (OFF 11.9%),
cuts with ProbCut at 4.5% (1.0%), is in check at 24.5% (19.7% with the
extension on OFF), and runs 1.6 quiescence nodes (0.58). The singular
rows: 129,831 tries, 36,961 firings of which **36,909 triples** (99.9%),
25,946 truncated by the budget (70.2%), 31,394 multi-cuts, **55,019
negative extensions of which 40,971 at cut nodes with `tt_score < beta`**
(OFF: 1,880 firings, 829 multi-cuts, 683 negatives of −1). By depth
(`t12_pos11.json`), the defaults read 25k, 187k, 277k, 594k, 1.63M and
**57.2M** nodes at depths 8–13: one iteration multiplies the tree by 35.
The position is won by more than twenty pawns from depth 8 (the queen
falls to Rxb7), and in a position where every move wins the exclusion
search finds a second winning move at almost every cut node: the TT move
is "not singular", the multi-cut rule admits it only when the exclusion
value clears beta (which a fail-soft bound 2·depth below the TT score
rarely does here), so the negative extension fires, the TT move is
searched three plies shallower, fails to confirm the cutoff it would have
given at full depth, and the node searches its other moves at full depth
with LMR; the triples that do fire chain into the budget. The donor's own
answer to this position class is `seekMate`: at root depth 16 and |score|
≥ 2000 it switches the singular block off and caps reverse futility; it
was ported with those constants and at depth 13 is silent.

**The switches on this position (Tune ON build, `go depth 13`, fresh,
Hash 64, `t12_pos11.json`, `t14b.log`, `t17.log`; rule frozen before the
run: a switch names a mechanism here when it moves the count by more than
3× either way):**

| Setting | Nodes at depth 13 | Against the defaults |
|---|---:|---:|
| defaults | 57,196,052 | 1.00 |
| `B3SeekMateDepth=8` (the donor's guard from root depth 8) | **299,611** | 0.005 |
| `B3SingMargin=252` + `B3SeekMateDepth=12` (V6) | **510,696** | 0.009 |
| `B3SeekMateDepth=12` | 700,200 | 0.012 |
| `B3SingNegative=0` | **722,172** | 0.013 |
| `B3ProbCutTt=1` | 6,193,688 | 0.11 |
| `B3SingMargin=252` | 52,791,981 | 0.92 |
| `B3SingMargin=252` + `B3SingNegative=1` | 60,036,824 | 1.05 |
| `B3MultiCut=1` | 72,569,826 | 1.27 |
| V1 (PV quiet pruning off, release) | 98,468,141 | 1.72 |
| `B3NmpEvalGate=1` | 161,680,917 | 2.83 |
| `B3SingTripleQuiet=-53` | 203,408,543 | 3.56 |
| V2 (negative only on `tt_score ≥ beta`, release) | 234,434,339 | 4.10 |
| `B3SingNegative=1` | 235,603,170 | 4.12 |
| `B3SingMargin=252` + `B3SingNegative=0` | 280,652,807 | 4.91 |
| `B3CheckExt=1` | unfinished: 21.8M at depth 12 after 40 minutes | — |

The singular block is the carrier: the donor's guard for won positions
collapses the tree to 0.69× the OFF arm's total nodes at root depth 8 and
to 1.6× at 12 (V6 0.97× on interior nodes: the block is on through depth
11, where the tree is 481k, and off from 12, where it would have
multiplied), and the negative extension off collapses it to 1.7×. The
block's response to
the negative extension's magnitude is non-monotone (0: 0.7M, 1: 236M, 3:
57M; with the margin at 252: 281M, 60M, 53M) and to its population too
(V2, the cut-node clause removed at −3: 234M): DESIGN §5's chaos, as in
amendment 2's S1/S2. So this position cannot choose the negative
extension's seed, and a gate it dominates cannot pass or fail a seed on
its merits; what it can do is show the guard the donor uses for it.
Question 1's answer touches it twice: the same clause (the negative
extension at cut nodes) is at the centre of both, and the margin at 252
leaves this position at 0.92 of the defaults, so the fixture's repair
neither helps nor harms it.

**The options, priced (each with the fixture and WAC at 100,000 nodes):**

1. *The donor's guard fired earlier* (Return 3's "another remedy"):
   `B3SeekMateDepth` 12 beside the margin at 252 (V6). Every registered
   gate passes as written (§1.5): the suite at 0.26× the OFF arm's
   interior nodes, position 11 at 0.97×, the fixture at six budgets, WAC
   237, the floors at 118, both test binaries, `bench 13` 6,263,268. It
   is the donor's own mechanism for this position class at a constant
   four plies shallower than the donor's, which is a ply seed (B.0 §7.2
   rule 1) moved by a read; the sweep keeps 16 (and 8) as points so the
   fit decides it. Its cost is P8b's lower edge (14.9%: the budget binds
   less because the won positions, where triples chain, no longer run the
   block above depth 11) and a question for games: whether switching
   singular extensions off in won positions from depth 12 costs
   conversion at longer controls (the floors at five budgets say no; KR-K
   at 30,000 nodes improves from 32.7 to 28.2 plies; B.3.4's `10+0.1`
   direction read is where it would show). Recommended.
2. *The gate re-stated per position* (Return 3's option 1) with the
   margin at 252 and the donor's 16 kept: the geometric mean of the
   per-position interior-node ratios ≤ 2, position 11 recorded. V5 reads
   0.41 (0.47 on total nodes), the ticket-6 arm 0.57. It is a gate
   changed after a read and the maintainer's to take; the case for it is
   that the sum was written to catch an explosion of the arm and catches
   one won middlegame's response to the singular block below the depth
   the donor's guard watches. Its cost: position 11 stays at 77× (21.3M
   interior nodes at depth 13 and growing 35× at that iteration), P11
   below predicts the re-scoped screen fails on it at ticket 10, and the
   paired run reads whatever a 77× tree in won middlegames costs. The
   alternative to 1.
3. *The small ProbCut on by default* (Return 3's option 2): position 11
   at 6.2M, but WAC 229 and the fixture at 0 (a8=Q by order only), both
   floors failed; with the eval gate as well 12.4M, WAC 230, fixture +314.
   Not admissible by the floors.
4. *The negative extension off* (`B3SingNegative=0`): position 11 at
   0.72M and the fixture at +319, but WAC 230 (233 with the margin at
   252, where position 11 is 281M). Not admissible as a default; the
   magnitude goes to games and the sweep (§4).

Recommendation: option 1, with option 2 as the alternative if the
maintainer keeps the donor's 16. The alternative that keeps both the sum
and the 16 is a return of B.3 to research as a whole, because no
admissible setting then brings position 11 under 50M (§3).

### 3. Whether the coherent ON arm is still the right next step (question 3)

Three returns in one leaf, four counting this one, each traced to a
pairing: the donor's bars with a margin eight times the donor's (Return 2);
the donor's small-ProbCut margin with Basilisk's stand-pat store depth and
the donor's null-move margin with Cluster 1's reverse futility (amendment
2); now the donor's negative extension and PV pruning with an evaluation
that reads a won ending as level, and the donor's singular block with a
won middlegame below the depth its own guard watches. The first three were
seeds and forms; this one is a property of the evaluation that no seed
removes and that the floors exist to catch, with the donor's own guard as
the lever. Priced:

- **Continue B.3.1 to tickets 7–10 on this amendment** (recommended).
  Maintainer time: the two decisions of §5 and one Opus session for
  ticket 6b and tickets 7–10 (ticket 6b is two seed lines, two switch
  lines and a surface regeneration), then the review session; compute:
  the per-ticket counter reads (minutes each), the re-scoped cost screen
  on the idle host (five positions, four depths, two arms, about half an
  hour), no games before B.3.2. Condition: the amended arm repeats the
  reads of §1.5 on the committed tree at ticket 6b.
- **Land a smaller coherent subset first** (the check policy, the null
  move and IIR; ProbCut and singular after). The ticket-4 arm had its own
  unresolved cost on position 11 (26M at depth 15, the RFP–NMP band of
  amendment 2 §3) and WAC 156/234, so it is not a clean half; two clusters
  mean two paired runs (4,000 games), two sweeps, two SPRTs and two
  reviews, roughly twice B.3.2–B.3.4's compute and maintainer time, and
  the categoricals of §9 read on a search that lacks the block they
  interact with (the check policy against singular, `B3MultiCut` against
  the null move's population). It is also against the programme's unit of
  strength (PROCESS *Cluster delivery shape*; the maintainer's cohesion
  rule of 2026-10-30). Not recommended; it becomes the right step only if
  the fixture cannot be held with the floors by any seed, which §1.5 shows
  is not the case.
- **Return B.3 to RESEARCH as a whole.** It would re-open B.3.0 with the
  evaluation's blindness in won endings as a premise (a week of research
  and a dependency on the evaluation programme) while the mechanisms,
  the order and the switches stand and every return has located its
  cause within a day. Not recommended; the condition for it is the
  maintainer keeping both the summed node gate and the donor's 16, which
  no admissible setting passes.

**Condition for `READY_FOR_IMPLEMENTATION`:** the maintainer's two
decisions of §5. On both as recommended, B.3.1 resumes at ticket 6b. On
the margin alone with the per-position gate (option 2), likewise, with
P11's warning. On the margin alone with the summed gate, ticket 6b lands
and the leaf returns at the gate with the counters already here. On
neither, B.3 returns to research as a whole.

### 4. Clauses changed, predictions and calibration (question 4)

**Clauses replaced** (old → new; the originals stand as written):

- §7 "Singular margin, per ply (1/63 cp) … **126** (2.0/ply); range
  13..378" → "**252** (4 cp per ply, Basilisk's fitted value); range
  13..378; the donor's 17 and the geometric mean 126 are the sweep's extra
  points" (maintainer decision 1, §5).
- §7 "`seekMate` … **16, 564** (F lines)" → "**12**, 564; `B3SeekMateDepth`
  a registered sweep coordinate at 8, 12 and 16" (maintainer decision 2,
  §5; under its alternative the 16 stands and amendment 2 §5's node bound
  becomes "the geometric mean over the 40 bench positions of the
  per-position ON/OFF interior-node ratios ≤ 2, position 11's ratio
  recorded").
- §9 gains item 14, `B3SingNegative` 3 against 1 (F line, one paired run
  at ±5 after item 5; default the donor's 3), and item 15,
  `B3PvQuietPrune` (1 the donor's PV-node quiet pruning off the previous
  PV; 0 the OFF arm's exemption; default 1; a new S line on ticket 2's
  seam), both also ablation levers for a negative paired run before any
  mechanism is blamed.
- §11's BAS-S42 sweep registration gains `B3SeekMateDepth` at 8, 12 and 16
  and `B3SingNegative` at 0, 1 and 3 as registered sweep coordinates (F
  lines read by the curvature sweep only, never SPSA), beside the margin's
  extra points 17 and 126.
- §10 ticket 6's end-of-ticket reads gain the fixture at 100,000 nodes
  (a8=Q with a positive score); ticket 10's two-arm table gains the
  fixture's budget series (T16's five budgets, recorded), KR-K's
  30,000-node mean (recorded) and BAS-R07's rec1 at 100,000 nodes
  (recorded, a tablebase win expected).
- §10's cheap qualification: "the conversion floors at the five budgets …
  the wrong-bishop draws" → adds "the promotion fixture at 100,000 nodes
  and its budget series".
- Amendment 2 §3's cost-screen re-scoping stands.
- Amendment 1 §6's `follow_pv` quiet-pruning consumer stays on the ON arm
  (V1 shows it is not the fixture's cause alone and costs WAC 3 when
  removed); it gains the switch above.

**Frozen predictions and calibration.** P8b (frozen in amendment 2 §6:
[25%, 70%] at depth 13; its upper edge the return trigger) read 53.5% at
`1e77ec9` and reads **14.9%** on V6: a miss on the lower edge if decision
2 is taken, to be copied into BAS-S41: *"P8b's lower edge assumed the
singular block runs in won positions, where the triples chain into the
budget (position 11 alone carried 25,946 of the ticket-6 arm's 30,255
truncations); with the donor's guard fired from root depth 12 those
positions leave the block above depth 11 and the budget binds on the
rest at 15%. The band should have been conditioned on the guard."* The
binding tests of ticket 6b stay amendment 2's (the node bound, the 100k
floor, the screen) with the fixture added; P8b's upper edge remains the
return trigger. P5 (bench 13 within 0.6–1.4× at the final arm) is not yet
scored: at `1e77ec9` the ON arm reads 25.7× with position 11 at 97% of
it, on V6 0.38×; the final read is ticket 10's. Return 3's "every single
switch flip recovers a7a8q" is scored here as a tree-width effect (§1.4),
not a mechanism.

**New predictions, frozen before their reads:**

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P11 | The re-scoped cost screen (amendment 2 §3: per position, the geometric mean over depths 16–19 of the ON arm's per-depth growth ratio over the OFF arm's ≤ 1.5, every depth finished) **passes on all five positions on the V6 arm** (probability 75%), and on the margin-252 arm with the donor's 16 **fails on position 11** (probability 60%) and passes on the other four | moderate / low-moderate | a V6 fail says the growth comes from outside the singular block (ProbCut's 4.5% of cuts, the null move's hand-off band) and ticket 6b returns; a pass on position 11 with the 16 says the won-position cost is in size, not growth |
| P12 | B.3.3's sweep reads `B3SeekMateDepth` 8 and 16 within ±5 Elo of 12 at `3+0.03`, and `B3SingNegative` 1 within ±5 of 3; neither moves by games what position 11 moves by nodes | moderate | either outside ±5 says the won-position tree is Elo, and the fit moves it |
| P13 | The paired run of `B3PvQuietPrune` 0 against 1 reads within ±5 | moderate | ≤ −5 says the donor's PV pruning costs the engine the endings its evaluation cannot see, and the default flips to the OFF arm's exemption |
| P14 | B.3.4's `10+0.1` direction read of the fitted arm shows no conversion regression against the OFF arm: the won-position share of its decisive games (a win from a position the oracle scores ≥ +564 at move 40) within 3 points of the OFF arm's | low-moderate | a drop says `seekMate` at 12 costs conversion at length and the sweep's 16 is re-read at `10+0.1` |

### 5. Decisions, recommendations, the amended tickets, the condition

**Research decides (its own):**

1. The fixture "promotion: a queen where a rook only transposes" at
   100,000 nodes is a theory-truth fixture of the ON arm, binding at every
   ticket, with the budget series and the fixtures of §1.5 beside it.
2. §9 gains `B3SingNegative` and `B3PvQuietPrune` as read-by-games
   categoricals (items 14 and 15, §4), and the sweep gains
   `B3SeekMateDepth` and `B3SingNegative` as coordinates.
3. P11–P14 are registered; P8b's lower edge is scored (§4).
4. The evaluation finding is recorded for the evaluation programme, not
   implemented here: along a tablebase-won KRP–KPP line the corrected
   static eval reads −13 to +78 (§1.3), and the OFF arm's own fixture
   survives on tree shape (`searchmoves a7a8q` reads 0 at depths 11–12).
   No leaf owns it; B.3.2's oracle differential will show whether the
   class recurs.

**The maintainer decides, each with its alternative and the main
counter-argument:**

1. **The singular margin seeded at 252** (4 cp per ply; range 13..378
   unchanged) as the fixture's repair. Recommended: it is the one repair
   that holds the floors and its own ticket's reads (fixture +956, WAC
   237 and 136, P8b 50.6%, the other 39 positions at 0.38× the OFF
   arm's), it is Basilisk's own fitted value and a registered sweep
   point, and it acts through the mechanism the OFF arm's trace shows.
   Alternative: `B3SingNegative=1` (WAC 239, the mechanism's own clause),
   rejected by ticket 6's reads (P8b 72.7%, 12.5× interior nodes,
   position 11 at 236M). Counter-argument: 126 was the geometric-mean seed
   of §7's rule 2, confirmed twice, and amendment 2 §1 defended it;
   moving it on a fixture read means the fit starts from Basilisk's
   column rather than between the columns. The answer is that the sweep
   keeps 17 and 126 as points, so the fit loses nothing, and a
   theory-truth veto outranks a seed rule.
2. **The donor's `seekMate` guard fired from root depth 12** (`B3SeekMateDepth`
   seed 12; 564 unchanged; 8, 12 and 16 on the sweep). Recommended: with
   it every registered gate passes as written (§1.5, §2 option 1),
   position 11 reads 0.97× the OFF arm's, and the conversion floors hold
   at 118 with KR-K faster at 30,000 nodes. Alternative: keep the donor's
   16 and re-state the node gate per position (§2 option 2), which passes
   on the margin alone at a geometric mean of 0.41 and leaves position 11
   at 77× for the screen and the paired run to price. Counter-argument: a
   ply seed moved four plies from the donor's on one position's node
   count is the hand-tuning the contract forbids, and a guard that
   switches singular extensions off in every won position from depth 12
   could cost conversion in games where the floors' 120,000 nodes do not
   reach. The answer is that the sweep reads 16 beside it (P12), the
   `10+0.1` read is registered for it (P14), and the alternative's price
   is a gate changed after a read plus a 77× position the screen is
   predicted to fail on (P11).

**The amended ticket text implementation resumes on** (replacing the
ticket-6 paragraph of the resumption brief):

- *Ticket 6b.* `B3SingMargin` seed 252 (range 13..378) and
  `B3SeekMateDepth` seed 12 (range 8..64), both in `search_params.h`;
  `B3SingNegative` stays 3 and becomes §9 item 14; `B3PvQuietPrune` is
  added on ticket 2's seam (`!is_pv || (kB3Proof && kp.b3_pv_quiet_prune
  && !ss->follow_pv)`, default 1) as §9 item 15; the surface regenerated
  with `B3SeekMateDepth` (8, 12, 16) and `B3SingNegative` (0, 1, 3) as
  sweep coordinates and the margin's points 17 and 126. OFF 16,615,757;
  ON recorded (V6 read 6,263,268 on this source less the switch line). At
  its end, on the committed tree: P8b recorded, its upper edge binding;
  WAC at 100,000 nodes ≥ 232; the fixture at 100,000 nodes a8=Q with a
  positive score; the depth-13 read's interior nodes within 2× the OFF
  arm's (V6 read 0.26×), position 11 recorded; the two-arm `bench 13`.
  Outside any of these, Return 4 with the counters, no seed moved by hand.
- *Tickets 7–10* as packet §10 with amendments 1–2 and this one: the cost
  screen as re-scoped (idle host; P11 is its prediction), ticket 10's
  depth-6 floor re-based on the ON arm, the fixture's budget series,
  KR-K's 30,000-node mean and BAS-R07's rec1 recorded in the two-arm
  table.

**Condition for resuming:** the maintainer's decisions 1 and 2 above, as
§3 states. B.3.1 stays `RESEARCH` until then.

**Maintainer decisions, 2026-10-30, after the amendment:** both taken as
recommended. 1. `B3SingMargin` is seeded at 252 (range 13..378; 17 and 126
the sweep's extra points). 2. `B3SeekMateDepth` is seeded at 12 (range
8..64; 8, 12 and 16 on the sweep; `B3SeekMateScore` 564 unchanged). The
amended ticket 6b text above is the contract implementation resumes on;
B.3.1 resumes `READY_FOR_IMPLEMENTATION`.

## Return 4 — B.3.1 ticket 7: training the correction under exclusion turns the wrong-bishop draws into wins (2026-10-30)

**What resumed.** Amendment 3's ticket 6b landed and passed every gate.
Ticket 7 was built as §10 wrote it. Ticket 7 fails a floor that §10
binds on both arms at every ticket: the KBP-K wrong-bishop draws (B.12).

**Ticket 6b** (`eeeaa86`, with the sweep notes in `cfb9232`). The changes:

- `B3SingMargin` is 252 and `B3SeekMateDepth` is 12.
- `B3PvQuietPrune` (S, default 1) is added on ticket 2's seam.
- The B3 surface is regenerated: `b3.tune.toml` carries the sweep
  registrations as generated notes, which the generator checks against
  the table. `B3SeekMateDepth` 8, 12, 16 and `B3SingNegative` 0, 1, 3
  are F-line sweep coordinates. `B3SingMargin`'s extra points are 17
  and 126.

Its reads, on the committed tree (`tools/results/b31-20261030/a3t6b/`):

| Read | Result | Rule |
|---|---|---|
| OFF `bench 13` | 16,615,757 | exact |
| ON `bench 13` | **6,263,268** | recorded (V6's read; the switch at 1 is the V6 behaviour) |
| Interior nodes, depth 13, 40 positions | 2,061,141 against the OFF arm's 8,008,320 = **0.26×** | ≤ 2× |
| Position 11 | 267,873 against 277,125 = 0.97× | recorded |
| Geometric mean of per-position ratios | 0.31 | — |
| P8b | 1,557 / 10,453 = **14.9%** | upper edge 70% binds; the lower-edge miss scored by amendment 3 §4 |
| WAC | 237 at 100,000 nodes; 136 at depth 6 | ≥ 232 |
| The fixture at 100,000 nodes | **a7a8q +956** at depth 10 (OFF a7a8q +616) | a8=Q, positive |
| Release CTest | 25/25 on both arms | — |
| Diag `test_b3_proof`, `test_search`, `test_stack_unwind` | pass | — |

The counter table against ticket 6 (`compare.md`) flags 17 Cluster 1 rows
over 20%. That is the read's composition: bench position 11 was 87.9% of
the previous read and is now 13% of this one. Over the other 39
positions every Cluster 1 row is within 20% except `hist_reward_updates`
(+32.7%).

**Ticket 7 as built** (`6007ab5`). Three changes:

- `B3CorrDecisive` (default 1) removes the `!is_decisive(best_score)`
  refusal on the ON arm. The bonus stays clamped at ±256.
- `B3CorrExcluded` (default 1) removes the exclusion refusal. The
  exclusion node trains with the parent's corrected static eval, as the
  donor's `eval = ss->staticEval` does (`search.cpp:831-832`).
- `CoreHistoryFills` (default 0) at 1 fills fresh tables with sf_19's
  `Worker::clear()` values: main −5, capture −742, pawn −1,338,
  continuation −586, structural correction −5, continuation correction
  +5. They are applied at the first search after a clear, so a tune
  build's option takes effect at `ucinewgame`.

Tests in `test_b3_proof`:

- The fill values.
- Each switch moves the ON arm's tree and leaves the OFF arm's.
- 79/79 on both arms.

Ticket 7's per-ticket reads (`tools/results/b31-20261030/t7/`):

| Read | Result |
|---|---|
| OFF `bench 13` | 16,615,757 |
| ON `bench 13` | **4,517,333** |
| WAC | 249 at 100,000 nodes; 134 at depth 6 |
| Fixture | a8=Q at 100,000 nodes (`test_search` passes) |
| Interior nodes | 2,529,401 (1.23× ticket 6b) |
| Cluster 1 rows over 20% | `cont_hist_pruned` only (2.40% to 3.22% of interior nodes), following the correction's new producers |
| OFF release CTest | 25/25 |
| ON release CTest | **24/25** |

The failing ON test is `test_endgames`: 103/121, all 18 failures in
"KBP-K wrong bishop reads a draw at depths 8-14".

**Premise returned.** §10 *Invariants*: "the mate-drive floors, the
wrong-bishop draws (B.12) and the mate band bind both arms at every
ticket". §11's stop rule: "a floor failure at any ticket returns the
leaf". The §9 default under ticket 7, `B3CorrExcluded` 1 ("the donor";
B-15, "low stakes"), breaks it.

**Attribution** (read W7, `wb_switches.py`, `wb_switches.json`). Setup:

- The test's four FENs at depths 8–14 under its 200,000-node cap.
- Hash 8, a fresh `ucinewgame` per search, on the ON Tune build of the
  ticket-7 source.
- Rule, written before the run: a setting holds when all 28 searches
  read |score| ≤ 75 (the test's `DRAW_TOL`) at full depth. A switch
  whose flip alone restores that carries the failure.

| Setting | Held | KBP 7k/8/6KP…5B2, depths 8–14 | a-file k7/8/PK6…2B5 | reversed 5b2/…/7K b | 7k/7P/6K1…3B4 |
|---|---:|---|---|---|---|
| defaults | 10/28 | 0 0 0 +151 +259 +259 +256 | 0 0 0 +396 +358 +358 +382 | +68 +68 +149 +235 +233 +233 +233 | +32 +36 +218 +225 +225 +225 +225 |
| `B3CorrDecisive=0` | 10/28 | the same to +404 | | | |
| `B3CorrExcluded=0` | **28/28** | 0 at every depth | 0 | 0 | 0 |

`B3CorrExcluded` alone carries it, and `B3CorrDecisive` is innocent.
The false scores are not of the known-win class. They are 150–400 cp,
the size of a corrected static eval.

A guess was refuted, so it is not claimed: refusing exclusion training
when |best score| ≥ `KNOWN_WIN` (temporary build `t7-v1`, patch
`v1_excluded_known_win_guard.patch`, reverted) reads the same 10/28.

The likely route, a hypothesis for research and not established here:

- In these endings the evaluation's recognised draw is 0.
- `corrected_eval` adds the correction to it unconditionally.
- An exclusion search that excludes the defender's one drawing move
  returns a large error that the normal nodes of a dead draw never
  produce.
- So the pawn-key correction drifts, and the corrected static eval of
  a dead draw reads ±2–4 pawns.
- The OFF arm never trains there.

**The ticket's own read: the floors at five budgets, by switch.** Release
builds; the switch variants are temporary builds with that default
flipped (`t7-cd0`, `t7-ce0`, patches `cd0.patch` and `ce0.patch`,
reverted). Logs are `conv_*.log` and `endgames_*.log`.

| Setting | KQ-K / KR-K / KBB-K | KBN-K 30k / 45k / 60k / 90k / 120k (sum) | KR-K mean at 30k | `test_endgames` | `bench 13` | WAC 100k / depth 6 |
|---|---|---|---:|---|---:|---|
| defaults (1, 1) | 24 at every budget | 23 / 23 / 24 / 24 / 24 (**118**) | 26.4 | 103/121 | 4,517,333 | 249 / 134 |
| `B3CorrDecisive=0` | 24 at every budget | 24 / 21 / 24 / 24 / 24 (117) | 29.8 | 103/121 | 5,128,849 | 243 / 136 |
| `B3CorrExcluded=0` | 24 at every budget | 22 / 23 / 24 / 24 / 24 (117) | 27.7 | **121/121** | 4,372,801 | 239 / 135 |

Every setting holds the conversion floors: KBN-K ≥ 112, and the
families at 24. The means at budgets other than 60,000 are recorded,
not judged. KQ-K's 10.7 at 120,000 trips the test's staleness ratio
there, as amendment 3's read noted for KR-K at 30,000.

**Options.**

1. *`B3CorrExcluded` default 0* (the legacy refusal), with the switch
   kept for B.3.2's paired run (§9 item 11).
   - It restores 121/121 and holds every floor (WAC 239, KBN-K 117).
   - The stakes are those §9 already gave it: B-15, "low stakes";
     Rarog read +2.3 ± 9.5.
   - It is a §9 default and so the maintainer's. The donor trains there.
2. *Keep the donor's default and guard the producer or the consumer*:
   no correction applied to an evaluation the evaluator has scaled to a
   recognised draw, or no exclusion training on such a node.
   - This is new mechanism, research's to specify and test.
   - It touches the correction form, which §10 lists as a non-goal for
     this leaf.
3. *Both*: 1 now, and 2 as a finding for the evaluation programme,
   beside amendment 3's KRP–KPP finding (§5, research decision 4).

Recommendation: option 3. Option 1 because a theory-truth floor is a
veto and the switch costs little. Option 2 because the route above, if
research confirms it, also affects the OFF arm's correction wherever a
recognised draw meets a pawn key trained elsewhere. No default has been
moved.

**State at the return.**

- Tickets 0–7 are committed (`6007ab5`).
- The OFF arm is 16,615,757 with release CTest 25/25 at every commit.
- The ON arm's release CTest passes at ticket 6b. At `6007ab5` it fails
  `test_endgames`' wrong-bishop section, which was not relaxed.
- Tickets 8–10 are not started.
- Amendment 2's surface additions (the RFP coordinates and the donor's
  pair) belong to ticket 10's regeneration.
- No idle-host read was run.

## Research amendment 4 — the correction meets the recognised draw: the exclusion producer, the shared consumer and ticket 7's default (2026-10-31)

Written on Return 4, at `6286acf`, class `R2`: no ticket implemented, no
engine code committed, no seed or default moved. Reads are node-count or
depth reads and ran on the busy host under the host rule; no timed read was
needed. Temporary builds are byte-exact edits reverted after the build, their
patches under `tools/results/b31-20261030/amend4/` (`trace.patch`,
`gb.patch`, `gc.patch`, `gd.patch`); the scripts are in `amend4/scripts/`
(`a4lib.py` the UCI helpers, `variant.py` the save-and-restore builder,
`a4_trace_patch.py`, `a4_r1.py`, `a4_r2.py`, `a4_guards.py`, `a4_price.py`).

### 0. What this run says, plainly

Return 4's hypothesis is confirmed, with instrumentation, and it is half
the story. The exclusion search at the defender's corner node excludes the
one drawing king move, reads the loss of the position without it (−3 to −9
pawns at depth 2–5, never a decisive score), and the training block charges
that error to the node's static evaluation, which is the recogniser's 0.
The bonus saturates at −256 and lands on every structural table at once,
not on the pawn key alone: in these endings the pawn key is one pawn, the
minor key one bishop, the defender's non-pawn key is empty (one entry for
every bare-king position in any ending) and the attacker's is the bishop's
square. The corrected static evaluation of a dead draw then reads
±1.0–4.3 pawns, and the score reaches the root as the PV leaf's quiescence
stand-pat, carried up unchanged by thirteen plies of king and bishop
moves and stored as exact TT entries that seed the next iteration.

The other half: the consumer is shared. `corrected_eval` adds the
correction to a recognised draw on both arms, and the OFF arm is exposed
whenever its tables were trained on the same structures elsewhere. One
position proves it organically: `5k2/8/6KP/8/8/8/8/5B2 b` (Black to move,
Kg8 the only drawing move, the recogniser silent until the king reaches
g8) reads **+90 for White at depth 11 on the OFF arm**, because the root's
ordinary evaluation (−797 for Black, true value 0) trains Black's shared
entries +256 per iteration and the recognised nodes one ply later inherit
them. Priming the OFF arm's tables with two lost same-structure positions
breaks the test's own fixtures (4 of 28, up to +172). The normal nodes of a
recognised draw cannot repair this: their search value is the corrected
evaluation itself, so the error they measure is zero whatever the
correction says. Neither donor composes these two mechanisms: sf_19's
evaluation has no recognised draw for a correction to meet, and the
classical donor had the recognisers and no correction history. Basilisk's
composition order — recogniser, then correction — is the defect, and it
is older than Cluster 2; ticket 7's exclusion training is the first
producer strong enough to show it inside a 200,000-node search.

For B.3.1 the consequence is that the two theory-truth fixtures pull in
opposite directions. The exclusion training that breaks the drawn
wrong-bishop corner is what gives the won promotion fixture its positive
score at 100,000 nodes: the same biased estimator — the value without the
only move understates every singular position — is right by accident
where the only move merely delays a loss and wrong where it holds a
draw. `B3CorrExcluded=0` holds the 28 wrong-bishop reads and leaves the
fixture at a7a8q with score 0 at depths 9–11 (positive from 200,000
nodes), which amendment 3's rule as written forbids. The producer guard
on the raw evaluation (G-C below) holds both, keeps the donor's default
and its tactics (WAC 249), and names the mechanism exactly. The consumer
guard (G-D) is the general repair and belongs to its own correctness
leaf, because it moves the OFF arm.

### R. Reads registered before running

Frozen before any build or read of this amendment (the scratch record is
reproduced here; `price_*.json`, `r1_*.log`, `r2.json`, `r2.txt` are the
results).

- **R1, the route.** Trace build `a4-on-trace` (ON release plus
  `trace.patch`): `a4x` every correction-training event at an exclusion
  node (iteration, ply, side, path, excluded move, best score, static eval,
  recomputed raw eval with the recogniser flag, depth, bonus, the four
  structural entries before the update); `a4n` every PV interior node at
  exit (raw, flag, correction and its per-table parts, corrected static
  eval, best, bound); `a4q` every PV quiescence stand-pat; `a4it`
  per-iteration counts of training events by node kind, side and sign;
  later `a4y` every normal-node event with |bonus| ≥ 64 at ply ≤ 14
  (added for R2a′, below). Searches: the test's first two FENs at
  `go depth 11 nodes 200000`, Hash 8, and the first at depth 12. Rule: the
  route is CONFIRMED when, on the first search reading > 75, (a) `a4x`
  events exist with the defender to move, static eval 0 from the
  recogniser, best below −75, a negative bonus, and an excluded king move
  that keeps the corner; (b) the PV leaf has raw 0 from the recogniser and
  a corrected value of magnitude ≥ 75 favouring the attacker; (c) the
  per-table parts at that leaf sum to the correction and name the
  carrier. REFUTED if the moving events are normal-node events
  (exclusion events fewer than a tenth of the same-sign normal events on
  the defender's side) or the PV leaf's raw eval is an ordinary
  evaluation. Also recorded: |best| at `a4x` events against `KNOWN_WIN`
  and `tablebaseWinInMaxPly`; the route to the root from the PV records.
- **R2a, organic, both arms, fresh tables.** `5k2/8/6KP/8/8/8/8/5B2 b`,
  depths 8–14 under 200,000 nodes, Hash 8, `ucinewgame` per search. Rule:
  a setting holds when all seven read |score| ≤ 75 at full depth. **P15a:**
  OFF holds 7/7 (65%); ON fails at least one depth (70%).
- **R2a′** (registered after R2a's miss, before running): the OFF trace
  build on the same FEN at depth 11. Rule: the OFF drift is the same
  consumer with a different producer when (i) no `a4x` event exists,
  (ii) normal-node events at ordinary-evaluation nodes (flag 0) on the
  shared keys carry it, and (iii) the PV nodes of the +90 iteration have
  raw 0 from the recogniser and a correction of magnitude ≥ 75 favouring
  White. (ii) was first written as "negative-bonus events with Black to
  move"; the counts-only first run showed the sign and side were not that,
  so the `a4y` record was added and (ii) re-stated as either side and
  sign before the second run; the first wording is the calibration note.
- **R2b, primed, OFF arm.** One process, Hash 8, one `ucinewgame`, two
  priming searches `go depth 12` with the defender to move on lost
  same-pawn-key positions (`8/8/5K1P/8/8/8/k7/5B2 b`,
  `8/8/6KP/8/8/8/8/k4B2 b`), then the test's 28 searches without
  `ucinewgame`. Rule: the OFF consumer is EXPOSED when any of the 28 reads
  |score| > 75 at full depth; the control (the test's form, `ucinewgame`
  per search) must hold 28/28 or the read is void. **P15b:** exposed, at
  least 4 of 28 (75%).
- **R2c, the donor.** `git show sf_19:src/evaluate.cpp` read for any
  evaluation-level recognised draw.
- **R3, the guards priced.** On the ticket-7 source, ON release:
  **G-A** `B3CorrExcluded` 0 (Return 4's `t7-ce0`, bench 4,372,801
  reproduced); **G-B** fail-high only — an exclusion node trains only when
  its search failed high, `(ss->excluded == MOVE_NONE || (train_excluded
  && best_score >= beta))`; **G-C** the raw-evaluation guard — an
  exclusion node trains only when the node's raw static evaluation is not
  0, carried from the outer pass in a new stack field `ss->raw_eval`,
  `(ss->excluded == MOVE_NONE || (train_excluded && ss->raw_eval != 0))`;
  **G-D** the consumer guard — `corrected_eval` returns 0 for a raw
  evaluation of 0 and the quiescence stand-pat adds no correction to one
  (both arms; priced on the ON arm, the OFF fingerprint moves). For each,
  with the defaults beside them: the test's 28, `bench 13`, WAC at depth
  6 and at 100,000 nodes (floor 232), the promotion fixture at 33,000,
  60,000, 100,000, 200,000, 400,000 and 1,000,000 nodes, `test_endgames`,
  the conversion floors at the five budgets (KQ-K, KR-K, KBB-K 24 at
  each; KBN-K's sum ≥ 112). **P16:** G-B holds the 28 (60%); G-C holds the
  28 (80%); G-D holds the 28 (90%); every guard holds the floors and the
  fixture (70%); G-B's WAC at 100,000 nodes within 3 of the defaults' 249
  (55%). Admissibility: holds the 28, every floor and the fixture; then
  preferred by the narrowest semantics that names the mechanism, donor
  precedent, and WAC and nodes recorded rather than judged.

### 1. The route (question 1)

**1.1 What the trace shows on the test's first FEN** (`7k/8/6KP/8/8/8/8/5B2 w`,
`r1_wb0_d11.log`, `r1_wb0_d12.log`; the a-file FEN `k7/8/PK6/8/8/8/8/2B5 w`
in `r1_wb1_d11.log` reads the same way with the squares mirrored). Depths
1–10 read 0 with no exclusion training at all (`a4it`: zero `x` events;
the tree is 2,832 nodes at depth 10). Iteration 11 is the first with
exclusion searches at the defender's corner nodes, and the first to read a
false score: 151 at depth 11, 259 at 12 (a-file: 396 at 11).

**1.2 Which exclusion nodes train, and what they exclude.** Every negative
exclusion event has Black to move with the king on g8 (or g7 after the
white king left g6), the recogniser's 0 as its static evaluation
(`raw=0 rec=1`) and the excluded move **Kh8** — the one move that keeps
the corner (`g8h8` 177 of 213 events at depth 12, `g7h8` 28; a-file
`b7a8` 98, `b8a8` 96 of 197). The exclusion search finds Kf8 (or the
mirror) and reads the attacker's h7–h8 as material: best −315 to −962,
median −415 (a-file −527), at exclusion depths 2–6. Three white-side
events (`g6f6`, `e2d1` excluded) and 69 fail-high positive events
(best ≥ beta, the multi-cut and negative-extension cases) complete the
population. **Every negative event is a fail-low exclusion** (213 of 213
at depth 12; 197 of 197 on the a-file), and every fail-high event has a
positive bonus. That split is what the G-B guard reads.

**1.3 The size of the trained error.** The bonus is
`(best − static) · depth · scale · 3/2 / 128` clamped to ±256 (the
exclusion fails low, so `raised_alpha` is false and the 3/2 factor
applies); with an error of −4 pawns at depth 3 it is −225. The median
bonus is −146 (a-file −154) and 34 of 213 events saturate at −256; the
error is one to two orders larger than the few-centipawn errors the
correction is built to learn. After one aspiration pass of iteration 11
the defender's entries read −400 to −630 of the ±1024 bound (`a4x`:
`pw=-518 mi=-414 nw=-500 nb=-633` at a ply-19 node); weighted, that is
−60 cp from the pawn table, −33 from the minor and −50 to −62 from each
non-pawn table.

**1.4 Which tables carry it.** All four structural tables and the
continuation correction, because in this ending every key is constant
across the drawn region or nearly so: the pawn key is the single pawn
(one entry for the whole tree until it advances), the minor key the
bishop's square (a handful of entries), the defender's non-pawn key is
**empty** (`nonpawn_key[BLACK] = 0`: one entry shared by every position in
which Black has a bare king, in this or any other ending), the attacker's
non-pawn key the bishop's square again, and the continuation correction
is keyed by the king's shuffle moves (`[KING][g8]`, `[KING][h8]`). At the
root of iteration 12 (`a4n it=12`, the last pass): correction 225 cp =
pawn 24 + minor 14 + non-pawn(W) 20 + non-pawn(B) 17 + continuation 150
(the remainder); at the PV node at ply 12, iteration 11: 110 = 45 + 8 +
13 + 47 − 3; on the a-file at ply 12: 392 = 96 + 69 + 91 + 83 + 53. The
bookkeeping holds at every PV node (the parts are integer-divided, so the
remainder absorbs rounding of a few cp). The largest single part is the
pawn table on the h-file FEN and the pawn and non-pawn tables on the
a-file; Return 4's guess ("most likely the pawn key") named the largest
structural carrier but missed that the drift is spread over every table
the training touches — a guard on one table would not hold.

**1.5 The false-scored nodes.** Every PV interior node of iteration 11
(168 on the h-file, 232 on the a-file) has `raw=0 rec=1`: the evaluator
returned the recogniser's 0 at every one, and the correction added
±100–154 (h-file) or ±237–433 (a-file). The PV leaf is a quiescence
stand-pat at ply 19 (`a4q path=[f1e2 h8g8 e2d3 g8h8 g6f6 h8g8 d3c2 g8h8
f6g5 h8g8 c2b1 g8h8 g5h4 h8g8 h4g3 g8h8 g3h3 h8g8 b1d3] raw=0 rec=1
corr=-151 sp=-151`), and the root reads exactly 151. One PV node on the
a-file (ply 3, Black to move after `d2f4`) carries `raw=32002`, a stored
mate-band static eval from an earlier underpromotion line — recorded,
not pursued; the node's best is the PV's −396 regardless.

**1.6 How it reaches the root.** By the PV, not by pruning and not by a
TT cutoff: the leaf's stand-pat is the corrected static evaluation of a
recognised draw, and every ancestor's best equals it because every move
in the corner shuffle leads to another recognised draw with the same
correction. The scores are stored as exact entries (`flag=1`, `TT_EXACT`)
and iteration 12 opens with them (`a4n it=12` root: `se=68 best=170`
before any new training). The static evaluation's pruning uses are not
on the route at PV nodes; they carry the corrupted value at the non-PV
siblings, which is why the exclusion search's own answer (the loss after
Kf8) stays undisturbed.

**1.7 Why the `KNOWN_WIN` guard failed, and why `B3CorrDecisive` is
innocent.** `|best|` at the negative events is 24–962: `<1000` for 249
of 283 events, `<75` for the rest, none ≥ 10,000 and none ≥ 31,744. The
exclusion search runs at depth 2–6 and never reaches the promotion; it
reads the material after Kf8 h7. A guard at `KNOWN_WIN` excludes nothing
that trains here, and `is_decisive` (|score| ≥ `tablebaseWinInMaxPly`
= 31,744) is further still: `B3CorrDecisive` admits mate and tablebase
scores, and no such score is involved. Return 4's attribution (10/28 with
`B3CorrDecisive=0`, 28/28 with `B3CorrExcluded=0`) stands.

**1.8 The mechanism in chess terms.** An exclusion search measures the
value of the position *without its TT move*. Where the move is singular
that value understates the position by construction: the search is
asking "how bad is it if the only move is unavailable?" and charging the
answer to the static evaluation of a position in which the move is
available. The estimator is biased toward "worse for the side to move"
exactly at singular nodes. The bias is right by accident where the only
move merely postpones a loss — the promotion fixture's defender has a
string of only moves along a tablebase-lost line, and the training pushes
its corrected evaluation toward the truth, which is why ticket 7 reads
a7a8q +747 at depth 10 where `B3CorrExcluded=0` reads 0 — and wrong where
the only move holds a draw forever, which is the wrong-bishop corner.
The donor accepts the bias on NNUE evaluations whose errors are small and
varied; its multi-cut block trains the correction only on a fail-high
exclusion result (`search.cpp:1280-1287`), which is the half of the
estimator that is a true lower bound, and its node-level training
(`:1631-1638`) trains on both halves. Rarog's B-15 read, +2.3 ± 9.5, is
consistent with a small effect either way and is a prior only.

Two properties of Basilisk make the bias load-bearing here. First, the
recogniser returns exactly 0 at every node of the drawn region
whatever the kings and bishop do, so the structural keys receive hundreds
of consistent −256 updates with no counter-signal from the evaluation's
own variation. Second, the normal nodes of a recognised draw cannot
restore the tables: their search value is the corrected evaluation of
another recognised node, so `best − static` is zero whatever the
correction has become (`a4it`: the 439 and 650 positive normal events on
Black's side in iterations 11 and 12 are small bonuses from the
aspiration passes, and the drift grows from 151 to 259 through them). The
only restoring force is the gravity term of `HistoryBound::update`, which
acts only when an entry is updated. On the OFF arm the same consumer
exists and the same absence of a restoring force; what the OFF arm lacks
is a producer that fires inside the drawn region (§2).

### 2. The OFF arm and the donor (question 2)

**2.1 Organic exposure, one position (R2a).** `5k2/8/6KP/8/8/8/8/5B2 b`:
Black to move, Kg8 the only drawing move (Ke7/Ke8 lose to h7), the
recogniser silent at the root (the king is two squares from h8) and
firing after Kg8. Fresh tables, `ucinewgame` per search, Hash 8:

| Depth | OFF arm | ON arm (ticket 7 defaults) |
|---|---|---|
| 8–10 | 0, 0, 0 | 0, 0, 0 |
| 11 | **−90** (for Black; 3,075 nodes) | 0 |
| 12 | −66 | 0 |
| 13 | −66 | −7 |
| 14 | −3 | **−152** (24,886 nodes) |

Both arms hold 6 of 7. **P15a is a miss on the OFF arm** (predicted 7/7
at 65%) and a hit on the ON arm. The OFF arm's correction corrupts a
recognised draw from one position's own search, inside 3,075 nodes.

**2.2 The OFF arm's producer (R2a′,** `r2a_off_trace_d11.log`). No `a4x`
event (the OFF arm never trains under exclusion). The producer is the
**root itself**: its raw evaluation is −797 for Black (the ordinary
evaluation of a bishop and a pawn, the recogniser not firing) and its
search value is 0 (Kg8 draws), so every iteration trains Black's shared
entries +256 (`a4y it=1..10 ply=0 raw=-797 rec=0 best=0 bonus=+99, +186,
+250, +256…`; the pawn entry climbs 0 → 883 of 1024). Those entries are
the pawn key, the minor key (the bishop on f1), the attacker's non-pawn
key (the same bishop) and the **empty** defender's non-pawn key. One ply
later, in the corner, the recognised nodes read 0 plus Black's inherited
correction, the search value flows through them, and a handful of
−256 updates at the corner nodes (`a4y rec=1 stm=b` five events) and the
circular training of §1.8 settle the PV at +90 for White: every PV node
of the depth-11 iteration has `raw=0 rec=1` with corrections +63…+83
(White) and −70…−77 (Black), the continuation correction the largest part
(+48 of +83 at ply 1). (ii) holds in its re-stated form — the carrier is
normal-node training at an ordinary-evaluation node on the shared keys —
and the first wording's sign and side were wrong: the drift here is
seeded by the *defender's* correction trained *upward* at the root, not
by negative training in lost sub-lines. The calibration lesson is that
the direction of a shared-table drift is not predictable from the
producer's sign once the circular training of recognised nodes takes
over; only its presence is.

**2.3 Primed exposure (R2b).** The OFF arm, one process, two priming
searches on lost same-structure positions (both read the promotion:
−19,871 and mate −11 at depth 12), then the test's 28 searches without
`ucinewgame`:

| FEN | depths 8–14 |
|---|---|
| `7k/8/6KP/…/5B2 w` | **172 137** 25 25 48 48 71 |
| `k7/8/PK6/…/2B5 w` | 0 0 0 0 0 0 0 |
| `5b2/…/6kp/8/7K b` | 64 29 11 11 11 0 0 |
| `7k/7P/6K1/…/3B4 w` | 28 **107 107** 24 31 31 31 |

Held 24 of 28; the control (the test's form) 28 of 28. **P15b is a hit**
(predicted at least 4 of 28 at 75%; read exactly 4). The a-file FEN is
untouched because its pawn and bishop keys differ from the priming
positions' (an a-pawn and a c1-bishop); the h-file FENs share the pawn
key and the empty defender key. `test_endgames` passes on the OFF arm
only because each of its searches starts from cleared tables; a game
reaching the corner with trained tables is the primed read, not the
control.

**2.4 The donor.** sf_19's `Eval::evaluate` is the network's two outputs
blended with optimism and material, damped by the rule-50 count and
clamped inside the tablebase band (`evaluate.cpp:40-70`); it has **no
evaluation-level recognised draw**, no scale factor and no endgame table.
Draws in the donor are search-level (`pos.is_draw`, repetition, the
50-move rule, insufficient material), and the correction is never added
to them. The classical donor `9587eeeb` had the recognisers (`Endgame<KBPsK>`
is its wrong-bishop rule, `SCALE_FACTOR_DRAW`) and no correction history.
So the donor's precedent for `B3CorrExcluded=1` — and for `B3CorrDecisive`,
and for the correction form itself — says nothing about an engine whose
evaluation classifies positions: Basilisk composes two mechanisms neither
donor composes, and composes them in the order that lets the later one
overwrite the earlier one's verdict. PLAN already carries one face of
this: C.5.2's E7 switch and C.10's ablation ("skip the correction when
the score came from a recogniser", the 2026-10-27 code audit). This
amendment adds the measured defect and the second face: the producers
that train on a recognised 0 (the exclusion search on the ON arm, the
ordinary-evaluation root of a drawn position on both).

**2.5 What this means for the floors.** B.12's test is a theory-truth
floor and stays binding as written. On the OFF arm it passes by the
test's form (fresh tables), not by the engine's behaviour in a game; the
leaf that owns the consumer guard (§3.4) owes a fixture in the primed
form — the test's 28 after two same-structure priming searches in one
process — and the organic FEN of §2.1 at depths 8–14. Neither is added to
B.3.1's floors: both fail on the OFF arm today, and §10 does not fix the
OFF arm.

### 3. The decision (question 3)

**3.1 The guards, exactly.** All on ticket 7's source, the ON arm's
release build, source restored after each (`gb.patch`, `gc.patch`,
`gd.patch`, `gc_off.patch`).

- **G-A** — Return 4's option 1: `B3CorrExcluded` default 0, the legacy
  refusal; the switch stays for §9 item 11's paired run. Precedent: the
  pre-ticket-7 rule (B-15's refusal).
- **G-B** — fail-high only. An exclusion node trains the correction only
  when its own search failed high: `(ss->excluded == MOVE_NONE ||
  (train_excluded && best_score >= beta))`. Chess meaning: the value
  without the TT move is a lower bound on the position only when the
  other moves also hold; when they do not, it is the value of a different
  position. Precedent: the donor's multi-cut block trains the correction
  on exactly this half (`search.cpp:1280-1287`, `value > ss->staticEval`
  after `value >= beta`); its node-level training has no such clause.
- **G-C** — the raw-evaluation guard. An exclusion node trains only when
  the node's raw static evaluation is not 0: `SearchStack` gains
  `int raw_eval = VALUE_NONE`, written beside `ss->eval` from
  `raw_static_eval` in the normal branch (the exclusion re-entry reuses
  the record, so the outer pass's value is read), and the clause is
  `(ss->excluded == MOVE_NONE || (train_excluded && ss->raw_eval != 0))`.
  Chess meaning: a raw 0 from the evaluator is a classification — a
  recognised draw (`return 0`) or a position scaled to `SCALE_DRAW` — and
  a classification has no error for the exclusion search to measure; the
  coincidental ordinary 0 is skipped with them, harmlessly. Precedent:
  none in either donor (neither has both mechanisms); PLAN's C.5.2 E7 /
  C.10 ablation is the same idea on the consumer side.
- **G-D** — the consumer guard. `corrected_eval(raw, corr)` returns 0 for
  a raw evaluation of 0, and the quiescence stand-pat adds no correction
  to one; both arms, every node. Precedent: PLAN C.5.2's E7 switch as
  planned by the 2026-10-27 audit. It moves the OFF arm, so it is priced
  here and not admissible in B.3.1.

**3.2 The price** (`price_defaults_ce0_gb_gc_gd.json`, `price_run.log`,
`conv_*_*.log`, `endgames_*.log`). The defaults column is ticket 7 as
committed (`6007ab5`), re-read on the same host.

| Read | defaults (1) | G-A (`B3CorrExcluded` 0) | G-B fail-high only | **G-C raw-eval guard** | G-D consumer guard |
|---|---:|---:|---:|---:|---:|
| Wrong-bishop 28 (test's form) | 10/28 | **28/28** | **28/28** | **28/28** | **28/28** |
| `test_endgames` (ON release) | 103/121 | 121/121 | 121/121 | 121/121 | 121/121 |
| ON `bench 13` | 4,517,333 | 4,372,801 | 6,295,754 | 3,498,408 | 4,619,025 |
| OFF `bench 13` | 16,615,757 | 16,615,757 | 16,615,757 (by construction) | **16,615,757** (read, `a4-gc-off`) | moves (not read) |
| WAC at 100,000 nodes (≥ 232) / depth 6 | 249 / 134 | 239 / 135 | 242 / 134 | **250** / 134 | 244 / 136 |
| Fixture a7a8q, 33k / 60k / 100k nodes | +47 / +538 / **+747** | 0 / 0 / **0** (d11) | +10 / +336 / +336 | +94 / +661 / **+661** | +50 / +479 / +479 |
| Fixture 200k / 400k / 1M | +918 / +932 / +932 | +1154 / +1349 / +1520 | +1192 / +1296 / +1473 | +926 / +1180 / +1180 | +885 / +1091 / +1091 |
| KQ-K, KR-K, KBB-K at five budgets | 24 each | 24 each | 24 each | 24 each | 24 each |
| KBN-K 30k/45k/60k/90k/120k (sum ≥ 112) | 23/23/24/24/24 (118) | 22/23/24/24/24 (117) | 23/24/24/24/24 (119) | 23/23/24/24/24 (118) | 23/23/24/24/24 (118) |
| KR-K mean at 30,000 (ceiling 29.5, recorded) | 26.4 | 27.7 | 30.7 | 26.4 | 26.4 |

G-C's and G-D's floor rows are identical to the defaults' to the decimal:
no node of a pawnless mate drive has a raw evaluation of 0, so neither
guard touches those trees. The `test_endgames` runs at 120,000 nodes exit
1 on the defaults, G-C and G-D for KQ-K's mean 10.7 against the test's
staleness ratio, as Return 4 recorded; G-B's 30,000-node run exits 1 on
KR-K's 30.7 against the 29.5 ceiling. Both are means recorded, not judged
(amendment 3 §4); the floors hold everywhere.

**P16 scored:** G-B holds the 28 — hit (60%); G-C holds — hit (80%); G-D
holds — hit (90%); every guard holds the floors and the fixture — hit
(70%; G-B's fixture is +336, positive); G-B's WAC within 3 of 249 — **miss**
(242, −7; 55%). The miss is in the expected direction: G-B discards every
fail-low exclusion update, three quarters of the population on these
positions, and the tactical read moves with it.

**3.3 Does a guard fall under §10's non-goal on the correction form?**
§10 keeps "the correction form and weights" untouched: the tables, their
keys, the update rule, the weights, and `corrected_eval`. The training's
*admission* — which nodes train — is the seam ticket 7 already edits
(`B3CorrDecisive`, `B3CorrExcluded`); G-B and G-C are admission clauses
on that seam and change nothing on the OFF arm (G-C's OFF read is
16,615,757; G-B's OFF clause is dead by `train_excluded`). G-D changes
the consumer on both arms: it is the form, it is the non-goal, and it is
the right repair for a defect that exists without Cluster 2 — so it goes
to its own leaf (§3.4), not into B.3.1. Research reads G-B and G-C as
inside the non-goal's boundary; the maintainer owns that reading, and §5
gives the path if the reading is the other way.

**3.4 Recommendation.** **G-C as ticket 7b, with `B3CorrExcluded`'s
default kept at 1** and its value 1 defined as "trains at singular-excluded
nodes except where the node's raw static evaluation is 0". It is the only
candidate that holds every floor as written — the 28, the fixture's
positive score at 100,000 nodes (+661), the conversion floors (118), WAC
(250, the best read of the leaf so far) — while keeping the donor's
default for the paired run and the OFF arm's fingerprint; and it names the
mechanism exactly: the exclusion search may not charge a classification
with the value of a position the classification was never about.

*Alternative:* G-A (option 1, default 0). It holds the 28, costs 10 WAC
points against the defaults, and leaves the fixture at a7a8q with score 0
at 33,000–100,000 nodes (depth 11), positive only from 200,000; amendment
3's clause ("a8=Q with a positive score at 100,000 nodes") would have to
be reduced to the move, a floor relaxed after a read, which §10 forbids
and which research will not propose. If the maintainer reads §10's
non-goal as covering the admission seam too, G-A is the only path left,
and the fixture's clause then needs its own maintainer decision; that is
the second-best outcome and research says so plainly.

*The main counter-argument to G-C:* it is a Basilisk-only device with no
donor precedent, keyed to a proxy (`raw == 0`) for a recogniser verdict,
and it leaves the biased estimator of §1.8 live in every fortress the
evaluation does not classify: a drawn position the evaluation reads at
+0.3 with one holding move will still have its defender's correction
pushed toward a loss by every exclusion search. The answer is that this
bias is the donor's accepted form, read by §9 item 11's paired run as
contracted (`B3CorrExcluded` 1 against 0), and that where the evaluation
does classify, the classification must win — which is what G-C enforces
on the producer side now and what G-D will enforce on the consumer side
in its own leaf. G-B was the principled search-side alternative; it holds
everything too, but at WAC 242 and `bench 13` 6.3M it is a different
mechanism from the donor's rather than the donor's with a guard, and the
paired run would then not be reading B-15.

*Where G-D belongs.* A correctness leaf of its own in Phase B's `ANY
TIME` class, like B.12: the defect is the composition order recogniser →
correction on both arms, measured here as +90 on the OFF arm from one
position (R2a) and 4 of 28 after priming (R2b); the candidate is G-D; its
fixtures are the primed 28 and the organic FEN at depths 8–14 (both fail
on today's head); its gate is B.12's — a recorded fingerprint move, CTest,
the floors at five budgets, a repair SPRT `[-5,5]` at `3+0.03`. Research
recommends the deadline "before C.0", because C.0 measures evaluation
residuals by cohort on the B.9 head and a correction that overwrites
recognised draws would be read as evaluation error in the endgame
cohorts. It is also PLAN's C.5.2 E7 / C.10 ablation, already registered
by the 2026-10-27 audit; this amendment gives that entry its measured
defect and its fixtures. The finding sits beside amendment 3's KRP–KPP
finding (§5, research decision 4) in the evaluation programme's list, and
unlike that one it has an owner candidate. Opening the leaf is the
maintainer's decision; it does not block B.3.1.

### 4. Clauses changed, predictions and calibration (question 4)

Clauses of §9, §10 and amendments 1–3 this amendment changes, on the
maintainer's decision 1 (§5):

1. **§9 item 11** (`B3CorrExcluded`): values "1 trains at singular-excluded
   nodes (the donor) *except at a node whose raw static evaluation is 0*
   (amendment 4's guard); 0 refused"; default 1 unchanged; the read
   unchanged (paired run, last; B-15, low stakes; ±5).
2. **§4's B-14/B-15 row**: "B-15 the categorical `B3CorrExcluded`,
   guarded at a raw evaluation of 0 (amendment 4)".
3. **§10 *Producer → stored state → consumer***: new producer
   `ss->raw_eval` (the raw static evaluation of the outer pass, written on
   both arms beside `ss->eval`, read by the ON arm's training block
   only). The clause "new stack fields the OFF arm leaves at their
   defaults" reads "leaves at their defaults or writes without reading".
4. **§10 ticket 7** → the amended ticket 7b text of §5.
5. **§10 *Invariants***: unchanged — the wrong-bishop draws bind both arms
   at every ticket — with the note that the test's form (fresh tables per
   search) is what binds; the primed 28 and the organic FEN are recorded
   on both arms in ticket 10's table and bind nothing in B.3.1 (both fail
   on the OFF arm today, §2).
6. **§10 *Non-goals***: "the correction form and weights" is stated as the
   tables, keys, update rule, weights and `corrected_eval`; the training's
   admission at exclusion nodes is ticket 7's seam. The consumer guard
   (G-D) is named as the evaluation programme's / a correctness leaf's,
   not B.3.1's.
7. **§10 ticket 10's two-arm table** gains two recorded rows on both arms:
   the organic FEN `5k2/8/6KP/8/8/8/8/5B2 b` at depths 8–14 under 200,000
   nodes (Hash 8), and the primed wrong-bishop 28 (R2b's recipe, Hash 8).
8. **§10 *Cheap qualification***: "the wrong-bishop draws" reads "the
   wrong-bishop draws (the test's form; the primed and organic reads
   recorded)".
9. **§9's attribution table, `B3CorrDecisive`**: unchanged; its innocence
   is on record (§1.7), and its 2×2 with `B3CorrExcluded` on the fixture
   is recorded in §5 for B.3.2's ablation order.
10. **Amendment 3 §5, the fixture clause**: unchanged ("a8=Q with a
    positive score at 100,000 nodes"); G-C reads +661. The clause is the
    reason G-A is the alternative and not the recommendation.
11. **Amendment 1 §7's per-ticket method**: the `trace.patch` records
    (`a4x`, `a4n`, `a4q`, `a4y`, `a4it`) are a reusable temporary
    instrument for any correction question; they stay a patch, not a Diag
    feature, until B.3.2 asks for one.

Tickets 8–10 need no change of substance: ticket 10's table gains the
two rows above and its implementation record names the guard among the
deviations with this amendment as its reason.

**Predictions scored** (frozen in §R before the reads): P15a — OFF 7/7:
**miss** (6/7, −90 at depth 11; 65%); ON fails at least one depth: hit
(−152 at depth 14; 70%). P15b — primed OFF exposed at ≥ 4 of 28: hit
(exactly 4; 75%). P16 — four hits and one miss (G-B's WAC), §3.2. The
calibration note on R2a′'s wording is in §R and §2.2. Lesson for the
model: research predicted the OFF arm safe because its producer never
fires in the drawn region, and forgot that the region's boundary — the
position one move before the recogniser fires — is an ordinary-evaluation
node whose search value is the draw, the strongest producer of all.

**New prediction, frozen now.** **P17:** B.3.2's paired run of §9 item 11
(`B3CorrExcluded` 1 against 0 on the ON arm, G-C in place) reads within
±5 Elo — probability 60%; a read ≤ −5 says the biased estimator costs
more than the donor's tactics gain on this evaluation and flips the
default to 0 by §9's rule, no return. **P18:** the G-D leaf's repair SPRT
`[-5,5]` at `3+0.03`, if opened, does not read H0 at −5 (the fixtures
occur too rarely in games to cost Elo; the gain is correctness) —
probability 75%.

### 5. Decisions, recommendations, the amended ticket, the condition

**Research decides (its own):**

1. The route is confirmed (§1) with the trace; Return 4's hypothesis
   stands with one correction: the drift is spread over every table the
   training touches, not the pawn key alone.
2. P15a, P15b and P16 are scored; P17 and P18 are registered (§4).
3. `B3CorrDecisive` is innocent and its row stands; the fixture's 2×2 on
   the Tune build is recorded for B.3.2's ablation order:
   (`B3CorrDecisive`, `B3CorrExcluded`) = (0, 0) a7a8q +956 at depth 10
   (ticket 6b's read); (0, 1) +481 at depth 13; (1, 0) **0** at depths
   9–11; (1, 1) +747 at depth 10. The fixture is a knife-edge in the
   correction's settings as amendment 3 §1.3 said of the OFF arm's;
   `B3CorrDecisive` alone, without exclusion training, is the setting
   that drops it to 0, and that is a B.3.2 question, not a floor failure
   of the recommended tree.
4. The composition defect — the correction added to a recognised draw, on
   both arms — is recorded for a correctness leaf with G-D as its
   candidate and the primed 28 and the organic FEN as its fixtures
   (§3.4), beside amendment 3's KRP–KPP finding; the two recorded rows
   join ticket 10's table so B.3.1's final state carries the defect's
   measurement on both arms.

**The maintainer decides, each with its alternative and its main
counter-argument:**

1. **Ticket 7b takes G-C** — the raw-evaluation guard on the exclusion
   producer, `B3CorrExcluded` default 1 with the guarded semantics — as
   §3.4 recommends. Alternative: G-A (default 0) with amendment 3's
   fixture clause reduced to the move at 100,000 nodes and the positive
   score at 200,000, a relaxation research does not propose.
   Counter-argument: a Basilisk-only guard on a proxy for the recogniser,
   with the biased estimator left live elsewhere; answered in §3.4 (the
   bias is the donor's form and the paired run reads it; the consumer
   repair that covers every case has its own leaf).
2. **A correctness leaf for the consumer guard (G-D)** in the `ANY TIME`
   class, deadline before C.0, with the fixtures and gate of §3.4.
   Alternative: leave it to C.5.2's E7 switch / C.10's ablation in Phase
   C as PLAN already plans, with this amendment's measurements attached.
   Counter-argument: a leaf now costs an SPRT and a fingerprint move while
   B.3 is open; answered by the defect's class — a false score on a
   recognised draw is a theory-truth matter on the OFF arm's own play,
   and C.0's cohort residuals would otherwise read it as evaluation
   error. This decision does not block B.3.1.

**The amended ticket text implementation resumes on** (replacing §10's
ticket 7 paragraph and the resumption brief's):

- *Ticket 7b* (on `6007ab5`'s tree). `SearchStack` gains
  `int raw_eval = VALUE_NONE`, written beside `ss->eval` from
  `raw_static_eval` in the static-evaluation block's normal branch (both
  arms write it; only the ON arm reads it); the training block's
  exclusion clause becomes `(ss->excluded == MOVE_NONE || (train_excluded
  && ss->raw_eval != 0))`; `B3CorrExcluded` keeps default 1 with §9 item
  11's amended semantics, 0 the refusal; `B3CorrDecisive` and
  `CoreHistoryFills` unchanged. Tests in `test_b3_proof`: the ON arm reads
  |score| ≤ 75 on `7k/8/6KP/8/8/8/8/5B2 w` at depth 12 within 200,000
  nodes with `B3CorrExcluded` 1 (the ticket-7 tree reads 259 and fails
  it) and at 0; the OFF arm's tree on that position is unchanged by the
  switch. Reads at the ticket's end, on the committed tree: OFF
  `bench 13` 16,615,757 exact (the guard's OFF read); ON recorded (the
  guard's read 3,498,408); release CTest on both arms (`test_endgames`
  121/121 on the ON arm); WAC at 100,000 nodes ≥ 232 (read 250) and
  depth 6 recorded (134); the fixture a7a8q with a positive score at
  100,000 nodes (read +661) and its budget series recorded; the
  conversion floors at five budgets (read KQ/KR/KBB 24, KBN-K 118); the
  per-ticket counter table against ticket 6b; and, recorded on both arms,
  the organic FEN at depths 8–14 and the primed 28. Outside any binding
  read, Return 5 with the counters, no default moved by hand.
- *Tickets 8–10* as packet §10 with amendments 1–4: ticket 10's two-arm
  table carries the two recorded rows of §4 item 7 beside the fixture's
  budget series, KR-K's 30,000-node mean and BAS-R07's rec1; the
  implementation record names the guard among the deviations with this
  amendment as the reason; the cost screen as re-scoped (idle host, ask
  first; P11 its prediction); the depth-6 floor re-based on the ON arm.

**Condition for resuming:** the maintainer's decision 1 above (G-C, or
its alternative with the fixture clause decided). B.3.1 stays `RESEARCH`
until then. Decision 2 is independent of B.3.1.

**Maintainer decision, 2026-10-31, after the amendment:** decision 1 taken
as recommended. Ticket 7b takes G-C: no correction training at a
singular-excluded node whose raw static evaluation is 0, carried in
`ss->raw_eval`; `B3CorrExcluded` keeps default 1 with §9 item 11's amended
semantics. The amended ticket 7b text above is the contract implementation
resumes on; B.3.1 resumes `READY_FOR_IMPLEMENTATION`. Decision 2 taken the
same day: the consumer guard is PLAN leaf **B.14** (`I2`, ANY TIME, before
C.0) with the two fixtures and B.12's gate; it does not block B.3.1.

### 6. Artifacts (`tools/results/b31-20261030/amend4/`, ignored storage)

| Artifact | Content |
|---|---|
| `trace.patch`, `gb.patch`, `gc.patch`, `gd.patch`, `gc_off.patch` | the temporary edits, all reverted (`git status` clean after each build) |
| `scripts/` | `a4lib.py`, `variant.py`, `a4_trace_patch.py`, `a4_r1.py`, `a4_r2.py`, `a4_guards.py`, `a4_price.py` |
| `r1_wb0_d11.log`, `r1_wb1_d11.log`, `r1_wb0_d12.log`, `r1_analysis.txt` | R1's full engine output with the `a4x/a4n/a4q/a4it` records and the condensed analysis |
| `r2.json`, `r2.txt`, `r2a_off_trace_d11.log` | R2a (both arms), R2b (primed and control), R2a′ (the OFF trace with `a4y`) |
| `price_defaults_ce0_gb_gc_gd.json`, `price_run.log`, `conv_<build>_<budget>.log`, `endgames_<build>.log` | R3's prices |
| `ctest_gc.log` | release CTest on the G-C tree: 25/25 |
| `D:/code/stockfish`, `git show sf_19:src/evaluate.cpp`, `sf_19:src/search.cpp` | the donor's evaluation (no recognised draw) and its correction blocks (`:85-130`, `:824-856`, `:1280-1287`, `:1631-1638`) |

Builds: `build/a4-on-trace`, `a4-off-trace`, `a4-gb`, `a4-gc`, `a4-gd`,
`a4-gc-off` (release, `-DUSE_PEXT=ON -DPORTABLE_BUILD=ON`, B3_PROOF as
named), the ticket-7 trees `build/b3-on`, `b3-off`, `b3-on-tune` and
Return 4's `t7-ce0` (bench 4,372,801 reproduced).

## Return 5 — B.3.1 ticket 8: the upcoming-repetition test is a cutoff, and the registered terminal tests read it as a score (2026-10-31)

**What resumed.** Amendment 4's ticket 7b landed and passed every binding
read (`920cc5d`). Ticket 8 was built as §10 wrote it. Its two pre-registered
terminal tests cannot be written as registered. In the donor's form the
mechanism never puts a draw score on a principal variation, so a fixed-depth
root search does not read 0 where the registered tests expect it.

**Ticket 7b** (`920cc5d`). `SearchStack::raw_eval` is written beside
`ss->eval` in the normal branch, and the training block's exclusion clause
is amendment 4's `(ss->excluded == MOVE_NONE || (train_excluded &&
ss->raw_eval != 0))`. The test is in `test_b3_proof`: the wrong-bishop FEN
at depth 12 under 200,000 nodes at Hash 8, both switch settings. Its reads,
on the committed tree (`tools/results/b31-20261030/t7b/`, `gates.txt`):

| Read | Result | Rule |
|---|---|---|
| OFF `bench 13` | 16,615,757 (Release and Diag) | exact |
| ON `bench 13` | **3,498,408** (Release = Diag) | recorded (amendment 4 read 3,498,408) |
| Release CTest | 25/25 on both arms; ON `test_endgames` 121/121 | — |
| ON Diag `test_stack_unwind`, `test_b3_proof`, `test_search` | pass | — |
| WAC | **250** at 100,000 nodes; 134 at depth 6 | ≥ 232; recorded |
| The fixture a7a8q | +94 / +661 / **+661** / +926 / +1,180 / +1,180 at 33k / 60k / 100k / 200k / 400k / 1M nodes | a8=Q, positive at 100k |
| Conversion floors | KQ-K, KR-K, KBB-K 24 at every budget; KBN-K 23/23/24/24/24 = **118** | ≥ 112 |
| KR-K mean at 30,000 | 26.4 | recorded |
| Wrong-bishop 28 (the test's form) | 28/28 | binds |
| Organic `5k2/8/6KP/8/8/8/8/5B2 b`, depths 8–14 | OFF 6/7 (−90 at depth 11); ON 7/7 | recorded |
| Primed 28 (R2b's recipe) | OFF 24/28; ON 27/28 (+86 at depth 8 on the first FEN); controls 28/28 | recorded |
| Counters against ticket 6b | interior 1.23×; Cluster 1 over 20%: `cont_hist_pruned` +32.7% (as ticket 7) | recorded |

The organic and primed rows are amendment 4's finding on the OFF arm,
now B.14's; they bind nothing here.

**Ticket 8 as built** (`7e92ecb`):

- **The tables.** `Zobrist::Cuckoo` and `CuckooMove` (8,192 slots, the
  donor's `H1`/`H2`) are built in `Zobrist::init()` from the empty-board
  geometry and asserted complete (3,668 moves).
- **The repetition test.** `Board::upcoming_repetition(ply)` is the donor's
  loop over the key history. Its bound is `min(rule50, plies_from_null,
  history)`; it applies the between-squares occupancy test, and at or
  before the root it asks for a position already met again four or more
  plies before.
- **The two consumers, behind `B3Cuckoo` (S line, default 1).**
  - Negamax, after `is_draw(ply)` and before mate-distance pruning (and
    before the hand-off to quiescence).
  - Quiescence, after its draw check, on nodes it was not handed.
  - Each runs only when `alpha < 0`. It raises alpha to 0 and returns 0
    when that reaches beta. It counts `cuckoo_draws` and traces
    `CuckooDraw`.
- **Tests in `test_b3_proof`, both arms.**
  - Every reversible move's key is found, checked by the attack tables.
  - No false positive on 10,000 random-walk positions against a slow
    reference that makes every legal reversible move. The walks undo a move
    from two plies back half the time, and the test probes at search plies
    0, 2, 5 and 64. Result: 9,915 positive reads, no false positive and no
    miss.

Its reads:

| Read | Result | Rule |
|---|---|---|
| OFF `bench 13` | 16,615,757 (Release and Diag) | exact |
| ON `bench 13` | **5,635,655** (Release = Diag) | recorded |
| Release CTest | 25/25 on both arms (`test_b3_proof` 89/89; `test_endgames` 121/121; `test_search`'s fixture) | — |
| ON Diag `test_stack_unwind`, `test_b3_proof`, `test_search` | pass | — |
| WAC | **243** at 100,000 nodes; 132 at depth 6 | ≥ 232; recorded (−7 and −2: under the attribution rule's 10) |
| The fixture a7a8q | +315 / +315 / **+913** / +1,084 / +1,255 / +1,324 at 33k / 60k / 100k / 200k / 400k / 1M nodes | positive at 100k |
| Conversion floors | KQ-K, KR-K, KBB-K 24 at every budget; KBN-K 24/23/24/24/24 = **119** | ≥ 112 |
| Wrong-bishop 28 (the test's form) | 28/28 | binds |
| Organic FEN / primed 28 | OFF 6/7, 24/28; ON 7/7, 25/28 (+95, +113 on the first FEN, +347 at depth 8 on the reversed one); controls 28/28 | recorded |
| Counters against ticket 7b | interior 0.88×; `cuckoo_draws` 0.87% of interior nodes; no Cluster 1 row over 20% | recorded |

**The premise.** §10 ticket 8 registers, for BAS-S12's trigger:

- (a) a position, reached by a pre-root move list, in which the side to move
  is lost by evaluation and by a depth-2 search but has a reversible move
  completing a repetition. It should read **0** at `go depth 2` on the ON
  arm and the loss on the OFF arm.
- (b) a losing bound stored in the TT from a path without the repetition,
  pre-empted by the draw on a path with it. It should read **0** at
  `go depth 1` on the ON arm.

Both treat the test as a rule that scores a node as a draw. The donor's form
(`search.cpp:736-742`, `:1661-1667`), which the ticket specifies and which
is built here, is a **cutoff**:

- It raises `alpha` to the draw and returns only when that reaches `beta`.
- Otherwise the node searches on, and fail-soft returns its own best value,
  which may lie below the raised alpha.

The consequence for a fixed-depth root read:

- A return happens only at a node whose `beta ≤ 0` for the drawing side. Its
  value is a lower bound that fails low in the parent, so the parent rejects
  that move. A node on the final, exact principal variation is never such a
  node.
- Basilisk searches depths 1–3 with the full window (`search_worker.cpp`:
  `depth <= 3`). Along that variation the drawing side's `beta` comes from
  the opponent's best alternative, so it is positive or infinite.
- So the mechanism changes which lines the search rejects. It never makes a
  draw the root's score. A root score of 0 at `go depth 1` or `2` has to come
  from `is_draw` itself, and then it comes on the OFF arm too.

The read (`t8/terminal_reads.py`, `terminal_reads.txt`) uses this position:

- FEN `rb6/p1p2K1k/p1p1p3/p1p1p3/P1P1P1N1/8/8/8 b - - 0 1` with the moves
  `h7h8 f7f8 h8h7 f8f7 h7h8`.
- White is lost by material: Black's eight blocked pawns and a trapped rook
  and bishop.
- Black's king is held to h8/h7. After the root's `f7f8 h8h7`, White's
  `f8f7` reaches a position met twice before (a three-fold).
- At the depth-2 node White has that move, and `upcoming_repetition(2)` is
  true there.

| Read (`go depth N`, Hash 8, fresh) | d1 | d2 | d3 | d4 | d5 | d6 |
|---|---:|---:|---:|---:|---:|---:|
| ON, `B3Cuckoo` 1 | −1,114 | **−935** | −935 | −730 | −730 | −613 |
| ON, `B3Cuckoo` 0 | −1,114 | −935 | −935 | −730 | −730 | −615 |
| OFF | −1,114 | −935 | −935 | −730 | −730 | −659 |
| ON, `searchmoves f7f8` | −1,276 | −190 | −127 | −13 | 0 | 0 |
| ON, `searchmoves f7f8`, `B3Cuckoo` 0 | −1,276 | −1,157 | −1,157 | −972 | — | — |
| OFF, `searchmoves f7f8` | −1,276 | −1,157 | −1,157 | −972 | 0 | 0 |

Every full search plays `g4e5` and reads the loss on all three settings, so
(a) as registered fails on the ON arm. The ON Diag build counts
`cuckoo_draws` 0 at depths 2 and 4 and 10 at depth 6. Under the root's null
window the line `f7f8` is cut at Black's node by reverse futility before it
reaches White's node, so the test never runs where it would act.

Restricted to `f7f8` the test fires at depth 2 (`cuckoo_draws` 1), and the
root reads **−190**, which is neither the draw nor the loss. Quiescence's
delta bound produces it: with alpha raised to 0, `stand_pat + queen + 200 <
alpha` returns that optimistic upper bound, and the PV parent takes it as
exact. This is a separate finding, reported below and not repaired.

**Options.**

1. *Register the terminal tests as the cutoff they are* (recommended). Drive
   the node with a window whose `beta ≤ 0` for the drawing side.
   - The test: in a kernel-driving test as `test_stack_unwind`, or a root
     search of the position after the pre-root list restricted by
     `searchmoves` to a move whose null-window search reaches the node.
   - The ON arm returns 0 (a cutoff) and the OFF arm the loss or its bound.
   - Two cases stay as registered: `alpha ≥ 0` is never touched, and with
     `B3Cuckoo=0` the node is the OFF arm's.
   - (b)'s TT pre-emption becomes the same read at a non-PV node holding a
     losing TT bound, where the cutoff returns 0 before the TT probe would
     return the bound.
2. *Keep a score test, at the root of a winning side.* The cutoff steers the
   winning side away from a line where the defender has a reversible move to
   a repetition. The read is the best move rather than a score, but it
   depends on the root's move order (the move that allows the repetition
   must not be searched first), which the test would have to pin.
3. *Read the mechanism by its counters and the paired run only.*
   - Use `cuckoo_draws`, the reference test above, and §9 item 9's paired
     run.
   - Drop the terminal tests.
   - It loses BAS-S12's trigger as a deterministic fixture.

Recommendation: option 1. It tests the construct at the node where it acts
and binds both arms. Option 2's dependence on move order makes it a test of
the picker. The ticket's mechanism, tables, reference test and switch are
committed as built. Because the registered fixture cannot be written as
registered, this returns the leaf rather than letting implementation choose
a replacement experiment. No default was moved.

**A separate finding, not repaired.** Quiescence's delta bound
(`search_kernel_core.cpp`, the `prune_by_gain && delta_bound < alpha`
return) returns an optimistic upper bound below alpha. At a PV node whose
alpha was raised inside the node, the parent cannot tell that this value is
a bound, and it takes it as exact. The cuckoo is the first producer that
raises alpha far above the stand-pat at a PV node. On the read above it
puts −190, −127 and −13 on the root at depths 2–4, values no line
produced. Research decides whether this belongs to ticket 8's contract, for
example the donor's quiescence, which returns no delta bound, or to a
separate leaf.

**State at the return.**

- Tickets 0–7b are committed (`920cc5d`), and ticket 8's implementation is
  committed (`7e92ecb`) without the two terminal tests.
- The OFF arm is 16,615,757 with release CTest 25/25 at every commit.
- Tickets 9 and 10 are not started.
- No idle-host read was run.

## Research amendment 5 — ticket 8's terminal test is a cutoff read at the draw window (2026-10-31)

Written on Return 5, at `36107e6`, class `R2`: no ticket implemented and
no default moved. The reads are node-count reads at depths 1–3 and ran on
the busy host. The instrument is a test-only root-window seam (below),
built into the release trees and run through a temporary section of
`test_b3_proof`. The patches and outputs are in
`tools/results/b31-20261030/amend5/`.

### 0. What this run says, plainly

Return 5 is right. In the donor's form the upcoming-repetition test is a
cutoff. A whole search reads it only where a node is searched with
`beta ≤ 0` for the side that can repeat. On a non-PV node that is always
true when the test can fire: a null window with `alpha < 0` has
`beta ≤ 0`. On a PV node it is true only under a narrow root window. The
registered tests (a) and (b) read it on an open-window PV, where the node
raises alpha and then searches on, so they could not pass as written.

The construct can still be read through the root, deterministically and
in release CTest on both arms. Return 5's fixture needs one more ply, so
the node that can repeat is the root's child. The root is then searched
under a null window at the draw, (0, 1), the question every null-window
search asks: is this move better than a draw? The ON arm reads exactly
**0** at depths 1, 2 and 3. The same arm with `B3Cuckoo` 0, and the OFF
arm, read the win. Under the window one below, (−1, 0), the repeating
node's alpha is 0, and the switch moves neither the score nor the node
count. That is BAS-S12's one terminal test.

Test (b), the TT pre-emption, is withdrawn as a whole-search test. A
stored bound can cut only at a non-PV node, and the fixture's line has a
single reply at every ply, so every node on it is the first child of a PV
node (R3). The pre-emption follows from code order, because the cut
returns before the probe, and the review checks it. Return 5's delta-bound
finding belongs to a PV node that raises alpha without cutting. It is not
ticket 8's, and it is recorded for B.4.

### R. Reads registered before running

The instrument is `SearchLimits::root_alpha` and `root_beta`. When the two
differ they fix every iteration's root window. They follow the precedent
of `ext_budget_cap`, a test seam on the same struct that the driver
otherwise ignores. Fresh tables and Hash 16 are used per read; the
release ON and OFF trees are both read; `B3Cuckoo` is 1 and 0.

The registration, frozen before R1 (`amend5_R.md`):

- **R1** is Return 5's fixture, `rb6/p1p2K1k/p1p1p3/p1p1p3/P1P1P1N1/8/8/8 b - - 0 1`
  with `h7h8 f7f8 h8h7 f8f7 h7h8` and `searchmoves f7f8`, read at depth 2
  under (−2000, −1999). **P19:** ON reads 0; ON with `B3Cuckoo` 0 and OFF
  read the loss. 85%.
- **R2** is the same fixture under (0, 1). **P20:** `B3Cuckoo` 1 and 0
  give the same score and nodes. 75%.
- **R3** first fills the TT by searching the ply-2 position from its FEN to
  depth 6, then reads depth 3 under R1's window. **P21:** ON reads 0,
  `B3Cuckoo` 0 reads a stored loss, and OFF reads as `B3Cuckoo` 0. 60%.

Rule: a test is written only in a form whose read matched its prediction
on both arms. A miss is recorded and the form is re-chosen here.

R4 and R5 were registered after R1–R3 and before they ran. They use the
fixture plus `f7f8`. Black is to move and has one legal move, `h8h7`.
White's node at ply 1 has `f8f7`, which reaches a position seen twice;
`upcoming_repetition(1)` reads 1 through the prior repetition.

- **R4** reads under (0, 1) at depths 1–3. **P22:** ON reads exactly 0;
  ON with `B3Cuckoo` 0 and OFF read above 0. 80%.
- **R5** reads under (−1, 0). **P23:** ON with `B3Cuckoo` 1 and 0 read
  the same score and node count. 80%.

| Read | ON, `B3Cuckoo` 1 | ON, `B3Cuckoo` 0 | OFF (either) |
|---|---|---|---|
| R1, depth 2 | −666, 5 nodes | −1,526, 5 | −1,526, 5 |
| R2, depth 2 | −95, 5 | −95, 5 | −95, 5 |
| R3, depth 3, prefilled (unprefilled) | −499 (−499) | −1,927 (−1,683) | −1,939 (−1,683) |
| R4 (0, 1), depths 1 / 2 / 3 | **0 / 0 / 0**, 2 / 4 / 6 nodes | 95 / 130 / 73, 2 / 7 / 12 | 95 / 65 / 340, 2 / 7 / 14 |
| R5 (−1, 0), depths 1 / 2 / 3 | 95 / 130 / 72, 2 / 7 / 12 | 95 / 130 / 72, 2 / 7 / 12 | 95 / 64 / 339, 2 / 7 / 14 |

The predictions scored:

- **P19 missed in form, though the direction held.** The root softens a
  fail-high value to `(v·d + beta)/(d + 1)`. −666 is the draw softened at
  depth 2; −1,526 is the loss (about −1,290 raw) softened the same way.
  The cutoff fired. The read cannot show a 0, because a root that fails
  high never reports its raw value.
- **P20 hit.**
- **P21 missed.** With one reply at every ply, every node on the line is
  a PV first child, so the TT never cuts there. On ON, the prefill moved
  nothing (−499 either way).
- **P22 hit.** The fail-low root reports its raw 0. The positive reads
  are softened fail-highs: 95 at depth 1 is the delta bound's −190 seen
  from White's side, a correct upper bound here.
- **P23 hit.** The OFF arm also reads the same with the switch at 1 and
  0, as a compiled-out mechanism must.

Calibration: research predicted R1 from the cutoff's logic and forgot the
root's own fail-high adjustment. That adjustment is Cluster 1's, adopted in
B.2, and it is a consumer of every root read. The lesson: a root-score
test must put the root's verdict on the fail-low side, where the value is
reported raw.

### 1. The terminal test (question 1)

Option 1 of Return 5 is taken, in the form R4 measured: a whole-search
read at the root under a null window at the draw. A kernel probe was not
used. Basilisk has no seam that runs one node with a chosen window, and
building one means rebuilding the driver's per-search set-up outside
`search()`. The root window gets the node the same window with a
two-line driver branch the default leaves dead.

Option 2, a best-move read, is not taken, for Return 5's reason: it tests
the root's move order.

Option 3 is not taken either. The counters alone cannot fail where the
logic is wrong but the tree happens to shrink.

**(b) is withdrawn as a separate test.**
- **Why it can't be built here:** the consumer is the TT probe at a
  non-PV node with `alpha < 0` and `beta ≤ 0`. Driving one needs a parent
  with at least two moves whose order the test controls (a PV first child
  never takes a TT cut, as R3 shows). That brings back option 2's move
  order dependence.
- **How the pre-emption is checked instead:**
  - by code order: the cut returns before the TT probe and before
    mate-distance pruning, on both node types;
  - by the review, which reads that order in ticket 8's diff;
  - by B.3.2's paired run, which reads the whole consumer. Every non-PV
    firing is a cut, so `cuckoo_draws` at non-PV nodes counts the
    pre-emptions.
- **BAS-S12:** its trigger asks for one terminal test, and R4 is it.

### 2. Return 5's delta-bound finding (question 2)

What the arm does at a PV node:

- The cuckoo raises alpha to 0 at a PV node that does not cut
  (`alpha < 0 < beta`), and then searches on.
- At depth ≥ 1 the repeating move is itself searched, and its child is
  `is_draw`, so the node returns at least 0 unless that move is pruned.
- At depth 0, quiescence has no quiet move to play. It returns a
  fail-soft value below the raised alpha, and the PV parent takes that
  value as exact:
  - the stand-pat, on the donor's form;
  - the delta bound `stand_pat + Q + 200`, on Basilisk's.

**The donor:** its quiescence also returns a value below the raised alpha
there (`sf_19` qsearch: `alpha = VALUE_DRAW`, then `bestValue` fail-soft),
which is the stand-pat or the best capture, about the loss. Basilisk's
reads −190 on Return 5's line. Neither reads the true 0.

**Scope:**
- **Only PV nodes.** A non-PV node with `alpha < 0` has `beta ≤ 0` and
  always cuts.
- **The PV ranks a value no line produced.** The line is ranked by the
  loss on the donor's form, or by the delta bound on Basilisk's. It is
  never ranked by the draw.

**Decision:**
- **Not a B.3.1 matter.** Ticket 8 stays the donor's form.
- **The finding is recorded for B.4,** whose surface is quiescence's
  delta bound and stand-pat: a quiescence node whose alpha was raised by
  a lower bound it knows (here the draw) returns that bound, not a value
  below it. It is the donor-divergent candidate if B.4 wants one.
- **B.3.2 records it:** the reported score on Return 5's line at depths
  2–6 beside the counters, so B.4 starts with a measured case.

No new prediction is registered; the finding has no gate in B.3.

### 3. The seam (question 3, implementation's)

`SearchLimits` gains `int root_alpha = 0, root_beta = 0`. The driver's
iteration searches the root under that window at every depth when the two
differ. No other driver path reads them. The default is dead, so it is
behaviour-neutral: the OFF arm stays at 16,615,757, and so does the ON
arm's fingerprint. The seam belongs to the driver, which both arms share,
and it lands in ticket 8's commit beside the test.

### 4. Clauses changed

1. **§10 ticket 8, terminal tests.** "(a) … reads 0 at `go depth 2` …
   (b) … read as 0 at `go depth 1`" → the following, both arms, in
   release CTest:
   - The fixture: `rb6/p1p2K1k/p1p1p3/p1p1p3/P1P1P1N1/8/8/8 b - - 0 1`
     with `h7h8 f7f8 h8h7 f8f7 h7h8 f7f8`, root `searchmoves h8h7`, fresh
     tables per read.
   - Under the root window (0, 1) at depths 1, 2 and 3, the ON arm reads
     **0**. The ON arm with `B3Cuckoo` 0, and the OFF arm, read **> 0**.
   - Under (−1, 0) at the same depths, `B3Cuckoo` 1 and 0 give equal
     score and nodes on each arm.
   - `upcoming_repetition(1)` is true at White's ply-1 node.
   - (b) is withdrawn, as §1 says.
2. **§5.8**: "the node returns 0 on `alpha ≥ beta`" gains: "It never puts
   the draw on a PV it does not cut; a PV node that raises alpha and then
   searches on returns its own fail-soft value (amendment 5 §2)."
3. **§10 *Producer → stored state → consumer***: the seam
   `SearchLimits::root_alpha`/`root_beta` (§3), read by the driver alone.
4. **Return 5's options**: option 1 taken in R4's form.
5. **Ticket 10's two-arm table** gains a recorded row: Return 5's line
   (`searchmoves f7f8`, depths 1–6) on both arms, B.4's starting case.

**Tickets 9 and 10** stay as packet §10 with amendments 1–4. Ticket 8's
implementation record names the seam among its deviations, with this
amendment as the reason.

### 5. Decisions and the condition

Research decides (its own):

1. Return 5's option 1, in R4's form, with (b) withdrawn (§1).
2. The delta-bound finding goes to B.4 and is recorded in B.3.2 (§2).
3. P19–P23 are scored (§R).

There is no maintainer decision. The test's form is the meaning of an
experiment, which research owns.

**Condition for resuming:** none outstanding. B.3.1 resumes
`READY_FOR_IMPLEMENTATION` at ticket 8's terminal tests (the seam and the
tests, then the ticket's reads again), then tickets 9 and 10.

### 6. Artifacts (`tools/results/b31-20261030/amend5/`, ignored storage)

| Artifact | Content |
|---|---|
| `amend5_R.md` | the registration as frozen, with R1–R3 scored before R4–R5 ran |
| `seam.patch` | the root-window seam as read |
| `read.patch` | the temporary read section (R4–R5 form) |
| `r1_r3.txt`, `r4_r5.txt` | the reads, both arms, `B3Cuckoo` 1 and 0 |

## Return 6 — B.3.1 ticket 9: the fail-low store throws the promotion fixture through IIR, and with the penalty it costs a KR-K floor (2026-10-31)

**What resumed.** On amendment 5, ticket 8's terminal test landed
(`5b6bae1`). It uses the root-window seam, with the ON arm reading 0 at
depths 1–3 and the switch and the OFF arm reading the win. Its tree was
unchanged, with OFF 16,615,757 and ON 5,635,655, and release CTest passed
25/25 on both arms. Ticket 9 was then built as §10 writes it (`58ea1ed`).
Two of its binding reads fail on the contract's defaults.

**Ticket 9 as built** (`58ea1ed`):

- **The fail-low store.** `B3FailLowStore` is an S line, default 1. On a
  fail low (`TT_ALPHA`), negamax's final store passes no move, and the
  table keeps the entry's own move as it already did for a no-move store.
  Otherwise it stores the best-scored move, which there is the move that
  raised alpha. Quiescence already stored only a move that raised alpha.
- **The bound-mismatch penalty.** `B3TtPenalty` is an S line, default 1.
  It is the donor's `else if` on the TT cutoff:
  - the conditions are a non-PV node, no exclusion, a valid score, and
    `tt_depth > depth − (tt_score ≤ beta)`;
  - the bound must be on the wrong side, `TT_ALPHA` when `tt_score ≥ beta`
    and `TT_BETA` otherwise, at depth > 5;
  - the effect is `TranspositionTable::penalize(key)`, which lowers the
    entry's depth byte by one and nothing else. An entry at depth ≤ 0 is
    left alone. It counts `tt_penalized` and traces `TtPenalty`.
- **Tests.**
  - `test_tt` checks `penalize`: depth −1 with every other field kept,
    and depth 0, evaluation-only and missing entries unchanged.
  - `test_b3_proof` checks a root that fails low under the window
    (5000, 5001). Its entry holds no move with the store on the ON arm,
    and the best-scored move otherwise.
  - Both switches move the ON tree and not the OFF tree, and
    `tt_penalized` fires only with the penalty on.

| Read (ticket 9, both switches 1) | Result | Rule |
|---|---|---|
| OFF `bench 13` | 16,615,757 (Release and Diag) | exact |
| ON `bench 13` | **3,345,971** (Release = Diag) | recorded |
| Release CTest | OFF 25/25; ON **24/25**: `test_search`, "a queen where a rook only transposes", plays `h3h4` | — |
| WAC | 249 at 100,000 nodes; 133 at depth 6 | ≥ 232; recorded |
| The fixture a7a8q | h3h4 0 / h3h4 0 / **h3h4 0** / a7a8q +1,196 / +1,192 / +1,387 at 33k / 60k / 100k / 200k / 400k / 1M | a8=Q, positive at 100k: **fails** |
| Conversion floors | KQ-K, KBB-K 24 at every budget; **KR-K 23 at 30,000**; KBN-K 24/24/22/24/23 = 117 | KR-K 24 at every budget: **fails**; KBN-K ≥ 112 holds |
| Wrong-bishop 28 (the test's form); `test_endgames` | 28/28; 121/121 | binds |
| Organic FEN / primed 28 | OFF 6/7, 24/28; ON 6/7 (−111 at depth 13), 23/28 | recorded |
| Counters against ticket 8 | interior 0.90×; `tt_move_absent` 65.4% → 72.7%; `tt_penalized` 0.84%; `iir_pv_nodes` +195%; Cluster 1 over 20%: `cont_hist_pruned` +35.5% | recorded |

**The two switches separated.** Each variant is a temporary build with
one default flipped, then restored byte-exact; the patches are
`t9/t9_fls0.patch` and `t9_pen0.patch`. The fixture reads are on the
Tune build with the option.

| (`B3FailLowStore`, `B3TtPenalty`) | `bench 13` | WAC 100k | Fixture at 100k | KR-K at 30k | KBN-K sum |
|---|---:|---:|---|---:|---:|
| (0, 0), ticket 8 | 5,635,655 | 243 | a7a8q +913 | 24 | 119 |
| (1, 0) | 5,133,969 | 246 | **h3h4 0** | 24 | 114 |
| (0, 1) | 6,723,172 | 241 | a7a8q +918 | 24 | 120 |
| (1, 1), ticket 9 | 3,345,971 | 249 | **h3h4 0** | **23** | 117 |

- **The fixture fails whenever the store is on.** It is not a budget
  knife-edge: the store-on arm reads `h3h4` 0 at every budget from 50,000
  to 130,000 nodes in steps of 10,000, and a7a8q from 140,000 on. The
  store-off arm reads a7a8q at every one (`t9/fixture_sweep.txt`).
- **The KR-K miss needs both switches.** Each switch alone holds 24 at
  every budget.

**The carrier is IIR.** With `B3Iir` 0 and the store on, the fixture
reads a7a8q **+382** at 33k, 60k and 100k (`t9/fixture_iir.txt`).
- The store removes the TT move from every fail-low entry, so
  `tt_move_absent` rises 7.3 points of interior nodes.
- The ON arm's IIR reduces a PV or cut node of depth ≥ 6 that has no TT
  move. That is the donor's step 11 exactly (`!followPV && !allNode &&
  depth >= 6 && !ttMove`), and `iir_pv_nodes` triples.
- On the a7a8q line, the nodes whose entries lost their move are reduced
  a ply. The line reads ≤ 0 at depths 9–11 (a7a8q +506 at depth 8, then
  `h3h4` 107 and 0), and `h3h4` wins the root until 140,000 nodes.

Packet §5.6 predicted this population effect (the store "enlarges the
no-TT-move population, which IIR, razoring's and RFP's admission ... all
read"). It put the read in B.3.2. The fixture shows the effect inside
B.3.1's binding reads.

**The premise.** Ticket 9's contract carries `B3FailLowStore` and
`B3TtPenalty` at their §9 defaults, 1 and 1. On those defaults two binding
reads fail:
- amendment 3's fixture clause (a8=Q with a positive score at 100,000
  nodes);
- the conversion floor (KR-K 24 at every budget).

Both store and IIR are the donor's forms, so the ticket is built as
contracted. Implementation may not move a default, relax the fixture or
the floor, or change IIR. Either one of those changes, or the contract
holds as written and the binding reads fail, so the leaf returns.

**Options:**

1. *Land ticket 9 with `B3FailLowStore` 0* (the penalty at 1). Every
   binding read holds: fixture +918, floors 24, KBN-K 120, WAC 241.
   - §9 already registers the store's paired run ("ablation on a
     negative paired run"). B.3.2 would read 1 against 0 with IIR in the
     tree, and §5.6's population read (`tt_move_absent`, both settings)
     comes with it.
   - Cost: the ON arm carries B-12 unrepaired into B.3.2, and its bench
     tree is 6.72M, 1.19× ticket 8 and 2.0× ticket 9.
2. *Keep both defaults and re-read the fixture clause and the 30k floor.*
   The fixture has now flipped with several ON-arm settings: the
   correction 2×2 (amendment 4 §5.3), the store, and IIR. That says it reads the
   tree's composition at 100k as much as the conversion.
   - Amendment 4 declined exactly this relaxation, and the KR-K miss is a
     floor, not a fixture.
3. *Research the store × IIR composition*: the donor composes the same
   two, so why does the donor's own tree not lose this conversion?
   - This is the expensive option. It may end in a Basilisk-only IIR
     admission, which is a mechanism change.

Recommendation: option 1. It keeps every binding read without relaxing
anything, and it moves the store's verdict to the paired run §9 already
registered for it. Main counter-argument: B-12 is a defect, and option 1
defers its repair. The defect is "a move stored at fail-low", whose harm
is a wrong TT move. That is ordering noise, and the paired run prices it;
the repair's harm here is a lost won ending at 100k.

**State at the return.**

- Tickets 0–8 are complete. Ticket 9 is committed as built (`58ea1ed`),
  with the ON arm's CTest at 24/25 (the fixture).
- The OFF arm is 16,615,757 with release CTest 25/25 at every commit.
- Ticket 10 has not started. No idle-host read was run, and no default
  was moved.
- Evidence is in `tools/results/b31-20261030/t9/`: `gates.txt` is the check
  log, plus `t9_reads.log` and `.json`, `compare.md`, `fixture_sweep.txt`,
  `fixture_iir.txt`, `variants.txt` and the two patches. Builds are in
  `build/t9-fls0` and `build/t9-pen0`.

## Research amendment 6 — the fail-low store's population, the knife-edge fixture and ticket 9's defaults (2026-10-31)

Written on Return 6, at `6b0e4c4`, class `R2`: no ticket implemented, no
default moved, no floor or clause relaxed. Every read is a node-count or
depth read on the busy host; no game was played and no timed read was run.
Temporary builds went through `amend4/scripts/variant.py` (byte-exact
edits, restored, patches under `tools/results/b31-20261030/amend6/`); the
registration (`amend6_R.md`) was frozen before each read, with two
addenda registered before their reads ran.

### 0. What this run says, plainly

Return 6 is right that the store and IIR together lose the fixture, and
its framing ("the carrier is IIR") is too narrow. The whole-root trace
shows the fixture flips at iteration 9: the first root move a7a8q fails
low under its aspiration window, h3h4 takes the root, and from then on
a7a8q is searched second under a null window at h3h4's score and never
again deeply (its subtree fires no IIR at iterations 10–11 at all). At
the decisive iteration the store's own product — a fail-low entry without
a move, found at a node IIR then reduces — is 7 of the 83 IIR firings in
the a7a8q subtree. The other 76 fire on fresh nodes, evaluation-only
entries and quiescence stand-pat entries, which both settings produce.

What the store does is reshape the tree through every consumer of "no TT
move": IIR, the cut-node LMR term `CoreLmrCutNodeNoTt` and the first-move
step-down. Each of three single-constant changes restores a7a8q with the
store on — IIR off (+382), IIR's minimum depth 7 (+284 → +1,235), the LMR
term 0 (+365 → +983) — and each holds from 50,000 to 130,000 nodes. And
the fixture flips the other way under a root-window change: with
`searchmoves a7a8q` the store-on tree reads +952 at depth 11 and the
store-off tree reads **0** at depths 9–11. Amendment 3 §1.2 said the
fixture is knife-edge on both arms; this run says it is knife-edge in the
store's setting too, in both directions.

The KR-K miss at 30,000 nodes is the same kind of event, not a second
mechanism. The failing position is `4K3/8/8/8/8/5k2/8/1R6 w`, a cold
rook ending the (1, 1) tree shuffles for 100 plies; every one of six
single-setting changes converts it (the store off in 21 plies, the
penalty off in 45, IIR off in 69, IIR's minimum 7 in 69, 8 in 25, the LMR
term 0 in 23). Nothing in it is specific to the penalty beyond the
penalty being one more way to change the tree.

The donor composes exactly these forms (§2): no move stored after a fail
low, the entry keeping its move, IIR at depth ≥ 6 on PV and cut nodes off
the previous PV, `933·!ttMove` in the cut-node LMR term, the 1127
step-down, the evaluation-only and stand-pat stores without a move. There
is no structural difference around the store, IIR or the TT move. The
difference is calibration: the donor's consumers were fitted on the
population the store makes; Basilisk's were fitted (B.2, `CoreLmrCutNodeNoTt`
864) or carried (IIR at 6) on the population the OFF arm makes, where a
fail-low node keeps its best-scored move. B.3.3's surface already carries
both consumers (§5.6, `config_b3.json`), so the re-fit is planned; what
the arm carries into B.3.2 before that fit is the question.

**Recommendation (§4): Return 6's option 1.** `B3FailLowStore` default 0
for B.3.1 (the OFF arm's form: the best-scored move stored), the penalty
at 1, and the store read by one paired run in B.3.2 as §9 item 16 with
`tt_move_absent` on both settings. Every binding read holds on that tree
at every budget with margin (fixture +918, floors 24 × 20, KBN-K 120, WAC
241, `test_endgames` 121/121, the 28 draws); it keeps the consumers on
the population they were fitted for until B.3.3 refits them; and it
costs one default line, one test section and one paired run. Its price
is the tree: 6,723,172 at `bench 13`, 1.19× ticket 8 and 2.0× ticket 9,
which the cost screen will read. The alternative priced in full is
`B3IirMinDepth` seeded 7 with both switches at 1: it passes every binding
read too (fixture +284 → +1,235, floors 24 × 20, KBN-K 119, WAC 245,
5,486,508 nodes), keeps both donor forms, and is a seed moved on two
low-budget conversion reads with no mechanism behind the choice of 7
over 8 or the LMR term.

### R. Reads registered before running (`amend6_R.md`, frozen at `6b0e4c4`)

R1 the a7a8q subtree's IIR records under `searchmoves a7a8q` (P24); R1b,
registered after R1's result and before it ran, the same on the
whole-root search (P30); R2 the fixture by option on the Tune build (P25);
R2b, registered after R2, the 50,000–130,000 sweep for the recovering
settings (P29); R3 two release builds, IIR admitting cut nodes only and
PV nodes only (P26); R4 the KR-K miss: the harness with a per-job print,
a UCI cold-replay instrument checked against the harness on four
positions, then the failing position by setting (P27, P28); R5 the
donor's source at `edb0d9db`, read only; R6/R7 the pricing recipe of
amendment 4 (wb28, `bench 13`, WAC at depth 6 and 100,000 nodes, the
fixture's series, `test_endgames`, the floors at five budgets) on any
candidate that recovers the fixture (P31). The verdict rules are in the
file; §4 scores the predictions against them unchanged.

Instruments: `a6-trace` (ON arm, `TUNE=ON`, an environment-gated stderr
print of every ON-arm IIR firing — iteration, ply, depth, node type, the
entry's presence, bound and depth, the path — and a per-job print in the
floors; `bench 13` 3,345,971, so the prints change no node); `a6-iir-cut`,
`a6-iir-pv`, `a6-iirmin7` and `a6-iirmin7-off` (release). The existing
`b3-on-tune` and `b3-on-diag` reproduce 3,345,971; `t9-fls0` and `t9-pen0`
are Return 6's.

### 1. The mechanism on the fixture (question 1)

**1.1 The whole-root search, traced (R1b).** `go depth 11`, Hash 64,
Threads 1, the fixture `3r4/P7/2K1p3/R7/5p2/6kP/8/8 w`:

| Iteration | Store 1: score, nodes, PV | Store 0: score, nodes, PV |
|---|---|---|
| 8 | a7a8q +506, 9,046, `a7a8q d8a8 a5a8 f4f3 a8f8 f3f2 c6d6 g3g2` | a7a8q +384, 6,237 |
| 9 | **h3h4 +107**, 21,069, `h3h4 d8a8 a5a1` | a7a8q +315, 17,069, `… g3h3 a8f8 h3g3 c6c5 e6e5 c5d5 f4f3 d5e4` |
| 10 | h3h4 0, 31,655, `h3h4 f4f3 a7a8q d8a8 a5a8 g3h4 a8a1 f3f2 a1h1 h4g3 h1b1` | a7a8q +918, 73,485 |
| 11 | h3h4 0, 39,452 | a7a8q +1,144, 170,195 |

The win is Rf8 behind the f-pawn with the king walking in (the store-0
PV at depth 9; the store-1 PV at depth 8 reaches it one move later). At
iteration 9 the store-1 tree's first move a7a8q fails low under the
aspiration window around +506, the re-search gives h3h4 +107, and from
iteration 10 a7a8q is a later root move searched under a null window at
h3h4's score (the root does not reduce its moves, `search_kernel_core.cpp`
`reducible = !is_root`), where it fails low at once. The depth-10 PV
shows the engine still sees the queen promotion after h3h4 f4f3, but
rates the whole line 0.

The ON-arm IIR firings whose path begins with a7a8q, by iteration and by
the entry IIR found (NF none; EO an evaluation-only entry; SP a
quiescence stand-pat or delta-bound entry, bound without a move; FL a
negamax fail-low entry without a move, the store's product):

| Iteration | Store 1: records (NF / EO / SP / FL) | Store 0: records (NF / EO / SP / FL) |
|---|---|---|
| 9 | 83 (25 / 44 / 5 / 9, of which 2 are depth-0 delta-bound entries and **7** negamax fail-low entries at depths 1–10) | 88 (16 / 61 / 9 / 2) |
| 10 | **0** | 360 (140 / 107 / 99 / 14) |
| 11 | **0** | 213 (89 / 56 / 64 / 4) |

Four of the 83 are PV-node firings (store 0: one, at iteration 10). The
seven store-made firings sit at plies 5–10 on the Kh3/Kd6/Rf8 lines
(`r1b_store1.iir`). So at the iteration that flips the root, the
store's direct product is 8% of the IIR firings on the line; the rest fire
on entry classes both settings produce. After the flip the subtree is not
searched deeply enough to fire IIR at all. "The store removes the TT
move and IIR reduces the a7a8q line" is true of seven nodes; it is not
the account of why depth 9 reads +107.

**1.2 The same search under `searchmoves a7a8q` (R1).** The root's only
move, its aspiration window its own: store 1 reads +389 / +516 / +716 /
**+952** at depths 8–11 (136,048 nodes); store 0 reads +446 at depth 8
and **0** at depths 9, 10 and 11 (15,629 / 17,078 / 19,595 nodes) on the
Rg8+ shuffle amendment 3 §1.2 described. The fixture flips under a
root-window change, and in the opposite direction to the whole-root
flip. P24's and P30's "at least half of the IIR firings are the store's
product" missed on both reads (§4).

**1.3 Competing carriers, by option (R2, the Tune build, Hash 64, store
1, penalty 1).** The fixture at 33,000 / 60,000 / 100,000 nodes:

| Setting | 33k | 60k | 100k |
|---|---|---|---|
| defaults (1, 1) | h3h4 0 | h3h4 0 | h3h4 0 |
| `B3FailLowStore` 0 | a7a8q +315 | +315 | +918 |
| `B3Iir` 0 | a7a8q +382 | +382 | +382 |
| `B3IirMinDepth` 7 | a7a8q +284 | +284 | +1,235 |
| `B3IirMinDepth` 8 | a7a8q 0 | +344 | +344 |
| `B3IirMinDepth` 9 | a7a8q +372 | +372 | +774 |
| `B3IirMinDepth` 10 | a7a8q +372 | +388 | +388 |
| `CoreLmrCutNodeNoTt` 0 | a7a8q +365 | +365 | +983 |
| `B3IirStale` 1 | a5a3 +147 | a5a3 +147 | a7a8q +961 |

Three of the "no TT move" consumers were separable by option — IIR (its
switch and its minimum depth), the cut-node LMR term, and a second IIR
admission (the stale clause) — and each alone restores the win. The
first-move step-down (+1127 without a TT move, the donor's) has no
option and was not read. Razoring runs at depth ≤ 1 on this engine
(`CoreRazorDepthCap` 1) and cannot carry a depth-9 flip; RFP's
`tt_move == MOVE_NONE || tt_capture` admission has no switch, its share
moved −1.3% against ticket 8 (`t9/compare.md`), and it was not
discriminated. P25's "the LMR term leaves h3h4" missed: the term is as
sufficient as IIR.

**1.4 Robustness of the recoveries (R2b).** Over 50,000–130,000 nodes in
steps of 10,000: `B3Iir` 0 reads a7a8q +382 at seven budgets and +1,166
at two; `B3IirMinDepth` 7 reads +284, +1,235, +1,205, +1,212; the LMR
term 0 reads +365 then +983. P29 hit twice — with the calibration note
that nine budgets of one deterministic search are that search's
iterations 10–11 seen at nine stopping points, not nine reads; Return
6's "not a budget knife-edge" sweep has the same structure.

**1.5 IIR's node type (R3).** Two release ON builds:

| Build | `bench 13` | WAC 100k | Fixture 33k / 60k / 100k / 200k | Floors at 30k |
|---|---:|---:|---|---|
| IIR at cut nodes only | 4,418,297 | 242 | h3h4 +76 / a7a8q 0 / **+856** / +1,065 | 24 / 24 / 24 / 22 |
| IIR at PV nodes only | 3,445,881 | 246 | a7a8q +393 / +393 / +393 / +1,197 | 24 / 24 / 24 / 23 |

Removing cut-node IIR (98% of IIR's firings) restores the win robustly
and grows the tree 1.3×; removing PV-node IIR (`iir_pv_nodes`, 1.5% of
IIR's firings, the row Return 6 saw triple) restores it at 100,000 nodes
only, with a7a8q 0 at 60,000. P26 hit on both halves. The tripled
`iir_pv_nodes` is a symptom of the PV changing more often, not the
carrier.

**1.6 What the trace settles.** The store's effect on the fixture is a
population effect spread over every consumer of the no-TT-move signal,
as §5.6 predicted ("enlarges the no-TT-move population, which IIR,
razoring's and RFP's admission, `CoreLmrCutNodeNoTt` and the first-move
step-down's +1127 all read"); `tt_move_absent` rose from 65.4% to 72.7%
of interior nodes. The fixture reads the tree's shape at depths 9–11
through all of them at once, and reverses under a root-window change.
It is not a theory-truth read of the store; it is a coin the (1, 1) tree
loses and the (0, 1) tree wins, on the whole-root search only.

### 2. The donor's composition (question 2)

Read at `edb0d9db` (Stockfish 19; the local checkout's head is a 2026-10-28
development revision whose `search.cpp` differs in 292 lines, so the
pinned revision was read with `git show`). The local binary has no network file and searches no
node, and building the donor was not attempted (the net is a download).

| Form | Donor (`edb0d9db`) | Basilisk ON arm (ticket 9) | Same? |
|---|---|---|---|
| Fail-low store | `bestMove` is set only when a move raises alpha; the final write passes it, so a fail low stores no move (`search.cpp:1621-1627`); `TTEntry::save` keeps the entry's move on a none-move write to the same key (`tt.cpp:104-106`) | `stored_move = MOVE_NONE` on `TT_ALPHA`; `TranspositionTable::store` keeps the entry's move on a none-move store to the same key | yes |
| IIR | `!ss->followPV && !allNode && depth >= 6 && !ttData.move` (`:1051`), not in check by the step's position | `!follow_pv && (is_pv || cut_node) && depth >= 6 && tt_move == MOVE_NONE`, not in check | yes |
| Cut-node LMR term | `r += 4026 + 933 * !ttData.move` (`:1328`) | `lmr_cut_node + lmr_cut_node_no_tt·(tt_move == MOVE_NONE)`, 864 fitted in B.2 | form yes; constant Basilisk's |
| First-move step-down | `if (!ttData.move) r += 1127` before the full-depth search (`:1398-1399`) | `rr = r + 1127·(tt_move == MOVE_NONE)` | yes |
| RFP admission | `(!ttData.move || ttCapture)` (`:996`) | `tt_move == MOVE_NONE || tt_capture` | yes |
| Evaluation-only store at a miss | `BOUND_NONE, DEPTH_UNSEARCHED, Move::none()` | `TT_NONE, TT_DEPTH_UNSEARCHED, MOVE_NONE` | yes |
| Quiescence stand-pat store | `BOUND_LOWER, DEPTH_UNSEARCHED, Move::none()` | `TT_BETA`, depth 0, `MOVE_NONE` (and the delta bound as `TT_ALPHA`) | yes (the SP class of §1.1) |
| Bound-mismatch penalty | `ttWriter.penalize(1)` at depth > 5 (`:919`) | `penalize(hash)`, the same conditions | yes |
| Root TT move | `rootMoves[pvIdx].pv[0]` (`:820`) | the shared table's move (PLAN B.5's S6) | no — but the root follows the PV on both, so IIR never fires there |
| Move update on a declined store | the move is written before the depth test (`tt.cpp:104-106`), so a shallower store with a move still updates the move | a declined store returns before the move | no — bears on moves, not on none-move stores |

So the donor composes the same forms with the same constants (one fitted
to 864 instead of 933), and nothing around the store, IIR or the TT move
differs structurally. Whether the donor "loses" this fixture is not
readable here, and the question presumes an answer the evidence does not
give: at 100,000 nodes on a position that needs twelve plies to resolve,
any engine's answer is a property of its tree's shape. What differs is
calibration. The donor's IIR threshold, LMR term and step-down were
fitted on a population where a fail-low node carries no move; Basilisk's
`CoreLmrCutNodeNoTt` was fitted in B.2 on a population where it carries
its best-scored move, and IIR's minimum was carried from the donor at 6
with the IID-era 7 and a rail at 8 in its own history (§7). The ON arm
puts the donor's population under consumers fitted for the OFF arm's.
That is the Basilisk-only fact, and B.3.3's surface already carries both
consumers for the re-fit (`B3IirMinDepth` 3..10 step 2 in
`tools/spsa_configs/config_b3.json`; `CoreLmrCutNodeNoTt` by §5.6).

### 3. The KR-K miss at 30,000 nodes (question 3)

The harness with a per-job print (R4 i) reproduces Return 6's read —
KR-K 23/24, mean 27.5 plies (P28 hit) — and names the position:
`4K3/8/8/8/8/5k2/8/1R6 w - - 0 1`, no mate in 100 plies. A UCI cold
replay (one process, `ucinewgame` before every move, Hash 8, `go nodes
30000`, the game's moves on the position) reproduced the harness's
plies on three converting positions (3, 5, 11) and the miss (R4 ii); only
then was the position read by setting (R4 iii):

| Setting on the failing position | Plies to mate |
|---|---:|
| (store 1, penalty 1, IIR 1) | none in 100 (`Kf7 … Rb2 … Rb3 … Rb8 Rc8 Rc7 …`: the rook wanders, the king does not close) |
| IIR 0 | 69 |
| penalty 0 | 45 |
| store 0 | 21 |
| `B3IirMinDepth` 8 | 25 |
| `B3IirMinDepth` 7 | 69 |
| `CoreLmrCutNodeNoTt` 0 | 23 |

P27 hit. Every single-setting change converts the position, and the
penalty's removal is one of six, not a mechanism of its own: the miss is
the (1, 1) tree's shape at a budget where the mate is beyond the
horizon and the mop-up gradient has to carry the king, the same class of
event as the fixture. It is the same interaction in the sense that
matters — no single carrier, every perturbation flips it — and not a
second mechanism. It stays a floor miss: the floor rule is 24 at every
budget, and research proposes no relaxation of it.

A finding beside the leaf: the floors' generator admits positions with
the side not to move in check (`3k3R/8/1K6/8/8/8/8/8 w` is a KR-K sample
position, the harness's plies 15; `Board::try_set_fen` validates
nothing by default, and `test_epd_verdicts` validates strictly only the
packaged EPD). The harness plays them (White moves first, Black is in
check); the UCI path refuses them. Changing the generator would re-draw
the samples and re-base every floor, so it is reported, not done.

### 4. The decision (question 4)

**4.1 The arms priced.** Every line is the ON arm; the OFF arm is
16,615,757 on every source here (the min-depth build's OFF arm read it
exactly; the IIR edits are under `if constexpr (kB3Proof)`).

| Arm | `bench 13` | WAC 100k / d6 | Fixture 33k / 60k / 100k | Floors (five budgets) | KBN-K sum | wb28 | `test_endgames` |
|---|---:|---|---|---|---:|---|---|
| (1, 1) ticket 9 | 3,345,971 | 249 / 133 | h3h4 0 / 0 / 0 ✗ | **KR-K 23 at 30k** ✗ | 117 | 28/28 | 121/121 |
| (0, 1) store 0, Return 6's option 1 | 6,723,172 | 241 / 132 | +315 / +315 / +918 | 24 at every budget | 120 | 28/28 | 121/121 |
| (1, 0) penalty 0 | 5,133,969 | 246 / 133 | a5a3 +50 / h3h4 +46 / h3h4 +21 ✗ | 24 at every budget (KBN-K 21 at 45k) | 114 | 28/28 | 121/121 |
| (0, 0) ticket 8 | 5,635,655 | 243 / — | +913 | 24 | 119 | 28/28 | 121/121 |
| (1, 1), `B3IirMinDepth` 7 | 5,486,508 | 245 / 132 | +284 / +284 / +1,235 | 24 at every budget | 119 | 28/28 | 121/121 |
| (1, 1), IIR at PV nodes only | 3,445,881 | 246 / 134 | +393 / +393 / +393 | 24 at every budget | 119 | 28/28 | 121/121 |
| (1, 1), IIR at cut nodes only | 4,418,297 | 242 / 133 | h3h4 +76 / a7a8q 0 / +856 | 24 at every budget | 116 | 28/28 | 121/121 |

Release CTest on `t9-fls0` is 24/25: `test_b3_proof`'s "both switches
move the ON arm only" section asserts the base tree differs from the
store-0 tree, which with default 0 it cannot; the fail-low root test sets
the parameter explicitly on both values and passes. That is the one test
section option 1 rewrites.

**4.2 Return 6's option 1 — `B3FailLowStore` default 0, the penalty at 1.**
Binding reads: all hold, with margin at every budget. Bench tree:
6,723,172, 1.19× ticket 8 and 2.0× ticket 9; the cost screen (§10: the
geometric mean of ON/OFF cumulative-node ratios over depths 16–19 ≤ 3 on
five positions, an idle host) reads growth at depth, not size at 13, so
the risk is moderate and is the screen's to price. B.3.2/B.3.3 surface:
the store becomes §9 item 16, one 2,000-game paired run at ±5 (1 against
the default 0) with `tt_move_absent` on both settings, placed last; if
it adopts 1, B.3.3's sweep of `B3IirMinDepth` and `CoreLmrCutNodeNoTt`
runs on the store's population, which is the right order (the consumers
re-fitted for the population). Maintenance: one default line, one test
section, one PLAN §2 row re-worded (B-12's repair is built and switched,
its default read by games). Doctrine: §9 says an attribution switch's
default is the repair and a paired run never moves it; this amendment
asks the maintainer to reclassify the store from attribution switch to
categorical, because its "harm" on the OFF arm has never been measured
and its repair's measured harm is two low-budget conversions through
consumers fitted for the other population. Search cohesion argues the
same way: the consumers and the population they were fitted on travel
together until the fit moves.

**4.3 Option 2 — keep (1, 1), re-read the fixture clause and the floor.**
The evidence for re-reading the fixture clause is now strong: it flips
both ways (§1.2), under three single constants (§1.3), and amendment 3
§1.2 already called it knife-edge on both arms. But the KR-K floor is a
floor: 24 at every budget is the registered rule, the miss is a
100-ply shuffle in a won ending, and research does not propose to relax
a floor on the evidence that a different tree passes it. Option 2 is
therefore not available as written; a maintainer who wants (1, 1) kept
has to decide the floor clause too, and the main counter-argument is
B-3's reason for the multi-budget floors in the first place: a
conversion that fails at a low budget is the early sign of fragile
knowledge, which A.11.9 let through for a release cycle.

**4.4 Option 3 — a composition change.** Three were priced.
- `B3IirMinDepth` seeded 7 (both switches 1): passes every binding read;
  5,486,508 nodes (0.97× ticket 8); WAC 245; the KR-K position converts
  in 69 plies and the family's mean at 30,000 nodes is 28.4 against a
  29.5 ceiling. It keeps both donor forms and both repairs, moves a seed
  inside its registered range to a value Basilisk once ran (IID at 7), and
  is already a sweep coordinate. Against it: it is a seed moved on two
  low-budget conversion reads with no mechanism naming 7 rather than 8 or
  the LMR term (8 converts the rook ending in 25 plies but reads the
  fixture 0 at 33,000); and it thins IIR, which the donor's scaler note
  says is sensitive in the other direction only.
- IIR at PV nodes only: passes every binding read at the smallest tree
  (3,445,881) and WAC 246, and deletes 98% of IIR's firings — a mechanism
  change away from the donor's form made on a fixture read. Not proposed.
- IIR at cut nodes only: passes the clause as written (+856 at 100,000)
  with a7a8q 0 at 60,000 and h3h4 at 33,000. Not proposed.

**4.5 `B3TtPenalty`.** Keep default 1. Alone it grows the tree to
6,723,172 against ticket 8's 5,635,655 and moves no zero-game read
beyond noise (WAC 241 against 243, KBN-K 120 against 119, the fixture
+918 against +913); with the store it shrinks the tree to 3,345,971 and
the pair is the donor's. A 19% change in bench nodes is not a strength
read, the cost screen is the instrument that prices growth, and §9's
ablation order names the penalty first if the paired run is negative.
The alternative is 0, which lands ticket 8's tree exactly (fully
qualified at ticket 8) and leaves the penalty as an ablation arm that is
off; its counter-argument is that it drops a donor form on a bench-node
read, which §9's doctrine forbids as much as it forbids moving the store
on one.

**4.6 Predictions scored and registered.**

| P | Prediction (frozen) | Result | Verdict |
|---|---|---|---|
| P24 | R1, store 1: ≥ half of the a7a8q subtree's IIR records at iterations 9–11 are fail-low entries without a move; store 0's count ≤ half of store 1's (70%) | 93 of 405 (23%); 89 against 405 — but because the store-0 tree read 0 and stayed small | half miss |
| P25 | `CoreLmrCutNodeNoTt` 0 leaves h3h4 at 100k (75%); the smallest recovering `B3IirMinDepth` ≤ 9 (65%) | +983; 7 | miss; hit |
| P26 | cut-only IIR recovers a7a8q at 100k (60%); PV-only keeps it (55%) | +856; +393 | hit; hit |
| P27 | the KR-K position converts with IIR 0 (60%) | 69 plies | hit |
| P28 | the per-job harness reproduces 23/24 and 27.5 (85%) | reproduced | hit |
| P29 | the LMR term 0 and IIR 0 hold a7a8q at all nine budgets (55%, 65%) | both hold (one search's iterations) | hit; hit |
| P30 | whole root, store 1: h3h4 best with a7a8q ≤ 0 at depths 9–11 (80%); class-FL IIR records at iterations 9–11 outnumber store 0's IIR records 2:1 (60%) | h3h4 +107 / 0 / 0; 9 against 88 at iteration 9, 0 against 573 after | hit; miss |
| P31 | IIR at PV nodes only passes every binding read (50%); `B3IirMinDepth` 7 passes them (50%) | both pass | hit; hit |

Calibration: the two misses share one wrong belief — that the store's
direct product (a fail-low entry without a move) would be the bulk of
the IIR firings on the line. It is a tenth of them; the no-TT-move
population at IIR's depth is mostly fresh nodes and evaluation-only
entries, which both settings produce, and the store's effect runs
through ordering and the shape of the tree rather than through the
entries it writes. The second lesson is R2b's: a budget sweep of one
search is that search's iterations, and "holds at nine budgets" is two
or three data points.

Registered for B.3.2 (BAS-S41) if option 1 is taken:

- **P32.** §9 item 16's paired run (store 1 against the default 0, 2,000
  games at `3+0.03`) reads inside ±5 and leaves the default (60%); a
  read ≥ +5 adopts 1 and B.3.3's sweep of `B3IirMinDepth` and
  `CoreLmrCutNodeNoTt` then runs on that population.
- **P33.** `tt_move_absent` at depth 14 on the ON arm is at least 5
  points of interior nodes higher with the store at 1 than at 0 (80%;
  the ticket-8 → ticket-9 read was +7.3).

### 5. Clauses changed, decisions, the amended ticket, the condition

**Clauses changed on the maintainer's decision 1 (option 1):**

1. **§9's attribution table** loses the `B3FailLowStore` row; **§9's
   categorical table gains item 16**: `B3FailLowStore` — 0 the
   best-scored move stored after a fail low (the OFF arm's form); 1 no
   move stored (the donor's form, B-12's repair) — default **0** —
   decided by one paired run at ±5, last in the order, with
   `tt_move_absent` read on both settings at depth 14 before it. The
   `B3TtPenalty` row is unchanged (default 1, ablation only).
2. **§4's B-12 row** reads: a defect by PLAN §2's table; its repair is
   built in the ON arm behind `B3FailLowStore` (ticket 9) and its default
   is 0 until §9 item 16's paired run reads it, because the repair's
   population effect on IIR, `CoreLmrCutNodeNoTt` and the step-down
   (§5.6) loses two low-budget conversions on consumers fitted for the
   OFF arm's population (amendment 6). **PLAN §2's table row** for B.3
   needs the same re-wording, which is the maintainer's edit.
3. **§5.6** gains: the population effect is measured (`tt_move_absent`
   65.4% → 72.7%); each of its separable consumers alone flips the
   promotion fixture and the KR-K position at 30,000 nodes; the store is
   read as §9 item 16, and B.3.3's sweep of the two consumers runs on
   whichever population that run chooses.
4. **§10 ticket 9 → ticket 9b** (below). **§10 ticket 10's two-arm table**
   gains, as recorded rows on the ON arm: the (store, penalty) 2×2 of
   Return 6 (bench, WAC, the fixture at 100,000, KR-K at 30,000) and the
   fixture at depth 11 with and without `searchmoves a7a8q` on both store
   settings (§1.1–1.2). The cheap qualification is unchanged.
5. **Amendment 3 §5's fixture clause** is unchanged and still binds; its
   composition sensitivity is on record (§1) and is the reason it is read
   on the final tree only, never used to pick a constant.
6. **P24–P31 scored; P32–P33 registered** (§4.6).

**Research decides (its own):**

1. The mechanism is a population effect through every no-TT-move consumer,
   not IIR alone (§1); the donor composes the same forms and the
   difference is calibration (§2); the KR-K miss is the same class of
   event and stays a floor miss (§3).
2. Option 3's three candidates are priced and not proposed; the
   `B3IirMinDepth` 7 arm is the alternative to decision 1 below.
3. The floors' generator finding (§3) is reported beside the leaf.

**The maintainer decides, each with its alternative and its main
counter-argument:**

1. **Ticket 9b lands with `B3FailLowStore` default 0 and `B3TtPenalty` 1**
   (§4.2), the store reclassified as §9 item 16. Alternative:
   `B3IirMinDepth` seeded 7 with both switches at 1 (§4.4), every binding
   read passing, both donor forms kept. Counter-argument to the
   recommendation: B-12 is a defect by PLAN §2 and the arm goes to its
   paired run without the repair; answered by the repair's measured harm
   being two conversions lost through consumers fitted for the other
   population, the "defect" having no measured harm, and the paired run
   being the instrument that decides a question of this kind.
2. **`B3TtPenalty` keeps default 1** (§4.5). Alternative: 0, ticket 8's
   tree exactly. Counter-argument: 19% more bench nodes for no zero-game
   gain; answered by bench nodes not being a strength read and the cost
   screen being the gate that prices growth.

**The amended ticket text implementation resumes on** (replacing §10's
ticket 9 paragraph and Return 6's premise):

- *Ticket 9b* (on `58ea1ed`'s tree, `6b0e4c4`). `B3FailLowStore` default
  0 in `search_params.h` (`t9/t9_fls0.patch` is the line); its
  semantics: 1 stores no move after a fail low and the entry keeps its
  own (B-12's repair, the donor's form), 0 stores the best-scored move
  (the OFF arm's form); `B3TtPenalty` default 1 unchanged; no other
  source change. `test_b3_proof`: the "both switches move the ON arm
  only" section compares the base tree against `b3_fail_low_store = 1`
  and `b3_tt_penalty = 0`, each differing from the base and neither
  moving the OFF arm; the fail-low root section unchanged (it sets both
  values explicitly); `tt_penalized` fires with the penalty on only.
  Reads at the ticket's end, on the committed tree: OFF `bench 13`
  16,615,757 exact; ON recorded (the variant read 6,723,172); release
  CTest 25/25 on both arms; WAC at 100,000 nodes ≥ 232 (read 241) and
  depth 6 recorded (132); the fixture a7a8q with a positive score at
  100,000 nodes (read +918) and its budget series recorded; the
  conversion floors at five budgets, KQ-K, KR-K and KBB-K 24 at every
  budget and KBN-K's sum ≥ 112 (read 24 × 15 and 120); the wrong-bishop
  28 (read 28/28) and `test_endgames` (read 121/121); the per-ticket
  counter table against ticket 8 with `tt_move_absent` on both store
  settings; recorded on both arms, the organic FEN and the primed 28
  (amendment 4's rows). Outside any binding read, Return 7 with the
  counters, no default moved by hand.
- *Ticket 10* as packet §10 with amendments 1–5 and this one: the
  two-arm table carries the rows of §5 item 4; the implementation record
  names the store's default among the deviations with this amendment as
  the reason; the cost screen as re-scoped (idle host, ask first; P11 its
  prediction); the depth-6 floor re-based on the ON arm.

**Condition for resuming:** the maintainer's decision 1 above (option 1,
or its alternative with the seed decided). B.3.1 stays `RESEARCH` until
then. Decision 2 is taken with decision 1.

**Maintainer decisions, 2026-10-31, after the amendment:** both taken as
recommended. 1. Ticket 9b lands with `B3FailLowStore` default 0 and
`B3TtPenalty` 1; the store is §9 item 16, read by one paired run in
B.3.2. 2. `B3TtPenalty` keeps default 1. The amended ticket 9b text above
is the contract implementation resumes on; B.3.1 resumes
`READY_FOR_IMPLEMENTATION`.

### 6. Artifacts (`tools/results/b31-20261030/amend6/`, ignored storage)

- `amend6_R.md`: the registration, frozen before each read, with the
  R1b, R2b, R4 and R7 addenda and one clerical note.
- Patches, each restored byte-exact: `trace.patch` (the IIR print and
  the floors' per-job print), `iir_cut.patch`, `iir_pv.patch`,
  `iirmin7.patch`.
- Scripts: `scripts/a6_builds.py`, `a6_build_min7.py`, `a6_r1.py`,
  `a6_r1b.py`, `a6_r2.py`, `a6_r2b.py`, `a6_r3.py`, `a6_r4.py`,
  `a6_price.py` (amendment 4's recipe with the output here).
- Reads: `r1.txt`, `r1_store{1,0}.{stdout,iir}`, `r1_*_recs.json`,
  `r1b.txt`, `r1b_*`, `r2.txt`/`r2.json`, `r2b.txt`/`r2b.json`,
  `r3.txt`/`r3.json`, `r4.txt`/`r4.json`, `r4_floors_30000.log` (the
  per-job floors), `price_r3.log`, `price_t9variants.log`,
  `price_iirmin7.log` with their `price_*.json`, `conv_*_<budget>.log`
  and `endgames_*.log`.
- Builds: `build/a6-trace` (`TUNE=ON`, 3,345,971), `build/a6-iir-cut`,
  `build/a6-iir-pv`, `build/a6-iirmin7` (5,486,508), `build/a6-iirmin7-off`
  (16,615,757); the source tree clean after every build (`git status`).

## Implementation record — tickets 6b to 10, the final state (2026-10-31)

Continues the record of tickets 0 to 6 (above, at Return 2). Branch
`b3-pruning-extensions`; the release preset, PEXT portable, Clang 23.1.4,
non-PGO `all` trees. Flavours: Release, Tune, Diag and Ablate on both arms
(`build/b3-{on,off}`, `-tune`, `-diag`, `-ablate`) and the sanitizer trees
(`build/b3-{on,off}-san`: the `debug` preset, ASan and UBSan). Every read
below is on the committed tree, each binary rebuilt from it; the
temporary variant builds restored the source byte-exact (`git status`
clean after each). Evidence: `tools/results/b31-20261030/t9b/` and
`t10/` (ignored storage), with `t9b.check.log` in the ticket checks'
shape.

### Per ticket

| Ticket | Commits | OFF `bench 13` | ON `bench 13` | ON WAC depth 6 / 100k | Fixture at 100k | Release CTest |
|---|---|---:|---:|---|---|---|
| 6b | `eeeaa86`, `cfb9232` | 16,615,757 | 6,263,268 | 136 / 237 | a7a8q +956 | 25/25 both arms |
| 7 (Return 4) | `6007ab5` | 16,615,757 | 4,517,333 | 134 / 249 | a7a8q | ON 24/25 (`test_endgames`) |
| 7b | `920cc5d` | 16,615,757 | 3,498,408 | 134 / 250 | a7a8q +661 | 25/25 both arms |
| 8 | `7e92ecb`, `5b6bae1` | 16,615,757 | 5,635,655 | 132 / 243 | a7a8q +913 | 25/25 both arms |
| 9 (Return 6) | `58ea1ed` | 16,615,757 | 3,345,971 | 133 / 249 | h3h4 0 | ON 24/25 (`test_search`) |
| 9b | `738261f`, `fb559d0` | 16,615,757 | **6,723,172** | 132 / 241 | a7a8q +918 | 25/25 both arms |
| 10 | `5dcf030`, `3321cf3` | 16,615,757 | 6,723,172 | 132 (floor 119) / 241 | a7a8q +918 | 25/25 both arms |

The Diag binary of each source read the release count at every ticket.
The ON arm's Diag build ran `test_stack_unwind`, `test_b3_proof` and
`test_search` with node checks live at every ticket, and passed.

### Ticket 9b's binding reads (amendment 6 §5)

| Read | Result | Rule |
|---|---|---|
| OFF `bench 13` | 16,615,757 (Release and Diag) | exact |
| ON `bench 13` | 6,723,172 (Release = Diag = Tune) | recorded; amendment 6's variant read 6,723,172 |
| Release CTest | 25/25 on both arms | binds |
| WAC | 241 at 100,000 nodes; 132 at depth 6 | ≥ 232; recorded |
| The fixture `3r4/P7/2K1p3/R7/5p2/6kP/8/8 w` | a7a8q +315 / +315 / **+918** / +1,097 / +1,097 / +1,464 at 33k / 60k / 100k / 200k / 400k / 1M nodes | a8=Q, positive at 100k |
| Conversion floors | KQ-K, KR-K, KBB-K 24 at every budget; KBN-K 24 × 5 = **120** | 24; ≥ 112 |
| Wrong-bishop 28 (the test's form); `test_endgames` | 28/28; 121/121 | binds |
| Organic `5k2/8/6KP/8/8/8/8/5B2 b`, depths 8–14 | OFF 6/7 (−90 at depth 11); ON 7/7 | recorded |
| Primed 28 | OFF 24/28; ON 27/28 (+295 at depth 8 on the first FEN, +64 at depth 9 on the third); controls 28/28 | recorded |
| `tt_move_absent`, depth 13, the 40 positions (Tune build, the option) | store 0: 66.11% of 2,021,950 interior nodes; store 1: 72.67% of 2,003,134 | recorded (P33 is B.3.2's, at depth 14) |
| Counters against ticket 8 | interior 0.909×; `tt_penalized` 0.70%; `tt_move_absent` +1.1%; Cluster 1 over 20%: `cont_hist_pruned` −29.8% | recorded |

### Cluster 1 rows that moved by more than 20%, tickets 6b to 9b

Shares of interior nodes, ON arm, against the previous ticket; never
repaired.

- **Ticket 6b.** Return 4's table: within 20% except
  `hist_reward_updates`, on a tree 0.29× ticket 5b's hybrid.
- **Ticket 7 and 7b.** `cont_hist_pruned` +34% (2.40% to 3.22%) and
  +32.7%, following the correction's new producers.
- **Ticket 8.** None.
- **Ticket 9b.** `cont_hist_pruned` −29.8% (3.75% to 2.63%). The whole
  move is bench position 11: −30,382 of the −30,186 net, its tree halving
  under the penalty (457,467 to 251,939 interior nodes); the other 39
  positions net +196.

### Ticket 10

- **The mask.** Bits 2, 3, 4 and 6 sit on the ON arm's mechanisms from
  their tickets (the null move, ProbCut, IIR, and the singular block with
  its multi-cut and negative extension); ticket 10 adds bit 3 to the small
  TT ProbCut, which is off by default (`B3ProbCutTt` 0), so no fingerprint
  moves. The check extension exists on the ON arm only at
  `B3CheckExt=1`, a Tune-build option, so the Ablate flavour never has it
  and bit 6 stays matched to the oracle's extension family (B-109).
- **Liveness** (`ablation_liveness.py`, eight `suite_v2` positions, depth 9,
  `ablation_liveness_{on,off}.json`): all eight bits live on both arms,
  every bit moving all eight positions. ON (101,363 nodes unmasked):
  razoring 0.89×, RFP 1.70×, null move 1.03×, ProbCut 0.99×, IIR 1.14×,
  shallow move pruning 3.25×, extensions 0.97×, LMR 1.38×. OFF (209,378):
  0.80×, 1.59×, 0.78×, 1.03×, 0.56×, 3.61×, 0.55×, 1.21×.
- **The depth-6 WAC floor** re-registered on the ON arm: 132 less the 13
  of headroom the OFF floor carries, **119** (`test_wac.cpp`, by
  `kB3Proof`); `test_wac_floor` runs `--floor` on both arms again. OFF 182
  against 175, ON 132 against 119; both pass.
- **Sanitizers.** `bench 10` on the sanitizer trees equals release on both
  arms (ON 1,367,728, OFF 4,034,698); CTest under ASan and UBSan,
  `-LE strength`, 23/23 on both arms, no sanitizer report.
- **CI.** The `flavors` job's `B3_PROOF` cell has existed since ticket 0.
  Nothing was pushed, so CI has not run on this branch; it runs at the PR.

### The two-arm table (B.2's final-state shape)

| Read | OFF | ON |
|---|---:|---:|
| `bench 13` Release / Tune / Diag / Ablate | 16,615,757 × 4 | 6,723,172 × 4 |
| `bench 10` release / sanitizer | 4,034,698 / 4,034,698 | 1,367,728 / 1,367,728 |
| Release CTest / sanitizer CTest | 25/25 / 23/23 | 25/25 / 23/23 |
| WAC depth 6 (floor) | 182 (175) | 132 (119) |
| WAC 100,000 / 400,000 nodes | 240 / 265 | **241** / 261 |
| WAC `depthpv 10` | 248 | 223 |
| The fixture, 33k / 60k / 100k / 200k / 400k / 1M | a7a8q +68 / +370 / +616 / +841 / +841 / +841 | a7a8q +315 / +315 / +918 / +1,097 / +1,097 / +1,464 |
| Floors, KQ-K / KR-K / KBB-K | 24 at every budget | 24 at every budget |
| KBN-K, 30k–120k (sum) | 22 / 23 / 24 / 23 / 23 (115) | 24 × 5 (120) |
| KR-K mean plies at 30,000 | 28.9 | 22.6 |
| BAS-R07's rec1 `7r/5R2/8/2k1PB2/8/4K3/8/8 w` at 100k | e5e6 +1,095 (depth 10) | f5g4 +747 (depth 9) |
| Organic FEN, depths 8–14 | 6/7 | 7/7 |
| Primed 28 / control 28 | 24 / 28 | 27 / 28 |
| Return 5's line, `searchmoves f7f8`, depths 1–6 | −1,276 / −1,157 / −1,157 / −972 / 0 / 0 | −1,276 / −190 / −127 / −13 / 0 / 0 |

The OFF arm's 120,000-node conversion run exits 1 on the floors' own
staleness guard (KQ-K's mean 10.8 plies, as §2.2 recorded for the head),
not on a floor.

Recorded on the ON arm (amendment 6 §5 item 4), the (store, penalty) 2×2
of Return 6 at `6b0e4c4`'s source:

| (`B3FailLowStore`, `B3TtPenalty`) | `bench 13` | WAC 100k | Fixture at 100k | KR-K at 30k |
|---|---:|---:|---|---:|
| (0, 0) | 5,635,655 | 243 | a7a8q +913 | 24 |
| (1, 0) | 5,133,969 | 246 | h3h4 0 | 24 |
| (0, 1), the final defaults | 6,723,172 | 241 | a7a8q +918 | 24 |
| (1, 1) | 3,345,971 | 249 | h3h4 0 | 23 |

And the fixture at depth 11 (Tune build, Hash 64, `fixture_d11.txt`),
reproducing amendment 6 §1.1–1.2 exactly:

| Root | Store 1, depths 8 / 9 / 10 / 11 | Store 0, depths 8 / 9 / 10 / 11 |
|---|---|---|
| Whole | a7a8q +506 / h3h4 +107 / h3h4 0 / h3h4 0 | a7a8q +384 / +315 / +918 / +1,144 |
| `searchmoves a7a8q` | +389 / +516 / +716 / +952 | +446 / 0 / 0 / 0 |

**The screen's position set, from ticket 10b** (research amendment 8 §5,
the maintainer's decision 1 of 2026-11-01). The rule: from the 40 bench
and 106 `suite_v2` positions, the OFF arm's largest depth-14 tree among
those whose depth-14 root score is within ±564 (`B3SeekMateScore`, so
every ON mechanism is live) and whose tree grows less than 2× at both
depths 13 and 14 (no resolution event). 53 of the 146 qualify. The
largest is `suite_v2` position 12,
`rn2kb1r/pp2pppp/2p5/3q4/3P1Bb1/5NP1/PPP2P1P/R2QKB1R w KQkq`: 746,048
nodes at depth 14, root score +104, growth 1.96 and 1.27 (the next is
bench position 2, 653,952). The data is `amend8/a8_select.json`
(SHA-256 1bae09fa…; the OFF arm, Hash 64, `go depth 14`), re-scored by
the rule for this record. Position 12 replaces bench position 11 on the
first line of `t10/screen_positions.epd` (now SHA-256 1ff7b03d…);
amendment 7's set is kept as `t10/a7/screen_positions_a7.epd`
(65b6a56a…).

**The conversion-band read, bench position 11** (amendment 8 §2 and
decision 1: recorded, not judged). Cumulative nodes, growth over the
previous depth and root score in centipawns; Hash 64, `go depth N` under
the 600 s per-iteration watchdog (`t10/a7/b11_reads.json`;
`amend8/a8_r1.json`, the ON ablation build at `AblationMask 8`). The
band's iterations are in bold.

| Depth | OFF | ON | ON, ProbCut off (bit 8) |
|---|---|---|---|
| 19 | 6,663,683 · 1.01 · 2,424 | 4,617,856 · 1.47 · 2,593 | 5,165,192 · 1.17 · 2,608 |
| 20 | 6,746,421 · 1.01 · 2,424 | **20,917,059 · 4.53 · 2,704** | 7,015,730 · 1.36 · 2,716 |
| 21 | 6,820,051 · 1.01 · 2,424 | **140,180,782 · 6.70 · 2,878** | **22,919,965 · 3.27 · 2,985** |
| 22 | 9,665,168 · 1.42 · 2,462 | **640,119,499 · 4.57 · 3,053** | **83,036,567 · 3.62 · 3,307** |
| 23 | **36,257,665 · 3.75 · 2,700** | — | 93,608,334 · 1.13 · 3,714 |
| 24 | **157,177,066 · 4.34 · 3,106** | — | — |
| 25 | **1,781,283,940 · 11.33 · 4,953** | — | — |

Each arm's band begins in the iteration whose root score climbs past
about 2,700, and every iteration that grows 3× or more ends there or
higher. The ON arm enters the band three plies before the OFF arm; with
ProbCut off it enters one ply later than the control and passes it more
cheaply (amendment 8 §2–§4).

### Cheap qualification

| Item | Result | Rule |
|---|---|---|
| The two-arm table, every fingerprint from a clean tree | above | — |
| WAC at 100,000 nodes; depth 6 | 241; 132 | ≥ 232; ≥ the re-based 119 |
| Floors at five budgets; the fixture at 100k | 24 × 15 and KBN-K 120; a7a8q +918 | 24; ≥ 112; a8=Q positive |
| Wrong-bishop draws (the test's form) | 28/28; primed and organic recorded | binds |
| `B3CorrDecisive` 0 and 1 | 0: every family 24 at every budget, KBN-K 120 (`bench 13` 5,508,967); 1: the same, 120 | recorded |
| The KBN-K multi-cut read (§9 item 4) | `B3MultiCut` 0: 120; 1: 23/22/24/23/23 = 115 (`bench 13` 4,339,893). The ON arm passes the majority rule (base 115) at its default, so the default stays 0 and `B3MultiCut` goes to its paired run | rule |
| The depth-20 wire check (ticket 3) | bench position 11: `null_verifications` 237, `ext_budget_bound` 370 of 1,081 singular firings; position 15: 0 and 263 of 566 (Diag, finished in 27 s and under 1 s) | ≥ 1 on position 11 |
| Liveness | all eight bits, both arms | every bit moves nodes |
| The decision trace on WAC.001, 085, 203, 253 (depth 8, the WAC move as root, Diag) | ON 477 / 332 / 851 / 108 records; OFF 620 / 492 / 824 / 139; `trace_{on,off}_WAC.*.json` | archived |
| The deep-iteration cost screen (re-scoped, amendment 2 §3; P11) | **FAIL** on `suite_v2` position 50: growth geomean 2.53; the other four pass (*Return 7*) | growth geomean ≤ 1.5, every depth finished |
| The cost screen on amendment 8's set (ticket 10b: amendment 7 §R's clause, `suite_v2` position 12 for bench position 11; P41) | **PASS** on all five. `suite_v2` 12: steps 20 / 21 / 22 at 0.996 / 0.933 / 1.038, geomean 0.988; both arms finish every depth to 25; no watchdog stop. Bench 15 and `suite_v2` 50, 42 and 60 as amendment 7 read them (0.493, 1.020, 1.016, 1.163), node for node | (a)–(d): steps 20–22 geomean ≤ 1.5, every depth 16–22 finished |

The screen's positions are in `t10/screen_positions.epd` (bench positions
11 and 15, `suite_v2` positions 50, 42 and 60); `t10/screen_growth.py`
scores `deep_iteration_screen.py`'s JSON by the re-scoped rule and records
the absolute ratio.

**Ticket 10b (2026-11-01).** From ticket 10b the set is `suite_v2`
position 12, bench position 15 and `suite_v2` positions 50, 42 and 60
(amendment 8 §7's clause).

- **The run.** It was read once with `deep_iteration_screen.py
  --depth-min 16 --depth-max 25`, the 60 s watchdog and Hash 64
  (`t10/t10b/screen25b.json`, SHA-256 bc4f052e…, and `screen25b.log`).
  The binaries were ticket 10's (`t10/a7/hashes.txt`, re-hashed equal;
  no source change since `f2fa3c6`).
- **The score.** `t10/a7/screen_score_a7.py` (SHA-256 57c0ea35…, the
  registered file) scored it into `t10/t10b/score.log`, which exits 0.
- **The host.** It was in use, but no watchdog fired on either arm, so
  (d) does not arise.
- **Identity.** Positions 2–5 reproduce amendment 7's `screen25.json` at
  every depth on both arms.
- **Position 12.** The ON arm's tree is 0.41–0.65× the OFF arm's at
  every depth from 15 to 22 (0.499 at 22) and 0.24× at 25. Steps 17–19
  read 1.498, 0.865 and 0.920. These numbers are recorded, not judged.
- **The tool's own verdict.** The screen's printed verdict, the original
  absolute rule, also reads pass; it is not the verdict.

**P41 scored: hit** (75%).

### Tests added, tickets 7 to 10

- `test_b3_proof`, both arms:
  - the donor's fills (`clear(true)` against `clear()`);
  - the three correction switches moving the ON arm only;
  - exclusion training sparing a recognised draw (KBP-K, depth 12);
  - every reversible move's cuckoo key found, and no false positive
    against a slow reference over 10,000 positions;
  - the cuckoo cutoff at the draw window (depths 1–3);
  - the fail-low root's stored move under both store settings;
  - both TT switches moving the ON arm only.
- `test_tt`: `penalize` lowers the depth by one and keeps every other
  field, and leaves depth-0, evaluation-only and missing entries alone.
- `test_wac`: the ON arm's floor 119.
- `test_spsa_surface.py`: the B3 sweep points (ticket 6b).

### Deviations and interpretations, each with its reason

- Ticket 6b: the singular margin seeded at 252 and the donor's `seekMate`
  fired from root depth 12 (amendment 3, the maintainer's decisions);
  `B3PvQuietPrune` added (amendment 3 §5).
- Ticket 7b: the raw-evaluation guard on the exclusion producer (no
  exclusion training where the raw static eval is 0), `B3CorrExcluded`
  default 1 kept (amendment 4, the maintainer's decision 1).
- Ticket 8: the terminal test reads the cutoff at the root's child under a
  test-only root window, `SearchLimits::root_alpha`/`root_beta`, read by
  the driver alone; test (b) withdrawn (amendment 5).
- Ticket 9b: **`B3FailLowStore` default 0**, the best-scored move stored
  after a fail low (the OFF arm's form), with the store reclassified as §9
  item 16 and read by a paired run in B.3.2; `B3TtPenalty` 1 (amendment 6,
  the maintainer's decisions 1 and 2). B-12's repair is built and switched,
  not on.
- Ticket 9b's switch test reads bench position 3 (`bench_fens()[2]`) at
  depth 12, not position 2: with the store at 0 the penalty fires and leaves the node
  count unchanged on bench positions 1, 2, 6, 9 and 10 (position 2: 32,555
  nodes either way; position 3: 67,822 against 84,770). The assertions are
  unchanged (`fb559d0`).
- Ticket 10: bit 3 also covers the small TT ProbCut (above); the depth-6
  floor is chosen in `test_wac.cpp` by `kB3Proof` rather than by a CMake
  branch.

### What the reviewer must check

- Each ticket's diff against §10 as amended by amendments 1–6, and the
  deviations above.
- The OFF arm's code: shared seams only since ticket 6 (the root-window
  fields at their equal defaults, `TranspositionTable::penalize` reached
  only from an ON-arm branch, the counters).
- The fingerprints of the two-arm table, reproduced from a clean tree on
  every flavour.
- The cost screen's result, once the maintainer has run it on an idle host.
- Added at ticket 10b (§10 *Review* with amendment 8 §6):
  - amendment 7 §R's rule against its §0 argument, and the registration
    order (`50a97b8` before the read);
  - research amendment 8 §1–§4: the port read line for line, the band on
    every arm, the constants as a phase reading, and H3's channel left
    unmeasured;
  - ticket 10b's selection, re-scored from `amend8/a8_select.json`, and
    the five-position read with its score.

## Return 7 — B.3.1 cheap qualification: the re-scoped cost screen fails on `suite_v2` position 50 (2026-10-31)

**What ran.** The maintainer ran the deep-iteration cost screen on an idle
host on 2026-10-31, as registered: `deep_iteration_screen.py` with its
defaults (depths 16–19, a 60 s watchdog per iteration, Hash 64), base
`build/b3-off`, arm `build/b3-on`, the five positions of
`t10/screen_positions.epd`. The binaries' SHA-256 equal ticket 10's
(`t10/hashes.txt`), so the read is the committed tree (`f2fa3c6`); the
JSON is `t10/screen.json` (SHA-256 b4f160bd…). `t10/screen_growth.py`
scores it by amendment 2 §3's rule. No watchdog fired on either arm.

| # | Position | OFF cumulative nodes, depths 16 / 17 / 18 / 19 | ON cumulative nodes, depths 16 / 17 / 18 / 19 | Growth ratio, steps 17 / 18 / 19 | Growth geomean | Absolute geomean (recorded) | Rule |
|---|---|---|---|---|---|---|---|
| 1 | bench 11 | 2,950,852 / 4,040,018 / 6,576,469 / 6,663,683 | 1,353,587 / 2,290,243 / 3,148,082 / 4,617,856 | 1.24 / 0.84 / 1.45 | 1.147 | 0.542 | pass |
| 2 | bench 15 | 223,388 / 252,698 / 372,366 / 695,230 | 90,545 / 94,944 / 99,328 / 108,493 | 0.93 / 0.71 / 0.59 | 0.727 | 0.282 | pass |
| 3 | `suite_v2` 50 | 30,080,153 / 30,092,599 / 30,151,520 / 30,177,476 | 702,258 / 1,594,587 / 3,075,838 / 11,418,629 | **2.27 / 1.93 / 3.71** | **2.531** | 0.083 | **FAIL** |
| 4 | `suite_v2` 42 | 10,403,275 / 10,517,412 / 10,534,184 / 10,576,913 | 2,461,948 / 2,478,915 / 2,502,649 / 2,528,501 | 1.00 / 1.01 / 1.01 | 1.003 | 0.237 | pass |
| 5 | `suite_v2` 60 | 8,664,742 / 8,677,782 / 8,696,737 / 8,717,040 | 95,545 / 105,215 / 117,884 / 132,956 | 1.10 / 1.12 / 1.13 | 1.114 | 0.013 | pass |

**The miss.** Position 3 is `4rrk1/pppb4/7p/3P2pq/3Qn3/P5P1/1PP4P/R3RNNK b`.
The OFF arm's tree grows from 9.67M at depth 13 to 30.07M at depth 14,
then adds 6,000 to 59,000 nodes per iteration through depth 19 (growth
1.000–1.002): its iterations have settled. The ON arm finishes every
depth and is smaller at every depth (0.023 of the OFF arm's cumulative
nodes at 16, 0.378 at 19), but it is still growing: 0.70M, 1.59M, 3.08M
and 11.42M at depths 16–19, per-iteration growth 2.27, 1.93 and 3.71. The
geometric mean of the growth ratios is 2.53 against 1.5. The clause fails
as registered; no default and no clause was moved.

**What the read does and does not say.** Positions 4 and 5 show the same
settled OFF arm (growth ≤ 1.004 from depth 15) and an ON arm settled too,
so they pass. On position 3 the rule divides the ON arm's growth by an OFF
growth of about 1, and the screen records only node counts. It cannot tell
an ON-arm explosion on this position from an ON arm that has not yet
reached whatever settled the OFF arm at depth 14 (a decisive score, a
resolved PV, a TT-carried bound). P11's falsifier names a fail on this arm
as growth from outside the singular block and returns ticket 6b; that
attribution is not yet shown either.

**P11 scored:** predicted a pass on all five positions (probability 75%);
missed on position 3.

**Reads that discriminate** (deterministic depth reads, minutes each; they
wait until the host is free of the maintainer's other runs):

1. Both arms' `info` lines on position 3 to depth 19 (score, PV, seldepth
   per iteration): what settles the OFF arm at 14, and whether the ON arm
   has reached it by 19.
2. The ON arm continued to depths 20–22 under the watchdog: whether its
   growth falls to about 1 once it reaches the same score, or keeps rising.
3. The Diag counters on the ON arm at depths 17–19 on position 3
   (`ext_budget_bound`, singular firings and extensions, null
   verifications, ProbCut and small-ProbCut cuts, IIR firings), and the
   ablation bits 2, 3, 4 and 6 at depth 19: which mechanism carries the
   growth.

**Options for research.** (1) The growth is a mechanism's: amend that
mechanism, as P11's falsifier says. (2) The growth is the approach to the
OFF arm's settling point: the clause has no registered treatment of a
settled OFF arm, and research decides one before any re-run. Changing
the clause is research's decision, not implementation's.

**State.** B.3.1 returns to `RESEARCH` and resumes on research amendment 7.
Everything else of the cheap qualification passes (*Implementation record
— tickets 6b to 10*).

### Return 7 addendum — the three discriminating reads (2026-11-01)

**What ran.** The three reads above, once, on a free host (no other engine
running), with the binaries of ticket 10's `t10/hashes.txt` (re-hashed
equal; no source change since `f2fa3c6`). `t10/p50_reads.py` drives the
screen's UCI sequence (Hash 64, `ucinewgame`, `go depth N`, a 60 s
per-iteration watchdog); every engine line is in `t10/p50_r*.log`, the
summary in `t10/p50_reads.json` (SHA-256 331b4f87…). No watchdog fired.
Identity holds: both arms reproduce the screen's per-depth nodes to the
node, and the diag build and the ablation build at mask 0 reproduce the
ON arm's to depth 19.

**Read 1 — what settles each arm.** Both arms find the same `mate 9`
(PV `e4f2 d4f2 f8f2 e1e8 h5e8`, transpositions after). The OFF arm finds
it at depth 14, with 30,068,087 cumulative nodes (score at depths 12/13:
cp 1910 / 2682), and every later iteration re-proves it. The ON arm
climbs cp 1725 / 2260 / 2523 at depths 16/17/18 and finds the mate at
depth 19, with 11,418,629 cumulative nodes: 0.380 of what the OFF arm
spent to find it.

**Read 2 — the ON arm past 19.** Cumulative nodes 11,607,784 /
11,822,394 / 12,173,197 at depths 20/21/22, per-iteration growth 1.017 /
1.018 / 1.030, the mate and PV unchanged. Once it has the mate, the ON arm
settles as the OFF arm did from depth 15.

**Read 3 — which mechanism carries the growth.** The Diag build's
increments per iteration (depth 17 / 18 / 19; the depth-16 dump is the
base):

| Counter | Iteration 17 | Iteration 18 | Iteration 19 | Share of interior nodes, 17 / 18 / 19 |
|---|---|---|---|---|
| interior nodes | 475,263 | 818,100 | 5,436,705 | — |
| singular tries / fired | 0 / 0 | 0 / 0 | 0 / 0 | all 47 tries and 27 firings precede depth 17 |
| `ext_budget_bound` | 0 | 0 | 0 | — |
| null verifications | 1 | 3 | 2 | — |
| null-move cuts | 23,091 | 41,215 | 75,085 | 4.9% / 5.0% / 1.4% |
| ProbCut cuts | 38,218 | 70,758 | 186,203 | 8.0% / 8.6% / 3.4% |
| IIR reductions | 11,631 | 18,177 | 97,794 | 2.4% / 2.2% / 1.8% |
| `sing_seekmate_refused` | 15,461 | 34,724 | 155,293 | 3.3% / 4.2% / 2.9% |
| in-check nodes | 133,546 | 210,319 | 1,770,299 | 28.1% / 25.7% / 32.6% |

The ON ablation build to depth 19, one bit at a time, growth over the
OFF arm's per step 17 / 18 / 19 (screen rule, unablated OFF base):

| Mask (bit) | Growth 17 / 18 / 19 | Geomean | Mate found at | Cumulative nodes there |
|---|---|---|---|---|
| 0 (control) | 2.27 / 1.93 / 3.71 | 2.531 | depth 19 | 11,418,629 |
| 4 (null move) | 4.56 / 2.42 / 1.02 | 2.239 | depth 18 | 7,683,780 |
| 8 (ProbCut) | 2.02 / 5.15 / 2.35 | 2.900 | depth 19 | 17,951,189 |
| 16 (IIR) | 3.49 / 1.41 / 1.73 | 2.046 | not by 19 (cp 9448) | 13,829,026 at 19 |
| 64 (extensions) | 1.03 / 5.51 / 1.02 | 1.793 | depth 18 | 6,780,561 |

**What the reads say.**

- The singular block is idle in the failing iterations. No mechanism's
  share of interior nodes rises with the growth: null-move, ProbCut and
  IIR shares fall at iteration 19, and only the in-check share rises
  (the mate hunt).
- No single ablation removes the growth. Every masked arm keeps growing
  until it finds the mate, then flattens, and every one still reads above
  1.5.
- The growth on this position is the ON arm's approach to the mate the
  OFF arm found at depth 14. The ON arm reaches that mate at 0.38 of the
  OFF arm's nodes, and its growth ends there (read 2).
- Return 7's option (2) describes the reads. Option (1) has no mechanism
  to name.
- P11's falsifier, read literally ("the growth comes from outside the
  singular block"), is met. But the two mechanisms it names do not carry
  the growth either.
- What the clause does with an OFF arm that settled before depth 16, and
  whether the ticket-6b return applies, are research amendment 7's
  decisions. No clause, default or prediction was moved.

## Roadmap audit — B.3's remaining path (2026-11-01)

A read-only audit of Phase B's state and its remaining leaves, its
recommendations taken by the maintainer the same day. It is not research
amendment 7, which answers Return 7. It changes no engine code, no
default, no floor and no frozen prediction. The originals stand where
they are written; the clauses below are replaced, and the entry files of
BAS-S41 to BAS-S43 are assembled from §11 plus every *clauses replaced*
list (§11 now says so).

**What the audit found.**

- §9 said items 1-4 come first "because they decide what the paired run
  of the whole arm is measuring" while BAS-S41 listed the whole-arm run
  first: if item 1, 2 or 3 flipped a default, the whole-arm run and its
  −40 stop were read on a different arm.
- B-12 sat in PLAN §2's defects table, whose rule repairs a defect and
  refits its neighbours, while ticket 9b keeps the OFF form, B.3.3 fits
  the no-TT-move consumers around it and B.3.4 asks the repair to win
  against constants fitted for its absence. Amendment 6's evidence (the
  store's own product 7 of 83 IIR firings on the fixture line) reads a
  population calibration, not a wrong computation.
- B.14 ("ANY TIME, before C.0") moves both arms' fingerprints; landing it
  inside B.3.2 to B.3.5 puts the paired run, the categoricals, the fit and
  the gate on different heads, and the ON arm's primed 28 reads 27/28
  until it lands.
- BAS-S43 (b) priced "the fit" with a candidate that carries B.3.4's
  adoptions.
- ±5 at 2,000 games: BAS-S40 read ±5.2 after 5,134 games, so 2,000 games
  read about ±8 and a true-zero categorical crosses ±5 about one time in
  four; §9 has sixteen categoricals, B.3.4 re-read the inside-±5 ones
  and B.6.1 reads them again.
- P5 is outside its band on both reads (bench 0.40x, suite 0.26x) and
  unscored; the depth-6 WAC floor of 119 is the arm's own reading minus
  headroom, a regression guard, not a correctness floor.
- §11 is copied "verbatim" into the entry files, but amendment 3 moved the
  singular margin's sweep seed from 126 to 252 and PLAN renumbered the
  packet's B.3.4 to B.3.5; "double and triple bars 8" is nine X lines
  (`config_b3.json`'s twenty coordinates match `search_params.h`).
- The floors tool exits 1 on the OFF arm at 120,000 nodes on its
  staleness guard (KQ-K's mean 10.8 plies); the floors' generator admits
  positions with the side not to move in check (amendment 6 §3).

**Clauses replaced** (old → new; the originals stand as written):

- §9's preamble "The first four reads come first …" → items 1-3 precede
  the whole-arm run, which runs on the defaults they leave; items 4-16
  follow; items 6, 7, 8 and 11 run only on a measured population move
  (5.12's 10% rule); adoption and rejection need the 95% interval to
  exclude 0 as well (edited in place, the original quoted there).
- §4's B-12 row "a defect by PLAN §2's table …" → a gap, read by item 16
  (edited in place); PLAN §2's defects table loses B-12 and PLAN B.3's
  B-12 input records the reclassification.
- §11 BAS-S41 "The paired run … The categorical runs: §9's table in its
  order" → the zero-game screens with P5 scored at the opening (the
  mechanism named from ticket 10's ablation table); §9 items 1-3; the
  whole-arm paired run on the defaults they leave; items 4-16 as §9 now
  orders them; the floors tool's staleness exit told from a floor failure
  before the first read.
- §11 BAS-S42 "the double and triple bars 8" → 9.
- §11 BAS-S43 (b) "The fitted ON arm against the unfitted ON arm" → the
  fitted arm at B.3.3's bake revision, before B.3.4's adoptions, against
  the unfitted arm.
- PROCESS *Cluster delivery shape*, the categorical rule → adopted at
  ≥ +5 and rejected at ≤ −5 only when the 95% interval also excludes 0.
- PLAN B.3.4 → re-reads only a categorical whose mechanism's constants
  B.3.3 moved; adoption at ≥ +5 with the interval excluding 0.
- PLAN B.14 "before C.0; lands between leaves" → after B.3.1's accepted
  review and before B.3.2's first game.
- PLAN B.9 gains the floors' generator repair (between clusters, never
  inside one); PLAN B.3.1's ticket 10 record labels the 119 floor a
  regression guard.

**Not changed.** Every default, seed, floor, frozen prediction and
bracket; Return 7's state (B.3.1 `RESEARCH`, resuming on research
amendment 7) and its three discriminating reads; the independent review,
which still precedes B.3.2.

## Research amendment 7 — the cost screen's window: a late root resolution is not a deep-iteration explosion (2026-11-01)

This amendment answers *Return 7* and its addendum. It was written on the
maintainer's instruction of 2026-11-01 ("do the research and continue"),
in the session that implemented tickets 9b and 10 and wrote Return 7.
PROCESS's role table puts a research amendment at `R3`. This one is
treated as `R2`: it moves no mechanism, default, seed, floor or frozen
prediction, only the depths at which one qualification clause reads its
statistic, and the addendum's reads had already answered the causal
question. Both departures, the class and the session, are for the
independent review to weigh (§3).

### 0. What this says, plainly

- Position 50's fail is not an explosion. The ON arm finds at depth 19
  the `mate 9` that the OFF arm found at depth 14, using 0.38 of the OFF
  arm's nodes. No mechanism's share of the tree rises with the growth,
  no single ablation removes it, and it stops once the mate is found
  (growth 1.02–1.03 at depths 20–22).
- The clause compared the arms at matched depth. Amendment 2 §4 named why
  that is confounded between arms of different selectivity (the ON arm
  turns nodes into depth) and re-based the depth-6 WAC floor for it.
  Amendment 2 §3 took the level out of the screen but left the confound:
  when one arm resolves the root inside depths 16–19 and the other before
  them, their growth differs by the resolution.
- The position set made that likely. The three `suite_v2` positions were
  chosen as the OFF arm's three largest trees at depth 14, and on all
  three the OFF arm's tree jumps at depth 13 or 14 and then stops growing
  (the table in §R). A large depth-14 tree is a resolution event at
  depth 14. An arm that resolves the same point later reads its approach,
  inside 16–19, against a flat baseline.
- The clause keeps its statistic and its bound, but reads them where an
  explosion still shows and a resolution has ended: the last three steps
  of a read to depth 22, continued to depth 25 when those fail. The
  explosion the screen was built for (B-5/B-6: a ply bound that does not
  bound the tree) persists, so the later window loses no power against
  it, while a resolution hump ends.
- P11's frozen consequence for a fail on this arm is not taken. It reads
  "the growth comes from outside the singular block (ProbCut's 4.5% of
  cuts, the null move's hand-off band) and ticket 6b returns". The
  addendum reads both mechanisms' shares falling in the failing iteration
  and the growth surviving each one's ablation (2.90 with ProbCut off,
  2.24 with the null move off). The attribution the consequence rests on
  is refuted.

### R. The rule and the predictions, registered before running

Registered in this commit, before any depth past 19 is read on positions
1, 2, 4 and 5, and before the OFF arm is read past 19 on position 3. The
ON arm's depths 20–22 on position 3 are already exposed (the addendum),
so the rule scores position 3 but does not count it as evidence.

The OFF arm's per-iteration growth on the three `suite_v2` positions
(`t10/screen.json`):

| Position | Depth 13 | Depth 14 | Depths 15–19 |
|---|---|---|---|
| 3, `suite_v2` 50 | 4.41 | 3.11 | 1.000–1.002 |
| 4, `suite_v2` 42 | 3.60 | 9.07 | 1.000–1.011 |
| 5, `suite_v2` 60 | 1.80 | 6.28 | 1.001–1.003 |

**The amended clause.** It replaces amendment 2 §3's growth clause; its
purpose, catching a deep-iteration explosion, is unchanged.

- **Read.** `deep_iteration_screen.py` on §10's five positions, base
  `build/b3-off`, arm `build/b3-on`, `--depth-min 16 --depth-max 25`, the
  60 s watchdog, Hash 64. A step d is a depth both arms finish together
  with d − 1. Its growth ratio is the ON arm's cumulative-node growth
  from d − 1 to d over the OFF arm's.
- **(a)** The ON arm finishes every depth from 16 to 22 that the OFF arm
  finishes.
- **(b)** The geometric mean of the growth ratios at steps 20, 21 and 22
  is **≤ 1.5**.
- **(c)** Only when (b) fails: the geometric mean over the last three
  steps both arms finish at depths ≤ 25 is ≤ 1.5, and the ON arm finishes
  every depth from 23 to 25 that the OFF arm finishes.
- **(d)** Node counts decide, so the host need not be idle. A watchdog
  stop on either arm at a depth the rule judges (16–22, or 16–25 under
  (c)) makes that position inconclusive unless the host was idle; the
  read is then repeated on an idle host before any verdict. Load can only
  slow an iteration, so a pass on a busy host is also a pass on an idle
  one.
- **Recorded, not judged:** steps 17–19 (amendment 2's window) and the
  absolute ratio at depth 22. The verdict that `deep_iteration_screen.py`
  prints itself (the original absolute rule, ≤ 3) is not the verdict.
- **Scorer:** `t10/a7/screen_score_a7.py` (SHA-256 57c0ea35…). It was
  self-tested on five synthetic rows: a pass, a pass under (c), a
  sustained ×2 growth that fails, a stop beyond the judged depths, and a
  stop inside them.

**P34.** The amended screen passes on positions 1, 2, 4 and 5
(probability 70%). Falsifier: a fail under (b) and (c) on any of them
returns the leaf with the addendum's attribution reads on that position.
The rule is not re-scoped a second time unless a mechanism is named.

**P35.** Position 1 (bench 11) is the only position that needs (c): its
ON arm is still growing at depth 19 (1.47) while its OFF arm has
flattened (1.013) (probability 40%).

### 1. The result (2026-11-01)

**What ran.** §R's read, once, after the registration was committed
(`50a97b8`). The binaries are ticket 10's (`t10/a7/hashes.txt` equals
`t10/hashes.txt`). The JSON is `t10/a7/screen25.json` (SHA-256
f2ecc570…), scored by `t10/a7/screen_score_a7.py` into
`t10/a7/score.log`. On position 1 the scorer read **inconclusive** under
(d): the ON arm's watchdog fired at depth 21, the OFF arm's at 24, and
the host was not certified idle. The same binaries were then read on
position 1 with a 600 s per-iteration watchdog (`t10/a7/b11_reads.py`,
`b11_reads.json`, SHA-256 d2bb2f8f…). Those two iterations took 36.6 s
and 37.5 s there, so the host was loaded during the screen, and (d) did
what it was registered for.

| # | Position | Growth steps 20 / 21 / 22 | (b) geomean | (a), depths 16–22 | Steps 17–19 geomean (recorded) | Absolute at 22 (recorded) | Verdict |
|---|---|---|---|---|---|---|---|
| 1 | bench 11 | 4.47 / 6.63 / 3.22 (600 s read) | **4.57** | **fails at 22** | 1.147 | 66.2 | **FAIL** |
| 2 | bench 15 | 0.47 / 0.45 / 0.58 | 0.493 | holds | 0.727 | 0.019 | pass |
| 3 | `suite_v2` 50 | 1.02 / 1.02 / 1.03 | 1.020 | holds | 2.531 | 0.401 | pass (exposed; not evidence) |
| 4 | `suite_v2` 42 | 1.01 / 1.02 / 1.02 | 1.016 | holds | 1.003 | 0.251 | pass |
| 5 | `suite_v2` 60 | 1.15 / 1.16 / 1.18 | 1.163 | holds | 1.114 | 0.024 | pass |

**Position 1 fails on node counts, whatever the host.**

- (b) is a node-count statistic: 4.57 against 1.5, so (c) is required.
- (a) fails at depth 22. The ON arm's iteration 22 is 499,938,717 nodes
  (151 s at 3.29M nodes per second). At the highest single-thread rate
  any read on this host measured (3.38M), 60 s holds at most about 203M
  nodes. The OFF arm's iteration 22 is 2,845,117 nodes.
- The idle re-run that (d) asks for can only make iterations faster. It
  cannot finish a 500M-node iteration in 60 s, so it cannot turn the
  position into a pass, and the verdict is taken from the node counts.

**P34 scored:** missed: position 1 fails. **P35 scored:** hit: (b)
fails on position 1 alone, but (c) fails with it.

**What it says.** The window change does what §0 argued on position 3:
the resolution hump has ended by depths 20–22. The failure on position 1
is a different event. P34's falsifier applies: the leaf returns with the
attribution reads on position 1 (*Return 8*). Those reads name a
mechanism, so §R's bar on a second re-scope does not come into play, and
the next step is a mechanism amendment.

### 2. Clauses replaced, and what stands

- Amendment 2 §3's growth clause → §R's clause. It stands as registered,
  and Return 8 is a fail under it.
- Return 7's options → option (2) for position 50: its fail is the
  approach to a resolution the OFF arm reached earlier.
- P11's consequence ("ticket 6b returns") → not taken on position 50,
  where its attribution is refuted (§0). On position 1, Return 8 names
  ProbCut, one of the two mechanisms P11 named. P11's model was right
  about the arm's risk and wrong about which position would show it.
- Recorded for the later clusters, with no PLAN change: a screen
  position chosen by the base arm's tree size at one depth is likely a
  resolution event at that depth. B.4's and B.5's screens should read
  both arms past their resolutions.

### 3. The class and the session

This amendment was written as `R2`, in the implementing session, at the
maintainer's instruction (preamble). The independent review checks §R's
rule against §0's argument and the registration commit's order (rule
first, read after). Return 8's answer is a mechanism decision and goes
to research under PROCESS's class, `R3`, in a separate session.

## Return 8 — B.3.1 cheap qualification: the amended cost screen fails on bench position 11, and ProbCut carries it (2026-11-01)

**What ran.** Amendment 7's screen (above). Then, on bench position 11
(`2r3k1/1q2Rp1p/p2p2p1/1p1P4/1Pp1P3/2Q5/1P4PP/6K1 w`), deterministic
reads with a 600 s per-iteration watchdog (node counts are read, not
time):

- both arms' `info` lines, the ON arm to depth 22 and the OFF arm to 25
  (`t10/a7/b11_reads.py`, `b11_on.log`, `b11_off.log`);
- the Diag builds, one search per depth, so that an iteration's counters
  are the difference of two dumps: the ON arm at depths 19–22, the OFF
  arm at 21–24, each arm's growth event;
- the ON ablation build at depth 21 with masks 0, 4, 8, 16 and 64
  (`t10/a7/b11_attr.py`, `b11_attr.json`, SHA-256 b74502c7…).

Every Diag run and the mask-0 control reproduce the release arms'
per-depth nodes.

**Both arms, per depth** (cumulative nodes, score, seldepth):

| Depth | ON | OFF |
|---|---|---|
| 19 | 4,617,856, cp 2593, 52 | 6,663,683, cp 2424, 54 |
| 20 | 20,917,059, cp 2704, 65 | 6,746,421, cp 2424, 44 |
| 21 | 140,180,782, cp 2878, **128** | 6,820,051, cp 2424, 58 |
| 22 | 640,119,499, cp 3053, **128** | 9,665,168, cp 2462, 58 |
| 23 | — | 36,257,665, cp 2700, **128** |
| 24 | — | 157,177,066, cp 3106, 123 |
| 25 | — | 1,781,283,940, cp 4953, **128** |

Both arms play `e7b7` (Rxb7, winning the queen) throughout, with the
score past +24 pawns and climbing. At depth 24 the OFF arm switches to
the ON arm's reply `c8f8` (from `c8e8`).

- **ON:** grows 4.53×, 6.70× and 4.57× at depths 20–22 (138× in three
  iterations).
- **OFF:** grows 3.75×, 4.33× and 11.33× at depths 23–25 (184×).
- **Both:** lines reach the 128-ply maximum.

The OFF arm has the same kind of event, three plies later.

**The counters at the matched event** (ON iterations 20–22 against OFF
iterations 22–24, per interior node; the singular rows are the ON arm's
`B3SeekMateDepth` guard at work, since the score is far past
`B3SeekMateScore`):

| Counter | ON, iterations 20 / 21 / 22 | OFF, iterations 22 / 23 / 24 |
|---|---|---|
| interior nodes | 6.24M / 45.1M / 192.2M | 1.80M / 16.1M / 70.6M |
| singular tries | 0 / 0 / 0 (seekMate refused 64k, 213k, 423k) | 3.0% / 2.6% / 2.6% |
| ProbCut tries | **5.5% / 5.5% / 5.2%** | 1.3% / 1.3% / 1.2% |
| ProbCut cuts | 4.3% / 4.1% / 3.8% | 0.9% / 0.8% / 0.7% |
| quiescence nodes | **1.68 / 1.75 / 1.74** | 0.62 / 0.69 / 0.77 |
| quiescence evasion nodes | **23% / 24% / 22%** | 3.6% / 4.8% / 6.6% |
| razoring cuts | 6.7% / **10.9% / 14.6%** | 3.5% / 4.4% / 6.3% |
| LMR applied / re-searched | 37% / 6.2% (iteration 21) | 20% / 1.6% (iteration 23) |
| no TT move | 66% / 72% / 75% | 44% / 49% / 57% |
| null-move tries | 1.8% / 1.2% / 1.0% | 9.6% / 8.5% / 6.7% |
| `tt_pv` nodes | 0.17% / 0.06% / 0.02% | 4.6% / 4.1% / 4.5% |
| IIR reductions | 0.59% / 0.43% / 0.35% | 0.66% / 0.71% / 0.80% |
| `ext_budget_bound` | 0 | 0 |

**The ablations at depth 21** (ON ablation build; amendment 3's rule: a
switch names a mechanism when it moves the count by more than 3× either
way):

| Mask (bit) | Nodes at 21 | Against the control | Growth at 20 / 21 |
|---|---|---|---|
| 0 (control) | 140,180,782 | 1.00 | 4.53 / 6.70 |
| 4 (null move) | 123,792,413 | 0.88 | 2.48 / 3.55 |
| **8 (ProbCut)** | **22,919,965** | **0.16** | 1.36 / 3.27 |
| 16 (IIR) | 1,216,038,985 | 8.67 | 2.01 / 3.27 (the event moves to depth 19: 185M) |
| 64 (extensions) | 155,103,128 | 1.11 | 3.17 / 4.59 |

**What the reads say.**

- ProbCut is named. It is the only ablation that shrinks the tree by
  more than 3× (6.1×), and on the ON arm it is tried at four times the
  OFF arm's share at the matched event.
- With ProbCut off, the ON arm's event starts a ply later and smaller
  (3.27 at depth 21). Its tree at 21 (22.9M) is still 3.4× the OFF arm's
  (6.8M), so ProbCut is not the whole difference.
- IIR off makes the event earlier and larger, so IIR restrains it.
- The singular block is silent (seekMate), and the extension budget
  never binds.
- The ON arm's quiescence load (1.7 quiescence nodes per interior node,
  a quarter of them evasions) and its rising razoring share also set it
  apart. That ProbCut's quiescence pre-test produces this load is an
  inference that has not been measured.

**What it does not say.**

- Whether ProbCut's form, its gate or its constants carry the growth.
- Whether the OFF arm's later event (184× over 23–25) is the same
  position-driven event, which ProbCut only brings forward three plies.
- Whether either arm's event costs anything in a game. The position is
  won by more than 24 pawns on every read.

Amendment 3 recorded this position's response to the singular block's
constants as chaotic (DESIGN §5). The same may hold for ProbCut, and a
read of one constant here would not choose a seed.

**Options for research.**

1. *ProbCut in the won-position regime.* The donor's seekMate guard
   (root depth ≥ `B3SeekMateDepth`, |score| ≥ `B3SeekMateScore`) turns
   the singular block off and caps reverse futility, but leaves ProbCut
   on. Research reads the donor's ProbCut gate at `edb0d9db` against
   Basilisk's port (`B3ProbCutBase` 68, `B3ProbCutPv` 1, the min depth
   3) to decide whether the port differs from the donor's form, or
   whether the donor's form needs a guard for this regime.
2. *ProbCut's constants.* These are B.3.3's sweep, but only after a
   qualification that passes, which this does not give.
3. *A shape finding.* Both arms have the event, and ProbCut brings it
   forward. That is a second re-scope, which amendment 7 §R bars unless
   a mechanism is named. A mechanism is now named, so research answers
   the mechanism first.

**State.** B.3.1 returns to `RESEARCH` and resumes on research amendment
8. Every other qualification read passes (the four other screen
positions and every read of *Implementation record — tickets 6b to 10*).
No default, clause, floor or frozen prediction was moved.

## Research amendment 8 — ProbCut in the conversion band: the position's event, three plies earlier (2026-11-01)

Answers *Return 8*. Class `R3`, in a separate session from the one that
implemented tickets 9b and 10 and wrote Returns 7 and 8 (PROCESS's role
table). No engine code is changed in the committed tree and no default,
seed, floor, clause or frozen prediction is moved here; every read is a
deterministic node-count or counter read on the ticket-10 binaries
(`t10/a7/hashes.txt`) or on the Tune build's options, with a 600 s
per-iteration watchdog that only stops a runaway. No timed read, no game.
Recipes, logs and JSON: `tools/results/b31-20261030/amend8/` (`a8_r1.py`,
`a8_r2.py`, `a8_*.log`, `a8_*.json`). This section's §R was committed
before any read ran (`45fca15`); §0 and §1 onward were written after.

### R. Hypotheses, reads and predictions, registered before running

**What Return 8 established.** On bench position 11 the ON arm's tree
grows 4.53×, 6.70× and 4.57× at depths 20–22 and the OFF arm's 3.75×,
4.33× and 11.33× at depths 23–25; ProbCut off (ablation bit 3) leaves the
ON arm's depth-21 tree at 0.16×; the singular block is silent under
`seekMate`; the extension budget never binds. What the logs add, read
before this registration (`t10/a7/b11_*.log`, the ablation logs):

- Every arm's event begins in the iteration whose root score climbs out
  of the 2,400–2,600 plateau. ON: 2,593 → 2,704 (depth 20, growth 4.53).
  OFF: 2,462 → 2,700 (depth 23, 3.75). Bit 8 (ProbCut off): 2,716 at 20
  (1.36), 2,985 at 21 (3.27). Bit 4: 2,711 at 20 (2.48), 2,884 at 21
  (3.55). Bit 16: 2,837 at 19 (9.3). Bit 64: 2,695 at 20 (3.17). Before
  the climb no arm grows more than 2.2× at a depth ≥ 17.
- Both arms' PVs carry the same conversion: `...Rf8`, `a5 bxa5`, the
  d-pawn to d7, with the score rising 100–400 cp an iteration and lines to
  the 128-ply maximum; the OFF arm's depth 25 reads 4,953.
- The iterations fail high four to seven times each (`asp_fail_high` 4 /
  5 / 5 on ON at 20–22; 2 / 5 / 7 on OFF at 22–24), and the ON arm's
  last window of depth 21 (beta 2,971 after five fail-highs, settling at
  2,878) is 74M of the iteration's 119M nodes. Basilisk re-searches a root
  fail-high at full depth (BAS-D17; the donor reduces by the fail-high
  count), so a climbing score costs a full tree per widening.
- Per interior node at the matched event the ON arm runs 1.7 quiescence
  nodes (OFF 0.6–0.8), 23% of them evasions (OFF 4–7%), tries ProbCut at
  5.5% (1.3%) and cuts at 4.2% (0.8%); its null-move tries at depth ≥ 8
  are 0.03–0.13% (OFF 1.0–1.6%), its RFP cuts 6% (10%, the legacy RFP
  capped at depth 10 on both arms), its TT-move-absent share 66–75%
  (44–57%).
- The port is the donor's step 12 line for line (`edb0d9db` search.cpp
  `:1046-1093` against kernel `:924-963`): `improving |= staticEval ≥
  beta` before it; `beta + 241 − 64·improving` converted to `68 − 18`;
  depth ≥ 3; the valid-TT-value guard; the picker over captures and queen
  promotions by capture history with the TT move first and SEE at
  `probCutBeta − staticEval`; the quiescence pre-test; the reduced search
  at `depth − (improving ? 5 : 3)` with `!cutNode`; the store at
  `probCutDepth + 1` as a lower bound with the value; the return of
  `value − (probCutBeta − beta)` unless decisive. The donor's step 13 (the
  small ProbCut: a lower bound at `ttDepth ≥ depth − 4` clearing
  `beta + 428` returns `beta + 428`) consumes exactly the entries step 12
  stores; Basilisk carries it as `B3ProbCutTt`, off (amendment 2 §2).

**Hypotheses.**

- *H1, the port.* Basilisk's ProbCut differs from the donor's form in a
  way this regime exposes. The reading above says no; the reads below
  can only reopen it if a constant at the donor's raw value behaves
  unlike the converted one in a way the units do not explain.
- *H2, the constants.* A ProbCut constant inside its sweep bracket
  (`B3ProbCutBase` 34–240, `B3ProbCutImproving` 0–48,
  `B3ProbCutMinDepth` 2–8, `B3ProbCutDepth` 2–6,
  `B3ProbCutDepthImproving` 3–8, `B3ProbCutPv`) carries the growth: a
  setting other than switching ProbCut off removes the ON arm's event at
  depth 21.
- *H3, the regime.* The donor's form, in a position Basilisk's
  material-scale evaluation reads as climbing with depth, multiplies the
  cost of the climb: each ProbCut proof is a quiescence search of a
  checking capture (evasions) and a reduced search, stored two to four
  plies shallow, so it is re-proven at every widened window and every
  iteration, and the donor's consumer of those stores (step 13) is off.
  Under H3 no in-bracket constant removes the event; switching ProbCut
  off (bit 8, or `B3ProbCutMinDepth` 8) does; and the small ProbCut on
  shrinks it.
- *H4, the position.* The event is the conversion band of a position won
  by more than 24 pawns under an evaluation with no won-position
  compression: every arm has it in the iterations where its score climbs
  past about 2,650, and what ProbCut (or any mechanism that reaches the
  band at a lower nominal depth) moves is the depth at which it starts,
  not whether it happens. H3 and H4 are not exclusive: H4 says both arms
  have the event; H3 says ProbCut makes the ON arm's iterations in it
  costlier.

**Reads** (bench position 11, Hash 64, `ucinewgame`, `go depth N`, the
600 s watchdog; node counts and counters are read, never time):

- *R1* (`a8_r1.py`): `build/b3-on-ablate`, `AblationMask 8`, to depth
  23, every line kept. Identity: depths 1–21 reproduce `b11_ablate_m8.log`.
- *R2* (`a8_r2.py`): `build/b3-on-tune` with `Diag true`, one search to
  depth 21 per setting, counters from the dump, the depth-20 dump of the
  control for iteration differences where needed:
  control (defaults; identity: 140,180,782 at 21, every depth equal to
  `b11_on.log`);
  (a) `B3ProbCutMinDepth 5` (the legacy depth gate);
  (b) `B3ProbCutBase 189`, `B3ProbCutImproving 0` (the legacy margin);
  (c) `B3ProbCutBase 240`, `B3ProbCutImproving 48` (the bracket's top,
  the donor's raw 241/64);
  (d) `B3ProbCutDepth 4`, `B3ProbCutDepthImproving 4` (the legacy
  reduction);
  (e) `B3ProbCutPv 0` (§9 item 8);
  (f) `B3ProbCutMinDepth 8` (the bracket's top: ProbCut nearly off);
  (g) `B3ProbCutTt 1` (the small ProbCut, the donor's step 13).
  Rule (amendment 3 §2's, frozen there): a setting names a carrier when
  it moves the depth-21 count by more than 3× either way (≤ 46.7M or
  ≥ 420.5M against 140.2M). Recorded per setting: nodes at 19, 20, 21,
  growth at 20 and 21, the score path and fail-high count, and per 100
  interior nodes `probcut_tries`, `probcut_cuts`, `probcut_tt_cuts`,
  `qs_nodes`, `qs_evasion_nodes`, `razor_cuts`, `null_tries`,
  `tt_move_absent`.
- *R3*, conditional: only if R2 leaves no in-bracket setting under 0.33×
  while (f) is under it, a temporary release build of the ON arm with
  ProbCut standing down under `seek_mate` (the guard the singular block
  already obeys), read at depths 19–23 on position 11 and then priced with
  the full binding set (§R's last paragraph). Its edit, patch and restore
  follow `amend4/scripts/variant.py`.

**Predictions.**

- **P36.** R1's arm has the band event: a step with growth ≥ 3 at depth
  22 or 23, ending with a root score ≥ 2,650 (probability 80%).
  Falsifier: both steps under 2 — then the ON arm's event is ProbCut's,
  not the position's, and H4 falls.
- **P37.** In R2 none of (a)–(d) moves the depth-21 count by more than
  3× either way (65%); (e) moves it by under 1% (90%; `probcut_pv_tries`
  read 0, 0, 3 in iterations 20–22); (f) moves it under 0.33× (85%; the
  ablation's 0.16 within DESIGN §5's chaos).
- **P38.** (g) moves the depth-21 count under 0.33× (60%): the donor's
  consumer of ProbCut's stores short-circuits the re-proofs.
- **P39.** Across R2's settings, `qs_evasion_nodes` per 100 interior
  nodes falls by at least half in every setting whose `probcut_tries`
  per 100 interior nodes fall by 3× or more (70%): the pre-test's
  checking captures carry the quiescence load.
- **P40.** In R1 and R2 every iteration at depth ≥ 19 with growth ≥ 3
  ends with a root score ≥ 2,650 (90%).

**Pricing, if a candidate default is left.** `bench 13` on both arms
(the OFF arm 16,615,757), WAC at 100,000 nodes ≥ 232 and the depth-6
regression guard 119, the conversion floors at five budgets (KQ-K, KR-K,
KBB-K 24 each; KBN-K sum ≥ 112), the promotion fixture at six budgets
(a8=Q, positive at 100,000), the 28 wrong-bishop reads, and amendment 7's
screen on all five positions (node counts decide; a watchdog stop on
this host is inconclusive, not a fail).

### 0. What the reads say, plainly

- The blowup on bench position 11 is the position's, not ProbCut's. It
  is the conversion band of a game won by more than 24 pawns under an
  evaluation that reads material and nothing of "won": the root score
  climbs 100–400 cp an iteration from about 2,650 up, every iteration
  fails high four to eight times, each widened window is re-searched at
  full depth (BAS-D17), and lines run to the 128-ply maximum. Every arm
  has it in the iterations where its own score enters the band: the ON
  arm at 20–22, the OFF arm at 23–25, the ON arm with ProbCut off at
  21–22 (3.27×, 3.62×; P36 hit), and every one of seven ProbCut settings
  at some step between 19 and 21 (§3). Every iteration with growth ≥ 3
  at depth ≥ 19, in every arm, ends with a root score of 2,704 or more
  (P40 hit).
- ProbCut's port is the donor's step 12 line for line (§1). Its
  constants do not carry the event, and the depth-21 count cannot read
  them: inside the sweep bracket the count moves from 0.24× to 4.1×
  without monotony (the legacy margin 189 reads 3.8×, the donor's raw
  240/48 reads 0.48×, the legacy depth gate 5 reads 0.33×, the legacy
  reduction 4/4 reads 4.1×; P37's first clause missed), and each count
  is where that arm stands in the band (root scores 2,776 to 3,001 at
  depth 21), not whether it has the event. DESIGN §5's chaos, as
  amendment 3 §2 read for the singular block on this same position.
- What ProbCut does here is move the ON arm's entry into the band three
  plies earlier and make its passage costlier: with ProbCut off the arm
  reaches 2,985 at depth 21 for 22.9M nodes and 3,714 at depth 23 for
  93.6M, against the control's 2,878 for 140.2M and 3,053 for 640.1M.
  The quiescence load Return 8 pinned on ProbCut's pre-test is not its:
  with ProbCut tried at 0.22% of interior nodes the arm still runs 1.24
  quiescence nodes per interior node, 16.5% of them evasions (P39
  missed). The channel that remains, proofs stored two to four plies
  shallow and re-proven at every widened window, is consistent with the
  counts and unmeasured (§4); it is a property of the donor's form met
  by Basilisk's evaluation scale and full-depth re-search, which no
  Cluster 2 constant repairs and which the donor's step 13 does not
  relieve here (0.79×; P38 missed).
- The screen cannot read this position. Amendment 7's clause compares
  the arms' per-iteration growth at matched depth; on position 11 both
  arms have a position-driven event, and the clause reads which arm
  enters it first. An arm that reaches the band's information at a lower
  nominal depth, which is what a more selective arm does, fails; the
  ProbCut-off arm would fail (b) too (ratios 1.35, 3.24, 2.55 at 20–22).
  The screen's purpose, catching a ply bound that does not bound the tree
  (B-5/B-6), is served by positions that are not decisively won.
- Recommended: no default, seed or clause moves; position 11 leaves the
  screen's position set for a position the rule of §5 chooses, and its
  band reads are recorded in the two-arm table, not judged; B.3.1
  resumes at ticket 10b (the screen on the new set) on the maintainer's
  decision (§6). The alternative, ProbCut standing down under `seekMate`
  as the singular block does, is priced in prospect and not recommended:
  it would be a departure from the donor's form taken on one +24 position
  where no admissible setting avoids the band, it would still fail the
  clause as written, and the question it answers (ProbCut's cost in won
  positions at game depths) is B.3.2's paired run's and the Diag read's,
  which §6 extends by one row.

### 1. The port (H1): the donor's step 12, line for line

Read side by side (`edb0d9db` search.cpp `:1046-1093`, kernel
`:924-963`), with §R's list: the `improving` OR before the block, the
margin converted at B.0 §2.4's factor (241 → 68, 64 → 18), the depth gate
3, the valid-TT-value guard, the picker (captures and queen promotions by
capture history, the TT move first, SEE at the margin less the static
eval), the quiescence pre-test, the reduced search at `depth − (improving
? 5 : 3)` as `!cut_node`, the store at `pc_depth + 1` as a lower bound
with the value, the return of `value − (pc_beta − beta)` unless decisive.
Two differences, neither reachable here: the port skips the root (one
node), and `is_decisive` reads Basilisk's tablebase band (`search_types.h
:144`) as the donor's does its own; no score on this position is within
either. Read (e) confirms the PV admission is inert on this position
(identical count, `probcut_pv_tries` 0 to depth 21). H1 is closed: the
form is the donor's.

### 2. The event is the position's (H4)

**R1** (`a8_r1_m8_d23.log`, identity with `b11_ablate_m8.log` at depths
1–21): the ON arm with ProbCut off.

| Depth | Nodes | Growth | Root score | Fail-highs |
|---|---:|---:|---:|---:|
| 19 | 5,165,192 | 1.17 | 2,608 | 0 |
| 20 | 7,015,730 | 1.36 | 2,716 | 0 |
| 21 | 22,919,965 | **3.27** | 2,985 | 5 |
| 22 | 83,036,567 | **3.62** | 3,307 | 8 |
| 23 | 93,608,334 | 1.13 | 3,714 | 7 |

The band on every arm read on this position (the iteration where growth
first reaches 3, and the root score it ends with; the ON arm's settings
are §3's):

| Arm | First step ≥ 3 | Score before → after | Steps in the band (growth) |
|---|---|---|---|
| OFF | 23 | 2,462 → 2,700 | 23–25 (3.75, 4.33, 11.33) |
| ON control | 20 | 2,593 → 2,704 | 20–22 (4.53, 6.70, 4.57) |
| ON, bit 8 (ProbCut off) | 21 | 2,716 → 2,985 | 21–22 (3.27, 3.62); 1.13 at 23 |
| ON, bit 4 (null move off) | 21 | 2,711 → 2,884 | (2.48 at 20) 21 (3.55) |
| ON, bit 16 (IIR off) | 19 | 2,640 → 2,837 | 19 (9.3), 21 (3.27) |
| ON, bit 64 | 20 | 2,585 → 2,695 | 20–21 (3.17, 4.59) |
| ON, (a) depth gate 5 | 19 | 2,603 → 2,726 | 19 (4.18) |
| ON, (b) margin 189 | 20 | 2,685 → 2,861 | 20 (13.78) |
| ON, (c) margin 240/48 | — (2.97 at 20) | 2,713 → 2,979 | 20 (2.97) |
| ON, (d) reduction 4/4 | 20 | 2,671 → 2,778 | 20–21 (5.05, 6.81) |
| ON, (f) depth gate 8 | 20 | 2,666 → 2,839 | 20 (3.91) |
| ON, (g) small ProbCut on | 20 | 2,560 → 2,735 | 20–21 (4.78, 4.16) |

No arm grows 3× at a depth ≥ 17 before its score reaches 2,650, and
every arm grows 3× (or 2.97×) in the iteration its score passes about
2,700. Both arms' principal variations carry one plan: `...Rf8`, `a5
bxa5`, the d-pawn to d7, Black's pawns falling; the ON arm's depth-22
line and the OFF arm's depth-24 line differ in Black's h-pawn tempi only.
The OFF arm's event at 23–25 is the same event; the ON arm enters it
three plies earlier because it reaches the band's information at a lower
nominal depth (its depth-19 tree is 4.6M nodes against the OFF arm's
6.7M, with the score 169 cp higher), as amendment 7 §0 read on position
50 in the other direction. P36 and P40 hit.

### 3. The constants (H2): chaotic, and a phase reading

**R2** (`a8_r2_*_d21.log`, `a8_r2.json`; the control reproduces
`b11_on.log` at every depth; counters per 100 interior nodes over the
whole search to depth 21):

| Setting | Nodes at 19 / 20 / 21 | Growth 20 / 21 | Score at 21 | × control | ProbCut tries / cuts | Quiescence / evasions | Razoring |
|---|---|---|---:|---:|---|---|---:|
| control | 4.62M / 20.92M / 140.18M | 4.53 / 6.70 | 2,878 | 1.000 | 5.54 / 4.23 | 171 / 23.4 | 10.1 |
| (a) `B3ProbCutMinDepth` 5 | 17.31M / 21.20M / 46.42M | 1.23 / 2.19 | 2,776 | **0.331** | 1.28 / 1.02 | 143 / 19.5 | 8.7 |
| (b) `B3ProbCutBase` 189, `Improving` 0 | 17.29M / 238.26M / 529.11M | 13.78 / 2.22 | 2,969 | **3.775** | 3.58 / 2.44 | 172 / 22.2 | 13.4 |
| (c) `B3ProbCutBase` 240, `Improving` 48 | 11.33M / 33.59M / 67.77M | 2.97 / 2.02 | 3,001 | 0.483 | 3.76 / 2.70 | 127 / 18.3 | 10.7 |
| (d) `B3ProbCutDepth` 4, `DepthImproving` 4 | 16.69M / 84.24M / 573.39M | 5.05 / 6.81 | 2,955 | **4.090** | 4.93 / 3.61 | 178 / 22.9 | 13.5 |
| (e) `B3ProbCutPv` 0 | = control | = control | 2,878 | 1.000 | 5.54 / 4.23 | 171 / 23.4 | 10.1 |
| (f) `B3ProbCutMinDepth` 8 | 4.72M / 18.44M / 33.30M | 3.91 / 1.81 | 2,869 | **0.238** | 0.22 / 0.18 | 124 / 16.5 | 8.4 |
| (g) `B3ProbCutTt` 1 | 5.59M / 26.74M / 111.22M | 4.78 / 4.16 | 2,816 | 0.793 | 5.90 / 4.56 (+1.15 small) | 169 / 23.4 | 8.9 |

- Amendment 3 §2's rule names a carrier when a switch moves the count by
  more than 3× either way. Four settings do, in both directions: (a) and
  (f) under 0.33×, (b) and (d) over 3×, with the count non-monotone in
  the margin (68: 1.0; 189: 3.8; 240: 0.48), in the depth gate (3: 1.0;
  5: 0.33; 8: 0.24) and in the reduction (3/5: 1.0; 4/4: 4.1). This is
  the chaos amendment 3 §2 recorded for the negative extension on this
  position (0: 0.7M, 1: 236M, 3: 57M), and the same conclusion holds: a
  read of one constant here chooses nothing. P37's first clause missed;
  its second and third hit.
- The counts are phase readings. Every setting has the band (§2's table);
  what differs is which iteration the climb lands in and how far the arm
  is through it at depth 21: (a) spent its 4.18× at depth 19 and sits at
  2,776; (c) climbed to 2,979 at depth 20 for 33.6M and is at 3,001; (b)
  climbed 13.8× at depth 20. Read at matched information instead, the
  nodes to the first completed iteration with a root score ≥ 2,860:
  ProbCut off 22.9M (2,985), (f) 33.3M (2,869), (c) 33.6M (2,979 at 20),
  control 140.2M (2,878), (b) 238.3M (2,861 at 20), (d) 573.4M (2,955),
  (a) and (g) not reached by depth 21; the OFF arm 157.2M (3,106 at 24).
  The ordering is not the sweep's bracket order either way.
- The quiescence load is the ON arm's, not ProbCut's. Across settings
  the tries move 25-fold (0.22 to 5.90 per 100 interior nodes) while
  quiescence nodes per interior node move from 1.24 to 1.78 and the
  evasion share from 16.5% to 23.9%; (f) keeps 72% of the control's
  quiescence and 71% of its evasions with 4% of its ProbCut tries. P39
  missed. The load's producers are the legacy razoring at depths 1–3
  (8.4–13.5 cuts per 100 interior nodes, each a quiescence call, against
  the OFF arm's 3.5–6.3) and the arm's leaves under the donor's move-loop
  pruning (capture futility 21.5, quiet SEE 17 per 100, against 5–7 and
  4–8 on the OFF arm); Return 8's inference is withdrawn.
- The small ProbCut (g) does not relieve the band: 1.15 cuts per 100
  interior nodes, the count 0.79×, the score 2,816 (lower than the
  control's). The stored values from step 12 (≥ beta + 68 at the window
  they were proven in) rarely clear `beta + 240` at the widened windows
  that follow. P38 missed; amendment 2 §2's decision stands.

### 4. ProbCut in the regime (H3): what stands, what does not, and why it is not a port defect

- What stands: on this position the arm without ProbCut is the cheapest
  arm through the band (§3's matched-information list), and the control
  the costliest ON arm but two. ProbCut's cost here is real and is not a
  defect of the port.
- What does not stand: the pre-test's quiescence as the channel (P39).
- The channel left, unmeasured: a ProbCut cut stores its proof at
  `pc_depth + 1` (two to four plies shallower than the node), so on the
  next widened window the entry serves no cutoff, the valid-TT-value
  guard admits ProbCut again whenever the stored value still clears the
  new margin, and the proof (a quiescence search and a reduced search)
  is run again; a normally cut node is served by its full-depth lower
  bound for free. With five to eight widened windows an iteration, each
  re-searched at full depth (BAS-D17's refusal of the donor's
  `failedHighCnt` reduction, kept), every ProbCut-cut node near the
  principal variation pays its proof that many times. The counter that
  would measure it is per-window `probcut_tries` at nodes whose TT entry
  is a ProbCut store; it is not needed for this amendment's decision and
  is left to B.3.2's Diag read if the paired run asks (§6).
- Why no Cluster 2 constant repairs it: the regime is made by two
  Basilisk partners, the evaluation's material scale (a +24 position
  climbs 100–400 cp a ply pair with no won-position compression; the
  donor's NNUE output saturates) and the root's full-depth re-search of
  every widened window. The donor's own answers are those two, plus
  `seekMate`'s RFP cap (B.4's). Cluster 2's share is `seekMate`, which
  already stands the singular block down; extending it to ProbCut is the
  one Cluster 2 form that would move this position (it reads as bit 8 at
  root depth ≥ 12, which is where every band iteration is), and it is a
  departure from the donor priced in §6.
- Game relevance: at this position the ON arm enters the band at depth
  20 after 20.9M nodes, beyond the nodes a `3+0.03` move has, with the
  move decided at depth 1 and the game won. Whether ProbCut's form costs
  in won positions at game depths is a question the floors have already
  answered for the pure endings (24 at every budget, KBN-K 120) and the
  paired run answers for the rest; §6 adds the Diag row that lets the
  ablation-by-switch name it if that run is negative.

### 5. What the screen can read

- Amendment 7's clause is right for its purpose and wrong for this
  position: it reads growth ratios at matched depth, and on a position
  where both arms have a position-driven event it reads only which arm
  enters first. An arm that is better informed at a nominal depth enters
  first and fails; here even the ProbCut-off arm fails (b) (ratios 1.35,
  3.24, 2.55 at steps 20–22, geometric mean 2.2), and under (c) the
  verdict would turn on whether two position-driven events happen to
  align in depth, which is not a mechanism reading. Amendment 7 §2
  recorded the resolution-event form of this confound for the later
  clusters; the conversion-band form is its twin, and the rule below
  excludes both at selection.
- The replacement rule, for ticket 10b: from the 40 bench positions and
  the 106 `suite_v2` positions, the OFF arm's largest depth-14 tree among
  those whose depth-14 root score is within ±564 (`B3SeekMateScore`: the
  arm's own won-position line, so every ON mechanism is live on it) and
  whose OFF tree grows less than 2× at both depths 13 and 14 (not a
  resolution event). The data is `amend8/a8_select.json` (the OFF arm,
  Hash 64, `go depth 14`, deterministic); the choice is in §6. Position
  11 stays in the record as the conversion-band read, with both arms'
  and the ProbCut-off arm's band tables (§2) in the two-arm table,
  recorded and not judged.
- The rule changes the screen's position set, not its clause: §R's
  read, (a)–(d), the scorer and the bound are unchanged, and P34's
  consequence (a fail returns the leaf with the attribution reads) stands
  on the new set.

### 6. Decisions, the recommendation, the amended ticket and the condition

**Research decides (its own):**

1. Return 8's attribution is corrected: ProbCut does not carry the
   event; the position does, and ProbCut moves the ON arm's entry into
   it three plies earlier and makes its passage costlier (§2–§4). Its
   quiescence inference is withdrawn (§3).
2. H1 is closed (§1). H2 is answered: no in-bracket constant removes the
   event, and the depth-21 count on this position is a phase reading
   that chooses nothing (§3); `B3ProbCutBase`, `B3ProbCutImproving`,
   `B3ProbCutMinDepth`, `B3ProbCutDepth` and `B3ProbCutDepthImproving`
   stay on B.3.3's sweep at their seeds, read by the fit on games.
3. Recorded for the roadmap, with no PLAN change here: the
   conversion-band event of a decisively won position under the
   material-scale evaluation (both arms, 138× and 184× over three
   iterations, lines to the ply maximum) is a property of the evaluation
   and of the root's full-depth re-search, owned by no Cluster 2 leaf.
   Position 11 at depths ≥ 20 is a test position for B.5's aspiration
   work (B-76: the fail-high depth reduction BAS-D17 refused) and for
   the evaluation programme's won-position scale; the donor-form re-read
   when NNUE lands asks whether the band exists under a saturating
   evaluation.
4. BAS-S41's Diag read gains one row, no games: the ON arm's
   `probcut_tries`, `probcut_cuts`, `qs_nodes`, `qs_evasion_nodes` and
   `razor_cuts` per interior node on the subset of the 40 bench positions
   whose OFF root score at depth 14 is at or beyond ±564, beside the
   whole-suite shares, so that a negative paired run's ablation by switch
   can name ProbCut's won-position cost or clear it.

**The maintainer decides, with the alternative and the main
counter-argument:**

1. **The screen's position set** (§5): bench position 11 leaves the set
   and `suite_v2` position 12
   (`rn2kb1r/pp2pppp/2p5/3q4/3P1Bb1/5NP1/PPP2P1P/R2QKB1R w`, the OFF
   arm's largest depth-14 tree under the rule: 746,048 nodes, +104,
   growth 1.96 and 1.27 at depths 13 and 14) takes its place; position
   11's band tables (§2) go to the two-arm table as the conversion-band
   read, recorded and not judged; the clause, scorer and bound of
   amendment 7 §R are unchanged. Recommended: the mechanism question is
   answered, the clause cannot read a position where both arms have a
   position-driven event, and the screen's purpose is served by a
   position where every ON mechanism is live. Alternative: keep position
   11 and take its fail as the leaf's verdict, which returns B.3 to
   research as a whole with no Cluster 2 clause to amend. Counter-
   argument: this is the screen's second re-scope and the set's four
   other positions are exposed passes (three resolve to `mate 9` at
   depth 14 and bench 15 reads −1,025, all outside the rule). The answer
   is that §R barred a re-scope only until a mechanism was named and
   answered, the four passes stand as recorded reads whatever the rule
   would have chosen, and the rule is written down for B.4's and B.5's
   screens before their positions are picked.
2. **ProbCut's form.** Unchanged: the donor's step 12 at its seeds.
   Alternative: ProbCut standing down under `seek_mate` as the singular
   block does (kernel `:931`, one condition; a Basilisk departure from
   the donor's form, which leaves step 12 on under `seekMate`), either as
   a default or as a §9 categorical `B3ProbCutSeekMate` (0 the donor's
   form, 1 standing down) read by a paired run. Not recommended now: on
   this position it reads as bit 8 (the band at 21–22, 3.27× and 3.62×,
   and (b) still failed), the departure would rest on one +24 position
   beyond game depth, and the question it answers is read without a
   code change by decision 4's row and the paired run. If chosen, its
   price is the binding set of §R's last paragraph on a temporary build
   (`amend8/a8_r3.py`, written and not run; R3's condition in §R was not
   met, since (a) read 0.331); the switch form moves no fingerprint.
   Retry trigger: B.3.2's paired run negative with the ablation by
   switch pointing at ProbCut and decision 4's row showing its
   won-position shares above the whole-suite shares, or the oracle
   differential showing conversion losses in won middlegames.

**The amended ticket text implementation resumes on** (ticket 10b, no
engine code):

- Replace the first line of `t10/screen_positions.epd` with `suite_v2`
  position 12's FEN; record the rule, the chosen position's depth-14
  numbers and `amend8/a8_select.json` beside the two-arm table.
- Run amendment 7 §R's read on the five positions (`--depth-min 16
  --depth-max 25`, the 60 s watchdog, Hash 64, `t10/a7/screen_score_a7.py`);
  the verdict is the scorer's under (a)–(d); a watchdog stop on a busy
  host is inconclusive and the position is re-read on an idle host.
- Add position 11's band tables (§2: ON 20–22, OFF 23–25, bit 8 to 23)
  to the two-arm table as the conversion-band read, recorded and not
  judged.
- A pass completes the cheap qualification; the review follows as §10's
  *Review* states, with this amendment's §1–§4 among what it checks. A
  fail returns the leaf with the attribution reads on the failing
  position (P34's consequence, on the new set).

**P41.** The amended screen passes on `suite_v2` position 12 (probability
75%); the other four positions' reads are already exposed passes and are
not evidence. Falsifier: a fail under (b) and (c) on it, which returns
the leaf as above.

**Condition for resuming:** the maintainer's decision 1 (and 2, if the
alternative is taken, which adds its pricing before ticket 10b). B.3.1
stays `RESEARCH` until then and resumes `READY_FOR_IMPLEMENTATION` at
ticket 10b.

**Maintainer's decision, 2026-11-01:** decision 1 taken as recommended
(`suite_v2` position 12 replaces bench position 11 in the screen's set;
position 11's band tables are recorded, not judged); decision 2 not
taken (ProbCut's form unchanged; the guard variant is not priced, its
scripts stay under `amend8/` against the retry trigger). B.3.1 is
`READY_FOR_IMPLEMENTATION` at ticket 10b.

### 7. Clauses replaced, predictions scored and calibration

**Clauses replaced** (old → new; the originals stand as written):

- §10 *Cheap qualification*, the screen's positions ("bench positions 11
  and 15 and `suite_v2` positions 50, 42 and 60") → "`suite_v2` position
  12, bench position 15 and `suite_v2` positions 50, 42 and 60; bench
  position 11 recorded as the conversion-band read, not judged" (on
  maintainer decision 1; the clause itself is amendment 7 §R's,
  unchanged).
- Return 8 "ProbCut carries it" → "the position carries it; ProbCut
  moves the ON arm's entry three plies earlier and its passage dearer;
  no in-bracket constant removes it" (§2–§4).
- §11 BAS-S41's Diag read gains the won-position row of decision 4.
- Recorded for B.4's and B.5's screens (with amendment 7 §2's note): a
  screen position is chosen with the OFF arm's depth-14 root score within
  ±564 and its tree growing less than 2× at depths 13 and 14.

**Predictions scored.**

| | Registered | Result | Score |
|---|---|---|---|
| P36 | bit 8 has the band at 22 or 23, ending ≥ 2,650 (80%) | 3.62 at 22, 3,307 | hit |
| P37 | no (a)–(d) beyond 3× (65%); (e) under 1% (90%); (f) under 0.33× (85%) | (a) 0.331, (b) 3.78, (d) 4.09; (e) identical; (f) 0.238 | first clause missed; second and third hit |
| P38 | (g) under 0.33× (60%) | 0.793 | missed |
| P39 | evasions halve where tries fall 3× (70%) | (a) −17%, (f) −29% | missed |
| P40 | growth ≥ 3 at depth ≥ 19 ends ≥ 2,650 (90%) | every such iteration ends ≥ 2,704 | hit |

**Calibration.** The two hits are the position-driven model (P36, P40).
The three misses share one wrong part of the registered model: H3's
channel was written as ProbCut's own cost (its pre-test's quiescence,
its constants' monotone effect, its store served by step 13), and the
reads say the arm's cost in the band is set by where the climb lands
(chaos in the constants, P37) and by producers other than ProbCut
(razoring and the move-loop leaves, P39), with step 13 inert at the
widened windows (P38). The channel left in §4 is registered as
unmeasured, with its counter named, not as a finding.

## Review — B.3.1 accepted with named follow-ups (2026-11-01)

Independent review of B.3.1 at `ee90a12`, in a separate session from the
implementing one (PROCESS *Reusable review prompt*; §10 *Review*; the
implementation record's *What the reviewer must check*, tickets 0 to 6 and
6b to 10b). Class `R2`. Nothing was fixed, no default, seed, floor, test or
clause was moved, and no game was played. Every read below is a node-count
or depth read on trees rebuilt from the committed source; the host was in
use throughout, which the reads tolerate (amendment 7 §R (d)). Evidence:
the review's logs and JSON are in `tools/results/b31-20261030/review/`
(ignored storage), with the clean trees under `build/rev-*`.

**Verdict: accept, with the follow-ups of §4.** Every fingerprint, test and
binding read of the implementation record reproduces from a clean tree on
every flavour and both sanitizer trees; the code matches §10 as amended by
research amendments 1 to 8 and the maintainer's decisions, with the
deviations the record lists plus four it does not, none a change of
mechanism; DESIGN §3's invariants hold on both arms; the OFF arm's edits are
shared seams only; amendment 7's rule was registered before its read and
is consistent with its argument, and the departure in its class and session
cost the leaf one more return without letting a wrong clause stand;
amendment 8 §1 to §4 reproduce from its own data and the donor's source.
The follow-ups are records and registrations for B.3.2 and B.3.3, not
returns.

### 1. Rebuilt from clean sources, every fingerprint reproduced

Ten trees configured and built from `ee90a12` with the record's recipe
(`cmake --preset release`, `-DUSE_PEXT=ON -DPORTABLE_BUILD=ON -DCOMP=clang`,
target `all`, Clang 23.1.4; the sanitizer trees `--preset debug`, ASan and
UBSan), into fresh directories (`build/rev-{on,off}[-tune|-diag|-ablate|-san]`),
not the implementer's.

| Read | OFF | ON | Record |
|---|---:|---:|---|
| `bench 13` Release / Tune / Diag / Ablate | 16,615,757 × 4 | 6,723,172 × 4 | equal |
| `bench 10` release / sanitizer | 4,034,698 / 4,034,698 | 1,367,728 / 1,367,728 | equal |
| Release CTest | 25/25 | 25/25 | 25/25 both |
| Sanitizer CTest, `-LE strength` (ASan, UBSan; node checks armed) | 23/23, no report | 23/23, no report | 23/23 both |
| WAC at 100,000 nodes | 240 | **241** (floor 232) | 240 / 241 |
| WAC at 400,000 nodes | — | 261 | 261 |
| WAC depth 6, count (the ON arm's regression guard 119, headroom 20) | — | 132 (`--floor` passes) | 132 |
| Conversion floors, KQ-K / KR-K / KBB-K at 30k, 45k, 60k, 90k, 120k | — | 24 at every budget | 24 at every budget |
| KBN-K at the five budgets (sum ≥ 112) | — | 24 / 24 / 24 / 24 / 24 = **120** | 24 × 5 = 120 |
| `test_endgames` (the 28 wrong-bishop reads among its 121 sections) | — | 121/121 | 121/121 |
| The promotion fixture `3r4/P7/2K1p3/R7/5p2/6kP/8/8 w` at 100,000 nodes | a7a8q +616 (depth 11) | **a7a8q +918 (depth 10)** | a7a8q +616 / +918 |
| Liveness of the eight ablation bits (`ablation_liveness.py`, depth 9) | all eight | all eight | all eight, both arms |
| Depth-20 wire check, bench position 11 (ON Diag) | — | 237 verifications, 0 failed; 370 of 1,081 firings truncated; 20,917,059 nodes | `null_verifications` 237, `ext_budget_bound` 370 / 1,081 |
| Depth-20 wire check, bench position 15 (ON Diag) | — | 0 verifications; 263 of 566; 122,131 nodes | 0 and 263 / 566 |

Binaries' SHA-256 (release): ON `7ab40104…`, OFF `11386c4b…`; the Diag,
Ablate and Tune builds beside them in `review/hashes.txt`. The hashes differ
from ticket 10's (`t10/hashes.txt`) as a fresh non-reproducible build's do;
the fingerprints are what the record claims and are what the review
compares.

### 2. The two research reads re-done

**Amendment 8's selection (ticket 10b), re-scored from `amend8/a8_select.json`**
(SHA-256 `1bae09fa…`, unchanged): 146 rows; the rule (`|score14| < 564`,
growth at 13 and 14 both < 2) admits 53; the largest is `suite_v2` 12 at
746,048 nodes, +104, growth 1.96 and 1.27; the next is bench 2 at 653,952.
Bench 11 reads +2,079 at depth 14 (701,309 nodes) and is excluded by the
score clause. Exactly as recorded.

**Amendment 7's screen on amendment 8's set**, run once by this session
(`deep_iteration_screen.py --depth-min 16 --depth-max 25`, the 60 s
watchdog, Hash 64, base `build/rev-off`, arm `build/rev-on`,
`t10/screen_positions.epd` at SHA-256 `1ff7b03d…`), scored by the
registered `t10/a7/screen_score_a7.py` (SHA-256 `57c0ea35…`):

| # | Position | Steps 17–19 (recorded) | Steps 20–22 | (b) geomean | (a) 16–22 | Absolute at 22 (recorded) | Verdict |
|---|---|---|---|---|---|---|---|
| 1 | `suite_v2` 12 | 1.498 / 0.865 / 0.920 | 0.996 / 0.933 / 1.038 | 0.988 | holds | 0.499 | pass |
| 2 | bench 15 | 0.927 / 0.710 / 0.585 | 0.465 / 0.447 / 0.578 | 0.493 | holds | 0.019 | pass |
| 3 | `suite_v2` 50 | 2.270 / 1.925 / 3.709 | 1.015 / 1.017 / 1.027 | 1.020 | holds | 0.401 | pass |
| 4 | `suite_v2` 42 | 0.996 / 1.008 / 1.006 | 1.012 / 1.015 / 1.020 | 1.016 | holds | 0.251 | pass |
| 5 | `suite_v2` 60 | 1.100 / 1.118 / 1.125 | 1.147 / 1.157 / 1.183 | 1.163 | holds | 0.024 | pass |

The scorer reads **PASS** (exit 0) with no watchdog stop on either arm, so (d) does not arise. Every per-depth node count on both arms, at every depth 1 to 25 of all five positions, equals ticket 10b's `t10/t10b/screen25b.json` (SHA-256 `bc4f052e…`): the read is reproduced node for node from the clean trees. P41's hit stands.

### 3. The code against the handoff, ticket by ticket

Read against §10 as amended (amendments 1 to 8 with the maintainer's
decisions), §7's seeds as amended, §9's defaults as amended, and the donor's
`search.cpp` at `edb0d9db` (steps 10 to 13, 16, the TT penalty at `:913-920`,
the final store, `is_shuffling` at `:153`, `qsearch`'s repetition test at
`:1661-1667`). E = an ordinary engineering choice; M = a change of mechanism;
R = recorded in the packet (the record's deviations, or the amendment named).

- **Ticket 0.** The option, definition and CI cell as written (`flavors`
  gains the `B3_PROOF` cell, CTest and `bench 13`). The counters are
  appended (117 entries, never renamed), each with its denominator in the
  table and its unit in `run_suite.py` (22 names: the 21 of the ticket plus
  `pv_quiet_pruned` from amendment 1 §6); the six trace events are
  appended and `decision_trace.py` rejects unknown names. The counters
  landed on the OFF arm's legacy blocks too (`sing_tries`, `sing_multicut`,
  `sing_cap_bound`, the null-move depth buckets): E, Diag-only, R.
- **Ticket 1 / 1b.** `follow_pv`, `ext_plies`, `ext_taken`, the previous PV
  and raw root score in the driver (MultiPV lines after the first carry an
  empty PV), `root_score_` set in `root_search()`. The budget's arithmetic
  is in `search_b3.h` and applied at ticket 6 (amendment 1, R). The node
  checks hold: a child's `ext_plies` is its parent's plus `ext_taken`;
  `!B3ExtBudget || ext_plies ≤ root_depth_`; `follow_pv` only under a
  following parent. The null and ProbCut children and the same-ply
  re-entries inherit the node's `ext_plies`. The root's `ext_plies` is never
  written and stays at its default 0.
- **Ticket 2.** No in-check extension on the ON arm unless `B3CheckExt`;
  `check_prune` admits in-check nodes to LMR, the first-move step-down, the
  `depth -= 3` step and the whole move-loop block with quiet futility gated
  `!in_check`: the donor's own gates (its capture futility has no in-check
  clause; its quiet futility does). The PV quiet-pruning consumer
  (`!is_pv || (b3_pv_quiet_prune && !follow_pv)`), amendment 1 §6 and
  amendment 3 (R). `sing_in_check` and `sing_triple` count the categorical's
  extension only. The ON arm's `test_wac` ran `--count` until ticket 10: E, R.
- **Ticket 3.** The donor's step 10 line for line at the contract's seeds
  (`16·d + 16·improving − 151`, floor −564, `R = 7 + d/3 + max((se − β)/72, 0)`,
  no depth floor, the null child `NonPV` with `cut_node = false`, no win
  returned unproven, the region `ply + 3(d − R)/4` at depth ≥ 16 outside a
  verification, the verification at `d − R` under `(β − 1, β)` with
  `cut_node = false` and null moves allowed, the region cleared after it,
  the null score returned only when the verification reaches β, no
  verification inside one). `B3NmpCutOnly`, `B3NmpVerify` 0/1/2 and
  amendment 2's `B3NmpEvalGate` (default 0, the maintainer's decision 2).
  Basilisk's `allow_null` and previous-null guards kept, the exclusion
  search passing `allow_null = false` in place of the donor's
  `!excludedMove`: E, R. B-37: the verification re-enters the node's record;
  `move_count` and `stat_score` are saved and restored around it because
  ProbCut's children read `(ss − 1)->move_count` and `stat_score`: E, R
  (the record's "first branch"). Two things the record's field list does
  not carry, both shared with the donor and neither a change of mechanism:
  (i) the re-entry rewrites `ss->eval` and `ss->raw_eval` from the same
  raw evaluation but the correction as the null subtree and the
  verification left it, so after a verification the node's `improving |=
  ss->eval ≥ β` and its children's `(ss − 1)->eval` reads (hindsight, the
  eval-difference training, the in-check inheritance) see the re-entry's
  corrected value while the node's own `static_eval` and `eval` locals keep
  the first; the donor's `ss->staticEval` behaves the same, and its
  `improving |=` reads it; (ii) `tt_hit` and `in_check` are rewritten equal.
  Follow-up 2. The wire check: the clause reads "bench positions 11 and 15
  read `null_verifications ≥ 1`"; position 15 (`8/8/p1p5/1p5p/1P5P/P1P5/8/K1k5 w`)
  is a pawn ending with no non-pawn material, where the null move is never
  tried, so the clause was unsatisfiable there by construction; the record
  reads 0 and narrows the rule column to position 11 without listing the
  narrowing as a deviation. The wire is proved live by position 11
  (reproduced above). Follow-up 3.
- **Ticket 4.** `!follow_pv && (is_pv || cut_node) && depth ≥ 6 && tt_move
  == NONE`, after the null move's `improving |= ss->eval ≥ β` and before
  ProbCut, as the donor; `B3Iir`, `B3IirStale`, `iir_pv_nodes`. Not in check
  (the donor reaches step 11 only out of check): E, R.
- **Ticket 5.** The donor's step 12 line for line at the converted margin
  (`β + 68 − 18·improving`, depth ≥ 3, `!is_decisive(β)`, the valid-TT-value
  guard, PV nodes admitted under `B3ProbCutPv`, `ProbCutPicker` over
  captures and queen promotions in capture-history order with the TT move
  first after the main picker's validation and SEE at `pc_beta −
  static_eval`, the excluded move skipped, quiescence at `(−pc_beta,
  −pc_beta + 1)`, the reduced search at `d − (improving ? 5 : 3)` with
  `!cut_node`, the store at `pc_depth + 1` as a lower bound with the raw
  evaluation and the node's `tt_pv`, the return `value − (pc_beta − β)`
  unless decisive). `!is_root` added, which §10's invariants require
  (nothing at the root is pruned by a B.3 mechanism) and amendment 8 §1
  records: E. No underpromotion (the donor's capture stage): E, R. The
  store's evaluation at an exclusion node is the entry's own raw value
  (the donor writes its corrected `ss->staticEval` there): E, R, and the
  one difference amendment 8 §1's line-for-line read does not name
  (follow-up 4). The small ProbCut behind `B3ProbCutTt` default 0 with
  margin 240 (amendment 2 §2, R), in check too, refusing decisive β and
  TT scores. The picker is tested over 3,000 positions.
- **Ticket 6 / 6b.** Entry as the donor's step 16 (`m == tt_move`, no
  exclusion, `depth ≥ 6 + tt_pv`, a valid non-decisive TT score proving a
  lower bound at `tt_depth ≥ d − 3` or `d − 2` under `B3SingTtDepth`,
  `is_shuffling` ported as written, `seek_mate` from root depth 12 at |root
  score| ≥ 564 on the previous completed iteration's raw score), `s_beta =
  tt_score − (252 + 19·(tt_pv && !PV))·d/63`, `s_depth = (d − 1)/2`, the
  exclusion search `NonPV` under the node's `cut_node`; the double and
  triple bars of §7 as amended (quiet triple term 0; the correction term
  `min(|corr|/198368, 101)`; the `ttMoveHistory` term `331·tmh/114178`;
  `deep = ply > root_depth_`); `1 + (…) + (…)` under the budget, truncated
  and counted, then `depth++`; multi-cut under `B3MultiCut` 0/1 refusing a
  decisive value, with `−421 − 110·d` to the TT-move history and the
  correction bonus `clamp((v − se)·1000/282·sd·177/1024, ±256)` out of
  check when `v > se`, returning the value; `−3` when `tt_score ≥ β || cut
  node`. The node's `depth++` deepens the later moves and not the TT move
  (`depth_bump`), with the TT move's reduction read before the bump as the
  donor's `reduction()` is: E, R. `ply < MAX_PLY − 2` on the entry: E, R.
  `singular_quiet_lmr` dead unless `B3SingularQuietLmr`; the double cap
  gone. The margin 252 and `B3SeekMateDepth` 12 (amendment 3, the
  maintainer's decisions 1 and 2; R); nine X lines for "the double and
  triple bars 8" (R). One definition the contract inherits from B.2 rather
  than the donor: Basilisk's capture stage is a capture or any promotion,
  the donor's a capture or a queen promotion, so `!ttCapture` in the bars
  and `is_shuffling`'s capture clause read an underpromotion TT move as a
  capture here: E, Basilisk's existing definition, not recorded, no
  consequence the review can name.
- **Ticket 7 / 7b.** `B3CorrDecisive` (the refusal of decisive scores
  removed, the bonus clamped), `B3CorrExcluded` with amendment 4's
  raw-evaluation guard on `ss->raw_eval` (written beside `ss->eval` on
  both arms, read on the ON arm's training block only; the maintainer's
  decision 1; R), `CoreHistoryFills` at the first search after a clear
  with sf_19's `Worker::clear()` values (tested). The floors with
  `B3CorrDecisive` 0 and 1 are recorded (120 both).
- **Ticket 8.** The cuckoo tables (8,192 slots, the donor's `H1`/`H2`, every
  reversible move of every non-pawn piece between two squares, asserted
  complete at 3,668, tested complete by the attack tables),
  `Board::upcoming_repetition(ply)` as the donor's loop with the bound
  `min(rule50, plies_from_null, history)` (the history bound is an added
  safety the slow reference shares: E, not recorded) and the
  between-squares occupancy test; the two consumers behind `B3Cuckoo`
  where the ticket puts them: negamax after `is_draw` and before the
  check-extension, the quiescence hand-off and mate-distance pruning;
  quiescence after its draw check on nodes it was not handed; `alpha < 0`
  only, raising alpha to 0 and returning it when that reaches beta (the
  draw value 0, not the donor's `value_draw`). The terminal test in
  amendment 5's form through the test-only `SearchLimits::root_alpha`
  /`root_beta` seam (R; test (b) withdrawn, R). No false positive against
  the slow reference over 10,000 positions.
- **Ticket 9 / 9b.** The fail-low store behind `B3FailLowStore` with the
  default **0** (amendment 6, the maintainer's decision 1; R): the OFF
  arm's form stores the best-scored move, the ON arm at 1 stores none and
  the entry keeps its own. The penalty behind `B3TtPenalty` 1 as the
  donor's `else if` on the cutoff block (`!PV`, no exclusion, a valid score,
  `tt_depth > d − (tt_score ≤ β)`, the bound on the wrong side, `d > 5`):
  Basilisk's cutoff block carries Cluster 1's typed-cutoff clause inside
  the same `if`, which cannot divert a wrong-side bound into the `else if`
  and cannot reach it at `d ≤ 4`, so the two conditions are equivalent.
  `TranspositionTable::penalize` rewrites the payload with `depth − 1` and
  nothing else, leaving an entry at depth ≤ 0 alone where the donor
  clamps its byte at 0: E, unreachable (the condition needs `tt_depth ≥ 5`),
  tested, R (the test's text). The switch test on bench position 3: E, R.
- **Ticket 10 / 10b.** Bits 2, 3 (the small ProbCut included), 4 and 6 on
  the ON arm's mechanisms; the check extension exists only under the
  tune-build categorical so the Ablate flavour never has it (B-109 by
  construction); the depth-6 floor 119 chosen in `test_wac.cpp` by
  `kB3Proof` (E, R); the CI cell since ticket 0. The screen's set replaced
  by amendment 8's rule (R). Not made at ticket 10, where Return 3 placed
  them: amendment 2 §3's three RFP coordinates on B.3.3's surface and the
  donor's whole pair (margin 17 with the quiet triple term at −53) as a
  registered sweep point (the maintainer's decision 1 on amendment 2);
  `config_b3.json` carries the twenty B3 X lines and `b3.tune.toml` the
  three sweep notes of amendment 3 only. Follow-up 1.

**The OFF arm's shared seams.** Every legacy block the ON arm replaces is
prefixed `!kB3Proof &&` (NMP, ProbCut, IIR, the singular block); the ON
blocks are `if constexpr (kB3Proof)`. What the OFF arm shares: the counters
and trace events (Diag-only); the stack fields `follow_pv`, `ext_plies`,
`ext_taken` (never written on the OFF arm) and `raw_eval` (written, never
read); the driver's previous PV and raw root score, `root_score_`,
`nmp_min_ply_` (reset per search), `hist_fresh_`; the root-window seam at
its equal defaults; `penalize` reached only from an ON branch; the quiet
futility `!in_check` that the OFF arm never reaches in check; `KNOWN_WIN`
moved from `eval.cpp` to `eval.h` unchanged; and B.12's quiescence repair,
which is this branch's (`d8d30e9`) and inside the OFF fingerprint. The
fingerprint and CTest reproduce on every OFF flavour, which is the proof.

**DESIGN §3's invariants, both arms.** The mate constants stay in
`constants.h`; no static score enters the mate band; TT scores are
ply-adjusted on store and probe (every new store goes through `tt_store`);
every TT move is validated at every consumer (the ProbCut stage validates
piece, colour, legality and candidacy); the stored static eval is the raw
one; the fail-high blend is untouched; the draw value is 0; nothing at the
root is pruned, reduced or extended by a B.3 mechanism (`!is_root` on the
cuckoo, ProbCut, the small ProbCut and the singular entry; NMP runs only
at cut nodes, never the root); the exclusion search keeps the node's
`tt_pv`; the node checks hold at every entry under the sanitizer CTest
(`test_stack_unwind`, `test_b3_proof`, `test_search`); `ply < MAX_PLY − 2`
bounds the singular entry; the budget's invariant is a node check; no
mechanism admits a decisive value the donor refuses. The TT's two-publication
tolerance (BAS-C05) applies to `penalize` as to any store and is said so in
the seam.

**The interactions §5 names** are read where the handoff puts them: the
check policy against every move-loop rule and LMR (ticket 2's in-check
counters); the proof-search children's `cut_node` typing (the null child an
all node, ProbCut's `!cut_node`, the exclusion search the node's own); the
RFP–NMP hand-off band (amendment 2 §3, follow-up 1); the no-TT-move
population (amendment 6, §9 item 16); `seekMate` against the mate drives
(the floors); the budget against hindsight and the negative reductions
(neither consumes it). The donor's `ttMoveHistory` has two producers on
the ON arm as in the donor.

### 4. Amendment 7's rule, class and order; amendment 8 §1–§4

**Order.** `50a97b8` (2026-11-01 12:53:36) carries §R and no §1; the screen's
JSON `t10/a7/screen25.json` was written at 12:57:52 and §1 landed in
`bb64565` (13:31:47). The rule preceded the read.

**The rule against its argument.** §0 argues that a position whose OFF arm
resolves at depth 13 or 14 and whose ON arm resolves later shows the ON
arm's approach as growth inside 16–19 against a flat baseline, and that an
explosion of the kind the screen was built for (a ply bound that does not
bound the tree) persists. §R keeps the statistic and the bound and moves
the judged steps to 20–22, with (c) a second window to 25 and (d) a
watchdog rule that makes node counts decide. That follows from the
argument for a resolution hump, and the scorer's self-test shows a
sustained ×2 explosion still fails. Two limits, both now on record: the
window was chosen after position 50's depths 20–22 were exposed (the
addendum's read 2), which §R says and excludes from the evidence; and the
rule still compares the arms at matched depth, so a position-driven event
entered at different depths fails it, which Return 8 then read as a
mechanism and amendment 8 corrected.

**The departure.** PROCESS's role table puts a research amendment at `R3`
in a separate session; amendment 7 was written as `R2` in the implementing
session at the maintainer's instruction. What it moved was one clause's
reading window, on a causal question the addendum's deterministic reads
had answered and this review reproduces (position 50's steps 20–22 read
1.015, 1.017, 1.027 here). The cost of the departure was realised in the
next step, not in the clause: the same session's Return 8 misattributed
bench 11's band to ProbCut, and the `R3` amendment in a separate session
(amendment 8) corrected it with the port read line for line. The clause
itself stands unchanged and passes on the set amendment 8's rule chooses.
The review weighs the departure as acceptable for a window change under
the maintainer's instruction, with this review as the control amendment 7
§3 asked for; a mechanism decision would not have been.

**Amendment 8 §1–§4.** §1: the donor's step 12 at `:1046-1093` against the
kernel's block reads line for line as the amendment says, with its two
named differences (the root skipped; the `is_decisive` band) and one it does
not name (the exclusion-node store's evaluation, ticket 5's recorded
deviation; unreachable on the band, since no ProbCut store there is at an
exclusion node of consequence). §2: the band on every arm reproduces from
the logs (`b11_on.log`, `b11_off.log`, `a8_r1_m8_d23.log`, the seven
`a8_r2_*_d21.log`): every step with growth ≥ 3 at depth ≥ 17 ends with a
root score of 2,700 or more on every arm, and no arm grows 3× before its
score passes about 2,650 (P40). §3: the R2 counts reproduce from
`a8_r2.json` (control 140.18M at 21; (a) 46.42M, (b) 529.11M, (c) 67.77M,
(d) 573.39M, (e) identical, (f) 33.30M, (g) 111.22M) and are non-monotone in
the margin, the depth gate and the reduction as stated. §4: the channel left
is labelled unmeasured with its counter named, and the game-depth argument
is a statement of where the read would be taken, not a finding. The
maintainer's decisions (1 taken, 2 not taken) are recorded in the packet.

### 5. Follow-ups (records and registrations; no code, no default)

1. **B.3.3's surface registration** (BAS-S42's assembly): add amendment 2
   §3's `CoreRfpPerPly`, `CoreRfpImproving` and `CoreRfpDepthCap` to the
   Cluster 2 surface by §5.12's rule, and the donor's pair (margin 17 with
   `B3SingTripleQuiet` −53) as a registered curvature-sweep point beside
   the margin's extra points, as the maintainer's decision 1 on amendment 2
   states; decide whether `B3ProbCutTtMargin` stays an X line while
   `B3ProbCutTt` is 0 (an inert coordinate on the surface). Owner: B.3.3's
   registration, before the sweep.
2. **The B-37 field list** gains `ss->eval` and `ss->raw_eval` (rewritten by
   the verification re-entry under the correction as the null subtree left
   it; read afterwards by the node's `improving |=` and by its children;
   shared with the donor) and the equal rewrites of `tt_hit` and `in_check`.
   Owner: the packet's record, at B.3.2's opening.
3. **Ticket 3's wire check** is recorded as satisfiable on bench position 11
   only (position 15 is a pawn ending); the record's rule column says so
   rather than narrowing silently. Owner: the packet's record.
4. **Amendment 8 §1** gains the third port difference (the exclusion-node
   store's evaluation, raw here, corrected in the donor). Owner: the
   packet's record.
5. **Delivery shape.** B.12's repair (`d8d30e9`) and B.3.0 are on this
   branch and not on `master`, whose head is `43b4c35`; the OFF fingerprint
   16,615,757 is the branch's. AGENTS *Commits and reporting* lands one
   feature per PR; how B.12, B.3.0 and B.3.1 reach `master` is the
   maintainer's delivery decision, raised here so the review's "OFF arm
   reproduces master" is read correctly: it reproduces the branch's OFF
   arm. Owner: the maintainer, before B.3.2's first game.

Not follow-ups: the `is_shuffling` capture-stage definition and the history
bound in `upcoming_repetition` (§3), both Basilisk's own definitions with no
consequence the review can name; they are listed for completeness.

## B.14 — the consumer guard on both arms (2026-11-01)

PLAN B.14, the repair amendment 4 §3.4 assigned to its own leaf: no
correction history is added where the raw static evaluation is 0. Engine
commit `e883a13` on `b3-pruning-extensions`, both arms. Evidence:
`tools/results/b14-20261101/` (ignored storage). `head/` holds the
head's binaries (`c15d097`, equal to ticket 10's hashes), their
`bench 13` logs and the fixtures' first run. `new/` holds the guarded
tree's reads (`b14_reads.py`, `b14_reads.json`, `ctest_*.log`,
`flavours.txt`, `hashes.txt`). `count/` holds the exposure read
(`b14_count.py`, `count.patch`). The reads ran on the tree as committed
except for one comment, and `bench 13` was re-read equal after the
commit.

### The change

- **`corrected_eval`** (`search_worker.h`) returns 0 for a raw
  evaluation of 0.
- **The quiescence stand-pat** now calls `corrected_eval` instead of
  repeating its arithmetic. With a nonzero raw evaluation it computes
  exactly what it did before, so both consumers carry the guard and
  cannot drift apart.
- **Unchanged:** correction training and ticket 7b's producer guard.
  Training still measures its bonus against the static evaluation, which
  is now 0 at these nodes.

### The fixtures (`test_endgames`, both arms)

- **"a recognised draw reads a draw from its own root's training, depths
  8-14".** `5k2/8/6KP/8/8/8/8/5B2 b`, depths 8–14 under 200,000 nodes,
  Hash 8, fresh tables per depth; each depth must finish and read
  |score| ≤ 75.
- **"KBP-K wrong bishop reads a draw at depths 8-14 after trained
  tables".** One table and one searcher. The two priming searches
  (`8/8/5K1P/8/8/8/k7/5B2 b` and `8/8/6KP/8/8/8/8/k4B2 b`, depth 12) run
  first, then B.12's 28 searches without clearing. Amendment 4 §R's R2b
  read this over UCI; the in-test form reproduces those reads exactly
  (below).
- Both fixtures print the score of a failing depth.

On the head (`head/endgames_{on,off}.log`), both fail:

| Fixture | OFF arm | ON arm |
|---|---|---|
| Organic | **fails**: −90 at depth 11, as amendment 4 §2.1 | holds 7/7 |
| Primed | **fails**: 172 and 137 at depths 8–9 on the first FEN, 107 at depths 9–10 on the fourth (24/28, as R2b) | **fails**: 295 at depth 8 on the first FEN (27/28, as ticket 9b's read) |

On `e883a13` both hold on both arms (191/191). The UCI reads agree:
organic 7/7 and primed 28/28 on both arms, controls 28/28.

### Qualification

| Read | OFF | ON | Rule |
|---|---:|---:|---|
| `bench 13`, head → guarded | 16,615,757 → **15,100,437** (−9.1%) | 6,723,172 → **6,178,678** (−8.1%) | recorded; B.3.1's OFF arm re-bases |
| Tune / Diag / Ablate `bench 13` | 15,100,437 × 3 | 6,178,678 × 3 | equal to release |
| Release CTest | 25/25 | 25/25 | binds |
| `test_endgames` | 191/191 | 191/191 | binds |
| Wrong-bishop 28, test's form / primed / organic | 28 / 28 / 7 | 28 / 28 / 7 | binds |
| WAC at 100,000 nodes; depth 6 | 241; 184 (floor 175) | **242**; 132 (guard 119) | ON ≥ 232; floors |
| KQ-K, KR-K, KBB-K at 30k/45k/60k/90k/120k | 24 each | 24 each | 24 |
| KBN-K at the five budgets (sum) | 22/23/24/23/23 (115) | 24 × 5 (120) | ≥ 112 |
| The fixture `3r4/P7/2K1p3/R7/5p2/6kP/8/8 w`, a7a8q at 33k / 60k / 100k / 200k / 400k / 1M | +168 / +168 / **0** / +834 / +938 / +1,053 | +276 / +599 / **+599** / +864 / +1,247 / +1,491 | the ON arm's clause: a8=Q, positive at 100k |

- **The 120,000-node floor run's exit 1 on the OFF arm** is the floors
  tool's staleness guard (KQ-K's mean 10.8 plies), as B.3.2's leaf text
  records. It is not a floor.
- **The fixture's positive-score clause binds the ON arm** (amendment 3
  §5, research decision 1), and the ON arm passes it. `test_search`
  asserts the move on both arms and passes.
- **The OFF arm's fixture at 100k** reads `a7a8q` with score 0 at depth
  13 (head: +616 at depth 11), and +834 from 200k up. It is recorded, not
  judged. On that search 1,352 of the 1,550 main-search nodes with a raw
  evaluation of 0 are classifications, nearly all drawn KPK, which the
  bitbase makes exact. The head's +616 included correction added on top
  of exact draws.

### Exposure (`count/`)

A temporary counting build of the guarded source, patched and built into
`build/b14-count-{on,off}` with the source bytes restored. It reproduces
both guarded fingerprints. At both consumers it counts the evaluated
nodes, the nodes with a raw evaluation of 0, the share of those that are
classifications (KNNK, drawn KPK, the wrong-corner draw, a pawnless
scale of 0; the flag is re-derived by one more, pure, evaluation), and
those whose dropped correction is nonzero. Over `bench 13`:

| Arm, consumer | Evaluated nodes | Raw 0 | Of which classifications | Correction nonzero |
|---|---:|---:|---:|---:|
| ON, main search | 2,811,285 | 8,369 (0.30%) | 1,868 (22%) | 8,175 |
| ON, quiescence | 2,214,157 | 4,430 (0.20%) | 894 (20%) | 4,248 |
| OFF, main search | 7,214,022 | 15,562 (0.22%) | 2,107 (14%) | 15,186 |
| OFF, quiescence | 3,499,473 | 7,703 (0.22%) | 1,069 (14%) | 7,396 |

- **Most guarded nodes are ordinary.** 78–86% of the nodes the guard
  changes are ordinary evaluations that happen to be exactly 0, not
  classifications. Amendment 4 §3.1 called that case harmless for the
  producer guard; on the consumer side it is most of the change.
- **All 40 positions moved on both arms**, for this reason. Over three
  in four of the 0.2–0.3% of evaluations that are 0, the guard drops a
  nonzero correction from an ordinary estimate. So it is not confined to
  drawn endings, and every game's search meets it.
- **One position's fresh read, recorded and not judged.** On bench
  position 29 (`1B6/1p6/p1p2k2/P1P1p1p1/1P1nPp1p/5P1P/1K4P1/8 b`, Hash 64,
  depth 13) the head's OFF arm finds `...Nxf3` at depth 12 (+775). The
  guarded OFF arm plays `...Ne6` at −45 through depth 13. That is a
  fixed-depth difference in one position, not a measured harm.

### The gate: the repair SPRT is owed

The `[-5,5]` repair SPRT at `3+0.03` is owed. The rule is to measure the
exposure first and cap where further games could not change the
decision; this exposure is every game, not a band.

- The guard changes the search at 0.2–0.3% of evaluated nodes in every
  bench position on both arms.
- Most of those nodes are ordinary positions, so the sign of the effect
  is not given by the repair's chess argument.

The shape is BAS-S40's:

- Colosseum `sprt-repair.toml`, `[-5,5]`, α = β = 0.05, `3+0.03`, 1T,
  Hash 64, paired reversed UHO openings, no adjudication, tablebases off.
- Release PEXT final-PGO builds pinned by SHA-256.
- The cap is the full 4,000 pairs, since the exposure is above 2%.
- The arm is OFF, the engine `master` ships: the candidate is `e883a13`'s
  OFF arm, the baseline `c15d097`'s. The ON arm's measure is B.3.2's
  paired run, on this head.
- It would be registered as BAS-S44 (BAS-S41 to BAS-S43 are reserved)
  before any game, with its prediction frozen. It is prepared once the
  guard's form is settled (below). It is a game, and the maintainer
  starts it.

Until the verdict, the fingerprint record (GUIDE, `AGENTS.md`, DESIGN)
stays at 16,615,757, as BAS-S40's verdict rule did for B.12. The B.3
comparisons on this branch read OFF 15,100,437 and ON 6,178,678 from
`e883a13`.

### Finding for the maintainer: the guard's key

The guard is keyed on `raw == 0`, the proxy amendment 4 §3.4's
counter-argument named, and the exposure read measures the proxy: 14–22%
of the nodes it changes are the classifications the leaf is about.

- **A narrower form** keys the guard on the evaluator's own verdict:
  KNNK, the drawn KPK, the wrong-corner draw and a pawnless scale of 0,
  the set the counting build already flags. Where a raw 0 is read (from
  the TT or fresh), the node is evaluated once more to learn whether it
  was classified. That costs 0.2–0.3% more evaluations and leaves every
  ordinary 0 its correction.
- **What the narrower form would change:** it should repair the same two
  fixtures, whose zeros are the wrong-corner classification (amendment 4
  §1–§2's traces), and move the fingerprints less. Neither is measured.
- **It is not implemented here.** It narrows the decided candidate's
  semantics, which is the maintainer's decision.
- **It should come before the gate:** an SPRT run on the proxy and a
  narrowing afterwards would need a second gate.
