# B.8.2 — inventory of switches and replaced code (2026-11-14)

The maintainer asked for every switch and every replaced form in the engine
to be inventoried, and for each to be removed unless something needs it
("I am in favour of simplification … challenge me on this"). This packet is
that inventory and its decisions. B.8.2 carries them out. Nothing is deleted
here.

## 1. What decides

- **A switch costs the shipped engine.** Parameters are plain `int` fields of
  `SearchParams` in every build, read at run time through
  `config_.limits.params` (`search_params.h:563`). The header's line saying
  release builds hold "compile-time constants" is wrong. So every switch is a
  branch the release build takes at each node it guards. Deleting switches
  paid +1–6% NPS in Rarog (D-18).
- **A compile-time flavour does not.** Tune, Diag, Pricing, Texel, Ablation,
  the literal screen, the child log, the board trace and the profile symbols
  are compiled out of the release build. They are kept wherever an open leaf
  needs the measurement.
- **A form a later leaf will test is rebuilt there, not kept here.** The form
  is recorded below as it stands at `55e3010` on `master`, with file and line,
  and its leaf rebuilds the switch it needs. H.6's own method builds a switch
  for each donor form it tests, so this costs it little. It also needs to
  re-verify the form, because the code around it will have moved.
- **Effective values.** The values in force are the `X`/`S` lines' defaults
  after the five override tables in `SearchParams`' constructor (B3, B4, B5,
  B67, B6). Only `b3_fail_low_store` differs from its line, at 1 from B.4's
  table.

## 2. Runtime switches: delete, keeping the form in force

The other arm of each is an old Basilisk form, or one measured and not
adopted, and no open leaf reads it.

| Switch (value kept) | Read at (`55e3010`) | Why the other arm goes |
|---|---|---|
| `tt_cutoff_node_typed` 1, `corr_admission` 1, `lmr_negative_allowed` 1, `lmp_exempt_checks` 0 | `search_kernel_core.cpp:945`, `:2115`, `:1819`, `:1464` | B.6.6 row 31: the same as the donor's; the other arms are pre-B.2 forms |
| `pawn_hist_buckets_log2` 13 | `:2165` | row 31; becomes a constant, so the table's index folds |
| `b3_check_prune` 1, `b3_pv_quiet_prune` 1, `b3_nmp_cut_only` 1, `b3_nmp_verify` 1, `b3_probcut_pv` 1, `b3_corr_decisive` 1, `b3_corr_excluded` 1, `b3_cuckoo` 1, `b3_tt_penalty` 1 | `:751`, `:1522`, `:1137`, `:1166`, `:1239`, `:2109`, `:2110`, `:208` and `:698`, `:988` | Cluster 2's gate kept them; the other arms are the kernel before it |
| `b3_iir` 1 | `:1221` | off read −11.3 ± 9.2 (BAS-S41) |
| `b3_fail_low_store` 1 | `:378`, `:641`, `:2138` | row 1: the donor's form, closed |
| `b3_check_ext` 0 | `:725` | on read −22.4 ± 9.7 |
| `b3_nmp_eval_gate` 0 | `:1141` | on read −6.1 ± 9.7 on the fitted arm (BAS-S45) |
| `b3_iir_stale` 0, `b3_multi_cut` 0, `b3_sing_tt_depth` 0 | `:1223`, `:1672`, `:1609` | B.3.2 and B.3.4 read them inside ±5; defaults kept |
| `b3_singular_quiet_lmr` 0 | `:1671` | B.6.4: dead |
| `b4_evasion_prune` 1, `b4_qs_recapture` 1, `b4_stalemate` 1 | `:302` and `:343`, `:502`, `:625` | Cluster 3's gate kept them |
| `b4_qs_delta` 0 | `:438` | the delta rule before Cluster 3; B.6.4: dead. B.8.6 kept it "to restore the old rule", which nothing asks for |
| `b4_gain_values` 0 | `:516` | the two-vector card, not adopted |
| `b4_quiet_checks` 0 | `:580`, `:585` | B.6.4: dead. With it go `Board::gen_quiet_checks` (the engine's only caller), its checks in `test_board_v2` and `test_search_path`, the microbenchmark's `quiet_check_gen` column and the trace's `GEN_QUIET_CHECKS` op |
| `b5_window` 1 | `search_worker.cpp:647`, `:818` | the switch goes. The older aspiration loop does not: it still serves a root record left empty while legal moves exist (a helper whose root filter leaves it no move), so B.6.4's "dead" was wrong for it |
| `b5_root_lmr` 1, `b5_lmr_window_units` 1 | `search_kernel_core.cpp:1806`, `:72` | Cluster 4's gate kept them; row 27 is a unit conversion |

## 3. Switches whose other form a later leaf tests: delete and record

Each leaf rebuilds its form from `55e3010` at the place named.

| Switch (value kept) | Form to rebuild | Read at (`55e3010`) | Its leaf |
|---|---|---|---|
| `tt_cutoff_bonus_cap` 0 | the donor's `min(112·d, 695)` (row 34) | `search_kernel_core.cpp:953` | H.6 |
| `razor_donor` 0, with `B6_DONOR_FORMS` and its table | the donor's razoring, non-PV at any depth, `alpha − 482·d²` (row 30) | `:1062`, `:1065`; `search_params.h:546` | H.6 |
| `corr_nonpawn_kings` 0 | kings in the non-pawn correction keys (row 33) | `search_history_core.cpp:281`, `:307` | H.6 |
| `tt_key50` 0 | the donor's `adjust_key50` (row 32) | `search_worker.h:305` | H.6 |
| `b3_probcut_tt` 0 | the small TT ProbCut (row 3) | `search_kernel_core.cpp:1288` | H.6 |
| `core_history_fills` 0 | the donor's fill values after a clear (row 2); `CoreHistoryTables::clear(true)` stays, since the microbenchmark seeds its histories with it | `:2156` | H.6 |
| `b3_ext_budget` 1 | the donor's form, no budget (row 4); Basilisk's guard stays | `:838`, `:1650` | H.6 |
| `b4_see_values` 0 | the one-vector SEE (row 13); a Tune-only switch | `search_worker.cpp:408`, `Board::set_see_vector` | H.6 |
| `b4_rfp_seek_mate` 0 | reverse futility's seek-mate cap (row 16) | `search_kernel_core.cpp:1108` | H.2, on the network |
| `b5_fail_high_reduce` 0 | the fail-high depth reduction (row 17) | `search_worker.cpp:608` | D.1 |
| `b5_neg_ext_max` 1 | the unbounded ceiling (0) and the bound of 2 (row 18) | `search_kernel_core.cpp:1819` | H.6 (Phase H) |

## 4. Other replaced code: delete

- **`Board::see`.** The engine never calls it; `test_board`'s exact-value SEE
  tests do. Each `see(m) == v` becomes the pair `see_ge(m, v)` and
  `!see_ge(m, v + 1)`, which keeps its information. `see_pins` and
  `see_departure_opens_line` go with it: their only other user is the
  cross-check build.
- **The cross-check build:** the four reference forms (unmake, `see_ge`,
  `is_repetition`, `is_draw`), `PositionSnapshot::operator==` and the
  `BASILISK_BOARD_CROSSCHECK` option. Each was run once, as B.7.6 required.
  B.8.5's shortcuts add their own original-beside-it switch (TM-10) in the
  form B.7.6's commits show.
- **The override tables** `BASILISK_B3_CORE_DEFAULTS`, `_B4_`, `_B5_` and
  `_B67_`: folded into the lines, the value in force becoming each line's
  default, with no behaviour change. B.8.6 kept them because folding moved
  the core surface's start values before B.6.7's joint tune. That tune is
  done, and any later one starts from the values in force, which the folded
  lines then state. `tools/generate_spsa_surface.py --check` is regenerated
  with them.
- **`Board::compute_hash`**, unreferenced.
- **`RELEASE_DIAG_COUNTERS`**, a measurement option no tool, CI cell or open
  leaf uses.
- Already on B.8.2's list and unchanged here: the thirteen zero counters with
  `QsLatePrune` and `QsLate`, `SearchStack::moved_piece`, the default arm's
  `allow_null`, `best_scored` and `did_check_ext`, the release telemetry, the
  idioms and the misleading comments (to which the header's
  "compile-time constants" line is added).

## 5. What stays, and why

| Item | Why it stays |
|---|---|
| Tune and Diag flavours | every SPSA and every counter read; compiled out of release |
| Pricing (`BASILISK_PRICING`) | B.11.1's bridge price for G(0) |
| Texel | C's evaluation fits |
| Ablation (`ABLATED(n)`, the Ablate flavour) | removed at B.11.1's close, 2026-11-18 (§7.5) |
| Literal screen | the next joint tune of the kernel's literals; compiled only on Tune |
| Child log | `test_kernel_pins` |
| Board trace | B.8.5's replay reads |
| Profile symbols, the universal binary and ISA tiers | the profiler and the release |
| Numeric parameters with small ranges (`razor_depth_cap` 1..3) | tunables, not forms |
| `tests/board_performance.cpp` (cross-engine-board-v1) | kept on purpose for its comparability (B.7.2) |

## 6. Expected effect and its read

- **Code:** about forty runtime branches, five override tables, one option,
  and several hundred lines of arms and reference copies leave.
- **Behaviour:** every deletion keeps the form in force, so `bench 13`
  stays 4,247,965. Tests bound to a deleted arm are removed, or re-based
  to the form in force.
- **Speed:** B.8.2's pooled read after the deletions decides. The branches
  are well predicted, so the gain is likely small: a few tenths of a
  percent, not Rarog's +1–6%, which came from dead code paths as well.

## 7. Deletion ledger: what went, where it lives, how to rebuild

Added 2026-11-14, as the deletions landed (maintainer: nothing important for
later development may be lost; say what was deleted and where, so it can be
rebuilt). Every item below is intact at **`55e3010`** on `master`, the last
revision before B.8.2; the lines given are that revision's, read with
`git show 55e3010:<file>`. The branch's working commits stay under the PR's
`refs/pull/<n>/head` after the squash. §2 and §3 already ledger the runtime
switches; this section is everything else.

**What was not lost.** Every deletion kept the form the engine runs: `bench 13`
stays 4,247,965 on Release, Diag and Tune after each step, the bench board
trace is byte-identical (`4C6643CD9CB43460`), and a Tune build's `uci` option
list (every parameter, default and range) is identical before and after the
fold. Every measurement flavour of §5 stays. No result or registered surface
in `tools/results/` or `docs/experiments/` was touched.

### 7.1 Board

| Deleted | At `55e3010` | Rebuild from it when |
|---|---|---|
| `Board::see` (the exact SEE value), with `see_pins` and `see_departure_opens_line` | `src/board.cpp:1932`, `:1877`, `:1914`; declared in `src/board.h` | a mechanism needs the exchange's value, not a threshold (an SEE-scaled margin, an ordering score). `test_board.cpp`'s legal-exchange `oracle_see` is kept and still checks every exact value, through `see_ge`'s boundary |
| The Tune-only second SEE vector (`set_see_vector`, `see_values_`) | `src/board.h:280` | H.6 tests the one-vector SEE (§3, `b4_see_values`) |
| The cross-check build (`BASILISK_BOARD_CROSSCHECK`): `unmake_move_reference`, `see_ge_reference`, `is_draw_reference`, `is_repetition_reference`, `PositionSnapshot::operator==` | `CMakeLists.txt:197`; `src/board.cpp:795`, `:864`, `:1669`, `:1705`; `src/board.h:110` | B.8.5 or a later leaf rebuilds a board operation: B.7.6's commits show the pattern (the original beside the new form, compared at every call) |
| `Board::compute_hash` (the full key from scratch) | `src/board.cpp:111` | a key-consistency check is wanted; `assert_ok()` and the trace replay cover it today |
| `gen_quiet_checks` and its trace hook; the microbenchmark's `quiet_check_gen` column; the quiet-check walks in `test_board_v2` and `test_search_path` | `src/board.cpp:1606`; `tests/board_micro.cpp:190`, `:427` | quiescence searches quiet checks again. The trace keeps the op code `GEN_QUIET_CHECKS` reserved, so old trace files still parse |

### 7.2 Search

| Deleted | At `55e3010` | Why it could go; rebuild when |
|---|---|---|
| Parameter lines read only by deleted arms: `CoreRazorQuadMargin`, `CoreTtCutoffBonusMax`, `B3ProbCutTtMargin`, `B3ProbCutTtDepth`, `B4RfpSeekMateCap`, `B4RfpSeekMateDepth`, the `lmr_singular_quiet` macro | `src/search_params.h:115`, `:166`, `:313`, `:314`, `:408`, `:409` | each comes back with its form in §3 (H.6, H.2), at the default and range that revision gives |
| `negamax`'s `allow_null` parameter | `src/search_kernel_core.cpp:651`, `:766`, `:1137` | it never decided: the null move needs an expected cut node outside an exclusion search, and its only `false` callers are the null child (never a cut node) and the exclusion search. Rebuild if a null move is ever tried at an all node or under exclusion |
| `SearchStack::moved_piece` | `src/search_types.h:52`; `src/search_worker.h:347`, `:358` | written, never read; the continuation row already carries the piece |
| The shallower re-search (Rarog audit B-63) and `CoreResearchShallower` (1, range 1..12) | `src/search_kernel_core.cpp:1866`; `src/search_params.h:93`, `:156` | at margin 1 it cannot fire (`best_score <= alpha < score`; 0 of 45,108 re-searches at depth 11). The donor's margin is larger; rebuild with a margin above 1 if H.6 tests the donor's re-search |
| The override tables `BASILISK_B3_CORE_DEFAULTS`, `_B4_`, `_B5_` and `BASILISK_B67_DEFAULTS` | `src/search_params.h:358`, `:424`, `:472`, `:483` | folded: each line's default is now the value in force, which the tables computed. **The per-cluster fitted values are history, not lost:** each table at `55e3010` lists what that cluster's fit set, and the line's default there is the value before it (Cluster 1's fit for the core lines). An analysis of a fit's movement reads them there |

**Kept on purpose** (checked, not deleted): the older aspiration loop, which
serves an empty root record (§2); the PV re-search's one-ply floor for the TT
move (Rarog B-13, `src/search_kernel_core.cpp:1905` at `55e3010`). The floor
counted 0 in 2.35M nodes at depth 11, but it is reachable at a depth-1 PV node
whose TT move is not searched first (the root with a deep entry carried from
the previous move, or a helper's root order), so it is not dead; the
first-move floor beside it (`:1797`) likewise.

### 7.3 Instruments

| Deleted or changed | At `55e3010` | Notes |
|---|---|---|
| 23 Diag counters nothing wrote: `check_exts`, `lmp_prunes`, `hist_prunes`, `see_prunes`, `sing_in_check`, `sing_triple`, `hist_prune_tested`, `hist_below_half`, `_quarter`, `_eighth`, `lmr_blocked_searched`, `_in_check`, `_movetype`, `_gives_check`, `sing_cap_bound`, `iir_stale`, `probcut_tt_cuts`, `qs_delta_pruned`, `qs_cap_hits`, `qs_late_pruned`, `qs_quiet_checks`, `rfp_seekmate_capped`, `lmr_research_shallower` | `src/search_diagnostics.h:39` (the table), printed by `src/search_diagnostics.cpp` | the table falls from 196 to 173. A rebuilt form brings its counter back at the table's end (the header's rule: extend, never rename) |
| The oracle's side of the contract | `tools/diag/run_suite.py:48` | the oracle (patched classical Stockfish) still prints the fourteen it has; `run_suite.py` reads them as `ORACLE_COUNTER_UNITS` when present, all or none, so the oracle's runs validate as before. Nothing in the oracle changed |
| Trace events `check_extension`, `lmp_prune`, `probcut_tt_cutoff`, `qs_delta_prune`, `qs_late_prune`; price rules `probcut_tt`, `qs_delta`, `qs_late` | `src/search_diagnostics.h:319`; `src/search_pricing.h:36`; `tools/diag/decision_trace.py`, `tools/diag/pricing.py` | never emitted. A trace or price file written before B.8.2 that holds one of these names no longer parses with the current tools: read it with the tools at `55e3010` |
| Release telemetry: `eval_calls`, the pawn cache's probes and hits, the board's SEE and `gives_check` call counts, the lazy-eval audit | `src/eval.cpp`, `src/board.cpp` | **not deleted**: counted in Tune and Diag builds only (`DIAG_COUNT`, now in `types.h`), where every reader runs (Diag output, Pricing, `endgame_search_occurrence.py`) |
| `RELEASE_DIAG_COUNTERS` (counters in a production build) | `CMakeLists.txt:569`, `:695` | measured the counters' own cost; rebuild as a CMake option that defines `DIAG_COUNT` as the expression |

### 7.4 Tests and tools

| Deleted | At `55e3010` | Why |
|---|---|---|
| `test_search`'s `test_arm_defaults` | `tests/test_search.cpp:197` | it checked the override tables; `test_line_defaults` checks each line's range, compiled value and Tune advertisement |
| `test_board`'s `test_see_on_the_second_vector`; `test_board_v2`'s `KNOWN_SEE_PRUNE` | `tests/test_board.cpp:1231`; `tests/test_board_v2.cpp:698` | their subjects are deleted; the exact SEE values are still checked through the oracle and `see_ge`'s boundary |
| `test_b4_qsearch`'s `test_the_rules_the_arm_removes`; `test_run_suite`'s quiet-check test | `tests/test_b4_qsearch.cpp:536`; `tools/diag/test_run_suite.py:118` | they asserted counters that no longer exist |
| The surface generator's table readers (`b3_core_defaults` and the rest) and their tests | `tools/generate_spsa_surface.py:113`, `:194` | every surface now starts from its lines' defaults. The head's seeds are unchanged; the per-cluster surfaces (`config_core.json`, `config_b3.json` and the rest, and the B.3.3, B.4.6 and B.5.4 fit surfaces) now start from the values in force instead of their cluster's start. A rerun of a closed leaf's exact tune uses its registered surface at its own commit |

### 7.5 The ablation build (removed at B.11.1's close, 2026-11-18)

The search froze for C, so the matched ablation mask's last reader is gone.
Everything below is intact at **`4d2742f`** on `master`, and the lines are
that revision's. Bit order, A.5.4's:

- 0 razoring;
- 1 reverse futility;
- 2 null move;
- 3 ProbCut;
- 4 IIR;
- 5 move-loop pruning;
- 6 singular extension;
- 7 LMR, which also covers the first move's step and the step without
  LMR.

| Deleted | At `4d2742f` | Rebuild from it when |
|---|---|---|
| `ABLATED(bit)` and its eight uses in the kernel | `src/search_kernel_core.cpp:39`; uses `:979`, `:1007`, `:1046`, `:1127`, `:1145`, `:1324`, `:1497`, `:1682`, `:1699` (the LMR gate also guarded `:1775`) | a deficit is decomposed again by mechanism against the oracle (PROCESS *Matched ablation*); the oracle keeps its mask (`oracle-hybrid-ablate.patch`) |
| The `AblationMask` option, its field and its limit | `src/parameters.cpp:158`, `:220`, `:452`; `src/parameters.h:45`; `src/search_types.h:123`; `src/engine.cpp:90` | with the macro |
| The `ABLATION` CMake option, its `-ablate` suffix, its flavour exclusions and its PGO pass-through | `CMakeLists.txt:135`, `:655`, `:774`, `:793`, `:1194`, `:1305`; `cmake/pgo-build.cmake:41`, `:89` | with the macro |
| `test_uci_protocol`'s ablation sections | `tests/test_uci_protocol.cpp:210`, `:462` | with the option |
| The Ablate flavour of `build_test.ps1` and CI's ablation job | `tools/build_test.ps1:13`, `:52`, `:112`, `:129`, `:143`; `.github/workflows/ci.yml:355` | with the flavour |

`tools/diag/ablation_liveness.py` stays: it proves a mask's bits live on any
engine that exposes one, today the oracle's ablate build.

C.2.9 (2026-11-24) rebuilt the flavour for the evaluation: the same `ABLATION`
option, suffix, exclusions, PGO pass-through, `build_test.ps1` flavour and CI
job, now exposing `EvalAblationMask` (one bit per evaluation family,
`src/eval.h`). The search's `ABLATED(bit)` uses stay deleted. `bench 13` stays
4,777,412 on Release, Tune and Diag after the removal, and no build exposes
`AblationMask`.
