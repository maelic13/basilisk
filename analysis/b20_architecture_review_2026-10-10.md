# B.2.0 — Architecture review of the B.1 head, and the seams Cluster 1 needs

- State / class: `R2`; **CLOSED 2026-10-10**, BAS-P17 read 0.00% (§6).
  Nothing in play changes.
- Owner / date: 2026-10-10. Reviewed revision `8c706a8` (the B.1 head);
  upgrades landed as `c434ee4`, `79a0f2d`, `c55d06c`, `76eab94`, `0482615`,
  `9b51b46`, every engine commit at `bench 13` = **14,978,465**.
- Decision needed: none from the maintainer beyond running BAS-P17. The B.2.1
  contract is the B.0 packet's §12.2; §5 below refines it to the code as it
  now stands and is part of that contract.

Every claim here is **evidence** (a count, a fingerprint, a diff) unless it is
marked as a decision; the census is reproducible with
`python tools/diag/source_census.py` on any head, and the raw outputs for both
heads are in ignored `tools/results/b20-20261010/`. Prose comments were read,
but no finding rests on one.

## 1. Verdict

The B.1 head is the right shape for the cluster and needs no structural
rewrite before it. B.1 delivered what A.6 asked: value types, root helpers,
diagnostics, history policy, the picker, a worker with `SearchConfig`,
`SearchShared` and `SearchState`, the pool, and `NodeType` as a template
parameter. Dependencies point one way (board, evaluation, search, engine,
protocol), the recursion allocates nothing, and the single make/unmake seam
and the single TT-store wrapper that A.6 required are intact.

What the review found is accumulated weight at exactly the places the cluster
will press on, and it removed that weight at the fingerprint:

- **The kernel and the driver shared one 1,804-line file, and the driver named
  the kernel's internals** (`negamax<Root>` at four sites, the LMR table, the
  history tables and age counter, the stack and its sentinels). A cluster that
  replaces the kernel would have had to edit the driver at every one of those
  sites. The kernel now lives in `search_kernel.cpp` and the driver reaches it
  through three entry points (§4, F2).
- **Three contracts the cluster must extend were hand-synchronised lists:** the
  57 diagnostic counters with a manual `add()` and a literal count; one
  parameter table that would have mixed Cluster 1's coordinates with the legacy
  kernel's and the driver's; and the TT `flag_age` bit layout written as
  literals (`& 3`, `& 0xFC`, `+ 4`) in five places. Each is now one table or
  one set of named constants (F4–F6), and the Tune build's option set and its
  `Diag` and trace output are byte-identical to the B.1 build's.
- **The umbrella did not exist.** `B2_CORE` now does; both arms compile the
  legacy kernel and reproduce the fingerprint, which proves the option, the
  define and the PGO arm forwarding before the cluster depends on them (F3).
- **Comments narrated history.** 391 references to retired phase, step and
  ledger numbers in `src/`, 86 of them in `search_worker.cpp`. The driver, the
  shared headers and the pool now carry 2 (both version-like numbers the
  pattern over-matches); the legacy kernel keeps its 55 because B.8 deletes it,
  `eval.cpp` keeps its 82 for C.1, and the board its 26 for D.3.1 (F7).

Deferred by design, as Rarog's review deferred its state split: anything the
cluster rewrites. The per-ply record, the tables, the picker, the TT
encoding, the counters and the trace fields all change in B.2.1, so moving
them now would qualify the same lines twice. §5 freezes their design instead.

## 2. Measurements

### 2.1 Fingerprints and tests on the B.2.0 head (`9b51b46`)

| Check | Result |
|---|---|
| `bench 13`, release PEXT (`build/b1-pext`) and plain (`build/b1-plain`) | **14,978,465** at every commit `c434ee4`..`0482615` |
| `bench 13`, Tune, Diag, Ablate flavors | 14,978,465 each |
| `bench 13`, `B2_CORE=ON` arm (`build/b20-core-on`) | 14,978,465 (the define is unread; the two binaries differ only in embedded build metadata) |
| `bench 10`, sanitizer build (`build/san`, ASan+UBSan, Debug) against release | 3,190,673 = 3,190,673, no sanitizer report |
| Release CTest | 19/19, strength floors 2/2 |
| Sanitizer CTest (`-LE strength`) | 17/17 |
| Tune `uci` option list against the B.1 Tune build | same 54 options; `AspirationDelta` moved from position 28 to 44 (F5) |
| `Diag` and `DecisionTrace` output, four searches (startpos d11, Italian d11, KPK d14, WAC.001 d7 `searchmoves g3g6`) | 634 lines identical after stripping `nps` and `time`: 23 diag lines, 11 kv lines, 598 trace records |
| `tools/generate_spsa_surface.py --check` | 3 generated files current (41 parameters) |
| Pooled-PGO NPS against the B.1 pool | **pending**, BAS-P17 (§6) |

### 2.2 Source census, B.1 head against B.2.0 head

`source_census.py`: lines, comment lines, and references to retired numbering
inside comments (the pattern over-matches version-like numbers, so the figure
is an upper bound and only its change is meaningful).

| File | B.1 lines / comments / retired | B.2.0 lines / comments / retired |
|---|---|---|
| `search_worker.cpp` | 1,804 / 370 / 86 | **692 / 117 / 2** (driver only) |
| `search_kernel.cpp` | — | 1,109 / 232 / 55 (the legacy kernel, moved) |
| `search_worker.h` | 258 / 77 / 10 | 277 / 87 / 0 |
| `search_params.h` | 147 / 96 / 27 | 157 / 92 / 9 |
| `search_diagnostics.h` | 244 / 67 / 19 | 170 / 48 / 0 |
| `search_diagnostics.cpp` | 359 / 29 / 13 | 353 / 24 / 0 |
| `tt.h` | 295 / 66 / 13 | 304 / 68 / 7 |
| `search_types.h` | 115 / 22 / 3 | 122 / 25 / 0 |
| `search_thread_pool.cpp` | 347 / 29 / 4 | 343 / 25 / 0 |
| `move_picker.h` | 221 / 28 / 9 | 208 / 26 / 8 |
| `src/` total | 13,588 / 2,103 / 391 | 13,540 / 2,065 / 288 |

### 2.3 Coupling of the worker's regions to its state

Occurrences of each member access, by file, on the B.2.0 head:

| File | lines | `config_.limits.params` | `state_.` | `hist_.` | `shared_.` | `evaluator_.` |
|---|---:|---:|---:|---:|---:|---:|
| `search_worker.cpp` (driver) | 692 | 2 | 159 | **0** | 38 | 11 |
| `search_kernel.cpp` | 1,109 | 22 | 141 | 5 | 6 | 4 |
| `search_history.cpp` | 204 | 3 | 13 | 22 | 0 | 0 |

On the B.1 head the single worker file read the parameters 25 times, the
state 304 times and the histories 5 times; the driver now reads the two root
parameters (aspiration, time) and no history. The kernel's reads of the
driver's state are the board, the diagnostics, `stopped`, `nodes`, the PV
table, the selective depth, the root effort counters, the MultiPV exclusion
list, the root ordering inputs and the move buffers (§5.2 lists them as the
contract).

### 2.4 Include edges after the split

```
search.h              -> search_types search_root search_worker search_thread_pool
search_worker.h       -> board eval search_diagnostics search_root search_types syzygy tt
                         + history move_picker            (the kernel's headers)
search_kernel.cpp     -> search_worker constants syzygy
search_history.cpp    -> search_worker
search_thread_pool.h  -> board search_root search_types search_worker tt
move_picker.h         -> board history search_root search_types
search_diagnostics.h  -> eval move search_types
search_root.h         -> board search_types syzygy
search_types.h        -> constants move search_params syzygy
tt.h, history.h       -> types move
```

Every edge points down the layer list; no search file includes the engine or
the protocol. `move_picker.h` no longer defines the state's value types
(`ScoredMove`, `PIECE_VALUE`, `RootOrdering` moved to `search_types.h` and
`search_root.h`), so a kernel that brings its own picker does not have to
include the legacy one.

## 3. The architecture as found

### 3.1 Layers and one `go`

`main` composes the engine; `uci_protocol` parses into `Parameters` and the
command queue; `Engine` owns the table, the pool and the option state and
prints `bestmove`; `SearchThreadPool` owns one `Searcher` per thread and
merges results; `Searcher` searches; `Evaluator` evaluates; `Board` moves.
`Syzygy` is process-global because Fathom is. A `go` copies the board by
value into the worker, resets `SearchState`, derives the time budget from the
`go`-receipt instant, ages the table once per `go` (the pool does it for all
threads), and iterates depths with aspiration from depth 4; helpers start at
depth 1 or 2 and never carry a depth limit; stop, ponderhit and the node
limit are polled every 2,048 nodes in both negamax and quiescence.

### 3.2 State ownership

| Lifetime | Where it lives now | Reading |
|---|---|---|
| Engine resources | `Engine::tt_`, `search_pool_`; borrowed by `SearchShared` | right |
| Pool per `go` | `PoolResources` (node and tablebase counters, root table), set on each worker's `shared_.pool` | right; null at one thread, so a single-thread search has no pool machinery by construction |
| Per search, fixed | `SearchConfig` (limits with `SearchParams`, thread id and count, root filter) | right |
| Per thread, persistent | `Evaluator` (pawn cache), `HistoryTables`, the LMR table, the age counter | right; the last three are the kernel's and sit in its fenced section |
| Per search, mutable | `SearchState` (board pointer, counters, time, root state, MultiPV exclusion, diagnostics, trace, stack, PV table, move buffers) | right; the kernel-shaped members (stack, PV, buffers) stay here because the driver reads the PV and the kernel writes it |

### 3.3 Allocation and footprint

The recursion allocates nothing: `MoveList` is a 256-slot stack array,
`quiets_searched` and `bad_caps_searched` are fixed arrays, ProbCut's capture
list is a stack `MoveList`, the PV table and move buffers are fixed arrays in
`SearchState`. Per thread today: history tables **3.24 MiB** (three
continuation tables 1.15 MiB, pawn history 1.75 MiB at 2,048 buckets,
correction tables 0.25 MiB), `SearchState` about 0.55 MiB (move buffers 512
KiB, PV table 32 KiB, stack 6.4 KiB), the evaluator's pawn cache 768 KiB.

### 3.4 Instruments

57 counters compiled out of release builds, printed as human lines and a
`kv name=value` mirror that `run_suite.py` parses by name; the decision trace
records 14 positional fields at plies 1–2 under `searchmoves`, with
`cutoff_count` fixed at `-1` and `version=1` in its header, which
`decision_trace.py` asserts. The eight ablation bits are read through
`ABLATED(bit)` in the kernel.

### 3.5 Tests

Twelve test binaries over the public surface (`search.h`, `tt.h`,
`history.h`, the board). The search is reached only through `Searcher` and
`SearchThreadPool`; `HistoryTables` is constructed directly in one test
(`test_history_row_base_decomposition`), the TT in `test_tt`. The picker,
the kernel's history policy and the correction code have no direct tests; the
strength floors (WAC solved count, KBNK/KQK/KRK/KBBK conversion, the mate
band) are labelled `strength` and run in release CTest.

## 4. Findings

Each finding is **keep**, **landed** (this leaf, at the fingerprint) or
**owner** (a named later leaf).

**F1 — keep.** Layering, the three-lifetime state partition, the single
make/unmake seam, the single TT-store wrapper, allocation-free recursion. B.1
delivered A.6's contract and nothing here contradicts it.

**F2 — landed (`c434ee4`): the kernel/driver split and the entry contract.**
`search_kernel.cpp` holds quiescence, negamax, the LMR table, the trace and
ablation macros, the tablebase probe helper and `tt_store`;
`search_worker.cpp` keeps the driver. The driver calls `root_search(depth,
alpha, beta)`, `kernel_begin_search()` and `kernel_new_game()` and nothing
else of the kernel's; `Searcher`'s kernel members sit in one fenced section
at the end of the class with the rule written above it. `STACK_SENTINELS`
names the four plies below the root. Fingerprint exact; the driver's reads of
`hist_` fell from 5 to 0.

**F3 — landed (`79a0f2d`): the `B2_CORE` umbrella.** `option(B2_CORE ... OFF)`
defines `BASILISK_B2_CORE` on every target, including the tests, and the
kernel sources are one CMake list, `BASILISK_SEARCH_KERNEL_SOURCES`, that the
ON arm will replace. Both arms build and bench identically today.
`build_test.ps1 -ArmOption B2_CORE -Arm On|Off` already forwards a declared
Boolean option through both PGO phases and records it in the manifest.

**F4 — landed (`c55d06c`): counters as one table.** `BASILISK_DIAG_COUNTERS(X)`
generates the fields, `reset()`, `add()` and `COUNT`; a static assertion pins
the count at 57 and the struct's size to it. Printing is unchanged and was
diffed byte for byte. New counters append to the table and print on new `kv`
lines, never inside existing ones, so the two parsers keep working.

**F5 — landed (`c55d06c`, `76eab94`): parameter tables by owner.**
`BASILISK_ROOT_PARAMS` (aspiration, time), `BASILISK_LEGACY_KERNEL_PARAMS`,
and an empty `BASILISK_CORE_KERNEL_PARAMS` that the ON arm compiles in place
of the legacy table; `BASILISK_SEARCH_PARAMS` is the arm's kernel table then
the root table. The option set is unchanged; `AspirationDelta` now follows
the kernel's options, and the SPSA surface was regenerated from the header.
The generator still emits every `X(` line it sees; B.2.3's surface must be
the core table alone (F15).

**F6 — landed (`c55d06c`): the TT layout as constants.** `TT_BOUND_MASK`,
`TT_AGE_MASK`, `TT_AGE_STEP`, `TTEntry::bound()` and `age()`; the replacement
quality is written as generations times two, the same value. The legacy
kernel's `& 3` reads stay valid because the bound keeps the low two bits in
every layout the cluster considers (§5.6).

**F7 — landed (`0482615`) and owner.** The driver, the limits, the pool, the
diagnostics and the TT header explain their code without retired numbers; a
measured reason stays in one sentence with its ledger ID as the pointer
(BAS-D16, BAS-D17, BAS-C05, BAS-S13, BAS-S16). Owners for the rest: the
legacy kernel, picker and history files die with B.8; `eval.cpp` and
`eval_params.h` are C.1's; the board is D.3.1's.

**F8 — keep.** `SearchConfig` / `SearchShared` / `SearchState`. Rarog had to
design this split as its B.2.1 ticket 0; B.1 already did it here, and the
kernel's reads of the three aggregates (§2.3) are what a kernel legitimately
needs. No further split.

**F9 — owner B.2.1 (ticket 1): the per-ply record and the sentinels.**
Specified in §5.4. `STACK_SENTINELS` rises from 4 to 7 for both arms when the
core kernel reads six continuation plies; the legacy kernel reads at most
`(ss - 4)` and a default record is the sentinel, so the off arm's fingerprint
proves the change neutral.

**F10 — owner B.2.1 (ticket 3): tables and footprint.** Specified in §5.5.
The core kernel's tables weigh **9.2 MiB per thread** at the donor's 8,192
pawn-history buckets against 3.2 MiB today, 7.0 MiB of it the pawn history.
The bucket count is a size, not an Elo coordinate: B.2.2's time-to-depth
screen decides between 8,192 and 2,048 (4.0 MiB), and it is never an SPSA
coordinate.

**F11 — owner B.2.1 (ticket 2): the TT encoding on the ON arm.** Specified in
§5.6 with the occupancy rule the miss-store needs.

**F12 — owner B.2.1 (ticket 3): the picker.** Specified in §5.7: stages,
the skip-quiets contract, the one-buffer partition and the exhaustiveness test.

**F13 — owner B.2.1 (ticket 1): the threat producer.** A free function over
the board's bitboards and `attacks.h`, in the board library, tested against a
brute-force reference (§5.8).

**F14 — owner B.2.1 (ticket 0): counters and trace.** Append-only `kv` lines;
the trace record gains `lmr_depth`, `skip_quiets` and the pruning family and
its header becomes `version=2` with `cutoff_count=available`;
`decision_trace.py` asserts `version=1` and the `-1` sentinel today and must
be extended in the same ticket, as must `run_suite.py`'s key handling if it
rejects unknown keys (§5.9).

**F15 — owner B.2.1 / B.2.3: the surface generator.**
`generate_spsa_surface.py` must take the table to emit (`--table core`)
before B.2.3 registers its surface; today it would include the root and,
on a legacy build, the legacy kernel's coordinates.

**F16 — owner B.8.** Deleting `search_kernel.cpp`, `search_history.cpp`,
`move_picker.*`, `history.*`, `BASILISK_LEGACY_KERNEL_PARAMS`, the `#else`
arms of the fence and the option itself once the default has flipped.

**F17 — owner B.7.** The per-candidate `gives_check` in the legacy move loop
(1.43 calls per node, B.0 §2.5) is a throughput item that the core kernel's
picker-level count skip removes for quiets; the threat producer's placement
(per node against incremental in `make_move`) and the pawn-history footprint
are B.7's to price if B.2.2's pooled NPS asks.

**F18 — owner D.2.** Stockfish 19 shares its continuation, pawn and
correction histories across threads through atomic entries
(`SharedHistories`); Basilisk keeps every table per thread. At one thread
nothing differs, and the gates are at one thread; the choice is D.2's with
its own NPS and scaling measurements, not B.2's.

**F19 — keep, recorded for the core kernel in chess terms.** Behaviours of
the legacy kernel the review read and the core kernel must not inherit by
accident, each with its disposition:

- `score_from_tt` returns 0 for a non-decisive score at a rule-50 clock of
  100 or more. The donor downgrades decisive scores by the remaining clock and
  refuses TT cutoffs at a clock of 96 or more; B.0 adopts that (§3.1), so the
  core kernel's probe converts scores its own way and the legacy function
  stays for the off arm.
- Killers are never cleared for a child ply, so a killer from an unrelated
  subtree can rank first at a ply. Moot: killers and countermove go with the
  cluster.
- At an in-check node `ss->eval` is `VALUE_NONE` and `improving` is false;
  the donor inherits `(ss - 2)->staticEval` so `improving` stays defined two
  plies later. Adopted (B.0 §3.3); the sentinel plies must therefore carry
  `VALUE_NONE`, which the default record does.
- `tt_pv` is reconstructed per node (`is_pv || exact TT entry at depth - 1`);
  the persisted bit replaces it, and the donor also inherits `(ss - 1)->ttPv`
  into a fail-low store (`ss->ttPv = ss->ttPv || (ss - 1)->ttPv` before the
  final write).
- The tablebase probe stores at `depth + 6` with `INF_EVAL` as the static
  evaluation. The miss-store writes only when the probe missed, so it never
  overwrites a tablebase entry; a later probe of such an entry finds no
  evaluation and evaluates, as today.
- Nothing is stored at a singular-exclusion node or at a MultiPV root line
  searched without its best moves. Both stay.
- The null-move verification re-enters negamax at the same ply with the same
  stack record and `allow_null` false. `allow_null` stays a runtime flag until
  B.3 replaces it with the donor's `nmpMinPly` region (B.0 §3.1), and the
  core kernel's record must stay coherent across that re-entry
  (`move_count`, `cutoff_cnt` and `tt_pv` are reset at node entry as the
  donor does).
- The check extension is unconditional and checking moves are never reduced
  or pruned in the move loop. B.3's, measured against this head (BAS-S08,
  BAS-S16); the core kernel keeps both (B.0 §3.5), and the direct-check
  exemption from the count skip is a categorical switch (§5.10).
- Quiescence is fail-hard with a documented reason (mate-distance stores
  degraded when it was made fail-soft). B.4's; the core kernel calls the
  legacy quiescence unchanged in B.2 and the stand pat keeps its current form.

**F20 — observation, owner B.2.1 (ticket 0).** `DecisionTrace::record` takes
14 positional integers; a new field means a 15th at every one of the
kernel's 27 call sites. The core kernel records through a `TraceRecord`
built with designated initialisers and one `record(const TraceRecord&)`
overload; the legacy kernel keeps its calls.

## 5. Cluster 1 seams — the contract refinement for B.2.1

B.0 §12.2 is the contract: goal and semantics (§3.2–3.4, 3.6–3.7, §5), seeds
(§7.3), the producer map (§4), invariants, ticket order 0–8, qualification
(§10), non-goals. This section says where each part lands in the code as it
stands after B.2.0, and what shape it takes, so implementation does not invent
it. Where this section and §12.2 could be read differently, §12.2's semantics
win and this section's placement wins.

### 5.1 The umbrella, mechanically

What exists: the option, the define on every target, the kernel source list,
the fenced kernel section in `Searcher`, the kernel-selected parameter table,
the TT constants, `build_test.ps1 -ArmOption B2_CORE -Arm On|Off`.

What B.2.1 adds, each at the off arm's exact fingerprint:

1. `search_worker.h`: the kernel section and the two kernel includes become
   `#if defined(BASILISK_B2_CORE)` / `#else` / `#endif`, the ON branch
   declaring the core kernel's tables, helpers and recursive signatures. The
   driver section is untouched. `STACK_SENTINELS` becomes 7 for both arms
   (F9).
2. `CMakeLists.txt`: `if(B2_CORE)` sets `BASILISK_SEARCH_KERNEL_SOURCES` to
   the core files (`search_kernel_core.cpp`, `search_history_core.cpp`,
   `move_picker_core.cpp`, `history_core.cpp`); `threats.cpp` joins the board
   library on both arms. The ON arm's `id name` gains a `+b2core` suffix so a
   binary states its arm (manifests already record it).
3. `tt.h`: the ON-arm layout under the same `#if` (§5.6); the three constants
   and `pack_entry`/`unpack_entry` are the only lines that change.
4. `search_params.h`: `BASILISK_CORE_KERNEL_PARAMS` filled (§5.10).
5. CI: one more job building and testing the ON arm (`-DB2_CORE=ON`,
   release-pext, Linux is enough) and reading its bench; the fingerprint
   agreement job keeps comparing the OFF arm only.
6. CTest runs on both arms at every ticket; the strength floors bind both.

### 5.2 The kernel entry contract

The driver calls, in `Searcher::search`:

- `kernel_begin_search()` once per `go`, after the table was aged and before
  the first iteration: the kernel builds its reduction table from
  `config_.limits.params`, ages its histories on its own schedule and resets
  `state_.stack` to default records (a default record is the sentinel).
- `root_search(depth, alpha, beta)` once per aspiration attempt and once per
  extra MultiPV line. Contract: searches the root at ply 0 with the full
  move list filtered by `config_.limits.root_moves`, the pool's root filter
  (`root_filter_index` / `root_filter_count` on the move ordinal),
  `root_tablebase_allows` and `state_.root_excluded`; returns the root's
  score, fail-soft outside the window; whenever a root move raised alpha it
  leaves that move's line in `state_.pv_table[0][0..pv_len[0])` with the
  move at index 0; keeps `state_.sel_depth`, `state_.root_depth_nodes` and
  `state_.root_best_nodes` as the time manager reads them; honours
  `state_.stopped` by returning promptly (its value is then discarded by the
  driver except at the root with a best move); records nodes through
  `record_node()` and polls through `check_stop()` every 2,048 nodes.
- `kernel_new_game()` on `clear()`.

The kernel reads `config_` (limits, params, root filter, thread id and
count), `shared_` (table, stop flag, pool counters through `record_node`),
`state_.board`, `state_.tb_probe_in_search`, `state_.root_ordering` and
`state_.root_excluded`; it writes `state_.nodes`, `tb_hits`, `stopped`,
`sel_depth`, the PV table, the root effort counters, `state_.diag` and
`state_.trace`. Nothing else crosses. The core kernel's `negamax<NT>` may take
any signature it likes (the donor's `(ss, alpha, beta, depth, cutNode)` with
`ss->ply` is the recommended one); only `root_search` is fixed.

### 5.3 Files

| New | Holds |
|---|---|
| `src/search_kernel_core.cpp` | the core `quiescence` (the legacy body, re-wired to the new estimated score and stack record, B.4 owns its policy), `negamax<NT>`, `root_search`, `kernel_begin_search`, `kernel_new_game`, the reduction table |
| `src/search_history_core.cpp` | the update policy of B.0 §3.6 (bonus and malus shapes, what trains on a cutoff, a fail low, an eval swing, a TT cutoff, after LMR), the correction form and admission of §3.3 |
| `src/history_core.h/.cpp` | the table set of §5.5 as self-contained types with `update(bonus)` gravity, `clear()`, per-search decay |
| `src/move_picker_core.h/.cpp` | the staged picker of §5.7 |
| `src/threats.h/.cpp` (board library) | the threat producer of §5.8 |
| `tests/test_history_core.cpp`, `tests/test_move_picker_core.cpp`, `tests/test_threats.cpp` | §5.11; the first two compiled on the ON arm only, the third on both |

### 5.4 The per-ply record (`SearchStack`, ON arm)

The donor's `Stack` in Basilisk's names. Fields and who writes them:

| Field | Written by | Read by |
|---|---|---|
| `move`, `moved_piece` | `do_move` / `do_null_move` | continuation indices, `(ss-1)->move != MOVE_NULL`, correction |
| `excluded` | the singular search | TT store guard, probe |
| `eval` (corrected static eval; `VALUE_NONE` in check, inherited from `(ss-2)` for `improving`) | node entry | `improving`, `opponent_worsening`, hindsight, eval-difference training, correction update |
| `stat_score` | the move loop, for the move made | LMR, the TT-cutoff penalty of the parent's early quiet, bonus scaling |
| `reduction` | the move loop (the child's `priorReduction`) | hindsight at the child |
| `move_count` | the move loop (`++move_count`, counts every picked move) | the child's LMR terms, the fail-low parent bonus scale |
| `cutoff_cnt` | `(ss+2)->cutoff_cnt = 0` at node entry; `ss->cutoff_cnt += (extension < 2) || PV` at a cutoff | the parent's LMR (`(ss+1)->cutoff_cnt`), NMP in B.3 |
| `tt_pv` | node entry: `excluded ? ss->tt_pv : PV || (hit && entry.is_pv())`; `ss->tt_pv |= (ss-1)->tt_pv` before a fail-low store | RFP veto, LMR terms, the TT write, replacement |
| `tt_hit` | node entry | RFP's `!ttHit` multiplier term, eval-difference training's `!ttHit` guard |
| `in_check` | node entry | continuation block index of the children, correction admission, bonus scale |
| `cont_hist` (pointer to the `[pt][to]` row for this ply's move) | `do_move`: `&block[in_check][capture][moved_piece][to]`; a sentinel row for the null move and the plies below the root | quiet scoring (plies 1–6), continuation pruning, `stat_score`, updates |
| `cont_corr` (pointer to the continuation-correction row) | `do_move` | correction value and update at plies 2 and 4 |
| `double_exts` | as today | B.3 keeps the cap |

`killers` leaves the record. `ply` may join it so the recursion drops the
parameter. Seven sentinels precede the root; a default record carries
`MOVE_NONE`, `NO_PIECE_TYPE`, `VALUE_NONE`, the sentinel continuation rows,
zero counts and a false `tt_pv`.

### 5.5 Tables (`history_core.h`, per thread)

Basilisk indexes pieces by type (`PIECE_TYPE_NB` = 7) because the side to
move is known at every consumer; the donor's `PIECE_NB` = 16 indexing is
colour-redundant here. Limits are the donor's (B.0 §7.1, history units
convert at 1).

| Table | Index | Limit | Size |
|---|---|---:|---:|
| main (butterfly) | `[colour][from][to]` (promotions share the from-to; the donor keys the 16-bit move, a difference only for under-promotions) | 7,183 | 16 KiB |
| capture | `[attacker pt][to][captured pt]` | 10,692 | 6 KiB |
| continuation block | `[in_check][capture][pt][to]` → row `[pt][to]`; plies 1–6 read rows through the stack pointers | 30,000 | 1,568 KiB (heap) |
| pawn | `[pawn_key & (buckets-1)][pt][to]`, 8,192 buckets (2,048 if B.2.2's speed screen says so) | 8,192 | 7,168 KiB (1,792 at 2,048) (heap) |
| low-ply | `[ply < 5][from][to]`, refilled toward 102 per search | 7,183 | 40 KiB |
| TT-move history | one entry | 8,192 | 2 B |
| pawn / minor / non-pawn correction | as today, `[colour][key & 16383]` and `[colour][colour][…]` | 1,024 | 256 KiB |
| continuation correction | `[pt][to]` → row `[pt][to]`, read at plies 2 and 4 | 1,024 | 392 KiB (heap) |

Total 9.2 MiB per thread at 8,192 buckets, 4.0 MiB at 2,048, against 3.2
today. Every table is a type with `update(int16_t& e, int bonus)` gravity
`e += bonus - e * |bonus| / LIMIT`, `clear()`, and the per-search ageing of
§3.6 (main ×729/1024, low-ply refilled, nothing else), so tests reach bounds
and gravity without a `Searcher`. `HistoryTables` (legacy) is untouched.

### 5.6 The TT on the ON arm

Layout of `flag_age`: bound bits 0–1 (unchanged, so `bound()` and the legacy
reads stay valid), **pv bit 2**, age bits 3–7: `TT_BOUND_MASK` 0x03,
`TT_PV_MASK` 0x04, `TT_AGE_MASK` 0xF8, `TT_AGE_STEP` 0x08, cycle 32 searches
(the donor's). `TTEntry::is_pv()` reads bit 2; `store` gains a `bool pv`
parameter; `hashfull` and `entry_quality` use the masks (quality stays
`depth − 2·generations + 2·exact`; the donor's `+ 2·pv` in replacement is not
adopted, B.0 §3.2).

The miss-store (raw static eval, no move, no score, depth unsearched) needs
an occupancy rule, because today an empty slot is "bound == TT_NONE" and a
miss-stored entry has no bound. Decision for B.2.1: store depth with an
offset, `depth8 = depth + 2`, so an empty slot (payload 0) decodes as depth
−2 and no real entry does; `TT_DEPTH_UNSEARCHED = −1` encodes as 1 and
quiescence's depth 0 as 2; occupancy is `depth8 != 0`; `probe_copy` returns
a hit for a miss-stored entry with `bound() == TT_NONE`, `score ==
VALUE_NONE`, and the consumers test the bound before using the score, as the
donor does (`ttData.bound & BOUND_LOWER`). The clamp in `pack_entry` becomes
`[−1, 125]`. `score_from_tt` on the ON arm downgrades a decisive score by the
remaining rule-50 clock and never zeroes a non-decisive one; the cutoff guard
at a clock of 96 or more lives in the kernel. `test_tt` gains: the pv bit
round-trips and survives a same-key refusal; the age cycle at step 8; a
miss-store is a hit with no bound and keeps its eval; a later real store
replaces it; an empty slot that hashes to key 0 is still a miss.

### 5.7 The picker (`move_picker_core`)

Stages, the donor's: TT move (validated as today: piece, colour, legality);
captures generated and scored by `capture_history + 7·PIECE_VALUE[captured]`,
good ones (`see_ge(move, −value/18)`) delivered, bad ones deferred; quiets
generated and scored by the §3.6 formula with the threat terms, partially
sorted above `−3560·depth`, good quiets (score > −14000) delivered, bad quiets
deferred; bad captures; bad quiets. `skip_quiets()` ends quiet delivery:
after it, good and bad quiets are not delivered, bad captures still are. One
256-slot buffer per node partitioned by index (`cur`, `end_bad_captures`,
`begin_bad_quiets`, `end`), as the donor; the second buffer in `SearchState`
stays for the legacy kernel. Evasions: the legacy kernel generates all legal
moves in check through the same picker; the core keeps that (the donor's
evasion stage orders by history too, which B.2.1 may adopt without a
categorical run since in-check nodes are never pruned). ProbCut and
quiescence keep their own capture loops in B.2 (B.3 and B.4 own them).

Contract and test: over a corpus of random-walk positions (the generator in
`test_invariants`), every legal move is delivered exactly once with
`skip_quiets` never called; with `skip_quiets` called after the first
quiet, the delivered set is exactly the captures and promotions; the TT move
is first when legal and never delivered twice; an illegal or foreign TT move
is skipped.

### 5.8 The threat producer (`threats.h`)

```
struct Threats { Bitboard by_pawn, by_minor, by_rook, all; };
Threats threats_against(const Board& b, Color us);   // attacks of ~us
```

`by_pawn` = pawn attacks of the opponent; `by_minor` = knight and bishop
attacks (sliders through the full occupancy) plus `by_pawn`; `by_rook` = rook
attacks plus `by_minor`; `all` = those plus queen and king attacks. Computed
once per node, lazily at the first quiet scoring (TT-hit nodes that cut off
never pay), from `attacks.h`'s tables and `Board::piece_bb`; no board state
changes. The quiet-scoring terms it feeds are the donor's: a bonus for
moving a threatened piece to safety and a malus for moving into a threat,
scaled by piece value, plus the `check && see_ge(−75)` term through
`check_squares`. Test: against a brute-force attacker set (`is_square_attacked`
over all 64 squares, filtered by attacker type) on 10,000 random-walk
positions, bit for bit.

### 5.9 Counters and trace (ticket 0)

Counters: append B.0 §9's names to `BASILISK_DIAG_COUNTERS` after
`lmr_blocked_gives_check`, change the pinned count, print them on new `kv`
lines (one or two) in `print_search_diag`, each with its denominator
(`rfp_cuts_d1_3`/`d4_7`/`d8p`, `razor_cuts_*` likewise, `lmr_floor_hits`,
`lmr_extended`, `lmr_research_deeper`, `lmr_research_shallower`,
`hindsight_up`, `hindsight_down`, `skip_quiets_nodes`, `corr_cont2_updates`,
`corr_cont4_updates`, `tt_cutoff_quiet_bonus`, `tt_cutoff_graph_refused`,
`cont_hist_pruned`, `capture_futility_pruned`, `quiet_see_pruned`,
`capture_see_pruned`, `moveloop_tested`). `hist_below_*` stay in the table
(the legacy kernel increments them) and read 0 on the ON arm; they retire in
B.8. `run_suite.py`: confirm it tolerates keys it does not know, or extend
its key list, before the first ON-arm suite.

Trace: `TraceRecord` gains `lmr_depth`, `skip_quiets` (0/1) and `family`
(the pruning family that fired, as a small enum printed by name);
`cutoff_count` gets its producer; the header becomes `version=2
cutoff_count=available`; the record line appends the new tokens after
`result`. `decision_trace.py` accepts version 2 and the new keys in the same
ticket, with its test updated. The four positions of B.0 §10 (WAC.001, 085,
203, 253) are traced before B.2.2.

Ablation: bits 0, 1, 5 and 7 re-declared on the core kernel's razoring, RFP,
move-loop family and LMR; the liveness proof (`ablation_liveness.py`) re-run
on the ON arm and recorded.

### 5.10 Coordinates and categorical switches

The core table's coordinates, grouped as B.0 §8, with seeds from §7.3 and
ranges spanning the three columns (half the smallest column to twice the
largest, rounded; B.2.3 may widen before registration). Units: evaluation
units unless stated; 1024ths and counts as marked. This inventory is the
proposal B.2.1 types into `BASILISK_CORE_KERNEL_PARAMS`; a value that §7.3
fixes wins over this table where they disagree.

| Group | Coordinate (UCI name) | Seed | Range | Unit |
|---|---|---:|---|---|
| node margins | `CoreRazorMargin` | 256 | 128–527 | eval |
| | `CoreRfpPerPly` | 133 | 40–227 | eval per ply |
| | `CoreRfpImproving` | 89 | 35–160 | eval |
| | `CoreRfpWorsening` | 11 | 0–60 | eval |
| | `CoreRfpCorrDiv` (0 = term off) | 0 | 0–64 | 1/div of |corr| |
| | `CoreRfpDepthCap` | 9 | 6–19 | plies |
| | `CoreRfpBetaWeight` (eval weight is 1024 − this) | 661 | 512–1024 | 1024ths |
| | `CoreIirMinDepth` | 4 | 3–8 | plies |
| | `CoreTtCutoffBonusSlope` | 131 | 0–300 | history per ply |
| move-loop pruning | `CoreLmpBase` | 3 | 1–8 | moves |
| | `CoreLmpSquareScale` (×d²/16) | 16 | 8–32 | 16ths |
| | `CoreQuietFutBase` | 158 | 80–300 | eval |
| | `CoreQuietFutSlope` | 108 | 34–190 | eval per lmrDepth |
| | `CoreQuietFutAlphaTerm` | 25 | 0–90 | eval |
| | `CoreQuietFutDepthCap` | 8 | 5–12 | lmrDepth |
| | `CoreCaptFutBase` | 92 | 40–270 | eval |
| | `CoreCaptFutSlope` | 115 | 60–390 | eval per lmrDepth |
| | `CoreCaptFutHistDiv` | 27 | 8–128 | divisor |
| | `CoreCaptFutDepthCap` | 7 | 4–8 | lmrDepth |
| | `CoreContHistPruneSlope` | 4136 | 1500–8000 | history per ply |
| | `CoreQuietSeeCoeff` (×lmrDepth²) | 10 | 4–30 | SEE units |
| | `CoreCaptSeeCoeff` (×depth) | 74 | 30–200 | SEE units |
| | `CoreCaptSeeHistDiv` | 80 | 20–200 | divisor |
| LMR | `CoreLmrTableScale` (×ln(i)/128) | 2872 | 1500–4500 | 1024ths |
| | `CoreLmrOffset` | 982 | 0–2000 | 1024ths |
| | `CoreLmrNotImproving` (×R/512) | 197 | 0–500 | 512ths |
| | `CoreLmrTtPv` | 929 | 0–2000 | 1024ths |
| | `CoreLmrTtPvBase` | 3023 | 0–6000 | 1024ths |
| | `CoreLmrTtPvPvNode` | 1004 | 0–2500 | 1024ths |
| | `CoreLmrTtPvAboveAlpha` | 885 | 0–2000 | 1024ths |
| | `CoreLmrTtPvDepthOk` (+940·cutNode fixed) | 816 | 0–2000 | 1024ths |
| | `CoreLmrCutNode` | 4026 | 1500–7000 | 1024ths |
| | `CoreLmrCutNodeNoTt` | 933 | 0–2500 | 1024ths |
| | `CoreLmrTtCapture` | 1079 | 0–2500 | 1024ths |
| | `CoreLmrCutoffCnt` | 264 | 0–1000 | 1024ths |
| | `CoreLmrCutoffCnt2` | 1095 | 0–2500 | 1024ths |
| | `CoreLmrCutoffCntAll` | 1138 | 0–2500 | 1024ths |
| | `CoreLmrStatScale` (/4096) | 439 | 100–1200 | 4096ths |
| | `CoreLmrAlphaEvalScale` (×clamp(alpha − eval, −18, 27)) | 3 | 0–10 | 1024ths per eval unit |
| | `CoreLmrAllNodeScale` (×r/(256d + 268)) | 276 | 0–800 | 1024ths |
| | `CoreResearchDeeper` | 15 | 4–40 | eval |
| | `CoreResearchShallower` | 2 | 0–12 | eval |
| histories | `CoreQuietBonusCap` | 1487 | 600–3000 | history |
| | `CoreQuietBonusSlope` (−81 intercept fixed) | 133 | 60–300 | history per ply |
| | `CoreMalusCap` | 2244 | 900–4500 | history |
| | `CoreMalusSlope` (−235 intercept fixed) | 968 | 400–2000 | history per ply |
| | `CoreMalusDecay` (per earlier move) | 921 | 700–1024 | 1024ths |
| | `CoreMainHistDecay` (per search) | 729 | 400–1024 | 1024ths |
| correction | `CoreCorrUpdateScale` (×depth·diff/128; the no-best-move case uses 3/2 of it) | 12 | 4–30 | 128ths |
| | `CoreCorrContWeight` | 8761 | 2000–16000 | 131072ths |

That is 51 coordinates against B.0's "expected 46"; the difference is the
LMR group counted individually. Capture-history scoring weight (7), the
quiet threat weights, the `−14000` good-quiet threshold, the `−3560·depth`
sort limit, the continuation weights {520, 390, 145, 251, 66, 209}, the
positive-consistency multipliers and the eval-difference training constants
are fixed at the donor's values in B.2 (history units, rule 1).

Categorical switches, tune-build UCI options never on an SPSA surface,
settled by 2,000-game paired runs on one tune build (PROCESS, Cluster
delivery shape) or by the time-to-depth screen where marked:

| Option | Values | Default | Decides |
|---|---|---|---|
| `CoreRazorDepthCap` | 1–3 | 1 | B.0.1 was flat (+2.4 ± 9.9), so the cluster decides; B.2.2's paired run at 1 against 3 |
| `CoreLmpExemptChecks` | 0/1 | 1 | direct quiet checks survive the count skip; pulled only if a quiet-mate canary fails |
| `CoreLmrNegativeAllowed` | 0/1 | 1 | the `newDepth + 2` ceiling (negative reduction) against a hard `newDepth` ceiling |
| `CoreTtCutoffNodeTyped` | 0/1 | 1 | the donor's `cutNode == (value ≥ beta) || depth > 4` admission against bound-and-depth alone |
| `CoreCorrAdmission` | 0/1 | 1 | the donor's `(best > eval) == bool(bestMove)` admission against the legacy `best ≥ beta || best > alpha` |
| pawn-history buckets | 2048 / 8192 | 8192 | a size: B.2.2's time-to-depth and pooled NPS, not Elo |

Killer removal is not a switch: the donors have neither killers nor
countermove, B.0 §6 dropped them with the cluster under rule 3, and this
review does not ask for the categorical run that would make them one.

### 5.11 Tests

New, compiled on the ON arm (`test_history_core`, `test_move_picker_core`)
or both (`test_threats`, `test_tt` additions), all under `_ALL_TARGETS` so
the sanitizer runs them:

1. Every core table: a bonus never exceeds the limit (saturation at +limit
   and −limit after 10,000 updates), gravity is monotone, `clear()` zeroes,
   the per-search decay is the registered factor, the continuation and
   correction row pointers address the same entries as the full index (the
   shape of `test_history_row_base_decomposition`).
2. Picker exhaustiveness and the skip-quiets contract (§5.7).
3. TT (§5.6).
4. Threats against brute force (§5.8).
5. Stack unwind: a search of 20 fixed positions at depth 8 on a `Board`
   passed by value leaves the caller's board unchanged, returns legal best
   and ponder moves, and a second identical search returns identical nodes
   and lines (determinism at one thread); the same at `Threads 4` with the
   pool's result legal (the existing threading tests cover the pool).
6. Both arms: `test_search`'s mate-in-one, only-move, in-check, node-limit,
   corrupt-TT-move and tablebase cases; the strength floors (WAC, KBNK, KQK,
   KRK, KBBK, the mate band).

### 5.12 Invariants restated at the seams

- The off arm is the legacy kernel byte for byte in behaviour: 14,978,465 at
  every commit on PEXT and plain, Tune, Diag, Ablate and the sanitizer build
  agreeing with release at `bench 10`.
- DESIGN §3: the mate band, ply adjustment on store and probe, rule-50
  semantics (the ON arm's downgrade form), TT-move validation at every
  consumer, the empty-slot rule (§5.6's occupancy test), one mutation seam
  (`do_move`/`undo_move`, now in the kernel section).
- Nothing pruned or reduced at the root, in check, or before one move is
  searched; the reduced depth at least one ply; a legal PV and best move from
  `root_search` whenever a root move raised alpha.
- Correction trains only from nodes not in check, not under exclusion, not
  decisive, and admitted by §3.3's rule; the TT stores the raw static
  evaluation only.
- Diagnostics observe and never decide; the trace and counters compile out of
  release.
- Per-node code allocates nothing (the threat set is a struct of four
  bitboards on the stack; the picker's buffer is the caller's).

### 5.13 Cheap qualification at every ticket

Off arm: exact bench on PEXT and plain, `ctest` release and sanitizer.
ON arm: builds, `ctest` both suites, `bench 13` recorded per ticket (it
changes), the liveness proof once the bits are re-declared, the four traces
before B.2.2, §10's zero-game rows at the end. The pooled NPS and
time-to-depth are B.2.2's (P5 against B.1's 4.216M).

## 6. The B.2.0 NPS pool (BAS-P17), registered

Two final-PGO Release builds of the B.2.0 head `9b51b46` by `build_test.ps1`
(`basilisk-b20-pgo1-pext-pgo.exe` `FD7384FC…A134`, `basilisk-b20-pgo2-pext-pgo.exe`
`18AED349…7811`, each verified at 14,978,465 from a clean tree) against
BAS-P16's two B.1 builds (`FC797381…0C96`, `6A7BB438…B56D`), through
`tools/run_b20_nps_gate.ps1`: a self pair over the two B.2.0 builds, then
the pair against the pair; `nps_ab.ps1` at `bench 13`, 16 alternating
rounds × 3 repeats, one pinned physical core at High priority, behind the
idle-host guard. BAS-P16's recipe.

**Prediction, frozen before the run:** 0.0%, 80% interval [−0.6%, +0.6%],
confidence moderate-high. Per-node code did not change: the kernel moved to
its own translation unit (LTO inlines across it as before), the driver calls
it once per iteration, the counter and parameter tables generate the same
fields, and the TT constants compile to the same instructions. The one way
to a measurable delta is code layout, as BAS-P16's +1.21% was.

**Reading:** a failed self pair voids the run. Above −0.5% the leaf closes
and B.1's 4.216M stays B.2's reference; at or below −0.5% the split is the
suspect and is located by building `c434ee4` alone against its parent; above
+0.5% passes and is recorded unexplained.

**Result (2026-10-10, maintainer-run):** 0.00%, 95% CI [−0.22%, +0.28%],
self pair −0.03% [−0.26%, +0.15%] OK, host idle, fingerprints identical;
pooled medians 4.190M against 4.190M in this session (BAS-P16's 4.216M for
the same B.1 builds is session drift, which the interleaving cancels). The
leaf closes; B.1's 4.216M stays B.2's NPS reference. Calibration: inside the
80% interval at its centre.

## 7. Handed to owners

| Owner | Item |
|---|---|
| B.2.1 | §5 in full: the fence, `STACK_SENTINELS` 7, the record (§5.4), tables (§5.5), TT encoding (§5.6), picker (§5.7), threats (§5.8), counters and trace with their consumers (§5.9), coordinates and switches (§5.10), tests (§5.11); the `+b2core` version suffix; the ON-arm CI job |
| B.2.2 | the pawn-history bucket decision by time-to-depth; the categorical paired runs of §5.10 |
| B.2.3 | `generate_spsa_surface.py --table core` before the surface is registered (F15) |
| B.3 | `allow_null` to the `nmpMinPly` form; the check policy; the direct-check exemption's fate if the canary speaks |
| B.4 | quiescence's fail-hard form and stand pat; the first-ply quiet checks |
| B.7 | threat producer placement, pawn-history footprint, any `gives_check` cost left in the core move loop |
| B.8 | the legacy kernel files, the legacy parameter table, the `#else` arms, the option; `hist_below_*` |
| C.1 | `eval.cpp` and `eval_params.h` comments (119 references) |
| D.2 | shared histories across threads (the donor's `SharedHistories`); BAS-C05 stands |
| D.3.1 | the board's 26 references and its public surface |
| E.1 | re-run `source_census.py` on the B.9 head against `tools/results/b20-20261010/census_after.txt` |

## 8. Artifacts (`tools/results/b20-20261010/`, ignored storage)

| File | Content |
|---|---|
| `census_b1_head.txt/.json` | the census of `8c706a8`'s `src/` (from `git archive`) |
| `census_after.txt/.json` | the census of the B.2.0 head |
| `tune_options_ref.txt`, `tune_options_new.txt` | the Tune `uci` option lists diffed in §2.1 |
| `diag_trace_ref.txt`, `diag_trace_new.txt` | the four-search `Diag` and trace outputs diffed in §2.1 |
| `b20-nps/` | BAS-P17's transcript and results once run |
