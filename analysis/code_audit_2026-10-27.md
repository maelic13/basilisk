# Code audit 2026-10-27 — defects, architecture, universal binaries

- State / class: CLOSED (analysis delivered), `R3`
- Owner / date: maintainer's request of 2026-10-26; delivered 2026-10-27
- Source revision: `00809cc` (`master` after A.11.9), `bench 13` = **7,857,713**,
  CTest 24/24 on a clean `release-pext` rebuild of that head
- Decision needed: the Phase A leaves A.13–A.15 this audit adds, the audit
  inputs appended to A.11.10, and the amendments to D.3.2 and G.2

**Recommendation.** Repair the one evaluation defect the audit found (a won
KPK scored as a dead draw, A.13.1) and the one undefined read (A.13.3) before
B.3, give A.11.10 the research inputs below, do the two behaviour-neutral
simplifications of A.14 while the search is quiet, and treat the universal
binary as a measurement first (A.15.2) and a Phase G build second: the
engine's crucial paths read clean, so the cheapest strength is in not fitting
B.3 around the known fail-hard defect, not in a rewrite.

Every claim here is **evidence** (a line read, a count, a program run) unless
marked as an inference or a decision. Line numbers are at `00809cc`.

## 1. Scope and method

**Read in full by the auditing session**, with the two lenses the maintainer
asked for (chess and search semantics; C++ engineering: undefined behaviour,
overflow, lifetime, concurrency, dead or misleading code):
`src/tt.h`; `src/search_kernel_core.cpp` (quiescence, negamax, root_search,
kernel_begin_search); `src/search_history_core.cpp`; `src/history_core.h/.cpp`;
`src/move_picker_core.h/.cpp`; `src/threats.cpp`; `src/search_worker.h/.cpp`;
`src/search_thread_pool.h/.cpp`; `src/search_root.h/.cpp`;
`src/search_types.h`; `src/search_params.h` (the core table); `src/eval.cpp`
(every function); `src/eval.h`; `src/eval_params.h` (array sizes against every
index used); `src/board.h`; `src/board.cpp` (FEN, make/unmake, null move,
attacks, legality, the generator, checks, draws, SEE); `src/zobrist.cpp`;
`src/engine.cpp`; `src/engine_entry.cpp`; `src/engine_command.h`;
`src/parameters.cpp`; `src/uci_protocol.cpp`; `src/uci_output.h`;
`src/main.cpp`; `src/bench.cpp` (the driver); `src/syzygy.cpp`;
`src/constants.h`; the CI matrix in `.github/workflows/ci.yml`; the section
lists of `tests/test_tt.cpp`, `test_search.cpp`, `test_eval.cpp`,
`test_engine_threading.cpp`, `test_uci_protocol.cpp`.

**Verified mechanically** (not by reading):
- the KPK bitbase against Syzygy on every legal king-and-pawn position, both
  colours, both sides to move: 662,704 positions, program and logs in the
  session's scratch directory (`kpk/kpk_check.cpp`, `kpk/kpk_diag.cpp`), the
  tables at `D:/chess/tablebases/syzygy3456`; §2 F1;
- the full CTest on the rebuilt head (24/24, 15 s);
- the source census (§3), by a separate agent from scripts, not by hand.

**Not read line by line**, and why: `src/search_diagnostics.cpp` (diagnostic
output only), `src/attacks.cpp` and `src/magics.h` (`test_magics` and the
movegen oracle hold them), `src/bitboard.cpp`, `src/wac.cpp`, `src/version.cpp`,
`tools/texel/` (C.2.1 owns it), the legacy kernel and every `#else` arm of
`BASILISK_B2_CORE` (B.8 deletes them; repairs do not touch them by the
maintainer's decision of 2026-10-24).

**Built on, not repeated**: `analysis/rarog_audit_2026-10-24.md` and its
A/D parts (items A-*, B-*, C-*, D-*, each with an owner),
`analysis/b20_architecture_review_2026-10-10.md`,
`analysis/codebase_consolidation_v1.md`, the Known-defects table in PLAN §2.
Where this audit confirms a known item it says so and adds only what is new.

## 2. Defects and research inputs

Ranked: correctness of play first, then undefined behaviour, then protocol.

### F1. A won KPK is scored 0 by the pawnless scaler — NEW defect

- **Where.** `src/eval.cpp:745-754` (the KPK branch of `apply_endgame`) and
  `:829-842` (the "winning side has no pawns" scaler).
- **Claim.** The KPK branch returns 0 for a drawn KPK and otherwise "keeps the
  (already winning) tapered score" (`:753`). When the tapered score favours
  the bare king, the later scaler takes the bare king as the strong side
  (`strong = (score > 0) ? WHITE : BLACK`, `:833`), finds it pawnless with no
  material edge, and scales the score to `SCALE_DRAW`: a theoretically won
  KPK evaluates to exactly 0.
- **Evidence.** Over all 662,704 legal KPK positions (both colours, both
  sides to move) the bitbase (`kpk_init`, `kpk_strong_wins`, `:150-285`)
  agrees with Syzygy WDL on every one, and the drawn-KPK return fires on
  exactly the 217,576 draws. The engine nevertheless evaluates 2,022 Syzygy
  wins (0.31%; 1,011 per pawn colour, exact mirrors) as 0: in every one the
  tapered score going into `apply_endgame` is ≤ 0 for the pawn side (−192 to
  0), and the pawnless scaler fired on 2,002 of them (the other 20 were
  already exactly 0). None is a stalemate. Example (White to move, Syzygy
  win): `8/8/8/8/8/8/Pk6/3K4 w - - 0 1`, pre-scaler score −28, served 0.
  Most are pawns on the second rank with the lone king adjacent.
- **Before A.11.7** the scaler chose the strong side by material totals, so
  the pawn side was strong and the scaler did not fire: these positions were
  served at −28-like values, the wrong sign but not a declared draw. A.11.7's
  sign rule (donor-correct for pawnless-versus-pawnless) exposed the missing
  KPK result. The donor avoids this because its `Endgame<KPK>` returns a
  definite value (`VALUE_KNOWN_WIN + PawnValueEg + rank`) before any scaler.
- **Effect.** Theory truth (DESIGN §4: an absolute veto) at the evaluation
  layer: a static 0 for a won KPK leaf inside pawn endings. At the root the
  search resolves it (the example above reads +187 at depth 1, +19,871 at
  depth 12), so the cost is in-tree pruning and ordering, not thrown games
  from KPK roots. Occurrence in real trees is not measured (BAS-E43's method
  applies).
- **Repair (A.13.1).** In the KPK branch, when the bitbase says won, return a
  definite win from the pawn side's view: `KNOWN_WIN` plus a drive (the
  donor's `PawnValueEg + rank`), under the bare-king cap, and never reach the
  scalers. The first draft also proposed an explicit scaler invariant (a
  side with only a king is never the strong side); the second pass
  withdrew it — see the verification note below.
- **Verification (second pass, 2026-10-27).** An independent reading
  confirmed the control flow (`eval.cpp:749-754` falls through; the tiers
  at `:763-802` and the wrong-bishop rule at `:805-826` need a piece; the
  scaler at `:833-843` fires with `SCALE_DRAW`), the White-view sign at
  that point (`:1239`, `:2011`; the side-to-move flip is after
  `apply_endgame`), that no cache or lazy path returns first, and the
  served 0 at a leaf (`8/8/8/8/8/2k5/P7/3K4 b`, `go depth 1 searchmoves
  c3b2`: cp 0; a won control leaf: cp −261). Corrections it made: the
  donor also picks the strong side by sign and also scales a bare king to
  a draw (`evaluate.cpp:829`, `material.cpp:205-207`), so only the KPK
  return is donor form; the invariant would change KPPK, KNPK and KBPK
  scores favouring the bare king from 0 to a wrong-signed value, untested
  and fingerprint-moving, so it is withdrawn; the repair value lands at
  10,207–10,212, inside no other band; the promotion closure is minimax
  (only the defender can capture into a won KPK); the donor returns the
  value before rule-50 damping while Basilisk damps every score; the test
  must read the undamped evaluation; and the fixture needs its own
  materialised directory (`test_endgames` lacks the fixture define,
  `syzygy_fixture.h:92-93` hard-codes the KQvK files, `test_search.cpp:954`
  counts the shared directory). A.13.1 carries all of it.
- **Test.** The exhaustive comparison, as a `test_endgames` section against a
  bundled `KPvK.rtbw` fixture (7,824 bytes; `tests/fixtures/syzygy/` already
  bundles KQvK as base64 and `tests/syzygy_fixture.h` materialises it): every
  legal KPK position's evaluation sign equals the WDL verdict, and no won
  position evaluates below `KNOWN_WIN`. On the old code it fails on 2,022
  positions.
- **Interactions and invariants.** `apply_endgame` runs on both the full and
  the lazy path and receives a white-perspective score (DESIGN §3). The
  KBP-wrong-bishop rule and the bare-king tiers run before and are
  unaffected. The promotion closure (BAS-E51): a won KPK promotes into the
  queen tier (28,700+), so the new KPK value must stay below
  `BARE_KING_QUEEN_TIER` and above every KP-K-with-more-material neighbour it
  can reach by a capture (none: the lone king captures nothing but the pawn,
  which is the drawn branch). The evaluation is frozen: the repair needs the
  maintainer's admission, as A.11.7 and A.11.9 had.
- **Gate.** Bench accounting (position 6 of the bench suite is a pawn ending
  and can reach KPK in its tree, so the fingerprint may move), CTest on both
  arms, the conversion floors, and the `[-5,5]` repair SPRT of A.11's rule
  unless the fingerprint does not move.

### F2. Fail-soft quiescence (A.11.10) — research inputs, not a new defect

Known defect B-19, owned by A.11.10. The audit read every return and store
in `quiescence` (`src/search_kernel_core.cpp:116-327`) and the moves of the
tie rule, so the leaf's research starts from these facts:

1. **Every path that must change together.** The TT cutoff takes any proven
   bound at any quiescence node (`:145-147`), with no node type: a PV
   quiescence node takes a depth-0 bound cutoff where the donor's qsearch
   cuts only at non-PV nodes (B-27). The stand-pat is tightened by a TT bound
   (`:203-207`) with no exclusion of decisive scores, where negamax's
   refinement excludes them (`:510-516`). Delta pruning returns `alpha`
   (`:230`) and stores nothing, so a fresh evaluation at a delta-pruned node
   is lost (negamax stores an evaluation-only entry on a miss, `:493-495`;
   quiescence never does). The depth cap return (`:235`) and the in-check
   branch (`:155-181`) are already fail-soft; the in-check branch stores
   nothing. The final store (`:324-326`) is `TT_EXACT` or `TT_ALPHA` of
   `alpha`, where the donor stores only lower or upper bounds of the
   fail-soft best and never an exact bound from qsearch.
2. **The only record of the earlier attempts** is the two comments at
   `:224-233` and `:313-320`: a fail-soft delta-pruning return broke the KBNK
   fixed-depth conversion canary, and seeding the final store from the
   tightened stand-pat turned a KQK mate in 5 into `mate 63`. Each attempt
   changed one return alone. Neither measurement is in the ledger.
3. **H1 (leading).** The KQK degradation came from the node-type-free TT
   cutoff at PV quiescence nodes consuming the depth-0 upper bounds that a
   fail-soft store seeds from a lower-bound-tightened stand-pat (a value that
   is not a provable upper bound), not from fail-soft itself. Falsifier: make
   quiescence fail-soft in the donor's form (best seeded from the tightened
   stand-pat, stored as lower/upper only, the bound cutoff refused at PV
   quiescence nodes) and run the KQK and KBNK fixtures and the conversion
   floors; if they hold, H1 stands; if fail-soft alone breaks them and
   adding the PV exclusion does not repair them, H1 is false and the
   stand-pat tightening is the next suspect.
4. **The tie rule's second divergence.** `(int(state_.nodes) & 14) == 0`
   (`:1253-1255`) fires one node in eight; the donor's `& 15` fires one in
   sixteen. The leaf owns the rule's calibration and decides this with it.
5. **The counter the leaf needs.** Tie-rule promotions where the equal score
   is `alpha` exactly (a bound echo) against those where it is an exact value,
   in a Diag build, read before and after.

### F3. The PV copy reads one past the per-ply length array — UB, neutral

- **Where.** `src/search_kernel_core.cpp:1272`
  (`std::clamp(state_.pv_len[ply + 1], ply + 1, MAX_PLY)`), with
  `int pv_len[MAX_PLY]` at `src/search_worker.h:135`.
- **Claim.** When a move raises alpha at `ply == MAX_PLY - 1` the copy reads
  `pv_len[MAX_PLY]`, one past the array (the first bytes of `move_buffers`).
  The value is clamped to `[MAX_PLY, MAX_PLY]`, so the result is `MAX_PLY`
  whatever was read: behaviour is unaffected and the fingerprint cannot move.
- **Reach.** A negamax node at ply 127 needs the check extension to carry
  depth that far; the extension stops at `ply < MAX_PLY - 2`. Rare, but the
  read is undefined whenever it happens.
- **Repair (A.13.3).** Size `pv_len` to `MAX_PLY + 1`, or guard the copy
  with `ply + 1 < MAX_PLY`. Exact fingerprint. No failing test is
  constructible without exposing `negamax` (a fixed-depth search cannot
  reach ply 127); the leaf argues that exemption and relies on the sanitizer
  job.

### F4. Protocol divergences — low, for D.3.2

All read in `src/parameters.cpp` and `src/engine.cpp`; none changes play.
Line references corrected and one item refuted by the second pass
(2026-10-27).

- A bare `go` searches to `defaultSearchDepth` 7 (`constants.h:12`, applied
  at `parameters.cpp:229-231` and `:282-283`; `test_search.cpp:240` and
  `:265` assert it), where Stockfish treats a bare `go` as infinite. A GUI
  that sends `go` and waits for `stop` gets an unrequested `bestmove` after
  a fraction of a second.
- `go mate N` caps the depth at `2N - 1` (`parameters.cpp:305-311`; the
  search also stops early on a mate within N, `search_worker.cpp:661-663`),
  so a mate the extensions find deeper is not searched for; Stockfish
  searches until the mate is found or `stop`.
- REFUTED: the first draft said a `stop` between two searches answers the
  next `go infinite` at once. The stray `stop` sets the flag
  (`uci_protocol.cpp:181`) but also enqueues a Stop command, and the queue
  is first-in first-out (`engine_command.h:39-45`); the Stop handler clears
  the flag (`engine.cpp:461`) before the next `go` is handled, and the
  immediate-bestmove branch (`engine.cpp:284-288`) fires only for a stop
  sent after that `go`. Run: `go depth 3`, `stop`, `go infinite` searched
  to depth 22 and answered on the second `stop`.
- A `SyzygyPath` whose `tb_init` fails, or that initialises and finds no
  files, is cached as the current path (`engine.cpp:102-114`), so
  re-sending the same text after fixing the files does nothing until the
  path text changes. Fathom's `tb_init` rarely fails; the no-files case is
  the common one.

### F5. Read and found sound

So that nobody re-audits them: the thread pool's protocol (epoch, requested
and active helper counts, the done condition, the results vector's lifetime,
helpers without root filters); the shared node counter's batching and the
node-limit estimate; the stop and ponderhit flags with the command epoch,
including `isready` during setup (`searching_` is set on the UCI thread
before the `go` is queued); mate-distance pruning; the fail-high blend
using the pre-cutoff alpha as the donor does; the TT's packing ranges
(every stored score and evaluation fits `int16_t`), depth clamp, five-bit age
cycle, replacement rule, empty-slot test and the move-preserving store; the
score bands and the rule-50 downgrade; the history bounds' arithmetic at the
limits; the picker's stages (the TT move once, the excluded move never,
promotions in the capture stage, direct checks under the quiet skip); the
move generator (pins, en passant with both pawns removed, castling squares
and rights, promotions in capture-only mode); make/unmake for every move
type and the null move's clock rule; repetition with the null-move fence;
rule-50 mate precedence; insufficient material; FEN validation (kings,
pawns on back ranks, promoted-piece counts, castling rights against the
pieces, the en-passant square against a legal capture); every evaluation
array index against its declared size (`threat_by_*[7]` admits a checked
king, `safety_table[25]` with units clamped to 24, mobility tables sized to
the maximum count); the KPK bitbase itself; the bare-king tiers' static
asserts against the bands; the Syzygy preconditions (no castling rights,
piece count, clock 0 when the rule-50 flag is on) and the WDL mapping; the
LMR table's `std::log` (its cross-platform agreement is what CI's bench
agreement job checks on nine cells).

## 3. Architecture and code size

From a mechanical census of the tracked sources at `00809cc` (scripts and
`report.md` in the session's scratch `census/` directory; counts are physical
lines; functions measured by brace matching and confirmed with clang), read
against the A.6 map and the B.2.0 review so that nothing already decided is
re-proposed.

**Size.** Shared engine source 14,699 lines in 57 files. The legacy kernel:
1,462 lines in its four `.cpp` files, 290 in `history.h` and
`move_picker.h` (included only on the `!BASILISK_B2_CORE` branch), 123 in
eight `#else` arms of shared files (91 of them `search_worker.h`'s kernel
section), 48 in tests, ten `DiagCounters` fields written only there, 20
X-macro lines used only there, the `+legacy` id suffix, the `B2_CORE` CMake
option and CI's full legacy build-and-test job: about 1,900 lines, 11% of
the engine, plus a CI cell. Tests 9,692 lines in 23 files (23 CTest entries
on the core build); `tools/` 4,961 lines of PowerShell and 13,339 of
Python (3,313 more in `tools/texel`); documents 5,240 (docs) + 6,659
(experiment entries) + 13,259 (analysis).

**Flavour-conditional code.** 664 of the 14,699 shared lines (4.52%) sit in
`BASILISK_TUNE`, `BASILISK_DIAGNOSTIC`, `BASILISK_ABLATION` or `TEXEL_TRACE`
arms; with the macro sites outside arms, 893 (6.08%). `eval.cpp` carries
297 of them (126 `TR_*` trace sites), `search_diagnostics.*` 223, the kernel
24 lines plus 58 `DIAG_COUNT`, 27 `TRACE_DECISION` and 11 `ABLATED` sites.
Inference: the instrumentation is a visible but not dominant weight, and the
B.2.0 review's F20 (`DecisionTrace::record` takes 14 positional integers)
remains the one shape worth changing when B.8 cleans up.

**Long functions.** 29 over 80 lines, three of them legacy. `negamax`
1,084 lines (`search_kernel_core.cpp:331`), `evaluate_undamped` 964
(`eval.cpp:1069`), `Searcher::search` 380, `gen_legal_impl` 325,
`try_set_fen` 266, `print_search_diag` 222, `quiescence` 212, `eval_pawns`
162. Decision (inference from the B.2.0 review and PLAN rule 3): the kernel
and the evaluator stay as single functions through the clusters that rewrite
them (B.3–B.5, C.1); splitting them first would be churn at the exact seams
the clusters press on. B.8 may name helpers for the move loop once the
clusters are in, under the fingerprint and the NPS floor.

**Duplication.** 48 near-duplicate runs of twelve lines or more, excluding
the legacy-versus-core pairs: 18 inside single `src` files (longest 19
lines), one across `src` files (the legacy `move_picker.h`), 26 in tests
(the longest: `movegen_oracle.h` repeats 46 lines for the two colours;
`test_engine_ponder.cpp` and `test_engine_threading.cpp` share three engine-
driving blocks of 32, 24 and 23 lines), one in the tuner. The legacy kernel
shares 64% of its normalised windows with the core kernel. Same-named
helpers defined twice: `can_castle_kingside`/`queenside` and `has_piece_on`
(board and the oracle, deliberately independent), `perft` (three places),
`set_option` (two tests). Inference: engine duplication is negligible; the
test drivers are the one worthwhile merge, owned by D.3.2 which rewrites
those tests anyway.

**Layering.** The include graph points one way with one exception: four
engine-layer files (`bench.cpp`, `engine.cpp`, `parameters.cpp`, `wac.cpp`)
include the protocol-layer `uci_output.h` for its locked `uci_write_line`.
`board.h` (13 includers) and `constants.h` (12) are the hubs. Mutable
static state is the usual engine set — attack and bitboard tables, Zobrist
keys, `MG_TABLE`/`EG_TABLE`, the KPK bitbase with its `once_flag`,
`g_eval_params`, the Syzygy globals behind a mutex, the output mutex — all
written at init or under a lock; nothing is written on the hot path.

**Dead code (core build).** `Board::compute_hash` and
`Board::gen_quiet_checks` have no reference (the second is B.4's quiet-check
card and stays until B.4 decides). The UCI `Ponder` option is parsed and
stored but never read, as in Stockfish, where `go ponder` alone governs.
Every `search_params.h` field of the core table is read (67 of 75 exactly
once), every `eval_params.h` field is read, every `DiagCounters` field is
printed. Members used only by tests or the tuner (`assert_ok`, the position
snapshot, `get_fen`, `Board::see`, `probe_root`, `push_priority`) are test
seams, not dead code.

**C++23 and style.** The A.8.12 idiom pass already took what matters:
`std::expected` for FEN errors, `std::format` for output, `<bit>`, templated
lambdas, `constexpr` tables, `[[nodiscard]]`, `std::unreachable`. The
remaining raw arrays (`SearchState`'s per-ply tables, the history tables,
`MoveList`) are hot-path by design and a `std::array` would not have caught
F3 (`operator[]` is unchecked); `std::jthread`/`stop_token` would replace
only the pool's `shutdown_` flag in a protocol that is explicit and tested.
Decision: no feature pass; the language work is done.

**Recommendations.**

- **R1 (declined 2026-10-27; the deletion stays at B.8).** Delete the
  legacy kernel now rather than at B.8: about 1,900 lines and a CI cell go, every leaf record stops
  carrying a second fingerprint, and `search_worker.h`, `search_types.h`,
  `tt.h`, `search_params.h` and CMake lose their arms. Exact fingerprint on
  the only remaining arm; no falsifier because nothing is replaced. The
  counter-argument is the maintainer's decision of 2026-10-24 to keep it
  compilable until B.8; the audit's reading is that the kernel's remaining
  role (a comparison arm) ended when B.2.4 accepted the core, since `git`
  keeps the code. The maintainer declined on 2026-10-27: the kernel is
  deleted at B.8, whose inputs now carry this inventory.
- **R2 (A.14.1).** Define the score bands once. `MATE_SCORE` and `MAX_PLY`
  live in `tt.h:106-107` and `search_types.h:18-19`, `KBNK_STATIC_MATE_FLOOR`
  in `eval.h` as a literal, held equal by static asserts in
  `search_worker.cpp:28` and `tests/test_endgames.cpp:33`; `constants.h`
  already holds the tablebase band for the same reason and is included by
  twelve files. Move the four constants there, derive the floor, delete the
  duplicates and the asserts that only pinned them. Exact fingerprint; one
  silent-corruption hazard (DESIGN §3) removed.
- **Folded into B.8:** the unreferenced `Board::compute_hash`; the
  engine-layer includes of `uci_output.h` (an output sink that belongs below
  the protocol, or an injected writer); `DecisionTrace::record`'s positional
  arguments (F20); the ten legacy-only counters and 20 X-lines, which go
  with the kernel; a move-loop split of `negamax` only under fingerprint
  and NPS.
- **Folded into D.3.2:** the engine-test driver shared by
  `test_engine_ponder.cpp` and `test_engine_threading.cpp`.
- **Folded into D.3.1:** `movegen_oracle.h`'s colour duplication, if the
  oracle is kept beside the ported board-v2 corpus.
- **Not recommended:** a frozen-launcher consolidation (`tools/run_*.ps1`
  are frozen per experiment by PROCESS and the ledger cites them by name); a
  C++23 feature pass; splitting the kernel or the evaluator before their
  clusters.

**Do not touch** (sound, and load-bearing): the driver/kernel seam and its
three entry points; the single make/unmake seam with its continuation-row
bookkeeping; the one `tt_store` wrapper; the X-macro parameter tables; the
per-thread `Evaluator` with its pawn table; the pool's epoch protocol; the
baseline launcher object outside LTO; `constants.h` as the band's home.

## 4. Universal binaries and the GitHub runners

Research by a bounded agent on 2026-10-27, every external fact sourced; the
Stockfish internals read at tag `sf_19` (`edb0d9db`). Condensed here; the
design detail belongs to A.15.1's own packet.

**What Stockfish ships now.** Stockfish 17.1 (2025-03-30) published 23
per-ISA assets and Stockfish 18 (2026-01-31) 27. Stockfish 19 (2026-09-05)
publishes eight: `linux-x86-64-universal`, `linux-arm64-universal`,
`linux-riscv64-universal`, `windows-x86-64-universal`,
`windows-arm64-universal`, `macos-universal`, `android-arm64-universal` and
`android-armv7-neon` ("we have transitioned to universal binaries for our
releases", release notes at
https://github.com/official-stockfish/Stockfish/releases/tag/sf_19). The x86
universal build came in through PR
https://github.com/official-stockfish/Stockfish/pull/6740 (merged 2026-04-19)
and the macOS one through https://github.com/official-stockfish/Stockfish/pull/6860.

**How it works** (`src/Makefile` targets `universal-object-pgo`,
`universal-object-nopgo`, `macos-lipo`; `src/universal/`): a fat binary with
one dispatch at `main`, not function multiversioning and not a launcher.
Every ISA level (`x86-64` through `x86-64-avx512icl`, eight levels) is a
full engine build with its own PGO, compiled with its namespace renamed
(`-DStockfish=Stockfish_<arch>`), linked as one LTO object per level. Each
level's static constructors go into renamed, inert sections and only the
selected level's run. Windows links with `--allow-multiple-definition` and
the baseline object first. The network is embedded once. Dispatch is
`__builtin_cpu_supports` over 17 bits plus one model check that sends
slow-PEXT Zen 1/2 to AVX2. There is no option to force a level; CI emulates
CPUs with Intel SDE and asserts the selected level and the bench per CPU
(`scripts/check_universal.sh`). macOS is `lipo` of an x86-64 slice and an
Apple-silicon slice, with the x86 slice patched to read the network from the
arm64 slice.

**Basilisk today.** Nine PGO assets from `release.yml` (three x86 tiers on
Linux and Windows, arm64 on both, macOS arm64), named by
`_basilisk_dist_asset_name`. Runtime CPU detection exists only as the refusal
in `src/main.cpp` (a baseline-compiled launcher object outside LTO, verified
by `tools/diag/verify_isa.py`); there is no dispatch and no advisory
(D-25). PEXT enters the source only in `src/attacks.cpp`; the AVX2 tier has
no intrinsics, it is flags plus autovectorisation. Only `syzygy.cpp` and
Fathom's `tbprobe.c` carry dynamic initialisers. Engine code is in the
global namespace except `Zobrist` and `Syzygy`. No per-tier NPS evidence
exists (D-24).

**Options, priced.**
- (a1) function multiversioning: unavailable on Windows (Clang's `target`
  and `target_clones` need ELF IFUNC, https://clang.llvm.org/docs/AttributeReference.html;
  GCC likewise), and it turns every attack lookup into an indirect call LTO
  cannot inline, forfeiting the whole-engine `popcnt`/`tzcnt` codegen the
  tier flags give (Rarog measured 4.59% for that). Rejected.
- (a3) the Stockfish-style fat binary: build the engine libraries once per
  tier in a `BASILISK_NS` namespace, keep `src/main.cpp` as the baseline
  dispatcher, one dispatch at start-up so the hot path is unchanged
  (PR #6740's reviewer read universal 8.365M against avx512icl 8.363M nps).
  Prerequisites: the namespace wrap (behaviour-neutral, exact fingerprint);
  Fathom built once at baseline or namespaced; constant-initialised globals
  in `syzygy.cpp` and Fathom, or renamed init sections, so no tier's
  constructor runs before dispatch; a check that shared COMDAT template
  instantiations never come from a non-baseline tier (symbols resolve
  first-wins without a warning). With clang, per-tier bitcode with
  `-fprofile-instr-use=<tier>.profdata` and one LTO link avoids the donor's
  `-r`/awk rewriting, which LTO's per-function target attributes must be
  shown to honour (by `verify_isa`, not assumed). PGO stays one profile per
  tier, trained natively on the existing runners; SDE is needed only for the
  selection matrix. New `verify_isa` contract: every function outside a tier
  namespace passes the portable set; each tier namespace meets its set; the
  image carries nothing above PEXT. Tests: `BASILISK_ISA=portable|avx2|pext`
  forcing with refusal of an unsupported tier; each forced tier gives the
  identical `bench 13`; the chosen tier in the startup banner; an SDE matrix;
  a PGO liveness check per tier. Costs: three builds per x86 cell, about
  three times the asset size (2.2 MB to 5–6 MB), nine assets become five, and
  two silent failure modes (first-wins symbols, early initialisers).
- (b) macOS universal2: reverses `CMakeLists.txt:69`'s Intel refusal; needs
  separate slices and `lipo` on a macOS job; low value, macOS 26 is the last
  Intel release.
- (c) per-ISA assets behind a launcher process: multi-file, process-tree
  risk on Windows; nobody ships it. Not recommended.
- (d) keep nine assets and add the D-25 advisory on stdout.

**GitHub-hosted runners (2026-10).** Every label Basilisk uses is current:
`ubuntu-26.04` and `ubuntu-26.04-arm` (GA 2026-09-17; `ubuntu-latest` moves
to them between 2026-10-19 and 2026-11-19), `ubuntu-24.04` (release Linux
cells, kept for its baseline `libstdc++.a`), `windows-2025`
(`windows-latest`), `windows-11-arm` (free for public repositories since
2025-08-07), `macos-26` (`macos-latest`, arm64). Deprecated or retired
labels Basilisk does not use: `ubuntu-22.04` (brownouts from 2027-03-23,
retired 2027-04-17), `macos-14` (retired 2026-11-02), `macos-13` (retired
2025-12-04). `macos-26-intel` exists (GA 2026-02-26) with no end date found.
Sources: https://github.com/actions/runner-images,
https://github.blog/changelog/2026-09-17-ubuntu-26-generally-available-and-latest-migration,
https://github.blog/changelog/2026-02-26-macos-26-is-now-generally-available-for-github-hosted-runners/,
https://github.blog/changelog/2025-08-07-arm64-hosted-runners-for-public-repositories-are-now-generally-available/.
**No label change is needed.** Toolchains on the images: `ubuntu-26.04`
Clang 20/21/22, GCC 13–15, CMake 4.4, Ninja 1.13; `ubuntu-24.04` Clang
16–18 with the release cells installing `clang-19`; `macos-26` Xcode 26.6
with Apple clang 21, CMake 4.4, Ninja 1.13. Three hygiene findings: the
`brew install ninja` step and the ci.yml comment that Ninja is missing on
macOS are stale; the macOS Xcode major is not pinned (Windows pins its clang
major in `.github/windows-clang-version`), so an image update can change
AppleClang codegen and NPS without a failing check; `verify_isa.py`'s
portable tier does not forbid SSE4.1, AVX1 or BMI1 mnemonics (`andn`,
`tzcnt`), which the fat binary's shared code would need it to.

**Recommendation for the leaves.** Measure the tier ladder first (A.15.2:
pooled-PGO NPS of portable, avx2 and pext, four builds per tier,
interleaved, a null pair first, Rarog's method of reading each hop directly;
D-24), with the decision rule frozen before the read. Do (d) and the hygiene
items now (A.15.3). Write the (a3) design as A.15.1's packet from the facts
above. Build the fat binary in G.2 when the ladder or Phase F's SIMD tiers
justify it; pull G.2 forward if the ladder clears the rule.

## 5. Verification gaps found

- The KPK bitbase had no truth test; the exhaustive comparison now exists as
  a scratch program and becomes A.13.1's test with a bundled `KPvK.rtbw`.
- The measurements that refused fail-soft quiescence live only in two
  comments; A.11.10 re-measures them (F2.2).
- No test reaches the ply ceiling; A.13.3 argues the exemption (F3).
- `verify_isa.py`'s portable set is incomplete for shared code (§4).
- The macOS toolchain is unpinned (§4).

## 6. Second pass (2026-10-27, after the usage limits reset)

The first pass's multi-agent verification died on the usage limit. This
pass re-ran it as four bounded reviews, each a single Opus agent with a
read-only brief and the relevant PLAN section in hand so that it reported
only what PLAN did not own: an adversarial verification of F1–F4 and H1
(§6.1), a search feature-cohesion review against the pinned donor
`edb0d9db` (§6.2), an evaluation review against `D:/code/sf9587-ks`
(§6.3), and a correctness audit of `tools/texel/` (§6.4). The main session
re-verified every item that went into Phase A or a B.3 switch (D1 by
running the binary; S1, S2 and S11 by reading the cited lines). §6.5 is the
serious-engine checklist and §6.6 the C++26 decision. PLAN carries each
item at the leaf named; this section is the evidence and the inventory.

### 6.1 Verification of the first pass

F1, F3 and H1 hold. F4(c), the stale `stop`, was refuted (the Stop command
clears the flag in queue order before the next `go`); F4(a), (b) and (d)
hold with corrected line references, and (d) is wider than stated: a
`SyzygyPath` that initialises and finds no files is cached too. The A.13.1 text
was corrected in five places (§2 F1's verification note): the scaler
invariant is withdrawn, the repair value is 10,207–10,212, the promotion
closure is minimax, the test reads the undamped evaluation, and the fixture
needs its own directory. The 662,704- and 2,022-position counts were not
re-run (no build in the verifier's brief); the served 0 was reproduced at
one leaf.

### 6.2 Search: interaction map and cohesion findings

Read at `fe20f09` against the pinned donor `edb0d9db` (`D:/code/stockfish`,
`src/search.cpp`). B2_CORE kernel; `search_kernel_core.cpp` unless noted.

| Decision | Reads | Writes |
|---|---|---|
| TB probe `:353` | depth, excluded, piece count, rule 50; runs before the TT probe | TT at depth+6 with `ss->tt_pv` (stale: S11) |
| Check extension `:398` | in check, excluded | depth+1 at entry |
| Hindsight `:544` | prior reduction, opponent worsening, `ss->eval + (ss-1)->eval` | depth, before the TT cutoff (as the donor) |
| TT cutoff `:555` | TT value/bound/depth, cut node at depth ≤ 4, rule 50 < 96, child entry at depth ≥ 7 | quiet bonus to the TT move; continuation malus to the parent |
| Static/corrected eval | TT raw eval, rule-50 damping, pawn/minor/non-pawn correction, cont-corr (ss-2, ss-4) | `ss->eval`; eval-only TT entry |
| Refined `eval` | static eval plus a non-decisive TT bound | razoring, RFP, NMP (S7), the LMR alpha term |
| improving / opp. worsening | `ss->eval` against ss-2 / ss-1; OR'd with `eval >= beta` after ProbCut | RFP, LMR, LMP |
| Eval-difference training | ss-1/ss evals, check state, capture | main history; pawn history on a TT miss |
| Razoring | eval, depth cap, TT move capture-or-none, bound ≠ BETA, non-PV | into qsearch |
| RFP | eval, depth, improving, worsening, |corr|, `!tt_pv`, TT-move type | blended return |
| NMP `:655` | refined eval (gate and R), allow_null, non-pawn material; child typed cut (S1) | — |
| ProbCut `:699` | static eval (SEE), fixed margin, captures in generator order; child typed cut (S1) | TT LOWER at depth−3 |
| IIR | non-PV and all nodes, TT move, TT depth < d−3 | depth−1 |
| Picker | TT move; capture history + 7×value with SEE; quiets: 2×main, 2×pawn, cont 1/2/3/4/6, direct check, threat-by-lesser, low-ply, root table | — |
| Move-loop pruning `:854` | move count, depth, improving, `ss->eval`, LMR depth (r + history), capture history, cont1+cont2+pawn, main; never at the root or in check (S3, S4); quiets non-PV only (S8) | best score raised by futility |
| Singular `:974` | TT move/depth/bound/score, depth, PV, double_exts; verification typed cut (S1) | extension, multi-cut return |
| LMR `:1086` | table, delta/root_delta, improving, ttPv terms, move count, |corr|, cut/all node, TT capture, `(ss+1)->cutoff_cnt`, stat score, alpha − LMR eval; never at the root or in check | `ss->reduction`; post-LMR continuation bonus |
| `depth -= 3` `:1266` | not at the root or in check | later moves' depth, stored depth, bonuses |
| Qsearch | TT without node type (B-27), stand-pat = raw + correction then TT-tightened; delta/futility/SEE/late pruning on it; MVV×16 − LVA + capture history; evasions in generator order (S9) | TT at depth 0 |
| History update | alpha move, depth after −3, `(ss-1)->stat_score`, list sizes | main, low-ply, continuation ×6, pawn, capture, TT-move history (never read) |
| Correction update | not in check, not excluded, not decisive, capture-best guard, admission sign | four structure tables + cont-corr |
| TT store `:1408` | final depth, bound, best move (also at fail-low, B-12), raw eval, inherited `tt_pv` | — |
| Root / helpers | root TT move from the shared TT (S6); root table of completed best moves; merge by depth then score; helpers start at depth 1 + id%2 | root table |

Findings, ranked by expected strength impact; each is verified by reading
both codebases unless marked a donor prior. PLAN placement in brackets.

- **S1** `:675`, `:715`, `:983` — NMP, ProbCut and the singular verification
  type their child `cut_node = true`; the donor passes `false`, `!cutNode`,
  `cutNode`. [B.3.0, three categorical bits]
- **S2** `history_core.h:51`, `:65`, `search_worker.h:256-257` — capture,
  continuation and pawn histories and the continuation correction are
  indexed by piece type with no colour; B.2.0's "colour-redundant"
  rationale is false; the donor indexes by `Piece`. [B.3.0 residual, before
  B.6]
- **S3** `:854`, `:1121`, `:1137`, `:1268` — in-check nodes are never
  reduced or move-loop pruned; the donor gates only quiet futility on
  check. [B.3 check-extension card]
- **S4** same lines — the root is never reduced; the donor reduces root
  moves after the first from depth 2. [B.5]
- **S5** donor `search.cpp:352`, `:395-396` — `seekMate` (root score ≥ 2000
  internal from depth 16: RFP capped at depth 6, singular extensions off)
  is absent; Basilisk's live problems are won-ending trees. [B.3, B.4]
- **S6** `:444`, `:1411` — the root's first move comes from the shared TT,
  not the root's own previous PV. [B.5; D.2 for helpers]
- **S7** `:655-659` — NMP gates and sizes R on the TT-refined eval; the
  donor uses the static eval. [B.3]
- **S8** `:916` — no quiet pruning at any PV node; the donor prunes off the
  previous PV (`followPV`) and gates IIR on it. [B.3]
- **S9** `:166` — qsearch evasions in generator order, TT move ignored.
  [B.4]
- **S10** — the TT bound-mismatch penalty (`penalize(1)`, depth > 5) is in
  `edb0d9db`; B.0 deferred it as post-SF-19. [B.3.0 note]
- **S11** `:356-375`, `:371`, `:455-457` — the TB store passes `ss->tt_pv`
  before this node assigns it; the probe runs before the TT probe, the
  donor's after. [A.13.4]
- **S12** `:1077` — `(ss+1)->double_exts` stale under NMP and ProbCut
  children; off by one or two against a cap of 16. [B.3 bookkeeping]
- **Upcoming repetition (cuckoo)** — absent; BAS-S12's retry trigger wanted
  one named missing consumer, now named (the TT probe against a stored
  decisive bound, A-13). [B.3 card]
- **B.4 futility card details** — base is the TT-tightened stand-pat (donor:
  static eval + margin); no recapture exemption; no move-count pruning; the
  qsearch MVV-LVA formula differs from the picker's capture formula. [B.4]

Already in PLAN and confirmed by this read: A.11.10, B-27, B-12, B-14/15,
B-49/53, B-80/81, B-31/32/37, B-33, B-39/42/43, B-5/55, the TT-move history
without a reader, B.4's value scale card, B-22, D.2's shared histories and
the `& 14` tie rule. Identical to the donor and therefore not findings:
hindsight in check and before the TT cutoff; the TT-cutoff history bonus
before the graph-history refusal; the post-LMR bonus; the excluded search
overwriting `move_count` and `cutoff_cnt`; the stat-score and pruning
compositions; the `tt_pv` inheritance condition.

### 6.3 Evaluation: term inventory and cohesion

Read at `fe20f09` against `D:/code/sf9587-ks/src` (a classical checkout
assumed to be `9587eeeb`: it carries extra king-safety and winnable trace
arrays and no git metadata). Served scores were read with `go depth 1
searchmoves <quiet move>` into a quiet child; Syzygy truth from
`syzygy3456` (`scratchpad/hce/kq2m.py`).

| Donor term | Basilisk | Where (`eval.cpp`) |
|---|---|---|
| Material + PSQT | different: PeSTO tables; phase counts pieces N=B=1, R=2, Q=4 of 24 (donor: npm between limits) | `:86`, `:882-891`, `:1080-1101` |
| Imbalance (quadratic) | same form plus a linear slot; dead/collinear slots (C-20) | `:1111-1136` |
| Bishop pair | four carriers: imbalance slot 0, `bp`, `bishop_pair_pawns` | `:1169`, `:1776` |
| Isolated | same; no doubled-isolated case | `:971` |
| Backward | different (C-12) | `:983-993` |
| Doubled | different: every pawn on a file with two or more, supported or not (donor: rear unsupported pawn only) | `:965` |
| Connected / phalanx / supported | different: flat supported bonus + one rank table; no opposed factor, no support count (C-16) | `:977`, `:998-1003` |
| Weak lever | absent (donor eg 56) | — |
| Weak unopposed, blocked | same | `:1005`, `:1011` |
| Pawn majority, islands | Basilisk-only | `:1026-1050` |
| Passer definition | strict passer only; no lever candidates, no blocked-passer rule (C-117) | `:954` |
| Passer rank bonus | mg/eg tables in the pawn cache | `:957` |
| Passer king distance | different: to the pawn, ×rank; king-to-block 19 a step, not rank-weighted (donor: to the block square, 19/4 vs 2, ×(5r−13), from rank 4) | `:1734-1755` |
| Passer path / unsafe | different: free ×5 and unattacked stop ×10 gated on an empty stop; unattacked path ×9 not gated (D2); no adjacent-file span, no x-ray behind | `:1147-1166`, `:1740-1756` |
| Passed file | absent | — |
| Outposts | knight only (C-10, BadOutpost owned); bishop outpost absent despite comments at `:1783`, `eval_params.h:222`; reachable outpost counts squares and fires on an outposted knight | `:1207-1224`, `:1806-1812` |
| Minor behind pawn | own pawn only (donor: either colour) | `:1796`, `:1818` |
| Bishop pawns | linear count; no ×(unprotected + blocked central) factor | `:1787` |
| Bishop x-ray pawns | absent | — |
| Long diagonal | absent (C.7) | — |
| Minor / rook on king ring | real attacks into the larger zone, knights included, beside the attack units (donor: exclusive `else if`); rook weight fitted −2/−4 | `:1791`, `:1813`, `:1844` |
| Rook on file | same; Basilisk-only closed file, seventh rank, connected rooks | `:1190-1202`, `:1848`, `:1859` |
| Rook on queen file | enemy queen only | `:1852` |
| Trapped rook, weak queen | absent (owned) | — |
| Queen infiltration | different form (C-48) | `:1832` |
| King protector | same form; knight weight +1 against the donor's −8 | `:1801`, `:1823` |
| Trapped bishop | Basilisk-only, phase < 12 | `:1911` |
| Mobility | area as the donor's (C-13); no x-ray or pin line (C-115); tables near linear seeds, rook mg flat | `:1337-1385`; `eval_params.h:265-272` |
| King danger | different form (E1) | `:1387-1391`, `:1516-1596` |
| Shelter / storm | wing kings only, linear, recomputed each eval, no castling alternative; storm sign (C-15) | `:1599-1657` |
| King-pawn distance | absent (nil in Rarog, C-111) | — |
| Threats: hanging, by minor/rook/king, restricted, weak-queen protection, pawn push | same form | `:1433-1506` |
| Threat by safe pawn | flat 58/25 whatever the target; unsafe pawns count | `:1403-1426` |
| Slider / knight on queen | absent (owned); dead `hang_pen` loop | `:1705` |
| Space | different (C-119, E8) | `:1876-1908` |
| Winnable | eg half only, no `almostUnwinnable`, weights 0 (C-47) | `:1981-2008` |
| Scale factors | pawn-count `36+7p` absent (C.5.3); OCB owned; one-flank rook ending 36 absent; lone queen vs minors 37+3m absent; pawnless (owned, plus D1); KRPKR/KRPPKRP with a floor | `:716-873` |
| Rule 50 | 199 curve (C-128) | `eval.h:125-139` |
| Tempo | mg only, so tapered and scaled (donor: constant 28 after scaling) | `:1967` |
| Lazy evaluation | one stage at 700 on the tapered cheap score, no npm term (donor: 1400 / 1300 + npm/64 on (mg+eg)/2, two stages) | `:1237-1248` |
| Caches | pawn cache pure (pawns only, shelter outside it); no material/imbalance cache; the TT stores the eval before rule-50 damping | `:909-923`, `:1053` |
| Contempt | absent (E.2) | — |

Cohesion findings (verified by reading):

- **E1** king danger mg-only and short of the donor's inputs; open-file
  double counting with shelter; ring bonuses beside attack units. [C.3]
- **E2** phase: mg share 33/25/33% in RR, R+B v R+N and Q v Q endings
  against the donor's 10/3/13%; mg-only terms stand in for eg king danger.
  [C.0, C.7 with C.3]
- **E3** scalers multiply the whole tapered score, the donor eg only; order
  of operations differs. [C.5.3]
- **E4** passers carry rank five times; unweighted king race; lazy split.
  [C.6, C.2.1 manifest]
- **E5** tempo tapers away and is scaled; absent from the lazy path. [C.7]
- **E6** fitted signs opposite the donor's: `space_blocked_mg`,
  `rook_king_ring`, `king_protector_n_mg`, `restricted_eg`,
  `threat_by_rook_mg[PAWN]`; islands +6 against isolated −13. [C.0]
- **E7** correction history overrides exact recogniser scores through
  shared structure keys. [C.5.2, C.10]
- **E8** lazy cut: one stage, no npm term, skips king danger, threats,
  mobility, passer proximity/path, mop-up, tempo. [C.12]
- **E9** no material or imbalance cache. [C.12]

Defects:

- **D1 (A.13.2)** the pawnless classifier compares in `EG_MAT` units, so
  KQ against two minors (Q − (B+N) = 300 ≤ 330) is scaled 4/64; the donor
  compares in mg values (932 > 825) and does not scale. Syzygy over 4,000
  random positions a class, from the queen's side: KQ-KBN 82.5% won,
  KQ-KBB 71.0% (6% cursed), KQ-KNN 73.5%. Served at depth 1, White's view,
  re-run by the main session: KQ-KBN `8/8/3nb3/4k3/8/8/8/K6Q b - - 1 1`
  +21, KQ-KNN +27, KQ-KB +697 (the agent read +27/+31/+801 into a quiet
  child; same picture).
- **D2 (C.6)** `passed_path_safe_eg` is not gated on an empty stop square:
  `8/3n4/3P4/8/8/8/k7/4K3 w - - 0 1` is paid +45 eg for a blockaded pawn.

Leads for Phase C, ranked by expected Elo: C.3 king-danger form (E1); C.7
phase with tempo and eg king danger (E2, E5); C.6 passers (D2, E4, weak
lever, passed file); C.5.3 scalers (E3, the two absent scalers); A.13.2
(D1); C.0 sign and collinearity inventory (E6); C.5.2/C.10 correction over
recognisers (E7); C.0 space (E8); C.7 bishop terms; C.4 the flat pawn
threat; C.12 lazy margin and material cache (E8, E9). None of the Phase C
leads was measured; each names its first measurement in PLAN.

### 6.4 Texel tuner

`tools/texel/tuner.cpp` and the Python pipeline, read at `fe20f09`; the
unit tests were run (`test_bake` 2/2; the other two suites need
python-chess, absent here) and `bake.py` was dry-run on a scratch header.
Parity of the linear part with `evaluate` is sound (PST mirroring, phase
buckets, tempo, rule-50 damping, imbalance, perspective) and runs are
deterministic. Five new items, T1–T5 in C.2.1: the Adam schedule bounds
coordinate travel to about lr × epochs with no convergence criterion (30 cp
under `phase911.ps1`, against C-66's 11–57 cp PST movements); `bake.py`
writes every differing slot with no provenance or completeness check (a
stale dump reverted 42 members and both PST blocks in a dry run); hidden
clamps apply to every coordinate whatever group is fitted and sit in no
manifest; the linear stage can break `safety_table`'s monotonicity, after
which the king-safety stage's `std::clamp` has `lo > hi`; and winnable has
no working instrument while `--audit-coverage` credits its slots. Confirmed
by reading: C-1 (with the KmK rule's sign dependence added), C-2, C-5, C-8,
C-19, C-20, C-27 to C-29, C-49, C-50, C-71. Hygiene items are listed in
C.2.1. Nothing needs a Phase A leaf: either PLAN owns the area or the bug
is latent at today's defaults.

### 6.5 Serious-engine checklist

PLAN was checked for the features a serious engine carries. Owned already:
MultiPV (A.8.9), ponder (A.8), Syzygy WDL/DTZ (A.8.14, D.4), time
management (D.1), Lazy SMP and NUMA (D.2, G.1), large pages and prefetch
(B.7.2), TT coherence (D.2), perft and board oracle (D.3.1), command
fuzzing and lifecycle (D.3.2), sanitizers and bench agreement (CI), PGO and
reproducible assets (release), NNUE (Phase F), datagen (F.2.1), contempt
(E.2), WDL display (A.8.19). Absent and added: Chess960 (D.3.3, maintainer
decision; castling is a fixed-square mask, `board.h:246-247`) and upcoming
repetition detection (B.3 card, §6.2). Absent and not added: strength
limiting (`UCI_LimitStrength`/`UCI_Elo`);
`nodestime`, not needed. Chess960 was drafted as a D.3.3 decision leaf
and removed the same day: the maintainer declined it and strength
limiting (2026-10-27); either returns to PLAN only by the maintainer's
hand. The feature-cohesion question the maintainer
asked is §6.2 and §6.3's content: the first pass audited mechanisms one at
a time, this pass audited their couplings.

### 6.6 C++26

Finished at the March 2026 London meeting (411 national-body comments
resolved, no feature changes); ISO publication pending. Compiler support
on 2026-10-27 (GCC's status page, last modified 2026-10-03; cppreference):
GCC 16.1 (2026-04-30) has reflection (`-freflection`), contracts, constexpr
exceptions and erroneous behaviour for uninitialised reads; GCC 15 has
`#embed`, pack indexing, `= delete("reason")` and structured bindings as a
condition; Clang 19–22 has those GCC 15 items and none of the GCC 16 set;
libc++ 22–24 has `optional<T&>`, `function_ref` and `concat_view`; MSVC
has the least and is not in Basilisk's matrix. Basilisk's release
compilers are Clang 22 (Linux), MSYS2 Clang 23 (Windows) and AppleClang
with libc++ (macOS); GCC 15 runs only the PGO smoke job. On that floor
`-std=c++26` adds nothing the engine needs; the features that would remove
code (reflection for the hand-kept option, parameter and counter tables)
are GCC-only. Decision recorded in G.2: stay on C++23; re-decide when the
macOS and Windows clangs ship reflection. `#embed` is usable now as an
extension in C++23 mode (GCC 15, Clang 19) and is noted in F.4 for the
network file.

Sources: https://gcc.gnu.org/projects/cxx-status.html ;
https://en.cppreference.com/w/cpp/compiler_support/26.html ;
https://herbsutter.com/2026/03/ (trip report, March 2026).

### 6.7 Placement of the evaluation repairs (maintainer question, 2026-10-27)

The maintainer asked whether F1 and D1 must be repaired inside the
evaluation freeze or can wait for Phase C: does the search, its features or
its fitting need them, and would the repairs change search behaviour in a
way the search should take into account?

**Does the search key on them?** No. `src/search_kernel_core.cpp`,
`search_worker.cpp` and `search_types.h` reference no `KNOWN_WIN`; the only
band the search reads is `is_decisive` at 31,743. The KPK repair moves won
KPK leaves from about 0 to 10,207–10,212, a band the search already sees
from KBNK and KXK leaves today; the queen repair unscales ordinary values
(about +300 instead of +25). Neither adds a kind of value the search has
not been fitted on, and the leaves in question are a handful per million
nodes, so the B.3–B.5 constants fitted with them unrepaired stay valid
after the repairs. The correction history sees a different error at those leaves
(mate against 10k instead of mate against 0), both clamped. The one
behavioural effect is a gradient reaching pawn-ending trees earlier, which
is the repair's purpose, not a dependency.

**Cost in games.** Read over the archived games (`tools/results` and
`D:/chess/results`, 107 PGN files, 974,796 games; filtered to the 191,216
games with 27 or more captures, the minimum for either ending; Syzygy WDL
from `syzygy3456`; script `scratchpad/kpk_games_par.py`):

| Ending | Strong side | Syzygy won | Converted | Drawn | Lost |
|---|---|---|---|---|---|
| KPK | Basilisk | 3,775 | 3,764 (99.7%) | 11 | 0 |
| KPK | opponents | 4,424 | 4,388 (99.2%) | 33 | 3 |
| KQ v two minors | Basilisk | 25 | 20 | 5 | 0 |
| KQ v two minors | opponents | 63 | 48 | 15 | 0 |

Basilisk defending a drawn KPK held 4,019 of 4,020. The queen ending is
reached in 0.003% of games and converted at the same rate by the field,
which is the ending's difficulty at these budgets, not the classifier.
Caveat: most archived games predate A.11.7 (`fdf4277`, 2026-10-26), when
the won-KPK leaf served a wrong-signed −28 rather than 0; both are small
leaf errors the search overrides from the root, and the conversion rate
says the search does.

**Decision (maintainer, 2026-10-27).** The read showed no cost in games
and no dependency of the search on the repairs, so they could have
waited for Phase C (a first placement at C.5.0 was recorded and reversed
the same day). The maintainer chose not to carry known theory-truth
defects through Phase B and admitted both into the evaluation freeze as
A.13.1 and A.13.2, with B.10 and A.11.9 as the precedent: Phase B's
clusters fit on the repaired evaluation. C.5.3 rewrites the scaler lines
later and keeps A.13.2's truth-table test.
