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
  at the root, with tablebase moves ranked and the PV continued to mate.
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
the one for your computer:

| Your computer | Download |
| --- | --- |
| Windows, Intel or AMD processor | `basilisk-{version}-windows-x86_64.exe` |
| Linux, Intel or AMD processor | `basilisk-{version}-linux-x86_64` |
| Windows on ARM | `basilisk-{version}-windows-aarch64.exe` |
| Linux on ARM | `basilisk-{version}-linux-aarch64` |
| Mac with Apple silicon | `basilisk-{version}-macos-aarch64` |

**One download for every Intel or AMD processor.** It contains builds for
several processor generations and, when it starts, runs the fastest one your
processor supports, so there is nothing to choose. Every build plays exactly
the same moves; only the speed differs.

Upgrading from 1.10 or earlier? Those releases had separate `-avx2` and `-pext`
downloads; the plain `x86_64` download now replaces all three.

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
| `MultiPV` | `1` | Number of best lines shown per depth, best first, for analysis. `bestmove` is always the first line. |
| `SyzygyPath` | empty | Folders holding Syzygy tablebases. Empty disables probing. |
| `SyzygyProbeDepth` | `1` | Minimum depth at which tablebases are probed. |
| `SyzygyProbeLimit` | `7` | Largest tablebase to probe. `0` disables probing. |
| `Syzygy50MoveRule` | `true` | Whether tablebase results respect the fifty-move rule. |

`SyzygyPath` accepts several folders separated by `;` on Windows or `:`
elsewhere. With tablebases enabled, Basilisk ranks the root moves from the
tablebase, probes the tables during the search, and counts resolved positions
in `tbhits`. A tablebase win is reported as `cp 20000` less the plies needed to
reach it, so a quicker win scores higher; a win the fifty-move rule will spoil
shows a small score. When the score is a tablebase win or loss, the `info … pv` line is
continued to mate the way Stockfish does it. The searched line is kept for as
long as it holds the tablebase result, then extended with minimal-DTZ moves,
and the ponder move comes from it. In analysis (no clock) every line is
extended. In games only the final line is, within half of `Move Overhead`,
and only when at least ten times `Move Overhead` is left on the clock. If the
time runs out, an `info string` suggests raising `Move Overhead`.
Playing strength with an empty path is unchanged.

### Supported commands

`uci`, `debug`, `isready`, `setoption`, `ucinewgame`, `position`, `go`, `stop`,
`ponderhit`, `bench`, `help` and `quit`.

Any command can also be given on the command line: `basilisk bench 13` runs the
bench and exits. An unknown command there exits with status 2.

`go` supports `depth`, `nodes`, `movetime`, `wtime`, `btime`, `winc`, `binc`,
`movestogo`, `mate`, `searchmoves`, `ponder`, `perft` and `infinite`. A bare
`go` searches to depth 7.

A `position` command that Basilisk cannot accept is fatal. That covers a
malformed command or FEN, an illegal position, and an illegal move in the move
list. Basilisk prints the reason and an `info string CRITICAL ERROR` line, then
exits with status 1, rather than play on from a position the GUI did not
mean.

---

## Build From Source

You do not need to build Basilisk to play: the downloads are already optimized.
Build it to develop or test the engine, or for a system without a download.

| You need | |
| --- | --- |
| Build tools | CMake 3.24 or newer, and Ninja |
| Compiler | Clang 19+ with libstdc++, Clang 16+ with libc++, or GCC 14+. Clang builds the fastest engine (GCC about 3% slower); MSVC is not supported. |

### The fastest engine for your computer

```bash
cmake --preset release
cmake --build --preset release --target pgo
```

This checks the processor of the computer that builds it, builds the one
of the download's four builds that runs fastest there (`pext` on Intel
Haswell and newer and AMD Zen 3 and newer), and optimizes it with a profile
of the `bench` suite (PGO). It is the same code the download runs on your
processor, in a single smaller file. It lands in `build/dist`, named for its
version, system and build, for example
`basilisk-v1.11.0-windows-x86_64-pext-pgo.exe`; it may not start on an older
processor.

### Other builds

A quick build without the profile step, for development and tests:

```bash
cmake --preset release
cmake --build --preset release
```

One file for any Intel or AMD computer, as released (see below):

```bash
cmake --preset universal
cmake --build --preset universal --target universal
```

A debug build, slow, with AddressSanitizer and UBSan:

```bash
cmake --preset debug
cmake --build --preset debug
```

Every build is copied to `build/dist` under a name that says what it is: its
version, system, the tier built for this computer, and any build type other
than a release. On a Ryzen 9 5950X the three builds above give:

| Build | In `build/dist` |
| --- | --- |
| Quick | `basilisk-v1.11.0-windows-x86_64-pext.exe` |
| Profile-guided (`--target pgo`) | `basilisk-v1.11.0-windows-x86_64-pext-pgo.exe` |
| Universal | `basilisk-v1.11.0-windows-x86_64-universal-pgo.exe` |
| Debug | `basilisk-v1.11.0-windows-x86_64-pext-debug.exe` |

Each build's own folder, `build/<preset>`, holds the same binary as `basilisk`,
and the tests.

### Options

Add them to the first command, for example `cmake --preset release -DCOMP=gcc`.

| Option | Effect |
| --- | --- |
| `-DCOMP=clang`, `gcc` or `llvm` | Chooses the compiler. The default is Clang on Linux and Windows, AppleClang on macOS. Remove the build directory before switching. |
| `-DNATIVE=ON` | Builds with `-march=native` instead. It measured 1% slower on a Ryzen 9 5950X; it may be faster on another processor, so compare with `bench`. |

### Windows

Install [MSYS2](https://www.msys2.org), then its Clang toolchain from the
MSYS2 shell:

```bash
pacman -S mingw-w64-clang-x86_64-clang mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-ninja
```

Build in the MSYS2 CLANG64 shell. The engine links its runtime statically, so
`basilisk.exe` runs anywhere without MSYS2.

### macOS

```bash
brew install cmake ninja
```

Add `brew install llvm` to try `-DCOMP=llvm`. Apple silicon only.

### The universal binary

The `universal` build is the release's Intel/AMD download: four
profile-guided builds of the engine in one file, and a small start-up check
that runs the fastest one the processor supports.

| Build | Chosen on | Speed |
| --- | --- | --- |
| `pext` | Intel Haswell (2013) and newer, AMD Zen 3 (2020) and newer | Fastest |
| `avx2` | AMD Zen 1 and Zen 2, and other AVX2 processors | About 3% slower |
| `sse41-popcnt` | Intel Nehalem to Ivy Bridge (2008–2012), AMD Bulldozer to Steamroller | About 3% slower |
| `base` | Any other 64-bit Intel or AMD processor | About 8% slower |

The first line Basilisk prints names the build it chose. To run one build on
purpose, for testing, set the environment variable `BASILISK_ISA` to `base`,
`sse41-popcnt`, `avx2` or `pext`; a build the processor cannot run is refused.
How the binary is built and checked:
[`docs/release_tiers.md`](docs/release_tiers.md).

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
