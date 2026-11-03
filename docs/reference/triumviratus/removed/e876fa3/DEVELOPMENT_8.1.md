# Triumviratus 8.1: development log

The work toward 8.1, on the `dev` branch. The record of 8.0 is in [`DEVELOPMENT_8.0.md`](docs/development/archive/DEVELOPMENT_8.0.md); the
rules for contributions are in [`CONTRIBUTING.md`](docs/project/CONTRIBUTING.md).

## Plan

8.1 keeps the Consilium network of 8.0: all the work is on the engine side. The order follows one principle: the
structure of the search first, then the tuning of its parameters, because a tuning made before a rewrite would no
longer fit the rewritten code.

**Goal.** 8.1 moves away from Stockfish's code, with no loss of strength. The search and the inference of the network
are rewritten block by block; every block passes a non-regression SPRT before the old code is removed. Known ideas
stay, credited in the documents. Status on 20 October 2026 (audit against Stockfish 19): the text of the search is our
own (about 0.4% of literal token sequences in common, the same as engines written independently), but its structure
and many formulas still follow Stockfish 19, and replacing them with forms of our own is the next step. The three smaller
parts that still followed Stockfish's code closely (the threat update of the network, the SEE threshold test, the
repetition tables) were rewritten on 20 October 2026 with identical results (same bench; every accumulator checked
against a refresh; every SEE call checked against the previous form).

**Measurement rules**, as in 8.0: one test at a time on the test machine; every search change by an SPRT run to a
decision, with at least 20,000 games; a short time control first, then a long one for the changes that pass; small
levers go to SPSA rather than to single SPRTs; speed only with hardware counters at an identical search tree, the
engine pinned to one core.

**In order:**

1. **The base.** A non-regression SPRT of `dev` (pull request 6 and the prefetch limited to non-AVX-512 builds)
   against 8.0; if it passes, `dev` is the base of 8.1. For the release build, an option to import a PGO profile
   trained on another machine: Zen 4 trains variants 1 to 6 natively, an Intel processor with AVX-VNNI trains
   variant 7. The gain of native profiles for `vnni512` and `avx512icl` is measured on Zen 4 first.
2. **Where we lose.** A study of the games 8.0 loses against the strongest engines (CCRL and TorsGUI games): the
   phase of the game, which expert of the network misjudged the position, the time left; the use of time at the CCRL
   controls (40/15 with increment); scaling with 4 and 8 threads, which matters for the 4CPU
   lists; the search in endgames and the use of tablebases. It runs on stored games and does not need the test
   machine, so it starts at once. Its result says which search components to rewrite first.
3. **A search of our own** ([future directions, section 11](docs/project/FUTURE_DIRECTIONS.md#11-a-search-shaped-by-the-network)).
   A rewrite of the search one component at a time, designed for a network of experts, each with a non-regression
   SPRT at unchanged parameters. Six blocks, and a seventh for the infrastructure outside the search; the first four in this order, because each one feeds the next:
   1. **histories and move ordering**: a history of quiet moves indexed by the threat context of the move, in a form
      of our own built on the value of the attacking pieces, and the ordering rethought around it
      ([block 1](docs/development/8.1/SEARCH_1_ORDERING.md));
   2. **reductions**: one formula of our own on our signals (the expert of the network, the causal reduction, the
      rank of the move's ordering score) ([block 2](docs/development/8.1/SEARCH_2_REDUCTIONS.md));
   3. **pruning**: reverse futility, null move, futility, ProbCut and losing captures, with per-expert margins where
      they pay ([block 3](docs/development/8.1/SEARCH_3_PRUNING.md));
   4. **extensions**: singular extension, the low-depth hash-move extension and internal reduction in one design
      ([block 4](docs/development/8.1/SEARCH_4_EXTENSIONS.md));
   5. **the core of the search**: quiescence, the use of the transposition table, iterative deepening with aspiration
      windows, and the threads ([block 5](docs/development/8.1/SEARCH_5_CORE.md));
   6. **the inference of the network**: the forward pass, the SIMD layers and the loading of the network, rewritten to
      give bit-identical evaluations, so that strength is unchanged by construction and only speed is measured
      ([block 6](docs/development/8.1/CONSILIUM_INFERENCE.md)). **Done on 18 October 2026**: a module of our own,
      written in a clean room from a specification of the network, identical evaluations;
   7. **the infrastructure outside the search**: the last files of Stockfish origin outside the search (slider
      attacks, large pages and their helpers), rewritten in the same clean-room way. **Done on 18 October 2026**: the
      folder `source/nnue/` is gone. Blocks 6 and 7 together: 5.5% fewer cycles per node in the middlegame and 4.7%
      in endgames on AVX-512, 2.0% and 1.5% on AVX2, at the same search tree
      ([block 6 document, section 6.9](docs/development/8.1/CONSILIUM_INFERENCE.md#69-block-7-the-last-files-of-stockfish-origin-outside-the-search)).

   Each stage: a written design, code behind an option (off, the tree is unchanged), a non-regression SPRT, then the
   old code is removed. Each block has its own document with the design, the tests and their results. Time management
   runs as a fifth line in parallel ([time management](docs/development/8.1/TIME_MANAGEMENT.md)).

   **New levers.** The levers a rewrite adds start off and are tested at the end of the queue. Each block document
   lists its own, with their tests: [block 1, section 1.7](docs/development/8.1/SEARCH_1_ORDERING.md#17-the-tuning-queue),
   [block 5, section 5.7](docs/development/8.1/SEARCH_5_CORE.md#57-tests-and-tuning-of-block-5).
4. **Tuning with the corrected schedule** ([section 8](docs/project/FUTURE_DIRECTIONS.md#8-tuning-again-with-the-corrected-schedule)).
   The six SPSA runs P1 to P6 on the final structure, without hyperthreading; each vector enters the engine only
   through an SPRT against the release. The blocks are tuned one at a time, so before the release one joint run
   tunes a selected set of levers of all the blocks together: the ones whose runs were still moving, and the ones
   that act on the same nodes from different blocks (ordering scales and the reduction and pruning margins built on
   them). A run on every lever of every block would be several hundred levers, too many for SPSA in the time
   available; the selection is listed in each block document.

   **A lesson from the tuning of block 1, for every later run.** The block 1 runs used a schedule built for 3,000
   iterations and were stopped after 200 to 860, so the perturbation was still about 2.25 times its final size. For a
   few levers the lower or upper test point fell where the formula changes nature: at `CorrDepthQ` = −36 the reward
   of the correction tables changes sign from depth 2, and at `CorrGain` = 1.7 the corrections are exaggerated. Those
   levers moved fast, away from the broken side, and their drift says little about their best value. From now on:
   8 to 15 levers per run; ranges chosen so that the start value ± 2.25 c never crosses such a point; the iteration
   count of the schedule equal to the iterations actually played; no hyperthreading; 20,000 to 40,000 games per run;
   halfway through, a regression test of the gradient, and a run without signal is stopped. The block 1 runs are
   repeated this way at the end of the queue, for the small gain left (an analysis of the logs puts it at 0 to 3 Elo
   for the block).

   **Ideas to test after the closing of block 1** (from an analysis of the tuning logs and of about 25,000 games;
   expected gains are estimates):

   | # | idea | where | expected | test |
   |---|---|---|---|---|
   | 1 | **Time follows where games are decided.** The expert with 24 pieces or more takes 46% of the thinking time and decides 6% of the games; the expert with 10 to 15 pieces takes 15% and decides 43%. Save time in the first and spend it in the second (per-expert time levers `TmExp0`-`TmExp3`, `TmEarlySave`), checking that the real shift of time is at least 20% | time manager | +2 to +5 | UHO 40+0.4, then 10+0.1 |
   | 2 | **Correction tables keyed on eight regimes** (piece band × queens on the board): the queens double the evaluation swings inside a band; the same tables with a different key | correction histories | 0 to +3 | UHO 10+0.1 without hyperthreading, 20,000 games |
   | 3 | **One joint tuning of the levers that link the histories to reductions and pruning** (history gravity, reward to penalty ratio, history thresholds of pruning and reduction, about 12 levers), with phase entering as a base and a slope on a continuous variable instead of four free values | blocks 1 to 3 | +1 to +3 | SPSA at 10+0.1 without hyperthreading, then SPRT |
   | 4 | **Endgame rules**: no null move when the side to move has very few legal moves (exact count from the legal generator); quiet checks with a non-losing exchange at the first quiescence ply only with 15 pieces or fewer | blocks 3 and 5 | +1 to +4 on endgames | endgame book without draw adjudication, then UHO |
   | 5 | **The correction levers retried in a safe form**: `CorrDepthQ` in [0, 32] or as a multiplicative term that cannot change the sign of the reward | block 1 | unknown | SPRT on its own |

5. **Release of 8.1.**

**In parallel:**

- **The open measurements of 8.0** ([sections 6, 7 and 12](docs/project/FUTURE_DIRECTIONS.md#7-open-engine-measurements)).
  The time-management ones (time per expert, the graded surprise rule, the two defence rules) do not depend on the
  search and run whenever the machine is free. The search ones (LDSE, reduction scale per expert, `DrawDefend`, the
  null-move guard from the analysis build) are taken up inside step 3, with the rewrite of the component they touch.
- **Speed.** A Tin Man continues his work (a legal generator with the AVX-512 compacted writing is the first
  proposal); every pull request is measured on the Xeon, on Zen 4 and on an Intel processor with AVX-VNNI. Changes
  at an identical tree do not touch the parameters, so they do not disturb steps 3 and 4.
- **The move generator and the search.** The legal generator gives the exact number of legal moves at every node at
  no cost. Known search ideas built on that count are tested inside the stages of step 3 that they touch: reductions
  and late-move pruning scaled by the legal count (stages 2 and 3); an extension when only one move is legal, with no
  singular verification search in that case, and no check extension when the evasions are many (stage 4); a
  null-move guard when few moves are legal; mate and stalemate recognised in quiescence; an instant move with a
  single legal reply (time management). If they gain, the legal generator is used on every build and the search
  builds on the count. If they do not, both generators stay and each variant of the universal executable uses the
  faster one: the two give the same moves in the same order, so the bench is the same on every build.
- **For users.** Playing strength levels (`UCI_LimitStrength`, `UCI_Elo`, calibrated in games,
  [section 13](docs/project/FUTURE_DIRECTIONS.md#13-playing-strength-levels)) and Chess960 in the release build,
  which today has it only in development builds; with it the engine can enter the CCRL FRC list.
- **Machines.** The test machine also plays CCRL tournaments: an OpenBench instance with the testers' machines would
  let SPRT and SPSA runs use them too.

## 1. Speed at an identical tree: the first external contribution (15 October 2026)

Pull request 6, by **A Tin Man**, merged into `dev` on 15 October. Six changes, all keeping the search tree identical
(bench 137419 in every build the author could run; the check on the pull request confirmed it):

| # | change | where it acts |
|---|---|---|
| 1 | **Legal move generator** (`source/movegen_legal.h`), ported from the author's engine [Zoomies](https://github.com/CodeAdopter/Zoomies) (MIT; its generator descends from [Surge](https://github.com/nkarve/surge), MIT) and written to give the moves in exactly the order of our generator. Generated moves skip the legality test before the move and the king test of the make; the hash move keeps the old checks; Chess960 keeps the staged generator. `tdperft` compares the two generators at every node | every build |
| 2 | **Per-thread evaluation cache removed**: 12 hits in 2.5 million lookups at bench depth 20, 62 in 9.4 million over a 40-ply game. The transposition table already stores the evaluation | every build |
| 3 | **Prefetch of the hash entry of the next move's child**, decided at start-up from the processor vendor: on for Intel, off for AMD, where it loses | Intel |
| 4 | AVX2 rank counting of the move ordering in full groups of 8, as the AVX-512 path already did | AVX2 builds |
| 5 | Threat rows of the network widened without `vpmovsxbw`, through a byte interleave at load | `avx2-nopext`, `avxvnni` |
| 6 | **AVX-VNNI variant** (variant 7 of the universal executable, `make avxvnni`): the dot products of the network with `vpdpbusd` in its 256-bit form, for Intel processors from Alder Lake on without AVX-512. `TRIUMV_ISA` now checks a forced variant against the processor's features | Intel 12th generation and later |

The evaluation cache could be removed without changing the tree because it stores the raw output of the network,
which depends only on the position. An early note of the 8.0 log (section 5) said that dropping it changed the tree:
that was true of an older design, which stored evaluations together with the optimism of the iteration.

**The author's measurements**, whole series against `dev`, nodes identical, g++ with link-time optimisation and no PGO:

| CPU | build | gain |
|---|---|---:|
| Intel i9-14900KF (Raptor Lake) | `avxvnni` (chosen there by the universal executable) | **+9.17%** [+8.76, +9.58] |
| Intel i9-14900KF | `avx2` | +3.63% [+3.36, +3.91] |
| AMD Ryzen 7 5700 (Zen 3) | `avx2` | +5.00% [+3.29, +6.75] |
| AMD Ryzen 7 2700X (Zen+) | `avx2-nopext` | +3.23% [+2.66, +3.80] |

**Measurements on the project's machines.** Two profile-guided universal executables of the same batch (clang-cl,
deterministic PGO training), built from `dev` before the pull request (47ea7b9) and after the merge (a2e06ed); bench
137419 in every variant the build machine can run. A third engine, `base_copia`, is the same file as the first under
another name: its difference from `base` is the noise of the measurement.

Hardware counters (xperf), Xeon Gold 6138, `avx512` variant, engine pinned to one processor, 30 positions at 400,000
nodes, 4 rounds in alternating order:

| | cycles/node, middlegame | cycles/node, endgames | instructions/node | branch mispredictions/node |
|---|---:|---:|---:|---:|
| pull request against base | +0.36% | +0.40% | −0.54% / −0.81% | +2.2% / +1.9% |
| noise (`base_copia` against base) | +0.14% | +0.45% | +0.03% / −0.02% | +0.05% / +0.03% |

Raw speed, `bench 18` (2,840,711 nodes) repeated with the three engines in rotating order, engine pinned to one
processor; mean of the per-round ratios with its 95% interval:

| machine | variant | rounds | pull request against base | noise |
|---|---|---:|---:|---:|
| Xeon Gold 6138 | `avx512` | 30 | +0.32% [−0.92, +1.57] | −0.95% [−3.45, +1.56] |
| Xeon Gold 6138 | `avx2` (forced, `TRIUMV_ISA=avx2`) | 20 | **+1.03%** [+0.19, +1.86] | −0.32% [−1.11, +0.46] |
| Zen 4 laptop | `vnni512` | 30 | +0.81% [−0.82, +2.44] | −0.88% [−2.38, +0.62] |
| Intel Core 7 150U (Raptor Lake, laptop) | `avxvnni` (base: `avx2`) | 30 | **+6.34%** [+3.76, +8.92] | +0.82% [−2.26, +3.89] |

On processors with AVX-512 only changes 1 to 3 act, and the result is level or slightly positive: the AVX-512 build
already wrote the generated moves with `vpcompressd` (−1.5% of cycles, [8.0 log §20](docs/development/archive/DEVELOPMENT_8.0.md)), so
the legal generator saves instructions there but adds branch mispredictions. On the AVX2 path the gain is real but
smaller than on the author's machines (+1.0% against +3.6% on Raptor Lake), probably because the profile-guided build
already arranges the branches that the changes remove. On an Intel processor of the Raptor Lake family the
universal executable picks the new AVX-VNNI variant, and the gain with release-style builds is +6.3%, against the
author's +9.2% without PGO; the node counts of the bench are identical in that variant too. The AVX-VNNI variant of
these builds is trained on the Xeon, which lacks AVX-VNNI, with AVX2 instructions. Trained on the Core 7 150U
itself, with its own instructions (`build_universal.ps1 -Only 7`, the same deterministic training), the variant gains
more:

| Intel Core 7 150U, `bench 18`, 30 rounds | against base (`avx2`) |
|---|---:|
| universal executable, `avxvnni` trained on the Xeon | +5.51% [+4.06, +6.96] |
| `avxvnni` alone, trained on the 150U | **+6.84%** [+6.02, +7.66] |

A native profile adds about 1.3%, and the speed is steadier from round to round (lowest round 992,908 nodes per
second against 835,503). The 8.1 release will therefore train the AVX-VNNI variant on a processor that has it and
bring that profile into the universal build; the build script needs an option to import a profile trained elsewhere. The change stays: it costs nothing on AVX-512 and helps the processors where it
acts.

**Each change on its own, Xeon.** Five `avx512` builds with PGO from the same batch (base, the full pull request,
and the pull request without one change each), xperf with the engine pinned, 4 rounds; cycles per node against the
full pull request:

| | middlegame | endgames |
|---|---:|---:|
| without the next-move prefetch | **−0.40%** | **−0.72%** |
| with the old staged generator | +0.26% | −0.22% |
| with the evaluation cache back | +0.19% | +0.32% |
| noise (`base_copia` against base) | 0.28% | 0.19% |

The prefetch costs on the dual-socket Xeon in both workloads, and without it the branch mispredictions return to the
level of the base: it is now off in the AVX-512 builds (avx512, vnni512, avx512icl) and stays on where it was
measured, on Intel processors without AVX-512. Removing the evaluation cache helps here too. The legal generator and
the old one are level on AVX-512: the legal generator saves the legality tests, while the old one wrote the moves of a
piece sixteen at a time with `vpcompressd`, which the legal generator does not do. A legal generator with that
compacted writing might take both gains on AVX-512 processors; it is proposed to the author as a next step. Which
generator each build uses in the end depends on the search ideas built on the legal move count (see the plan).

**Node counts.** The bench is identical, but in a workload of 30 positions searched one after another in the same
process, without `ucinewgame` in between, the pull request searches 12,008,338 nodes per round against 12,008,522,
the same difference in every round. The likely cause is the removed evaluation cache: a hit stored with a different
fifty-move count returned a value that could differ by rounding from a fresh evaluation, so the old tree depended on
what the cache held. The difference is 15 nodes in a million and does not matter for strength; the tree is identical
when the cache is cold, as in the bench.

## 2. Code review by Bastiii (17 October 2026)

**Bastiii** read the development branch and sent a list of findings. Each one was checked against the code; the ones
that hold are in `dev`.

| finding | change | tree |
|---|---|---|
| A race in the transposition table: with several threads, another store can rewrite a way between the scan and the two reads of `store_tt`, and the old move and evaluation are then taken from a different position | the words are reused only if they still carry the tag of the position; otherwise the entry is overwritten (`tt.h`) | identical |
| Dead work in the evaluation: the network-scale cache (`NnLast`, two 64-bit products and divisions and three stores per evaluation) had no readers left | `nn_pos_eval` no longer writes it (`nnue_bridge.cpp`) | identical |
| `hashfull` counted entries kept from earlier searches | only entries of the current search count; output only | identical |
| The `Makefile` still named the 7.0 network, so Linux builds missed the embedded file | `NET_NAME ?= nn-consilium.nnue`; `CODEGEN=1` adds the two code-generation flags of the Windows build through `lld`, opt-in until tried on Linux | identical |
| `gen_cooc_perm.py` defaulted to a co-occurrence sample too small for the network | default `--cooc-n 8192` | tools only |
| The static exchange evaluation ignored the gain of a promotion, and the search never sent promotions to it | promotions go to the exchange evaluation, which adds the promotion gain and values the piece on the target square as the promoted one (`see.cpp`, `see_holds`) | changes the tree |

**Speed.** xperf on the Xeon, engine pinned, 4 rounds, against a copy of the base (noise of the copy against the base:
0.50% middlegame, 0.31% endgames): the fixes cost 0.0% cycles per node in the middlegame and +0.4% in endgames, inside
the noise. A second suggestion, skipping the slider lookup when a knight recaptures in the exchange loop, adds 0.2% to
0.3% instructions with no measurable saving in cycles; the loop stays without that branch.

**The promotion gain in the exchange evaluation.** SPRT on endgame positions (12 to 18 pieces) without draw
adjudication, 8+0.08, both sockets, against the same build without it: **+1.78 ± 1.76 Elo over 14,874 games** (LLR +2.63 for non-regression on [−3, 0]; stopped just short of the +2.94 bound to free the machine, with the lower end of the interval above zero). It is kept: the new bench is
**126352**.

## 3. First incremental result: the rewritten code at the tree of 8.0 (19 October 2026)

From this point 8.1 grows one piece at a time. Each piece is measured alone against the current base and enters only
if it holds; a piece that loses inside the noise of a mixed test would otherwise add up with the others unseen.

**The base.** Blocks 1, 5, 6 and 7 are in our own code and give the tree of 8.0 at their starting values; the network
inference of block 6 and the speed patches of block 9 make the engine faster at the same tree. The only change to the
tree is the promotion gain in the exchange evaluation (section 2). Bench **126352**; with that one change taken out,
137419, the bench of 8.0, checked on a clean build.

**Against the official 8.0.** Frozen PGO build (AVX-512) against the universal executable of 8.0, 8+0.08 with
hyperthreading, 36 games per socket: **+10.97 ± 5.67 Elo over 4,054 games** (LLR +2.92 on [−3, +1]). At the same
time per move the new build searches deeper on both sockets (15.75 against 15.63 plies on the faster socket, 14.97
against 14.75 on the slower one, whose memory has two channels instead of four), which is what a faster engine at the
same tree should do.

**What was taken out first.** Before this base, 8.1 carried the reduction formula of our own (block 2, section 2.4 of
[`SEARCH_2_REDUCTIONS.md`](docs/development/8.1/SEARCH_2_REDUCTIONS.md)), a re-search rule of our own and a lever that
reduces less in volatile nodes, each accepted on a level or slightly positive test of its own. Together, against 8.0,
they lost at every long time control (−4.5 ± 9.7 at 10+0.1 without hyperthreading, −23 ± 11 at 8+0.08 with it) and
searched 0.35 to 0.6 ply less deep at the same time per move. They stay in the code as options, off, with their fitted
values, and come back one at a time, each with its own test against this base.

**Tuning: all the coarse runs are discarded.** The 34 tuned values of block 1, measured alone on this base, lost
(−5.83 ± 8.13 over 2,086 games at 4+0.06 with hyperthreading) and stay out, as do the packages of the other coarse
runs (evaluation scale, block 2 levers). The values of our own will come from new runs at a longer time control and
with many more iterations, where a lever enters only when its net move is at least 2.5 times the random-walk
expectation.

**Block 2: the reduction of our own is back, with the learned correction (19 October 2026).** The reduction formula of
our own and `RfLearn`, against the formula of 8.0 on this base: −0.51 ± 3.55 Elo over 10,272 games, parity. Baked;
the formula of 8.0 is removed (bench 213149). The singular extension of block 4 is in our code at the same tree, and
its first tuning runs at 10+0.1 on one socket ([`SEARCH_4_EXTENSIONS.md`](docs/development/8.1/SEARCH_4_EXTENSIONS.md)).

**Block 2 on hold (19 October 2026, midday).** On a Zen 4 at long time controls, with both engines universal builds,
the reduction of our own lost against 8.0 (−4.43 ± 7.35 over 1,962 games at 12+0.12), so block 2 goes back to the
formula of 8.0 (bench 126352) and stays on hold. The tuning of block 4 continues on one socket of the Xeon.

**Block 4 tuned and baked (19 October 2026, evening).** The SPSA vector of block 4, level against the previous values
(+0.22 ± 2.89 over 14,266 games), is baked: its values are now our own except four thresholds (bench 209655). Five
reduction levers of our own are written and off, to be tested one at a time: disagreement between the two parts of the
network, captures into another expert, attacks on the defender's king zone, danger to our own king, pawn breaks in
closed structures.

**Reduction levers and the ladder of block 4 (19 October 2026, night).** The reduction levers were measured one at a
time at strong values (8+0.08 with hyperthreading): the attacks on the king zone lost in every direction, the pawn
breaks were neutral (+0.19 ± 5.74 over 3,724 games: the search already finds them), less reduction with disagreement
inside the network lost (−4.94 ± 6.16), and captures into another expert together with more reduction on disagreement
lean positive but small (+0.75 ± 3.25 over 11,652), to be closed later. In block 4 the extra plies of the singular
extension became a ladder of our own, a first rung and a step with its own weights, at the same tree (bench 209655);
its new degrees of freedom and a smaller negative extension did not gain and stay off
([§4.4](docs/development/8.1/SEARCH_4_EXTENSIONS.md)). Block 4 is now ours except three generic gates.

**Blocks 2 and 3 in our form (20 October 2026).** The reduction formula in use (block 2) and every pruning rule (block
3) are rewritten in a form of our own at the same tree (bench 209655). Block 3 also has a data build that records
which rule decided and what a search of the pruned move would have found; the levers per expert it suggested, and a
28-parameter SPSA of the pruning margins, did not gain (the SPSA vector: −2.83 ± 4.35 over 6,130 games), so the values
stay as they are ([§3.3](docs/development/8.1/SEARCH_3_PRUNING.md)). The search code is now ours apart from the base
formulas of time management.

**Time management in our form (20 October 2026).** The two stages of the time manager are rewritten: a budget for
the move (a reserve spread over a horizon of moves, a share of it as the target, a multiple of the target as the
ceiling) and a verdict after each iteration (the target scaled by the trend of the score, how long the best move has
held, how often it changed and how much of the effort it takes). Every remaining constant is a parameter. A test
harness runs the previous and the new code side by side on 5,000,000 random cases (clocks, increments, moves to go,
plies, overhead, and the existing parameters at random values): the target, the ceiling and the time of each
iteration agree to the bit. With this, the whole search is code of our own.
