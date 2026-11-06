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
