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
| Ablation (`ABLATED(n)`, the Ablate flavour) | B.11's closing leaf removes it after its last read |
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
