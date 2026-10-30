# Cluster 2 research and handoff — PLAN B.3.0

Started 2026-10-30 on `master` `43b4c35` (`bench 13` 19,470,325), the
accepted B.2 head with A.11, A.13, B.10 and A.16 on top. Donor: Stockfish
`sf_19` `edb0d9db` (`src/search.cpp` line numbers below are that file's);
kernel line numbers are `src/search_kernel_core.cpp` at `43b4c35`. This is
the working packet: sections are added as the research proceeds, and the
leaf closes `READY_FOR_IMPLEMENTATION` only when §8's conditions are met.

**Status: RESEARCH, draft 1.** §2 re-bases B.0's handoff, §3 compares every
Cluster 2 mechanism with the donor, §4 disposes of every input PLAN B.3
lists. §6 names the zero-game reads still owed before the contract freezes.

## 1. Verdict so far

- B.0's Cluster 2 contents stand: NMP, ProbCut with the small TT ProbCut,
  singular in the donor's shape, IIR against hindsight, and the check
  policy. The re-base changes no item's direction; it changes three seeds
  and adds four mechanisms the 2026-10-27 code audit found (§4).
- The largest single semantic gap is the **singular margin**: Basilisk's
  `SingularBetaMult` is 4 cp per ply, the donor's converted margin about
  0.26 cp per ply (B-45), and the range 1..6 cannot reach it. Basilisk's
  singular search therefore asks a far weaker question ("is every other
  move 4·d below the TT value?") and fires on 0.45% of interior nodes
  against Rarog's 4.3% attempts (B.0 §2.5).
- The **child node types** of the three proof searches (S1) are wrong
  against the donor in all three, and NMP's entry reads the TT-refined
  `eval` where the donor reads the static eval (S7). Both are
  population changes, not tuning: they belong to the cluster's ON arm, not
  to separate categoricals.
- The **check policy** is the cluster's hardest decision: an unconditional
  in-check extension with no reduction or move-loop pruning in check (S3)
  on 15.6% of interior nodes, measured twice against older margins
  (BAS-S08 −10.17 for removal, BAS-S16 −3.48 for a cap). BAS-S16's retry
  trigger fires here. The packet proposes the donor's form as the ON arm
  and the current form as a categorical, read by games, not by WAC alone
  (B-89, B-90).

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

### 2.1 Stride-1 counters on the head

Pending: `run_suite.py --depth 14 --stride 1` on `suite_v2`, Diagnostic
build of `43b4c35` (`tools/results/b30-20261030/`).

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
| `ttMoveHistory` | absent | one scalar, trained at multi-cut and at the node's end | — | ADOPT; also an LMR term (B-51 puts its bases on the surface) |
| `singular_quiet_lmr` | relaxes LMR for the node's quiets after a singular TT move (BAS-D18, unmeasured) | the donor's `ttPv`/singular-margin LMR terms | — | subsumed (B.0 §6); removed with the ON arm |

### 3.5 Check policy (kernel `:357-373`, `:854`, `:1121`, `:1137`, `:1268`)

| Aspect | Basilisk | Donor | Disposition |
|---|---|---|---|
| Extension | every in-check node `depth++` (15.6% of interior, B.0) | none | the cluster's ON arm takes the donor's (none); the current extension is a categorical read by games at STC and LTC |
| Reduction and pruning in check | none: LMR, LMP, futility, SEE and history pruning all skip in-check nodes | LMR, the step-down, `depth −= 3`, LMP, capture SEE, history pruning and quiet SEE all run; only quiet futility is gated | ADOPT with the extension decision (S3): the two are one policy, BAS-S16's trigger |
| Composition with singular | a checking node can take 3 plies (BAS-D11 kept it on WAC) | — | follows the extension decision; BAS-D11 re-read by games (B-90) |

### 3.6 Couplings the donor has and Basilisk lacks

- **`seekMate`** (S5): at root depth ≥ 16 with `|root score| ≥ 2000`
  (≈ 564 cp) the donor caps RFP at depth 6 and disables singular
  extensions. The singular half is B.3's; RFP's is B.4's. Read on the KBNK
  and KQK fixtures at several budgets and BAS-R07's rec1 timeline.
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
| S10 TT bound-mismatch penalty | a note: post-SF-19 in B.0's table, in `edb0d9db` in fact; B.3.0 checks the line and, if present, lands it in the ON arm |
| S12 `double_exts` inheritance | moot if the deletion arm removes the counter; else fixed in the ON arm |
| B-3 KBNK multi-budget majority | §6, before the multi-cut categorical is decided |
| B-5, B-6, B-7, B-97, B-98 | gates on every IIR arm (§3.3) |
| B-12 best move stored at fail-low | a categorical: it changes the no-TT-move populations of IIR, razoring, RFP and LMR |
| B-14, B-15 correction training at decisive results and singular-excluded nodes | categoricals of the ON arm |
| B-31, B-32, B-37 NMP verification | §3.1 |
| B-33 cut-node-only NMP at LTC | §3.1 |
| B-34 margin terms against their entry guards | a B.3.1 review item |
| B-35, B-54, B-94, B-112 instruments | §6 |
| B-39, B-42, B-43 ProbCut | §3.2 |
| B-45, B-46, B-50 singular | §3.4; B-46's TT-depth margin 2 against 3 is a categorical |
| B-49, B-53 extension composition | §3.5 |
| B-51 new LMR term bases on the surface | §7 |
| B-55, B-103 IIR range | §3.3 |
| B-89, B-90 zero-game reads mispredicting games | BAS-D11 and BAS-D13 re-read by games |
| B-109 ablation bit 6 also removes the check extension | the mask splits: bit 6 singular only, a new bit for the check extension |
| B-114 CI covers the cluster's switch arm | B.3.1 adds the OFF arm to CI, as B.2 did |
| Double-extension counter and cap | deletion arm at `[-5,0]` if §6 shows the cap never binds |
| Same-ply NMP verification | deletion arm at `[-5,0]`, the comparison for the region |
| Upcoming repetition | §3.6 |

## 5. Interactions with Cluster 1's fitted core

To be completed with §2.1's numbers. The couplings already known:

- NMP's population moves from "any non-PV node with `eval ≥ beta`" to
  cut nodes with the static eval above a depth-dependent margin. RFP
  (fitted in B.2.3) fires first on the same nodes; B-35's crossover band
  per depth measures how many nodes RFP now leaves to NMP.
- The child-type change (S1) moves the cut-node LMR term (about four plies
  at the donor's value) off every NMP and ProbCut refutation, so those
  subtrees grow; `LmrCutNode` was fitted on the current typing.
- A larger singular population raises the TT-move extension rate and
  `ttMoveHistory` feeds LMR: Cluster 1's LMR base was fitted without it.
- Removing the check extension shortens forcing lines everywhere Cluster
  1's margins assumed they would be one ply deeper; the mate-drive floors
  (KBNK, KQK) are where this bites first.

## 6. Zero-game reads owed before the contract freezes

1. §2.1's counters on the head (running).
2. The double-extension cap: how often it binds per singular firing at
   `suite_v2` depth 14 and at the B.3.2 budgets (a Diag counter, landed at
   the exact fingerprint).
3. The IIR/hindsight overlap: the share of IIR reductions on nodes
   hindsight also reduced, and IIR's stale-entry clause's share of IIR.
4. ~~KBNK and KQK at several budgets for the current head~~ — done, §2.2.
5. B-35's RFP/NMP crossover band and B-54's in-check share against the
   oracle, as instruments B.3.2 reads.

## 7. Seeds

By B.0 §7.2's rules: plies, counts and history units take the donor's
value (rule 1); an evaluation-unit constant takes the donor's converted
value (rule 3) unless it lies more than 2× outside the range of the
oracle-converted and Basilisk-fitted values, when it takes their geometric
mean, or the oracle's alone where Basilisk has no fitted coordinate
(rule 2). The oracle is classical Stockfish `9587eeeb` (×0.485).

| Constant | Donor (SF 19) | ×0.282 | Oracle | ×0.485 | Basilisk fitted | Rule | Seed |
|---|---|---:|---|---:|---|---|---|
| NMP entry: per ply | 13 | 3.7 | 33 | 16 | none (`eval ≥ beta`) | 2, oracle alone (B.0 §7.2 names the NMP margin) | **16** |
| NMP entry: improving | 47 | 13 | 33 | 16 | none | 2, oracle alone | **16** |
| NMP entry: constant | 365 | 103 | 311 (+112·ttPv) | 151 (+54) | none | 2, oracle alone; the donor's shape has no `ttPv` term | **151** |
| NMP reduction base, depth divisor | `7 + d/3` | plies | `(737 + 77d)/246` | plies | `3 + d/4` | 1 | **7, 3** |
| NMP reduction, eval divisor | 256 | 72 | 192, capped at 3 | 93, capped | 192, capped at 3 | 3 (72 is within 2× of 93) | **72, uncapped** (range widened below 80) |
| ProbCut margin | `241 − 64·improving` | 68, 18 | `176 − 49·improving` | 85, 24 | 189 | 3 | **68, 18** |
| Small TT ProbCut | 428 | 121 | (TT capture form) | — | none | 3 | **121** |
| Singular margin, per ply | `59·d/63` | 0.26 | `2·d` (`(4 + formerPv)·d/2`) | 0.97 | 4 | 2: 0.26 is more than 2× below 0.97; geometric mean of 0.97 and 4 | **2.0** (126/63), range down to 0.2 so the donor's value is reachable (B-45) |
| Singular margin, `ttPv && !PV` per ply | `66·d/63` | 0.30 | `formerPv·d/2` | 0.24 | none | 3 | **0.30** (19/63) |
| Double margin | `−2 + 204·PV − 152·!ttCapture − corr − ttMoveHist − 38·deep` | −0.6, 58, −43, …, −11 | none | — | 4, non-PV only | 3 | the converted vector; the correction term by Amendment 3's rule 4 |
| Triple margin | `70 + 279·PV − 188·!ttCapture + 81·ttPv − corr − 43·deep` | 20, 79, −53, 23, …, −12 | none | — | none | 3 | the converted vector |
| Negative extension | −3 | plies | — | — | −1 | 1 | **−3** |
| Singular min depth | `6 + ttPv` | plies | 6 | plies | 5 | 1 | **6 + ttPv** |
| IIR min depth | 6 | plies | IID at 7 | plies | 8 (rail) | 1 | **6** |
| `ttMoveHistory` | updates `−421 − 110·d`, `918 / −747` | history units | — | — | none | 1 | as the donor |

Two seeds move far from Basilisk's fitted values and are flagged for
B.3.2's paired run: the NMP reduction (base 3 → 7) and the ProbCut margin
(189 → 68). Both are the donor's shape at rule 3; B.3.3's ranges span all
three columns, as B.2.3's did.

## 8. Ready when

- §2.1, §5, §6 and §7 are complete;
- every categorical and deletion arm has a registered bracket;
- the B.3.1 contract lists its tickets, each with its fingerprint rule and
  test;
- the maintainer has the one open decision: whether the check policy is
  the ON arm's (recommended) or a separate leaf.
