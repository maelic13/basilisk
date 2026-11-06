# Cluster 3 research and handoff — PLAN B.4.3

Started 2026-11-06 on `b4-quiescence` at `86769bf`: the B.4.2 head
(`d3add9c`, `bench 13` 6,090,091) with the counter seam `1fd783b` and its
suite-runner units `86769bf` on top, both behaviour-neutral. Donor:
Stockfish `sf_19` `edb0d9db` (`src/search.cpp` line numbers below are that
file's; `qsearch` is `:1653-1883`). Kernel symbols are
`src/search_kernel_core.cpp`'s at `86769bf`.

**Status: `READY_FOR_IMPLEMENTATION`, 2026-11-06.** Section R was frozen
at `8de2c95` before any read of it ran and is not rewritten; §6 scores it.
§1 is the verdict, §2 the head re-measured, §3 every quiescence mechanism
against the donor, §4 every input PLAN B.4 lists, disposed, §5 the
interactions with Clusters 1 and 2's fitted cores, §7 the seeds, §8 the
decision, §9 the switches and their brackets, §10 the B.4.4 contract, §11
the frozen predictions and the B.4.5 to B.4.7 registrations, §12 the four
questions, §13 the artifacts, §14 the departures table.

## R. Reads registered before running (frozen at this commit)

Two diagnostic builds of one source, both `bench 13` 6,090,091:

- **the counter build**: `86769bf` clean, `build_test.ps1 -Flavor Diag`;
- **the census build**: the same source with the throwaway instrument
  `tools/results/b43-20261106/census.patch` (recorded by its SHA-256 in
  §13). It counts and never decides: diagnostic builds only, the search
  unchanged, which its `bench 13` proves.

Recipe for both: `python tools/diag/run_suite.py --depth 14 --stride 1
--hash 64` on `suite_v2` (106 positions, a fresh process per position).

**Exposure before freezing.** The counter build's non-PGO twin was run
once at depth 8 to validate the new counters' identities. That run
showed, of the rows below: the exits (table cutoff 20.1%, stand-pat
26.7%, fail high 12.2%, fail low 30.9%, evasion 7.4%, delta 2.7%, the
ply cap 0.02% of quiescence nodes), the first-move share of fail highs
(captures 91.8%, evasions 55.6%), D2 to D5's quantities (26 to 33% of
the main search's estimate refinements from depth-0 entries; fail lows
with a searched capture 24.5%; table moves from depth-0 entries 4.8% of
interior nodes; the correction term blocking 4.0% of reverse-futility
decisions, 18% of those on a table bound, 0.24% of interior nodes) and
a mean stand-pat margin over beta of 353. The D rows are therefore
descriptive at depth 14, and their rules are stated with that exposure.
No census counter has been read: the census build did not exist.

The HCE's material for the census is the mean of its two phases, rounded
down: **88 / 316 / 343 / 538 / 1041** (`eval_params.h`: 83/323/363/514/1085
and 94/310/323/562/998) against the search's 100 / 300 / 300 / 500 / 900.

| # | Read | Prediction | Rule it decides |
|---|---|---|---|
| C1 | Card 1, SEE: `see_ge` verdicts that change under the HCE vector, over every call (`cx_see_changed / cx_see_calls`) and at quiescence's alpha-relative test (`cx_qs_see_changed / cx_qs_see_tested`) | 0.2 to 0.8% of all calls; under 1.5% at quiescence's test (70%) | SEE keeps its vector unless more than **1%** of all calls or more than **2%** of quiescence's change; above either, the vector becomes injectable (§9) |
| C2 | Card 1, futility: quiescence futility verdicts that change when the captured piece is valued by the HCE vector (`cx_qs_fut_changed / cx_qs_fut_tested`) | 1 to 4% (65%); the queen (+141) and the bishop (+43) carry it | above **2%** the captured value quiescence's gain rules read becomes a two-vector categorical (§9); at or below it they keep the search's values |
| C3 | Card 1, order: nodes with two or more captures whose first pick changes under the picker's capture score, and under the quiescence formula with the HCE vector | 5 to 20% under the picker's score; 1 to 5% under the HCE vector (60%) | none: the arm orders by the picker's score (§3); the read sizes the change |
| C4 | The donor's count rule on today's tree: captures at pick index 2 or later that neither check, recapture on the previous move's square nor promote (`cx_qs_late_plain`), those searched today, and those that fail high or raise the node's best | 10 to 25% of captures generated; under 3% of quiescence fail highs come from them; under 5% of those searched raise the best (60%) | the count limit is the donor's 2, fixed, unless these moves make more than **5%** of quiescence's fail highs; then the limit is a categorical 2 against 3 (§9) |
| C5 | Recaptures among futility-pruned captures (`cx_qs_fut_recapture / qs_futility_pruned`) | 5 to 15% (55%) | none: the exemption is adopted (§3); the read sizes it |
| C6 | Quiet evasions searched after the node already holds a value that is not a loss, and how many raise the node's best or fail high (generator order, which no history informs) | 50 to 75% of evasions searched; 10 to 25% of them raise the best; under 3% fail high (55%) | none here: the rule is read in games and by the floors (§9); the read sizes what it removes and bounds what it hides |
| C7 | Positions quiescence stands pat on with no legal move (`cx_qs_stalemates`) | under 0.01% of quiescence nodes (80%) | none: sizes the donor's stalemate test |
| D1 | Evasion order: first-move share of evasion fail highs against the capture loop's; evasion nodes holding a legal table move | evasions 50 to 60%, captures 90 to 93%; a legal table move at under 5% of evasion nodes | none: sizes S9 |
| D2 | The main search's estimate refined by a table bound: share of interior nodes, the depth-0 share of the refinements and of their summed distance (B-20, B-24) | 18 to 24% of interior nodes; depth-0 entries 25 to 40% of both | none: sizes B-21's surface |
| D3 | Fail-low exits with a searched capture; interior table moves from depth-0 entries | 20 to 30% of fail-low exits; 4 to 6% of interior nodes | none: inputs of the fail-low store's decision (§5) |
| D4 | RAR-S40: nodes the correction term alone keeps from a reverse-futility cut while the estimate is a table bound | under 0.5% of interior nodes | the form is a categorical on B.4's surface only above **1%** of interior nodes (Rarog's arm was 9.0% of nodes) |
| D5 | The quiescence ply cap: `qs_cap_hits / qs_nodes` | under 0.1% | at or under **0.1%** the arm drops the cap without games; above it the cap is a categorical |
| D6 | The interpolation's reach: mean margin over beta at stand-pat cuts and at capture fail highs | 250 to 450 at stand-pat; 150 to 400 at fail highs | none: sizes B-21 |

Also read, without a prediction beyond its range: WAC at 100,000 nodes
on the release build of `86769bf`, expected 236 to 246 (the B.3 head
read 241), the base of the arm's floor.

Stop rule for this section: a census build whose `bench 13` is not
6,090,091 is not read.

## 1. Verdict

- Cluster 3's contents stand: the donor's quiescence as one arm behind
  `B4_QSEARCH`. Basilisk already has the table cutoff, the corrected and
  table-tightened stand-pat, fail-soft returns, the stored PV bit and
  depth-0 entries. The arm adds the rest (§3).
- The largest gap is the **gain rules' shape**, not their margin.
  - Quiescence futility hides more than any other rule in the search
    (BAS-S47: 4.27% of its decisions, 5.1% of the first three captures).
  - A quarter of what it prunes today are **recaptures** (24.5%, C5), which
    the donor never futility-prunes. A queen recapture is the clear case:
    the stand-pat is a queen down, 1,041 by the HCE's mean and 1,085 in
    the middlegame, and the rule credits 900 + 150. At an alpha near the
    level before the exchange the recapture clears the test by 9 or
    fails it by 35, so piece-square terms decide whether it is searched.
  - The donor's count rule is not free here: of the captures it would
    remove that are searched today, 14.5% fail high (C4). It mostly
    re-prunes what futility and SEE already prune (85% of its population),
    so it is adopted at the donor's 2 with its limit readable.
- **Evasions** are the cluster's riskiest piece. Quiescence searches 1.85
  evasions per in-check node in generator order with no table move (63%
  first-move fail highs against 92.6% for captures). The donor orders
  them, prunes quiets once one evasion holds and stores the node. In
  generator order 40.6% of the quiets that rule would skip raise the
  node's value (C6), so the rule is the arm's first categorical and the
  conversion floors read it before any game.
- **Card 1 fires, narrowly.** The HCE's material changes 1.03% of `see_ge`
  verdicts and 2.30% of quiescence's futility verdicts, against registered
  thresholds of 1% and 2%. Both become two-vector categoricals with exact
  default identity (§9); no value is tuned.
- **Card 2** becomes a gated categorical: quiet checks at the first
  quiescence ply are built behind a switch that defaults off, and a
  paired run is owed only if they lower razoring's priced hidden share
  (§9 item 5). Two Stockfish-shaped engines read their removal as a gain.
- **The fail-low store** takes the donor's form in both searches on the
  arm (no move stored after a fail low), read by a paired run before the
  fit and bound by the floors. Three quarters of quiescence's fail lows
  search no move at all, so Basilisk's form there would be a new
  mechanism nobody has measured (§5.6). This is the decision the
  maintainer is most likely to want to overturn; it costs one default.
- Decision: `READY_FOR_IMPLEMENTATION` (§8). The contract is §10.

## 2. The head re-measured

`run_suite.py --depth 14 --stride 1 --hash 64` on `suite_v2`, the Diag
build of `8de2c95` (§13), `tools/results/b43-20261106/suite_d14.json`.
B.3.0's column is its §2.1 (the B.12 head, before Cluster 2 was fitted).

| Quantity | This head | B.3.0 | Reading |
|---|---:|---:|---|
| interior / quiescence nodes | 9,434,361 / 6,891,318 (quiescence **42.2%**, 0.73 per interior node) | 51.5M / 29.3M (36.2%) | the tree is about five times smaller and quiescence's share rose six points (B-28) |
| in-check interior nodes | 17.2% | 26.4% | the donor's check policy |
| quiescence exits, of its nodes | table cutoff 17.2%, stand-pat 27.3%, delta 3.8%, ply cap 0.008%, capture fail high 11.6%, fail low 30.7%, evasion node 9.5% | — | the seven sum to the node count exactly; PV nodes are 1.2% |
| stand-pat | replaced by a table bound at 2.2% of nodes that reach it; mean margin over beta at a cut **378** | — | the interpolation's reach (D6) |
| capture loop | 2.40 captures per node; futility 24.0%, SEE 29.1%, late rule 289 moves, searched 23.4% (the rest follow a cutoff) | — | the late rule is dead (B-18) |
| capture fail highs | first searched move 92.6%, mean index 0.089; mean margin over beta 213 | — | ordering is not the capture loop's problem |
| fail-low exits | 25.4% searched a capture | — | §5.6 |
| evasions | 1.85 searched per node; fail highs at 34.2% of nodes, **63.0%** on the first evasion, mean index 0.79; a legal table move at 4.5% of nodes | — | S9 |
| the main search's estimate | refined by a table bound at 17.7% of interior nodes; depth-0 entries are 25.5% of the refinements and 16.7% of their summed distance | — | B-20, B-24 |
| table move | none at 65.1% of interior nodes; from a depth-0 entry at 3.4% | — | §5.6 |
| reverse futility | 24.5% of interior nodes; 9.0% of cuts on a table bound; the correction term alone blocks 11.3% of the decisions that would otherwise cut, 17.0% of those on a table bound (0.53% of interior nodes) | 14.9% | RAR-S40 (D4) |
| razoring; null move; ProbCut | 3.3%; tries 6.7%, conversion 61.2%; tries 6.2%, conversion 76.1% | 5.1%; 7.5%, 41.5%; 1.4%, 54.0% | Cluster 2's fit |
| table | hits 40.3%; interior cutoffs 6.8%; 14.2M stores | 45.3%; 7.9% | |

The conversion floors of the head are B.4.2's table, read on the same
engine (`d3add9c`): KQ-K, KR-K and KBB-K 24 at each of the five budgets,
KBN-K 21 / 22 / 24 / 24 / 24, sum **115**. WAC at 100,000 nodes is
**247/300** (`wac100k_head.log`).

## 3. Mechanisms against the donor

Kernel symbols are `Searcher::quiescence`'s; donor lines are `sf_19`
`src/search.cpp`. "Arm" is the `B4_QSEARCH` arm.

| Mechanism | Basilisk today | Donor | Disposition on the arm |
|---|---|---|---|
| Table cutoff | non-PV, any entry, bound on the right side | the same (`:1709`) | KEEP |
| Stand-pat value | corrected static eval, replaced by a non-decisive table bound that proves a better estimate | the same (`:1729-1735`) | KEEP |
| Stand-pat cut | returns the stand-pat; stores it as a depth-0 lower bound | returns `(441·v + 583·beta)/1024`; stores the evaluation alone, and only on a miss (`:1745-1754`) | ADOPT the blend; KEEP the store, with the returned value (§14) |
| Known-loss guard | gain rules stand down when `stand_pat + 150 ≤ −KNOWN_WIN` (B.12) | `!is_loss(futilityBase)`, `!is_loss(bestValue)` | KEEP Basilisk's band on the new base |
| Delta rule | `stand_pat + 900 + 200 < alpha` cuts the node | none | REMOVE; deletion arm `B4QsDelta` (§9) |
| Ply cap | stand-pat returned at quiescence ply 10 | none | REMOVE without games (0.008% of nodes, D5) |
| Capture order | table move, then `16·victim − attacker + capture history` | table move, then `capture history + 7·victim` in the picker's units (`movepick.cpp`) | ADOPT: the picker's `capture_order_score` (5.2% of multi-capture nodes change their first pick, C3) |
| Futility base | the table-tightened stand-pat `+ 150` | the corrected static eval `+ 306` (`:1759`) | ADOPT the static eval; seed 86 (§7) |
| Futility exemptions | checks, promotions | checks, promotions, **recaptures** on the previous move's square (`:1790`) | ADOPT (`B4QsRecapture`) |
| Count rule | none; a late rule at index 6 with SEE below −50 | `moveCount > 2` prunes, inside the same exemptions (`:1793`) | ADOPT at 2 (`B4QsMoveCount`); the late rule goes |
| Futility prune | `base + gain ≤ alpha`; raises the best to it | the same (`:1796-1804`) | KEEP the form on the new base |
| Alpha-relative SEE | every capture: `see_ge(m, clamp(alpha − stand_pat − 200, −800, 200))`; the −800 is dead (B-17) | inside the exemptions: `see_ge(m, alpha − futilityBase)`, raising the best to `min(alpha, futilityBase)` (`:1807-1811`) | ADOPT |
| Plain SEE | none separate | every capture, checks included: `see_ge(m, −74)` (`:1819`) | ADOPT; −28 in Basilisk's units, which is 0 on today's vector (§7) |
| Evasions | every legal move in generator order; no table move, no pruning, no store | the picker's evasion stage with the table move; once the best is not a loss, quiets and SEE-losing captures are skipped (`:1787`, `:1815-1820`); stored on exit | ADOPT (`B4EvasionPrune`), through the kernel's own picker |
| Exit blend | none | `(462·v + 562·beta)/1024` above beta, not decisive (`:1871`) | ADOPT |
| Exit store | depth 0, lower or upper bound, the move that raised alpha; not in check | the same, in check too (`:1876`) | KEEP; in check too; the fail-low form is §5.6 |
| Stalemate | not seen: the stand-pat is returned | tested when no move was searched, the side has only king and pawns and the last capture was a piece (`:1863-1868`) | ADOPT (`B4Stalemate`); quiescence stands pat on 418 stalemates in 6.9M nodes (C7), and the donor's two conditions select a subset |
| Quiet checks | none (the old loop left with the legacy kernel) | none since 2024 | CARD 2: `B4QuietChecks`, default 0 (§9) |
| Quiet table move | ignored | delivered, counted by the count rule, then skipped | KEEP Basilisk's (§14) |

Outside quiescence, the two donor couplings PLAN assigns here:

| Mechanism | Basilisk today | Donor | Disposition |
|---|---|---|---|
| Reverse futility in a won root (S5) | depth cap `CoreRfpDepthCap` 10 always | `depth < (seekMate ? 6 : 19)` (`:996`) | ADOPT the cap of 5 under Cluster 2's `seek_mate` (`B4RfpSeekMate`) |
| The correction term on a table bound (RAR-S40) | added whatever the estimate's source | the same | KEEP: 0.53% of interior nodes, under the registered 1% (D4) |

## 4. PLAN B.4's inputs, disposed

| Input | Disposition |
|---|---|
| Card 1, the value scale | §6 C1 to C3: both rules fire; `B4SeeValues` and `B4GainValues`, two vectors each, exact default identity (§9 items 3, 4). The order question is settled by the picker's score |
| Card 2, first-ply quiet checks | `B4QuietChecks` 0 / 6, default 0, behind a zero-game gate on razoring's priced share (§9 item 5). The kernel has no check generation to remove |
| B-19 fail-soft | done at A.11.11 (BAS-S36); the arm keeps it |
| B-87 the tablebase band reachable without tables | closed by A.11.11 and B.12: no static value reaches the band, the stand-pat refuses a decisive table score, and the arm's blends refuse decisive values (§10 invariants) |
| B-21 the estimate moves under the blend | reverse futility, razoring and the null move's base join B.4.6's surface from the start (§5.2, §11) |
| B-22 quiet-evasion pruning; no store in check | the arm has both; `B4EvasionPrune` is read first, floors before games (§9 item 1) |
| B-25 margins as coordinates | futility base and the two blend weights are X lines; the count limit and the SEE margin are F lines (§7) |
| B-17 dead clamp; B-18 late rule | both go with the donor's forms (289 late-rule decisions in 16.3M nodes) |
| `MAX_QSEARCH_PLY` | removed on the arm (D5); `ply ≥ MAX_PLY` stays the backstop |
| B-27 decisive cutoffs at PV nodes | done at A.11.11: a PV node takes no table cutoff |
| B-16 the PV bit; B-23 no SEE exemption for checks | kept as they are |
| Simplification read: delta pruning | removed on the arm; its `[-5,0]` is B.4.7 (c). It is live (3.8% of nodes; BAS-S47 priced it at 3.08%), so the bracket is owed |
| Instruments B-20, B-24, B-28, B-26 | built at `1fd783b` (33 counters) and read in §2; B-26 is card 1's census |
| B-29 WAC gains did not predict games | the reason card 2's gate is priced razoring, not WAC |
| B-2 the late-mate exposure of `CoreLmpExemptChecks` 0 | named: a quiet mate-giving check late in a node's order is skipped by the count rule and, with quiescence captures-only, is seen only when a search reaches it. `B4QuietChecks` is the one place B.4 can read it; Rarog's late-mate fixture stays a known-failing case and no test asserts it |
| B-63 the dead shallower rule | B.8's deletion; untouched here |
| S9 evasion order | §2: 63.0% first-move fail highs; the arm orders evasions through the picker with the table move (ticket 1) |
| Futility card details | §3: base, recapture exemption, count rule, capture order |
| S5's RFP half | §3; ticket 7 |
| TS-13, TM-5 | card 2's priors and the reason its read is taken on the arm, not on the head |
| TS-14 negative priors | not adopted: no passed-pawn push, no quiet table move and no material stand-pat in quiescence |
| Already the donor's form | nothing to do |
| B.3.4's fail-low store | §5.6: one form for both searches, the donor's, as the arm's default; read before the fit |
| RAR-S40 | §3, D4: not material; no categorical |
| B.3's hand-on (its §5.11) | `probcut_*` and `null_*` are re-read on both arms in B.4.5; the small table ProbCut stays off (`B3ProbCutTt` 0); the `seekMate` cap is ticket 7. The oracle's `VALUE_KNOWN_WIN` bound on reverse futility is not adopted: the donor's cap answers the same mate-drive question and Cluster 2 already carries its producer |

## 5. Interactions with Clusters 1 and 2's fitted cores

Producer → stored state → consumer, with the head's numbers. Quiescence
is 42.2% of all nodes, so every row below moves a population the earlier
fits saw.

### 5.1 What quiescence returns, and who reads the distance

The blends pull every fail high toward beta: a stand-pat cut returns 43%
of its margin (mean 378 today), a capture or evasion fail high 45% (mean
213). The value stays a valid lower bound at or above beta. Readers of
the distance, all in the main search: the deeper re-search after a
reduced fail high (`CoreResearchDeeper` 13), the singular bars, the
fail-high blend at interior nodes, aspiration's next window, and every
table bound a parent stores from a quiescence child. BAS-S36 showed these
readers respond to leaf distances (the fail-soft repair read +12.1 where
−2 was predicted), so the sign of the blend is not assumed: it is a
coordinate whose range reaches 1024, today's form.

### 5.2 The estimate reverse futility and razoring read (B-21)

The pruning estimate is a table bound at 17.7% of interior nodes, and a
quarter of those bounds are quiescence's. Reverse futility takes 9.0% of
its cuts on a table bound; razoring reads the same estimate and then
returns quiescence's value. Under the blend a depth-0 lower bound sits
closer to beta, so fewer reverse-futility cuts clear their margin on it,
and razoring's returned value changes where the node fails high.
`CoreRazorMargin`, the six `CoreRfp*` coordinates and `B3NmpBase` are on
B.4.6's surface from the start, as PLAN requires; Rarog's depth-14 tree
grew 41% at this step.

### 5.3 ProbCut and the null move into quiescence

ProbCut's pre-verification is a quiescence search at
`(−probcut_beta, −probcut_beta + 1)`; it converts 76.1% of 6.2% of
interior nodes. A null move whose reduced depth is 0 or less lands in
quiescence. Both read the arm's values and its pruning. They are not
edited; `probcut_cuts/tries`, `null_cuts/tries` and the null move's
depth-1-to-3 bucket are read on both arms in B.4.5, and `B3ProbCutBase`,
`B3ProbCutImproving` and `B3NmpPerPly` join the surface by 5.8's rule.

### 5.4 Evasions against the check policy

Cluster 2 removed the in-check extension, so a check given at depth 1 is
answered in quiescence: evasion nodes are 9.5% of quiescence. Today each
searches every evasion; the arm searches the table move and the picker's
order, then stops delivering quiets once one evasion holds. 47.0% of
evasion nodes have at most one quiet evasion and are untouched. In the
rest the rule removes up to 45.0% of the evasions searched. In generator
order 40.6% of those raise the node's value and 4.6% fail high (C6); the
picker's history order will lower both, by an amount only the arm can
show. The readers of an evasion node's value are the checking capture
above it in quiescence, razoring, ProbCut's pre-verification and the
depth-1 moves of the main search, which is where the mate drives live.
Hence the order of §9: the floors at five budgets with the rule on and
off, then the paired run, before anything else is read.

### 5.5 The evasion store and the table move

Evasion nodes do not store today, so only 4.5% hold a legal table move.
On the arm they store a depth-0 bound and the move that raised alpha.
Consumers: the same node's next visit (order), the main search's table
move at that position (3.4% of interior nodes take theirs from a depth-0
entry today), and the estimate refinement, which never runs in check.
The stored static evaluation of an in-check node is the same "none"
marker the main search stores for one.

### 5.6 The fail-low store, decided for both searches

Today the main search stores the best-scored move after a fail low
(`B3FailLowStore` 0) and quiescence stores none, which is the donor's
form. PLAN asks for one form.

- 74.6% of quiescence's fail-low exits searched no capture: every one
  was pruned or none existed. Basilisk's form could act only at the
  other 25.4% (537,809 nodes), where the best-scored capture is the
  highest of several null-window upper bounds.
- Its consumers would be the main search's `tt_capture` (the LMR term
  `CoreLmrTtCapture` 964, applied to every move of the node), IIR and
  the two no-table-move terms: a refuted capture would suppress IIR and
  raise reductions at up to 2% more interior nodes.
- For the main search, the donor's form has one game read, +3.3 ± 9.1
  unfitted (B.3.2), and on B.3.1's arm it halved the bench tree at WAC
  249 against 241. It failed two conversion reads there that research
  amendment 6 found to be knife-edge in both directions.

**Decision.** The arm's single form is the donor's: `B3FailLowStore` 1
for the main search, quiescence as it is. `B3IirMinDepth` and
`CoreLmrCutNodeNoTt` join B.4.6's surface, as PLAN provides. Under
`B3FailLowStore` 0 the arm gives both searches Basilisk's form, so the
switch still names one form for both. The binding reads of ticket 6
(floors at five budgets, the promotion fixture, WAC) decide the default
without games if the donor's form fails them and Basilisk's passes; the
paired run of §9 item 2 then reads the pair before the fit.

Counter-argument: the maintainer's rule is to keep a working form, the
main search's form works, and a paired run on consumers fitted for form
0 is tilted against form 1. The answer is that a single form is
required, the tilt runs against the form chosen here, and the fit that
follows covers it.

### 5.7 Card 1's vectors against the fitted thresholds

SEE's values are multiples of 100, so every SEE threshold in the search
is a staircase (`CoreQuietSeeCoeff` is an F line for that reason). The
HCE's vector makes bishop-for-knight a loss of 27 and turns the
thresholds continuous. Every threshold fitted in B.2 and B.3 was fitted
on vector 0: the picker's good-capture split, `CoreCaptSeeCoeff` 68,
ProbCut's SEE filter. A paired run of vector 1 is therefore tilted
against it; it can adopt and cannot clear (§14). The gain vector touches
quiescence's futility alone, whose base is fitted after the choice.

### 5.8 The surface rule

As B.3.0 §5.12: a Cluster 1 or 2 coordinate joins B.4.6's surface when,
on B.4.5's depth-14 read of the arm against the OFF arm, its mechanism's
firing share moves by 10% or more of itself. Named unconditionally by
PLAN: `CoreRazorMargin`, `CoreRfpPerPly`, `CoreRfpImproving`,
`CoreRfpWorsening`, `CoreRfpCorrMult`, `CoreRfpBetaWeight`,
`CoreRfpDepthCap`, `B3NmpBase`; and, while the store is at 1,
`B3IirMinDepth` and `CoreLmrCutNodeNoTt`. Candidates under the rule,
frozen here: `B3NmpPerPly`, `B3ProbCutBase`, `B3ProbCutImproving`,
`CoreCaptFutBase`, `CoreCaptSeeCoeff`, `CoreQuietFutBase`,
`CoreLmrTtCapture`, `CoreResearchDeeper`.

### 5.9 What B.4 hands on

- B.5: the blended leaf values feed aspiration's fail-high and fail-low
  distances; `ply ≥ MAX_PLY` in check still returns the static
  evaluation (Rarog sweep item 2, B.5's).
- B.6.1: §14's table.
- B.8: the OFF arm's delta rule, late rule, ply cap and generator-order
  evasions are deleted with the arm's switch.

## 6. Zero-game reads, done

Both builds read `bench 13` 6,090,091, and every counter the two share
is equal over the suite, so the census build searched the same tree.

| # | Result | Against the prediction | Verdict of its rule |
|---|---|---|---|
| C1 | 227,159 of 21,980,006 calls, **1.03%**; quiescence's test 38,074 of 3,674,898, 1.04% | all calls above the 0.2 to 0.8% range; quiescence inside | above 1%: SEE's vector becomes injectable |
| C2 | 120,349 of 5,241,633, **2.30%** | inside | above 2%: the gain vector becomes a categorical |
| C3 | the picker's score changes the first pick at 5.24% of 1,931,689 multi-capture nodes; the HCE vector at 1.56% | both inside | none |
| C4 | 1,032,391 captures, 14.8% of those generated; 157,828 of them searched today, of which 38.2% raise the best and **14.5%** fail high; 22,921 fail highs, 2.88% of quiescence's | population and fail-high share inside; "under 5% raise the best" missed by a wide margin | 2.88% is under 5%: the limit is the donor's 2, fixed |
| C5 | 411,267 of 1,675,656 futility prunes, **24.5%** | above the 5 to 15% range | none |
| C6 | 543,948 quiet evasions, 45.0% of evasions searched; **40.6%** raise the best; 4.6% fail high (25,057, 11.2% of evasion fail highs) | the share just under its range; both outcomes above theirs | none |
| C7 | 418 of 6,891,318, 0.006% | inside | none |
| D1 | evasions 63.0%, captures 92.6%; a legal table move at 4.5% | evasions three points above; the rest inside | none |
| D2 | 17.7% of interior nodes; depth-0 entries 25.5% of refinements, 16.7% of distance | the node share and the distance share just under | none |
| D3 | 25.4% of fail-low exits; 3.4% of interior nodes | the first inside; the second under | inputs of §5.6 |
| D4 | 49,804 nodes, 0.53% of interior | 0.03 over the prediction | under 1%: no categorical |
| D5 | 573 of 6,891,318, 0.008% | inside | under 0.1%: the cap goes without games |
| D6 | 378 at stand-pat cuts; 213 at capture fail highs | inside | none |
| WAC | 247/300 | one above the range | the arm's floor is 239 |

Calibration. Six of the fourteen ranges missed, four of them in one
direction: the moves the donor's rules would remove are better than
predicted (C4's 38% and 14.5%, C6's 40.6%, C5's 24.5%). The prediction
treated a capture or evasion late in today's order as a move that had
already lost, because the order is good where it is measured (92.6%
first-move cutoffs). That figure is over fail highs only. It says
nothing about the moves a fail-low node goes on to search, and in
generator order an evasion's position carries no information at all.

Two consequences are written into the contract rather than into a rule
changed after exposure. C4's limit stays at 2, as registered, and is an
F line so that a large value turns the rule off for an ablation. The
rules C4 and C6 size are priced on the arm in B.4.5 before any game
(ticket 0 adds their pricing rules), and §11's P4 and P5 state what
that read must show.

## 7. Seeds

By B.0 §7.2: plies, counts and blend weights take the donor's value
(rule 1); an evaluation-unit constant takes the donor's converted value
(× 0.282, rule 3) unless it lies more than 2× outside the range of the
oracle's converted value and Basilisk's own (rule 2); a material-unit
threshold converts by × 0.38 (rule 5).

| Constant | Donor | Converted | Oracle | Basilisk today | Rule | Seed | Line |
|---|---|---:|---|---|---|---|---|
| Futility base over the static eval | 306 | 86 | 141 (68) | 150, over the tightened stand-pat | 3: 86 lies inside 68 to 150 | **86**, range 40 to 220 | X `B4QsFutBase` |
| Stand-pat blend, weight of the value in 1024ths | 441 | — | none | 1024 (no blend) | 1 | **441**, range 256 to 1024 | X `B4QsStandPatBlend` |
| Exit blend, weight of the value in 1024ths | 462 | — | none | 1024 | 1 | **462**, range 256 to 1024 | X `B4QsFailHighBlend` |
| Count limit | 2 | — | none | none | 1 | **2**, range 1 to 64 | F `B4QsMoveCount` |
| Plain SEE margin | −74 | −28 | 0 | none | 5 | **−28**, range −200 to 0 | F `B4QsSeeMargin` |
| Reverse futility's depth cap under `seek_mate` | `depth < 6` | — | none | none | 1 | **5**, range 1 to 19 | F `B4RfpSeekMateCap` |
| Quiet checks searched at the first ply | 0 | — | first ply, uncapped | 0 | — | **0**; read at 6 (BAS-D21's setting) | S `B4QuietChecks` |
| SEE vector | — | — | — | 100/300/300/500/900 | card 1 | vector 0; vector 1 is 88/316/343/538/1041 | S `B4SeeValues` |
| Gain vector | donor's `PieceValue` | — | — | 100/300/300/500/900 | card 1 | vector 0; vector 1 as above | S `B4GainValues` |

The SEE margin is an F line because on vector 0 every value from −99 to
0 prunes the same captures. The count limit is an F line because it is
a count. The futility seed moves far from today's 150 and is flagged for
the sweep.

## 8. Decision: `READY_FOR_IMPLEMENTATION` (2026-11-06)

§2, §5, §6 and §7 are complete; every switch has a registered rule and
bracket (§9); the contract lists its tickets with their fingerprint
rules and tests (§10); the predictions are frozen and the three
registrations written (§11).

Two decisions are taken here as recommended and can be overturned for
one default each before B.4.4 starts:

1. **The fail-low store is the donor's in both searches on the arm**
   (§5.6). Main counter-argument: the main search's present form works
   and its consumers were fitted for it.
2. **Quiet-evasion pruning is on in the arm** (§5.4), read first and
   bound by the floors. Main counter-argument: C6's 40.6%, and Rarog's
   zero-game rule had set it off before its game read adopted it.

Not implemented, and why:
- the oracle's `VALUE_KNOWN_WIN` bound on reverse futility (§4);
- Triumviratus's three quiescence additions, on their negative priors;
- Reckless's subtree marking and a SEE exemption for checks (B-16, B-23);
- a tuned SEE vector: card 1 asks whether decisions move, and the answer
  is two fixed vectors read by games, not five new coordinates;
- any change to the main search's move loop, ProbCut or the null move.

## 9. Categoricals, attribution switches and the deletion arm

Tune-build UCI options, each settled by one 2,000-game paired run on one
tune build at `3+0.03` (PROCESS *Cluster delivery shape*: adopted at
+5 Elo or more, rejected at −5 or less, each only when the 95% interval
also excludes 0; left at the default otherwise; never an SPSA
coordinate). Items 1 and 2 are read before the whole-arm paired run,
which then runs on the defaults they leave; items 3 to 5 follow it. Arm
A is the alternative, arm B the default.

| # | Option | Values | Default | What decides | Bracket |
|---|---|---|---|---|---|
| 1 | `B4EvasionPrune` | 1 once an evasion holds, quiets and SEE-losing captures are skipped; 0 every evasion is searched | 1 | first the floors at five budgets and the mate fixtures on both settings: if 1 fails a binding read that 0 passes, the default is 0 without games. Then a paired run, 0 against 1 | ±5 |
| 2 | `B3FailLowStore` on the arm | 1 no move after a fail low in either search; 0 the best-scored move in both | 1 (the arm's default; the OFF arm keeps 0) | first ticket 6's binding reads, by the same rule; `tt_move_absent` and `tt_move_d0` on both settings at depth 14; then a paired run, 0 against 1 | ±5 |
| 3 | `B4GainValues` | 0 the search's values; 1 the HCE's | 0 | a paired run, 1 against 0 | ±5 |
| 4 | `B4SeeValues` | 0 the search's values; 1 the HCE's, for every `see_ge` and `see` call | 0 | a paired run, 1 against 0. Tilted against 1 (§5.7): it can adopt, never clear | ±5 |
| 5 | `B4QuietChecks` | 0 none; 6 up to six quiet checks with SEE ≥ 0 at the first quiescence ply | 0 | first the gate: `Pricing 4` at depth 16 on `suite_v2` with 0 and with 6. A paired run, 6 against 0, is owed only if razoring's beat share at expected cut nodes (25.0% on the B.3 head) falls by a quarter of itself or more with 6. Otherwise the card closes at 0 with that reading | ±5 |

Attribution switches. Their defaults are the donor's form; a paired run
never moves them. They exist so that a negative whole-arm run is ablated
by switch before any mechanism is blamed, and so the floors can read one
rule alone.

| Option | Values | Default |
|---|---|---|
| `B4QsRecapture` | 1 a recapture on the previous move's square is exempt from the count rule, futility and the alpha-relative SEE; 0 not exempt | 1 |
| `B4Stalemate` | 1 the stalemate test; 0 none | 1 |
| `B4RfpSeekMate` | 1 reverse futility's depth is capped at `B4RfpSeekMateCap` under `seek_mate`; 0 no cap | 1 |
| `B4QsDelta` | 0 no delta rule; 1 the OFF arm's delta rule | 0 |
| `B4QsMoveCount` (F) | the count limit; 64 turns the rule off | 2 |

Deletion arm: the delta rule, live at 3.8% of quiescence nodes. Its
`[-5,0]` SPRT is B.4.7 (c): the fitted arm against itself with
`B4QsDelta` 1, on one tune build. The late rule and the ply cap are
deleted without games (289 moves and 573 nodes in 16.3M).

## 10. The B.4.4 contract (`I2`)

- **Goal and exact semantics.** §3's "Disposition" column with §7's
  seeds and §9's defaults, behind one CMake `option(B4_QSEARCH ... OFF)`
  (compile definition `BASILISK_B4_QSEARCH`, `constexpr bool
  kB4Qsearch`), flipped ON in one commit after B.4.7. Both arms build at
  every commit. The OFF arm reproduces `bench 13` **6,090,091** at every
  commit; the ON arm's fingerprint is recorded per ticket. Engineering is
  the implementer's. The preferred form is B.3's: `if constexpr`
  branches in the existing kernel, both arms compiling on every build.
  The OFF arm's code is edited only for seams both arms share (counters,
  trace, parameters the OFF arm leaves at their defaults).
- **Why here.** §1, §5, §6.
- **Producer → stored state → consumer.** §5. New state: none on the
  stack. Quiescence reads `(ss − 1)->move` for the previous move's
  square, the picker's continuation rows for evasions, and the driver's
  `root_depth_` and `root_score_` through `b3::seek_mate` for ticket 7.
- **Files and subsystems.** `src/search_kernel_core.cpp` (quiescence;
  reverse futility's cap; the exit store's move), `src/search_params.h`
  and `src/parameters.cpp` (a `BASILISK_B4_PARAMS(X, F, S)` group and
  the arm's own default for `b3_fail_low_store`, in the manner of
  `BASILISK_B3_CORE_DEFAULTS`), `src/search_types.h` (`kB4Qsearch`),
  `src/search_diagnostics.h`, `.cpp` (counters, trace events),
  `src/search_pricing.h`, `.cpp` (two rules), `src/board.h`, `.cpp`
  (the SEE vector, tune builds only), `src/move_picker_core.*` only if
  the evasion delivery needs a seam, `CMakeLists.txt`,
  `.github/workflows/ci.yml`, `tools/diag/run_suite.py`,
  `tools/diag/decision_trace.py`, `tools/diag/pricing.py`,
  `tools/generate_spsa_surface.py` (a B4 table), `tests/`.
- **Invariants that must not change.** DESIGN §3:
  - no static score in the mate band, and no blend applied to a decisive
    value;
  - a value quiescence returns at or above beta is still at or above
    beta after its blend, and a stored lower bound is the value returned;
  - the stand-pat never takes a decisive table score;
  - a mate score from quiescence is a proven mate: no evasion is skipped
    while the node's best is a loss;
  - table scores are ply-adjusted on store and probe, the stored static
    evaluation is the raw one, and an in-check node stores none;
  - every table move is validated before it is played, the evasion
    picker's included;
  - a PV quiescence node takes no table cutoff and its exit store keeps
    the PV bit;
  - the gain rules stand down in the known-loss band (B.12), on both
    arms: the wrong-bishop draws bind at every ticket;
  - the draw value stays 0, the stalemate's included;
  - the per-ply stack unwinds, and quiescence stays allocation-free;
  - the conversion floors, the mate fixtures and the WAC floor bind both
    arms at every ticket.
- **Maintainer-owned expensive gate.** B.4.5's paired runs (BAS-S50),
  then B.4.7's SPRTs (BAS-S52), §11.
- **Non-goals and adjacent mechanisms not to alter.** The main search's
  move loop, razoring, reverse futility beyond ticket 7's cap, the null
  move, ProbCut, singular extensions, LMR, the histories and their
  updates, the correction, the table's replacement rule, root,
  aspiration and time. No constant outside §7 is moved by hand. No
  quiescence addition the donor lacks, except card 2's switch. No tuning
  before B.4.6. A Cluster 1 or 2 coordinate that 5.8 pulls onto the
  surface is fitted there, never adjusted in B.4.4.

### The arm's quiescence, stated once

Entry, the draw and repetition tests, the table probe and the non-PV
table cutoff are today's. Then:

**In check.** The best starts at minus infinity. Evasions come from the
kernel's picker as at an interior in-check node, the table move first
when it is a legal evasion. For each evasion, when the best so far is
not a loss (above `−tablebaseWinInMaxPly`) and `B4EvasionPrune` is 1: a
quiet evasion is skipped (so quiet delivery may end), and a capture or
promotion with `!see_ge(m, B4QsSeeMargin)` is skipped. Otherwise it is
searched. No legal move is mate, `−(MATE_SCORE − ply)`.

**Not in check.** The static eval is the corrected one; the best starts
there and takes a non-decisive table bound that proves a better
estimate, as today.
- Stand-pat: if the best is at or above beta, it becomes
  `(B4QsStandPatBlend · best + (1024 − B4QsStandPatBlend) · beta) / 1024`
  unless decisive, is stored as a depth-0 lower bound with no move, and
  is returned.
- Alpha rises to the best. `futility_base = static eval + B4QsFutBase`:
  the corrected static eval, not the tightened best. The gain rules run
  only when `futility_base > −KNOWN_WIN`.
- With `B4QsDelta` 1 the OFF arm's delta rule runs here, on the best.
- Captures and promotions are the legal captures ordered by the picker's
  capture score, the table move first when it is one of them. A quiet
  table move is neither searched nor counted. `move_count` counts each
  capture delivered, pruned ones included.
- For each, in this order:
  1. If the gain rules run, the move does not give check, is not a
     promotion, and is not a recapture (its destination is not the
     previous move's destination; with `B4QsRecapture` 0 this test is
     dropped):
     - `move_count > B4QsMoveCount`: skipped;
     - `futility_value = futility_base + gain`, the gain being the
       captured piece's value in the gain vector (a pawn for en
       passant): if `futility_value ≤ alpha`, the best rises to it and
       the move is skipped;
     - `!see_ge(m, alpha − futility_base)`: the best rises to
       `min(alpha, futility_base)` and the move is skipped.
  2. `!see_ge(m, B4QsSeeMargin)`: skipped. Checks, recaptures and
     promotions included.
  3. Searched.
- With `B4QuietChecks` above 0, at the first quiescence ply only and
  after the captures without a cutoff: up to that many legal quiet
  checks with `see_ge(m, 0)`, in generator order, searched as captures
  are.
- Stalemate (`B4Stalemate` 1): when no capture was delivered, the side
  to move has no non-pawn material, the previous move captured a knight
  or heavier, and the side has no legal move, the best is 0.

**Both.** A searched value above the best becomes the best; above
alpha it names the best move and raises alpha, or ends the loop at
beta. On exit, a best above beta that is not decisive becomes
`(B4QsFailHighBlend · best + (1024 − B4QsFailHighBlend) · beta) / 1024`.
The store is depth 0, a lower bound at or above beta and an upper bound
otherwise, the PV bit kept. The move stored is the best move; with
`B3FailLowStore` 0 a fail low stores the best-scored searched move
instead. There is no late rule and no quiescence ply cap.

### Tickets

Ordered; one mechanism each; every ticket keeps the OFF arm's exact
fingerprint and records the ON arm's. From ticket 1 on, each ticket ends
with the ON arm's `bench 13` counters at stride 1 (Diag build) against
the previous ticket's. A Cluster 1 or 2 row whose share moves by more
than 20% of itself is reported with its cause in the implementation
record and not repaired. The floors at five budgets, the wrong-bishop
draws and `test_wac --count` are read on the ON arm at every ticket
that changes its fingerprint.

0. **The switch and the instruments.** The option, the definition and
   `kB4Qsearch`; a CI cell that configures `-DB4_QSEARCH=ON`, builds
   `all`, runs CTest and prints `bench 13`; the `BASILISK_B4_PARAMS`
   group, advertised by tune builds on the arm alone; the B4 table in
   `generate_spsa_surface.py`.
   - Counters, appended and never renamed, each with its denominator in
     the table's comment and its unit in `run_suite.py`:
     `qs_count_pruned`, `qs_recapture_exempt`, `qs_evasion_quiets_pruned`,
     `qs_evasion_see_pruned`, `qs_stalemates`, `qs_quiet_checks`,
     `rfp_seekmate_capped`.
   - Trace events `QsCountPrune`, `QsEvasionPrune`, `QsStalemate`;
     `decision_trace.py` learns the names.
   - Pricing rules `QsCount` and `QsEvasion`, as `QsFutility` is priced
     (the move searched in quiescence with the node's window), with
     `pricing.py` reporting them; the isolation check repeated on the
     first 20 positions of `suite_v2` at depth 10.
   - Both arms 6,090,091: the arm is empty.
1. **Order.** Captures by the picker's capture score with the table
   move first; evasions through the kernel's picker with the table
   move. No pruning changes. Test: a position whose table move is a
   legal evasion searches it first; one whose table move is a quiet is
   unaffected outside check. Read: evasion first-move fail highs.
2. **Evasions.** The two evasion rules behind `B4EvasionPrune`, and the
   exit store in check. Tests: a mate found through an evasion node is
   reported at the same distance with the switch at 0 and 1 (the
   mate-recognition fixtures of `test_endgames` and the KQK
   shortest-mate test of `test_search`); an in-check node with one
   holding capture and several quiets searches one quiet at most; a
   node whose every evasion loses searches all of them. Read: the
   floors at five budgets on both settings, recorded, with §9 item 1's
   rule applied.
3. **The gain rules.** The base on the static eval, the recapture
   exemption, the count rule, the alpha-relative SEE with its raise,
   the plain SEE margin, the delta rule behind `B4QsDelta`, the late
   rule and the ply cap gone. Tests: the known-loss band (the
   wrong-bishop test on both arms); a queen recapture below alpha by
   more than the futility margin is searched with `B4QsRecapture` 1
   and pruned with 0; the third non-checking capture is skipped at
   limit 2 and searched at 64; en passant is credited a pawn.
4. **The blends.** Stand-pat and exit, with the stored value. Tests:
   the blend's arithmetic at the two seeds and at 1024 (identity); a
   decisive value is untouched; a returned fail high is at or above
   beta for every weight in range.
5. **Stalemate.** Test: a position where the side to move, a bare king
   with blocked pawns after a piece was captured, has no legal move
   reads 0 at depth 1 on the arm with the switch at 1 and the static
   evaluation with it at 0.
6. **The fail-low store.** The arm's default 1 and quiescence's form
   under 0. Binding reads on both settings: the floors at five budgets,
   the promotion fixtures of `test_search`, `test_endgames`, WAC at
   100,000 nodes. If 1 fails one that 0 passes, the arm's default is 0
   and the implementation record says so; no return is needed.
7. **Reverse futility's cap.** `depth ≤ B4RfpSeekMateCap` when
   `b3::seek_mate` holds, behind `B4RfpSeekMate`. Test: the gate with
   forced `root_depth_` and `root_score_`, as the singular half has.
8. **The vectors.** `B4GainValues`, and `B4SeeValues` through a seam in
   `Board` that tune builds alone compile; release builds keep the
   constants. Tests: at 0 the tune build's `bench 13` equals the release
   build's on both arms; at 1 each moves it; `test_board`'s SEE fixtures
   pass on vector 0 and the oracle agrees with vector 1 on its own
   values.
9. **Quiet checks.** `B4QuietChecks`. Test: at 0 the fingerprint is
   ticket 8's; at 6 a quiet mate-giving check at the first quiescence
   ply is found at depth 1.
10. **The final state.** The Ablate flavour's quiescence-facing bits
    re-declared if any names a deleted rule; the liveness proof on the
    arm; `test_wac --floor` registered on the arm; the sanitizer suite
    on both arms; the two-arm table in B.3's shape (`bench 13` on
    Release, Tune, Diag and Ablate, `bench 10` sanitizer, CTest, WAC at
    depth 6 and at 100,000 nodes); CI green on both arms.

### Cheap qualification (end of B.4.4, before the review)

- The two-arm table, every fingerprint reproduced from a clean tree.
- The arm's WAC at 100,000 nodes **≥ 239** and the depth-6 floor.
- The floors on the arm at the five budgets: KQ-K, KR-K and KBB-K 24 at
  each; KBN-K's sum **≥ 112**; the wrong-bishop draws.
- The deep-iteration cost screen on B.3's five positions (bench
  positions 11 and 15, `suite_v2` positions 50, 42 and 60), ON against
  OFF, read by the amended clause of B.3.0's research amendment 7.
- The decision trace on B.2's four positions, both arms, archived.
- Ticket 2's and ticket 6's reads with their verdicts.

### Review

A separate session reproduces every fingerprint and test on both arms,
reads each ticket's diff against this section and §7, checks the
invariants above, and records its acceptance or return in this packet
before B.4.5 starts.

## 11. Frozen predictions and the B.4.5 to B.4.7 registrations

Frozen 2026-11-06, before any ON-arm build exists. Calibration is
appended in the experiment entries, never here. IDs reserved:
**BAS-S50** (B.4.5), **BAS-S51** (B.4.6), **BAS-S52** (B.4.7); each
entry file is created at its leaf's opening from the text below, before
any game.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P1 | The unfitted paired run (2,000 games, ON against OFF at one revision, §7's seeds, the defaults items 1 and 2 leave) reads **+5 Elo**, 80% interval [−20, +30]; probability positive 60% | low-moderate | below −30 is a defect or a wrong seed: ablate by switch in the order `B4EvasionPrune`, `B4QsMoveCount` 64, `B4QsRecapture`, the blends at 1024, `B4QsFutBase` 150 |
| P2 | After B.4.6 and the gate, B.4 reads **+10 Elo** at STC, 90% interval [−3, +28]; H1 at `[0,3]` probability 50% | low-moderate | H0 with P3 to P5 held says a donor-shaped quiescence carries no Elo on this HCE; the arm stays off and B.5 proceeds |
| P3 | Size: quiescence's share of nodes at depth 14 falls from 42.2% to **30 to 40%**; the arm's `bench 13` is 0.6 to 1.3× the OFF arm's | moderate | above 45% says the recapture exemption outweighs the count rule; the sweep reads `B4QsFutBase` first |
| P4 | Evasions: searched per node falls from 1.85 to **≤ 1.45**; first-move fail highs rise from 63.0% to **≥ 78%**; a legal table move at **≥ 15%** of evasion nodes; `QsEvasion`'s priced beat share **≤ 8%** | moderate | a beat share above 12% says history order does not rescue the rule: item 1's paired run is then read with that figure beside it |
| P5 | Gain rules: `QsCount`'s priced beat share **≤ 6%**; `QsFutility`'s falls from 4.27% to **≤ 3.5%**; recaptures exempted are 15 to 30% of the moves the gain rules test | moderate | `QsFutility` above 4.3% says the hidden share was not the recaptures; `B4GainValues` and the futility base's sweep are read before the whole-arm run is blamed |
| P6 | Tactics: WAC at 100,000 nodes in **[239, 255]** | moderate | under 239 is a floor failure and returns the leaf |
| P7 | Floors: KQ-K, KR-K, KBB-K 24 at every budget; KBN-K's sum **≥ 112**; the wrong-bishop draws hold | moderate | a miss attributes by switch, zero-game: `B4EvasionPrune`, `B3FailLowStore`, `B4RfpSeekMate` |
| P8 | The estimate: the depth-0 share of the main search's refinement distance falls from 16.7% to **≤ 13%**; reverse futility's cuts on a table bound stay within 7 to 10% of its cuts | low-moderate | none: sizes 5.2 for the fit |
| P9 | Speed: pooled NPS **0.96 to 1.01×** the OFF arm (`gives_check` is computed for more captures; evasions pay the picker) | moderate | below 0.95 returns the evasion delivery of ticket 1 to implementation for its cost before the gate |
| P10 | Categoricals: item 1 within ±6 (60%); item 2 within ±5 (60%); item 3 within ±5 (65%); item 4 at or below +5 (75%); item 5's gate not met (60%) | low-moderate | item 1 at −5 or below with its interval excluding 0 flips the default and re-reads the floors |
| P11 | The delta rule's deletion: B.4.7 (c) accepts H1, removal is not worse (75%) | moderate | H0 restores the rule on the arm as `B4QsDelta` 1 and its margin becomes a coordinate |

Stop rules: P1 below −30 (then the ablation order above); a floor
failure at any ticket returns the leaf; PLAN rule 6 for the programme.

### BAS-S50 — B.4.5: Cluster 3 diagnostics, the unfitted paired run and the categoricals

- Baseline / candidate: one revision, the accepted B.4.4 head; the OFF
  arm against the ON arm (`-DB4_QSEARCH=ON`), §7's seeds, §9's defaults;
  PGO pairs per arm; the categoricals on one tune build of the arm, the
  alternative as arm A by UCI option.
- Zero-game screens, all recorded before the first game: §2's table on
  both arms at depth 14, with 5.8's candidates' shares and verdicts;
  `Pricing 4` at depth 16 on `suite_v2` on both arms (P4, P5), and with
  `B4QuietChecks` 6 for item 5's gate; WAC at 100,000 and 400,000
  nodes; the floors at five budgets; the canary calibration of B.3.2,
  re-used if the OFF arm is unchanged; the deep-iteration cost screen if
  B.4.4 ran it on an earlier revision; pooled NPS on an idle host.
- Items 1 and 2: their binding reads, then 2,000 games each.
- The whole-arm paired run: 2,000 games at `3+0.03`, 1T, Hash 64,
  paired reversed UHO openings, no adjudication, tablebases off,
  Colosseum `match-fixed`, the recount from the PGN. It governs: P1's
  interval and the −30 stop.
- Items 3 to 5 in order, 2,000 games each; item 5 only past its gate.
- Expected: P1, P3 to P10. Falsification: P1's stop; the floors.

### BAS-S51 — B.4.6: the curvature sweep, then SPSA in blocks

- Surface: `B4QsFutBase`, `B4QsStandPatBlend`, `B4QsFailHighBlend`;
  PLAN's reverse futility, razoring and null-move base coordinates
  (5.8's unconditional list); `B3IirMinDepth` and `CoreLmrCutNodeNoTt`
  while the store is at 1; and each 5.8 candidate that B.4.5's read
  admits. F lines and every switch stay off it. The generated
  configuration is cited by hash.
- Curvature sweep (PLAN rule 8), classification frozen before the first
  point: `B4QsFutBase` at 43, 65, 86, 129, 172; `B4QsStandPatBlend` and
  `B4QsFailHighBlend` each at 256, 348, 441 (462), 732, 1024, the last
  being no blend; `CoreRfpPerPly` and `CoreRazorMargin` at 0.5×, 0.75×,
  1×, 1.5× and 2×. Per point: `bench 13`, WAC at 100,000 nodes, paired
  depth at 300,000 nodes (`sweep.py`). Flat or monotone on all five
  skips the SPSA and the seeds ship.
- SPSA: Colosseum blocks on the tune build at `3+0.03`, the registered
  estimator and block rule; theta is the last completed block's rounded
  centres; the fitted-constant analysis precedes the bake; the bake is
  one commit with the arm's new fingerprint.

### BAS-S52 — B.4.7: the Cluster 3 gates

- (a) The fitted ON arm against the OFF arm at the bake revision, SPRT
  `[0,3]` nElo at `3+0.03`, 1T, Hash 64, paired reversed UHO, no
  adjudication, tablebases off, clean final-PGO builds and manifests.
  The default flips ON on H1.
- (b) The fitted arm against the unfitted arm, SPRT `[0,3]`, only if
  B.4.6 ran its SPSA.
- (c) The delta rule's deletion: the fitted arm (`B4QsDelta` 0) against
  itself with `B4QsDelta` 1, one tune build, SPRT `[-5,0]`.
- (d) A direction read, not a gate: 2,000 games at `10+0.1` of (a)'s
  pair.
- Reject: H0 on (a) leaves the default off, keeps the evidence and the
  fingerprint record. Nothing changes after a game is observed.

## 12. The four questions

1. **Mechanism.** A quiescence that spends its nodes on the captures
   that can change the result. It searches the two best-ordered captures
   plus every check, recapture and promotion; it prunes the rest by a
   static margin on the evaluation the capture would leave, not on a
   table bound; it answers a check with ordered evasions and stops once
   one holds; and it returns a fail high close to the bound it proved,
   so the main search's margins read a leaf's value rather than its
   excess. The chess property: a recapture is the move a static margin
   misjudges most (the exchange is half done), and a third quiet-looking
   capture is the one it misjudges least.
2. **Interactions.** §5, with numbers; the surface rule 5.8.
3. **Invariants.** §10's list.
4. **Falsifiers.** P1 to P11 and the stop rules; the floors at every
   ticket; the priced rules of P4 and P5 before any game; §9's reads,
   each on its own bracket.

## 13. Artifacts (`tools/results/b43-20261106/`, ignored storage)

| Artifact | Identity |
|---|---|
| Counter build | `tools/test_engines/basilisk-b43-diag-pext-diag-pgo.exe`, revision `8de2c95` clean, `bench 13` 6,090,091, SHA-256 `BEF86CBAA222B10B5759A91940231AECC4CC8ABB18F6254FED4208FBB07CE5A8` (manifest beside it) |
| Depth-14 counter suite | `suite_d14.json`, `suite_d14.log` (106 positions, Hash 64, stride 1, every identity holds) |
| Census instrument | `census.patch`, SHA-256 `FD040E2A02F6AE934FDDE7E29642656F8A8F50E501A62A59A5C8D0D1543465E6`; applies to `8de2c95` (`git apply --check`); never committed |
| Census build | `basilisk-b43-census-diag-nopgo.exe`, the patch on `8de2c95`, release preset with `-DDIAGNOSTIC=ON`, no PGO, `bench 13` 6,090,091 (`bench_census.log`), SHA-256 `B1E654FDC4FBAB373D4614D26C6257E8E3211DE0E2676452605A278DB4BFD98D` |
| Depth-14 census suite | `census_d14.json`, `census_d14.log` |
| WAC at 100,000 nodes | `wac100k_head.log` on `basilisk-b43-release-nopgo.exe` (`8de2c95`, release preset, no PGO, SHA-256 `44EB165CF10A34E875443CD154A74B05B45C7267B16FE72132F4BF70DD581081`), 28,533,731 nodes |
| The donor's sources | `git -C D:/code/stockfish show sf_19:src/search.cpp`, `sf_19:src/movepick.cpp` (`edb0d9db`) |

## 14. Departures table (for B.6.1)

Every place the arm keeps Basilisk's form where the donor's differs.
B.4.5 and B.4.7 add the switches that end at Basilisk's form.

| Place | Basilisk's form (kept) | The donor's form | Why kept | Waits on |
|---|---|---|---|---|
| Stand-pat store | a depth-0 lower bound with the returned value | the evaluation alone, on a miss only | it works, and the blend leaves it a valid bound; no read | H.2: a switch and both arms refitted |
| Known-loss guard | the gain rules stand down when the futility base is at or below `−KNOWN_WIN` | `!is_loss` on mate and tablebase scores | BAS-S40: the HCE's known-win band is far below the mate band | nothing; it leaves with the HCE |
| Quiet table move outside check | not delivered, not counted | delivered, counted, then skipped | the donor's form spends a count slot on a move it never searches; TS-14 read searching it as a loss | nothing |
| En passant's gain | a pawn | the empty destination square, 0 | exact | nothing |
| Stalemate test | legal generation whenever the two cheap conditions hold | a pawn-push pre-filter first | exact: a pinned pawn defeats the pre-filter | nothing |
| Evasion order | the kernel's picker stages (good captures, quiets, bad captures) | captures by victim, then quiets by history | one picker for both searches | H.2, if B.4.5's P4 fails |
| SEE vector (`B4SeeValues` 0) | 100/300/300/500/900 | the donor has one vector for SEE and evaluation-free material | §9 item 4's read, tilted against vector 1 | H.2: both arms refitted, unless item 4 adopts |
| Reverse futility's ordinary cap | `CoreRfpDepthCap` 10, fitted | 18 | the fit's | B.6's joint surface |

## Read R2 — futility priced by recapture and captured piece (registered 2026-11-06, before running)

Asked by the maintainer after the packet closed, as a read to take
before B.4.4 starts. §1 infers that quiescence futility's hidden share
sits in the recaptures and in the queen's value, from two reads that
were never joined: BAS-S47 priced the rule as a whole (4.27% of its
decisions beat it) and C5 counted its population (24.5% recaptures).
This read joins them.

- **Instrument.** B.16.2's pricing with one throwaway change,
  `tools/results/b43-20261106/pricing_split.patch` (SHA-256
  `AEB02F97B4B46A6E52AD1D2DA041D45AEC7B2ADB676536C0C5818A5FC07F4FE8`,
  on `de4a8d0`). Quiescence's cells always carry depth 0, so the depth
  slot takes the captured piece (1 pawn to 5 queen) plus 6 for a
  recapture on the previous move's square, for the futility and SEE
  rules. The per-rule sampling stride is untouched, so the same
  decisions are probed.
- **Validity, before the read counts.** The patched Diag build reads
  `bench 13` 6,090,091, and `pricing.py isolation --every 4 --depth 10
  --limit 20` finds no position that differs. Otherwise it is not read.
- **Recipe.** `python tools/diag/pricing.py read <build> --every 4
  --depth 16` on `suite_v2` (Hash 64), BAS-S47's reading recipe.
- **Exposure before freezing.** BAS-S47's table on the B.3 head and §6;
  nothing from this build, which does not exist yet.

| # | Prediction | Probability |
|---|---|---|
| Q1 | Futility's beat share on this head is 3.5 to 5.0% | 70% |
| Q2 | Recaptures' beat share is at least twice the other captures' (about 8 to 12% against 2 to 3.5%) | 60% |
| Q3 | Recaptures carry at least half of futility's beats | 55% |
| Q4 | By captured piece, the queen has the highest beat share, at least 10% | 55% |
| Q5 | The alpha-relative SEE rule's recaptures beat it more often than its other captures | 55% |

Rules, fixed here:

- **(a)** Q3 holds: §1 stands as written.
- **(b)** Recaptures' beat share is under 1.5 times the other captures':
  §1's inference is wrong. An amendment withdraws the claim that the
  exemption is the cluster's lead mechanism and re-registers P5. The
  exemption stays in the arm as the donor's form behind its switch.
  The futility base's seed, 86 against today's 150, becomes the arm's
  named main risk, and B.4.6's sweep reads it first.
- **(c)** Between the two: §1 stands with the measured shares written
  beside it.
- The queen's share (Q4) changes no rule: it is evidence beside
  `B4GainValues`' paired run.

No branch changes §10's contract.
