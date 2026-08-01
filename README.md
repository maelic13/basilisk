# Basilisk

<p align="center">
  <img src="logo/basilisk_detailed.png" alt="Basilisk logo" width="260">
</p>

Basilisk is a strong UCI chess engine written in C++23. It is meant to be used
from a chess GUI or an engine-testing tool.

---

## Highlights

- **Strong modern search** — iterative deepening with principal variation
  search, aspiration windows, null-move pruning, razoring, ProbCut, singular
  extensions, late move reductions and a quiescence search that handles checks
  and prunes losing captures.
- **Multi-threaded** — parallel search across cores with a shared transposition
  table, enabled with the standard `Threads` option, with coordinated clock
  handling and efficient shared node accounting.
- **Tuned evaluation** — a tapered evaluation fitted to millions of positions,
  covering king safety, mobility, threats, pawn structure and passed pawns,
  material imbalance, and endgame knowledge including an exact KPK bitbase and
  KBNK mating technique.
- **Syzygy tablebases** — optional endgame tablebase probing, both in-search and
  at the root, with tablebase moves ranked and tablebase lines shown in the PV.
- **Careful time management** — handles increments, `movestogo` and GUI latency,
  and spends longer on unstable positions than on obvious moves.
- **Pondering** — thinks on the opponent's clock when the GUI enables it.
- **Optimized binaries** — published for Windows, Linux and macOS, on both
  x86-64 and ARM64, and profile-guided-optimized.
- **Built-in benchmark** — a `bench` command for reproducible speed and search
  comparisons.

---

## Download

- [Latest release](https://github.com/maelic13/basilisk/releases/latest)
- [All releases](https://github.com/maelic13/basilisk/releases)

Every release provides ready-to-run executables — no installation needed. Pick
the one matching your operating system and CPU:

| Asset suffix | Use when |
| --- | --- |
| `pext` | Modern Intel, or AMD Zen 3 and newer. Usually the fastest. |
| `avx2` | AVX2-capable CPUs without fast PEXT, such as AMD Zen 1 and Zen 2. |
| *(none)*, `x86_64` | Any 64-bit Intel or AMD CPU. Use this if the others do not start. |
| *(none)*, `aarch64` | ARM64 Linux, Windows on ARM, and Apple Silicon Macs. |

If unsure, try `pext` first and fall back to `avx2`, then the plain `x86_64`
build. All builds play identically; they differ only in speed. A binary started
on a CPU that lacks its required instructions reports a clear error instead of
crashing.

Basilisk is 64-bit only — there is no 32-bit build.

---

## Use With A GUI

1. Download the executable for your system, or build one from source.
2. Add it as a UCI engine in your chess GUI.
3. Set `Hash` to a comfortable amount of memory and `Threads` to the number of
   cores you want to use.
4. Start a game or an analysis session.

Basilisk speaks standard UCI, so any UCI-compatible GUI should work — Arena,
Cutechess, ChessBase/Fritz, Banksia and others.

---

## UCI Options

| Option | Default | Description |
| --- | --- | --- |
| `Hash` | `64` | Transposition table size in MB. More memory helps longer searches, and helps more the more threads you use. |
| `Clear Hash` | — | Empties the transposition table. |
| `Threads` | `1` | Search threads. Set to the number of cores you want to use. Applied immediately. |
| `Ponder` | `false` | Think while the opponent moves. Enabled by the GUI. |
| `Move Overhead` | `10` | Milliseconds reserved for GUI and network delay. Raise it if you lose on time. |
| `SyzygyPath` | empty | Folders holding Syzygy tablebases. Empty disables probing. |
| `SyzygyProbeDepth` | `1` | Minimum depth at which tablebases are probed. |
| `SyzygyProbeLimit` | `7` | Largest tablebase to probe. `0` disables probing. |
| `Syzygy50MoveRule` | `true` | Whether tablebase results respect the fifty-move rule. |

`SyzygyPath` accepts several folders separated by `;` on Windows or `:`
elsewhere. With tablebases enabled, Basilisk ranks the root moves from the
tablebase, reports bounded tablebase scores, extends `info … pv` lines with the
tablebase continuation, and counts resolved positions in `tbhits`. Playing
strength with an empty path is unchanged.

### Supported commands

`uci`, `debug`, `isready`, `setoption`, `ucinewgame`, `position`, `go`, `stop`,
`ponderhit`, `bench` and `quit`.

`go` supports `depth`, `nodes`, `movetime`, `wtime`, `btime`, `winc`, `binc`,
`movestogo`, `mate`, `searchmoves`, `ponder`, `perft` and `infinite`. A bare
`go` searches to depth 7.

---

## Build From Source

Basilisk builds with **CMake ≥ 3.24** and Ninja, using the presets in
[`CMakePresets.json`](CMakePresets.json). You need a C++23 compiler: GCC 14+,
Clang 19+ with libstdc++, or Clang 16+ with libc++.

```bash
cmake --preset release
cmake --build --preset release
# Binary: build/release/basilisk
```

### Presets

| Preset | Notes |
| --- | --- |
| `release` | `-O3 -march=native` + LTO. Use this for playing and benchmarking. |
| `release-avx2` | Like `release`, plus AVX2 code generation. |
| `release-pext` | Like `release`, plus BMI2/PEXT sliding-piece attacks. |
| `relwithdebinfo` | Optimized with debug info, no sanitizers. |
| `debug` | Unoptimized, with AddressSanitizer and UBSan. |

Add `-DPORTABLE_BUILD=ON` when configuring if the binary has to run on other
machines: it keeps the optimization level but drops `-march=native`.

Pick the compiler with `-DCOMP=clang`, `-DCOMP=gcc` or `-DCOMP=llvm` (default
`auto`: Clang on Linux and Windows, AppleClang on macOS). Remove the build
directory before switching compilers. Which one is faster varies by CPU — use
`bench` to compare.

**Windows:** build in an MSYS2 Clang shell, or add `D:\msys64\clang64\bin` to
your `PATH` and build from any terminal. Release builds link the C++ runtime
statically, so the resulting `basilisk.exe` needs no MSYS2 DLLs
(`-DSTATIC_RUNTIME=OFF` disables that).

**macOS:** `brew install cmake ninja` is enough; add `brew install llvm` if you
want to compare against Homebrew LLVM with `-DCOMP=llvm`. Apple Silicon only —
Intel Macs are not supported.

### Profile-guided builds

Published binaries are profile-guided-optimized (PGO), which is worth a few
percent of speed. To build one yourself, configure a preset and build the `pgo`
target:

```bash
cmake --preset release-pext -DCOMP=clang
cmake --build --preset release-pext --target pgo
```

This builds an instrumented engine, trains it on the `bench` suite, and rebuilds
using the collected profile. For Clang builds, CMake selects the
`llvm-profdata` shipped with the configured compiler, so multiple installed LLVM
versions do not get mixed. The finished binary lands in `build/dist`.

### Tests

```bash
ctest --test-dir build/release --output-on-failure
```

---

## Bench

`bench` searches a fixed 40-position suite and reports a node count and speed.
The node count is identical on every platform, which makes it a quick way to
confirm a build is correct; the speed tells you how fast the machine is.

```text
bench
bench 13
```

`bench` is **single-threaded by default and ignores the `Threads` option**, so
the node count stays reproducible no matter how your GUI left the engine
configured. For a multi-threaded speed reading, pass the thread count as the
third argument — `bench [depth] [repeats] [threads]`:

```text
bench 13 1 8
```

Above one thread the node count is no longer identical between runs (the extra
threads do genuinely different work), so use that form for speed only, not to
check that a build is correct.

---

## License

GPL-3.0-or-later. See [LICENSE](LICENSE). Syzygy probing uses the vendored
MIT-licensed Fathom library under [external/fathom/LICENSE](external/fathom/LICENSE).

---

## Acknowledgements

Basilisk is an independent engine, but it benefits from the open chess-engine
community's published ideas, testing practices and protocol conventions. Special
thanks to Stockfish and its team for the inspiration their work provides to
chess engine authors and testers.
