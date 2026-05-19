# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[1.0.0]: https://github.com/maelic13/basilisk/releases/tag/v1.0.0
