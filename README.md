# Basilisk

A UCI chess engine written in C++23.

**Estimated strength: ~2388 ELO** (calibrated against Stockfish; strong International Master level)

---

## Features

### Search
- Iterative deepening with aspiration windows
- Negamax alpha-beta / Principal Variation Search (PVS)
- Transposition table — 3-entry clusters aligned to 32-byte cache lines, with generational aging
- Null move pruning
- Reverse futility pruning (RFP)
- Razoring
- ProbCut
- Late move reductions (LMR)
- Singular extensions
- Quiescence search with in-check evasion
- Static Exchange Evaluation (SEE) for move ordering and capture pruning

### Move ordering
- TT move
- MVV/LVA captures scored with SEE and capture history
- Killer moves (2 per ply)
- Countermove heuristic
- Quiet history `[color][from][to]`
- Capture history `[piece][to][captured]`
- Continuation history (1-ply and 2-ply)

### Evaluation
- Tapered material + piece-square tables (PeSTO, public domain)
- Game phase interpolation (midgame ↔ endgame)
- Pawn-structure correction history

### Time management
- Soft limit (target) / hard limit (maximum)
- Adaptive soft limit based on best-move stability
- `movestogo` aware; move-overhead compensation

---

## UCI options

| Option         | Type   | Default | Range     | Description                                   |
|----------------|--------|---------|-----------|-----------------------------------------------|
| `Hash`         | spin   | 64      | 1 – 4096  | Transposition table size in MB                |
| `Clear Hash`   | button | —       | —         | Clears the transposition table immediately    |
| `Move Overhead`| spin   | 10      | 0 – 5000  | Extra latency to subtract from clock (ms)     |

---

## Building

Basilisk uses **CMake ≥ 3.24** and supports GCC, Clang, and MSVC. [CMake presets](CMakePresets.json) are provided for all common configurations.

### Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.24 |
| GCC / Clang / MSVC | Any C++23-capable version |
| Ninja (recommended) | any |

### Quick build (Linux / macOS)

```bash
cmake --preset gcc-release   # or clang-release
cmake --build --preset gcc-release
# Binary: build/gcc-release/basilisk
```

### Windows (MSYS2 / MinGW-w64)

```powershell
$env:Path = "D:/msys64/mingw64/bin;D:/msys64/usr/bin;$env:Path"
cmake --preset gcc-release
cmake --build --preset gcc-release
```

### Build configurations

| Preset              | Compiler | Type    | Notes                          |
|---------------------|----------|---------|--------------------------------|
| `gcc-release`       | GCC      | Release | `-O3 -march=native -flto`      |
| `gcc-debug`         | GCC      | Debug   | ASan + UBSan                   |
| `clang-release`     | Clang    | Release | `-O3 -march=native -flto`      |
| `clang-debug`       | Clang    | Debug   | ASan + UBSan                   |
| `clang-release-pext`| Clang    | Release | + BMI2 PEXT (Haswell+)         |
| `msvc-release`      | MSVC     | Release | `/O2 /GL /LTCG`                |

> **PEXT note:** Enable `USE_PEXT=ON` (or use the `clang-release-pext` preset) on Intel Haswell+ / AMD Zen 3+ for faster sliding-piece attack generation via the BMI2 `pext` instruction.

### Static binary (for external tools)

To build a self-contained executable without MinGW DLL dependencies:

```powershell
cmake -B build/gcc-static -G Ninja -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_CXX_COMPILER=g++ `
      -DCMAKE_EXE_LINKER_FLAGS="-static"
cmake --build build/gcc-static
```

---

## Usage

Basilisk is a standard UCI engine. Load it in any UCI-compatible GUI (Arena, Cutechess, Fritz, Banksia, …) or use it from the command line for analysis:

```
position startpos moves e2e4 e7e5
go movetime 5000
```

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
| `quit` | Exit |

---

## Author

Miloslav Macurek
