# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.4.7] - 2026-05-28

### Fixed

#### UCI / Ponder
- `ponderhit` now preserves elapsed ponder time when switching to normal search, matching Stockfish-style time accounting instead of granting a fresh move budget
- Ponder searches that finish their depth limit now wait for `stop` or `ponderhit` before emitting `bestmove`, including multi-threaded searches
- Stale `stop` and `ponderhit` control flags are cleared after a search completes so a previous command cannot poison the next `go ponder`

#### UCI / Search Limits
- `go nodes N` is no longer treated as an infinite search by the engine thread, so node-limited UCI searches return `bestmove` without requiring a later `stop`
- Increment and `movestogo` UCI parameters are now included in finite-search detection instead of falling through to the infinite/ponder wait path

#### Search Threads
- Multi-threaded node limits are now enforced against an aggregate shared node counter instead of dividing the limit per helper
- UCI `info` from threaded searches now reports aggregate nodes and tablebase hits instead of main-thread-only values

#### Search / Move Legality
- Hardened TT move validation so stale, aliased, or SMP-corrupted TT moves must be pseudo-legal before search can play them
- Prevented malformed TT moves from corrupting board state and leaking illegal `info ... pv` lines to tournament GUIs
- Corrected full SEE minimax folding so defended losing captures and quiet promotions that can be recaptured are scored correctly

#### Bench
- Corrected an invalid built-in benchmark FEN so `bench 13` completes under strict FEN validation

### Added

#### UCI / Analysis Commands
- Added `go searchmoves ...` root move restrictions, including threaded search support
- Added `go mate N` parsing, converting mate targets to the corresponding bounded search depth and stopping once a requested mate is found
- Added `go perft N`, which reports legal leaf nodes from the current position without emitting `bestmove`

#### Search
- Added threshold SEE (`see_ge`) for hot pruning paths
- Added qsearch capture-futility and dynamic threshold-SEE pruning
- Added high-depth null-move verification before accepting risky null cutoffs

#### Testing
- Added `test_engine_ponder`, an engine-thread regression target covering completed ponder searches waiting for `stop`, completed ponder searches waiting for `ponderhit`, and stale control-state cleanup before the next ponder search
- Added `test_engine_threading`, an engine-thread regression target covering immediate `Threads` resize behavior before `isready` and threaded node-limited search completion
- Added search-layer coverage that verifies `ponderhit` uses elapsed ponder time instead of resetting the clock
- Expanded search-layer thread-pool coverage for exact grow/shrink behavior, repeated threaded searches, aggregate node limits, and threaded UCI node reporting
- Added strict board-legality and corrupt-TT regression coverage for the tournament-derived illegal PV sequence
- Added board/search/engine coverage for threshold SEE, `searchmoves`, `mate`, and `go perft`
- Verified live UCI behavior against local Stockfish: both engines withhold `bestmove` during `go ponder depth 1` and emit it only after `stop` or `ponderhit`

### Changed

#### Search Threads
- `setoption name Threads value N` now resizes the persistent search pool immediately, matching Stockfish's option lifecycle rather than delaying worker creation until the next search
- Thread resizing is exact in both directions: reducing `Threads` tears down helper workers instead of keeping stale workers alive
- Helper threads now run full-root Lazy SMP searches over the shared TT/root table instead of partitioning root moves
- The UCI `Threads` maximum now follows Stockfish's `max(1024, 4 * hardware_concurrency)` model; worker creation failure is reported and the active count is reduced

#### Version
- Bumped engine version metadata to 1.4.7

### Testing

#### Release Verification
- Built `release-avx2` successfully with MSYS2 Clang/Ninja
- Passed the full CTest suite: 8/8 tests
- Passed focused test binaries: `test_board` 253/253, `test_search` 122/122, `test_engine_ponder` 10/10, `test_engine_threading` 9/9, `test_uci_protocol` 32/32
- Live UCI comparison with `D:\chess\engines\stockfish.exe`: `go ponder depth 1` withheld `bestmove` until both `stop` and `ponderhit`; Basilisk now also applies `Threads` before `isready` and completes `go nodes N` without requiring `stop`
- Live UCI smoke verified `go searchmoves e2e4 depth 1`, `go perft 1`, and `go mate 1`
- Verified no current-engine illegal PV warnings in the focused Cutechess repro; warnings remaining in comparison logs came from the 1.4.5 opponent
- Ran quick Cutechess book smoke against 1.4.5: 1T scored 6-3-11 over 20 games (+52 +/- 105 Elo), 8T scored 16-0-0 over 16 games
- Recorded `bench 13` on the local release-avx2 build: 1T averaged 11,530,305 nodes at 2,871,808 nps; 8T averaged 16,709,108 nps over three runs

---

## [1.4.5] - 2026-05-28

### Fixed

#### UCI / Position Handling
- Added checked FEN parsing that rejects malformed board encoding, invalid side-to-move, invalid castling fields, invalid en-passant squares, invalid counters, missing kings, adjacent kings, pawns on impossible ranks, and impossible piece counts
- UCI `position` now rejects invalid FEN input without replacing the current valid board
- UCI `position ... moves ...` now applies the move list atomically, so an illegal move leaves the previous board untouched
- Castling rights from FEN are now sanitized when the corresponding king or rook is unavailable
- Fullmove counter `0` is tolerated as `1` for compatibility with common tournament-manager FEN output

#### Move Generation
- En-passant check evasions now correctly remove the captured pawn from the post-move attack test, so the only legal reply to a pawn check is not filtered out

### Added

#### UCI
- Added the `Ponder` UCI option and explicit `go ponder` / `ponderhit` protocol coverage

#### Testing
- Added FEN validation coverage for malformed placement, illegal counters, impossible material, invalid kings, castling-right repair, en-passant validation, and tournament-manager fullmove-zero input
- Added UCI protocol coverage for invalid `position` commands, atomic move-list application, and ponder mode

### Changed

#### Version
- Bumped engine version metadata to 1.4.5

### Testing

#### Release Verification
- Built `release-avx2` successfully with MSYS2 Clang/Ninja
- Passed the full CTest suite: 6/6 tests
- Passed focused test binaries: `test_board` 242/242, `test_search` 99/99, `test_uci_protocol` 32/32
- Replayed all 43 `illegal*` tournament artefact command files with 0 position errors, 0 illegal bestmoves, and 0 `0000` responses
- Ran a 100-game Cutechess smoke against Basilisk 1.4.4 AVX2 at 1+0.1 with concurrency 4: current scored 28-30-42, 49.0%, -6.9 +/- 52.2 Elo

---

## [1.4.4] - 2026-05-28

### Fixed

#### UCI / Search Safety
- Fixed a serious 100 ms/move tournament regression where Basilisk could emit illegal `bestmove 0000` from non-terminal positions
- Preserved queued `go` and `bench` commands when a GUI immediately follows them with `quit`, instead of priority-inserting `quit` ahead of the queued work
- Added EOF handling so the UCI loop always wakes the engine thread with a `quit` command when stdin closes
- Hardened stopped infinite/ponder searches so they return a legal fallback move when the GUI has already requested stop
- Strengthened final search-result sanitization coverage for both legal positions and terminal checkmates

#### Board Rules
- Castling generation and legality now require the king and rook to actually be present on their castling squares
- Castling moves now correctly report rook-discovered checks
- Repetition detection now distinguishes root repetitions from in-tree repetitions and stops scanning across null moves
- Draw detection now includes insufficient-material positions in the general `is_draw()` path

#### UCI Parsing
- Invalid numeric `go` and `setoption` values are ignored or clamped instead of throwing through `std::stoi`
- Invalid bare `go` numeric input falls back to the normal default depth search instead of accidental infinite search

### Added

#### Search
- Added 4-ply continuation history, pawn-structure keyed quiet history, and low-ply quiet history
- Added broader correction histories keyed by pawn structure, minor-piece placement, non-pawn placement, and continuation context
- Added staged good/bad tactical move ordering so SEE-losing captures are delayed behind quiet moves
- Added TT prefetching, exact-entry replacement preference, and current-age-aware `hashfull`
- Added root best-move effort tracking for adaptive time management
- Added Lazy SMP helper root diversification and helper history blending

#### Board
- Added incremental minor-piece and non-pawn Zobrist keys to support fast correction-history lookup
- Added `check_squares()` helper for cheap direct-check move scoring

#### Evaluation
- Added 50-move-rule score damping for non-mating evaluations

#### Testing
- Added a dedicated `test_uci_protocol` CTest target covering queued `go`/`bench`/`quit`, EOF-triggered quit, and `uci` output
- Added regression coverage for incremental piece keys, null-move repetition boundaries, stopped-search fallback, terminal sanitizer behavior, TT 50-move score clamping, current-age `hashfull`, and TT prefetch safety

### Changed

#### Move Ordering
- Quiet move scoring now combines main history, continuation history, pawn history, low-ply history, direct-check bonuses, killers, countermoves, tablebase root ordering, and shared root ordering

#### Version
- Bumped engine version metadata to 1.4.4

### Testing

#### Release Verification
- Built `release-avx2` successfully with MSYS2 Clang/Ninja
- Passed the full CTest suite: 6/6 tests
- Verified direct UCI stress on 40 legal non-terminal positions at 100 ms/move: 0 illegal moves and 0 `bestmove 0000`
- Verified a short 4-game `chess_tester` smoke against Basilisk 1.3.0 at 100 ms/move with 0 engine errors
- Recorded `bench 13` at 13,367,400 nodes and 2,435,750 nps on the local release-avx2 build

---

## [1.4.3] - 2026-05-27

### Fixed

#### Search
- Corrected pawn correction-history updates to compare search results against the corrected static evaluation, avoiding self-reinforcing correction drift
- Quiescence search now continues legal check evasions until the qsearch ply cap instead of falling back to static evaluation after one evasion ply
- Late move pruning, quiet-history pruning, bad-capture SEE pruning, and LMR now preserve checking moves

#### Evaluation
- Passed-pawn advance safety now checks all enemy attackers on the stop square, not only enemy pawn attacks

#### Tablebases
- Root Syzygy search now filters to the best tablebase rank before normal search, so inferior root tablebase moves are not searched as candidates

### Changed

#### Version
- Bumped engine version metadata to 1.4.3

### Testing

#### Strength
- Validated the release candidate with a 1,800-game 100 ms/move round-robin against Basilisk 1.3.0 and Lynx 1.2.1
- Basilisk 1.4.3 scored 62.5% vs Basilisk 1.3.0 (+88.6 +/- 28.7 Elo) and 60.5% vs Lynx 1.2.1 (+73.9 +/- 28.4 Elo)

---

## [1.4.2] - 2026-05-26

### Fixed

#### Build / Release
- Fixed `COMP=llvm` configuration on Apple Silicon macOS when CMake reports an empty host or target processor before compiler detection
- Improved Apple Silicon macOS build rejection diagnostics to include the detected host or target platform

### Changed

#### Version
- Bumped engine version metadata to 1.4.2

---

## [1.4.1] - 2026-05-26

### Added

#### Build / Release
- Added `COMP=auto|clang|gcc|llvm` compiler selection for CMake configuration

### Changed

#### Build / Release
- Local `COMP=auto` builds now default to Clang on all supported platforms, including AppleClang on macOS
- GitHub Actions now passes compiler selection explicitly for every release asset
- Linux and Windows release assets continue to use Clang explicitly
- macOS release assets continue to use AppleClang for compatibility
- Intel macOS builds now fail early because macOS release support is Apple Silicon only

#### Documentation
- Documented compiler selection, Apple Silicon defaults, and optional Homebrew LLVM comparison builds

#### Version
- Bumped engine version metadata to 1.4.1

---

## [1.4.0] - 2026-05-25

### Added

#### Tablebases
- Added `SyzygyProbeLimit` UCI option, matching the supported 0-7 tablebase cardinality range
- Added root tablebase metadata so root moves are ranked by tablebase outcome while the normal search still breaks equivalent TB ties
- Added tablebase PV expansion for UCI `info ... pv` output
- Added WDL/DTZ file count reporting when Syzygy tablebases are loaded

#### Testing
- Added coverage for bare `go` default depth, `SyzygyProbeLimit` option parsing/clamping, tablebase file counts, probe-limit gating, rule-50 root scoring, root TB metadata, and expanded TB PV legality

### Changed

#### Search / UCI
- Bare `go` now defaults to depth 7 instead of a fixed 500 ms search
- Syzygy root positions now search normally instead of returning a zero-node single tablebase move
- Tablebase wins/losses now report bounded `cp 20000` scores
- `Syzygy50MoveRule=true` now reports root cursed wins and blessed losses as draw scores
- Time management uses a lower minimum move budget and caps hard limits to avoid spending too much of the remaining clock

#### Version
- Bumped engine version metadata to 1.4.0

### Fixed

#### Search
- Avoided reapplying correction history to static evals loaded from the transposition table
- Skipped repetition scanning when the halfmove clock is too small for a prior reversible position

---

## [1.3.0] - 2026-05-25

### Added

#### Tablebases
- Added optional Syzygy tablebase support via the vendored MIT-licensed Fathom probe library
- Added root DTZ/WDL tablebase move selection before normal search when `SyzygyPath` is configured
- Added non-root WDL probes in search, limited to positions where the 50-move counter can be handled safely
- Added `SyzygyPath`, `SyzygyProbeDepth`, and `Syzygy50MoveRule` UCI options
- Added `tbhits` reporting to UCI search info

### Changed

#### Version
- Bumped engine version metadata to 1.3.0

#### Build / Release
- Removed Intel macOS release assets; macOS releases now target Apple Silicon only
- Kept x86_64 release assets for generic, AVX2, and PEXT builds on Linux and Windows
- PEXT builds now use BMI2 PEXT sliding-piece attack lookup instead of only enabling BMI2 compiler code generation
- AVX2 startup checks now compose correctly with PEXT when both feature flags are enabled

#### Search / Evaluation
- Internal iterative reductions now avoid PV nodes, preserving depth on the principal variation
- Added protected/candidate passed-pawn evaluation and lightweight passed-pawn advance bonuses
- Added enemy pawn-storm pressure against castled or flank kings

---

## [1.2.3] - 2026-05-23

### Fixed

#### UCI / Search Safety
- Added a final legality sanitizer so stale or malformed search results cannot emit an illegal `bestmove`
- Invalid ponder moves are now cleared before `bestmove ... ponder ...` is printed
- UCI `info ... pv` output is now truncated before any invalid PV move instead of reporting an illegal continuation
- Shared root-move aggregation now ignores moves that are not in the legal root move table

### Added

#### Testing
- Added regression coverage for the terminal positions from the tournament rules-infraction PGNs
- Added search-result sanitizer and PV legality tests to protect future UCI output changes

---

## [1.2.2] - 2026-05-22

### Changed

#### Build / Release
- Simplified x86_64 release assets to generic, `avx2`, and `pext`
- Added the `release-avx2` preset for manual local AVX2 builds
- Kept `release` and `release-pext` native by default for local builds while GitHub Actions produces portable release assets
- x86 feature builds now check CPU support at startup and report a clear error on unsupported hosts

---

## [1.2.1] - 2026-05-22

### Fixed

#### Search
- Iterative deepening no longer stops at the first forced mate; it now continues searching so deeper iterations can find shorter mating nets
- Mate-like previous scores now disable aspiration windows, avoiding unstable narrow-window re-searches around forced mates
- Added regression coverage for a KQK endgame where the engine previously accepted a longer mate instead of resolving the shorter mate

## [1.2.0] - 2026-05-21

### Added

#### Search
- `Threads` now drives a persistent Lazy SMP search pool with shared TT and root-move feedback
- Shared root move table for helper-thread result aggregation and root move ordering
- Staged `MovePicker` that searches TT move, tactical moves, then quiet moves
- Quiet-only legal move generation so quiet moves are generated only when needed

#### Engine / UCI
- Engine command queue with priority commands for `quit` and serialized state changes
- Separate stop, search, quit, and ponderhit control flags
- `bench [depth]` now uses the current `Threads` option and reports the active thread count

#### Testing
- Board coverage for color-aware pawn keys and quiet-only legal generation
- Search coverage for repeated thread-pool searches and command-queue priority ordering
- TT coverage updated to use copy-based probes

### Changed
- Transposition table storage is now a compact atomic format with 64-byte clusters for safer shared access
- Search no longer holds pointers into TT storage; probes return stable entry copies
- Capture move ordering avoids eager SEE calls; SEE is still used for pruning and bad-capture reductions
- Pawn correction history now uses a color-aware pawn key instead of color-blind pawn bitboards
- Release CI now runs the CTest suite before binary smoke tests and upload

### Fixed
- `isready`/`go` race where a readiness ping immediately after a queued search could wait behind the search
- Queued `stop`, `quit`, and replacement `go` commands can no longer be erased by a stale search startup
- `ponderhit` now transitions ponder searches through an atomic signal instead of restarting search state
- Pawn-key collisions between color-swapped pawn structures

---

## [1.1.0] - 2026-05-21

### Added

#### Search
- **Check extension** — extend search depth by 1 ply when the side to move is in check; guards prevent stacking with singular extensions and stack overflow
- **Extended IIR** — Internal Iterative Reductions now also trigger when the TT entry was searched at a much shallower depth (`tt_depth < depth - 3`), not only when no TT move exists
- TT probe in quiescence search for early cutoffs

#### Evaluation
- **King safety** — full attack-unit table with piece coordination and isolation bonuses; attack threat is reduced by one-third when the opponent has no queen
- **Mobility scoring** — bishop and rook mobility bonuses added to tapered evaluation
- **Pawn structure** — passed pawn, isolated pawn, and doubled pawn bonuses/penalties, scaled by game phase
- **Endgame scaling** — drawish endgame detection reduces evaluation magnitude in likely-drawn positions

#### Board
- Optimised `gen_legal` with `captures_only` flag to avoid generating all moves in quiescence
- Checkers cache for faster repeated in-check queries
- `gives_check(Move)` — O(1) check detection via direct + discovered attack tables
- `gen_quiet_checks(MoveList&)` — generates all legal quiet moves that give check

#### Testing
- Board correctness suite: perft tests for starting position, Kiwipete, Talkchess and endgame positions
- Move encoding tests: encoding/decoding round-trips for all move types
- Transposition table tests: store/probe, replacement policy, generational aging, thread safety
- Evaluation tests: symmetric evaluation, draw detection, passed/isolated/doubled pawn scoring
- Search tests: mate-in-N, draw detection, depth/node-limit compliance
- Board performance benchmark for regression tracking against other implementations

### Changed
- ELO estimate updated to ~2400 (calibrated against limited-strength reference matches; FIDE Master / International Master level)
- README expanded with full feature list, eval details, and testing instructions

---

## [1.0.0] - 2026-05-20

First public release.

### Added

#### Engine
- Full UCI protocol implementation (uci, debug, isready, setoption, ucinewgame, position, go, stop, ponderhit, quit)
- Iterative deepening with aspiration windows
- Negamax alpha-beta / Principal Variation Search (PVS)
- Transposition table — 3-entry clusters aligned to 32-byte cache lines with generational aging
- Null move pruning, reverse futility pruning (RFP), razoring, ProbCut
- Late move reductions (LMR) and singular extensions
- Quiescence search with in-check evasion
- Static Exchange Evaluation (SEE) for move ordering and capture pruning
- Move ordering: TT move, MVV/LVA captures scored with SEE, killer moves, countermove heuristic, quiet history, capture history
- Hand-crafted evaluation (HCE): material, piece-square tables, mobility, pawn structure (passed, isolated, doubled pawns), king safety, endgame scaling
- Zobrist hashing with repetition detection
- Magic bitboard attack generation

#### UCI Options
- `Hash` — transposition table size in MB (1 – 32768 MB)
- `Threads` — number of search threads (1 – 256)
- `Move Overhead` — clock safety margin in ms

#### Build / Tooling
- CMake presets: `release`, `release-pext`, `debug`, `relwithdebinfo`
- `CMakeUserPresets.json.example` for machine-local compiler paths
- Automatic static runtime linking on MinGW (produces self-contained Windows executables)
- `bench [depth]` command — 16-position built-in benchmark, prints per-position NPS and total node-count fingerprint
- GitHub Actions release workflow — builds for Linux x86_64, Linux aarch64, Windows x86_64, Windows aarch64, macOS aarch64; all built with Clang; PEXT variant produced for x86_64 platforms

[1.4.7]: https://github.com/maelic13/basilisk/compare/v1.4.5...v1.4.7
[1.4.5]: https://github.com/maelic13/basilisk/compare/v1.4.4...v1.4.5
[1.4.4]: https://github.com/maelic13/basilisk/compare/v1.4.3...v1.4.4
[1.4.3]: https://github.com/maelic13/basilisk/compare/v1.4.2...v1.4.3
[1.4.2]: https://github.com/maelic13/basilisk/compare/v1.4.1...v1.4.2
[1.4.1]: https://github.com/maelic13/basilisk/compare/v1.4.0...v1.4.1
[1.4.0]: https://github.com/maelic13/basilisk/compare/v1.3.0...v1.4.0
[1.3.0]: https://github.com/maelic13/basilisk/compare/v1.2.3...v1.3.0
[1.2.3]: https://github.com/maelic13/basilisk/compare/v1.2.2...v1.2.3
[1.2.2]: https://github.com/maelic13/basilisk/compare/v1.2.1...v1.2.2
[1.2.1]: https://github.com/maelic13/basilisk/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/maelic13/basilisk/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/maelic13/basilisk/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/maelic13/basilisk/releases/tag/v1.0.0
