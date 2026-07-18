# Basilisk Board and Infrastructure Analysis

**Audit date:** 2026-07-13  
**Engine version:** Basilisk 1.8.0, `development` branch  
**Scope:** board representation, move/state transitions, legal move generation,
SEE, hashing and repetition, evaluation-facing state, build/release
configuration, correctness/performance testing, and strength-testing
infrastructure.

This is a living document. It deliberately separates demonstrated correctness
problems from performance hypotheses that still require benchmarking and SPRT.
The search and HCE audits are in `analysis/search_analysis.md` and
`analysis/hce_analysis.md`.

## 1. Executive conclusion

Basilisk's board is a sound conventional hybrid bitboard implementation. Its
basic representation, make/unmake path, legal move generation, lazy en-passant
handling, and incremental Zobrist keys are already well beyond a toy engine.
The board is not, by itself, concealing the entire remaining 200+ Elo gap.

The main board-level difference from the strongest 2026 CPU engines is how much
derived information is treated as persistent per-position state. Basilisk
caches only `checkers`. It repeatedly reconstructs pins, check geometry,
material/PST information, and sometimes complete legal move lists. Stockfish,
Obsidian, Reckless, and PlentyChess differ in exact move-generation strategy,
but converge on cached blockers/pinners/check information and incremental
NNUE-aware state.

The practical conclusion is:

> Fix the correctness leaks first, then introduce a proper per-ply `StateInfo`
> and dirty-piece interface. That redesign should be driven by NNUE and threat
> inputs, not by isolated micro-optimization of the current HCE board.

The largest remaining strategic strength gain is NNUE quality, data, and
search re-tuning. Board work is important because it determines whether NNUE
can be updated cheaply and whether the search spends its time on useful nodes.

## 2. Reference engines and comparison boundary

The latest published CCRL 40/15 snapshot found during this audit is dated
2026-02-28. Its leading engines were:

| Rank | Engine | CCRL 40/15 |
|---:|---|---:|
| 1 | Stockfish 18 | 3651 |
| 2 | PlentyChess 7.0.0 | 3644 |
| 3 | Torch v4 | 3638 |
| 4 | Obsidian 16.0 | 3636 |
| 5 | Reckless 0.8.0 | 3634 |
| 6 | Alexandria 8.1.2 | 3633 |
| 7 | Viridithas 19.0.1 | 3632 |

Source: <https://www.computerchess.org.uk/ccrl/4040/>

Torch is proprietary, so implementation comparisons use current open-source
Stockfish, Reckless, PlentyChess, and Obsidian code. Their current development
trees may be ahead of the specific versions represented in the rating list.

Primary comparison sources:

- Stockfish position state:
  <https://github.com/official-stockfish/Stockfish/blob/master/src/position.h>
- Stockfish move/state implementation:
  <https://github.com/official-stockfish/Stockfish/blob/master/src/position.cpp>
- Stockfish move generation:
  <https://github.com/official-stockfish/Stockfish/blob/master/src/movegen.cpp>
- Reckless SEE:
  <https://github.com/codedeliveryservice/Reckless/blob/main/src/board/see.rs>
- PlentyChess: <https://github.com/Yoshie2000/PlentyChess>
- Obsidian: <https://github.com/gab8192/Obsidian>
- Stockfish 18 release notes:
  <https://github.com/official-stockfish/Stockfish/releases/tag/sf_18>

## 3. What Basilisk already does well

### 3.1 Board representation

`src/Board.h:44-64` maintains:

- piece bitboards by color and type;
- occupancy by color and combined occupancy;
- a 64-square mailbox;
- cached king squares;
- side, castling, en-passant, rule-50, and repetition-related state;
- the current checker bitboard.

This redundancy is reasonable for a high-performance engine. It gives direct
colored-piece access to evaluation and move generation while retaining O(1)
mailbox queries for captured/moved pieces.

### 3.2 Incremental keys

`src/Board.cpp:13-64` incrementally maintains:

- the full position key;
- pawn key;
- minor-piece key;
- per-color non-pawn keys.

The undo record restores these keys directly. This is a good foundation for
pawn caches, correction histories, NNUE refresh caches, and more specialized
position histories.

### 3.3 Legal move generation

`src/Board.cpp:1187-1511` performs strict legal generation in one templated
implementation, including:

- single- and double-check masks;
- absolute pins;
- king destination safety;
- en-passant discovered-check handling;
- promotions and castling;
- capture-only and quiet-only stages.

The design is coherent and the existing standard perft set passes through
depth 4 or 5. Strict legal generation is not inherently inferior: Reckless and
other strong engines also use it. The important difference is whether expensive
derived geometry is cached and reused.

### 3.4 Fixed move lists

`MoveList` is fixed-capacity and stack-allocated (`src/Board.h:26-40`), avoiding
allocation in ordinary move generation. Capacity 256 safely covers legal chess
positions.

### 3.5 Existing test and tuning infrastructure

The repository already contains:

- deterministic perft and state-transition tests;
- evaluation, search, UCI, tablebase, threading, and ponder tests;
- Release/LTO and PGO build support;
- local fastchess SPRT, gauntlet, SPSA, and data-generation scripts;
- paired-color opening tests and longer-time-control confirmation guidance.

These are meaningful strengths. The recommendations below extend this base
rather than replace it.

## 4. Demonstrated correctness problems

### 4.1 Rule-50 draw can override checkmate

**Severity:** P0 correctness  
**Evidence:** reproduced against the shipped 1.8.0 PEXT/PGO binary

`Board::is_draw(int)` returns immediately when the rule-50 counter reaches 100:

```cpp
// src/Board.cpp:1659-1662
bool Board::is_draw(int search_ply) const {
    if (is_insufficient_material()) return true;
    if (halfmove_clock >= 100) return true;
    return is_repetition(search_ply);
}
```

Both search functions ask for a draw before determining check/mate:

- `src/search.cpp:1001`, before `in_check` at line 1003;
- `src/search.cpp:1199`, before `in_check` at line 1216.

Reproduction:

```text
position fen 7k/5Q2/5K2/8/8/8/8/8 w - - 99 1
go depth 2
```

Observed result:

```text
info depth 1 ... score cp 0 ... pv f7h5
info depth 2 ... score cp 0 ... pv f7h5
bestmove f7h5
```

White has mate in one. After a quiet mating move the child reaches a halfmove
clock of 100, and Basilisk returns draw before recognizing checkmate. Checkmate
takes precedence over a rule-50 claim.

Current Stockfish uses the equivalent of:

```cpp
rule50 > 99 && (!checkers() || legal_moves_exist())
```

That generates legal moves only for the exceptional case where the rule-50
boundary is reached while in check.

Recommended tests:

1. Quiet checkmate taking the clock from 99 to 100.
2. Checked position at 100 with a legal evasion: draw.
3. Checked position at 100 without a legal evasion: checkmate.
4. Equivalent coverage in both normal and quiescence search.

The frequency is low, so the average Elo effect may be tiny. The result is
nevertheless objectively wrong and should be fixed before strength work.

### 4.2 Null moves advance the rule-50 clock

**Severity:** P0/P1 search-state correctness  
**Location:** `src/Board.cpp:623-651`, especially line 644

`make_null_move()` executes:

```cpp
halfmove_clock++;
```

A null move is a search fiction, not a played reversible move. Current
Stockfish and Reckless preserve the rule-50 counter across a null move while
resetting only the distance used to limit repetition scanning. Advancing the
counter can manufacture a false draw in null-move descendants near the
100-ply boundary and changes any rule-50-aware TT scheme.

PlentyChess currently increments its counter on null move, showing that this
choice exists in real engines, but it is not the robust choice when draw
detection can be reached below null moves.

Recommended change: preserve `halfmove_clock`, clear/reset only
`plies_from_null`, and add a boundary regression from 99 plies.

### 4.3 SEE accepts pinned recapturers

**Severity:** P0 functional bug with likely measurable search impact  
**Locations:** `src/Board.cpp:1697-1850`

Both `see()` and `see_ge()`:

1. compute all attackers to the exchange square;
2. intersect with the side's occupancy;
3. choose the least valuable attacker;
4. remove it and reveal x-rays.

They do not exclude attackers absolutely pinned to their king.

Reproduction:

```text
FEN:  4k3/4n3/2p5/1B6/8/8/8/K3R3 w - - 0 1
Move: Bxc6
```

The black knight on e7 is pinned to the black king by the rook on e1. It cannot
legally recapture on c6. Correct exchange value is therefore `+100`.

Observed from Basilisk's board library:

```text
legal=1 see=-200 see_ge0=0
```

This bug has a much larger fan-out than the rule-50 problem. `see_ge()` is used
for capture staging and pruning at multiple points in `src/search.cpp`,
including quiescence, ProbCut, capture pruning, and main-search move decisions.

Strong implementations either use cached pinners/blockers or recompute the
required pin relation inside SEE. Reckless explicitly removes pinned attackers
while their pinner remains on the board.

### 4.4 SEE accepts illegal king recaptures

**Severity:** P0 functional bug  
**Locations:** the attacker loops at `src/Board.cpp:1747-1763` and
`src/Board.cpp:1827-1846`

The loops include `KING` as an ordinary least valuable attacker. After choosing
the king, they do not check whether the opponent still attacks the exchange
square. A king may therefore be counted as capturing a protected piece.

Current Stockfish and Reckless stop or reverse the exchange result if a king is
the selected attacker and the opposing side still attacks the destination.

The existing test at `tests/test_board.cpp:880-908` checks that `see_ge(m, t)`
equals `see(m) >= t`. This validates agreement between two implementations
sharing the same omissions; it is not an independent correctness oracle.

Recommended tests:

- pinned knight, bishop, rook, and pawn recapturers;
- a pinned piece that may legally capture along the pin ray;
- legal and illegal king recaptures;
- x-ray pinners removed during an exchange;
- comparison against an independent slow legal-exchange oracle.

### 4.5 En-passant hashing is pseudo-legal rather than legal

**Severity:** P0 repetition/hash correctness, rare position class  
**Locations:** `src/Board.cpp:339-362` and `src/Board.cpp:539-547`

After a double pawn push, Basilisk stores an EP square when an adjacent enemy
pawn pseudo-attacks it:

```cpp
if (PawnAttacks[us][ep] & pieces[them][PAWN]) {
    ep_sq = ep;
    hash ^= Zobrist::EpKeys[file_of(ep_sq)];
}
```

It does not verify whether at least one of those captures is legal with respect
to the enemy king.

Reproduction:

```text
FEN:  4k3/8/8/8/4p3/8/3P4/K3R3 w - - 0 1
Move: d2d4
```

The black pawn on e4 is pinned to the king on e8 by the rook on e1. `e4xd3 ep`
is illegal, but Basilisk records `d3` (`ep_sq == 19`) instead of `SQ_NONE`.

Position identity for repetition depends on the available legal moves. The
spurious EP key distinguishes positions that should be equivalent, causing a
possible missed repetition. Stockfish 18 release notes explicitly mention a
rare threefold-repetition/en-passant/pin fix, and current Stockfish validates
that at least one legal EP capture exists before storing the square.

### 4.6 Fixed history capacity is release-unsafe

**Severity:** P2 robustness  
**Locations:** `src/Board.h:66-68`, `src/Board.cpp:485-486`,
`src/Board.cpp:636-637`

The board owns 1024 `UndoInfo` entries. Bounds are guarded only by `assert`.
Release builds therefore write out of bounds if a sufficiently long game plus
search line exceeds the capacity.

This is unlikely in normal play but is unnecessary risk. A cleaner design is:

- a dynamically sized/deque history for moves made before search;
- a fixed per-thread `StateInfo[MAX_PLY + margin]` stack for search moves.

That also eliminates the need for every `Board` instance to allocate a large
history array.

## 5. Board hot-path losses

### 5.1 Pins are recomputed for each generation stage

**Confidence:** high  
**Strength effect:** performance candidate; measure before assigning Elo

`gen_legal_impl()` computes diagonal and orthogonal x-ray pins at
`src/Board.cpp:1241-1262`.

The move picker first requests captures at `src/search.cpp:845`, then quiets at
line 851 if no earlier stage cuts off. A node reaching the quiet stage therefore
repeats:

- both pin computations;
- king move generation/safety work;
- check-mask setup;
- portions of pawn and piece setup.

Stockfish caches `blockersForKing`, `pinners`, and `checkSquares` in `StateInfo`.
Its pseudo-legal generator then calls full legality checking only for moves from
pinned pieces, king moves, and en-passant. Obsidian caches the same king geometry
while retaining staged generation. Reckless and PlentyChess also keep blockers
or pins in board state.

Strict legal generation does not need to be abandoned. Basilisk can retain its
current generator and feed it cached blocker/pinner state.

### 5.2 Full checker recomputation after every move

**Location:** `src/Board.cpp:562`

After every real move Basilisk runs:

```cpp
checkers = attackers_to(king_sq[side_to_move], all_occ, ~side_to_move);
```

Search often separately determines whether the move gives check through
`gives_check()`. Current Stockfish accepts a precomputed `givesCheck` flag in
`do_move()`: if false, the checker bitboard is immediately zero; if true, it
computes the exact attackers. It then refreshes other cached check geometry
once for the child state.

Recommended experiment:

1. Pass the already cached `move_gives_check()` result into `make_move()`.
2. Set `checkers = 0` for non-checking moves.
3. For checking moves, calculate only the necessary checker set.
4. Measure full bench nodes/second and run an SPRT; do not judge only isolated
   make/unmake throughput.

### 5.3 Quiet-check generation scans every legal move

**Location:** `src/Board.cpp:1602-1612`

`gen_quiet_checks()` generates all legal moves, discards captures/promotions,
and calls `gives_check()` for every remaining move. Quiescence invokes this at
`src/search.cpp:1153`.

Direct quiet-check generation can use:

- cached per-piece check squares for direct checks;
- blockers/pinners relative to the enemy king for discovered checks;
- explicit castling and promotion handling.

This avoids generating and filtering unrelated quiet moves in a search region
where node economics are particularly sensitive.

### 5.4 En-passant TT validation copies the entire Board

**Location:** `src/Board.cpp:1151-1155`

`Board::is_legal()` handles EP by copying the board, making the move, and testing
king safety:

```cpp
Board tmp = *this;
tmp.make_move(m);
return !tmp.is_square_attacked(tmp.king_sq[us], them);
```

`Board` owns a heap-allocated 1024-entry history array. The copy constructor
allocates this array and copies the active history. `is_legal()` validates TT
moves in the main move picker, so an EP TT move can allocate on the search hot
path.

The legal generator already contains occupancy-based EP logic. Extract that
logic into a shared helper and remove the board copy.

### 5.5 Material, PST, and phase are rebuilt at evaluation

**Location:** `src/eval.cpp:579-625`

Every evaluation walks all pieces to calculate material/PST/phase and then
performs additional popcounts for material imbalance. The board maintains rich
hash state but no piece counts, material totals, phase, or PST totals.

For the HCE branch, incrementally maintaining the following is straightforward:

- piece counts;
- middlegame and endgame material/PST totals;
- phase;
- optionally non-pawn material by color.

However, the NNUE branch should not accumulate a large set of HCE-only fields
that will soon become dead weight. Prefer a generic move delta (`DirtyPiece`)
and a small set of universally useful counts/material values.

### 5.6 Move representation is wider than necessary

**Location:** `src/move.h:11-44`

Moves occupy an `int` even though the encoding uses at most 16 bits. A
256-entry raw move list is therefore 1024 bytes instead of 512. Stockfish,
Reckless, and many other engines use a 16-bit move representation.

Changing this is not expected to produce a large isolated gain, because scored
moves may still align to 8 bytes. It reduces raw move-list/cache pressure and
enables SIMD move emission more naturally. Treat it as part of a state/layout
cleanup, not a headline Elo change.

### 5.7 Representation redundancy should be benchmarked, not assumed wrong

Basilisk stores `pieces[color][type]`, `occupancy[color]`, and `all_occ`.
Stockfish and Obsidian store piece-type bitboards plus color bitboards and use
intersections for colored piece sets.

The peer layout uses fewer bitboards and fewer stores per move. Basilisk's
layout provides cheaper direct colored-piece access, particularly for the HCE.
There is no basis for declaring either universally faster. If this is changed,
benchmark complete search with the intended NNUE evaluator rather than only
make/unmake.

## 6. Missing state used by top engines

### 6.1 Proposed `StateInfo`

The board currently mixes physical position, reversible state, search history,
and derived geometry in one heap-owning object. A better structure is:

```text
Position (physical board)
  board[64]
  byType[] / byColor[] or existing colored bitboards
  castling path/rook metadata
  side to move

StateInfo (one per ply)
  full/pawn/minor/non-pawn/material keys
  castling, EP, rule50, pliesFromNull
  captured piece and previous-state pointer/index
  checkers
  blockersForKing[2]
  pinners[2]
  checkSquares[piece type]
  repetition distance/status
  DirtyPiece / DirtyThreat delta

Per-thread evaluation state
  NNUE accumulator stack
  king-bucket refresh cache
  network scratch buffers
```

The accumulator should not be embedded in copyable `Board` instances. Search
already operates on a per-thread position; the accumulator stack should follow
the search ply and only update/refresh when evaluated.

### 6.2 Threat inputs

Stockfish 18 introduced SFNNv10 threat inputs. Current Stockfish position code
contains `DirtyThreats` handling so changes to piece relationships can be
propagated incrementally. PlentyChess's current development board maintains
piece-specific and combined threat maps and updates NNUE threat features from
piece add/remove/move operations.

Basilisk should implement NNUE in two stages:

1. A correct, efficiently updated baseline network with dirty-piece deltas and
   refresh caching.
2. Threat features only after the baseline network, trainer, quantization,
   validation, and search re-tuning are stable.

Design the board hooks so stage two does not require rewriting make/unmake.

### 6.3 Rule-50-aware TT key

Basilisk probes the TT with the pure position hash. Current Stockfish adjusts
the TT key using rule-50 buckets once the counter becomes relevant. Reckless
has a similar mechanism. This reduces graph-history interaction where identical
piece placements have materially different remaining winning horizons.

This is not a substitute for correct draw handling. It is a later, measurable
search experiment after rule-50/null behavior is fixed.

### 6.4 TT prefetch

Current Stockfish can prefetch the child TT entry from `do_move()`, and Obsidian
prefetches `keyAfter(move)` before making moves. Basilisk computes child state
without prefetching the future TT bucket.

A `key_after(move)` helper already fits the architecture needed for TT move
validation and rule-50-adjusted keys. Prefetch should be tested on supported
architectures; the value depends on TT size, memory latency, and node shape.

### 6.5 Upcoming repetition

Stockfish uses Marcel van Kervinck's cuckoo algorithm over 3668 reversible move
keys to identify moves that create a repetition without searching every child.
Basilisk performs backward history scans only after positions are reached.

Upcoming repetition is likely a smaller gain than cached geometry or SEE
correctness, but it belongs on the post-StateInfo roadmap because the required
repetition state becomes much cleaner there.

### 6.6 Chess960

Stockfish, PlentyChess, and OpenBench support Chess960/FRC workflows. Basilisk's
board currently assumes standard E1/E8 kings and A/H rooks for castling rights.
Chess960 support is not necessary for standard-chess Elo, but supporting it
forces a more robust representation of castling rook squares and paths and
unlocks FRC regression/competition testing.

Treat this as a product/test-coverage feature, not a near-term standard Elo
priority.

## 7. NNUE and the size of the remaining gap

`PLAN.md` currently targets `+200...400` Elo for the NNUE phase. A large gain
over the current HCE is plausible, but that number should be treated as a
hypothesis rather than a schedule commitment.

The 2026 frontier is no longer merely "has NNUE":

- Stockfish 18 uses SFNNv10 with threat inputs.
- Its release notes describe an automated/reproducible training workflow and
  training based on over 100 billion Lc0-evaluated positions.
- Top independent engines have engine-specific networks, accumulator schemes,
  quantization, feature sets, and search tuning.
- NNUE changes node cost, score distribution, pruning calibration, time
  management, and the value of correction histories.

Basilisk does not need Stockfish-scale data to obtain a strong first NNUE. It
does need a reproducible pipeline and enough board infrastructure that
incremental evaluation is cheaper than repeated refreshes.

Reasonable strength attribution:

| Area | Expected scale | Confidence |
|---|---|---|
| Correctness fixes | Usually tiny average Elo, mandatory | High correctness, low Elo predictability |
| SEE legality | Potential low-single to low-double digit Elo | Medium; high search fan-out |
| Cached geometry/direct checks | Several-percent NPS candidate | Medium; benchmark and SPRT |
| TT/repetition/layout refinements | Small cumulative gains | Medium-low individually |
| Baseline NNUE plus re-tuning | Largest remaining component | High direction, unknown magnitude |
| Threat-aware NNUE/data improvements | Required for frontier parity | High direction, long-term magnitude |

Exact Elo estimates should not be assigned to board micro-optimizations before
same-node benchmarking and paired SPRT.

## 8. Build and release infrastructure

### 8.1 PGO exists but is not published by the release workflow

**Severity:** P1 user-facing performance

CMake has a complete two-phase PGO driver:

- instrumented build;
- depth-13, 40-position bench training;
- `llvm-profdata` merge;
- final LTO+profile-use build;
- `-pgo` distribution asset support.

See `CMakeLists.txt:384-466` and `cmake/pgo-build.cmake`.

However, `.github/workflows/release.yml:143-154` performs a normal preset build,
and lines 185-197 upload the ordinary non-PGO asset. The workflow never invokes
the `pgo` custom target. Consequently the official release binaries are O3/LTO
builds rather than the PGO build used by local strength testing.

Recommended release flow:

1. Configure each desired architecture preset.
2. Run unit/functional tests on the ordinary build.
3. Invoke the PGO target for final release artifacts.
4. Smoke-test the actual PGO binary that will be uploaded.
5. Record bench signature, compiler/version, profile-training depth, and SHA in
   release metadata.

### 8.2 CPU feature tiers are fragmented

`CMakeLists.txt:474-500` defines independent PEXT and AVX2 modes:

- `USE_PEXT`: `-mbmi2`;
- `USE_AVX2`: `-mavx2 -msse4.1 -mpopcnt`.

The release workflow sets `PORTABLE_BUILD=ON`, so `-march=native` is omitted.
The distributed PEXT binary therefore does not explicitly enable POPCNT,
SSE4.1, or AVX2. The AVX2 binary does not select the PEXT attack implementation.

Local PEXT SPRT builds use `-march=native`, which masks this difference during
development. The binary tested locally is not equivalent to the official
portable PEXT asset.

Recommended tiers:

| Tier | Intended features |
|---|---|
| generic x86-64 | conservative portable baseline |
| x86-64-v2/popcnt | POPCNT and common SSE baseline |
| x86-64-bmi2 | AVX2 + BMI2/PEXT + POPCNT/SSE4.1 |
| x86-64-v4/AVX-512 | optional, mainly after NNUE benefits are established |
| aarch64 | NEON baseline and later architecture-specific NNUE kernels |

Either ship separate accurately named assets or implement runtime dispatch for
selected kernels. Stockfish uses a broad architecture matrix and increasingly
specialized NNUE/move-generation paths.

### 8.3 PGO training quality

The 40-position depth-13 bench is a reasonable compact PGO workload and follows
the broad Stockfish convention. The profile should eventually include the
final NNUE evaluator and representative accumulator refresh/update paths.

After NNUE lands, validate that the profile contains:

- normal incremental evaluations;
- king-bucket refreshes;
- captures, promotions, castling, and en-passant updates;
- threat updates, if implemented;
- multithreaded search paths if they materially differ.

PGO performance must be measured using a fresh profile generated from the same
source revision and production feature flags.

## 9. CI and correctness testing

### 9.1 Current CI runs only for releases/manual dispatch

`.github/workflows/release.yml:3-11` is triggered by `workflow_dispatch` and
published releases. There is no push/PR workflow. A broken board transition,
platform-specific warning, or sanitizer failure can therefore reach the merge
history before automation reports it.

Recommended split:

1. **PR CI:** Linux/Clang and Windows/Clang Release build, CTest, bench
   signature.
2. **Debug CI:** ASan+UBSan with randomized state tests.
3. **Nightly:** GCC/Clang matrix, longer randomized/differential tests, deeper
   perft corpus, optional aarch64.
4. **Release CI:** full platform/ISA matrix plus PGO and smoke testing of the
   exact upload assets.

### 9.2 Missing property and differential tests

The deterministic board tests are useful but do not explore combinations of
state. Add a reproducible random walker:

1. Start from a valid FEN.
2. Generate a random legal move sequence.
3. After every make/unmake, recompute and compare:
   - piece/color occupancy;
   - mailbox versus bitboards;
   - king squares;
   - full/pawn/minor/non-pawn keys;
   - castling and EP validity;
   - checker/blocker/pinner state;
   - NNUE incremental versus full refresh evaluation.
4. Unmake the complete sequence and require byte-/field-equivalent state.

Differential tests should compare legal move sets and perft against an
independent implementation such as python-chess and/or Stockfish over thousands
of random legal positions. Do not use Basilisk pseudo-legal generation as the
oracle for Basilisk strict legal generation.

### 9.3 Fuzz targets

Useful fuzz entry points:

- FEN parsing and round-trip;
- UCI `position ... moves ...` parsing;
- make/unmake move sequences;
- TT move decoding and legality validation;
- SEE versus a slow legal capture-tree oracle;
- castling and EP edge positions.

Sanitizers are especially valuable for the fixed history capacity, move-list
capacity assumptions, malformed TT moves, and state-stack boundaries.

### 9.4 Debug invariants

Stockfish has `pos_is_ok()` checks capable of validating piece counts,
bitboards, kings, EP, castling metadata, and material keys after state changes.
Basilisk has private full-hash recomputation but no comprehensive debug
invariant routine called after make/unmake.

Add a debug-only `Board::is_ok()`/`assert_ok()` with individually labeled
failure categories. It should be cheap enough for unit/property tests and
optionally callable after every move in sanitizer builds.

## 10. Performance benchmark problems

`tests/board_performance.cpp` is a useful manual smoke tool but not suitable as
a performance gate.

### 10.1 Measurement methodology

The harness takes the best of three runs (`lines 113-135`). Best-of-N favors
noise and transient boost clocks. Prefer:

- warmup until stable;
- 7-15 measured samples;
- median and median absolute deviation or confidence interval;
- pinned thread affinity where supported;
- compiler, flags, CPU, clock policy, and git SHA in output.

### 10.2 Make/unmake benchmark includes Board copying

`make_unmake_workload(std::vector<Board> boards)` at line 156 accepts the board
vector by value. Every workload invocation copies boards and allocates their
history arrays inside the timed region. This obscures the cost of make/unmake.

Use stable preallocated boards, restore via make/unmake, and prevent the
compiler from eliminating observable state.

### 10.3 Check benchmark is trivial

`check_detection_workload()` calls `is_in_check()`, which reads cached
`checkers`. This benchmarks an integer/bitboard comparison, not attack detection
or child check-state update.

Replace or supplement it with:

- `gives_check(move)` across representative moves;
- update of checkers/blockers/pinners after make;
- attacked-square queries with varied occupancy;
- direct quiet-check generation.

### 10.4 Missing benchmark coverage

Add focused workloads for:

- captures-only and quiet-only staged generation;
- in-check evasions and double check;
- pinned-piece-heavy positions;
- EP legal validation;
- `see_ge()` at representative thresholds;
- key-after plus TT prefetch;
- HCE/NNUE cold refresh versus incremental update;
- complete fixed-depth bench nodes/second.

Microbenchmark gains must be confirmed by full-search nodes/second and SPRT.
Doing more work per node can still gain Elo if it improves pruning; doing fewer
cycles can still lose if node ordering changes adversely.

## 11. Strength-testing infrastructure

### 11.1 What is already strong

The local scripts use fastchess, paired colors, normalized Elo SPRT, fixed test
conditions, PGO candidate builds, and longer-time-control confirmation. This is
considerably better than informal gauntlets and has already supported substantial
validated improvement.

### 11.2 Where top projects compound faster

Local scripts still rely on one machine and manual artifact/result management.
Stockfish Fishtest and OpenBench provide:

- distributed workers;
- automatic source checkout and reproducible compilation;
- persistent test definitions and results;
- automatic statistical stopping;
- game/result uploads;
- patch/commit association;
- consistent opening books and workload metadata.

Sources:

- Fishtest: <https://github.com/official-stockfish/fishtest>
- OpenBench: <https://github.com/AndyGrant/OpenBench>

This is not merely convenience. At the top end, most individual patches are
small and noisy. Infrastructure that cheaply rejects neutral changes and
retains genuine +1 to +3 Elo improvements compounds faster than occasional
large manual experiments.

### 11.3 Reproducibility gaps in local scripts

`tools/build_test.ps1` builds a TUNE-enabled PEXT/PGO candidate and copies a
binary for testing, but it does not itself:

- run CTest before exposing the binary;
- record git SHA, dirty state, compiler version, complete flags, and bench
  signature in a manifest;
- verify a production `TUNE=OFF` binary has identical defaults/bench;
- preserve the exact PGO training profile metadata.

`sprt.ps1` uses randomized opening order but does not persist an explicit random
seed/test manifest. PGN output is useful, but reconstructing every condition
should not depend on terminal history.

Recommended candidate manifest:

```text
git SHA and dirty diff hash
compiler and linker versions
CMake preset/cache variables
TUNE and CPU feature flags
PGO training input/profile hash
engine bench signature
opening book path and SHA-256
random seed/order policy
TC, concurrency, adjudication, SPRT bounds/model
fastchess version
```

## 12. Prioritized remediation roadmap

### Phase A: correctness before tuning

1. Fix rule-50/checkmate precedence.
2. Preserve the halfmove clock across null moves.
3. Correct SEE for pinned attackers and king captures.
4. Store/hash EP only when at least one legal EP capture exists.
5. Add regression tests for every reproduced position.
6. Add a slow independent SEE oracle for tests.

These changes should be separated into reviewable commits. SEE changes can
alter search behavior broadly and should receive both unit tests and SPRT.

### Phase B: state architecture and hot paths

1. Introduce per-ply `StateInfo`.
2. Cache blockers, pinners, and check squares.
3. Reuse cached geometry across capture/quiet stages.
4. Pass `givesCheck` into make-move/checker update.
5. Replace EP board-copy validation with occupancy logic.
6. Generate quiet checks directly.
7. Add piece counts/non-pawn material where universally useful.
8. Consider 16-bit `Move` as part of the same layout work.

For each step record:

- fixed-depth node count/bench signature;
- nodes/second on at least two representative CPUs;
- binary size;
- SPRT result if search behavior changes.

### Phase C: NNUE-ready board

1. Define dirty-piece deltas for every move type.
2. Add a per-thread accumulator stack and king-bucket refresh cache.
3. Validate incremental accumulator state against full refresh after every
   randomized move.
4. Implement a baseline network and quantized SIMD inference.
5. Re-tune search at the NNUE score scale.
6. Add dirty-threat propagation only after the baseline is stable.

### Phase D: release and continuous testing

1. Add PR and sanitizer CI.
2. Correct release ISA tiers.
3. Publish and smoke-test actual PGO artifacts.
4. Repair the performance benchmark and establish pinned baselines.
5. Add fuzz/property/differential test jobs.
6. Adopt OpenBench or an equivalent persistent distributed testing service.

### Phase E: later refinements

1. Rule-50-adjusted TT key.
2. Child TT/history prefetch.
3. Upcoming-repetition cuckoo lookup.
4. Chess960-capable castling representation.
5. AVX-512/VNNI and architecture-specific NNUE kernels where supported by
   measurement and user hardware distribution.

## 13. Verification performed during this audit

- Rebuilt and ran `test_board` in the Release PEXT/PGO configuration.
- Result: **253 / 253 board tests passed**.
- Reproduced the rule-50/checkmate error in the shipped 1.8.0 binary.
- Reproduced the pinned-recapturer SEE error directly through the board library.
- Reproduced the illegal-but-hashed en-passant square directly through the
  board library.
- Inspected current Stockfish, Reckless, PlentyChess, and Obsidian board/state
  implementations.
- Inspected Basilisk CMake presets, PGO driver, release workflow, board
  benchmark, unit tests, and local SPRT/SPSA scripts.

The fact that all existing board tests pass while the three concrete edge cases
fail is the central testing lesson: deterministic perft and self-consistency
tests are necessary, but property tests and independent oracles are required to
catch correlated implementation mistakes.

