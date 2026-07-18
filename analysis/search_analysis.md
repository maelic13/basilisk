# Basilisk search analysis

Status: working analysis, intended to be extended  
Analysis date: 2026-07-13  
Basilisk revision: `d4dd504` (`development`)  
Comparison baseline: Stockfish 18-era development, Reckless 0.9-era development, PlentyChess 7-era development, and the mid-2026 CCRL field

## Executive summary

Basilisk is not losing only because its search constants are less optimized. The larger search difference is architectural: the strongest engines pass substantially more information between transposition-table semantics, move ordering, history learning, pruning, late-move reductions, root search, time management, and parallel search. Basilisk implements most of the recognizable mechanisms, but many remain isolated from one another or use less context. Consequently, the engine spends too many nodes in some low-value subtrees and learns too little from completed searches.

The most important pure-search issue is the combination of:

1. an unconditional extension whenever the side to move is in check;
2. an exemption that prevents all checking moves from being late-move reduced;
3. conservative LMR that reduces only quiet moves and bad captures, starting from the third searched move; and
4. shallow-pruning decisions made before the full, history-aware reduction is known.

This makes forcing lines disproportionately expensive. Current top engines do not treat checks, captures, PV ancestry, history, and pruning as independent binary exceptions. They calculate a contextual estimate of move confidence and prospective search depth, then reuse it throughout the move loop.

Other important gaps are sparse history learning, the absence of a persistent TT-PV bit, a root search that discards information about all non-best moves, highly duplicative Lazy SMP, weak qsearch evasion handling, incomplete repetition/rule-50 handling, and correction-history updates that can learn from unsuitable tactical outcomes.

The overall distance to the strongest engines is unlikely to be pure search. Basilisk's planned NNUE work remains the largest potential single gain. As a working prior, structural search work may contain roughly 50-120 non-additive Elo at one thread, with a potentially larger benefit under multithreaded long-time-control conditions. This estimate is deliberately broad and must be established by SPRT rather than treated as a forecast.

## Evidence classifications

The report uses the following distinctions:

| Classification | Meaning |
|---|---|
| Verified defect | A reproducible correctness error or an unambiguous semantic problem |
| Measured behavior | Observed locally, but not automatically equivalent to Elo |
| Architectural gap | A concrete implementation difference from current leading engines |
| Strength hypothesis | A plausible Elo improvement that still requires controlled match testing |

Fixed-depth node counts are useful for understanding tree shape, but a reduction in nodes is not itself proof of strength. Conversely, a change that searches more nodes can be stronger if it searches tactically relevant nodes. All structural candidates should therefore pass correctness tests and then be decided by SPRT.

## Mid-2026 comparison baseline

The [CCRL 40/15 list](https://computerchess.org.uk/4040/) in mid-2026 places Stockfish 18 at approximately 3651, with Reckless 0.9, PlentyChess 7, Torch v4d, and Obsidian 16 close behind. Exact ratings move as games accumulate, and cross-list rating values are not directly comparable. The useful conclusion is that several modern engines now cluster near the top while sharing many search-design principles despite independent implementations.

The open-source comparison in this analysis uses exact snapshots where possible:

| Engine | Compared revision | Relevant source |
|---|---|---|
| Stockfish | `9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5` | [search.cpp](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/search.cpp), [tt.cpp](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/tt.cpp), [thread.cpp](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/thread.cpp) |
| Reckless | `dd0e676007f2e53e1bc59054b24b6ca9003d9ca2` | [search.rs](https://github.com/codedeliveryservice/Reckless/blob/dd0e676007f2e53e1bc59054b24b6ca9003d9ca2/src/search.rs), [history.rs](https://github.com/codedeliveryservice/Reckless/blob/dd0e676007f2e53e1bc59054b24b6ca9003d9ca2/src/history.rs) |
| PlentyChess | `04e07a98ee6ac104c30e7374450c94b96d94ef4d` | [search.cpp](https://github.com/Yoshie2000/PlentyChess/blob/04e07a98ee6ac104c30e7374450c94b96d94ef4d/src/search.cpp), [history.h](https://github.com/Yoshie2000/PlentyChess/blob/04e07a98ee6ac104c30e7374450c94b96d94ef4d/src/history.h) |

Torch is closed-source, so no detailed implementation claims are made about it. The comparison is not an assertion that every leader uses every Stockfish mechanism. It identifies designs that recur across multiple independently strong engines.

## Priority overview

| Priority | Area | Basilisk limitation | Expected importance |
|---:|---|---|---|
| 1 | Check extension and LMR | All in-check nodes are extended; checking moves are not reduced | High |
| 2 | Coupled selectivity | Pruning and LMR do not share one contextual depth/confidence estimate | High |
| 3 | History learning | Updates are sparse and context-poor | High |
| 4 | TT semantics | No persistent PV bit; lower entry density; weak rule-50 safeguards | Medium-high |
| 5 | Root search | Only the prior best move is retained meaningfully | Medium-high |
| 6 | SMP | Threads duplicate large portions of the same tree; weak result merge | High at multiple threads |
| 7 | Qsearch | Poor evasion ordering, no in-check TT store, unsafe tactical cap | Medium, plus correctness risk |
| 8 | Correction history | Asymmetric and tactically contaminated updates | Medium |
| 9 | Repetition | No upcoming-repetition detection | Low-medium |
| 10 | Experiment gate | Search-shape canaries prevent valid candidates from reaching SPRT | High development-process impact |

## 1. Unconditional check extension and overprotected checking moves

### Basilisk behavior

At [`src/search.cpp:1218`](../src/search.cpp#L1218), Basilisk increments search depth whenever the side to move is in check. This is a blanket check extension rather than a selective extension based on move singularity, tactical context, TT evidence, or search result.

Later, the LMR condition at [`src/search.cpp:1574`](../src/search.cpp#L1574) excludes every move that gives check. The shallow-pruning region beginning near [`src/search.cpp:1430`](../src/search.cpp#L1430) also exempts checking moves from multiple pruning decisions.

The mechanisms compound:

```text
checking move searched at full depth
    -> child is in check and receives +1 depth
        -> every evasion is searched at full depth
```

Checking sequences are important, but most legal checks in a typical move list are not best. Sacrificial checks with poor SEE are particularly likely to receive more work than their evidence warrants.

### Current top-engine behavior

Current Stockfish has no blanket check extension. Its move loop can reduce a late checking move, reduce a capture, or prune a bad checking move using SEE. Check status remains an important feature, but it is one input to the reduction calculation rather than an unconditional exception. See Stockfish's [move pruning and LMR logic](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/search.cpp#L1140-L1392).

Reckless and PlentyChess similarly integrate check/capture status into contextual reductions rather than protecting the complete category from reduction.

### Likely effect

This is the leading explanation for wasted nodes in forcing positions and for expensive benchmark outliers. It also distorts effective depth: nominal depth becomes less comparable across quiet and forcing positions.

### Recommended experiment

Remove the unconditional check extension as an isolated candidate after correcting qsearch-in-check termination and terminal draw handling. Do not initially compensate by changing several unrelated margins. Record:

- total nodes;
- nodes entered in check;
- average and maximum check-chain length;
- checking moves reduced, pruned by SEE, and re-searched;
- tactical-suite result changes; and
- SPRT result.

If full removal is too tactically fragile, a later candidate can reintroduce a narrowly justified extension, for example from singularity or a sufficiently strong reduced-search result. The default should not be to extend all checks again.

## 2. LMR and pruning are not driven by one confidence calculation

### Current Basilisk structure

Basilisk uses a preliminary LMR-depth approximation for shallow pruning, then calculates the real reduction later in the move loop at [`src/search.cpp:1584`](../src/search.cpp#L1584). The real reduction starts from a logarithmic table and adds a limited set of adjustments.

Several intended adjustments are zero in [`src/SearchParams.h:94`](../src/SearchParams.h#L94), including cut-node, TT-PV, not-improving, and TT-capture terms. Post-LMR deeper/shallower adjustments and post-LMR history are also inert.

The resulting behavior has several structural restrictions:

- Reduction starts only from the third searched move.
- Only quiet moves and SEE-negative captures are eligible.
- All good captures are searched at full depth.
- All checks are searched at full depth.
- Reduction is floored to a non-negative integral value, so exceptional late moves cannot receive a negative reduction/extension.
- Pruning uses the original depth or a cheap base estimate rather than the final contextual prospective depth.
- A reduced move that needs verification is normally re-searched at the unchanged full depth rather than a result-dependent depth.

### Why this matters

Move ordering, pruning, and LMR are all attempts to estimate the same latent property: how likely is this move to matter, and how much search is justified before rejecting it? If they use different estimates, contradictory decisions arise. A move can be considered low-value enough for history pruning but then receive an unexpectedly large full search, or be protected from pruning by a broad category even though TT and history evidence are strongly negative.

### Top-engine model

Current leading engines form a richer reduction using combinations of:

- PV/non-PV and cut-node status;
- stored TT-PV status, TT depth, bound and TT move class;
- improving and opponent-worsening signals;
- quiet/capture/check state;
- quiet and capture history;
- continuation history;
- correction-history magnitude;
- root score movement and aspiration uncertainty;
- move count and prior reduction;
- next-ply cutoff counts; and
- sometimes per-thread randomization.

They then use the prospective reduced depth for futility pruning, history pruning and SEE thresholds before conducting the reduced search. Reduction can be negative in a sufficiently promising context. The result of the reduced search can also cause a slightly deeper or shallower verification search.

Examples are available in [Reckless search.rs](https://github.com/codedeliveryservice/Reckless/blob/dd0e676007f2e53e1bc59054b24b6ca9003d9ca2/src/search.rs#L504-L959) and [PlentyChess search.cpp](https://github.com/Yoshie2000/PlentyChess/blob/04e07a98ee6ac104c30e7374450c94b96d94ef4d/src/search.cpp#L783-L1147).

### Recommended redesign

Refactor the move loop so it follows this conceptual order:

1. Gather move and node context.
2. Calculate a single signed reduction `r` for every move after the first.
3. Derive `lmrDepth = newDepth - r`, with safe bounds.
4. Use that same `lmrDepth` for futility, history and SEE pruning.
5. Conduct the reduced search when `r > 0`; allow a controlled extension when `r < 0`.
6. Select verification depth using the reduced result and node confidence.
7. Update history based on both reduced and verification outcomes.

This should be introduced in stages. A single large rewrite followed by an 18-dimensional tune will make attribution difficult.

## 3. History learning is sparse and has insufficient context

### Update coverage

The main history update is called on beta cutoff at [`src/search.cpp:1675`](../src/search.cpp#L1675). Important outcomes are not fully trained:

- An exact/PV best move that improves alpha but remains below beta does not receive the same move-history learning.
- A quiet TT move causing an immediate TT cutoff returns before the regular cutoff update.
- Fail-low nodes do not provide sufficiently rich countermove or refutation learning.
- Static-evaluation differences between consecutive positions are not converted into history evidence.
- Capture maluses are concentrated on SEE-negative captures; an apparently sound capture that searches badly may not receive the appropriate negative update.

### Table context

Basilisk's main history is indexed by color, origin, and destination. Continuation history uses previous piece/destination to current piece/destination at selected distances. Capture history uses attacker type, destination and captured type.

Current top engines add context such as:

- whether the origin square was threatened;
- whether the destination square is threatened;
- whether the previous move was searched in check;
- whether the previous or current move was a capture;
- multiple continuation distances; and
- target-threat context for noisy moves.

Reckless's concrete table organization is visible in [history.rs](https://github.com/codedeliveryservice/Reckless/blob/dd0e676007f2e53e1bc59054b24b6ca9003d9ca2/src/history.rs#L82-L251), while PlentyChess has comparable contextual dimensions in [history.h](https://github.com/Yoshie2000/PlentyChess/blob/04e07a98ee6ac104c30e7374450c94b96d94ef4d/src/history.h#L55-L93).

### Killers and countermoves

Basilisk gives killers and countermoves fixed ordering scores above normal history. The countermove table is not aged with the other history tables in the aging path near [`src/search.cpp:639`](../src/search.cpp#L639). A stale categorical countermove can therefore outrank newer statistical evidence indefinitely.

This does not prove that killers or countermoves must be removed—PlentyChess still uses countermove concepts—but their categorical priority should be tested against a model where contextual history determines their value.

### Recommended work

1. Add quiet-TT-cutoff rewards and previous-quiet maluses.
2. Train the best move at suitable exact nodes, not only cutoffs.
3. Add fail-low refutation/countermove updates.
4. Add static-evaluation-difference history.
5. Penalize searched captures that fail, not only SEE-negative captures.
6. Add threat context before adding many more continuation distances.
7. Age or replace the fixed countermove layer.

The benefit is not only better ordering. The same histories must become inputs to pruning and LMR; otherwise their value remains partially isolated.

## 4. Transposition-table semantics and density

### Missing persistent PV information

The Basilisk TT flag byte contains bound and age but no PV bit at [`src/tt.h:14`](../src/tt.h#L14). Search reconstructs `tt_pv` only from an exact entry of sufficient depth at [`src/search.cpp:1256`](../src/search.cpp#L1256).

This is weaker than preserving PV ancestry through the search graph. A bound entry can still originate from a PV subtree and should continue to influence reduction and pruning confidence. Current Stockfish explicitly stores a PV flag in its [TT entry](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/tt.cpp#L43-L61).

Persistent TT-PV status is used by modern engines for LMR, reverse futility pruning, singular-extension conditions, history selection and replacement policy. Adding the bit is a structural prerequisite for faithfully implementing those decisions.

### Entry density

Basilisk stores three entries in a 64-byte cluster at [`src/tt.h:36`](../src/tt.h#L36). Stockfish stores three entries in 32 bytes in its [cluster layout](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/tt.cpp#L150-L161).

At equal configured hash size, Basilisk therefore holds approximately half as many entries. Its use of full 64-bit collision protection and atomics is a robustness/concurrency tradeoff rather than a simple error, but it costs cache coverage and bandwidth. The likely impact grows with search duration and thread count.

This should be tested independently from the PV-bit change. A denser entry may use a partial key while retaining safe lock-free publication semantics.

### TT cutoffs and rule 50

Basilisk's cutoff compatibility primarily checks depth, bound and non-PV status. Modern engines add contextual restrictions such as cut-node compatibility and suppression near the rule-50 boundary. Basilisk's TT score conversion clamps only after the halfmove clock reaches 100 at [`src/tt.h:179`](../src/tt.h#L179), leaving positions near the boundary vulnerable to graph-history mismatch.

Recommended changes:

- Store and propagate TT-PV status.
- Reward/penalize histories on eligible TT cutoffs.
- Add a conservative rule-50 cutoff guard near the boundary, then tune its threshold.
- Test a denser cluster layout separately.

## 5. Root search discards information

`RootMoveTable::update` stores essentially the completed iteration's best move at [`src/search.cpp:84`](../src/search.cpp#L84), and it is updated once per completed iteration at [`src/search.cpp:1865`](../src/search.cpp#L1865). Other root moves do not retain a comparable persistent score/PV history.

If the former best move fails low or another move becomes competitive, ordering falls back too heavily on generic move history. The time manager's effort estimate is also centered on the last best move instead of the full root work distribution.

Current top engines keep a root object per legal move containing some combination of:

- score and previous score;
- running mean and variance;
- bound/completion state;
- full principal variation;
- selective depth;
- nodes or effort spent; and
- thread-local search state.

They sort the root list after each root move and build aspiration windows from per-move score uncertainty. Stockfish's implementation is in its [iterative deepening and aspiration loop](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/search.cpp#L346-L492).

### Recommended root representation

Each root move should retain at least:

```text
move
currentScore
previousScore
averageScore
scoreVariance
bound/completed
selDepth
nodes
PV
```

This supports better root ordering, per-move aspiration, time allocation, MultiPV behavior and SMP result merging. Root-state work should precede sophisticated time-management tuning because it supplies the missing observations.

## 6. Lazy SMP has limited diversity and a weak merge policy

### Current behavior

Basilisk helper threads start at slightly different initial depths, but otherwise run nearly the same root search with a shared TT and similar root ordering. When the main thread completes, it sets the global stop condition and terminates helpers at [`src/search.cpp:2133`](../src/search.cpp#L2133).

Result merging chooses primarily by completed depth and score. It does not estimate confidence from agreement among threads or from the amount of independent root evidence.

Histories are blended from helpers into the main history, but helper histories retain their existing state. Repeated blending can therefore overweight old helper evidence unless aging and ownership are carefully controlled.

### Local scaling smoke test

The start position searched to depth 17 produced:

| Threads | Aggregate nodes | Wall time | Score | Best move |
|---:|---:|---:|---:|---|
| 1 | 658,707 | 886 ms | +0.66 | `d2d4` |
| 8 | 3,770,848 | 594 ms | +0.58 | `e2e4` |

This is a 1.49x wall-clock speedup while consuming 5.72x the aggregate nodes. It is one position on one machine, so it is not a general scaling result. It is nevertheless consistent with substantial duplicate work and justifies a proper scaling suite.

### Top-engine behavior

Modern Lazy SMP implementations introduce diversity through different aspiration states, reduction jitter, depth schedules, root ordering, or thread-local statistics. Their merge policy considers multiple completed thread results. Stockfish performs score-weighted best-thread voting in [thread.cpp](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/thread.cpp#L350-L400). Reckless also uses soft-stop votes and per-thread root vectors.

### Recommended sequence

1. Build persistent per-thread root-move state.
2. Add measured diversity in reduction, aspiration or root ordering.
3. Add score/depth-weighted voting rather than selecting one result mechanically.
4. Introduce a soft-stop consensus before forcing all helpers to stop.
5. Measure at 1, 2, 4, 8 and 16 threads over a varied position set and multiple time controls.

SMP changes should be tested separately at one thread and at the target thread counts. An improvement may be neutral at one thread and important at eight threads.

## 7. Quiescence search: evasions, bounds and tactical cap

### In-check qsearch

At [`src/search.cpp:1021`](../src/search.cpp#L1021), the in-check qsearch path generates all legal evasions without normal TT-move or history ordering. The completed in-check result is not stored in TT. This is both slower and less informative for repeated tactical subtrees.

Basilisk also has a separate qsearch depth cap of ten plies. At the cap it can return static evaluation even while the side to move is in check. That value is not a valid evaluation of the forcing position and can produce an incorrect bound in a long check/evasion sequence.

Current Stockfish uses its staged MovePicker for captures/evasions, searches to the general maximum ply rather than a separate ten-ply tactical cap, and stores the resulting bound. See [Stockfish qsearch](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/search.cpp#L1619-L1845).

### Non-check qsearch

Basilisk's delta pruning can return the existing alpha after discarding moves without preserving a better futility-derived `bestValue`. Modern engines retain a more accurate fail-soft value and often smooth large cutoff returns toward beta. Better bounds matter because Basilisk has already measured a meaningful benefit from TT-bound-informed static evaluation.

The development comments proposing quiet qsearch checks are based on an older comparison. Current Stockfish explicitly restricts normal qsearch to captures and evasions. Quiet qchecks should not be a priority without independent evidence.

### Recommended changes

- Remove the unsafe independent qsearch cap, retaining the normal maximum-ply safeguard.
- Order evasions with TT move and contextual history.
- Store in-check qsearch results in TT where semantically safe.
- Preserve a fail-soft `bestValue` through delta/SEE pruning.
- Test bound smoothing separately from move-generation changes.

## 8. Correction-history semantics

Basilisk's corrected evaluation is the equal average of pawn, minor-piece, own non-pawn, opponent non-pawn, and one-ply continuation corrections near [`src/search.cpp:624`](../src/search.cpp#L624). Equal weighting is simple, but it assumes the components have equal reliability and scale.

The update near [`src/search.cpp:1708`](../src/search.cpp#L1708) is more concerning:

- Updates occur for exact/fail-high outcomes, but not symmetrically for appropriate fail-low outcomes.
- Capture outcomes can update correction history, contaminating positional evaluation correction with tactical effects.
- Only one-ply continuation correction is represented; current leaders also exploit deeper continuation contexts.

Current Stockfish protects correction history from unsuitable tactical updates and learns only when the search result's direction is informative relative to static evaluation. Its current correction update and surrounding history logic are visible in [search.cpp](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/search.cpp#L1521-L1604).

Recommended work:

1. Exclude tactical best captures from positional correction updates.
2. Update only when the bound direction makes the error informative.
3. Add two- and four-ply continuation correction contexts.
4. Learn or separately tune component weights.
5. Feed correction magnitude into selectivity only after the correction signal itself is trustworthy.

## 9. Repetition and graph-history handling

Basilisk detects repetition only after reaching a currently repeated position by scanning board history in [`src/Board.cpp:1636`](../src/Board.cpp#L1636). It does not recognize that a legal move can immediately force a repetition before searching that move.

Stockfish, Reckless, PlentyChess and Obsidian use an upcoming-repetition mechanism based on reversible move-key deltas, commonly implemented with a cuckoo table. An example is [Reckless board.rs](https://github.com/codedeliveryservice/Reckless/blob/dd0e676007f2e53e1bc59054b24b6ca9003d9ca2/src/board.rs#L292-L335).

Upcoming-repetition detection allows alpha to be raised or a subtree to be cut off without reaching the repeated position. It also improves repetition avoidance/selection when combined with a small node-dependent draw score. Basilisk currently uses an exact zero draw value, which is simple but can make repeated positions indistinguishable in situations where avoiding or seeking a repetition is strategically useful.

This is unlikely to be the largest Elo source, but it is a mature mechanism present across multiple top engines and is relatively self-contained.

## 10. Verified rule-50/checkmate defect

### Reproduction

Basilisk checks `is_draw()` before it establishes in-check and legal-terminal status in both the main and quiescence search paths. [`src/Board.cpp:1659`](../src/Board.cpp#L1659) returns draw immediately when the halfmove clock reaches 100, without preserving checkmate precedence.

The defect was reproduced with:

```text
7k/5K2/6Q1/8/8/8/8/8 w - - 99 50
```

At depth 3, Basilisk returned `0.00` and played `g6h5`, missing a legal mate in one. The mating move increments the reversible-move clock, and the child is incorrectly declared drawn before checkmate is recognized.

Stockfish's [`Position::is_draw`](https://github.com/official-stockfish/Stockfish/blob/9a8dd81dd7f98cbf02f16c59b4377d174d6eb4b5/src/position.cpp#L1516-L1523) preserves mate by requiring either that the side is not in check or that a legal move exists before declaring a rule-50 draw.

### Required correction

Terminal checkmate must take precedence over an automatic rule-50 draw. The fix should cover:

- main search;
- qsearch;
- root terminal handling;
- direct board API tests; and
- FEN tests at halfmove clocks 99, 100 and greater than 100.

This is likely negligible in Elo due to rarity, but it is a mandatory correctness fix.

## 11. Bound shaping and fail-soft stability

Basilisk reverse futility pruning returns the raw corrected evaluation when the margin proves a cutoff. Qsearch stand-pat cutoffs similarly return the full stand-pat value. A large fail-soft overshoot can enter TT and influence later pruning or aspiration more strongly than warranted by the shallow proof.

Current Stockfish often blends the returned value toward beta for reverse-futility and qsearch cutoffs. Reckless also uses bounded interpolation in selected search paths. This does not change the fact of the cutoff; it produces a more conservative estimate of how far above beta the node lies.

Basilisk's own measured gain from TT-bound-informed static evaluation suggests that bound quality is important. Bound smoothing is therefore a credible candidate, but should be tested separately because overly aggressive smoothing can discard useful score information.

## 12. ProbCut and null-move verification

### ProbCut

Basilisk ProbCut generates legal captures and searches them without the complete staged capture ordering used by the main move picker. Modern implementations typically prioritize the TT move, capture history and SEE threshold, and can skip ProbCut when existing TT evidence already contradicts the probabilistic cutoff. Their margins and depth reduction may also depend on improving status and expected result.

The current implementation is functional but first-generation: it spends more effort finding the proving capture and uses less context when deciding whether ProbCut is appropriate.

### Null move

Basilisk's null-move verification disables null move at the immediate verification node, but descendants can again use null move. Current Stockfish uses a minimum-ply region during deep verification to prevent recursive null-move cutoffs throughout the verification subtree.

This is most relevant to zugzwang resistance and very deep searches. It should follow the larger selectivity work rather than precede it.

## 13. Internal iterative reduction and node type

Basilisk applies internal iterative reduction broadly to non-PV nodes when no TT move is available. Current Stockfish's conditions are more closely tied to PV/cut-node semantics. PlentyChess differs, so there is no single universal rule to copy.

This should be audited after adding persistent TT-PV status and stronger cut-node coupling. Without those signals, an IIR comparison is confounded by missing node confidence.

## 14. Search regression gates are blocking structural progress

[`PLAN.md:73`](../PLAN.md#L73) treats fixed-depth KBNK/KQK outcomes as non-negotiable canaries before a candidate can reach SPRT. The corresponding tests use fixed search depth and mating-ply expectations. Search parameters contain multiple comments indicating that modern mechanisms were left inert because these trajectories changed.

Those tests mix two different purposes:

1. correctness: the engine must make a legal move, preserve a theoretical win, recognize mate, and not produce a false draw;
2. strength/trajectory: the search must choose a particular winning route and finish within a specific number of plies at a fixed depth.

The first category is a valid hard gate. The second is not: a search-shape change can choose a different legal winning path, fail the canary, and still gain Elo. In that case the current process rejects the candidate before the strength test capable of answering the real question.

Recommended test policy:

| Test type | Hard gate? | Purpose |
|---|---:|---|
| Perft, legality, make/unmake, terminal result | Yes | Correctness |
| Rule-50/checkmate and repetition edge cases | Yes | Correctness |
| Deterministic bench signature when expected | Yes, with deliberate update process | Reproducibility |
| KBNK/KQK remains won under generous limits | Yes | Gross regression protection |
| Exact fixed-depth best move or mating route | No; diagnostic | Tree-shape observation |
| Elo/SPRT | Yes for strength claims | Statistical strength decision |

Stockfish's development advantage is not only its algorithms. Search candidates are subjected to distributed game testing through Fishtest, allowing structural hypotheses to be judged by outcomes across enormous samples rather than by one expected endgame trajectory. The [Stockfish release history](https://github.com/official-stockfish/Stockfish/releases) describes the scale and cumulative nature of this process.

## Benchmark observations

### Deterministic one-thread bench

The local deterministic bench produced:

| Metric | Result |
|---|---:|
| Total nodes | 12,661,251 |
| Geometric-mean effective branching factor | 2.829 |
| Median nodes per position | 253,845 |
| Maximum nodes in one position | 1,737,984 |
| Largest-position share | 13.7% |

The concentration of nodes in a small number of positions is compatible with the check-extension/LMR concern, but this aggregate alone does not prove causation. Search telemetry should identify whether the outliers contain extended check chains, full-depth evasions, excessive LMR verification, or TT misses.

The built-in bench command forces one thread, so it cannot be used directly as an SMP benchmark without changing the harness or using separate UCI searches.

### Required future measurement

A useful diagnostic build should count at least:

- nodes by PV, cut and all node;
- nodes entered in check;
- checking moves searched, reduced and SEE-pruned;
- LMR attempts, reduced cutoffs and full-depth re-searches;
- LMR re-search successes and failures by reduction size;
- each pruning mechanism's attempts and accepted cutoffs;
- TT probes, hits, cutoffs, bound types, PV-bit hits and replacements;
- root nodes per move and per thread;
- qsearch capture/evasion nodes and maximum qsearch depth;
- null-move attempts, cutoffs and verified failures;
- ProbCut attempts and cutoffs; and
- repetition/upcoming-repetition cutoffs.

Counters should be compile-time or runtime disabled for production matches. Compare candidates at both fixed node budgets and fixed time; fixed-depth results alone are too sensitive to the very tree-shape changes being evaluated.

## Elo hypotheses

The following estimates are broad priors, not additive promises:

| Area | Working prior | Qualification |
|---|---:|---|
| NNUE/evaluation | 150-300+ Elo | Highly dependent on network quality, feature set and inference speed |
| Check extension plus coupled all-move LMR/pruning | 15-40 | Highest-confidence pure-search target |
| Contextual history and broader learning paths | 10-30 | Depends on feeding history back into selectivity |
| TT-PV semantics, cutoff feedback and density | 8-25 | Density likely matters more at LTC/SMP |
| Persistent root state and aspiration | 5-15 | Also enables better time management and SMP |
| SMP diversity and voting | 0 at 1T; 10-30+ at 8T/LTC | Must be tested at target thread count |
| Qsearch, corrections and repetition combined | 5-20 | Multiple separate candidates |
| Rule-50/checkmate fix | Negligible Elo | Mandatory correctness |

These values overlap heavily. For example, richer history makes contextual LMR better; TT-PV enables better LMR; root state enables better SMP voting. Summing the rows would double-count shared effects.

## Recommended implementation roadmap

### Phase 0: correctness and observability

1. Fix rule-50/checkmate precedence.
2. Remove unsafe in-check static returns at the qsearch cap.
3. Add targeted regression tests for terminal and graph-history cases.
4. Add search telemetry behind a disabled-by-default flag.
5. Establish mixed tactical, quiet, middlegame and endgame node-budget suites.
6. Establish 1/2/4/8/16-thread scaling measurements.
7. Convert exact endgame-trajectory gates into diagnostics while retaining correctness gates.

### Phase 1: core tree selectivity

1. Test removal of blanket check extension.
2. Refactor the move loop around one signed contextual reduction.
3. Use the derived `lmrDepth` consistently for futility, history and SEE pruning.
4. Permit contextual reduction of checks and good captures.
5. Start reductions from the second late move where appropriate.
6. Add result-dependent deeper/shallower verification.
7. Activate post-LMR learning only after the search semantics are stable.

### Phase 2: information retention and learning

1. Add a persistent TT-PV bit.
2. Add rule-50-aware TT-cutoff restrictions.
3. Train histories on TT cutoffs, exact nodes, fail-lows and static-eval differences.
4. Add threat and in-check/capture context to histories.
5. Correct correction-history update conditions and add deeper continuation correction.
6. Replace the one-best-move root table with persistent per-root-move state.
7. Add per-root uncertainty-aware aspiration.

### Phase 3: tactical and graph refinements

1. Improve qsearch evasion ordering and TT storage.
2. Preserve accurate fail-soft values through qsearch pruning.
3. Add upcoming-repetition detection.
4. Improve ProbCut capture staging and contextual conditions.
5. Add a deeper null-move verification exclusion region if testing supports it.
6. Test bound smoothing independently.

### Phase 4: SMP

1. Give every thread persistent root-move state.
2. Introduce controlled search diversity.
3. Add score/depth-weighted voting and soft-stop consensus.
4. Review history ownership and blending.
5. Retest scaling and Elo at every supported thread count.

### NNUE interaction

NNUE should proceed first or on a parallel branch because evaluation is probably the largest overall gap. However, not every search improvement needs to wait for the final network:

- Terminal correctness, TT-PV semantics, root state, repetition handling and SMP architecture are largely evaluation-independent.
- Futility margins, reverse-futility margins, ProbCut margins, correction weights and some LMR evaluation terms should be retuned after the intended network is stable.
- Large multidimensional tuning should follow structural changes, not substitute for them.

SPSA can optimize constants inside an existing architecture. It cannot invent a missing PV bit, a contextual history dimension, persistent root state, upcoming-repetition detection, or information sharing between pruning and LMR.

## Immediate candidate sequence

For clear attribution, the first experimental sequence should be:

1. correctness-only rule-50/checkmate fix;
2. correctness-only qsearch-in-check termination fix;
3. telemetry-only build;
4. blanket check-extension removal;
5. checking-move LMR using the existing model;
6. all-move LMR from the second move;
7. unified prospective `lmrDepth` for pruning;
8. TT-PV bit;
9. TT-cutoff history learning;
10. persistent root-move state.

Each item should have a baseline commit, deterministic correctness result, fixed-node telemetry comparison, and SPRT result. Failed candidates should be retained in an experiment log with conditions and confidence bounds so later architectural changes can justify retesting them.

## Open questions for continued analysis

The following areas should be expanded when additional findings or match data are available:

- Per-position attribution of the deterministic bench outliers.
- Search telemetry before and after removal of check extension.
- Exact history saturation and aging behavior over long games.
- TT replacement behavior and collision/density measurements under SMP.
- Root aspiration failure frequency and cost.
- Thread overlap measured by root move and TT key sampling.
- Evaluation/search interaction after the target NNUE is integrated.
- SPRT results for previously inert mechanisms after relaxing trajectory gates.
- Time-management behavior under sudden score changes and only-move positions.

This document should remain a working engineering record. Confirmed observations, failed experiments and measured match outcomes should be added alongside the original hypotheses rather than replacing them.
