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
