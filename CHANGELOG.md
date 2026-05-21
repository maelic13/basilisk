# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

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

[1.2.0]: https://github.com/maelic13/basilisk/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/maelic13/basilisk/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/maelic13/basilisk/releases/tag/v1.0.0
