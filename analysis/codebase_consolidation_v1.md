# A.6 — Codebase consolidation map

- State / class: CLOSED (analysis delivered), `R2`
- Owner / date: A.6, 2026-10-01
- Source revision: `2e7914e`
- Decision needed: the behaviour-neutral file moves and ownership seams for
  B.1 and C.1; this leaf changes no engine source.

**Recommendation.** Use the move tables below, but let B.0 freeze the final
B.1 handoff before implementation; the current monoliths mix public inputs,
shared engine resources, per-thread memory and per-search state, and moving
code without first separating those lifetimes would preserve the file size
problem under new names. The main counter-argument is extra indirection and
translation-unit boundaries on hot paths, so the tables keep the recursive
worker and evaluation accumulator cohesive and require the existing pooled-PGO
NPS floor.

## Scope and evidence

This is an architecture inventory, not a playing mechanism. It therefore has
no Elo prediction or game gate. The falsifier for a proposed move is any change
to the development fingerprint, a domain regression, a Texel reconstruction
failure, or a pooled-PGO slowdown outside measurement noise. Such a result
means the extraction changed semantics or put a costly boundary on a hot path;
the move is returned rather than explained away.

Physical line counts at the source revision, measured with
`[IO.File]::ReadAllLines(...).Count`, are:

| File | PLAN claim | Actual | Finding |
|---|---:|---:|---|
| `src/search.cpp` | 2,848 | **3,125** | The roadmap count predates the A.5 diagnostics and ablation work. |
| `src/eval.cpp` | 2,148 | **2,148** | Matches. |
| `src/search.h` | — | 606 | Most lifetime mixing is visible here, not only in the `.cpp`. |
| `src/eval.h` | — | 176 | Public API, cache storage and tuner-shared predicates are mixed. |

The search source already has reliable coarse boundaries: root-table and
result sanitation (75–231), construction/time/node control (232–480), history
policy (481–680), move ordering (681–963), diagnostics (964–1342), root output
and tablebases (1343–1446), qsearch (1447–1686), negamax (1687–2530), iterative
deepening (2531–2823), and the persistent thread pool (2824–3125).

The evaluator has three existing top-level boundaries: endgame knowledge and
scaling (147–918), pawn-cache evaluation (946–1110), and the main evaluation
(1111–2082), followed by tune-only file I/O. Inside the main evaluation the
lazy checkpoint at 1270 divides the cheap prefix from the attack-map-driven
tail.

## Search ownership and interactions

Today `Searcher` owns four different lifetime classes:

| Lifetime | Current members / producers | Consumers and reset |
|---|---|---|
| Engine shared | `tt_`, `stop_`, `ponderhit_`, `info_cb_`; pool-created shared node/TB counters and `RootMoveTable` pointers travel through `SearchLimits` | Every worker; pool creates and joins them per `go`; TT and stop outlive searches. |
| Per thread, persistent | `Evaluator evaluator_`, `HistoryTables hist_`, history age, LMR storage | Recursive search and picker; histories age per search and clear on `ucinewgame`; evaluator pawn cache persists until clear. |
| Per search configuration | `active_limits_`, root filters, Syzygy root moves, `SearchParams`, timing and diagnostic switches | Read throughout qsearch, negamax and iterative deepening; assigned at `Searcher::search`. |
| Per search mutable state | board pointer, stack, PV and fixed move buffers, nodes/TB hits, time state, root effort, diagnostics and trace | Recursive search; reset at `Searcher::search`, harvested before its board dies. |

`SearchLimits` currently mixes caller-owned configuration with worker plumbing:
the user limits and `SearchParams` sit beside mutable shared-counter pointers,
the shared root-table pointer, worker identity and copied Syzygy vectors. That
is the central seam B.1 must repair. A configuration value should not become a
back door to engine-owned shared state.

The dependency-complete B.2 path is:

`TT probe/raw eval -> correction histories -> corrected/pruning eval ->`
`node pruning -> MovePicker/history scores -> move-loop pruning -> LMR and`
`re-search -> history/correction updates -> TT store`.

The following seams must remain adjacent or explicit when that path moves:

- `tt_store` is the sole search store wrapper and owns TT diagnostics; probe
  score conversion must retain rule-50 and mate-ply semantics.
- `do_move` / `undo_move` and their null variants remain the only search-side
  board mutation seam.
- `MovePicker` needs explicit read-only inputs (board, stack, TT move, history,
  root ordering), not unrestricted access to a whole worker.
- history storage remains in `HistoryTables`; update policy and every training
  call remain part of the selectivity cluster.
- the eight matched ablation bits keep their order: razoring, RFP, NMP,
  ProbCut, IIR, shallow move pruning, extensions, LMR.
- diagnostics observe decisions but never become inputs. `cutoff_count` has no
  producer yet, but is a reserved trace field for B.2 rather than dead schema.
- root aspiration and time management remain boundary consumers of search
  results; they do not move into node policy merely because they share the
  current class.

## B.1 move table

`search.h` remains the compatibility facade used by Engine, bench, WAC and
tests. Internal headers may be included by their owning `.cpp` files; public
callers should not acquire knowledge of worker internals.

| Ticket order | Target | Move from current source | Contract |
|---:|---|---|---|
| 1 | `search_types.h` | Score/depth constants, `SearchStack`, the caller-facing portion of `SearchLimits`, `SearchResult`, and `is_tablebase_decisive` | Pure value types; mate constants remain equal to `tt.h` and the existing static assertion. |
| 2 | `search_root.h/.cpp` | `RootMoveTable`, legal-result sanitation and root-result helpers | Locking stays outside per-node work; every published best/ponder move remains legality-checked. |
| 3 | `search_diagnostics.h/.cpp` | `DiagCounters`, decision-trace record/schema and formatting, including pool aggregation | Compile away hot-path writes in production exactly as now; A.5 JSON/text contracts and counter units do not change. |
| 4 | `search_history.cpp` | History bonus/malus, scoring, correction and training policy currently at 481–680 | `history.h/.cpp` continue to own storage/lifecycle; B.2 gets one policy boundary without layout churn. |
| 5 | `move_picker.h/.cpp` | Scoring helpers and staged picker currently at 681–963 | Fixed caller-owned buffers; exhaustive legal staging and TT-move validation preserved; no allocation. |
| 6 | `search_worker.h/.cpp` | One per-thread worker: evaluator/history plus fixed stack/PV/move buffers; qsearch, templated negamax, iterative deepening and root/TB helpers | Introduce `NodeType { Root, PV, NonPV }` as the compile-time replacement for `is_pv`/root inference only. Keep `allow_null` and `cut_node` semantics unchanged until B.0 explicitly owns them. |
| 7 | `search_thread_pool.h/.cpp` | Persistent worker lifecycle, dispatch, shared counters/root table and result merge at 2824–3125 | Engine-owned resources are passed through an explicit shared context, not `SearchLimits`; no new shared ownership or per-node atomics. |
| 8 | `search.h` | Compatibility includes and the stable `Searcher` / `SearchThreadPool` surface | Existing Engine, bench, WAC and test call sites remain source-compatible unless a focused test-only include becomes clearer. |

Inside ticket 6, use three explicit aggregates without heap ownership:

- `SearchConfig`: immutable caller limits, params, root restrictions and
  Syzygy policy for one `go`;
- `SearchShared`: references/pointers to TT, stop/ponder state, root table,
  shared counters and output callback, owned by Engine/pool;
- `SearchState`: board, stacks, PV/move buffers, counters, root-TB material,
  time and diagnostic state, owned by one worker for one search.

This is a lifetime partition, not license to copy large state into recursive
calls. Recursive functions take references/pointers to the worker state and
retain fixed arrays.

## Evaluation ownership and interactions

The evaluation order is semantically significant even though most additions
are integer additions: the lazy return occurs after material/PST, imbalance,
pawns, dynamic passers, bishop-pair and rook/knight terms. The full tail then
shares one attack substrate across mobility, threats, king safety, hanging and
several positional terms. Finally tapering, `apply_endgame`, rule-50 damping
and side-to-move conversion occur in that order.

The shared substrate is already conceptually present but exists only as local
arrays in `Evaluator::evaluate`: `attacked_by`, `attacked`, `attacked2`, king
zones, blockers, cached slider attacks and each side's mobility area. C.1 must
materialize that as one non-owning, stack-resident product. Family modules
consume it; none may recompute its own attack map or change occupancy,
pin/mobility-area, or pawn-double-attack semantics.

## C.1 move table

| Ticket order | Target | Move from current source | Contract |
|---:|---|---|---|
| 1 | `eval_types.h` | Small score accumulator/context types and cache result types | White-perspective MG/EG accumulation; no allocation or large by-value passage. |
| 2 | `eval_trace.h/.cpp` | `EvalTrace` access, `g_trace`, trace macros and `reconstruct` | Group/index/sign meaning stays byte-for-byte; release macros compile away. |
| 3 | `eval_tables.h/.cpp` | `g_eval_params`, PST/material runtime tables and `init_eval_tables` | One initialized table set; tune reload rebuilds the same tables. |
| 4 | `eval_endgames.h/.cpp` | KPK bitbase, KBNK/KXK, rook-ending scaling and `apply_endgame` | Preserve white perspective, mate-band ceiling, scale floor, promotion closure, rule-50 order and lazy-path operation without `passed[]`. |
| 5 | `eval_pawns.h/.cpp` | `PawnEntry`, pawn table and `eval_pawns` | Direct-mapped 16,384-entry cache, identical key/hit semantics; passed and pawn-attack outputs remain available to both prefixes/tails. |
| 6 | `eval_attacks.h/.cpp` | The single attack-map, blocker, slider-cache and mobility-area producer now at 1297–1446 | One stack-resident producer, exact occupancy and accumulation order, shared by mobility/threats/king safety. |
| 7 | Family modules | `eval_material.cpp`, `eval_mobility.cpp`, `eval_threats.cpp`, `eval_king.cpp`, `eval_pieces.cpp`, `eval_passers.cpp` from the corresponding ordered blocks | Each mutates one accumulator by reference and emits the same trace features in the same order; shared inputs are read-only. |
| 8 | `eval.cpp` / `eval.h` | Coordinator, lazy checkpoint, taper/finalization, public `Evaluator`, and tune-only load/dump surface | Cheap prefix and full-tail boundary unchanged; side-to-move conversion happens once at return. |

CMake must hold one `EVAL_SOURCES` list used by both `basilisk_engine` and the
special `basilisk-texel` target. The latter recompiles **every** evaluation
translation unit with `TEXEL_TRACE` and `BASILISK_TUNE`; linking ordinary
release objects into it would silently drop features and invalidate
reconstruction.

## Dead, inert and deliberately retained code

Mechanical use-site searches at `2e7914e` give this disposition:

| Item | Evidence | Disposition |
|---|---|---|
| `Searcher::time_limit_` | Initialized and assigned from `hard_limit_`; never read. | Remove in B.1. |
| `RootMoveStat`, `root_stats_` | Fields are populated and PV vectors assigned in root negamax; no field has a reader. `variance()` is never called. | Remove in B.1. This also removes growing-vector work from the root recursive path, which violates the per-node allocation contract today. Keep the live scalar root-effort counters used by time management. |
| Capture futility (3 coordinates), quiet SEE (2), qsearch quiet checks (1), post-LMR history nudge (1) | A.2.3 proved the default paths dead/inert; the code remains in qsearch/negamax. | Remove code and the seven coordinates in B.1. B.0 may reintroduce only a specified donor-form mechanism. |
| TUNE `KBNK Drive` option | A.2.3 found its advertised default drifted from the compiled value; only an explicit set changes evaluation. | Remove option, parser, setter and option-only tests in B.1; keep the compiled KBNK constant and invariant tests. |
| 23 zero evaluation groups | Every group has a release consumer; many also produce Texel features. Zero weights make them inert, not dead. | Retain through C.1. C.0/family owners decide activation or removal. |
| `cutoff_count` trace field | Serialized with `-1`, no producer. | Retain for B.2's registered decision trace; not a dead output. |
| `PST_MG_BASE` / `PST_EG_BASE` | Used only under `TEXEL_TRACE`. | Retain and move with trace code. |

Two stale comments are corrected when their code moves: capture futility says
its default is 0 although it is 1 (the gate is unreachable for a different
reason), and old `search_params.h` prose says `DoubleExtMax` is 200/inert while
the live default is 16. Comment correction does not change either mechanism.

## Invariants and qualification handed to B.1/C.1

- Exact `bench 13` remains **14,978,465 at every implementation commit**.
- Score perspective, mate band/TT ply adjustment, draw-scaling floors,
  lazy/full-path endgame semantics and TT publication/TT-move validation remain
  those in DESIGN §3.
- Search recursion and evaluation stay allocation-free; fixed stacks, PV and
  move buffers are not replaced by growing containers or large by-value state.
- A.5 counters, trace schema, ablation bit order and 77 required canaries keep
  their units and results. Instruments may move files but do not get redefined.
- Release and sanitizer CTest cover both restructures. B.1 additionally runs
  search/threading/ponder/WAC/endgame tests; C.1 builds the TEXEL variant and
  proves trace reconstruction on the committed fixture.
- Each final restructure gets the registered pooled-PGO NPS comparison; a
  slowdown outside noise returns the relevant boundary. Exact nodes alone do
  not waive this gate.

## Decision

`A.6 CLOSED`: the move tables and dead-code dispositions are concrete, and no
engine source was refactored. B.1 remains dependent on B.0's final search
handoff; C.1 remains dependent on C.0's refreshed family/shared-input plan.

## Re-anchored on the A.8 head (A.8.20, 2026-10-08)

The ranges above are at `2e7914e`. Phase A's step A.8 changed `src/search.cpp`
(3,125 → 3,298 lines) and nothing in `src/eval.cpp` (2,148 lines, no diff),
so the evaluation ranges stand. Each section's opening line was located at
`2e7914e` and found again at `c71381b`'s source (unchanged since `b53358a`):

| Section | At `2e7914e` | On the A.8 head | What A.8 added there |
|---|---|---|---|
| Root table and result sanitation | 75–231 | 83–239 | — |
| Construction, time and node control | 232–480 | 240–488 | — |
| History policy | 481–680 | 489–687 | — |
| Move ordering | 681–963 | 688–971 | the MultiPV root exclusion (`root_excluded_`) in the root filter |
| Diagnostics | 964–1342 | 972–1322 | the 25 printers on `std::format` (A.8.12) |
| Root output and tablebases | 1343–1446 | 1323–1446 | `format_info_line`, `legal_line`, `needs_pool_line` (A.8.7–A.8.8), `root_tablebase_display` |
| Quiescence | 1447–1686 | 1447–1686 | the stand-pat clamp below the tablebase band (A.8.14) |
| Negamax | 1687–2530 | 1687–2556 | the bound-correct tablebase probe, `tablebase_probe`, `is_decisive` guards (A.8.14), no root hash-table store for later MultiPV lines (A.8.9) |
| Iterative deepening | 2531–2823 | 2557–2955 | bound lines, terminal roots, per-iteration seldepth (A.8.8), MultiPV lines (A.8.9), the past-optimum fail-high stop (A.8.17) |
| Thread pool | 2824–3125 | 2956–3298 | the merged result's line before `bestmove`, no vote with MultiPV (A.8.7, A.8.9) |

The move table's tickets read on the new ranges:
- ticket 4: 489–687;
- ticket 5: 688–971;
- ticket 7: 2956–3298.

Ticket 1's `search_types.h` also takes `is_decisive`, `format_info_line`,
`legal_line` and `needs_pool_line` from `search.h`. Ticket 6's worker takes
the MultiPV state (`root_excluded_`, `multipv_lines_`) and
`tb_probe_in_search_` as per-search state. No ticket's contract changes.
