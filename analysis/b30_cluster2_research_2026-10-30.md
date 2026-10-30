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
| B-12 best move stored at fail-low | a defect (PLAN §2's table): repaired in the ON arm (ticket 9) behind the attribution switch `B3FailLowStore`; its population effect on IIR, razoring, RFP and LMR is §5.6's and pulls `CoreLmrCutNodeNoTt` onto the surface |
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
coordinate), in the order below, before B.3.3's sweep. The first four
reads come first because they decide what the paired run of the whole arm
is measuring.

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

Attribution switches: defaults are the repair or the donor's form and a
paired run never moves them (PLAN §2: a correct repair that loses is
reworked, not reverted). They exist so a negative paired run of the arm is
ablated by switch before anything is blamed, and so the conversion floors
can read one repair alone.

| Option | Values | Default | Read |
|---|---|---|---|
| `B3FailLowStore` | 1 no move stored after a fail low (B-12 repaired); 0 the best-scored move | 1 | ablation on a negative paired run; `tt_move_absent` on both settings |
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
