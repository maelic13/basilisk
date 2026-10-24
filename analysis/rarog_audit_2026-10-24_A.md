# Rarog audit 2026-10-24, appendix A: correctness, protocol, time, SMP, tablebases

The correctness auditor's full report, verbatim (read-only audit of Rarog since 2026-08-11 against Basilisk at `ed3132f`). The index and the coordinator's verification notes are in [`rarog_audit_2026-10-24.md`](rarog_audit_2026-10-24.md).

## Part 1: Items

Status words: **FIXED**: Basilisk already does the right thing. **PRESENT**: the defect or the gap exists in Basilisk. **PARTIAL**: partly done. **PLANNED**: a PLAN leaf already owns it. **NOT-APPLICABLE**: doesn't apply to Basilisk. **UNVERIFIED**: not settled by reading.

### Board, SEE, FEN and move parsing

- **A-1. SEE stops an exchange early when a king is among the next attackers**
- Rarog source: board_audit_2026-09-05 §A; see_contract_2026-09-06; see_repair_2026-09-06 (RAR-M27/M28); CHANGELOG 2.4.0 Fixed.
- Finding: Rarog broke the exchange when the opposing king merely appeared in the attacker set, even after a pawn capture. Example: `7k/8/2p5/3pK3/8/8/3R4/8 w`, move d2d5. The correct value is −300, but Rarog gave −400 and `see_ge(0)` true. Rarog's fix tests king legality only when the king is the chosen recapturer, checked against 41 fixtures from python-chess.
- Category: board/SEE
- Basilisk status: FIXED
- Basilisk evidence: `src/board.cpp:1747` and `:1878` (the king is tested only when selected, against an opponent set without the pin filter). BAS-C08.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-2. SEE ignores pins created partway through an exchange**
- Rarog source: see_contract / see_repair (fixtures "pin created on a later exchange", "pinned knight skipped for a legal rook"); B.7.3 (an exact recapturer cost −2.09% NPS and was reverted).
- Finding: Rarog now tests each candidate recapturer's king safety against the current exchange occupancy.
- Category: board/SEE
- Basilisk status: PRESENT (an accepted, documented approximation).
- Basilisk evidence: `board.cpp:1702` and `:1840` compute `see_pins` once, after the mover leaves, so a pin opened by the mover's own departure is handled but a pin opened by a later capture is not. BAS-C09 measured 0 of 339,607 bench calls affected; BAS-S21 re-read it at 0.0004%.
- Suggested Basilisk owner: BAS-C09's retry trigger; D.3.1.
- Confidence: high

- **A-3. SEE values recapture promotions wrongly**
- Rarog source: see_contract (Rb2xb1 axb1=Q = −800); see_repair (a quiet Rb2-b1 actually loses 1,300).
- Finding: Rarog adds the promotion gain and the new occupant's value.
- Category: board/SEE
- Basilisk status: PRESENT (accepted approximation; the two errors cancel when the promoted piece is recaptured).
- Basilisk evidence: `board.cpp:1882` uses `SEE_VALUES[next_attacker]` with no promotion term. BAS-C09. Note that B.2's quiet SEE now sends quiet moves through this loop, which raises its reach (482 of 119,732 quiets diverge in BAS-C09's 2026-10-10 note).
- Suggested Basilisk owner: BAS-C09 retry trigger.
- Confidence: high

- **A-4. SEE contract for special moves and an inventory of its callers**
- Rarog source: see_contract_2026-09-06.
- Finding: for each move class (quiet, quiet promotion, en passant, castling) decide whether callers want "admit this move" or "the exchange value", and list every caller.
- Category: board/SEE
- Basilisk status: FIXED in behaviour; the written caller inventory is not in a contract document.
- Basilisk evidence: `board.cpp:1802-1819`: quiet moves and quiet promotions go through the exchange; castling answers from the gain.
- Suggested Basilisk owner: D.3.1.
- Confidence: medium

- **A-5. Self-consistency tests are not correctness oracles**
- Rarog source: RAR-C01; see_contract (an independent python-chess legal-capture-tree oracle); movegen_2026-09-07 (an independent mailbox ray-walker that checks pins).
- Finding: implementations sharing the same omission pass each other's tests. Important invariants need an oracle that doesn't share the engine's code.
- Category: test gap
- Basilisk status: PARTIAL.
- Basilisk evidence: `tests/movegen_oracle.h` and a promotion-aware SEE oracle exist, but they are in-repo C++. The board-v2 external corpus is not ported.
- Suggested Basilisk owner: D.3.1, which already names the board-v2 corpus.
- Confidence: medium

- **A-6. A malformed non-ASCII UCI move token crashed Rarog**
- Rarog source: board_audit §B; HISTORY 4.11b.3; CHANGELOG 2.4.0.
- Category: robustness
- Basilisk status: FIXED
- Basilisk evidence: `parameters.cpp:63-72` matches the token as a string against `move_to_uci` of each legal move, with no indexing. Test sections "15.0.d: non-ASCII move token is rejected", "stray UTF-8 continuation bytes".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-7. The fullmove counter overflowed (debug panic, release wrap)**
- Rarog source: board_audit §C.
- Category: robustness
- Basilisk status: FIXED
- Basilisk evidence: `board.cpp:378-383` bounds halfmove at 32767 and fullmove at 100000; `int` storage. Tests "fullmove above the 100000 bound is refused" and "fullmove zero tolerated".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-8. A FEN in triple check was accepted**
- Rarog source: CHANGELOG 2.5.0; commit 2e1266f.
- Category: robustness
- Basilisk status: FIXED
- Basilisk evidence: `board.cpp:543-545`; `test_board.cpp:253-254`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-9. Upper-case UCI move text is refused, and Basilisk then exits**
- Rarog source: tests/uci_process.rs `position_accepts_uppercase_move_text_before_perft`; search_options.rs `search_options_accept_uppercase_uci_move_text` (Little Blitzer artifacts).
- Finding: Rarog accepts tokens like `E2E4` and `e7e8Q`.
- Category: protocol/robustness
- Basilisk status: PRESENT
- Basilisk evidence: `parameters.cpp:67` compares `move_to_uci(move) == token` exactly, and `move.cpp:20-22` emits lower case only. A rejected `position` is fatal by design (`engine.cpp:432-445`, `std::_Exit(1)`), so a GUI sending `e7e8Q` ends the process.
- Suggested Basilisk owner: D.3.2.
- Confidence: high

### Draw rules, repetition and rule 50

- **A-10. A rule-50 draw could override a mate**
- Rarog source: RAR-C02; draw_policy_2026-09-08 policy 1; tests/draw_semantics.rs.
- Category: correctness
- Basilisk status: FIXED
- Basilisk evidence: `board.cpp:1529-1536` (mate precedence at clock 100). Tests "checkmate at clock 100: not a draw", "in check at 100 with evasion: draw", "clock 99->100 quiet mate".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-11. Rule-50 mate test in check allocated on the heap**
- Rarog source: commit 6433760; PLAN D.3.
- Category: robustness
- Basilisk status: NOT-APPLICABLE
- Basilisk evidence: `board.cpp:1534` uses a stack `MoveList`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-12. Null moves, the halfmove clock and repetition across a null**
- Rarog source: draw_policy policies 2 and 3; RAR-S18 (the null-clock and cross-null-fence bundles read −7.21 and −11.91 Elo).
- Finding: Rarog keeps the null move advancing the clock and has no fence. Crossing a null can only cause false negatives in repetition detection.
- Category: lesson
- Basilisk status: NOT-APPLICABLE (Basilisk's opposite policy is deliberate).
- Basilisk evidence: `board.cpp:1559-1581` (`min(halfmove_clock, plies_from_null)`); test "make_null_move keeps halfmove_clock"; BAS-C02.
- Suggested Basilisk owner: D.3.1's separate policy audit.
- Confidence: high

- **A-13. A stored TT win must never answer a position that repeats the game history**
- Rarog source: basilisk_review_2026-10-06 item 3; test `a_stored_win_never_answers_a_position_repeating_the_game_history` (0827ab9).
- Category: correctness / test gap
- Basilisk status: FIXED in code; the specific regression test is missing.
- Basilisk evidence: the draw check runs before the TT probe at `search_kernel_core.cpp:121` (qsearch) and `:344` (negamax); the root is PV so it never takes a TT cut.
- Suggested Basilisk owner: D.3.1 (add the test).
- Confidence: high

- **A-14. The halfmove clock must stay out of the position identity**
- Rarog source: draw_policy policy 4; test `the_halfmove_clock_is_not_part_of_the_position_identity`.
- Category: test gap
- Basilisk status: FIXED in code; no explicit test.
- Basilisk evidence: `board.h:137` (`position_key()` returns `hash`, no clock).
- Suggested Basilisk owner: D.3.1.
- Confidence: medium

### Mate scores, tablebase band and TT

- **A-15. The TT's rule-50 downgrade of a mate must leave the decisive band**
- Rarog source: b52_research P3; 84b1f74.
- Category: correctness
- Basilisk status: FIXED in B2_CORE; the legacy kernel returns 0 at clock 100 or more.
- Basilisk evidence: `tt.h:291-310` (downgrades to `DECISIVE - 1`).
- Suggested Basilisk owner: B.8 (legacy removal).
- Confidence: high

- **A-16. The tablebase band and probes**
- Rarog source: b52 P2/P3 (the band was empty; probes were always stored exact); RAR-S94.
- Category: tablebase
- Basilisk status: FIXED
- Basilisk evidence: A.8.13/A.8.14 and BAS-S19; `search_kernel_core.cpp:351-365` (zeroing-move condition, bounds, depth+6 store); `search_worker.cpp:277-278` (in-search probes off at a DTZ root).
- Suggested Basilisk owner: none.
- Confidence: high

- **A-17. Known-win evaluations must stay below the tablebase band**
- Rarog source: basilisk_review item 2.
- Category: correctness
- Basilisk status: FIXED (A.8.14).
- Basilisk evidence: PLAN A.8.14 / A.8.19.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-18. Null-move pruning promoted an unproven mate score to a cutoff**
- Rarog source: CHANGELOG 2.3.2 Fixed.
- Category: correctness
- Basilisk status: FIXED
- Basilisk evidence: `search_kernel_core.cpp:670` (`if (is_decisive(null_score)) null_score = beta;`).
- Suggested Basilisk owner: none.
- Confidence: high

- **A-19. An aspiration re-search on a mate score might never end**
- Rarog source: RAR-C04.
- Category: correctness
- Basilisk status: FIXED
- Basilisk evidence: `search_worker.cpp:442` (full window at a mate centre); `:504-511` (window opens fully at `delta >= 900`).
- Suggested Basilisk owner: none.
- Confidence: high

- **A-20. A per-ply stack field was written on one branch only, so it went stale**
- Rarog source: code_audit #1 (`stack[ply].reduction`, −7.2% bench nodes once repaired).
- Category: correctness
- Basilisk status: FIXED
- Basilisk evidence: `search_kernel_core.cpp:423-424` (the parent's reduction is consumed and reset), `:462` (`CORE_NODE_CHECK(ss->reduction == 0)`), `:1149`/`:1156`; `tests/test_stack_unwind.cpp`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-21. `improving` was forced false two plies after a check**
- Rarog source: code_audit #2.
- Category: correctness (heuristic input)
- Basilisk status: FIXED
- Basilisk evidence: `search_kernel_core.cpp:480`, `:527` (the eval is carried from two plies up).
- Suggested Basilisk owner: none.
- Confidence: medium

- **A-22. The evaluation's colour-symmetry sweep should include promoted material**
- Rarog source: PLAN-closed-2026-10-22 C.13.1; CHANGELOG 2.6.0 Fixed (connected rooks with three rooks scored asymmetrically; 12 asymmetric evaluations found with the lazy gate open); tests/eval_invariants.rs over a 2,000-row versioned sample.
- Category: test gap
- Basilisk status: PARTIAL.
- Basilisk evidence: `tests/test_eval.cpp:124-145` mirrors only 5 FENs, with no promoted-material or three-rook rows.
- Suggested Basilisk owner: C.1 or C.0 (or D.3.1).
- Confidence: high

- **A-23. Game-history capacity and the headroom a cloned worker inherits**
- Rarog source: history_contracts_2026-09-08.
- Category: robustness
- Basilisk status: FIXED
- Basilisk evidence: `board.h:72` (`HISTORY_RESERVE = 2048`, capacity preserved on copy); test "history grows past the reserve and unwinds exactly".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-24. Board-v2 corpus, per-piece relocation test, size guards**
- Rarog source: relocation_2026-09-07 (`ordinary_relocation_updates_and_restores_every_piece_class`); representation_2026-09-08 (compile-time size asserts); movegen ray-walker.
- Category: test gap
- Basilisk status: PLANNED
- Basilisk evidence: PLAN D.3.1.
- Suggested Basilisk owner: D.3.1.
- Confidence: medium

- **A-25. Engine-thread stack in very deep searches**
- Rarog source: uci_process.rs `long_endgame_search_does_not_overflow_engine_thread_stack`.
- Category: robustness / test gap
- Basilisk status: UNVERIFIED (looks safe).
- Basilisk evidence: release frames are about 1.4 KB a ply (`CMakeLists.txt:541-548` comment); the search runs on the default-stack `std::thread` at `engine_entry.cpp:49`. There is no deep-line process test.
- Suggested Basilisk owner: D.3.2.
- Confidence: low

### UCI output and process interface

- **A-26. After a stopped iteration, the last `info` line can name a different move from `bestmove`**
- Rarog source: commit 97ea52d (2026-10-01); PLAN D.3; test `a_stopped_fail_high_line_is_followed_by_the_line_of_the_move_played`.
- Finding: an iteration stopped after a root fail-high on an unconfirmed move leaves that move's `lowerbound` line last. `bestmove` is then the previous depth's move. Rarog now records the move its last line named and prints the result's line whenever they differ.
- Category: protocol
- Basilisk status: PRESENT
- Basilisk evidence:
  - `search_worker.cpp:450-459`: `bound_line` prints a lowerbound line with the fail-high PV (`pv_table[0]`) once more than 3 s have elapsed.
  - `:515`: the stopped iteration is discarded.
  - `search_root.cpp:211-214`: `needs_pool_line` compares against thread 0's *result*, not its last printed line.
  - `search_thread_pool.cpp:193-194`: at Threads 1 nothing is printed after the search.
  - It triggers on a hard-limit or GUI stop after a 3 s fail-high on a new move (long time controls, analysis, ponder). A.8.17's stop is consistent, because it fires only when the fail-high move is the previous best.
- Suggested Basilisk owner: new leaf in Phase D (D.3.2), output-only.
- Confidence: high

- **A-27. SMP: the pool's chosen line before `bestmove`**
- Rarog source: CHANGELOG 2.5.0; da6d8f9; rarog#1.
- Category: protocol
- Basilisk status: FIXED (A.8.7).
- Basilisk evidence: `search_thread_pool.cpp:270-296`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-28. A mated or stalemated root prints one depth-0 line**
- Rarog source: 5cba881.
- Category: protocol
- Basilisk status: FIXED (A.8.8).
- Basilisk evidence: `search_worker.cpp:384-397`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-29. `nps` within the first millisecond, and `bench` nps at 0 ms**
- Rarog source: 45779ee; 0827ab9.
- Category: protocol
- Basilisk status: FIXED (A.8.8).
- Basilisk evidence: `search_root.cpp:182`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-30. seldepth per iteration; `multipv 1` on every line; single-PV bound lines**
- Rarog source: 5a8d58b, 45779ee, 8a76826.
- Category: protocol
- Basilisk status: FIXED (A.8.8).
- Basilisk evidence: `search_worker.cpp:436`, `:450-460`; tests "uci info: seldepth resets…", "every search line carries multipv 1".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-31. A tablebase root score should print without a bound**
- Rarog source: 488d34a.
- Category: protocol
- Basilisk status: PRESENT (minor).
- Basilisk evidence: `search_worker.cpp:450-459` prints the raw bound score with `lowerbound`/`upperbound` even at a tablebase root, while the final line shows the tablebase display score. This happens only after 3 s.
- Suggested Basilisk owner: D.3.2, with A-26.
- Confidence: medium

- **A-32. `SyzygyPath <empty>`**
- Rarog source: 1e725fe.
- Category: protocol
- Basilisk status: FIXED
- Basilisk evidence: `parameters.cpp:356`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-33. MultiPV: later lines one ply shallower; a stopped depth filling lines from fail-low records**
- Rarog source: HISTORY B.2.0.2.
- Category: correctness
- Basilisk status: NOT-APPLICABLE / FIXED.
- Basilisk evidence: IIR is non-PV only (`search_kernel_core.cpp:727-728`); no root store for lines 2..N (`:1349`); a line cut by a stop is not reported (`search_worker.cpp:573-585`).
- Suggested Basilisk owner: none.
- Confidence: medium

- **A-34. Command-line arguments run as commands; an unknown argument exits 2; `help`**
- Rarog source: CHANGELOG 2.4.0 Added.
- Category: protocol
- Basilisk status: FIXED (A.8.10).
- Basilisk evidence: `engine_entry.cpp:50-56`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-35. Fatal errors reported on stdout from every thread**
- Rarog source: CHANGELOG 2.4.0 ("a panic is now reported on stdout"; Rust's panic hook covers every thread).
- Category: robustness
- Basilisk status: PARTIAL.
- Basilisk evidence:
  - `main.cpp:86-93` catches only exceptions on the main (UCI) thread.
  - The engine thread (`engine_entry.cpp:49`) and the helper threads have no handler and no `std::set_terminate`, so they abort silently.
  - A concrete source: `tt.h:130-131` (`make_unique` under `setoption Hash`; the advertised maximum is 33,554,432 MB) throws `bad_alloc` on the engine thread, after `clusters_.reset()`.
- Suggested Basilisk owner: D.3.2.
- Confidence: high

- **A-36. Start-up CPU messages and the slow-PEXT advisory**
- Rarog source: CHANGELOG 2.4.0 (CPU tier advisory).
- Category: robustness
- Basilisk status: PARTIAL.
- Basilisk evidence: `main.cpp:64-80` writes an incompatible-CPU message to stderr only, which harnesses don't record. There is no advisory for slow-PEXT CPUs (Zen 1/2).
- Suggested Basilisk owner: G.2.
- Confidence: medium

- **A-37. Invalid `setoption` and `go` values keep previous values; overflow is ignored**
- Rarog source: tests/search_options.rs.
- Category: robustness
- Basilisk status: FIXED
- Basilisk evidence: `parameters.cpp:39-50`, `:285-296`; test "go depth overflowing int64 is ignored".
- Suggested Basilisk owner: none.
- Confidence: high

### Ponder

- **A-38. A `ponderhit` written together with `go ponder` was lost**
- Rarog source: ponder_race_report; ef1a24b; B.3.5.
- Category: protocol
- Basilisk status: FIXED (BAS-C10).
- Basilisk evidence: `engine.cpp:290-301`; test "ponderhit during search setup is kept".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-39. A `stop` written together with `go ponder` dropped the whole `go`**
- Rarog source: same.
- Category: protocol
- Basilisk status: FIXED (Basilisk never had it).
- Basilisk evidence: `engine.cpp:272-276` and `:284-288` (exactly one `bestmove` per `go`).
- Suggested Basilisk owner: none.
- Confidence: high

- **A-40. A stray `stop` or `ponderhit` must not reach a later `go`**
- Rarog source: tests `stop_with_no_search_does_not_stop_the_next_go`, `ponderhit_never_converts_a_later_ponder_search`.
- Category: protocol
- Basilisk status: FIXED
- Basilisk evidence: `engine.cpp:298-301`; tests "previous stop state is cleared", "previous ponderhit does not end the next ponder".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-41. `ponderhit` after the time is spent must not restart the clock**
- Rarog source: test `ponderhit_after_spent_movetime_does_not_restart_search_clock`.
- Category: time
- Basilisk status: FIXED
- Basilisk evidence: `search_worker.cpp:208-218`; test "ponderhit: normal search keeps ponder elapsed time".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-42. No ponder credit (ADR-0065 checklist)**
- Rarog source: RAR-R14 checklist.
- Category: time
- Basilisk status: PRESENT (absent mechanism; ponder is off in rating play).
- Basilisk evidence: `search_worker.cpp:213`.
- Suggested Basilisk owner: D.1.
- Confidence: high

### Time management

- **A-43. The clock origin is the instant `go` is parsed**
- Rarog source: RAR-R11; time_forfeit_2026-09-09.
- Category: time
- Basilisk status: FIXED
- Basilisk evidence: `search_worker.cpp:356-363` (`go_recv_time`).
- Suggested Basilisk owner: none.
- Confidence: high

- **A-44. `Move Overhead` is multiplied by (2 + movestogo)**
- Rarog source: RAR-R12 (Move Overhead 40 read −80.85 Elo); BAS-E57.
- Finding: overhead is not a last-moves reserve; raising it starves every move.
- Category: lesson
- Basilisk status: NOT-APPLICABLE (already known).
- Basilisk evidence: `search_worker.cpp:80-81`.
- Suggested Basilisk owner: D.1.
- Confidence: high

- **A-45. No sub-second horizon contraction, so the clock settles near the increment**
- Rarog source: RAR-R14; d12_tm_research; RAR-R17 (horizon 100 against 53 read −0.9 ± 8.0 Elo, with 443 against 0 of 2,000 games going under 100 ms of clock).
- Finding: assuming fifty moves to go at every clock drifts a `3+0.03` game to about 155 ms of clock. 22–41% of games touch 100 ms. Stockfish's `mtg = time_ms/20` lifts this to about 400 ms. Rarog's forfeits are 69–129 ms OS holds at 70–163 ms of clock, which no reserve covers.
- Category: time
- Basilisk status: PRESENT
- Basilisk evidence: `search_worker.cpp:77` (`mtg = 50.0` in sudden death). The formula is Rarog's port, so the same drift is expected.
- Suggested Basilisk owner: D.1.
- Confidence: high

- **A-46. Reserve and hold arithmetic; four threads wait for helpers before `bestmove`**
- Rarog source: RAR-R14 (holds of 80 ms or more on 0.11–0.15% of moves at 1T and 0.4% at 4T, where the move waits for every helper); RAR-R02.
- Category: time / SMP
- Basilisk status: PRESENT (same design, stated as a lesson).
- Basilisk evidence: `search_worker.cpp:121-124` (reserve `2·overhead + 30` above one thread); `search_thread_pool.cpp:232-240` (join before `bestmove`).
- Suggested Basilisk owner: D.1, D.2.
- Confidence: high

- **A-47. Won-ending stall at the hard maximum (rec1 fail-high cascade)**
- Rarog source: RAR-R13; d11; basilisk_review; RAR-R19/R20 (on Rarog's PGO arm, rec3 at `180000+2000` still hit the maximum through one expensive window started under the target, so which iteration crosses the target depends on speed).
- Category: time
- Basilisk status: FIXED at `60000+600` (A.8.17, rec1 32.3 s → 4.75 s); UNVERIFIED at `180000+2000`.
- Basilisk evidence: PLAN A.8.17 probed one clock only.
- Suggested Basilisk owner: D.1. Add the long-clock probe on the PGO build.
- Confidence: medium

- **A-48. A clock stop on a root fail-high once the target is reached**
- Rarog source: RAR-R18 (removing Rarog's stop-at-target gained +15.5 ± 9.3 Elo; the stop at twice the target recovered +12.2 ± 8.8 and still passed the probe); RAR-R16/R17; a0f9755 (stop retired).
- Finding: the stop fired on 29% of searches at about 1.2× the target. The time it saved was left on the clock (about 1.0 s unused at the end of won games).
- Category: time
- Basilisk status: PARTIAL / risk.
- Basilisk evidence:
  - `search_worker.cpp:485-497` stops once `elapsed >= soft_limit`. That is the raw optimum: the stability, instability and effort scales are not applied (`:636-637`). It fires only when the fail-high move is the previous best.
  - BAS-S20 read −0.7 ± 4.5 Elo (its acceptance rule was not met) and was kept by maintainer decision.
- Suggested Basilisk owner: D.1. Read a threshold of 2× the soft target, and the clock left at the end of games.
- Confidence: medium

- **A-49. No proved-mate stop (rec2)**
- Rarog source: RAR-R13/R14 (Stockfish stops at `mate_in(3)` / `mated_in(2)`); D.1.3's mate stop (three stable mate iterations).
- Category: time
- Basilisk status: PARTIAL (no stall measured).
- Basilisk evidence: `search_worker.cpp:649-650` stops only on a mate in 1 for the side to move, with no "mated in 2" stop. Basilisk's rec2 took 3.8 s (A.8.17).
- Suggested Basilisk owner: D.1.
- Confidence: medium

- **A-50. No guard against a hard stop inside the first iteration; the fallback move is unguarded**
- Rarog source: RAR-R14 checklist; D.1.3 mechanism 6; tests/root_abort.rs.
- Category: correctness / time
- Basilisk status: PARTIAL.
- Basilisk evidence:
  - `search_worker.cpp:515` (`if (state_.stopped && depth > 1) break;`) accepts and prints a partial depth-1 result.
  - `search_root.cpp:153-154` falls back to `first_legal_move`.
  - The first clock poll is at 2,048 nodes (`search_kernel_core.cpp:336`), so this is rarely reached.
- Suggested Basilisk owner: D.1; D.3.2 for the tests.
- Confidence: medium

- **A-51. Freezing deepening after half the target (Stockfish's `increaseDepth`)**
- Rarog source: RAR-R20/R21 (the freeze alone read −7.8 ± 8.7 at equal speed and cut opening time 12%; tuned to 0.58 of the target).
- Category: lesson
- Basilisk status: NOT-APPLICABLE (absent in both; not adopted).
- Basilisk evidence: none.
- Suggested Basilisk owner: D.1.
- Confidence: medium

- **A-52. Root cause of the cascades**
- Rarog source: RAR-R19–R22.
- Finding: root fail-highs return exactly beta (93% on bench) because quiescence returns alpha. The first window is 12 wide whatever the score. A known-win evaluation without a score-scaled window made cascades worse; with `delta + score²/div` the cascade ended. The four-part cluster's gate read H0 at −7.4 ± 7.5.
- Category: lesson
- Basilisk status: PLANNED.
- Basilisk evidence:
  - Basilisk has known-win evaluations up to 27,420 with a fixed starting delta (`search_worker.cpp:445`).
  - Its quiescence is already fail-soft (`search_kernel_core.cpp:179`).
  - B.5 already lists "width from its mean squared score".
- Suggested Basilisk owner: B.5.
- Confidence: medium

- **A-53. Shallower re-search after a root fail-high**
- Rarog source: RAR-R19 (bench 0.61×, didn't end every stall).
- Category: lesson
- Basilisk status: NOT-APPLICABLE (consistent with BAS-D17).
- Basilisk evidence: `search_worker.cpp:463-467`.
- Suggested Basilisk owner: B.5.
- Confidence: high

- **A-54. Measuring time changes**
- Rarog source: RAR-R15/R16/R18 (whole-game time is nearly conserved between two arms on one clock; the "2 Elo per 1%" rule misreads redistributions; read the opening's time per move and the clock left at the end; verify every clock coordinate with a wire check).
- Category: lesson
- Basilisk status: NOT-APPLICABLE (method).
- Basilisk evidence: none.
- Suggested Basilisk owner: D.1 registration.
- Confidence: high

- **A-55. Own clock of 0 (or negative) with no increment gives an unbounded search** (found during this audit, not from Rarog)
- Rarog source: none (Stockfish budgets from `max(1, …)`).
- Category: time
- Basilisk status: PRESENT
- Basilisk evidence:
  - `parameters.cpp:291-293` clamps a negative `wtime`/`btime` to 0.
  - `search_worker.cpp:74` returns with no soft or hard limit when `time <= 0 && inc <= 0`.
  - `build_limits` doesn't mark this as infinite because the other clock is set (`engine.cpp:92-98`), so `go wtime 0 btime 5000` searches to `MAX_SEARCH_DEPTH`.
- Suggested Basilisk owner: D.1.
- Confidence: medium (the code is unambiguous; how often GUIs send this is unknown)

### Tablebases

- **A-56. Start rule for the tablebase PV extension**
- Rarog source: b2c98c8 (a 54 ms cold read lost a game at 58 ms).
- Category: tablebase
- Basilisk status: FIXED (A.8.11).
- Basilisk evidence: `engine.cpp:152-156`.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-57. Cold table reads on the clock (root probes before the search)**
- Rarog source: PLAN D.3 (a read blocked longer than the margin still loses; 151 GB of tables against 128 GB of RAM).
- Category: tablebase
- Basilisk status: PRESENT (the same open hazard).
- Basilisk evidence: `engine.cpp:239-243`: `probe_root_moves` runs `tb_probe_root` plus `tb_probe_root_dtz` after `go`.
- Suggested Basilisk owner: D.4.
- Confidence: medium

- **A-58. Root ranking ignores repetitions already in the game**
- Rarog source: b52 P1 (a repetition since the last zeroing move drops wins to the DTZ band; a move into a threefold or rule-50 draw ranks as DTZ 0).
- Category: tablebase
- Basilisk status: PRESENT (minor).
- Basilisk evidence: `syzygy.cpp:382-385` passes `hasRepeated = false` to `tb_probe_root_dtz`.
- Suggested Basilisk owner: D.4.
- Confidence: medium

- **A-59. Forced DTZ ranking where DTZ equals distance to mate**
- Rarog source: b52 P1 (Fathom kept 19 of 21 moves in KQvK).
- Category: tablebase
- Basilisk status: PRESENT (minor; the search chooses within the group).
- Basilisk evidence: `syzygy.cpp:186` uses Fathom's `tbRank`.
- Suggested Basilisk owner: D.4.
- Confidence: medium

- **A-60. The extension honours SyzygyProbeLimit**
- Rarog source: PLAN D.3 (Rarog's own open defect).
- Category: tablebase
- Basilisk status: FIXED
- Basilisk evidence: `engine.cpp:193-195` passes `syzygy_probe_limit`.
- Suggested Basilisk owner: none.
- Confidence: medium

- **A-61. Fathom thread safety with root DTZ probes beside helper WDL probes**
- Rarog source: b52 P7 (53M probes per run, 0 mismatches).
- Category: tablebase
- Basilisk status: NOT-APPLICABLE / UNVERIFIED on Basilisk.
- Basilisk evidence: `CMakeLists.txt:127` compiles Fathom as C++.
- Suggested Basilisk owner: D.3.2.
- Confidence: low

- **A-62. Fathom emits illegal `popcnt` in the portable build**
- Rarog source: PLAN D.4 (15 illegal `popcntq` shipped in 2.3.0/2.3.1).
- Category: robustness
- Basilisk status: FIXED
- Basilisk evidence: GNU path in `tbprobe.c:118-131`; `CMakeLists.txt:466` (Fathom inside `_ISA_TARGETS`); CI ISA verification.
- Suggested Basilisk owner: none.
- Confidence: high

### SMP, start-up, tests and harness

- **A-63. Start-up stalls: tables built in the first search, TT converted inside a search**
- Rarog source: B.2.8; 9a7b663, c0e6ef7.
- Category: SMP / time
- Basilisk status: FIXED (BAS-C11).
- Basilisk evidence: `engine.cpp:391-407`; test "hash resize and clear happen before go".
- Suggested Basilisk owner: none.
- Confidence: high

- **A-64. Multi-thread diagnostics need one owner (RAR-C03); the TT copy hazard**
- Rarog source: RAR-C03; PLAN D.2.
- Category: SMP
- Basilisk status: NOT-APPLICABLE
- Basilisk evidence: pool diagnostics print after the join (`search_thread_pool.cpp:246-252`); the TT holds a `unique_ptr`, so it can't be copied.
- Suggested Basilisk owner: none.
- Confidence: high

- **A-65. Root-abort sweep tests**
- Rarog source: tests/root_abort.rs (a legal move, no unproven mate, reported depth never beyond a completed iteration, determinism).
- Category: test gap
- Basilisk status: PARTIAL (only "stopped search: returns legal fallback").
- Basilisk evidence: tests list.
- Suggested Basilisk owner: D.3.2.
- Confidence: medium

- **A-66. Allocation guard test**
- Rarog source: B.7.1 (8836013).
- Category: test gap
- Basilisk status: PLANNED
- Basilisk evidence: PLAN B.7.1.
- Suggested Basilisk owner: B.7.1.
- Confidence: high

- **A-67. Fuzzing random `position` and command garbage**
- Rarog source: tests/fuzz_lite.rs.
- Category: test gap
- Basilisk status: PARTIAL ("malformed FEN never crashes").
- Basilisk evidence: test_invariants.
- Suggested Basilisk owner: D.3.2.
- Confidence: medium

- **A-68. Harness defect: Colosseum doesn't count the start position for threefold**
- Rarog source: PLAN-closed-2026-10-22 C.8.4.
- Category: lesson
- Basilisk status: NOT-APPLICABLE to the engine (Basilisk's history includes the FEN root).
- Basilisk evidence: `board.cpp:1550`.
- Suggested Basilisk owner: none.
- Confidence: medium

- **A-69. Basilisk's game terminations in the Colosseum database**
- Source: `colosseum.sqlite` (read-only).
- Category: robustness
- Finding, Super Rating Tournament (2026-09-12..15, `3+0.03`, 1T, no adjudication), nine Basilisk versions 1.2.3–1.10.0, 8,200 games each:
  - Basilisk lost **no game** by time forfeit or crash.
  - All 95 TimeForfeit games involving Basilisk were Rybka 2.3.2a forfeiting.
  - The 3 EngineCrash games were Manta 1.0.0 (60701da6) and Junior Yokohama (82a4b8b4, d51489b8).
  - The only Basilisk losses outside normal terminations are **2 illegal-move losses by Basilisk 1.2.3**: 648d1022-53ad-4a98-a743-ffd10ed4fece (against 1.3.0) and 83f4c40d-6894-4b61-a852-427999d7dab3 (against Manta 1.1.0-dev). Both came as replies to a check.
- Finding, Basilisk 1.10.1 Gauntlet (2026-10-05, 4T, Hash 512, 2,400 games): 0 forfeits. The 5 EngineCrash games ("abandoned") were all Houdini 3: 9a489ccb, 13bb1c2f, b9ac755a, 821b5008, cb4b0051. This run had a 2,000 ms time margin, so its zero forfeits are weak evidence.
- Basilisk status: FIXED for the illegal moves.
- Basilisk evidence: the current `sanitize_search_result` (`search_root.cpp:152-158`; BAS-C03) replaces an illegal `bestmove`.
- Suggested Basilisk owner: D.3.2 (replay the two final positions on the current binary).
- Confidence: high for the counts, medium for the cause.

## Part 2: Coverage

**Rarog sources read**

- **Experiment entries (26):** RAR-R01–R22 in full and RAR-C01–C04 in full (R11–R22 from the batch list; R01–R10 from section 4's index).
- **docs/EXPERIMENTS.md:** sections 4 and 7 in full; sections 8 and 9 read.
- **CHANGELOG:** [Unreleased] (empty), 2.6.0, 2.5.0, 2.4.0, and also 2.3.2 (the 2026-08-11 boundary).
- **analysis/, all 26 listed files:**
  - Read in full (25): board_audit, code_audit_2026_08_19, see_contract, see_repair, see_kernel, see_value_injection, movegen, pin_check_sharing, king_square_cache, representation, relocation, draw_policy, time_forfeit, uci_info_review, ponder_race_report, tb_root_pv, b52_research, d11, d12, d15, basilisk_review, gyatso_read, history_contracts, board_comparison_411b19, archive/basilisk_audit_2026-08-30.
  - Grep-scanned only (1): originality_review_2.6.0, for in-scope terms; it holds licensing material only.
- **Coordinator's extra files:**
  - phase4_6_audit (2026-08-20): scanned; search heuristics only.
  - board_comparison_2026-09-09: grep only; speed.
  - smp_analysis, infra_analysis, board_perft_compare: dated July 2026, outside the window; read headers plus a date grep only. infra_analysis (957 lines) was not read in full.
- **docs/HISTORY.md:** the summary and the record sections in scope (lines 18–130, 276–350, 390–580, 812–835); the rest only through keyword grep.
- **docs/PLAN.md:** Phase D lines 337–715 and 1195–1371 read; lines 716–1195 keyword-scanned.
- **Closed PLAN archives:** PLAN-closed-2026-10-05: A.1–A.9, B.2.5, B.2.8, B.3.5, B.7.1, B.7.3 (partly) and B.8 read, the rest grep. PLAN-closed-2026-10-22: grep plus the C.8.4 and C.13.1 passages.
- **git log since 2026-08-11 (all branches):** 975 commits. I reviewed the subjects of all 252 that touch engine source and inspected the diff of 97ea52d. Work between 2026-08-11 and 2026-09-11 was squashed into "Version 2.4.0", so I covered it through CHANGELOG 2.4.0 and HISTORY.
- **tests/:** all 31 files enumerated with test counts; test names read for 14 in-scope files (uci_process, draw_semantics, root_abort, search_behaviour, search_options, tt, tt_shared, tt_sizing, fuzz_lite, zugzwang, eval_invariants, multipv, syzygy_fixture, board_differential).
- **GitHub issues:** maelic13/rarog has 1 (read in full); maelic13/basilisk has 0; Manta #2 and #4 are listed and were already covered by A.8.
- **Colosseum DB:** both tournaments queried read-only.

**Basilisk sources read:** search_worker.cpp lines 40–692, search_thread_pool.cpp 150–342, engine.cpp 60–494, board.cpp (FEN, draws, SEE), search_kernel_core.cpp (excerpts), tt.h, syzygy.cpp 160–409, parameters.cpp (excerpts), main.cpp, engine_entry.cpp, CMakeLists.txt (excerpts), the test-section lists, PLAN A.8, B.5, B.7, Phase D, BAS-C09, and the A.8 parity review.

**Not done:** no engine runs, builds or probes; everything rests on reading. infra_analysis.md is not fully read (out of window). The legacy kernel (`search_kernel.cpp`) was not audited.

## Part 3: Top risks

1. **A-26:** after a stopped iteration the last `info` line can name another move than `bestmove`, at long time controls and in analysis. It costs no games but misleads GUIs and PGN annotators; output-only fix.
2. **A-55:** an own clock of 0 or a negative value with no increment leads to an unbounded search, which risks a forfeit or a hang under some GUIs.
3. **A-35:** exceptions on the engine or helper threads (for example `bad_alloc` from a large `Hash`) abort the process silently, with nothing on stdout.
4. **A-48:** the A.8.17 fail-high stop fires at the raw optimum. Rarog's version of that stop cost about 15 Elo; Basilisk's gate never resolved −5.
5. **A-45:** with no sub-second horizon contraction, the clock floor drifts to about 155 ms at `3+0.03`, which hurts forfeit robustness on other harnesses.
6. **A-9:** an upper-case move token in `position` makes Basilisk exit fatally.
7. **A-47:** the long-control stall is unverified at `180000+2000`; which iteration crosses the target depends on speed.
8. **A-57:** cold tablebase reads charged to the clock at the root.
9. **A-50 / A-65:** a partial depth-1 result is accepted, and there is no abort-sweep test.
10. **A-22:** the evaluation's colour-symmetry test covers only 5 positions and no promoted material.
