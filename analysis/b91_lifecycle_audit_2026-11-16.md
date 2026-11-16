# B.9.1 — Lifecycle and protocol audit and tests

- State / class: closed `READY_FOR_IMPLEMENTATION` for B.9.5; `R2`.
- Owner / date: 2026-11-16, on `master` at `ec9c86d` (B.8's merge). No
  engine source changed; `bench 13` 4,688,537.
- Decision needed: two protocol decisions (§4), the routing of four items
  that are not repairs (§5), then B.9.5.

## 1. Scope and method

Every input PLAN B.9.1 lists was read against the source and, where the
behaviour can be observed, against the binary: the Rarog audit's A-25, A-50,
A-57, A-58, A-61, A-65, A-67, A-69, B-122 and D-73
(`analysis/rarog_audit_2026-10-24*.md`); the code audit's F4
(`analysis/code_audit_2026-10-27.md`); Triumviratus's TR-1 to TR-5, TP-5
and TT-4 (`analysis/triumviratus_review_2026-11-03.md`); the Rarog sweep's
DRIVER-12, PROTO-11, PROTO-24 and TT-7 (`analysis/rarog_sweep_2026-11-04.md`);
the engine audit's `ponderhit` defect (`analysis/b64_engine_audit_2026-11-12.md`
§3.2); the import's F1, F2, F7 and F9 (`analysis/rarog_import_2026-11-11.md`
§3); Rarog's PROTO-12, PROTO-15, DRIVER-15 and D.5.20, and the end-to-end
cases PLAN names.

The binary is the head's release build (Clang, the PEXT tier, no PGO;
`bench 13` 4,688,537), driven over UCI as a process. Each item ends in one of:
a **defect** with a test that fails on the head, a **guard** that pins sound
behaviour end to end, a **decision**, **kept** with its reason, or
**routed** to its owner.

The tests are new and are the leaf's deliverable:

- `tests/uci_process_check.py`, the engine as a process: one case per
  contract, each a CTest entry `uci_<case>`.
- `tests/test_tb_root_rank.cpp`, the root's tablebase ranking through
  `Syzygy::probe_root_moves` on the KQvK fixture.
- A defect's test runs with `--expect-defect` and carries the CTest label
  `known-defect`. It passes only while it prints `KNOWN DEFECT REPRODUCED`
  (`PASS_REGULAR_EXPRESSION`), so it fails both when the defect is repaired
  and when the harness breaks. A repair's commit drops the flag; the case
  then holds the contract. Inverting the exit code alone (`WILL_FAIL`) would
  have counted a harness error as the defect.
- Each defect test was run on the head's release build and on the
  ASan/UBSan debug build, and each reproduced its defect for the reason it
  names (§3). One did not at first: `stop` sent during `bench` in the slower
  debug build arrived before the engine thread had taken `bench` up, so the
  run was cancelled unrun and the case "reproduced" without testing
  anything. The stop and quit cases now wait for the run's first record.
- CTest on the release build: 55 of 55. 17 are known defects on Windows:
  the 15 cases of D1 to D13 (D9 has three) and the two decisions' cases
  (§4); elsewhere 16, since D7's case is then a plain test.

## 2. Verdicts

| Input | Verdict | Test |
|---|---|---|
| DRIVER-12 `go depth N` at Threads > 1 | defect | `uci_depth_limit_threads` |
| A-50 a stop inside the first iteration; A-65 the abort sweep | defect | `uci_abort_sweep` |
| F7 a clock of 0, −1 or unreadable; TR-2's −1 clock | defect | `uci_clock_zero` |
| F4 a bare `go` | decision (§4.1), then defect | `uci_bare_go` |
| F4 `go mate N` | decision (§4.2), then defect | `uci_go_mate` |
| TT-7 Hash rounded down without a word | defect | `uci_hash_rounding_notice` |
| F4 a failed `SyzygyPath` cached | defect | `uci_syzygy_path_retry` |
| PROTO-11 Fathom's `exit` on a table it cannot map | defect | `uci_fathom_unmappable_table` |
| PROTO-24 a non-ASCII `SyzygyPath` on Windows | defect (Windows) | `uci_syzygy_non_ascii_path` |
| b64 §3.2 the game ply at `ponderhit` | defect | `uci_ponderhit_game_ply` |
| F1 `stop` and `quit` during `bench` and `wac` | defect | `uci_bench_stop`, `uci_wac_stop`, `uci_bench_quit` |
| F9 output gone | defect | `uci_closed_stdout` |
| B-122 seldepth without quiescence | defect | `uci_seldepth_qsearch` |
| Leading whitespace and tabs (new, beside TR-2) | defect | `uci_whitespace` |
| A-58 the root ranking ignores the game's repetitions | defect | `test_tb_root_rank` |
| TR-1, TR-2's CR, spaces, long lines and one output lock; terminal roots at Threads 4; `go infinite` waits for `stop`; `setoption` mid-search; end of input during `go infinite` and ponder; a rejected `position` mid-search; TP-5's node count; A-69 | sound | `uci_lifecycle_guards` |
| `ucinewgame` clears everything (Rarog D.6.23) | sound | `uci_new_game_resets` |
| D-73 MultiPV stopped searches | sound | `uci_multipv_stopped` |
| A-67 command fuzzing | sound | `uci_fuzz` |
| PROTO-15, A-61 root probes beside helpers' WDL probes | defect by reading; races only under ThreadSanitizer | `uci_tablebase_threads` (a guard until the cell exists) |
| F2 and TR-5 thread stacks | measured; repair specified | with the repair (§6) |
| A-57 cold table reads on the clock | measured; kept (§5) | none possible |
| DRIVER-15 a join behind a blocked table read | kept, to B.9.4 with A-46 (§5) | none |
| TR-3 MultiPV lines reprinted at a stop | kept (§5) | `uci_multipv_stopped` guards line 1 |
| TT-4 the game ply from a FEN's move number | kept, the donor's form (§5) | none |
| TT-4 `go movetime` scaled | sound: never scaled (`search_worker.cpp:55-61`) | none |
| TP-5 the main PV before helpers; a mate beats depth | sound (`search_thread_pool.cpp:198-199`, `:170-175`; `search_worker.cpp:450`) | none |
| PROTO-12 the file-count notice's path split | sound: `;` on Windows, `:` elsewhere, as Fathom's `SEP_CHAR` | none |
| D.5.20 an untrained PGO kernel; the ThreadSanitizer cell | CI changes, B.9.5's (§6) | none before they exist |
| A-25, TR-4 | B.5's, closed there | — |
| TR-5's arithmetic | answered by B.6.4 and B.6.5 | — |

## 3. Defects

Each with the head's reading. Every line was read on the release build and
again on the debug build.

**D1 — DRIVER-12.** `config_for_thread` gives helpers `infiniteDepth`
(`search_thread_pool.cpp:148-149`) and `merge_results` takes the deepest
result (`:174`). At Threads 4, `go depth 8` printed `info depth 9` in 3 of 12
searches (depth 4 and 5: 1 of 12 each); the comment that the main thread
keeps the depth contract is false. Stockfish answers from the main thread
under a depth limit. The test runs up to 60 searches and stops at the first
deeper line; on the head it fails at the first or second.

**D2 — A-50, A-65.** `if (state_.stopped && depth > 1) break;`
(`search_worker.cpp:703`) keeps a cut first iteration. `go nodes 1` from the
start position prints `info depth 1 … score cp 0` with no PV and plays
`b1a3`, the first legal move (`search_root.cpp:154`); after 1.e4 e5 2.Nf3
Nc6, `go nodes 5` plays `e1e2` at `cp -143` where depth 1 plays `c2c3`.
Neither sf_19 nor current Stockfish guards the first iteration, so this is a
departure Basilisk makes on Rarog's contract: a stopped search answers with
a completed iteration. The sweep stops at 14 node counts on two positions:
the move must be what a fresh `go depth d` plays, d its last printed depth,
and two runs must agree.

**D3 — F7, TR-2's −1.** `has_limit` (`parameters.cpp:315-320`) reads a clock
of 0 as no limit, and `set_search_parameter` floors −1 to 0 (`:337-340`),
so `go wtime 0 btime 0`, `go wtime -1 btime -1` and an unreadable clock are
each a depth-7 search with `hard_ms=0` (TM_Debug). Stockfish treats any
named clock as a clock. The test asks for `0 < hard_ms ≤ 50` on each.

**D4 — TT-7.** `Hash 100` became a 64 MiB table with no notice. The test
asks for an `info string` naming 64 for 100, and none for 128.

**D5 — the cached `SyzygyPath`.** `configure_syzygy` returns when the text
is unchanged (`engine.cpp:105-106`), so files that appear later are never
loaded. The test sends an empty directory, copies the fixture in, sends the
same path again and asks for `Found 1 WDL and 1 DTZ`.

**D6 — PROTO-11.** `map_tb` calls `exit(EXIT_FAILURE)` when the mapping
fails (`tbprobe.c:717-726`); tables are mapped at their first probe. The
test loads the fixture, empties both files, and searches KQ against K: the
process exits 1 with no `bestmove`. A zero-length file at load is refused by
`test_tb` (`:708`), so the exit needs a file that changes or fails to map
after loading: a lost drive, a share or a file replaced.

**D7 — PROTO-24.** Fathom is compiled without `_UNICODE`
(`tbprobe.c:328-340`) and Basilisk has no UTF-8 manifest, so a UTF-8 path
reaches `CreateFileA`. A fixture directory whose name has `č` and `š`
reports no tables. Registered as a known defect on Windows only; elsewhere
the case is a plain test, which passes.

**D8 — `ponderhit`'s game ply** (b64 §3.2). On the move-30 test position
with `wtime 6000 btime 6000 winc 60 binc 60`, a plain `go` budgets
`soft_ms=292`; `go ponder` then `ponderhit` budgets 316 (release) and 301
(debug): the ply of whichever node saw the flag.

**D9 — F1.** `bench` and `wac` run on flags of their own (`bench.cpp`,
`wac.cpp`), so `stop` and `quit` wait for the whole run: 5 s after `stop`
neither `bench 18` nor `wac 12` had ended, and `quit` left the process
running. The repaired run ends promptly and says it was stopped; the tests
wait for the run's first record before stopping it.

**D10 — F9.** With its stdout closed and stdin open, the engine searched on
under `go infinite` for the 5 s the test waits, on Windows; on POSIX, where
`SIGPIPE` is ignored (`engine_entry.cpp:24-29`), writes fail the same way
and nothing reads the failure (`output_sink.h`).

**D11 — B-122.** Quiescence does not raise seldepth. After 1.Ra6+ in
`r5rk/5p1p/5R2/4B3/8/8/7P/7K w`, a depth-1 search proves the mate in 3
through quiescence's evasions and reports `seldepth 1`. Stockfish's
quiescence raises it (`search.cpp`, "Used to send selDepth info to GUI" in
both functions). The test asks for at least 5 plies.

**D12 — whitespace (new).** `dispatch` splits at the first space
(`uci_protocol.cpp:29-31`). `  isready`, `\tisready` and `go\tdepth\t2` get
no answer. The UCI specification's own example accepts any whitespace
before and between tokens.

**D13 — A-58.** `probe_root_moves` passes `hasRepeated = false`
(`syzygy.cpp`, the `tb_probe_root_dtz` call), so every win ranks alike after
the game has repeated. On the KQvK fixture, after Qd2 Ke4 Qc2 Ke5 Qd2 Ke4
Qd1 Ke5, all 20 winning moves share the best rank, where the five with the
shortest DTZ (16 against 18) should. With the 3-6-man tables, replaying
BAS-S39's game 4965 to Black's 77th move (R+N+P against Q, the position
twice before) the head plays 77...Ke5 at depth 10 and 16, after which 78.Qa4
is the threefold the game ended in; without tables it plays 77...Rd2. The
evaluation reads that ending as level, so a draw by repetition scores as
well as the winning moves; ranking by DTZ is what makes progress.

**D14 — PROTO-15, A-61.** In an untimed search, `send_info` extends each line
through the tables (`search_worker.cpp:260-269`), and `extend_pv` calls
`probe_root_moves` and `rank_moves_by_dtz` (`tb_probe_root_dtz`,
`tb_probe_root`) on thread 0 while helpers probe WDL. Fathom documents
`tb_probe_root` as not thread safe (`tbprobe.h:271`). Under a clock only the
final line is extended, after the join (`engine.cpp`,
`publish_tablebase_pv`). `uci_tablebase_threads` runs that path (KQ
against KR, the rook capturable, Threads 4, untimed) and checks the answer;
a race shows only under ThreadSanitizer.

## 4. Decisions

### 4.1 A bare `go`

Recommendation: **search until `stop`, as Stockfish does**; answer
unasked only at the depth cap, as a bare Stockfish `go` does. A bare `go`
reaches depth 7 in about 20 ms today (`constants.h`, `parameters.cpp:266-268`
and `:319-320`), an unrequested `bestmove` to a GUI that waits for `stop`.
Counter-argument: `basilisk go` on the command line would then run until
the cap, since the argument path queues `quit` without `stop`
(`uci_protocol.cpp:85-88`). That path should keep its depth: the repair
gives `RunArguments` a bare `go`'s old meaning or a stop. `test_search.cpp`'s
two assertions of depth 7 change with the decision.

### 4.2 `go mate N`

Recommendation: **search until a mate in N or less is proven, or `stop`**,
as Stockfish does. The cap of `2N − 1` (`parameters.cpp:342-348`) missed the
mate in 6 of `8/8/8/8/8/k7/8/1K1R1N2 w` under `go mate 7`: depth 13 ended at
`cp 19871`, and the mate first shows at depth 15. Counter-argument: an
unbounded `go mate` on a position without the mate searches until `stop`;
that is the command's meaning.

## 5. Kept, with reasons

- **A-57, cold table reads on the clock.** Measured here: the first search
  after loading the 3-6-man tables (`D:/chess/tablebases`, first contact)
  took 4,006 ms to depth 6; the next process, warm, 44 ms to depth 10. No
  engine avoids a page-cache miss: the root probes must run after `go`, since
  the material is known only then, and pre-reading at `position` buys nothing
  when the GUI sends `go` straight after. Stockfish carries the same hazard.
  Recommendation: a known limitation in 1.11.0's notes, with Move Overhead
  as the remedy; read again in B.9.4. Retry trigger: a forfeit with tables
  loaded in any gate.
- **DRIVER-15, a join behind a helper blocked in a table read.** A thread in
  a page fault cannot be cancelled; the pool waits for every helper
  (`search_thread_pool.cpp:234-236`). Answering before the join is A-46's
  question, which B.9.4 reads; it joins it there.
- **TR-3, MultiPV lines at a stop.** A stop prints line 1 of the deepest
  completed search and leaves lines 2 to N at the depth before; Stockfish
  prints them all again. `bestmove` heads the last line 1 at every stop
  (`uci_multipv_stopped`). Cosmetic; kept.
- **TT-4, the game ply from a FEN's move number** (Triumviratus's
  `TmFenPly`). Stockfish's `gamePly` comes from the FEN the same way; a book
  given as FENs starts every game at its counter. A time-management lever,
  D.1's if anywhere.

## 6. Not testable before the repair

- **F2, TR-5: thread stacks.** B.8.5's frames (Clang, release code):
  `negamax<NonPV>` 1,416 bytes, `quiescence<PV>` 1,256, `evaluate_full`
  1,352 at a leaf, `CoreMovePicker::next` 792. A line to the cap of 128 plies
  needs about 128 × 1.4 KB + 4 KB ≈ 0.19 MB, under every default: macOS's
  512 KB for a second thread (2.7×), Windows' 1 MiB (the release
  executable's `SizeOfStackReserve`, which a thread inherits; the debug
  build's is 8 MiB), Linux's 8 MB.
  The engine thread also holds Fathom's root buffer (`TbRootMoves`, 193
  moves) during the root probes, which run at the shallowest level. Debug
  and sanitizer frames are larger and unmeasured; the Windows sanitizer
  link already sets 8 MB (`CMakeLists.txt`, `--stack`). Repair: every search
  thread, the engine thread included, created with an explicit 8 MB, as the
  donor does, and Fathom's buffer moved to the heap. Its test drives a line
  to the cap on a pool thread, which needs the thread-creation hook the
  repair adds.
- **The ThreadSanitizer cell** over the threading, ponder and tablebase
  tests, with one suppression for the table's tolerated race (BAS-C05), and
  **D.5.20**: the ordinary PGO path and the `gcc-pgo` job fail on an
  untrained kernel, as the universal build does. Both are CI changes.

## 7. Interaction map

- **The stop flag and the epoch.** `UciProtocol` bumps `control_epoch` and
  raises `stop_requested` (`uci_protocol.cpp:155-175`); the engine thread
  clears and re-checks it in `start_search` (`engine.cpp:300-303`). D9's
  repair must make `bench` and `wac` read this flag and this epoch, not a
  flag of their own, or a `stop` sent before a run starts will cancel it
  unrun (the race the debug build showed) and one sent during it will not
  stop it.
- **The clock's inputs.** `compute_time_limit` reads the game ply at the
  search's start (`search_worker.cpp:443-444`) and again at `ponderhit`
  (`:212-215`). D8 keeps the start's ply in the search state. D3 changes
  what `go` passes in; `infinite` (`engine.cpp:94-100`) must stay false for
  a named clock.
- **The result choice.** D1 and D2 both touch what the pool answers:
  `merge_results` (a mate beats depth; depth then score), the main
  thread's MultiPV override, and the pool line (`needs_pool_line`). Under
  a depth limit the main thread answers, as in Stockfish; the mate rule
  still applies to the main thread's own result.
- **The tables.** D5, D6 and D7 are in `Syzygy::init` and Fathom's file
  layer; D13 in the root ranking, which the search's root filter reads
  (`engine.cpp:241-265`, best rank only); D14 in the line extension. D13
  changes moves only with tables loaded and a repetition in the game.
- **Output.** D10 needs a write's failure to reach the UCI loop; the same
  sink carries `info`, `bestmove` and `readyok` under one lock
  (`output_sink.h`), which stays.

## 8. Instruments

- TM_Debug's `soft_ms` and `hard_ms` measure D3 and D8 and will measure
  their repairs; `tools/diag/ponderhit_read.py` reads D8 over many
  ponderhits.
- `tools/diag/tb_gate_audit.py conversion` counts clean tablebase wins not
  converted, the game-level reading of D13 (BAS-S39 had one in 2,286).
- The Colosseum database answered A-69: 1.2.3's two illegal moves were
  replies to a check by a double push, which the head answers by en passant
  (`h4g3`, `e4f3`) at Threads 1 and 4.
- Nothing measures D2's exposure in games yet: B.9.5 counts first-iteration
  stops over a gate's worth of searches with a Diag counter before sizing
  any games for it.

## 9. Decision and handoff for B.9.5

`READY_FOR_IMPLEMENTATION`, once the maintainer settles §4. Each repair in
its own commit, through the correctness-repair path; the commit drops its
test's `--expect-defect` (or moves `test_tb_root_rank` to a plain test).

- **Semantics.**
  - D1: under `go depth N` the main thread's result is the answer, and no
    line deeper than N is printed.
  - D2: a stop is honoured only after the first iteration is complete,
    which takes well under a millisecond, so a stopped search answers with a
    completed iteration.
  - D3: a `go` naming `wtime`, `btime`, `winc` or `binc` with any value is a
    clock search.
  - D4: one `info string` when the table is smaller than asked.
  - D5: a path whose load failed or found nothing is tried again when sent
    again.
  - D6: a table that cannot be mapped is skipped with an `info string` and
    the search goes on.
  - D7: wide-character paths on Windows.
  - D8: the root's game ply, kept from the search's start.
  - D9: `bench` and `wac` stop on the engine's flag and say so; `quit` ends
    the process.
  - D10: a failed write ends the engine cleanly on every platform.
  - D11: quiescence raises seldepth at PV nodes.
  - D12: any whitespace separates tokens.
  - D13: the repetition since the last zeroing move is passed to the DTZ
    ranking.
  - D14: at Threads > 1 the line extension waits for the join.
  - §4: the two decisions.
  - §6: the stacks and the two CI changes.
- **Invariants.** `bench 13` 4,688,537 for every repair that does not move
  the tree. D2, D11 and D13 can move the fingerprint only if the bench's
  searches reach them; D2 cannot (bench searches to a depth), D11 is
  display. The protocol's queue order and the epoch's stale-command rule
  (`engine.cpp:274-290`) stay.
- **Qualification.** CTest on the release and sanitizer builds, the known
  defects among them; the guards (§2) unchanged.
- **Games.** Only D2, D3 and D13 can change a move. D2 and D3 need a hard
  limit under the first iteration's time or an empty clock; D13 needs
  tables and a repetition. B.9.5 measures their exposure first, and a
  repair SPRT `[-5,5]` at `3+0.03` runs only if it is not nil, D13's with
  tables loaded.
- **Non-goals.** No time-management change beyond D3 and D8; no change to
  the thread-result choice beyond D1; no Fathom change beyond D6 and D7.

## 10. Artifacts

`tools/results/b91-audit-20261116/` (ignored): the probe script the audit
drove the binary with (`uci_probe.py`), game 4965's start position and moves
(`g4965.txt`, from `tools/results/a1134-tb-gate/games.pgn`), and the CTest
logs of the release build (55 of 55) and of the new tests on the debug
build (24 of 24). The tests reproduce everything else:
`ctest --test-dir build/release -L known-defect --output-on-failure`.
