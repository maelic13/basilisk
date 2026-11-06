# Cluster 3 research and handoff — PLAN B.4.3

Started 2026-11-06 on `b4-quiescence` at `86769bf`: the B.4.2 head
(`d3add9c`, `bench 13` 6,090,091) with the counter seam `1fd783b` and its
suite-runner units `86769bf` on top, both behaviour-neutral. Donor:
Stockfish `sf_19` `edb0d9db` (`src/search.cpp` line numbers below are that
file's; `qsearch` is `:1653-1883`). Kernel symbols are
`src/search_kernel_core.cpp`'s at `86769bf`.

**Status: `RESEARCH`.** Section R is frozen before any read of it runs;
the sections after it are written from the reads.

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
