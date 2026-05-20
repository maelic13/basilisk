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

Basilisk uses **CMake ≥ 3.24** with [CMake presets](CMakePresets.json) for all common configurations.
The recommended compiler is **GCC** (marginally faster NPS than Clang due to better auto-vectorisation of bitwise/integer loops), though Clang works equally well.

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
| `release-pext` | Release | Like `release` + BMI2 PEXT (Haswell+ / Zen 3+) |
| `debug` | Debug | `-O0 -g3` + AddressSanitizer + UBSan |
| `relwithdebinfo` | RelWithDebInfo | `-O2 -g -march=native`, no sanitizers |

### Linux / macOS

Install GCC (or Clang), CMake, and Ninja via your package manager, then:

```bash
cmake --preset release
cmake --build --preset release
# Binary: build/release/basilisk
```

### Windows (MSYS2 / MinGW-w64)

**Option 1 — Add MSYS2 to your PATH** (simplest; works from any terminal, CLion, VS Code, etc.):

Open *System Properties → Environment Variables* and prepend to `Path`:
```
D:\msys64\mingw64\bin
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

**CLion:** configure a *MinGW* Toolchain under *Settings → Build → Toolchains* pointing to `D:\msys64\mingw64`.
CLion will inject the compiler from that toolchain and use the `release` / `debug` presets from `CMakePresets.json` directly.

### Static binary (for external tools)

A dynamically linked binary may fail when launched from outside the MSYS2 environment (missing DLLs).
Build a fully self-contained executable with:

```powershell
cmake --preset release -DCMAKE_EXE_LINKER_FLAGS="-static"
cmake --build --preset release
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
