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

## Research amendment 1 — read R2's result (2026-11-06)

Run by the agent on the registered recipe. The patched build reads
`bench 13` 6,090,091 and the isolation check finds no differing position
(20 positions, 174,772 probes), so the read counts: 106 positions at
depth 16, `Pricing 4`, 31,212,872 search and 80,650,493 probe nodes
(`pricing_split/`; `python tools/results/b43-20261106/read_pricing_split.py
tools/results/b43-20261106/pricing_split` reproduces the tables).

### 1. Result

Beat share is the share of probed decisions whose move, searched,
exceeds alpha, with its 95% interval.

| Quiescence futility | Decisions | Beat % |
|---|---:|---:|
| all | 3,686,844 | 4.18 [4.14, 4.22] |
| recaptures | 939,239 (25.5%) | **14.77** [14.62, 14.91] |
| other captures | 2,747,605 | **0.56** [0.54, 0.57] |

Recaptures carry **90.1%** of the rule's beats.

| Captured piece | Decisions | Beat %, all | Recaptures | Other captures | HCE value less the 100-scale |
|---|---:|---:|---:|---:|---:|
| pawn | 2,760,504 | 0.31 | 1.23 | 0.16 | −12 |
| knight | 469,344 | 10.41 | 17.19 | 1.83 | +16 |
| rook | 105,163 | 14.28 | 19.07 | 3.93 | +38 |
| bishop | 344,254 | 22.41 | 31.82 | 4.61 | +43 |
| queen | 7,579 | 58.51 | 62.24 | 45.88 | +141 |

By move number, recaptures against other captures: moves 1 to 3, 15.72%
against 0.68%; moves 4 to 6, 4.38% against 0.12%.

The alpha-relative SEE rule: 1.60% over 4,472,173 decisions; recaptures
3.81%, other captures 1.37%; a captured queen 23.49% (recaptures 20.18%,
other captures 27.27%, 53,344 decisions).

Beside them on this head: the delta rule 3.33%, razoring 13.42%, reverse
futility 2.81%.

### 2. Predictions scored

| # | Result | Verdict |
|---|---|---|
| Q1 | 4.18% | held |
| Q2 | 14.77% against 0.56%, a factor of 26 | held; both of the sizes named in brackets missed, in opposite directions |
| Q3 | 90.1% of the beats | held |
| Q4 | the queen at 58.5% | held, far above the 10% named |
| Q5 | 3.81% against 1.37% | held |

Calibration: the direction was right and the size was not. The
prediction spread the hidden share over both classes; it sits in one.
A static margin is nearly exact for a capture that starts an exchange
(0.56%) and wrong one time in seven for a capture that finishes one.

### 3. Verdict of the registered rules

Rule (a): §1 stands as written. The recapture exemption is the
cluster's lead mechanism, now measured rather than inferred.

What the numbers add, without changing a rule:

- **The exemption's size.** With recaptures exempt, the futility rule's
  remaining population reads 0.56% at today's margin of 150. The seed
  of 86 prunes more of that population, so the arm's figure will lie
  above 0.56%; P5 is re-registered below.
- **Card 1.** Among captures that are not recaptures, the beat share
  rises with the gap between the HCE's value and the credited one:
  pawn 0.16%, knight 1.83%, rook 3.93%, bishop 4.61%, queen 45.88%.
  This is evidence beside `B4GainValues`' paired run and changes no
  rule, as registered.
- **The count rule.** The moves it adds to what futility already
  prunes sit at move 4 or later, where captures that are not recaptures
  beat the rule 0.12% of the time. P5's `QsCount` clause stands.

### 4. A finding outside the contract

Capturing a queen is misjudged by both gain rules whether or not it
recaptures: 45.9% of the queen captures futility prunes and 27.3% of
those the alpha-relative SEE rule prunes beat alpha when searched. An
even queen trade has an SEE of 0 on any vector, so this is not card 1's
unit question. The likely cause is that removing a queen moves the
evaluation's king-safety terms by more than any material margin; that
is a hypothesis, not a measurement.

The arm exempts the recaptures among them and keeps pruning the rest.
The rest is small: 26,810 decisions in 31.2M search nodes, about a
twentieth of the recaptures' hidden moves. It is not added to the arm.
B.4.5 reads it on the arm (clause 2 below); a queen capture exempt from
quiescence's gain rules is a candidate for H.2 on that reading.

### 5. Predictions affected and clauses replaced

Both made before any ON-arm build exists and before any game.

1. **P5, substantive.** Old: "`QsFutility`'s falls from 4.27% to
   **≤ 3.5%**", with the falsifier "`QsFutility` above 4.3% says the
   hidden share was not the recaptures". New: "`QsFutility`'s beat share
   on the arm is **≤ 1.5%**", with the falsifier "above 2.5% says the
   base of 86 costs more than the exemption saves; B.4.6's sweep reads
   `B4QsFutBase` first, and the whole-arm run is not blamed before it".
   P5's other clauses stand.
2. **§10, ticket 0, the pricing clause.** Old: "Pricing rules `QsCount`
   and `QsEvasion`, as `QsFutility` is priced (the move searched in
   quiescence with the node's window), with `pricing.py` reporting
   them". New: the same, and "the cells of quiescence's move rules
   (`QsFutility`, `QsSee`, `QsCount`) carry read R2's class, the
   captured piece and whether the move recaptures, as
   `pricing_split.patch` does; `pricing.py report` prints a rule by
   class; a test covers the decoding". B.4.5's pricing screen (BAS-S50)
   reads P5 and §4's queen captures by class on both arms.

No other clause of §10 or §11 changes.

### 6. Artifacts (`tools/results/b43-20261106/`)

| Artifact | Identity |
|---|---|
| Instrument | `pricing_split.patch`, SHA-256 `AEB02F97B4B46A6E52AD1D2DA041D45AEC7B2ADB676536C0C5818A5FC07F4FE8`; never committed |
| Build | `basilisk-b43-pricing-diag-nopgo.exe`, the patch on `cd2c294` (engine source of `1fd783b`), release preset with `-DDIAGNOSTIC=ON`, no PGO, `bench 13` 6,090,091 (`bench_pricing.log`), SHA-256 `7304C0F7165583B8D5DED814E6ABDC9FE336ECEA6B8786B70E62F23903B330E6` |
| Isolation | `pricing_isolation.log`: 20 positions at depth 10, 0 differ |
| The read | `pricing_split/runs.jsonl` (SHA-256 `154BE911…EFEE5B7E`), `pricing_split/summary.json` (`FD4973FA…C4FAF644`), `pricing_split.log` |
| The reader | `read_pricing_split.py` |

## Implementation record — tickets 0 to 7, at Return 1 (2026-11-06)

Branch `b4-quiescence`, from `bc8e449` (`b4bbefe` with one PLAN-only commit
on top). Builds: the release preset, PEXT portable, Clang 23.1.5, non-PGO
`all` trees (`build/b4-{on,off}`, `-diag`, `-tune`). `bench 13` is the
release binary's; the Diag and Tune binaries of the same source read the
same count at every ticket. Tickets 8 to 10 have not started. Evidence:
`tools/results/b44-20261106/<ticket>/` (ignored storage): `report.txt`,
`compare.md`, `counters_on.json`, the floors' and WAC's logs. The reader is
`read_ticket.py` beside them.

### Per ticket

| Ticket | Commits | OFF `bench 13` | ON `bench 13` | WAC depth 6 / 100k | KQ-K, KR-K, KBB-K | KBN-K by budget, sum |
|---|---|---:|---:|---|---|---|
| 0 | `8a15250`, `49480f9` | 6,090,091 | 6,090,091 | 133 / 247 | 24 at every budget | 21/22/24/24/24, 115 |
| 1 | `e429e48` | 6,090,091 | 6,740,352 | 137 / 244 | 24 | 22/23/24/24/23, 116 |
| 2 | `d9b11ab` | 6,090,091 | 4,997,291 | 139 / 250 | 24 | 24/23/23/24/24, 118 |
| 3 | `6d5a23f`, `ea3b49e`, `f94804b` | 6,090,091 | 4,287,895 | 141 / 252 | 24 | 23/24/24/23/24, 118 |
| 4 | `86feb7b` | 6,090,091 | 4,757,482 | 148 / 248 | 24 | 23/24/23/24/23, 117 |
| 5 | `f34b85e`, `3e1f41a` | 6,090,091 | 3,824,402 | 149 / 247 | 24 | 23/24/23/24/23, 117 |
| 6 | `af06733`, `dbc552e` | 6,090,091 | 4,003,560 | 149 / 258 | 24 | 23/23/24/24/24, 118 |
| 7 (Return 1) | `b003b3c` | 6,090,091 | **3,166,820** | 149 / 257 | **KR-K 24/23/24/24/24** | 22/23/23/22/24, 114 |

Release CTest passed 27/27 on both arms at the end of every ticket from 1
on (26/26 at ticket 0). The arm's Diag build ran `test_stack_unwind`,
`test_b4_qsearch`, `test_b3_proof` and `test_search` with node checks live
at every ticket, and passed. The wrong-bishop draws are in `test_endgames`,
inside CTest. `test_b4_qsearch` holds 110 checks in a release build and 158
in Diag on the arm (102 and 129 on the OFF arm). A temporary worktree built
`all` on both arms at each of the thirteen commits: every build passed, the
OFF arm read 6,090,091 at each, and the arm read the table's counts
(`every_commit.txt`).

### The registered zero-game rules

**§9 item 1, `B4EvasionPrune` (ticket 2).** Read on ticket 2's tree.

| Setting | `bench 13` | Floors | KBN-K sum | `test_endgames` | `test_search` | WAC 6 / 100k |
|---|---:|---|---:|---|---|---|
| 1, the default | 4,997,291 | 24 | 118 | pass | pass | 139 / 250 |
| 0 | 5,005,640 | 24 | 119 | 191/191 | 716/716 | 141 / 242 |

1 fails no binding read, so the default stays 1. The paired run is B.4.5's.

**§9 item 2, `B3FailLowStore` on the arm (ticket 6).** Read on ticket 6's tree.

| Setting | `bench 13` | Floors | KBN-K sum | `test_endgames` | Promotion fixtures | WAC 6 / 100k | `tt_move_absent`; `tt_move_d0`, depth 14 |
|---|---:|---|---:|---|---|---|---|
| 1, the arm's default | 4,003,560 | 24 | 118 | pass | pass | 149 / 258 | 72.58%; 3.76% |
| 0 | 3,548,571 | 24 | 117 | 191/191 | pass | 148 / 249 | 62.00%; 6.74% |

1 fails no binding read, so the default stays 1. The variant's one
`test_search` failure is the arm-default assertion reading the default the
variant build changed.

### Cluster 1 and 2 rows that moved by more than 20%

Shares of interior nodes on the bench positions at depth 13, stride 1,
against the previous ticket; none repaired. Full tables:
`<ticket>/compare.md`; `flagged_rows.py` sizes each by position.

- **`cont_hist_pruned` at tickets 1 (+60.7%), 2 (−60.8%), 3 (+75.8%) and
  6 (+55.0%).** Bench position 11 carries it: 70%, 82%, 51% and 47% of the
  row's gross movement. That position's tree swings with the arm's order and
  pruning; the row's total ends at 70,070 against 79,965 at ticket 0.
- **`null_verifications` and `null_region_refusals`.** Moved at tickets 1,
  2, 3, 4, 5 and 6, on 21 to 350 events in about 2.5M interior nodes.
  Positions 36 and 39 carry most of it; verification starts at depth 16,
  so the row follows which bench trees reach it.
- **Ticket 3: `tt_cutoffs` +22.3%, `hist_reward_updates` +28.7%,
  `sing_negative_cut` +32.1%, `sing_ttbeta` +30.8%, `tt_penalized` +25.8%.**
  No position carries these (at most 15% of the movement). The gain rules
  change what depth-0 entries hold across the tree, and every row reads the
  table.
- **Ticket 2: `sing_negative_cut` −20.5%,** position 11 again (52%).
- **Ticket 6: `follow_pv_nodes` +30.9%,** on a tree 0.80× the size, so
  the share rises with the smaller denominator.
- **Ticket 7:** none.

### Deviations from the handoff

1. **The start.** The branch stood at `bc8e449`, not `b4bbefe`: one commit
   placing the queen-capture finding in PLAN, no source.
2. **The WAC floor moved early, twice.** `test_wac --floor` has a stale
   bound, and the arm's count at depth 6 passed it at tickets 3 and 5. §10
   registers the arm's floor at ticket 10, but CTest had to stay green at
   every commit, so each move is its own test commit ahead of the engine
   commit that needed it (`6d5a23f`, `f34b85e`), on the file's own rule (the
   count less 13). The floor is 136 for now; ticket 10 re-registers it.
3. **`test_tt_switches_live`.** It compared the default against
   `B3FailLowStore` 1, which the arm's default makes the same setting. Its
   precondition is fixed in its own commit (`af06733`): it reads 0 against 1.
4. **The evasion picker's depth is 0.** "As at an interior in-check node"
   does not name it. Quiescence's depth is 0, so the picker sorts scores
   from 0 up and leaves the negative ones in generator order, where an
   interior node's limit is −3560 per ply. First-move fail highs read 98.7%
   at ticket 7, so nothing here asks for more. §14's evasion row stands.
5. **Quiet delivery ends early.** The arm calls `skip_quiets()` as soon as
   the best stops being a loss, so a node that holds on a capture never
   generates its quiets. The contract's wording, a quiet skipped when it
   arrives, selects the same moves.
6. **The stalemate takes no blend.** §10's "Both" blends any best above
   beta, and its invariants say the draw value stays 0, the stalemate's
   included. The two meet only when the static eval is below a beta that
   is below 0. I took the invariant: the value is exact, and the store is a
   lower bound at or above beta and an upper bound otherwise.
7. **Pricing's shape on the arm.** `QsSee` covers both SEE rules, as
   `qs_see_pruned` does. `QsEvasion` carries the class as well, so a quiet
   reads as class 0 and a SEE-losing capture by its victim; that is one
   more rule than amendment 1 named, and it costs nothing. Quiets are
   priced and counted, each one, only at a node that ended without a
   cutoff: a cutoff proves they hid nothing.
8. **`qs_recapture_exempt`'s denominator.** P5 reads it against "the moves
   the gain rules test", which no counter holds. The table's comment names
   the moves the loop delivered, `qs_searched` plus the rules' prunes,
   rather than adding an eighth counter.
9. **Ticket 3's band test is direct.** The wrong-bishop draws pass on the
   arm with the band's guard removed, because `Kxh8` is a recapture and is
   exempt either way. `test_the_known_loss_band_stands_down` holds a
   capture that is no recapture, and that mutant fails it (`f94804b`).

### What the reviewer must check

- Each ticket's diff against §10 and §7, with deviations 4 to 7 above read
  as judgements, not as settled.
- Both fingerprints at every commit, and the tests on both arms.
- The mutants: replacing the loss test in the two evasion rules with
  `evasions_searched > 0` fails two sections of `test_b4_qsearch`, and
  `prune_by_gain = true` fails the band's (`mutate.py`).
- Pricing isolation on the arm: 20 positions at depth 10, 114,787 probes,
  0 differ (`return1/isolation_on.log`); on the empty arm at ticket 0,
  174,772 probes, 0 differ.

Not yet read, all ticket 10's or the qualification's: the sanitizer suite
on both arms, the Ablate flavour, the liveness proof, the two-arm table
from a clean tree, CI, the deep-iteration screen and the archived traces.

## Return 1 — B.4.4 ticket 7: reverse futility's cap costs a KR-K floor (2026-11-06)

**The premise.** §11's P7 and §10's invariants: KQ-K, KR-K and KBB-K convert
24 of 24 at each of the five budgets, at every ticket. §11's stop rule: a
floor failure at any ticket returns the leaf. Ticket 7, built as §10 writes
it (`b003b3c`), reads **KR-K 23 at 45,000 nodes**. Tickets 1 to 6 read 24
at every budget. No later ticket can repair it: 8 and 9 hold their
fingerprint at their defaults, so ticket 7's tree is the arm's final tree.

**The failure.** `4K3/8/8/8/8/5k2/8/1R6 w`, reproduced over UCI with the
game's history, a fresh process per move (`return1_playout.py`).

- With the cap the rook shuffles to the rule-50 draw at 100 plies. No
  search reads a mate.
- With `B4RfpSeekMate` 0 it mates in 37 plies, first reading a mate at ply
  24.
- The two playouts are identical to ply 15. At ply 16 the capped search
  plays `h4h5` where the other plays `d7c6`.

This is the shape of B.3's Return 6: a long shuffle in a won ending at one
low budget.

**Attribution by switch (P7's falsifier).** Whole floors, five budgets,
temporary variant builds restored byte-exact (`return1/variant_*.txt`).

| Setting, the rest at defaults | `bench 13` | KR-K | KBN-K sum | WAC 6 / 100k |
|---|---:|---|---:|---|
| Defaults (ticket 7) | 3,166,820 | **24/23/24/24/24** | 114 | 149 / 257 |
| `B4RfpSeekMate` 0 (ticket 6's tree) | 4,003,560 | 24 | 118 | 149 / 258 |
| `B4EvasionPrune` 0 | 3,645,533 | 24 | 118 | 139 / 250 |
| `B3FailLowStore` 0 | 3,413,322 | 24 | 119 | 148 / 249 |
| `B4RfpSeekMateCap` 4 | 3,440,919 | 24 | 116 | 149 / 258 |
| `B4RfpSeekMateCap` 6 | 3,234,635 | 24 | 117 | 149 / 257 |
| `B4RfpSeekMateCap` 8 | 3,470,506 | 24 | 117 | 149 / 258 |
| `B3SeekMateDepth` 16 | 7,399,149 | 24 | 117 | 149 / 257 |

KQ-K and KBB-K are 24 at every budget in every row. `test_endgames` passes
in every row. `test_search` passes in every row, but for the arm-default
assertion in the `B3FailLowStore` 0 variant, as at ticket 6.

**How far the miss reaches.** The position alone, plies to mate at twelve
budgets (−2 is the rule-50 draw; `return1/sweep.txt`):

| Setting | 30k | 35k | 40k | 42.5k | 45k | 47.5k | 50k | 55k | 60k | 75k | 90k | 120k |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Cap 5 (default) | 27 | 27 | 27 | 71 | **−2** | 41 | 41 | 47 | 35 | 43 | 31 | 27 |
| Switch off | 27 | 27 | 29 | 71 | 37 | 41 | 41 | 49 | 45 | 37 | 51 | 25 |
| Cap 4 | 27 | 27 | 27 | 41 | 41 | 41 | 41 | 55 | 35 | 35 | 41 | 27 |
| Cap 6 | 27 | 27 | 27 | 71 | 41 | 41 | 41 | 39 | 29 | 43 | 49 | 47 |
| Cap 7 | 27 | 27 | 27 | 71 | 67 | 65 | 41 | 31 | 31 | 43 | 37 | 31 |
| Cap 8 | 27 | 27 | 29 | 71 | 67 | 41 | 41 | 55 | 45 | 39 | 37 | 25 |
| `B3SeekMateDepth` 16, cap 5 | 27 | 27 | 27 | 39 | 39 | 39 | 39 | 55 | 27 | 35 | 27 | 31 |
| OFF arm | 43 | 29 | 37 | 15 | 15 | 15 | 31 | 31 | 29 | 33 | 47 | 41 |

One cell in 96 fails, and the position's neighbours on both axes convert.

**A fact the contract did not carry.** The cap reads `b3::seek_mate`, and
§7 seeds the cap from the donor's `depth < 6`. But the donor's bar is root
depth 16, and Basilisk's is 12: the B.3 packet's amendment 3 lowered
`B3SeekMateDepth` for the singular half, to bound bench position 11. So the
arm's cap binds four plies of root depth earlier than the donor's would.

**Inference.** This reads as a knife-edge in the ending's tree more than a
loss of technique. The cap fires rarely (426 refusals against 502,535 cuts
on the bench positions), and at 45,000 nodes this ending completes depth 9
to 13, so the cap touches the last iterations only, which a bar of 16 would
not reach. That is why the row at 16 converts, but it is not clean evidence:
the bar moves the singular half too, which is the 7,399,149. The read does
not show the cap is harmful and does not show it is harmless: the floors
exist to catch fragile conversion early (the B.3 packet's amendment 6, §4.3).

**As built and kept.** Tickets 0 to 7 stand as committed, ticket 7 with
`B4RfpSeekMate` 1 and the cap at 5 as contracted. The counter
`rfp_seekmate_capped` and the arithmetic `b4::rfp_past_seek_mate_cap` stay
whichever way this goes.

**Options.**

1. *Default `B4RfpSeekMate` to 0 and read it in B.4.5.* Every binding read
   holds on ticket 6's tree, unchanged (floors 24, KBN-K 118, WAC 258). The
   switch becomes a categorical with a paired run, which §9 does not give
   it: this needs the reclassification amendment 6 made for the store. Cost:
   the arm enters B.4.5 without S5's half of the donor's mate drive, and its
   bench tree is 1.26× ticket 7's.
2. *Move the cap's seed.* Caps 4, 6 and 8 all pass. §7's rule 1 gives the
   donor's 5, no mechanism names another value, and a seed chosen by the
   cell it passes is the selection amendment 6 §4.4 warned against, though
   this one is an F line on no fit surface. 6 is the nearest to the donor's.
3. *Re-read the clause.* Accept 23 at 45,000 as a knife-edge. Amendment 6
   §4.3 declined this for a floor; a maintainer who wants it decides the
   floor clause too.
4. *Give the cap its own bar.* An F line at the donor's root depth 16,
   read only by reverse futility, restores the donor's pairing and leaves
   the singular half at 12. It is a new coordinate and its floors are
   unread: the row above moves both halves. It is the option that answers
   the mechanism, and it needs an amendment to §7 and ticket 7.

Recommendation: option 4, read before anything else. The contract meant
the donor's cap, and the donor's cap is paired with the donor's bar; the
read is one build. If its floors hold, ticket 7 lands as the donor's form
with no default moved. If they do not, option 1 keeps every floor and every
seed and sends the verdict to games, where amendment 6 sent the store. Main
counter-argument: 16 is chosen by the donor's number, not by evidence here,
and a bar this KR-K search rarely reaches may make the cap nearly inert at
`3+0.03`, which is close to option 1 with one more coordinate to carry.

**State at the return.** Tickets 0 to 7 are complete and committed; the OFF
arm reads 6,090,091 at every commit; release CTest passes 27/27 on both
arms. Tickets 8 to 10 have not started, because AGENTS.md resumes
implementation only on the amended contract. They do not depend on this
choice, except that ticket 8's and ticket 9's identity reads take ticket
7's final fingerprint. No game ran, no default was moved and no NPS read
was started.

## Reads A2 — Return 1's reads (registered 2026-11-06, before running)

Registered at `632bbb3`, before any of them runs. Class `R2`. They answer
Return 1. Every read is a node-budget or fixed-depth read, so the host's
load does not move it; no game, SPRT, SPSA or timed read is part of this.
Evidence goes to `tools/results/b44-20261106/amend2/` (ignored storage).

**Facts read before registering, none of them a result.**

- The donor's `seekMate` at `edb0d9db` is `rootDepth >= 16 &&
  abs(score) >= 2000`, read by reverse futility (`depth < (seekMate ? 6 :
  19)`) and by singular extensions alone. Its commit (`598ae2c4` in the
  donor's repository) states the purpose as mate finding, measured on a
  mate suite at fixed nodes, and passed non-regression bounds
  `<-1.75, 0.25>` at two time controls. It claims no Elo.
- Basilisk's cap reads Cluster 2's `B3SeekMateDepth`, which is 12: the B.3
  packet's amendment 3 lowered it to bound a singular-extension tree.
  Reverse futility's ordinary reach here is `CoreRfpDepthCap` 10 against
  the donor's 18, so the cap removes depths 6 to 10.
- `root_score_` is the previous iteration's score, and UCI `cp` is the
  internal score, so the bar's 564 is 564 in a PGN's `s=` field.

**The candidate form ("own bar").** A throwaway patch, never committed:
a new F line `B4RfpSeekMateDepth`, seed 16, range 8 to 64 (B.0 §7.2 rule
1, the donor's ply), read only by reverse futility's cap in place of
`B3SeekMateDepth`. `B3SeekMateScore`, `B4RfpSeekMateCap` 5 and
`B4RfpSeekMate` 1 are unchanged, and the singular half keeps its bar of
12. Trees: `CAND` the patch at its defaults; `T6` ticket 6's tree, the
committed source with `B4RfpSeekMate` default 0; `T7` the committed
source (cap 5 on the bar of 12); `OFF` the OFF arm.

**Instrument checks, which must hold before a read counts.**

- `T6` reads `bench 13` 4,003,560 and `T7` 3,166,820 (the implementation
  record's counts). The patch's Tune build reads 3,166,820 with
  `B4RfpSeekMateDepth` 12 and 4,003,560 with `B4RfpSeekMate` 0.
- The cap is live on `CAND`: on `8/8/8/4k3/8/8/8/4K2R w` at depth 18 the
  Diag build counts `rfp_seekmate_capped` above 0, and the Tune build's
  node count differs between `B4RfpSeekMate` 1 and 0.
- The UCI cold replay (one process per playout, `ucinewgame` before every
  move) reproduces `return1/sweep.txt` on four cells of the Return 1
  position: cap 5 at 30,000 (27 plies), 45,000 (the rule-50 draw) and
  60,000 (35), and the switch off at 45,000 (37). If it does not, the
  replay uses a fresh process per move.
- The source is restored byte-exact after every temporary build (`git
  status` clean), and each patch is saved with its SHA-256.

### R1 — the floors' base rate (question 3)

`test_endgames --conversion --nodes N`, with a print of every playout,
on `OFF`, `T6`, `T7` and `CAND`, at 25 budgets: the five registered ones
(30,000, 45,000, 60,000, 90,000, 120,000) and 25,000, 27,500, 32,500,
35,000, 37,500, 40,000, 42,500, 47,500, 50,000, 52,500, 55,000, 57,500,
65,000, 70,000, 75,000, 80,000, 85,000, 95,000, 105,000 and 135,000.
Each family is 24 positions, so 600 playouts a family a tree. Reported
per tree and family: playouts not converted, budgets reading under 24,
mean plies; for KBN-K also the count at each budget.

| # | Prediction | Probability |
|---|---|---|
| A2-P1 | `OFF` converts all 600 in each of KQ-K, KR-K and KBB-K | 70% |
| A2-P2 | `T6` converts all 600 KR-K playouts | 55% |
| A2-P3 | `T7` fails 1 to 3 KR-K playouts of 600, the known one included | 65% |
| A2-P4 | `CAND` equals `T6` playout for playout in KR-K at every budget | 70% |

Verdict rules.
- **Base rate.** For a tree and family, the rate is unconverted playouts
  over 600, and the chance that a five-budget read shows a miss is
  estimated as `1 − (1 − b/25)^5`, `b` being the budgets under 24.
- **Whose miss.** Over KQ-K, KR-K and KBB-K together (1,800 playouts a
  tree): the miss is `T7`'s own when `T7` fails at least 3 more than
  `T6`; the trees are not told apart when they differ by 2 or fewer.
- **The stop rule.** §11's rule stands as written whatever this reads,
  and no floor is relaxed on it. If `OFF` and `T6` both read 0 of 1,800
  it stands unqualified. If either reads 1 or more, the amendment records
  the false-alarm rate and puts to the maintainer, for later clusters
  only, whether a floor is registered over a denser grid.

### R2 — how often the bar is reached in games (question 2)

`tools/results/b42-pins-gate/games.pgn`: 3,000 games at `3+0.03`, no
adjudication, the moves of `basilisk-b42-pins-pext-pgo` alone (`bench
13` 6,090,091, today's OFF arm). A move is *live at bar B* when its
recorded depth is at least B and its recorded score is at least 564
either way, a mate score included. Reported for B of 12 and 16: the share
of moves, of games with at least one such move, and of recorded nodes.

| # | Prediction | Probability |
|---|---|---|
| A2-P5 | At bar 12: 12 to 30% of moves, and at least 60% of games | 70% |
| A2-P6 | At bar 16: 4 to 15% of moves, and at least 35% of games | 65% |

Verdict rule: a bar is *live in games* at 2% of moves and 20% of games or
more; *nearly inert* under 1% of moves or under 5% of games; *marginal*
between. The depths are the OFF arm's; the arm's smaller tree reaches a
bar more often, so the shares are lower bounds for it.

### R3 — how often the cap fires at game budgets (question 2)

Every 225th move of that engine in the PGN's order (about 600
positions), rebuilt from the game's FEN and moves, searched cold
(`ucinewgame`, Hash 64) at the node count the PGN records for the move,
on the arm's Diag builds: the committed one (bar 12) and the patch's
(bar 16). Reported: searches with a refusal, `rfp_seekmate_capped`
against `rfp_cuts`, over all searches and over the live ones.

| # | Prediction | Probability |
|---|---|---|
| A2-P7 | At bar 12 the refusals are under 1% of reverse futility's cuts over the sample; at bar 16 they are under a third of bar 12's | 75%; 65% |

No verdict rule of its own: it sizes R2.

### R4 — the mechanism's own read: mates found at fixed nodes (question 2)

The donor justified the cap by mates found at a node budget, so that is
read here. Populations: (A) the 96 floor positions; (B) from the games of
the R2 PGN that end in checkmate, the positions 10, 16 and 24 plies
before the mate, the first 150 such games (450 positions). Each is
searched cold at 250,000 and at 1,000,000 nodes on the patch's Tune
build under three settings: the switch off; bar 12; bar 16. A mate is
*found* when the last completed iteration's score is a mate score of
either sign. Reported per setting, population and budget: mates found,
and among positions every setting solves, how often each announces the
shorter mate.

| # | Prediction | Probability |
|---|---|---|
| A2-P8 | Pooled over both populations and budgets (1,092 searches a setting), bar 16 finds within ±8 mates of the switch off, and bar 12 within ±12 | 60%; 55% |

Verdict rule, per bar against the switch off, on the pooled count:
*benefit* when it finds at least 5% more mates and no fewer in either
population; *harm* when it finds at least 5% fewer; *no effect shown*
otherwise.

### R5 — conversion at budgets that reach the bar (question 1)

(a) The four families at 250,000 and 1,000,000 nodes on `T6`, `T7` and
`CAND`, with `OFF` for reference. (b) Won game positions: for each game
of the R2 PGN in order, the first position where that engine is to move
with a recorded score of +564 or more, not a mate score, at depth 12 or
more; the first 240 games that have one. One engine plays both sides from it, a cold search
per move at `N` nodes, on the patch's Tune build under the three
settings, until mate, stalemate, the rule-50 draw, a threefold
repetition, insufficient material or 300 plies. `N` is the median
recorded node count of that engine's moves in the PGN, rounded to the
nearest 10,000. The first 96 of those positions are also played at
1,000,000 nodes. A position is *won* when the side that led mates.

| # | Prediction | Probability |
|---|---|---|
| A2-P9 | (a) Every tree converts 24 in KQ-K, KR-K and KBB-K at both budgets | 75% |
| A2-P10 | (b) At `N`, bar 12 and bar 16 each win within ±4 positions of the switch off, and the mean plies over positions all three win are within ±5% of it | 70%; 60% |

Verdict rule, per bar against the switch off. *Harm* when (a) it reads
under 24 in a major-piece family that the switch off holds at 24, or
KBN-K more than 1 below the switch off's count at either budget; or (b)
at either budget the positions the two decide differently split against
it by at least 6 with a two-sided exact sign test under 0.05. *Benefit*
on (b) by the same test in its favour, or mean plies at least 5% shorter
over positions both win at both budgets. *No effect shown* otherwise.

### R6 — the candidate's binding reads (question 1)

On `CAND`'s release tree, `variant.py`'s reads: `bench 13`,
`test_endgames` (the mate fixtures and the wrong-bishop draws),
`test_search`, the floors at the five budgets, WAC at depth 6 and at
100,000 nodes.

| # | Prediction | Probability |
|---|---|---|
| A2-P11 | `CAND` reads `bench 13` 4,003,560, floors of 24 with KBN-K 118, and WAC 149 and 258: ticket 6's reads | 85% |

### R7 — the evasion picker's sort (question 4, deviation 4)

The donor sorts every evasion; the arm's picker at depth 0 sorts the
quiets that score above 0 and leaves the rest in generator order, and
once one evasion holds the other quiets are skipped. A throwaway variant
of `T6` sorts every quiet evasion in quiescence. Read on both: `bench
13`; on the bench positions at depth 13, stride 1, evasions searched per
node, first-move fail highs and the fail-high share of evasion nodes;
the floors at the five budgets; WAC at depth 6 and at 100,000 nodes.

| # | Prediction | Probability |
|---|---|---|
| A2-P12 | The full sort moves `bench 13` by under 3% and no floor or WAC count by more than 2 | 55% |

Verdict rule. If the full sort moves `bench 13` by under 3% and fails no
binding read, depth 0 is confirmed as immaterial. If it moves more, both
forms are priced (`Pricing 4`, depth 12, the first 40 positions of
`suite_v2`, `QsEvasion` alone): the full sort replaces depth 0 only when
its priced beat share is at least 2 points lower and it fails no binding
read; otherwise depth 0 stays. P4's registered read is B.4.5's and is
not this one.

### The decision rule (question 1), frozen

1. **Ticket 7 as built is not kept**, whatever R1 reads: the cap of 5 on
   the bar of 12 fails P7 on its own tree, and no read here re-opens the
   floor clause. A benefit for bar 12 in R4 or R5 is recorded for H.2 and
   changes nothing in B.4.4.
2. **The candidate is adopted**, `B4RfpSeekMate` 1 on its own bar of 16,
   when R6 passes every binding read (the major-piece floors at 24, KBN-K's
   sum at 112 or more, `test_endgames`, `test_search`, WAC at 100,000
   nodes at 239 or more), R1 shows `CAND` failing fewer than
   3 more major-piece playouts than `T6`, R5 shows no harm for bar 16,
   and R4 shows no harm for bar 16. It needs no benefit: it is the
   donor's form, and the donor kept it on non-regression.
3. **Otherwise the fallback**: `B4RfpSeekMate` default 0, the switch and
   its F lines kept, a departure row in §14 and a re-read on the network
   (H.2). No paired run in B.4.5: the rule acts in decided positions
   only, and a 2,000-game run at ±5 cannot read an effect of that size.
4. A2-P13: the candidate is adopted by this rule (70%).

### R8 — addendum, registered after R4's result and before it runs

R4 read no benefit for either bar (scored in the amendment). One
explanation is reach: Basilisk's reverse futility stops at depth 10
where the donor's stops at 18, so the cap removes five depths here
against thirteen there. R8 reads R4's two populations at 1,000,000
nodes on the patch's Tune build with `CoreRfpDepthCap` 18, under the
switch off and under bar 16.

| # | Prediction | Probability |
|---|---|---|
| A2-P14 | With the reach at 18, bar 16 finds at least 5% more mates than the switch off | 40% |

Verdict rule: benefit, harm and no effect as in R4. With a benefit,
§14's row for the cap names the reach as what it waits on, and B.6's
joint surface reads the two together. Without one it names the network
alone. Nothing in B.4.4 depends on this read.

### R9 — addendum, registered after R1's grid and before it runs

R1's rule for the candidate named the major-piece families. Its grid
also shows something the rule did not ask about: `CAND` leaves 23 KBN-K
playouts of 600 unconverted where `OFF`, `T6` and `T7` each leave 13,
and 21 of the 23 are at budgets of 60,000 nodes or fewer, where a KBN-K
search ends near root depth 16 (median depth 16 at 45,000 nodes and 17.5
at 60,000 on eight of the positions). `T7`'s KR-K miss is at 45,000
nodes, where a KR-K search ends near depth 12. That suggests a
hypothesis formed after exposure: the cap costs conversion at a budget
that ends near its bar. It cannot be scored on the grid that suggested
it, so R9 tests it on positions the grid did not hold.

100 new KBN-K positions, drawn by a fixed-seed generator on the
harness's own criteria (a legal position, White to move and not in
check, with a legal move). Each is played out by the validated cold
replay, 100 plies at most as the harness allows, at 30,000, 35,000,
40,000, 45,000, 50,000 and 60,000 nodes, on the patch's Tune build
under the switch off, bar 16 and bar 12: 600 playouts a setting. A
playout is unconverted when White has not mated within 100 plies.

| # | Prediction | Probability |
|---|---|---|
| A2-P16 | Bar 16 leaves at least 1.5 times as many playouts unconverted as the switch off | 55% |

Verdict rule. *Harm* for a bar when its unconverted count is at least
1.5 times the switch off's and a one-sided Fisher exact test on the two
counts of 600 is under 0.05. If R9 reads harm for bar 16, the decision
rule's fallback applies and the cap is left off. If it does not, the
grid's KBN-K difference is recorded as not replicated and the decision
rule is applied as it stands.

## Research amendment 2 — reverse futility's cap gets its own bar and stays off; what the KR-K miss measures (2026-11-06)

Written on Return 1, class `R2`. Nothing in the contract was implemented,
no default was moved and no floor was relaxed. The reads were registered
in *Reads A2* at `99ddb2c`, R8 at `1764dc3` and R9 at `6e460f9`, each
before it ran, and are scored here unchanged. Every read is a
node-budget or fixed-depth read: no game was played and no timed read
was run, so the Rarog session sharing the host could not move a result.
Temporary builds came from throwaway patches through `amend2/build.py`,
which restores the source byte-exact after each (§12).

Names: `OFF` is the OFF arm. `T6` is ticket 6's tree, the committed
source with `B4RfpSeekMate` 0. `T7` is ticket 7 as built, the cap of 5 on
Cluster 2's bar of 12. `CAND` is the candidate, the cap of 5 on its own
bar of 16. "Bar 12" and "bar 16" name the cap under each bar on one Tune
build; "the switch off" is no cap.

### 0. What this says, plainly

1. **Return 1's miss is a failed rule and not evidence about the cap.**
   On a 25-budget grid `T7` misses one KR-K playout of 600, the known
   one. The OFF arm also misses one of 600, at 35,000 nodes on another
   position. `T6` misses none. The floor read at five budgets has a
   false-alarm rate, and ticket 7 drew it (§5). The stop rule stands as
   written, so ticket 7 as built still does not land.
2. **The return is right that the contract did not carry the bar.** The
   donor pairs the cap with root depth 16. Cluster 2 lowered its bar to
   12 to bound a singular-extension tree, and that reason does not reach
   the cap, which grows a tree. The cap gets its own bar, an F line at
   the donor's 16 (§3).
3. **The implementer's worry is refuted: the cap is not inert at 16.** In
   3,000 games at `3+0.03` the OFF arm is at root depth 16 or more with
   a decided score on 7.0% of its moves and in 53% of its games (§4).
4. **The cap does not do here what the donor built it for.** On the
   donor's own measure, mates found at a node budget, it finds 505 of
   1,092 against 512 without it, and no more with reverse futility's
   reach raised to the donor's 18 (297 against 304). No read reaches a
   registered bar either way; two lean against the cap and one for it
   (§3).
5. **So the cap stays off in the arm.** `B4RfpSeekMate` defaults to 0 and
   the arm's tree is ticket 6's, `bench 13` 4,003,560. The switch, its two
   F lines, the counter and the tests stay, on the donor's pairing, for
   the re-read on the network. My registered rule would have adopted the
   cap; §3 says why the maintainer's standing rule outranks it. Keeping
   it on costs one default and is the maintainer's to choose (§10).
6. **Four of the five deviations are confirmed.** Deviation 8 is
   amended: P5's share needs one counter. A full sort of the evasions is
   a real effect that the registered rule keeps out of the arm (§6).
7. **No prediction is re-registered.** P3, P4's table-move clause and P6
   stand as frozen and will score as misses; P4's clause was a modelling
   error of mine (§7).

### 1. Instruments and their checks

- **Identity.** `T6` reads `bench 13` 4,003,560, `T7` 3,166,820 and `OFF`
  6,090,091. `CAND` reads 4,003,560 on Release, Diag and Tune.
- **A registered check that could not be read, and its replacement.**
  The registration checked the patch's Tune build through `bench 13` with
  an option set. `bench` ignores UCI options on every Tune build: the
  committed one reads 3,166,820 with `B4RfpSeekMate` 0 set. The check was
  replaced, before any read that needs it, by a stricter one through
  `go`. Over the 40 bench positions at depth 13 and two KR-K positions at
  depth 18, the patch's Tune build with `B4RfpSeekMateDepth` 12 equals
  the committed Tune build search for search, and with `B4RfpSeekMate` 0
  it equals the committed build with that switch at 0 (0 of 42 differ
  each; `identity.txt`). The patch does nothing but move the bar.
- **The cap is live on `CAND`.** On `8/8/8/4k3/8/8/8/4K2R w` at depth 18
  the Diag build counts 511 refusals, and the Tune build searches 347,065
  nodes under bar 16, 261,485 with the switch off and 413,183 under bar
  12.
- **The cold replay.** One process per playout with `ucinewgame` before
  every move and Hash 8 reproduces four cells of `return1/sweep.txt`: 27
  plies, the rule-50 draw, 35 and 37 (`checks.txt`).
- **Two clerical slips.** R3's "every 225th move" gives 835 positions,
  not "about 600"; the rule was applied as written. R4's first table
  keyed positions without the game, so 7 of population B's 450 collided;
  the pooled counts were right, and `r4_report.py` prints the table used
  here.
- **A driver fault, caught.** The driver for the later reads was named
  `queue.py`, which shadows Python's standard module, so each script that
  imported `concurrent.futures` ran the driver again. The copies were
  stopped and R5 (b), R8 and R9 were run again from clean processes. The
  reads that had finished are deterministic and were reproduced by the
  copies (`queue_runaway.out`); the driver is now `run_reads.py`.

### 2. The predictions, scored

| # | Prediction (frozen) | Result | Verdict |
|---|---|---|---|
| A2-P1 | `OFF` converts all 600 in each major-piece family (70%) | KR-K 599 of 600 | miss |
| A2-P2 | `T6` converts all 600 KR-K playouts (55%) | 600 | hit |
| A2-P3 | `T7` fails 1 to 3 KR-K playouts, the known one included (65%) | 1 | hit |
| A2-P4 | `CAND` equals `T6` playout for playout in KR-K (70%) | differs at 6 of 25 budgets | miss |
| A2-P5 | Bar 12: 12 to 30% of moves and at least 60% of games (70%) | 13.37%; 52.8% | half |
| A2-P6 | Bar 16: 4 to 15% of moves and at least 35% of games (65%) | 6.97%; 52.7% | hit |
| A2-P7 | Bar 12's refusals under 1% of cuts (75%); bar 16's under a third of bar 12's (65%) | 0.195%; 66% of bar 12's | hit; miss |
| A2-P8 | Mates found: bar 16 within ±8 of the switch off (60%); bar 12 within ±12 (55%) | −7; −14 | hit; miss |
| A2-P9 | Every tree converts 24 in the major-piece families at 250,000 and 1,000,000 nodes (75%) | 24 everywhere, KBN-K too | hit |
| A2-P10 | Won positions at 120,000 nodes: each bar within ±4 wins of the switch off (70%); mean plies within ±5% (60%) | 235, 234 and 235 of 240; 0.995× and 1.002× (at 1,000,000 nodes: 94 each of 96; 1.008× and 0.964×) | hit; hit |
| A2-P11 | `CAND` reads ticket 6's bench, floors and WAC (85%) | bench and WAC yes; KBN-K 115, not 118 | half |
| A2-P12 | A full evasion sort moves `bench 13` by under 3% (55%) | −25.2% | miss |
| A2-P13 | The candidate is adopted by the registered rule (70%) | every clause of the rule passes | hit; the decision differs (§3) |
| A2-P14 | With the reach at 18, bar 16 finds at least 5% more mates (40%) | −2.3% | miss |
| A2-P16 | New KBN-K positions: bar 16 leaves at least 1.5 times as many unconverted (55%) | 22 against 15, 1.47 times | miss |

There is no A2-P15.

Calibration. Three misses (P4, P11's KBN-K clause, and the half of P7)
share one wrong belief: that a bar of 16 is out of the floors' reach. I
took it from KR-K, where a search at 45,000 nodes ends at depth 9 to 12,
and from the return's sentence to that effect. KBB-K and KBN-K searches
pass depth 16 at 45,000 nodes (medians 17 and 16; `depths.txt`), so
`CAND` is not ticket 6's tree in those families. P1 missed because I took
five clean budgets for a clean tree, which is the error §5 measures. P14
missed because I blamed the reach for what belongs to the evaluation.

### 3. Question 1: what answers Return 1

**Decision.** The cap reads its own bar, `B4RfpSeekMateDepth` 16, and is
off by default, `B4RfpSeekMate` 0. The arm's tree is ticket 6's.

**The form: option 4, the cap's own bar.** It is what the contract meant.
§3's row adopted "the cap of 5 under Cluster 2's `seek_mate`", reading
`seek_mate` as the donor's condition, when the B.3 packet's amendment 3
had moved its bar to 12 for a reason of its own: a singular-extension
chain that multiplied bench position 11's tree. Standing singular
extensions down earlier shrinks a tree. Refusing reverse-futility cuts
earlier grows one. One bar cannot serve both, so each half reads its own,
and any later read of the cap tests the donor's pairing.

**The default: evidence.**

- *Binding reads on `CAND` (R6): all pass.* `bench 13` 4,003,560; the
  major-piece floors 24 at each of the five budgets; KBN-K 23 / 22 / 23 /
  24 / 23, sum 115 against the floor of 112 (`T6`: 118);
  `test_endgames` 191 of 191; `test_search` 719 of 719; WAC 149 at depth
  6 and 258 at 100,000 nodes.
- *The grid (R1).* `CAND` fails no major-piece playout of 1,800, as `T6`.
  In KBN-K it leaves 23 of 600 unconverted where `OFF`, `T6` and `T7`
  each leave 13:

  | Tree | KBN-K unconverted at 60,000 nodes or fewer (360) | Above (240) |
  |---|---:|---:|
  | `OFF` | 11 | 2 |
  | `T6` | 12 | 1 |
  | `T7` | 10 | 3 |
  | `CAND` | **21** | 2 |

  The rule registered for R1 named the major-piece families, so this is
  an observation made after exposure. R9 was registered to test it.
- *New KBN-K positions (R9).* 100 positions at six budgets from 30,000 to
  60,000 nodes, 600 playouts a setting: the switch off leaves 15
  unconverted, bar 12 leaves 12 and bar 16 leaves **22**. That is 1.47
  times the switch off, with a one-sided Fisher test of 0.16, against a
  registered bar of 1.5 times and 0.05. *Not harm* by the rule. The
  direction replicates and the size does not reach the bar.
- *Conversion at budgets that reach the bar (R5): no effect shown.* (a)
  At 250,000 and 1,000,000 nodes every tree converts 24 in all four
  families. (b) From 240 won game positions at 120,000 nodes the leading
  side wins 235 with the switch off, 234 under bar 12 and 235 under bar
  16, in 59.8, 59.5 and 59.9 plies. From the first 96 of them at
  1,000,000 nodes it wins 94 under each setting; over the 93 that all
  three win, bar 16 takes 60.6 plies against 62.9 (0.964 times) and bar
  12 takes 63.3. The registered bar for a benefit is 5% at both
  budgets.
- *Mates found (R4, R8)*, in §4: no effect shown, leaning against.

**The default: two rules, and which governs.**

- *The registered decision rule adopts the candidate.* Every clause
  passes: R6, R1's major-piece clause, R5, R4 and R9. Its clause 2 said
  the candidate "needs no benefit: it is the donor's form".
- *That clause was wrong when I wrote it.* The maintainer's standing
  decision of 2026-11-04 (PLAN H.2, "Basilisk's forms stay, since they
  work") is that a Basilisk form that works is kept and is not exchanged
  for the donor's without a reason. Ticket 6's form works: no
  major-piece playout of 1,800 fails on the grid, and every binding read
  holds. The reason PLAN gave for the cap was the mate drives, and R4
  and R8 measured it: there is none. A standing decision of the
  maintainer outranks a clause I registered against it, so it governs.
  The registered verdict is recorded beside it, unchanged.
- *The reads lean, and none reaches its bar.* For the cap: won game
  positions at 1,000,000 nodes convert in 3.6% fewer plies. Against it:
  mates found at 1,000,000 nodes, 293 against 302 with the discordant
  positions 7 to 16 (sign test 0.09); and KBN-K at 60,000 nodes or
  fewer, 21 unconverted against 12 on the grid and 22 against 15 on new
  positions. That is no reason to leave a form that works.

**The other options.**

- *Option 1 as written, the switch at 0 and a paired run in B.4.5:* the
  default is taken, the run is not. The cap acts in decided positions
  only, and a 2,000-game run at ±5 cannot read an effect of that size.
  The switch is therefore a departure (§14), not a categorical.
- *Option 2, another seed for the cap: declined.* Caps of 4, 6 and 8
  pass five budgets as the cap of 5 fails one, and §5 shows that any
  tree does that at a one-in-600 rate. A seed picked by the cell it
  passes is the selection the B.3 packet's amendment 6 §4.4 warned
  against.
- *Option 3, re-reading the floor clause: declined, and not needed.* §5.

**Semantics, seed, default, re-read.** With `B4RfpSeekMate` 1 a
reverse-futility cut is refused when the iteration's root depth is at
least `B4RfpSeekMateDepth`, the previous iteration's root score is at
least `B3SeekMateScore` either way, and the node's depth is above
`B4RfpSeekMateCap`. `B4RfpSeekMateDepth` is an F line, seed 16 (B.0 §7.2
rule 1), range 8 to 64. `B4RfpSeekMateCap` stays 5. `B4RfpSeekMate`
defaults to **0**. There is no fallback to register: the default is
ticket 6's tree, which has passed every read. The switch is re-read on
the network (H.2), by R4's recipe first: mates found at a node budget,
before any game.

### 4. Question 2: is the cap worth having

**What it serves (evidence, the donor's record).** The donor's commit
that introduced `seekMate` (`598ae2c4` there) justifies it by mates found
at a node budget on a mate suite, and passed it on non-regression bounds
at two time controls. It claims no Elo. The property: in a decided
position a reverse-futility cut returns the evaluator's word that a
subtree is good enough, so the search never proves the mate inside it.
Refusing those cuts at deep nodes makes a deep search prove mates
sooner.

**How often the bar is reached in games (R2, evidence).** The OFF arm's
188,020 moves in the B.4.2 gate's 3,000 games at `3+0.03`, no
adjudication, median 116,021 nodes a move:

| | Moves | Games with one | Nodes |
|---|---:|---:|---:|
| Decided: a score of 564 or more either way, or a mate score | 21.14% | — | — |
| Live at bar 12 (decided, depth 12 or more) | 13.37% | 52.8% | 10.99% |
| Live at bar 16 (decided, depth 16 or more) | 6.97% | 52.7% | 5.36% |

Both bars are *live in games* by the registered rule. Bar 16 is reached
in as many games as bar 12, on half as many moves. These are the OFF
arm's depths; the arm's smaller tree reaches a bar more often.

**How often it fires when live (R3, evidence).** 835 sampled moves,
searched cold at each move's recorded node count on the arm's Diag
builds:

| | Searches with a refusal | Refusals against cuts, all searches | In searches live by the PGN's record |
|---|---:|---:|---:|
| Bar 12 | 54 of 835 (6.5%) | 37,780 against 19,326,088 (0.195%) | 2.74% |
| Bar 16 | 23 of 835 (2.8%) | 24,783 against 19,321,845 (0.128%) | 3.80% |

When the bar is reached the cap refuses 3 to 4% of reverse futility's
cuts.

**Mates found at a node budget (R4, evidence).** Population A is the 96
floor positions. Population B is 450 positions 10, 16 and 24 plies
before the mate in 150 games.

| Population, budget | Switch off | Bar 12 | Bar 16 | Found only with / only without bar 16 |
|---|---:|---:|---:|---|
| A, 250,000 | 52 | 50 | 52 | 2 / 2 |
| A, 1,000,000 | 71 | 66 | 66 | 2 / 7 |
| B, 250,000 | 158 | 157 | 160 | 6 / 4 |
| B, 1,000,000 | 231 | 225 | 227 | 5 / 9 |
| Pooled, of 1,092 | **512** | **498** (−2.7%) | **505** (−1.4%) | 15 / 22 |

*No effect shown* for either bar by the registered rule (5% either way).
The cap costs depth at a fixed budget: mean depth falls by 0.1 to 0.9
plies, and the KR-K position of §1 takes 33% more nodes to depth 18.

**Whether the reach explains it (R8, evidence).** Basilisk's reverse
futility stops at depth 10 where the donor's stops at 18, so the cap
removes five depths here against thirteen there. With `CoreRfpDepthCap`
18, at 1,000,000 nodes, bar 16 finds 297 mates against 304 (−2.3%; 11
found only with it, 18 only without). *No effect shown*, and the reach
does not explain it. By R8's rule the departure waits on the network
alone.

**Answer.** The cap is worth nothing measurable on this evaluation. It
is live in half the games, it refuses a few per cent of reverse
futility's cuts when it is, and it proves no more mates for them at
either reach. Why it works in the donor and not here is not measured.
The hypothesis is the evaluation: a won position's hand-written
evaluation rises steadily toward the mate, so a cut on it hides little,
where a network's value in a won position does not order the mating
lines. That is the reason to read it again on the network and not
before.

### 5. Question 3: how noisy the floor is, and whether §11's stop rule stands

**Evidence (R1).** The four families at 25 budgets from 25,000 to 135,000
nodes, 600 playouts a family a tree (`r1/floors.json`; `r1_report.txt`):

| Tree | KQ-K | KR-K | KBB-K | KBN-K, of 600 | KBN-K at the five registered budgets |
|---|---|---|---|---:|---:|
| `OFF` | 600 | **599**: `R7/8/8/4k3/1K6/8/8/8 w` at 35,000 | 600 | 587 | 115 |
| `T6` | 600 | 600 | 600 | 587 | 118 |
| `T7` | 600 | **599**: `4K3/8/8/8/8/5k2/8/1R6 w` at 45,000 | 600 | 587 | 114 |
| `CAND` | 600 | 600 | 600 | 577 | 115 |

- **The base rate.** A healthy tree, the OFF arm that plays on `master`,
  fails one KR-K playout in 600 and one budget in 25. By the registered
  estimate a five-budget read then shows a miss 18.5% of the time. One
  event carries a wide interval, so read it as "of the order of one read
  in five to ten", not as a rate.
- **Whose miss.** `T7` fails 1 major-piece playout of 1,800 and `T6`
  fails 0: by the registered rule the two trees are not told apart. The
  cap on the bar of 12 put the bad budget on a registered one; the grid
  does not show it converting worse.
- **KBN-K.** `OFF`, `T6` and `T7` each convert 587 of 600. Their
  five-budget sums of 115, 118 and 114 are three draws from one rate,
  and the return's "114 against 118" is not a difference. The floor of
  112 sits about 5 below the expected sum of 117.4.

**The stop rule (decision).** It stands as written. This is not the
relaxation the B.3 packet's amendment 6 §4.3 declined, and it is not
needed: ticket 7b's tree is ticket 6's, which fails nothing on the grid.
What changes is how a miss is read. A single 23 in a major-piece family
at one budget still returns the leaf, and the return's first read is
this grid on the failing tree and on the previous ticket's, before any
mechanism is blamed.

**For the maintainer, for later clusters only.** The five-budget floor at
24 of 24 false-alarms on a healthy tree at a rate that will return a
leaf every few clusters. A floor read over the 25-budget grid with a
bound on unconverted playouts (`OFF` and `T7` read 1 of 600, `T6` and
`CAND` 0) would tell a tree that converts worse from one whose bad
budget moved. It costs about ten minutes of the host a tree. It changes
a registered instrument, so it is the maintainer's, and nothing in B.4
uses it.

### 6. Question 4: the deviations, ruled

**4, the evasion picker's depth is 0: confirmed by the registered
rule.** The donor sorts every evasion. The arm's picker at depth 0 sorts
the quiets that score above 0 and leaves the rest in generator order,
and once one evasion holds the remaining quiets are skipped, so which
quiet comes first can decide the node. R7 read a throwaway variant of
`T6` that sorts every quiet evasion:

| | `T6` (depth 0) | Full sort |
|---|---:|---:|
| `bench 13` | 4,003,560 | 2,996,564 (−25.2%) |
| Evasions searched per node, bench positions at depth 13 | 0.957 | 0.963 |
| First-move fail highs; fail highs per evasion node | 98.7%; 40.2% | 98.5%; 40.2% |
| `QsEvasion` priced beat share (`Pricing 4`, depth 12, 40 positions) | 7.99% [7.40, 8.64] | 6.06% [5.54, 6.63] |
| Major-piece floors; KBN-K's sum | 24; 118 | 24; 114 |
| `test_endgames`; WAC at depth 6 and 100,000 nodes | 191 of 191; 149 and 258 | **189 of 191**; 154 and 253 |

The bench moved by more than 3%, so the rule priced both forms. The full
sort is 1.93 points lower where the rule asked for 2, and it fails a
binding read: the wrong-bishop draw after trained tables no longer
reads as a draw.
Depth 0 stays on both counts. The priced intervals do not overlap, so
the effect is real: a full sort hides a quarter fewer beating evasions.
It is recorded in §11 and §14 and is not added to the arm.

**5, `skip_quiets()` as soon as the best stops being a loss: confirmed.**
The best never falls, so "not a loss" once is "not a loss" for the rest
of the node, and ending quiet delivery selects the moves §10's wording
selects. The table move is delivered before the test can hold.

**6, the stalemate takes no blend and stores on either side of beta:
confirmed.** §10's invariant governs and its "Both" paragraph was loose.
The donor sets the stalemate's 0 and then blends any value above beta,
so with beta below 0 it returns a stalemate at 0.55 of beta
(`search.cpp:1863-1872` at `edb0d9db`). A stalemate is a terminal fact
and its value is exact. An exact 0 is a true lower bound when it is at
or above beta and a true upper bound otherwise. §14 gains the row.

**7, pricing's shape: confirmed.** `QsSee` holding both SEE rules matches
`qs_see_pruned`, and no registered read needs them apart. The queen
captures of amendment 1 §4 stay readable: a queen capture never fails
the plain margin of −28, so every queen cell of `QsSee` is the
alpha-relative rule. `QsEvasion` carrying the class costs nothing.
Pricing the skipped quiets only at a node that ended without a cutoff
differs from §10's wording at one kind of node, a cutoff by a capture
the picker had set aside after the quiets, which is rare. P4's share is
read pooled over the rule's two halves, as frozen.

**8, `qs_recapture_exempt`'s denominator: amended.** P5 reads the
exempted recaptures against "the moves the gain rules test". The
captures the loop delivered include checks, promotions and the moves of
nodes where the gain rules stand down, so the share on that denominator
is not P5's. One counter is added, `qs_gain_tested`: the captures that
enter the gain rules' block. P5's share is `qs_recapture_exempt /
(qs_gain_tested + qs_recapture_exempt)`. This is a clerical statement of
the denominator P5 always named, made before its registered read.

### 7. Question 5: the predictions exposed during implementation

Nothing is re-registered. Each was seen on a read that is not its
registered one, and none carries a decision rule that the exposure
touches.

- **P3 stands.** Its share clause (30 to 40%) reads 40.9% on ticket 6's
  tree. Its bench clause (0.6 to 1.3×) reads 0.66× on the amended tree,
  4,003,560 against 6,090,091; the 0.52× belonged to ticket 7 as built
  and goes with it. P3's falsifier is a share above 45% and is not near.
  It is scored in BAS-S50 on its registered read.
- **P4 stands, and its table-move clause will miss for a reason that is
  mine.** §5.5 expected the evasion store to raise the share of evasion
  nodes holding a legal table move from 4.5% to 15% or more. The model
  left out two things. A stored fail high ends the node's next visit at
  the table cutoff, before the evasion loop is reached and before the
  counter can count it. And the arm's store writes no move after a fail
  low, in either search. So the figure never fitted that store; it would
  have needed the other form. This is a substantive error in the causal
  model, recorded now and scored as a miss in BAS-S50. Nothing decides
  on the clause: P4's falsifier is the priced beat share. The evasion
  reads that carry the order's verdict hit (0.88 searched per node
  against 1.45 or fewer; 98.3% first-move fail highs against 78% or
  more).
- **P6 stands.** 257 to 258 against [239, 255] is a miss on the high
  side. The binding rule is the floor of 239 and is unaffected.
- **P7.** Its falsifier fired once, at ticket 7 as built, and the
  attribution it names found no owner: each of the three switches
  restores 24, as in the B.3 packet's amendment 6. §5 is its reading. P7
  is read again on the final tree at the qualification.
- **Exposed by this amendment's R7**, on `T6` at depth 12 over 40
  positions, which is not B.4.5's read: `QsEvasion` 7.99% (P4: 8% or
  less), `QsFutility` 2.04% (P5 as amendment 1 re-registered it: 1.5% or
  less, falsifier above 2.5%), `QsCount` 2.02% (6% or less). They stand.
- **P1, P2, P8 to P11** do not move: ticket 7b's tree is ticket 6's.

### 8. Question 6: what changes for B.4.5 to B.4.7

- **§5.8, the surface rule: unchanged.** The cap's two lines are F lines
  and stay off the surface.
- **§11's ablation order: unchanged.** The cap is off and was never in
  it.
- **BAS-S50 gains two recorded reads and no rule:**
  1. `qs_evasion_tt_move` on both settings of item 2, beside
     `tt_move_absent` and `tt_move_d0`. It measures §7's account of P4.
  2. P4's `QsEvasion` share by node type as well as pooled. R7 read 21.9%
     [16.5, 28.4] at PV quiescence nodes against 7.6% elsewhere, on 183
     probes.
- **BAS-S50 does not read `B4RfpSeekMate`.** It has no paired run.
- **BAS-S51 and BAS-S52: unchanged.**

### 9. Question 7, and the clauses replaced

Tickets 8 to 10 proceed on ticket 7b with three changes: the identity
reads of tickets 8 and 9 take **4,003,560**; ticket 0's counter list
gains one counter; ticket 10 gains one check and two recorded rows. The
WAC floor stays 136, the count of 149 less 13. Ticket 10 registers that
on the arm unless the final count differs, when the file's rule applies.
The qualification's WAC bound at 100,000 nodes stays 239.

**§7, the cap's row, replaced by two.**

| Constant | Donor | Converted | Oracle | Basilisk today | Rule | Seed | Line |
|---|---|---:|---|---|---|---|---|
| Reverse futility's depth cap in a mate drive | `depth < 6` | — | none | none | 1 | **5**, range 1 to 19 | F `B4RfpSeekMateCap` |
| The root depth from which that cap binds | `rootDepth >= 16` | — | none | none; Cluster 2's 12 is the singular half's | 1 | **16**, range 8 to 64 | F `B4RfpSeekMateDepth` |

**§9, the attribution table loses `B4RfpSeekMate`.** It becomes a
departure switch, in a table of its own:

| Option | Values | Default | Read |
|---|---|---|---|
| `B4RfpSeekMate` | 0 no cap, Basilisk's form; 1 a reverse-futility cut is refused at a node deeper than `B4RfpSeekMateCap` when the iteration's root depth is at least `B4RfpSeekMateDepth` and the previous iteration's root score is at least `B3SeekMateScore` either way | **0** | no paired run; §14's row; H.2 on the network |

**§10, ticket 0, the counter list gains** `qs_gain_tested`: the captures
that enter the gain rules' block (not a check, not a promotion, not an
exempt recapture, at a node where the gain rules run). It is appended
after `rfp_seekmate_capped`, with its unit in `run_suite.py`, and
`qs_recapture_exempt`'s comment names the new denominator. It lands as
its own commit with both arms' fingerprints unchanged.

**§10, ticket 7, replaced by ticket 7b.**

7b. **Reverse futility's cap, on its own bar and off.** With
    `B4RfpSeekMate` 1 a reverse-futility cut is refused when the
    iteration's root depth is at least `B4RfpSeekMateDepth` (F, seed 16,
    range 8 to 64), the previous iteration's root score is at least
    `B3SeekMateScore` either way, and the node's depth is above
    `B4RfpSeekMateCap`. It is `b3::seek_mate` called with the cap's own
    bar, so there is no new arithmetic. `B4RfpSeekMate` defaults to 0.
    `B3SeekMateDepth` is read by singular extensions alone and is not
    touched. The counter `rfp_seekmate_capped` and
    `b4::rfp_past_seek_mate_cap` are unchanged. It lands as a new commit
    on top of `b003b3c`; ticket 7's commit is not rewritten.
    - Fingerprints: OFF 6,090,091. ON **4,003,560** on Release, Diag and
      Tune, ticket 6's count. Any other count returns the leaf.
    - Tests, in `test_b4_qsearch`:
      - the arm's default is 0;
      - the gate with forced root depths and scores reads the cap's bar:
        refused at root depth 16 and 24 with a score of ±564 and with a
        mate score, at node depth 6; not at root depth 15, not at 12
        (Cluster 2's bar), not with a score of 563, not at node depth 5;
      - the two bars are independent: with the switch at 1 and
        `b3_seek_mate_depth` 64 the cap still fires on the live
        position, and with `b4_rfp_seek_mate_depth` 64 it never does and
        the tree is the default's;
      - live: `8/8/8/4k3/8/8/8/4K2R w` at depth 18, which was 15. On the
        arm the node count differs between the switch at 1 and at 0, and
        `rfp_seekmate_capped` is above 0 at 1 and 0 at the default where
        counters are compiled; at depth 15 the two counts are equal. On
        the OFF arm they are equal at both depths. This amendment read
        347,065 against 261,485 nodes and 511 refusals over UCI at Hash
        64.
    - The tune surface: `B4RfpSeekMateDepth` is advertised by tune
      builds on the arm alone and listed as fixed in
      `generate_spsa_surface.py`'s B4 table.
    - Reads at the ticket's end, on the default: the tree is ticket 6's,
      so the counters equal ticket 6's row for row, the floors read 24
      with KBN-K 23 / 23 / 24 / 24 / 24, and WAC reads 149 and 258. A
      difference returns the leaf.
    - Recorded, with the switch at 1 on a variant build (`variant.py`):
      `bench 13`, the floors at five budgets, `test_endgames`, WAC. This
      amendment read 4,003,560, 24 with KBN-K 23 / 22 / 23 / 24 / 23,
      191 of 191, and 149 and 258 on its patch. They are §14's numbers
      on the committed code and bind nothing.

**§10, tickets 8 and 9.** "The fingerprint" each must hold at its
defaults is ticket 7b's, 4,003,560.

**§10, ticket 10, gains.**
- A node check in the builds that compile them: where quiescence returns
  a mate because no evasion was searched, the position has no legal
  move. The donor asserts the same. Evasions searched per node read
  below 1 (0.88 to 0.96), so at least one evasion node in twenty is a
  checkmate. That is plausible and has not been checked.
- Two recorded rows in the two-arm table: the Return 1 position at
  45,000 nodes on the final tree (`T6` mates in 37 plies), and ticket
  7b's variant row.

**§10, the cheap qualification.** Unchanged, read on ticket 10's tree.

**§10, "Producer → stored state → consumer".** The driver's
`root_depth_` and `root_score_` reach ticket 7b through `b3::seek_mate`
with the cap's own bar, when the switch is 1.

**§14 gains two rows and one sentence.**

| Place | Basilisk's form (kept) | The donor's form | Why kept | Waits on |
|---|---|---|---|---|
| Reverse futility in a mate drive | no cap: `CoreRfpDepthCap` at every root depth (`B4RfpSeekMate` 0) | depth under 6 from root depth 16 with a decided root score; the switch's 1 reads its own bar of 16, not Cluster 2's 12 | Basilisk's form works (no major-piece playout of 1,800 fails on the grid). The donor's shows no gain on the donor's own measure (505 mates of 1,092 against 512; 297 against 304 at the donor's reach) and leans against on KBN-K at 60,000 nodes or fewer (22 unconverted of 600 against 15) | the network: H.2, mates found at a node budget first |
| The stalemate's value | exactly 0, unblended | 0 blended toward a beta below it | a terminal draw is exact | nothing |

The "Evasion order" row gains: quiets scoring 0 or less stay in
generator order at depth 0, where the donor sorts every evasion. A full
sort reads 6.06% against 7.99% on the evasion rule's priced beat share,
a `bench 13` 25% smaller, and a wrong-bishop draw that no longer reads
as one (`t6_full_sort.patch`). It waits on B.4.5's P4, then H.2.

### 10. Decisions that are the maintainer's

1. **The cap off or on in the arm.** Taken here as off, on the standing
   decision that a working Basilisk form is kept. The case for on: it is
   the donor's form, the donor kept it on non-regression, my registered
   rule adopts it, and no read reaches its bar for harm. On costs one
   default (`B4RfpSeekMate` 1) and moves the arm's KBN-K floor sum from
   118 to 115; the fingerprint stays 4,003,560. My position: off. Five
   reads could have shown a gain and none reached its bar, the leans
   split two against and one for, and off gives up nothing, because the
   switch stays.
2. **A denser floor for later clusters** (§5). Not decided here. My
   position: register it at B.5's research, with the grid's bound taken
   from this read.
3. **The evasion sort** (§6). Taken here as an H.2 candidate. The
   alternative is a sixth categorical in B.4.5. My position: H.2. It
   fails a wrong-bishop draw as it stands, and its priced gain is a
   quarter of an 8% share on a tenth of quiescence's nodes.

### 11. Findings outside the contract

- **`bench` ignores UCI options on a Tune build.** `setoption` followed
  by `bench 13` reads the compiled defaults with no warning. The
  implementer's variant builds avoided it; a reader who sets an option
  and benches gets the default tree's count. One line in PROCESS or a
  refusal in `bench` would close it.
- **`bench 13` is path-dependent.** `bench` clears the table and the
  histories once a run, not once a position (`src/bench.cpp`), so a
  change that fires on one position moves every later one. Ticket 7 as
  built moved the count from 4,003,560 to 3,166,820 on 426 refusals in
  502,535 cuts, and the same 40 searches run cold through `go` differ by
  0.2% in total (3,247,003 against 3,252,375). A ticket's bench ratio is
  not its tree's size, and P3's bench clause is read with that in mind.
- **A full sort of quiescence's evasions** (§6): a candidate for H.2
  with its patch and both readings.
- **The evasion rule at PV quiescence nodes** hides more than elsewhere
  (21.9% against 7.6%, §8). B.4.5 reads it on its registered run.

### 12. Artifacts (`tools/results/b44-20261106/amend2/`, ignored storage)

| Artifact | Identity |
|---|---|
| Builder | `build.py`: applies each patch, builds, restores the source byte-exact, asserts `git status` clean; `build.out` lists every binary's SHA-256 |
| The candidate | `own_bar.patch`, SHA-256 `EE83CB6ED0D2797910AEE944727ECAEF9EFEC3CFB8E7A81566FEC459FE6BB2D7`, on `632bbb3`; trees `build/b4-a2-cand`, `-cand-tune`, `-cand-diag`; `bin/cand_basilisk.exe` SHA-256 `DEC2EBEB8CFB57F75B2DA58C58AA0949473A9DF318B7B05F6052A0574FA7E467` |
| Ticket 6's tree | `t6_sw0.patch`, SHA-256 `7DAC69C95AC15BD8CA3DB21F4E1887149DA781E6A0A2B889773A35149D5BF14D`; `build/b4-a2-t6`, `-t6-diag` |
| The full sort | `t6_full_sort.patch`, SHA-256 `14820EB227B8BC5A77DEEB34A1A9D7C2D44066AFEC5330905CF9E2CC69551F71`; `build/b4-a2-sort`, `-sort-diag` |
| Playout listing | `bin/{off-list,t7-list,t6,cand,sort}_test_endgames.exe`: `test_endgames` with a print of every playout |
| Checks | `checks.py`, `checks.txt`; `identity.py`, `identity.txt`; `depths.py`, `depths.txt` |
| R1, R5 (a), R7's floors | `floors.py`; `r1/`, `r5a/` and `r7/floors.json` with their logs; `r1_report.py`, `r1_report.txt` |
| R2 | `r2_exposure.py`, `r2_exposure.txt`; the PGN is `tools/results/b42-pins-gate/games.pgn`, and `games.json` is its parsed form |
| R3 | `r3_firing.py`, `r3_firing.json`, `r3_firing.txt` |
| R4, R8 | `r4_mates.py`, `r4_mates.json`, `r4_report.py`, `r4_mates.txt`; `r8_reach.py`, `r8_reach.json`, `r8_reach.txt` |
| R5 (b) | `r5b_playouts.py`; `r5b_120000.json`, `r5b_1000000.json` and their `.txt` |
| R6, R7 | `run_reads.py`, `reads.out` and the `*_test_*.log`, `*_wac_*.log` beside it; `r7/price_t6`, `r7/price_sort`, `r7/counters_*.json` |
| R9 | `r9_kbnk.py`, `r9_kbnk.json`, `r9_kbnk.txt` |

## Implementation record — tickets 7b to 10 (2026-11-06)

Continues the record of tickets 0 to 7 (above, at Return 1), on *Research
amendment 2*. Branch `b4-quiescence`, from `de91e38`. Builds: the release
preset, PEXT portable, Clang 23.1.5, non-PGO `all` trees; the sanitizer trees
are the `debug` preset, ASan and UBSan. Per-ticket reads used
`build/b4-{on,off}`, `-diag`, `-tune`. The final table used a detached
worktree at `cedc148` with every flavour built from nothing
(`build/b4-final`). Evidence: `tools/results/b44-20261106/{t7b,t8,t9,t10}/`
(ignored storage); `t10/final_table.py` holds the final table's recipe,
`t10/table.txt` its output and `t10/hashes.json` every binary's SHA-256.
No game, SPSA or NPS read ran.

### Per ticket

| Ticket | Commits | OFF `bench 13` | ON `bench 13` | WAC depth 6 / 100k | KQ-K, KR-K, KBB-K | KBN-K by budget, sum |
|---|---|---:|---:|---|---|---|
| 7b | `9cf84ae` | 6,090,091 | 4,003,560 | 149 / 258 | 24 at every budget | 23/23/24/24/24, 118 |
| The counter | `f895750`, `2b75777` | 6,090,091 | 4,003,560 | not read: no behaviour moved | | |
| 8 | `f771999` | 6,090,091 | 4,003,560 | not read: §10 reads them when the fingerprint changes | | |
| 9 | `417db0c` | 6,090,091 | 4,003,560 | not read, as ticket 8 | | |
| 10 | `2e84a10`, `cedc148` | 6,090,091 | 4,003,560 | 149 / 258 | 24 at every budget | 23/23/24/24/24, 118 |

Each row's OFF and ON counts held on Release, Diag and Tune.

- **Ticket 7b's tree is ticket 6's.** The counters on the bench positions at
  depth 13, stride 1: 176 rows compared, 0 differ, and 0 of 40 positions
  differ (`same_rows.py t6 t7b`). No row moved by more than 20%.
- **Tickets 8 and 9 moved nothing.** Against the ticket before, 0 rows and 0
  of 40 positions differ; ticket 8's read leaves out `qs_gain_tested`, which
  ticket 7b's file does not hold.
- **CTest.** Release 27/27 on both arms at tickets 7b, 8 and 9 and on the
  final tree. At each of those the arm's Diag build ran `test_stack_unwind`,
  `test_b4_qsearch`, `test_b3_proof` and `test_search` with node checks live,
  and passed. At the counter's commits the six builds, their fingerprints and
  `test_b4_qsearch` were read, and `test_run_suite.py` 8 of 8.
- **Every commit.** A temporary worktree built `all` on both arms at each of
  the seven commits: OFF 6,090,091, ON 4,003,560 at each
  (`every_commit_7b_10.txt`).

### Tests added

`test_b4_qsearch` holds 153 checks on the arm in a release build, 216 in Diag
and 233 in Tune, and 145, 174 and 191 on the OFF arm. `test_board` gains 23 in
Tune builds. By ticket:

- **7b.** The default is 0. The gate reads the cap's bar: refused at root depth
  16 and 24 with ±564 and with a mate score either way at node depth 6; not at
  root depth 15 or 12, not at 563, not at node depth 5. The bars are
  independent both ways. Live on `8/8/8/4k3/8/8/8/4K2R w`: the switch moves the
  arm's tree at depth 18 and not at 15, the counter is above 0 at 1 and 0 at the
  default, and the OFF arm is equal at both depths. The earlier gate and live
  tests are replaced, as the amendment's clause requires.
- **The counter.** `qs_gain_tested` read on the exempt and judged recapture (0
  and 1) and on the count rule's position (3). `run_suite.py` carries its unit
  and bounds `qs_gain_tested + qs_recapture_exempt` by what the loop delivered,
  with a test.
- **8.** The gain vector moves a rook's credit from 500 to 538 and en passant's
  to 88 (`test_the_gain_vector`). On tune builds a whole search reads the SEE
  vector on the arm alone. `test_board`, on tune builds: five fixtures on vector
  1 against the oracle, a bishop for a knight reading −27 among them; `see_ge`'s
  boundary equal to the oracle's on every capture of three positions; back on
  vector 0 the same bishop reads 0.
- **9.** Rd8 mate found at 6 and not at 0 at depth 1, reading
  `-(MATE_SCORE - 2)`. Two quiet checks at 6 and one at 1, none past the first
  ply, none that loses SEE.
- **10.** The node check: `has_no_legal_move` where quiescence returns a mate
  with no evasion searched.

Mutants, each run through `mutate.py` and each failing:

| Ticket | Mutation | Result |
|---|---|---|
| 7b | the kernel reads `b3_seek_mate_depth` | 3 sections fail |
| 7b | the kernel ignores `b4_rfp_seek_mate` | 3 sections fail |
| 8 | the search sets vector 0 | the SEE vector section fails |
| 8 | `set_see_vector` ignores its argument | 6 sections fail |
| 8 | `gain_value` ignores its vector | the rook section fails |
| 8 | the kernel passes vector 0 | 2 sections fail |
| 9 | no first-ply test; no SEE test; a fixed limit of 6 | each fails the rule's section |
| 10 | every evasion skipped | `node check failed: has_no_legal_move(*state_.board)`, abort |

### Ticket 8's reads

`bench` ignores options, so each is a Tune build with the default changed
(`tune_variant.py`, `t8/tune_variants.txt`), the source restored byte-exact.

| Setting | ON Tune | OFF Tune |
|---|---:|---:|
| Defaults | 4,003,560 (Release the same) | 6,090,091 (Release the same) |
| `B4GainValues` 1 | 3,675,826 | 6,090,091 |
| `B4SeeValues` 1 | 3,625,848 | 6,090,091 |

### The two-arm table (clean tree at `cedc148`)

| Read | OFF | ON |
|---|---:|---:|
| `bench 13` Release / Tune / Diag / Ablate | 6,090,091 × 4 | 4,003,560 × 4 |
| `bench 10` Release / sanitizer | 2,135,353 / 2,135,353 | 1,047,747 / 1,047,747 |
| Release CTest / sanitizer CTest (`-LE strength`) | 27/27 / 25/25, no report | 27/27 / 25/25, no report |
| WAC depth 6 (floor) | 133 (119) | 149 (136) |
| WAC 100,000 nodes | 247 | **258** |
| Floors, KQ-K / KR-K / KBB-K | 24 at every budget | 24 at every budget |
| KBN-K, 30k to 120k (sum) | 21/22/24/24/24 (115) | 23/23/24/24/24 (118) |
| Wrong-bishop draws (`test_endgames`) | pass | pass |
| Return 1's `4K3/8/8/8/8/5k2/8/1R6 w` at 45,000 nodes, recorded | not read | mate in 37 plies |
| Ticket 7b's variant row, `B4RfpSeekMate` 1, recorded | — | `bench 13` 4,003,560; floors 24; KBN-K 23/22/23/24/23 (115); `test_endgames` 191/191; `test_search` 719/719; WAC 149 / 258 |

The variant row was read at ticket 7b and again on the final source, with the
same result, and it equals the amendment's read on its own patch. Return 1's
row used a Tune build of the final source and Hash 8, cold per move
(`return1_playout.py trace 45000`, `t10/return1_45000.txt`).

Three conversion runs exit 1, and none is a binding floor. Both arms' 120,000-node
runs trip the fixture's own staleness guard on KQ-K (24/24, mean 10.8 plies), as
the B.3 record noted of its head. The OFF arm's 30,000-node run trips the test's
per-budget KBN-K line, 21 against 22, the OFF arm's known read; the binding rule
for KBN-K is the sum.

### Ticket 10 and the rest of the qualification

- **The mask.** No bit is re-declared. The eight bits name interior-node
  families and quiescence holds no `ABLATED` test, so none names a deleted rule.
- **Liveness** (`ablation_liveness_{on,off}.json`, eight `suite_v2` positions,
  depth 9). All eight bits live on both arms, each moving all eight positions.
  ON (67,564 nodes unmasked): razoring 1.47×, reverse futility 2.75×, null move
  1.62×, ProbCut 1.01×, IIR 1.13×, shallow move pruning 6.79×, extensions
  1.02×, LMR 2.00×. OFF (129,311): 0.78×, 1.39×, 0.90×, 0.64×, 0.97×, 2.70×,
  0.72×, 1.59×.
- **The WAC floor.** `test_wac --floor` on the arm is 136, the final count of
  149 less 13. The number did not move at this ticket; `cedc148` states it
  against the final tree.
- **The deep-iteration screen** (`screen25.json`, `screen_score.log`), on §10's
  five positions by the amended clause, depths 16 to 25, 60 s watchdog, Hash
  64, the clean tree's Release binaries, scored by
  `tools/results/b31-20261030/t10/a7/screen_score_a7.py`. **PASS.** No watchdog
  stopped on either arm, so clause (d) does not arise. Host load was about 10%
  at the start; the host was not certified idle.

  | Position | Steps 20, 21, 22 | (b) geomean | Steps 17–19 geomean (recorded) | Absolute at 22 (recorded) |
  |---|---|---:|---:|---:|
  | Bench 11 | 1.004, 0.806, 0.852 | 0.884 | 0.895 | 0.558 |
  | Bench 15 | 1.173, 1.139, 0.872 | 1.052 | 1.050 | 0.578 |
  | `suite_v2` 50 | 0.994, 1.005, 0.998 | 0.999 | 1.049 | 0.695 |
  | `suite_v2` 42 | 1.024, 1.035, 1.031 | 1.030 | 1.036 | 0.263 |
  | `suite_v2` 60 | 1.011, 1.007, 0.998 | 1.005 | 1.014 | 0.989 |

- **The decision trace** on WAC.001, 085, 203 and 253 (depth 8, the WAC move as
  root, Diag): ON 479 / 411 / 1,002 / 158 records, OFF 478 / 1,018 / 866 / 122;
  `trace_{on,off}_WAC.*.json`, archived.
- **Ticket 2's and ticket 6's reads** with their verdicts stand in the record
  above: both defaults stay 1.
- **Not run.** CI: nothing is pushed, so it first runs at the PR. The CI cell
  that builds the arm has existed since ticket 0. No PGO build.

### Deviations from the handoff

1. **`generate_spsa_surface.py` is not edited.** The contract asks that its B4
   table list the new line as fixed. The generator reads the header's table and
   keeps its X lines alone, so an F line is already off the surface:
   `--check` reports the 11 generated files current, with 3 B4 coordinates.
2. **The live tests read one root move.** They use `test_b4_qsearch`'s
   `read_node`, with `e1e2` as the root and a 16 MB table, so their counts are
   not the amendment's 347,065 and 261,485 over UCI. They assert only the
   amendment's differences and equalities.
3. **"At 1 each moves it", ticket 8.** The clause sits beside "on both arms".
   Each vector moves the arm's fingerprint and neither moves the OFF arm's: the
   gain rules and the seam's setter are the arm's, and §10 lets the OFF arm's code
   change only for shared seams. I read this as the intended sense.
4. **Where the vector lives.** `HCE_PIECE_VALUE` is in `src/types.h`, because
   `Board` and the kernel both read it. In a tune build `Board` holds a pointer
   to its vector, set once on the search's own copy in `Searcher::search`, on
   the arm alone, so a SEE call outside a search stays on vector 0. Release,
   Diag and Ablate builds compile the constants as before; `PROMO_SURPLUS` went
   from `constexpr` to `const` to allow the pointer.
5. **The oracle reads a vector.** `test_board`'s `ORACLE_VALUES` became a
   pointer, so the same oracle checks both vectors.
6. **The quiet-check loop repeats the capture loop's tail.** Sharing it would
   have meant editing the OFF arm's loop, which §10 forbids outside shared
   seams. B.8's deletion of the OFF arm removes the repetition. A quiet check
   counts in `qs_searched`, as "searched as captures are" and `run_suite.py`'s
   bound require, and in the fail-high index the capture loop keeps.
7. **Floors at tickets 8 and 9 were not read,** by §10's rule: neither changed
   the fingerprint, and each ticket's counters equal the one before. The final
   table reads them.
8. **The screen's position set.** §10 names B.3's five positions, and those were
   read. B.3's amendment 8 had replaced bench 11 in that cluster's own screen
   because it is a conversion-band position, so its row here is recorded as §10
   requires. It passes either way.

### What the reviewer must check

- Each diff against §10 and §7 as amendment 2 §9 replaces them.
- Both fingerprints at every commit, on every flavour, and the tests on both
  arms.
- The deviations above, 3, 4 and 6 first, as judgements and not as settled.
- That `B3SeekMateDepth`, `B3SeekMateScore`, the seed of `B4RfpSeekMateCap` and
  every Cluster 1 and 2 constant are unmoved (`git diff de91e38 cedc148 --
  src/search_params.h` touches the B4 table and its comments alone).
- The mutants above, and that the arm's tree equals ticket 6's row for row.

Separate, for B.4.5 and not a read: `qs_recapture_exempt` over
`qs_gain_tested + qs_recapture_exempt` is 22.1% on the bench positions at depth
13, on ticket 9's tree. P5 registers 15 to 30% on B.4.5's own read.

## Review — B.4.4 accepted with named follow-ups (2026-11-06)

Independent review of B.4.4 at `44b4868` (the source of `cedc148` with one
record-only commit on top), in a separate session from the implementing one
(PROCESS *Reusable review prompt*; §10 *Review*; both records' *What the
reviewer must check*). Class `R2`. Nothing was fixed. No default, seed,
floor, test or clause was moved, and no game, SPRT, SPSA or NPS read ran.
Every read below is a node-budget or fixed-depth read on trees built from
the committed source by this session; the host was shared throughout, which
those reads tolerate. Evidence is in `tools/results/b44-20261106/review/`
(ignored storage); the clean trees are a detached worktree, `build/rev-b4`.

**Verdict: accept, with the follow-ups of §7.** Every fingerprint, test and
binding read of both records reproduces from clean sources on every flavour
and both sanitizer trees. The code matches §10 as research amendments 1 and
2 replace it, clause for clause, with the deviations the records list and
six they do not, none a change of mechanism. The invariants of §10 and
DESIGN §3 hold on both arms. The OFF arm's edits are shared seams. Amendment
2's decision tables reproduce from its raw data, and the two the decision
leans on reproduce again from searches this session ran. Two statements of
the second record are wrong, and each is corrected by a read made here: the
screen's first row is not bench position 11, and `run_suite.py`'s bounds do
not hold with quiet checks on. Neither blocks a registered read of B.4.5,
so they are follow-ups and not a return.

### 1. Rebuilt from clean sources, every row reproduced

Ten trees configured and built from nothing at `44b4868` with the record's
recipe (`review/review_table.py`, which is `t10/final_table.py` pointed at
this session's worktree): the release preset, PEXT portable, Clang 23.1.5,
target `all`, for Release, Tune, Diag and Ablate, and the `debug` preset
(ASan, UBSan) for the sanitizer trees, on both arms.

| Read | OFF | ON | Record |
|---|---:|---:|---|
| `bench 13` Release / Tune / Diag / Ablate | 6,090,091 × 4 | 4,003,560 × 4 | equal |
| `bench 10` Release / sanitizer | 2,135,353 / 2,135,353 | 1,047,747 / 1,047,747 | equal |
| Release CTest | 27/27 | 27/27 | equal |
| Sanitizer CTest, `-LE strength`, node checks armed | 25/25, no report | 25/25, no report | equal |
| Diag: `test_stack_unwind`, `test_b4_qsearch`, `test_b3_proof`, `test_search` | 5, 174, 118, 719 | 5, 216, 118, 719 | equal |
| Tune: `test_b4_qsearch`, `test_board` | 191, 422 | 233, 422 | equal |
| WAC depth 6 (floor) | 133 (119) | 149 (136) | equal |
| WAC at 100,000 nodes | 247 | **258** | equal |
| KQ-K, KR-K, KBB-K at the five budgets | 24 at each | 24 at each | equal |
| KBN-K, 30k to 120k (sum) | 21/22/24/24/24 (115) | 23/23/24/24/24 (**118**) | equal |
| Wrong-bishop draws (`test_endgames`, inside CTest) | pass | pass | equal |
| Liveness of the eight ablation bits, depth 9 | all eight | all eight | the JSON equal, number for number |
| Decision traces, WAC.001 / 085 / 203 / 253 | 478 / 1,018 / 866 / 122 | 479 / 411 / 1,002 / 158 | the records equal, one for one |
| Deep-iteration screen on the record's five positions | — | PASS, no watchdog stop | every node count equal, depths 1 to 25, both arms |
| Return 1's position at 45,000 nodes, Hash 8, cold per move | mate in 15 plies | mate in 37 plies | equal (the OFF cell is `return1/sweep.txt`'s) |
| Variant row, `B4RfpSeekMate` 1 | — | `bench 13` 4,003,560; floors 24; KBN-K 23/22/23/24/23 (115); 191/191; 719/719; WAC 149 / 258 | equal |

The three conversion runs that exit 1 are the record's three (both arms at
120,000 on the KQ-K staleness guard, the OFF arm at 30,000 on its per-budget
KBN-K line). The binaries' hashes differ from `t10/hashes.json`, as a fresh
build's do; the fingerprints are what the review compares.

**Earlier tickets, spot-checked** in a second worktree
(`review/spot_commits.py`): at `e429e48`, `6d5a23f`, `ea3b49e`, `86feb7b` and
`b003b3c` the OFF arm reads 6,090,091 and the arm reads the first record's
6,740,352, 4,997,291, 4,287,895, 4,757,482 and 3,166,820. At `b003b3c` the
arm's KR-K reads 23 of 24 at 45,000 nodes, which is Return 1's premise. At
`6d5a23f` `test_wac --floor` passes on the arm (139 against a floor of 128),
so deviation 2's commit order does keep CTest green there.

**What `every_commit.txt` and `every_commit_7b_10.txt` measured.** One
worktree stepped through the 13 and the 7 commits, built `all` on both
arms' release trees incrementally, and read `bench 13` from the release
binary. That is the claim the records make of them. They did not run CTest,
the Tune or Diag flavours, or a build from nothing at each commit; those are
the per-ticket reads on the implementer's trees and the final table. The
five commits between `b003b3c` and `de91e38`, and `44b4868`, touch no
source.

### 2. The code against the handoff

Read against §10's "The arm's quiescence, stated once", §7 and §9 as
amendment 2 §9 replaces them, and the donor's `qsearch` at `edb0d9db`
(`:1653-1883`). Lines are `src/search_kernel_core.cpp`'s at `44b4868`.

| Clause | Code |
|---|---|
| Entry, draw, upcoming repetition, probe, non-PV table cutoff: today's | `:197-249`, untouched by the leaf |
| In check: best from minus infinity; the kernel's picker; the table move first when a legal evasion | `:290-298`; the picker's own stage validates piece, colour and legality (`move_picker_core.cpp:122-131`) |
| Once the best is not a loss, under `B4EvasionPrune`: quiets skipped, a capture or promotion failing `see_ge(m, B4QsSeeMargin)` skipped | `:305-315` and `:346-347` (`skip_quiets()`, so no quiet is delivered again) |
| No legal move is mate | `:349-352`, with the node check of ticket 10 |
| Static eval the corrected one; a non-decisive table bound may replace the best | `:430-446`, the tightening untouched |
| Stand-pat: blended unless decisive, stored as a depth-0 lower bound with no move, returned | `:448-461`, `b4::fail_high_value` |
| Futility base on the static eval; the gain rules only above `−KNOWN_WIN` | `:470-471` |
| `B4QsDelta` 1 restores the delta rule | `:475-494` |
| Alpha rises to the best; no ply cap | `:496-502` |
| Legal captures by the picker's score, the table move first; a quiet table move neither searched nor counted | `:504-516`: the move is matched among the generated captures, so a quiet never appears |
| `move_count` counts every capture delivered | `:567` |
| Gain rules, in order, for a move that is no check, promotion or exempt recapture: count, futility with its raise, alpha-relative SEE with its raise | `:568-602`; `b4::is_recapture`, `b4::gain_value` |
| Plain SEE for every capture | `:603-610` |
| Quiet checks, first ply only, after the captures, up to the limit, `see_ge(m, 0)`, generator order | `:685-727` |
| Stalemate: no capture generated, no non-pawn material, a knight or heavier just captured, no legal move | `:732-745` |
| A fail high blends unless decisive; the store is depth 0, a lower bound at or above beta, the PV bit kept; a fail low stores no move, or the best-scored one under `B3FailLowStore` 0 | `:342-344`, `:374-376`, `:675-678`, `:721-722`, `:750-752` |
| Ticket 7b: the cap on its own bar, off | `:1201-1208`, `b3::seek_mate` with `b4_rfp_seek_mate_depth` |

**Seeds, ranges and defaults.** `BASILISK_B4_PARAMS` holds §7's rows as
amended: `B4QsFutBase` 86 (40 to 220), the blends 441 and 462 (256 to 1024),
`B4QsMoveCount` 2 (1 to 64), `B4QsSeeMargin` −28 (−200 to 0),
`B4RfpSeekMateCap` 5 (1 to 19), `B4RfpSeekMateDepth` 16 (8 to 64), and the
switches at §9's defaults with `B4RfpSeekMate` 0. The arm's
`B3FailLowStore` is 1 through `BASILISK_B4_CORE_DEFAULTS`. The diff of
`src/search_params.h` from `bc8e449` adds that table, that default and the
constructor's second arm, and moves no existing value: `B3SeekMateDepth` 12,
`B3SeekMateScore` 564 and every Cluster 1 and 2 constant are unmoved. The
generated surface holds the three X lines (`generate_spsa_surface.py
--check`: 11 files current, 3 B4 coordinates).

**Tickets.** Each ticket's tests are the ones §10 lists, and each can fail
(§5). Ticket 0's counters are appended (166 to 174 entries), the three
trace events and the two pricing rules exist, and the CI cell builds `all`
on the arm, runs CTest and prints `bench 13`; it has not run, because
nothing is pushed.

**Deviations the records list, ruled.** E is an ordinary engineering
choice; none is a change of mechanism.

- *First record, 1 to 3 and 9.* The start at `bc8e449`: E. The WAC floor
  moved in its own commits ahead of the engine commits: E, and it is the
  rule of `agents/implementation.md`, since each move tightens a stale
  bound and is checked above. `test_tt_switches_live` reading 0 against 1:
  E, in its own commit. The direct band test: E, and it is the stronger
  test; its mutant fails in release as well as in Diag.
- *First record, 4 to 8*, ruled by amendment 2 §6. The code matches each
  ruling: the picker's depth is 0 (`:297`); quiet delivery ends as soon as
  the best is not a loss; the stalemate's 0 takes no blend and stores on
  either side of beta; `QsSee` prices both SEE rules and the skipped quiets
  are priced only at a node that ended without a cutoff; `qs_gain_tested`
  exists and `qs_recapture_exempt`'s comment names it.
- *Second record, 1 and 2.* The generator not edited: E; an F line never
  reaches a surface. The live tests on one root move: E.
- *Second record, 3: "at 1 each moves it" read as the arm alone.* Right.
  "Each" is each vector. The OFF arm is the baseline of every later read
  and may not answer a Cluster 3 switch, tune builds advertise the table on
  the arm alone, and §10 keeps the OFF arm's code to shared seams.
- *Second record, 4 and 5: the Board seam, `HCE_PIECE_VALUE` in
  `types.h`, the oracle's pointer.* E. The vector is the registered one,
  fixed, so a named constant is its right form. One property the record
  does not state: `Board`'s copy assignment does not carry the vector
  (`board.cpp:240`), so a copy reads vector 0. Nothing copies the search's
  board today, and every thread's `Searcher::search` sets its own.
- *Second record, 6: the quiet-check loop repeats the capture loop's
  tail.* E. The two tails are equal but for the trace index. The record's
  reason for counting a quiet check in `qs_searched` is half wrong: one
  bound of `run_suite.py` needs it and another is broken by it (found
  deviation 5 below).
- *Second record, 7.* By §10's rule. The final table reads the floors.
- *Second record, 8: the screen's set.* The sentence is wrong (found
  deviation 1 below).

**Deviations the records do not list.**

1. **The screen was not read on bench position 11.** The file the record
   read, `tools/results/b31-20261030/t10/screen_positions.epd`, is the set
   B.3's amendment 8 chose (the B.3 review cites it by the same content
   hash). Its first position is `suite_v2` 12,
   `rn2kb1r/pp2pppp/2p5/3q4/3P1Bb1/5NP1/PPP2P1P/R2QKB1R w`, and the
   record's table labels that row "Bench 11". §10 names bench 11,
   `2r3k1/1q2Rp1p/p2p2p1/1p1P4/1Pp1P3/2Q5/1P4PP/6K1 w`. This review read
   it by the same clause (`screen25_bench11.json`): steps 20 to 22 read
   0.679, 0.441 and 0.887,
   geometric mean 0.643; steps 17 to 19 0.669; the absolute ratio at depth
   22 is 0.059; no watchdog stopped on either arm. **Pass.** So §10's
   clause holds on its own set and on amendment 8's. E, a mislabelled
   read; recorded here.
2. **The delta rule under `B4QsDelta` 1 reads the arm's band guard**, the
   static eval plus `B4QsFutBase`, where the OFF arm's reads the stand-pat
   plus 150 (`:471`, `:476`). §10 says "the OFF arm's delta rule". The two
   differ in a band 64 wide at `−KNOWN_WIN` and where a table bound moved
   the stand-pat across it. E; it is B.4.7 (c)'s arm, and the registration
   should say so.
3. **Every promotion is a capture-stage move in both loops**, Basilisk's
   own definition, where the donor's is a capture or a queen promotion. A
   quiet underpromotion that answers a check is therefore tested on SEE
   and not skipped as a quiet, and quiescence searches quiet
   underpromotions at all, on both arms. E; §14 has no row for it.
4. **Tune builds run the evasion rule's accounting whether or not anything
   reads it** (`:353-372`): a legal quiet generation, a trace call and the
   pricing loop at every evasion node that ends without a cutoff with its
   quiets skipped. That is at most 97,624 of 5,261,149 nodes (1.86%) on
   `suite_v2` at depth 12. It runs only with `B4EvasionPrune` 1. §9 item
   1's paired run is one tune build with "no build offset between the
   arms", and this is an offset inside the build, against the default. E;
   follow-up 2.
5. **`run_suite.py` aborts on the arm with `B4QuietChecks` 6.** On the Tune
   build at depth 10 the bound "quiescence move rules within its captures"
   fails after 25 positions: a quiet check counts in `qs_searched` and
   not in `qs_moves`. `qs_gain_tested`'s comment names a denominator that
   is true at 0 only, for the same reason. At the defaults the bounds hold
   on real runs of both arms (depth 12, 106 positions, exit 0). E, an
   instrument defect at one switch's second value; follow-up 1.
6. **The mate return stores nothing** (`:349-352`), as the donor's. §10 is
   silent. E.

### 3. Invariants, both arms

- **No static score in the mate band; no blend on a decisive value.** The
  blend's inputs are a stand-pat that is never decisive and a searched
  value that `fail_high_value` returns untouched when decisive. Test:
  "blends: a decisive value is untouched"; it fails under mutant M12. A
  non-decisive value above a beta in the loss band blends to a value
  between the two, as the donor's does and as the interior fail-high blend
  already does: at the seed, 0 against −32,000 returns −18,218. That is a
  valid lower bound, and no consumer reads it as an evaluation.
- **A returned fail high stays at or above beta; the stored lower bound is
  the value returned.** The arithmetic test covers every weight in range
  and can fail. The store is by reading at the four sites; a test reads it
  at the stand-pat alone (M2 survives the tests and moves the fingerprint).
- **The stand-pat takes no decisive table score.** By reading, a line the
  leaf does not touch. No test fails without it (M9); the fingerprint
  moves.
- **A quiescence mate is a proven mate.** By reading: `holds` needs a best
  above the loss band, and the first evasion is always searched. Tests:
  two sections fail under R1; the node check aborts under R12. On this
  review's depth-12 Diag run at least 38,361 evasion nodes returned the
  mate under that check and none aborted, which answers amendment 2 §9's
  "plausible and has not been checked".
- **Table scores ply-adjusted, the stored evaluation raw, none in check.**
  Every store goes through `tt_store`; the stored evaluation is `tt_eval`,
  the undamped raw one; an evasion node stores `INF_SCORE`, which a test
  reads and M17 breaks.
- **Every table move validated.** The evasion picker's stage validates it
  (a test seeds a move that answers no check; it is never played). The
  capture loop matches it among generated legal captures.
- **A PV node takes no table cutoff; the exit store keeps the PV bit.** By
  reading; no test fails without either (M10, M7), and both move the
  fingerprint.
- **The gain rules stand down in the known-loss band.** The direct test,
  which fails under R2 in Diag and in release. The plain SEE still runs
  there, as the OFF arm's SEE at 0 does.
- **The draw value is 0, the stalemate's included.** Tests, with the
  window either side.
- **The stack unwinds; quiescence allocates nothing.** `test_stack_unwind`
  and the node checks on both arms, in Diag and under the sanitizers. By
  reading, the arm's lists are fixed arrays on the stack, the picker sorts
  in the search's per-ply buffer, and the quiet-check path and the node
  check's helper do the same. PLAN B.8.3's guard does not exist yet.

**What no test here reads.**

- *Rule 50 and repetition.* An evasion node is entered through the draw
  test like any other, and its quiet evasions' children are too. The arm
  stores evasion nodes, so a depth-0 bound can now carry a path's draw
  (a quiet check, a quiet evasion, a repeated position) under a key with
  no clock and no path. Interior nodes have that exposure today and the
  donor has it at depth 0. A quiet check is searched at the first ply only
  and its reply is an evasion, so no chain of reversible moves can form.
- *En passant and promotion.* En passant is credited a pawn (tested) and
  can never be a recapture, since it lands behind the pawn. A promotion is
  exempt from the gain rules and meets the plain SEE. In check, see found
  deviation 3.
- *A null move before the node.* `b4::is_recapture` refuses it and "no
  move". No test reads that (M11); without the guard the fingerprint reads
  3,792,866, so it is live.
- *SMP and the Board seam.* Tune builds only; each thread sets the vector
  on its own copy. No shared state is new, so no ThreadSanitizer run is
  owed.
- *A quiet table move outside check.* Tested: neither searched nor counted.

### 4. The OFF arm

Every OFF-arm edit is a seam. Those the fingerprint alone does not prove
are named with what does.

- *Quiescence.* The arm's branches are `if constexpr (kB4Qsearch)` and the
  OFF arm's rules sit unchanged in the `else` (futility on the stand-pat
  plus 150, the clamped SEE, the late rule). The stand-pat and fail-high
  values are ternaries that fold to today's. The delta rule and the ply cap
  keep their conditions. The exit store's flag became `best >= beta ?
  lower : upper` on both arms: on the OFF arm the best is below beta there
  by construction, so the flag is today's by argument, not by the bench.
- *Reverse futility.* `seek_mate_capped` is false on the OFF arm.
- *`SearchParams`.* Fifteen fields more on both arms. The OFF tune build
  advertises none of them (a test) and no arm default reaches it (a test).
- *`Board`, tune builds.* `see` and `see_ge` read through a pointer on both
  arms. The OFF arm never sets it. The bench proves the values where the
  bench reaches; `test_board` proves the fixtures on vector 0. The cost of
  the indirection is a tune build's alone.
- *Counters, trace, pricing.* Eight counters, which read 0 on the OFF arm
  over 106 positions at depth 12. The OFF arm's pricing cells for
  quiescence's move rules now carry the class in the depth slot, as
  amendment 1 asked; the bench cannot see that, and `test_pricing.py` and
  the isolation read can. Isolation on the final tree, 20 positions at
  depth 10: the arm 114,787 probes, the OFF arm 174,772, 0 positions
  differ on either.
- *Tests and tools.* `test_wac`'s floor is 119 on the OFF arm as before.
  The four tool test scripts pass.

### 5. The mutants

All in the review's worktree (`review/rev_mutate.py`), Diag tree unless
said, the source restored byte-exact after each and the arm's `bench 13`
read at 4,003,560 at the end.

**The records' twelve, re-run: each fails as recorded.** The loss test
replaced (2 sections); `prune_by_gain = true` (the band's section, in Diag
and in release); the kernel reading `b3_seek_mate_depth` (3 sections); the
kernel ignoring the switch, both ways (3 and 2); the search setting vector
0 and the setter ignoring its argument (Tune: the SEE vector section, and 6
of `test_board`); `gain_value` ignoring its vector and the kernel passing
0 (2 sections each); no first-ply test, no SEE test, a fixed limit of 6
(the rule's section each); every evasion skipped (the node check aborts).

**The review's own**, each run against `test_b4_qsearch`, `test_search`,
`test_endgames`, `test_b3_proof` and `test_stack_unwind`:

| Mutation | Tests | `bench 13` |
|---|---|---:|
| M1 no SEE rule for evasions | survive | 3,509,884 |
| M2 a capture fail high stores the unblended value | survive | 3,189,104 |
| M3 no alpha-relative SEE | survive | 4,269,348 |
| M4 the alpha-relative SEE without its raise | one section of `test_b3_proof` | — |
| M5 no plain SEE | survive | 4,314,417 |
| M6 the count rule counts searched captures only | survive | 4,858,064 |
| M7 the evasion stores drop the PV bit | survive | 4,414,090 |
| M8 futility without its raise | survive | 3,846,850 |
| M9 the stand-pat takes a decisive table score | survive | 2,934,041 |
| M10 a PV node takes the table cutoff | survive | 3,014,624 |
| M11 the recapture test without its null-move guard | survive | 3,792,866 |
| M12 the blend applied to a decisive value | the blend's section | — |
| M13 checks not exempt from the gain rules | survive | 3,410,360 |
| M14 promotions not exempt | one section of `test_b3_proof` | — |
| M15 no blend at an evasion or quiet-check fail high | one section of `test_b3_proof` | — |
| M16 no blend at a capture fail high | survive | 3,285,407 |
| M17 an evasion node stores an evaluation | the exit store's section | — |

Reading. Every test §10 asked for exists and can fail. Twelve clauses of
"the arm's quiescence, stated once" are bound by the fingerprint and by no
test: a mutant of each passes all five test programs and moves `bench 13`.
M9 and M10 are lines the leaf did not write. M4, M14 and M15 fail one
section only, `test_b3_proof`'s "B3FailLowStore and B3TtPenalty move the ON
arm only", which asserts that two settings give different node counts on
one bench position: it is a liveness test that any change of the tree can
flip, not a test of those clauses. Follow-ups 5 and 6.

### 6. Amendment 2, checked without leaning on it

**Order.** The reads were registered at `99ddb2c` (11:38:37) and the
first evidence file is `r2_exposure.txt` at 11:41:30. R8 was registered at
`1764dc3` (12:00:32), after `r4_mates.json` (11:56:37) and before
`r8_reach.json` (12:20:11). R9 was registered at `6e460f9` (12:09:40),
after `r1/floors.json` (12:08:14) and before `r9_kbnk.json` (12:18:00).
Each rule preceded its read.

**Re-scored from the raw data** by this review's own script
(`a2_rescore.py`, not the amendment's report scripts):

- *The grid (§5).* `OFF` 600 / 599 / 600 with the KR-K miss at 35,000 on
  `R7/8/8/4k3/1K6/8/8/8 w`; `T6` 600 / 600 / 600; `T7` 600 / 599 / 600 with
  the miss at 45,000 on Return 1's position; `CAND` 600 / 600 / 600. KBN-K
  587, 587, 587 and 577 of 600, with 11, 12, 10 and 21 unconverted at
  60,000 nodes or fewer; the five registered budgets sum to 115, 118, 114
  and 115. `CAND` differs from `T6` at 6 of 25 KR-K budgets. All as
  written.
- *Mates found (§4).* 512, 498 and 505 of 1,092; the four cells and the
  discordant pairs as written; at 1,000,000 nodes 293 against 302 with 7 to
  16, sign test 0.093. R8: 297 against 304, 11 to 18.
- *R9 (§3).* 100 positions, none on the grid; 15, 12 and 22 unconverted of
  600; 1.467 times; Fisher 0.158.
- *R2 and R5.* 188,020 moves; 13.37% and 6.97% of moves, 52.8% and 52.7%
  of games; 235, 234 and 235 of 240; 94 each of 96; 60.6 against 62.9
  plies. As written.

**Re-run from searches** on this session's clean builds:

- The OFF arm's `test_endgames --conversion --nodes 35000` reads **KR-K 23
  of 24**. The base rate's one event is real: a tree that plays on `master`
  misses a major-piece floor at a budget the register does not hold.
- R4's population A, 576 searches (96 positions, two budgets, three
  settings) on the committed Tune build with the switch set over UCI: all
  576 equal `r4_mates.json` in depth, kind and value. 52, 50 and 52 at
  250,000 nodes; 71, 66 and 66 at 1,000,000. So the committed ticket 7b is
  the patch the amendment read.
- The variant row of §1 equals the amendment's R6 on its patch.

**Scored as frozen?** Thirteen of the fifteen are. Two are softened:
A2-P5 and A2-P11 are each one statement with one probability, each has a
clause that missed (52.8% of games against "at least 60%"; KBN-K 115
against 118), and each is scored "half". As frozen, both are misses.
Nothing decides on either. Follow-up 3.

**Does "the cap stays off" follow?** Not from the registered rule, which
adopts the candidate, as the amendment says itself. It follows from the
maintainer's standing decision that a working Basilisk form is kept, which
the amendment sets above its own clause after the results were in. The
review's reading:

- The evidence is as stated. No read reaches a registered bar either way.
  The donor's own measure shows nothing for the cap at either reach, and
  that part is reproduced here from searches.
- Overriding a frozen rule after exposure is the thing registration exists
  to prevent. Here it is done in the open, the registered verdict is kept
  beside it, and the rule it yields to was the maintainer's before the
  read. The decision is therefore the maintainer's, not research's, and
  amendment 2 §10 item 1 puts it there.
- The stakes are one default. Both settings read `bench 13` 4,003,560 and
  pass every binding read; the arm's KBN-K sum is 118 off and 115 on.

The review's position is off, for the amendment's reasons. It should stand
in PLAN as the maintainer's decision once confirmed (§7, the maintainer's
decisions).

**Does a prediction need more before B.4.5?** No. P3, P4's table-move
clause and P6 stand as frozen and will score as misses on their registered
reads; none carries a decision rule the exposure touches. P5's
`QsFutility` clause read 2.04% on an unregistered read against 1.5% or
less, inside its falsifier's 2.5%; it stands and its consequence is an
order in B.4.6's sweep. P7 holds on the final tree.

### 7. Follow-ups (records and registrations; no code, no default)

1. **`run_suite.py` at `B4QuietChecks` above 0** (found deviation 5).
   B.4.5 records at its opening that the counter suite is not read with
   quiet checks on until the bound and `qs_gain_tested`'s comment are
   repaired in their own commit, with a test that fails first. Item 5's
   gate reads `pricing.py`, which does not pass through the bound, so the
   gate is not blocked. Owner: B.4.5's opening.
2. **Item 1's paired run carries a build offset** (found deviation 4).
   BAS-S50 registers, before that run, how it is handled: the accounting
   gated on the run-time flags that read it, in a behaviour-neutral commit
   at both fingerprints; or the offset stated beside the verdict, with its
   size from an NPS pair of the two settings on the tune build. Owner:
   BAS-S50's entry.
3. **Records for BAS-S50's entry and §14.** The screen's first row is
   `suite_v2` 12, and bench 11 passes by this review's read. A2-P5 and
   A2-P11 are misses as frozen. §14 gains a row for the capture stage
   (every promotion, where the donor's is a capture or a queen promotion).
   Owner: B.4.5's opening.
4. **B.4.7 (c)'s arm.** The registration says that `B4QsDelta` 1 restores
   the delta rule under the arm's band guard (found deviation 2). Owner:
   BAS-S52's entry.
5. **The clauses no test binds** (§5: M1, M2, M3, M5, M6, M7, M8, M11,
   M13, M16). Their tests are owed before the default flips, in their own
   commit, each shown to fail on its mutant. Owner: B.4.7, on H1, before
   the flip.
6. **`test_tt_switches_live` follows the tree.** Three unrelated
   quiescence mutants flip it. B.4.6's bake changes the arm's tree; if the
   test fails there, its precondition is repaired in its own commit, as
   `af06733` did. Owner: B.4.6's bake.
7. **CI.** The arm's cell has never run. It runs at the PR. Owner: the
   maintainer, at the push.

**The maintainer's decisions**, none of which the review takes: amendment
2 §10's three (the cap off; a denser floor for later clusters; the evasion
sort to H.2). The review's position on each is the amendment's.

Not follow-ups, listed for completeness: `Board`'s copy assignment not
carrying the vector, the mate return storing nothing, and the path-carried
draw in a depth-0 evasion bound (§3), none with a consequence the review
can name.

### 8. Artifacts (`tools/results/b44-20261106/review/`, ignored storage)

| Artifact | Identity |
|---|---|
| The table | `review_table.py`; `review_table.out`; `hashes.json` (release binaries' SHA-256: ON 5EEEE7EC…, OFF DCB1A92D…); the logs beside them |
| Screen, Return 1's position, the variant row | `review_reads.py`; `screen25.json`, `screen_score.log`; `screen25_bench11.json`, `screen_score_bench11.log`; `return1_45000.txt`; `reads_variant.out` |
| Earlier tickets | `spot_commits.py`, `spot_commits.txt` |
| Counter suite | `suite_on_d12.json`, `suite_off_d12.json` (Diag, depth 12, stride 1); `suite_on_qc6_d10.log` (the failing bound) |
| Pricing isolation | `isolation_on.log`, `isolation_off.log` |
| Mutants | `rev_mutate.py`; `mutants.json`, `mutants.out`; `mutant_bench_part1.json`, `mutant_bench.json` |
| Amendment 2 | `a2_rescore.py`, `a2_rescore.txt`; `a2_rerun.py`, `a2_rerun_hash64.json`, `a2_rerun.out`; `conv_off_35000.log` |
| Tool tests | `py_test_run_suite.log`, `py_test_pricing.log`, `py_test_decision_trace.log`, `py_test_spsa_surface.log` |

### 9. The maintainer's decisions on the review (2026-11-06)

The maintainer took the review's positions as decisions.

1. **The cap stays off in the arm.** `B4RfpSeekMate` 0, as amendment 2 §3
   left it. The switch, its two F lines, the counter and the tests stay
   for the re-read on the network (H.2, mates found at a node budget
   first). Amendment 2 §10 item 1 is closed.
2. **The tune build's offset is gated, not measured** (follow-up 2). The
   evasion rule's accounting runs only when something reads it: the Diag
   option or pricing on. It lands at B.4.5's opening as a behaviour-neutral
   commit at both fingerprints, before item 1's paired run, and BAS-S50
   cites it.
3. **A denser conversion floor is registered at B.5's research**
   (amendment 2 §10 item 2): the 25-budget grid with a bound on
   unconverted playouts. Nothing in B.4 uses it, and B.4's floors stay as
   registered.
4. **The full evasion sort is an H.2 candidate** (amendment 2 §10 item 3).
   B.4.5 gets no sixth categorical.
5. **`run_suite.py`'s bound is repaired on the tool's side** (follow-up 1):
   a quiet check stays in `qs_searched`, the two bounds that compare
   searched moves with delivered captures subtract `qs_quiet_checks`, and
   `qs_gain_tested`'s comment says so. No counter changes its meaning.

The review's clean trees (`build/rev-b4`) were removed after this record;
its logs, JSON and scripts stay in `tools/results/b44-20261106/review/`.
`review_table.py` rebuilds the worktree, and `rev_mutate.py` runs in it.

## B.4.5 — opening (2026-11-06)

What PLAN B.4.5 owed at its opening, from the review's §7 and the maintainer's
decisions of its §9. It is not B.4.5's screens and not its runs: no game, SPSA,
NPS read or zero-game screen ran. Evidence: `tools/results/b45-20261106/opening/`
(ignored storage).

### The commits

| Commit | What | Fingerprints |
|---|---|---|
| `cdee38f` | Tooling. `run_suite.py`'s two bounds that compare searched moves with delivered captures subtract `qs_quiet_checks`; `test_run_suite.py` gains a case that failed on the old tool | no engine change |
| `33654f8` | Engine, behaviour-neutral. The evasion rule's accounting runs only when `config_.limits.diag` or `state_.price.enabled()` holds. `qs_gain_tested`'s comment names its denominator: `qs_searched` less `qs_quiet_checks` plus the rules' prunes | OFF 6,090,091, arm 4,003,560 |

The trace starts only with `limits.diag` set (`search_worker.cpp:357`), and the
counters print only under it, so those two flags are every reader the
accounting has.

### Qualification

**`cdee38f`.** The new case failed before the repair (1 failed, 8 passed,
`test_run_suite_before.log`) and passes after it (9 passed). On real runs:

| Run | Exit | Against the review |
|---|---|---|
| The arm's Tune build, `B4QuietChecks` 6, depth 10 | 0 (`qs_quiet_checks` 123,921) | the review's run failed after 25 positions |
| The arm's Diag build, depth 12, stride 1, Hash 64 | 0 | `suite_on_d12.json`: 177 rows and 106 records, 0 differ |
| The OFF arm's Diag build, the same | 0 | `suite_off_d12.json`: 177 rows and 106 records, 0 differ |

**`33654f8`** (`gate_reads.py`, `gate_reads.txt`), on the committed tree:

| Read | Result |
|---|---|
| `bench 13` Release / Tune / Diag / Ablate | OFF 6,090,091 × 4, ON 4,003,560 × 4 |
| Release CTest | 27/27 on both arms |
| `test_b4_qsearch` Diag / Tune | ON 216 / 233, OFF 174 / 191, all pass |
| The arm's depth-12 counter suite | equal to `suite_on_d12.json`: 177 rows, 106 records, 0 differ |
| Pricing isolation, 20 positions, depth 10, every 4 | 114,787 probes, 0 positions differ |
| Decision traces, WAC.001 / 085 / 203 / 253 | 479 / 411 / 1,002 / 158 records, each equal to `t10/trace_on_*.json` apart from hash, path and time |

**Mutants**, the source restored byte-exact after each:

- *The gate forced false:* `test_b4_qsearch` fails in Diag, in "a holding capture
  and three quiets search no quiet" and "a quiet table move is the one quiet
  searched" (`mutant_gate_false.txt`).
- *The Pricing half dropped* (`pricing_mutant.py`): see below.

**One read beyond the list.** Isolation runs with Diag on in both arms, so it
cannot see the gate's Pricing half on its own. A `pricing.py read` with Diag off,
20 positions, depth 10, every 4, on a Diag binary of `cdee38f` and on one of
`33654f8`: all 20 runs equal, cells included, and `summary.json` differs in its
`engine` field alone (`price_base/`, `price_gate/`). With the Pricing half
dropped, all 20 runs differ, so the comparison can fail.

**What no read shows.** That the block is skipped with neither flag set has no
observable but speed, and no NPS read ran. The evidence is the code's one
condition and the two mutants above, each of which shows that a flag being false
stops the block.

### §14, one new row

| Place | Basilisk's form (kept) | The donor's form | Why kept | Waits on |
|---|---|---|---|---|
| The capture stage | a capture or any promotion, so quiescence searches quiet underpromotions and SEE-tests one that answers a check | a capture or a queen promotion, so quiescence does not search a quiet underpromotion and skips one that answers a check | it is Basilisk's existing definition, on both arms | B.6.1 |

### BAS-S50

`docs/experiments/BAS-S50.md` is registered, before any game, from §11's text as
amended. It carries amendment 1's re-registered P5, amendment 2 §8's two recorded
reads, that `B4RfpSeekMate` has no paired run, and the review's records: the cost
screen's first row is `suite_v2` 12 and bench 11 passes by the review's read
(steps 20 to 22, geometric mean 0.643); A2-P5 and A2-P11 are misses as frozen;
item 1's offset is gated at `33654f8`; `run_suite.py` is repaired at `cdee38f`.
The run table's seeds are not in §11; they follow BAS-S41's numbering (50000 and
the item's number).

### Revision

The categoricals and the paired run read the head with these two commits.
§11 says "the accepted B.4.4 head": `33654f8` is that source with the gate on top,
at both fingerprints and with the counters, prices and traces above unchanged.

## B.4.5 — the zero-game screens (2026-11-06)

BAS-S50's screens, on `abc3b0f`'s binaries (engine source `33654f8`). Node-budget
and fixed-depth reads, except the pooled NPS read, which ran first and alone on the
idle host the maintainer gave. No game ran. Evidence:
`tools/results/b45-20261106/` (`screens/`, `binding/`, `cost/`, `analyze.py`,
`analyze.txt`) and `tools/results/b45-nps/`. BAS-S50's *RESULT* holds the scored
predictions; this section holds the tables.

### §2's table, depth 14

`run_suite.py --depth 14 --stride 1 --hash 64` on `suite_v2`, Diag builds. The OFF
column equals §2's on the rows §2 prints.

| Quantity | OFF | ON |
|---|---:|---:|
| Interior / quiescence nodes | 9,434,361 / 6,891,318 (42.2%) | 6,491,965 / 4,487,603 (40.9%) |
| In-check interior nodes | 17.22% | 13.93% |
| Exits of quiescence nodes: table / stand-pat / delta / ply cap / fail high / fail low / evasion | 17.23 / 27.27 / 3.76 / 0.01 / 11.56 / 30.69 / 9.49 | 16.69 / 25.43 / 0 / 0 / 11.83 / 37.01 / 9.03 |
| Stand-pat: mean margin over beta at a cut | 378 | 446 |
| Capture loop, of captures: futility / SEE / late / count / searched | 24.0 / 29.1 / 0.0 / 0 / 23.4 | 13.9 / 24.1 / 0 / 17.1 / 24.8 |
| Capture fail highs: first move; mean margin | 92.6%; 213 | 94.3%; 186 |
| Fail-low exits that searched a capture | 25.4% | 34.7% |
| Evasions: searched per node; fail highs of nodes; first move; legal table move | 1.849; 34.2%; 63.0%; 4.5% | 0.881; 41.5%; 98.3%; 3.2% |
| Estimate: refined of interior; depth-0 of refinements; depth-0 of distance | 17.7%; 25.5%; 16.7% | 16.1%; 21.6%; 12.1% |
| No table move; from a depth-0 entry | 65.10%; 3.38% | 72.58%; 3.76% |
| Reverse futility: cuts; on a table bound | 24.5%; 9.0% | 26.4%; 6.0% |
| Razoring; null tries, conversion; ProbCut tries, conversion | 3.33%; 6.69%, 61.2%; 6.24%, 76.1% | 3.06%; 7.79%, 62.8%; 6.06%, 76.8% |
| Gain rules: tested; exempt; share | — | 2,661,020; 820,046; 23.6% |

### 5.8's read

The firing share on the arm against the OFF arm, the rule 10% of itself.

| Candidate (mechanism) | OFF | ON | Change | Rule |
|---|---:|---:|---:|---|
| `B3NmpPerPly` (null tries / interior) | 6.689% | 7.786% | +16.4% | **joins** |
| `B3ProbCutBase`, `B3ProbCutImproving` (ProbCut tries / interior) | 6.244% | 6.061% | −2.9% | stays off |
| `CoreCaptFutBase` (capture futility / moves tested) | 6.866% | 7.158% | +4.3% | stays off |
| `CoreCaptSeeCoeff` (capture SEE / moves tested) | 7.568% | 7.758% | +2.5% | stays off |
| `CoreQuietFutBase` (quiet futility / moves tested) | 37.523% | 39.794% | +6.1% | stays off |
| `CoreLmrTtCapture` (LMR applied / moves tested, a proxy) | 25.006% | 24.036% | −3.9% | stays off |
| `CoreResearchDeeper` (re-searches / LMR applied, a proxy) | 5.557% | 4.915% | −11.6% | **joins** |

§5.8 froze the candidates and not their denominators. These are B.3.2's where it
read the same mechanism, and no counter separates a table-move capture's LMR or the
deeper re-search, so those two rows are proxies. `CoreResearchDeeper`'s verdict
rests on one.

### Pricing 4, depth 16

`suite_v2`. ON 20,277,540 search and 43,005,065 probe nodes; OFF 31,212,872 and
80,650,493. Diag and Tune read the same search at the defaults, cell for cell.

| Rule | OFF | ON |
|---|---:|---:|
| `QsEvasion`, pooled | — | 6.37% [6.24, 6.49] of 150,008 |
| — at PV quiescence nodes | — | 20.26% [18.41, 22.25] of 1,683 |
| — elsewhere | — | 6.21% [6.09, 6.33] of 148,325 |
| `QsCount` | — | 2.15% [2.11, 2.20] of 407,968 |
| `QsFutility` | 4.18% [4.14, 4.22] of 921,677 | 1.86% [1.81, 1.91] of 334,369 |
| — a queen, no recapture | 45.88% [41.33, 50.50] of 449 | 21.24% [20.09, 22.43] of 4,690 |
| `QsSee` | 1.60% [1.58, 1.63] of 1,118,008 | 1.46% [1.43, 1.49] of 588,724 |
| — a queen, no recapture | 27.27% [26.19, 28.39] of 6,303 | 23.04% [22.04, 24.08] of 6,579 |

**Item 5's gate.** Razoring's beat share at expected cut nodes reads 17.31% [16.85,
17.78] at `B4QuietChecks` 0 and 6.44% [6.17, 6.73] at 6: −62.8%, past the rule's
quarter. At all nodes it reads −4.2%. The search grows 1.22× at 6. §9 item 5's gate
is met, so its paired run is owed; P10 said it would not be.

### Item 2's table-move reads

Tune build, depth 14, the same search as Diag at store 1 (only the cross-colour
telemetry rows differ, a Tune-only instrument).

| Setting | Total nodes | `tt_move_absent` | `tt_move_d0` | `qs_evasion_tt_move` of evasion nodes |
|---|---:|---:|---:|---:|
| `B3FailLowStore` 1 | 10,979,568 | 72.58% | 3.76% | 3.16% |
| `B3FailLowStore` 0 | 14,082,885 | 62.00% | 6.74% | 7.76% |

Amendment 2 §7's account of P4 holds in direction: the share more than doubles with
moves in the store, and it stays under 15% with them, as a stored fail high still
ends a node's next visit before the counter.

### The rest

The floors, WAC, canaries, cost screen, pooled NPS and items 1 and 2's binding reads
are in BAS-S50's *RESULT*. On the pooled NPS read: P9's falsifier fires. The arms'
trees differ, so −5.57% is the two engines on two trees and not the arm's cost per
node. PROCESS's 2 Elo per 1% would price it near −11 Elo if it were all speed.
Nothing here says how much is ticket 1's evasion delivery.

### Reads not taken

`depthpv 10` and the oracle differential are not in §11's list and were not read.
`wac_depthpv12.json` exists for the canaries.

## B.4.5 — the speed work (2026-11-06)

P9's falsifier fired at the screens, and the maintainer asked for a profile and for
the speed recovered before the games. Every change here is behaviour-neutral: no
default, seed, floor or fingerprint moved. Evidence: `tools/results/b45-20261106/prof/`
and `speed/`, and `tools/results/b45s-nps/`; BAS-P29 is the speed entry.

### What the arm's gap is made of

`tools/profile.ps1` on `bench 13`, 30 passes an arm, and a throwaway patch timing the
evasion delivery with `rdtsc` (`speed/evasion_cost.py`, restored byte-exact). The
arm costs about 24 ns a node more than the OFF arm, spread over the move picker
(about 10 ns), the main search and quiescence.

- **Ticket 1's evasion delivery, measured.** 458 cycles an evasion node on the arm,
  228 on the OFF arm: 1.78% of the run against 0.95%. It accounts for about **0.8
  points** of the gap, so P9's falsifier named a real cost and not the main one.
  What remains of it is the two generations and the quiet scoring an exact order
  needs. Making the attack maps lazy, below, is the part that could go.
- **The tree's mix, estimated.** The arm's tree holds more interior nodes (58.2%
  against 55.0% on the bench positions at depth 13), and an interior node costs
  several times a quiescence node. At a cost ratio of 2.5 to 3 that alone gives 2.6
  to 3 points. This is an inference from the counters, not a measurement.
- **`gives_check`.** 1.22 calls a node against 0.98, the capture loop testing every
  non-exempt capture for check before the gain rules. It cannot move behind them: a
  rule that would prune still needs the answer.

P9's premise, that the delivery would carry the cost, is therefore mostly false. The
ratio is a property of the arm's tree, and it stands at 0.945 after this work.

### What was tried

Same-tree A/B: two final-PGO builds a side, six interleaved cycles of a pinned
`bench 13 3 1` (`prof/ab.py`). Build-to-build noise is about ±0.3%.

| Candidate | Read | Kept |
|---|---:|---|
| The imbalance from a table keyed on the ten piece counts | **+2.46%** | yes |
| Material and piece-square values in one walk of the occupied squares | **+1.02%** | yes |
| SEE: attackers once at entry, then only the line a capture opens | **+0.43%** | yes |
| The sort's scan without a branch on the score | **+0.31%** | yes |
| `score_quiets`' attack maps wait for the first piece that uses them | **+0.27%** | yes |
| Quiescence prefetches the child's table entry | +0.10%; +0.09% at Hash 64 | no |
| `see_pins` skips a line with no enemy slider on the king's rays | +0.09% | no |
| The arm skips the plain SEE once the gain SEE settles it | −0.15% | no |
| LMR's window term once per window, not per move | −0.33% | no |
| `make_move` writes its undo record in place | −0.36% | no |
| The picker's check squares unzeroed, `fill` without a branch | −0.43% | no |
| The arm runs the higher of its two SEE thresholds first | −0.54% | no |
| `gives_check` skips lookups off the king's rays | −0.69% | no |

The five kept are three commits: `1ee67b7` (the evaluation), `1990343` (the quiet
stage) and `516d422` (SEE). They touch shared code, so both arms gain. The prefilters
lost because a PEXT lookup is cheaper than the branch that avoids it.

Two tool repairs came with them: `search_b4.h` (`7c4a381`) and `search_pricing`
(`4422d12`) were missing from `tools/diag/profile_regions.json`, which refused the
arm's profile and left `test_profile_regions.py` failing since B.4.4.

### The pooled read

`tools/nps_read.ps1`, four pools of four final-PGO builds, two cycles, host at 1% or
less, no cycle disturbed (`tools/results/b45s-nps`):

| Contrast | Delta | 95% interval | Verdict |
|---|---:|---|---|
| The arm, `b45s-on` over `b45-on`, same tree | **+3.99%** | [+3.84%, +4.14%] | ACCEPT |
| The OFF arm, `b45s-off` over `b45-off`, same tree | **+4.16%** | [+4.00%, +4.33%] | ACCEPT |
| P9's ratio, `b45s-on` over `b45s-off` | −5.49% | [−5.61%, −5.37%] | 0.945× |

The arm now reads 2.784M, 98.4% of the old OFF arm's 2.829M. P9's ratio does not
move, because the gain is shared: the paired run's speed handicap is what it was.

### Qualification on `516d422`

Clean tree (`speed/final/`, the recipe of `t10/final_table.py`), every flavour from
nothing:

| Read | OFF | ON |
|---|---:|---:|
| `bench 13` Release / Tune / Diag / Ablate | 6,090,091 × 4 | 4,003,560 × 4 |
| `bench 10` Release / sanitizer | 2,135,353 × 2 | 1,047,747 × 2 |
| Release CTest / sanitizer CTest | 27/27, 25/25, no report | 27/27, 25/25, no report |
| WAC depth 6 / 100,000 nodes | 133 / 247 | 149 / 258 |
| Floors; KBN-K sum | 24; 115 | 24; 118 |

Neutrality beyond the fingerprint, each against the record it must equal:

- the depth-12 counter suite on both arms: 177 rows and 106 records, 0 differ
  from the review's files;
- pricing isolation: 114,787 and 174,772 probes, 0 positions differ;
- the eight decision traces and both liveness reports: equal to `t10`'s;
- the cost screen on the final PGO binaries: node for node `cost/screen25.json`;
- game-like, Hash 64, 40 positions at 250,000 nodes: identical node totals, +3.66%
  (`prof/ab_game.py`);
- `TEXEL_TRACE` build: `test_eval` passes on the kept per-piece path.

Tests added: the imbalance table against a fresh evaluator and across a parameter
change, and the sort move for move against the one-pass reference with ties; each
mutant fails, and SEE's two line mutants fail the oracle corpus.

### Revision

BAS-S50's binaries are re-pinned, before any game, to `516d422`. The screens stand:
they read trees this work leaves unchanged. P9 stays scored as a miss.

## B.4.5 — the games (2026-11-06)

The maintainer ran BAS-S50's six games in the registered order on the binaries of
`516d422`, 2,000 games each at `3+0.03`, with no fault and no time loss. A positive
result favours arm A.

| Run | Arm A against arm B | W/D/L | Pentanomial | Elo, 95% interval | Verdict |
|---|---|---|---|---|---|
| item 1 | `B4EvasionPrune` 0 against 1 | 521/997/482 | 29, 223, 459, 258, 31 | +6.8 [−2.4, +15.9] | default stays 1 |
| item 2 | `B3FailLowStore` 0 against 1 | 498/986/516 | 27, 260, 448, 234, 31 | −3.1 [−12.3, +6.1] | default stays 1 |
| paired | ON against OFF | 638/892/470 | 26, 180, 446, 296, 52 | **+29.3 [+19.8, +38.7]** | P1's stop does not fire |
| item 3 | `B4GainValues` 1 against 0 | 538/913/549 | 38, 249, 434, 244, 35 | −1.9 [−11.5, +7.6] | default stays 0 |
| item 4 | `B4SeeValues` 1 against 0 | 506/985/509 | 25, 255, 451, 236, 33 | −0.5 [−9.7, +8.6] | default stays 0 |
| item 5 | `B4QuietChecks` 6 against 0 | 499/951/550 | 37, 267, 436, 230, 30 | −8.9 [−18.3, +0.5] | default stays 0 |

No default moves, and P1's stop does not fire. The arm enters B.4.6 as it played:
`bench 13` 4,003,560, `B4EvasionPrune` 1, `B3FailLowStore` 1, `B4GainValues` 0,
`B4SeeValues` 0, `B4QuietChecks` 0, `B4RfpSeekMate` 0.

- **The paired run** says the unfitted arm carries no defect. It is a diagnostic: the
  default flips only on B.4.7 (a), and nothing here attributes the Elo to a mechanism.
- **Item 1** leans towards searching every evasion, +6.8, but its interval holds 0,
  so the rule leaves it. No re-run is owed, and BAS-S52 registers none.
- **Item 5** met its gate at −62.8% and still read −8.9, at 0.945× the defaults' speed.
- **Items 3 and 4** are nulls of live options.

Speed in play, from the paired run (nodes over reported time, a bootstrap over pairs):

| Plies | Arm over OFF | 95% interval |
|---|---:|---|
| all | 1.012× | [1.009, 1.015] |
| 0 to 39 | 1.007× | [1.003, 1.011] |
| 40 to 79 | 0.994× | [0.988, 1.000] |
| 80 to 119 | 1.003× | [0.995, 1.011] |
| 120 on | 0.990× | [0.980, 1.000] |

The arm is level with the OFF arm in play, against 0.945× on `bench 13`. That is a
recorded read with no rule: P9 stays a miss, its falsifier still binds before B.4.7's
gate, and why the two reads differ is not established. The speed entry's inference
that the bench gap is the arm's tree is consistent with it and not proved by it.

Evidence: `tools/results/b45-20261106/games/` (`recount.py`, `liveness.py`,
`game_nps.py` and their outputs). BAS-S50 *RESULT* has the checks and hashes.
