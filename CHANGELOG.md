# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
- Added `SyzygyProbeLimit` UCI option, matching Stockfish's `0 – 7` probe cardinality cap
- Added Stockfish-style root tablebase metadata so root moves are ranked by tablebase outcome while the normal search still breaks equivalent TB ties
- Added tablebase PV expansion for UCI `info ... pv` output
- Added WDL/DTZ file count reporting when Syzygy tablebases are loaded

#### Testing
- Added coverage for bare `go` default depth, `SyzygyProbeLimit` option parsing/clamping, tablebase file counts, probe-limit gating, rule-50 root scoring, root TB metadata, and expanded TB PV legality

### Changed

#### Search / UCI
- Bare `go` now defaults to depth 7 instead of a fixed 500 ms search
- Syzygy root positions now search normally instead of returning a zero-node single tablebase move
- Tablebase wins/losses now report Stockfish-style `cp 20000` scores
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
- ELO estimate updated to ~2400 (calibrated against Stockfish UCI_LimitStrength; FIDE Master / International Master level)
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
- `Ponder` — enable ponder mode

#### Build / Tooling
- CMake presets: `release`, `release-pext`, `debug`, `relwithdebinfo`
- `CMakeUserPresets.json.example` for machine-local compiler paths
- Automatic static runtime linking on MinGW (produces self-contained Windows executables)
- `bench [depth]` command — 16-position built-in benchmark, prints per-position NPS and total node-count fingerprint
- GitHub Actions release workflow — builds for Linux x86_64, Linux aarch64, Windows x86_64, Windows aarch64, macOS aarch64; all built with Clang; PEXT variant produced for x86_64 platforms

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
