# Basilisk

A UCI chess engine written in C++23.

**Estimated strength: ~2400 ELO** (single-thread calibration against Stockfish; FIDE Master / International Master level)

---

## Features

### Search
- Iterative deepening with aspiration windows
- Negamax alpha-beta / Principal Variation Search (PVS)
- Persistent Lazy SMP thread pool with shared TT and shared root-move feedback
- Transposition table — atomic 3-entry clusters aligned to 64-byte cache lines, with generational aging
- Null move pruning
- Reverse futility pruning (RFP)
- Razoring
- ProbCut
- Late move reductions (LMR)
- Internal Iterative Reductions (IIR) — also fires on stale TT entries
- Singular extensions
- Check extension — extend by 1 ply when in check
- Mate-distance handling that continues past the first forced mate to prefer shorter mates
- Optional Syzygy tablebase probing at the root and in search
- Quiescence search with in-check evasion
- Static Exchange Evaluation (SEE) for capture pruning and bad-capture reductions

### Move ordering
- Staged MovePicker: TT move, tactical moves, then quiet moves
- Lazy quiet generation; quiets are not generated if tactical moves cut off
- MVV/LVA captures with capture history
- Killer moves (2 per ply)
- Countermove heuristic
- Quiet history `[color][from][to]`
- Capture history `[piece][to][captured]`
- Continuation history (1-ply and 2-ply)

### Evaluation
- Tapered material + piece-square tables (PeSTO, public domain)
- Game phase interpolation (midgame ↔ endgame)
- Mobility scoring
- Pawn structure: passed pawns, isolated pawns, doubled pawns
- King safety: attack unit table with piece coordination bonuses; reduced threat when opponent lacks a queen
- Endgame scaling
- Color-aware pawn key for pawn evaluation and correction history

### Time management
- Soft limit (target) / hard limit (maximum)
- Adaptive soft limit based on best-move stability
- `movestogo` aware; move-overhead compensation
- Final UCI legality guard for `bestmove`, ponder moves, and reported PV lines

---

## UCI options

| Option         | Type   | Default | Range     | Description                                   |
|----------------|--------|---------|-----------|-----------------------------------------------|
| `Threads`      | spin   | 1       | 1 – 1024 | Search worker threads; runtime may cap to a hardware-safe count |
| `Hash`         | spin   | 64      | 1 – 33554432 | Transposition table size in MB                |
| `Clear Hash`   | button | —       | —         | Clears the transposition table immediately    |
| `Move Overhead`| spin   | 10      | 0 – 5000  | Extra latency to subtract from clock (ms)     |
| `SyzygyPath`   | string | `<empty>` | —       | Semicolon-separated Syzygy tablebase directories; empty disables probing |
| `SyzygyProbeDepth` | spin | 1    | 1 – 100   | Minimum remaining search depth for non-root WDL probes |
| `Syzygy50MoveRule` | check | true | —        | Respect the 50-move rule in root tablebase move ranking |

---

## Building

Basilisk uses **CMake ≥ 3.24** with [CMake presets](CMakePresets.json) for all common configurations.
Both GCC and Clang are fully supported; use `bench` to measure which produces a faster binary on your CPU — results vary by microarchitecture and compiler version.

### Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.24 |
| Ninja | any |
| GCC ≥ 11 or Clang ≥ 16 | (C++23 required) |

### Presets

| Preset | Build type | Notes |
|---|---|---|
| `release` | Release | `-O3 -march=native` + LTO. **Use for playing/benchmarking.** |
| `release-avx2` | Release | Like `release` + AVX2 code generation |
| `release-pext` | Release | Like `release` + BMI2 PEXT sliding-piece attacks (Haswell+ / Zen 3+) |
| `debug` | Debug | `-O0 -g3` + AddressSanitizer + UBSan |
| `relwithdebinfo` | RelWithDebInfo | `-O2 -g -march=native`, no sanitizers |

For distributable binaries, add `-DPORTABLE_BUILD=ON` when configuring. This keeps the optimization level but omits `-march=native`, so release artifacts are not tied to the build machine's CPU.

GitHub release assets keep the x86_64 choices intentionally small:

| Asset suffix | CPU requirement | Notes |
|---|---|---|
| none | Generic target architecture | Safest default choice |
| `avx2` | x86_64 with AVX2 | Modern Intel/AMD x86_64 CPUs |
| `pext` | x86_64 with BMI2/PEXT | Uses PEXT sliding-piece attack lookup; benchmark against `avx2` on your CPU |

The x86 feature builds check CPU support at startup and print a clear error if the host CPU cannot run that binary.

Release builds are produced for Linux x86_64, Linux aarch64, Windows x86_64,
Windows aarch64, and macOS aarch64. Intel macOS and AVX-512 release assets are
not published.

### Linux / macOS

Install GCC (or Clang), CMake, and Ninja via your package manager, then:

```bash
cmake --preset release
cmake --build --preset release
# Binary: build/release/basilisk
```

### Windows (MSYS2 / MinGW-w64)

**Option 1 — Add MSYS2 to your PATH** (simplest; works from any terminal, CLion, VS Code, etc.):

Open *System Properties → Environment Variables* and prepend one MSYS2 toolchain directory to `Path`:
```
D:\msys64\clang64\bin
```
Then in any terminal:
```powershell
cmake --preset release
cmake --build --preset release
```

**Option 2 — `CMakeUserPresets.json`** (no PATH change; paths stay local and are gitignored):

Copy the example and edit the paths:
```powershell
Copy-Item CMakeUserPresets.json.example CMakeUserPresets.json
# Edit CMakeUserPresets.json: set the correct paths to ninja.exe, gcc.exe, g++.exe
```
Then build using the `local-` prefixed presets:
```powershell
cmake --preset local-release
cmake --build --preset local-release
```

**CLion:** configure a *MinGW* Toolchain under *Settings → Build → Toolchains* pointing to `D:\msys64\mingw64` (for GCC) or `D:\msys64\clang64` (for Clang).
CLion will inject the compiler from that toolchain and use the `release` / `debug` presets from `CMakePresets.json` directly.

> **Note:** Release builds on Windows/MinGW automatically link the C++ runtime statically (`-static`), so the resulting `basilisk.exe` has no dependency on MSYS2 DLLs and runs on any Windows machine. Disable with `-DSTATIC_RUNTIME=OFF` if you explicitly want a dynamic build.

---

## Usage

Basilisk is a standard UCI engine. Load it in any UCI-compatible GUI (Arena, Cutechess, Fritz, Banksia, …) or use it from the command line for analysis:

```
position startpos moves e2e4 e7e5
go movetime 5000
```

To use Syzygy tablebases, set `SyzygyPath` to the directory containing `.rtbw`
and `.rtbz` files before starting a search. With an empty path, tablebase code is
disabled and normal playing strength is unchanged.

### Supported UCI commands

| Command | Notes |
|---------|-------|
| `uci` | Identify engine and list options |
| `debug on\|off` | Toggle debug echoing of received commands |
| `isready` | Synchronise; always answered with `readyok` |
| `setoption name <n> [value <v>]` | Set an option; button types have no value |
| `ucinewgame` | Reset search state and clear TT |
| `position [startpos\|fen <fen>] [moves …]` | Set up board |
| `go [wtime … btime … winc … binc … movestogo … depth … nodes … movetime … infinite … ponder]` | Start search |
| `stop` | Stop search; engine replies with `bestmove` |
| `ponderhit` | Switch from ponder to normal search |
| `bench [depth]` | Run built-in benchmark (default depth 13) using the current `Threads` option |
| `quit` | Exit |

---

## Testing

Basilisk ships a comprehensive test suite covering board correctness, move encoding, the transposition table, evaluation, search, mate-distance regressions, illegal-move hardening, the thread pool, and command queue behavior. Run with:

```bash
ctest --test-dir build/release --output-on-failure
```

A board performance benchmark is also included (run manually — not part of the test suite):

```bash
./build/release/board_performance_test
```

Release CI runs the CTest suite before uploading binaries, then performs UCI smoke tests on every produced CPU-tier executable.

---

## License

GPL-3.0-or-later. See [LICENSE](LICENSE). Syzygy probing uses the vendored
MIT-licensed Fathom library under [external/fathom/LICENSE](external/fathom/LICENSE).
