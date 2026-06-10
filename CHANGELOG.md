# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.6.0] - 2026-06-20

### Changed

#### Strength / Evaluation
- Texel-tuned and SPRT-accepted material values (mg/eg piece values)
- Texel-tuned and SPRT-accepted mobility weights (knight/bishop/rook/queen, mg/eg)
- Texel-tuned and SPRT-accepted passed-pawn tables and king-proximity bonus
- Texel-tuned and SPRT-accepted pawn-structure terms (doubled, isolated, connected, backward)
- Baked a Texel-fit hanging-piece penalty table (holdout-loss improvement); not yet confirmed by its own isolated SPRT — see Known Gaps
- Tried and reverted a rook-term candidate (open/semi-open file, 7th rank, behind-passer bonuses) after an inconclusive SPRT

#### Time Management
- Replaced the clock-path time budget with a Stockfish-style formula (ported from the sibling Rarog engine): logarithmic time-left scaling, ply-aware sudden-death and explicit-`movestogo` branches
- Added an absolute time-safety reserve (2x `Move Overhead`) on top of the formula's percentage cap, bounding worst-case overshoot from command-queue dispatch latency, Syzygy root probing, and `check_stop()` polling granularity
- Fixed-`movetime` searches no longer subtract `Move Overhead` from the budget; GUIs tolerate ~10% over nominal and movetime games never forfeit on time the way clock play does

#### Version
- Bumped engine version metadata to 1.6.0

### Fixed

#### Time Management
- Fixed a regression where in-development eval/search work tripled time forfeits at fast clock controls (65 losses vs 1.5.0's 18, `tc=3+0.03`); the rewrite above produced zero time losses across a 35,000-game validation pool (see Testing)

### Added

#### Tooling
- Added a linear-trace gradient (Texel) tuner: `basilisk-texel` CMake target, `EvalParams` weight struct (~900 parameters), a tune-time eval-file loader/dumper, dataset generation scripts (`tools/datagen.ps1`, `tools/texel/extract.py`, `tools/texel/sample_fens.py`, `tools/texel/import_beast.py`), and staged per-group tuning with sign/shape constraints
- Added `-TC`/`-MoveTime`/`-TimeMargin` clock-control parameters to `tools/gauntlet.ps1`, matching `tools/sprt.ps1`'s clock-based default (`tc=10+0.1`)

### Testing

#### SPRT (eval scalars, vs previous accepted head, `tc=3+0.03`)
- Material vs the Phase 2 baseline (Phase 1's accepted defaults): `1,930` games, `+29.05 +/- 11.36 Elo`, H1 accepted
- Mobility vs material: `7,288` games, `+8.77 +/- 5.68 Elo`, H1 accepted
- Passed pawns vs mobility: `3,462` games, `+16.57 +/- 8.28 Elo`, H1 accepted
- Pawn structure vs passed pawns: `1,836` games, `+30.74 +/- 11.76 Elo`, H1 accepted
- Rooks vs pawn structure: `10,666` games (stopped manually), `+3.13 +/- 4.74 Elo`, LOS `90.21%`, inconclusive below the `elo1=5` target; reverted

#### Gauntlet / Pool Validation
- 35,000-game self-play pool at `tc=3+0.03` (partial-tuning development build, before the time-management fix): 2nd of 9, approx `+54 Elo` vs Basilisk 1.5.0
- 35,000-game LittleBlitzer pool at `tc=3+0.03` (post time-management fix, default `Move Overhead`): `0` time losses across all games for every engine in the pool; Basilisk scored rating `2696.8`, `62.7%` (2nd of 5), behind only Stockfish-18-2700-capped and well clear of Rarog 2.1.0 (`2620.0`) and Stockfish-2600/2500-capped

#### Build / Verification
- Passed the full CTest suite: 8/8 tests
- Recorded `bench 13`: `4,033,379` nodes (non-PGO release-pext)

### Known Gaps
- The hanging-piece penalty bake does not have its own isolated SPRT confirmation, unlike the other eval terms above — only the cumulative effect including it was validated via the gauntlet and LittleBlitzer pools. A standalone confirmatory SPRT is a candidate for a future release.

---

## [1.5.0] - 2026-06-07

### Changed

#### Strength / Search
- Re-tuned the existing pruning and LMR search constants with weather-factory SPSA, keeping the search algorithm itself unchanged
- Baked the SPRT-accepted pruning candidate:
  `RfpCoeff=160`, `RfpImproving=72`, `RazorCoeff=243`, `NullBase=3`,
  `NullEvalDiv=192`, `ProbCutMargin=189`, `FutilityBase=180`,
  `FutilityCoeff=128`, `HistPruneCoeff=4210`, `SeePruneCoeff=73`,
  `SingularBetaMult=4`, `SingularDoubleMargin=4`, `AspirationDelta=19`
- Baked the SPRT-accepted LMR candidate:
  `LmrBase=60`, `LmrDivisor=209`, `LmrHistDiv=7830`,
  `LmrNonPvAdj=1`, `LmrCutNodeAdj=0`, `LmrTtPvAdj=0`,
  `LmrNotImprovingAdj=0`
- Rejected and reverted the narrowed combined polish candidate after it failed
  its SPRT gate

#### Version
- Bumped engine version metadata to 1.5.0

### Added

#### Tooling
- Added `tools/gauntlet.ps1` for fixed-game phase-boundary validation matches
- Added `tools/spsa_configs/config_combined.json` for the rejected Phase 1
  combined-polish experiment, retained as documentation and a future reference
- Extended `tools/setup_spsa.ps1` with selectable engine suffixes and automatic
  archival of old weather-factory state before fresh runs
- Added `combined` as an SPSA config group for narrowed search-constant polish

#### Documentation
- Updated `PLAN.md` and `user_dev_guide.md` to record Phase 1 completion and
  make Phase 2 evaluation tuning the next planned work
- Documented the repo-local SPSA/SPRT/validation workflow and generated-artifact
  handling

### Testing

#### SPRT
- Pruning candidate vs 1.4.9/defaults:
  `2930` games, `850 - 691 - 1389`, `52.71%`,
  `+18.87 +/- 8.81 Elo`, H1 accepted for `[0.00, 5.00]`
- LMR candidate vs pruning head:
  `3714` games, `1091 - 924 - 1699`, `52.25%`,
  `+15.63 +/- 8.02 Elo`, H1 accepted for `[0.00, 5.00]`
- Combined-polish candidate vs LMR head:
  `23210` games, `6375 - 6402 - 10433`, `49.94%`,
  `-0.40 +/- 3.20 Elo`, H0 accepted for `[0.00, 5.00]`;
  candidate reverted

#### Phase 1 Validation
- Phase1Final vs Basilisk 1.4.9/defaults:
  `2000` games, `638 - 482 - 880`, `53.90%`, approx `+27.16 Elo`

#### Build / Verification
- Built `tools\test_engines\basilisk-phase1-final-pext-pgo.exe`
- Passed the full CTest suite: 8/8 tests
- Recorded `bench 13` for the accepted Phase 1 head:
  `4,283,684` nodes

---

## [1.4.9] - 2026-05-29

### Added

#### Build / Release
- Added profile-guided optimization support to CMake through `BASILISK_PGO=GENERATE/USE`, with Clang `.profdata` and GCC profile-directory flows
- Added a local CMake `pgo` build target that runs the instrumented build, `bench 13` plus embedded EPD-position training, profile merge, and optimized rebuild
- Added release-style binary copies under `build/dist`, including `-pgo` suffixed asset names for PGO builds
- PGO training now prints concise live progress during the build while keeping detailed engine logs in the PGO profile directory
- Expanded the embedded PGO training set with black-to-move middlegames, tactical/check positions, castling and en-passant paths, rook/minor-piece endgames, and promotion races
- Documented local PGO release usage

### Changed

#### Search / Performance
- Delayed direct-check detection in the LMR path until cheaper reduction gates have already passed
- Cached direct-check masks during quiet move scoring and removed an unused TT-move scoring branch from the quiet scorer
- Added a boolean attack-test helper and used it in legality, castling, passed-pawn, and hanging-piece checks to avoid materializing full attacker bitboards where only yes/no information is needed
- Added a fast non-insufficient-material exit before expensive draw-material checks
- Reduced move-picker overhead by scanning with pointers and avoiding self-swaps

#### Version
- Bumped engine version metadata to 1.4.9

### Testing

#### Release Verification
- Built `release-avx2` successfully with MSYS2 Clang/Ninja
- Passed the full CTest suite: 8/8 tests
- Ran AVX2 Cutechess regression against 1.4.7: `31 - 23 - 66` over 120 games, 53.3%, +23.2 +/- 41.8 Elo, LOS 86.2%
- Ran AVX2 Cutechess regression against the previous `Version 1.4.9` development commit: `40 - 35 - 45` over 120 games, 52.1%, +14.5 +/- 49.4 Elo, LOS 71.8%
- Recorded `bench 13` AVX2 over three runs: candidate averaged 4,972,548 nodes, 1625.7 ms, and 3,059,573 nps; 1.4.7 averaged 5,838,881 nodes, 1917.7 ms, and 3,044,946 nps; the previous 1.4.9 development commit averaged 6,156,236 nodes, 2050.7 ms, and 3,002,864 nps
- Verified the expanded 68-position embedded PGO training set with the AVX2 `pgo` target: no invalid or unsupported FEN messages, final PGO binary averaged 1620.9 ms and 3,068,384 nps over seven `bench 13` runs
- Compared EPD training depths 7, 8, and 9; kept depth 7 because depths 8 and 9 increased PGO build time by 8.7% and 10.7% without a meaningful bench-speed gain
- Clean Cutechess log scan found no illegal moves, crashes, disconnects, timeouts, or forfeits

---

## [1.4.8] - 2026-05-28

### Fixed

#### UCI / Ponder
- Added a child-position TT fallback for `pondermove` reporting when the root PV is too short, so Basilisk can still report a legal ponder reply after the selected best move
- Kept strict legality validation on the recovered ponder move before exposing it through UCI

#### Search
- Changed qsearch cutoff returns to fail-soft scores instead of clamping to `beta`, improving bound quality for callers and TT/search consumers

### Added

#### Testing
- Added focused search coverage for recovering a legal ponder move from the child-position transposition table entry

### Changed

#### Version
- Bumped engine version metadata to 1.4.8

### Testing

#### Release Verification
- Built `release-avx2` successfully with MSYS2 Clang/Ninja
- Passed the full CTest suite: 8/8 tests
- Ran medium Cutechess book regression against 1.4.7: 1T scored `15 - 13 - 36` over 64 games (+10.9 +/- 56.7 Elo), 8T scored `14 - 12 - 38` over 64 games (+10.9 +/- 54.6 Elo)
- Clean Cutechess log scan found no illegal moves, crashes, disconnects, timeouts, forfeits, warnings, or invalid-game markers

---

## [1.4.7] - 2026-05-28

### Fixed

#### UCI / Ponder
- `ponderhit` now preserves elapsed ponder time when switching to normal search instead of granting a fresh move budget
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
- Verified live UCI ponder behavior: `bestmove` is withheld during `go ponder depth 1` and emitted only after `stop` or `ponderhit`

### Changed

#### Search Threads
- `setoption name Threads value N` now resizes the persistent search pool immediately rather than delaying worker creation until the next search
- Thread resizing is exact in both directions: reducing `Threads` tears down helper workers instead of keeping stale workers alive
- Helper threads now run full-root Lazy SMP searches over the shared TT/root table instead of partitioning root moves
- The UCI `Threads` maximum now follows `max(1024, 4 * hardware_concurrency)`; worker creation failure is reported and the active count is reduced

#### Version
- Bumped engine version metadata to 1.4.7

### Testing

#### Release Verification
- Built `release-avx2` successfully with MSYS2 Clang/Ninja
- Passed the full CTest suite: 8/8 tests
- Passed focused test binaries: `test_board` 253/253, `test_search` 122/122, `test_engine_ponder` 10/10, `test_engine_threading` 9/9, `test_uci_protocol` 32/32
- Live UCI comparison verified that `go ponder depth 1` withheld `bestmove` until both `stop` and `ponderhit`; Basilisk now also applies `Threads` before `isready` and completes `go nodes N` without requiring `stop`
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
- Validated the release candidate with a 1,800-game 100 ms/move round-robin
- Basilisk 1.4.3 scored 62.5% vs Basilisk 1.3.0 (+88.6 +/- 28.7 Elo) and 60.5% in the second validation leg (+73.9 +/- 28.4 Elo)

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

[1.6.0]: https://github.com/maelic13/basilisk/compare/v1.5.0...v1.6.0
[1.5.0]: https://github.com/maelic13/basilisk/compare/v1.4.9...v1.5.0
[1.4.9]: https://github.com/maelic13/basilisk/compare/v1.4.8...v1.4.9
[1.4.8]: https://github.com/maelic13/basilisk/compare/v1.4.7...v1.4.8
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
