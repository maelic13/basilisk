# Universal x86-64 binary — design packet (A.15.1)

- State / class: `RESEARCH`, `R2`; closes A.15.1.
- Date: 2026-10-28, at `master` `75d7fce` plus A.14.1.
- Decision needed from the maintainer before A.15.2 reads: the decision rule
  in §9 as the leaf proposed it, or the amendment recommended there.
- Inputs: `analysis/code_audit_2026-10-27.md` §4; Rarog's record
  `docs/reference/rarog/analysis/universal_binary_2026-09.md` (a prior, not
  evidence for Basilisk); the Rarog audit items D-24, D-25, D-26, D-72 and
  A-36; Stockfish read at the pinned donor `edb0d9db` (tag `sf_19`) with
  `git show`, never from memory.

## 1. The question

Basilisk ships nine assets, three of them x86 tiers per desktop OS
(`portable`, `avx2`, `pext`). A user must pick one. One who picks too low
loses the tier ladder's speed. One who picks `pext` on an AMD Excavator or
Zen 1/2 gets microcoded `pext` on every slider lookup, the slowest engine of
the three. The universal binary removes the choice: one x86-64 executable
per OS that selects its tier once, at start-up.

This packet fixes the mechanism, Basilisk's form of it, the checks that make
its two silent failure modes loud, its costs and the rejected alternatives.
It also states the decision rule for building it and recommends an amendment.
It changes no engine source.

## 2. How Stockfish 19 does it (read at `edb0d9db`)

Files: `src/Makefile` (Section 6, targets `universal-object-pgo`,
`universal-object-nopgo`), `src/universal/entry_x86.cpp`,
`src/universal/rewrite_asm_sections.awk`, `src/main.cpp`,
`scripts/check_universal.sh`, `.github/workflows/universal_compilation.yml`.

1. **One full engine per ISA level.** Eight x86 levels, `x86-64` through
   `x86-64-avx512icl`. Each is built in its own directory as a complete
   PGO+LTO executable. The instrumented binary runs `bench` under
   `RUN_PREFIX` (Intel SDE `-future` for levels the runner cannot execute).
2. **Isolation by namespace rename.** The engine lives in `namespace
   Stockfish`; each level compiles with `-DStockfish=Stockfish_<arch>`, so
   every engine symbol differs per level. `main` becomes
   `Stockfish_<arch>::main`; a `--defsym`, a Mach-O alias, or a main shim
   (`UNIVERSAL_NEEDS_MAIN_SHIM`, clang on Windows) keeps the per-level link
   working.
3. **The LTO output is extracted as one relocatable object per level.** GCC
   uses `-save-temps` (`*.ltrans0.ltrans.o`). Clang on ELF uses
   `-Wl,--save-temps` (`*.lto.o`). Clang on Windows uses
   `-Wl,--lto-emit-asm`: the awk script folds every `.ctors` directive into
   one `<arch>_init` section with `__start_`/`__stop_` symbols, and the `.s`
   is assembled. `objcopy -R .group` drops the COMDAT groups LTO leaves
   around internalised template instantiations, and `-N main` deletes the
   shim.
4. **Inert initialisers.** Off the awk path, `.init_array`/`.ctors` are
   renamed to `<arch>_init` and flagged `alloc,data`, so the loader runs
   none of them. The chosen level's `entry_<arch>` walks its own array
   before calling its `main`. Only one level's static constructors ever run.
5. **Dispatch.** `entry_x86.cpp` is compiled at `-O2` with no ISA flags. It
   reads 17 `__builtin_cpu_supports` bits and makes one model check: AMD
   `bdver4`, `znver1` or `znver2` (slow `pdep`/`pext`) goes to `avx2` and
   not `bmi2`. There is no override to force a level.
6. **Final link.** The entry object, the network object (`#embed`, once) and
   the eight level objects, with the baseline level first. Windows passes
   `-Wl,--allow-multiple-definition -static` and the Makefile states the
   reason: "Windows --allow-multiple-definition uses the first copy of
   inline duplicates". Linux links `-static-libstdc++ -static-libgcc`.
7. **CI.** `check_universal.sh` runs the binary under SDE for ten Intel CPU
   models: `p4p` and `nhm` on Linux only, plus `snb`, `ivb`, `hsw`, `skl`,
   `skx`, `clx`, `icl` and `adl`. It asserts that the `compiler` command
   names the expected level and that `bench` gives the expected node count,
   with a 150 MB size ceiling. SDE emulates Intel parts only, so the Zen
   rule has no emulated test.
8. **Cost.** The hot path is unchanged, because the dispatch happens once,
   before `main`. PR 6740's reviewer measured 8.365M nps universal against
   8.363M for the `avx512icl` asset. The asset carries eight engines.

## 3. Basilisk today (measured 2026-10-28)

- **Tiers.** `CMakeLists.txt:829-857`.
  - `portable` passes no ISA flags: the x86-64 baseline, SSE2, no `popcnt`.
  - `avx2` adds `-mavx2 -msse4.1 -mpopcnt` and has no intrinsics (flags and
    autovectorisation only).
  - `pext` adds `-mbmi -mbmi2 -mlzcnt`. `USE_PEXT` reaches the source only in
    `src/attacks.cpp`.
  - There is no `popcnt`-only tier, so Nehalem, Sandy Bridge and Ivy Bridge
    get the software popcount of the portable asset.
- **Launcher.** `src/main.cpp` is the `basilisk_launcher` object, compiled
  without tier flags and kept out of LTO (`CMakeLists.txt:860-867`). It
  refuses an incompatible tier on stderr only (D-25, A-36) and then calls
  `run_engine`. `tools/diag/verify_isa.py --launcher-object` proves the guard
  survived optimisation (D-26).
- **Namespaces.** Engine code is in the global namespace, except `Zobrist`,
  `Syzygy` and file-local anonymous namespaces.
- **Dynamic initialisers.** In the shipped executable's objects,
  `llvm-nm` finds `_GLOBAL__sub_I_*` in exactly two:
  - `syzygy.cpp`, for `std::mutex g_init_mutex` and `std::string g_path`;
  - Fathom's `tbprobe.c` (compiled as C++), for `static std::mutex tbMutex`
    and its atomic-bearing tables.

  Fathom also exports C-linkage-style globals (`TB_LARGEST`,
  `TB_MaxCardinality`) in the global namespace.
- **Size** (`release-pext`, Windows, non-PGO):
  - 2,199,040 bytes in all, of which `.text` is 1,273,958 and `.rdata`
    419,300.
  - Classifying `.text` by symbol gives about 415 KB engine, 84 KB Fathom
    and 745 KB C++ runtime and library templates. The split is approximate:
    `std::` instantiations over engine types are counted as runtime.
- **Release.** `release.yml` has nine cells. Every x86 cell builds with
  `--target pgo` (an instrumented child build, `bench`, then the use build),
  checks the ISA contract on the asset and the launcher object, and requires
  the declared `bench 13`.
  - The `pext` cells already run their asset on the GitHub x86 runners, so
    those runners execute all three tiers natively. PGO needs no emulator.
  - Linux builds on `ubuntu-24.04` with Clang 19. Windows builds on
    `windows-2025` with the pinned MSYS2 Clang and lld. macOS is arm64 only.
    MSVC is a local preset (`msvc-release-pext`), not a release toolchain.
- **Fingerprint.** `bench 13` is identical on every tier and platform (the
  release check enforces it).

## 4. Basilisk's form

The Stockfish mechanism, with two places where Basilisk can do better:
shared C code is built once rather than resolved by link order, and tier
objects carry no initialisers rather than renamed ones.

### 4.1 The namespace wrap (prerequisite, behaviour-neutral)

- Every engine translation unit opens `namespace BASILISK_NS {`.
  `BASILISK_NS` defaults to `basilisk`; a tier build defines it as
  `basilisk_portable`, `basilisk_avx2` or `basilisk_pext`.
- `Zobrist` and `Syzygy` become nested namespaces, and `run_engine` becomes
  `BASILISK_NS::run_engine`.
- Outside the wrap: `src/main.cpp` (the dispatcher), Fathom and the C++
  runtime.
- It is a mechanical change across about 57 files, with the exact
  fingerprint as its proof. It also gives `verify_isa.py` what it needs to
  attribute every function to a tier by name (§4.6).
- **Done once, before any other step.** It is the only step that touches
  every engine file, and it is useful on its own, because `perf`/ETW
  profiles and crash reports name the tier.

### 4.2 Shared code outside the tiers

- **Fathom is compiled once at baseline and linked once**, outside every
  tier. Each tier object leaves `tb_*` undefined.
  - This is Rarog's validated fix (its packet §3). Flipping the one copy's
    defines moved every tier together, and no link order is involved.
  - It also gives the process one `tb_init` state instead of three.
  - Fathom is not on the hot path: probes are I/O- and cache-bound, a small
    share of nodes, and they already pay a non-inlined call.
- **The dispatcher stays the baseline launcher.** It is outside LTO and
  carries the guard `verify_isa.py` already checks.
- **The C++ runtime is linked once**, from the static archives the release
  already uses.

### 4.3 No initialiser in a tier object

- **Requirement.** A tier object has no dynamic initialiser: no
  `_GLOBAL__sub_I_*`, and empty `.init_array`/`.ctors`.
- **What that takes.**
  - Fathom leaves the tiers (§4.2).
  - `syzygy.cpp`'s two globals become constant-initialised and trivially
    destructible, or function-local statics. Either is behaviour-neutral
    and done with the wrap.
- **Why that is enough.** Basilisk's tables (attacks, magics, Zobrist, the
  piece-square tables, the KPK bitbase) are already filled by explicit init
  calls under `run_engine` or a `once_flag`. So only the selected tier's
  tables are ever built, and the others stay untouched BSS (no file size,
  no committed pages).
- **The check.** `llvm-nm` on each tier object before the link. A new global
  with a constructor fails the release, not a user's CPU.
- **What this saves.** None of Stockfish's section rewriting: no awk, no
  `objcopy --rename-section`, no `__start_`/`__stop_` walk.
- **Fallback.** If a future global cannot avoid a constructor, Stockfish's
  rename is the documented fallback (§2.3–2.4).
- **Linux check owed.** The release links `libstdc++` from GCC 13 or 14 on
  `ubuntu-24.04`, where `<iostream>`'s per-TU `ios_base::Init` object has
  moved into the library. The `llvm-nm` scan must still run on a Linux tier
  object once, since only the Windows objects were scanned here.

### 4.4 Building a tier object

- **Per tier.** Configure with `-DBASILISK_NS=basilisk_<tier>` and the
  tier's flags, and build a complete PGO executable:
  1. Instrumented build, with the same namespace.
  2. `bench` on the runner, native for all three tiers (§3).
  3. Use build, with LTO.
  - The executable links the tier's engine libraries and a one-line main
    shim calling `basilisk_<tier>::run_engine`.
- **Extract the LTO output.**
  - Linux: ld.lld `--save-temps`, which writes `.lto.o`.
  - Windows (MinGW lld): `/lldsavetemps` or `--lto-emit-asm`. If the latter,
    assemble the `.s`; no section rewriting is needed under §4.3.
  - Then delete the shim's `main` (`llvm-objcopy -N main`) and drop leftover
    COMDAT groups (`-R .group`), as the donor does.
- **Why a full executable LTO per tier.** LTO internalises everything the
  tier does not export, so a tier object exports `run_engine` and nothing
  else, and the shared templates question (§5.1) does not arise. This is
  the donor's measured form: no hot-path loss.
- **Rejected: one LTO link over all tiers' bitcode** (the audit's
  suggestion).
  - `std::` instantiations over non-engine types (`std::string`,
    `std::vector<int>`, `std::sort<int*>`) are `linkonce_odr` in every tier,
    with the same mangled name. The IR linker keeps one prevailing copy,
    with its tier's target attributes, for every caller.
  - If a `pext` copy prevails, a baseline CPU executes `pext` inside the
    portable tier. That is the first-wins trap moved into the LTO linker,
    where link order is not even a lever.
  - It is also no simpler once §4.3 removes the section rewriting.
- **Alternative, if save-temps extraction proves fragile:** an LTO
  relocatable link (`ld.lld -r`). It keeps COMDAT templates global, so the
  final link deduplicates them first-wins and the baseline object must come
  first, which is the donor's Windows rule. Not preferred, because it
  re-admits that rule.

### 4.5 The final link and the dispatch

- **Link inputs:** the dispatcher, Fathom, the three tier objects (portable
  first) and the static runtime.
  - First try without `--allow-multiple-definition`. Under §4.4 the tier
    objects should export nothing that collides.
  - If the link needs the flag anyway, that is a finding: record what
    collided before adding it.
- **Dispatch rule**, in `src/main.cpp` beside the existing `CpuFeatures`:
  - `pext` if `supports_pext_tier(f)` and the CPU's `pext` is not slow;
  - else `avx2` if `supports_avx2_tier(f)`;
  - else `portable`.
- **Slow `pext`.** "Not slow" means not AMD family 15h (Excavator) or 17h
  (Zen, Zen+, Zen 2), the donor's one model rule; family 19h (Zen 3) and
  later is fast. It is read from CPUID leaf 1 on both compilers, since the
  MSVC path has no `__builtin_cpu_is`.
- **Override.** `BASILISK_ISA=portable|avx2|pext` forces a tier.
  - An unsupported forced tier is refused, on stdout as an `info string` and
    on stderr, with exit status 1.
  - The donor has no override. Basilisk needs one for the forced-tier bench
    identity test (§4.7) and for measurement on one machine.
- **Banner.** The start-up line names the tier
  (`Basilisk <version> by <author> (pext)`). `id name` does not change,
  because rating lists key on it.

### 4.6 The ISA contract (`verify_isa.py`)

- **Before the link, per tier object:** each object meets its own tier's
  mnemonic set.
- **After the link, on the image,** attributed by the linker map
  (`-Wl,-Map`), which assigns every `.text` range to its input object, so
  attribution does not depend on symbols surviving in the asset:
  - every range from a tier object meets that tier's set;
  - every other range (dispatcher, Fathom, runtime) meets the portable set;
  - nothing anywhere is above `pext`.
- **The portable set must first forbid** SSE4.1, AVX1 and BMI1 (`andn`,
  `tzcnt`) as well as `popcnt`. That is A.15.3's item, and the fat binary
  depends on it, because shared code is then truly baseline.
- **The launcher check stays as it is** (D-26).

### 4.7 Tests

- **`test_cpu_features`:** a truth table for the dispatch rule over feature
  sets and AMD/Intel families, including the slow-`pext` rule (SDE cannot
  emulate Zen).
- **Forced-tier identity**, on each x86 runner: `BASILISK_ISA=` each tier
  gives the declared `bench 13`, and the banner names the forced tier.
- **SDE selection matrix** (Linux job), CPU model and the tier it must get:
  - `p4p`, `nhm`, `snb`, `ivb`: `portable` (Basilisk has no
    `sse41-popcnt` tier);
  - `hsw`, `skl`, `icl`, `adl`: `pext`.

  Each model also gives the declared bench.
- **PGO liveness per tier.** Build with `-Wprofile-instr-unprofiled`
  enabled for the engine targets (the release suppresses it today) and
  require no unprofiled engine function.
  - Do the absurd-profile check once: a tier given another tier's profile
    must lose NPS measurably.
  - Reason: profiles match by mangled name, and the namespace is in the name
    (D-72). A tier trained under the wrong `BASILISK_NS` silently gets no
    profile.
- **Hot-path cost check, on the 5950X:** a pooled NPS read of the universal
  asset against the `pext` asset, null pair first. It must be inside the
  null's interval. The donor's zero cost is a prior to be measured here,
  not assumed.

### 4.8 Release shape

- **Per OS (Linux, Windows):** three tier jobs upload their objects plus
  the toolchain fingerprint; a link job builds, verifies and benches the
  asset. Alternatively one job builds all three, with the 25-minute timeout
  raised.
- **Assets:** nine become five — `linux-x86_64`, `linux-aarch64`,
  `windows-x86_64`, `windows-aarch64`, `macos-aarch64` — named without a
  tier suffix.
- **README and CHANGELOG** say which tier a CPU gets and how to force one.
- **Local builds:** `build_test.ps1` and every measurement keep building
  single-tier binaries. The universal binary is a release artifact, and its
  NPS is the `pext` tier's (§4.7).
- **Out of scope:** the TUNE, DIAGNOSTIC, ABLATION and texel flavours stay
  single-tier.

## 5. Traps, each with its check

1. **First-wins symbols** (D-72). The fat binary's linkers resolve
   duplicates silently. The checks are §4.2 (Fathom once), §4.4 (no shared
   templates exported), §4.5 (no `--allow-multiple-definition` without a
   recorded reason) and §4.6 (map-attributed image check).
2. **Early initialisers.** A tier constructor running before dispatch can
   execute tier instructions on any CPU. Check: §4.3's `llvm-nm` scan.
3. **Profiles matched by mangled name.** A tier trained under another
   namespace has no profile. Check: §4.7's liveness test.
4. **`-march=native`.** It is incoherent with a universal build. The tier
   builds require `PORTABLE_BUILD=ON`, as the release already sets, and
   configure fails otherwise.
5. **A stale tier object.** All three objects must come from one revision
   and toolchain. The link job compares their recorded revision and clang
   version before linking.
6. **The guard folding.** The dispatcher must stay a baseline object outside
   LTO (D-26). `verify_isa.py --launcher-object` keeps checking it.

## 6. Costs

- **Build:** three PGO builds per x86 cell instead of one; the release grows
  by four jobs or by about 2.5 times per x86 cell's wall time.
- **Size:** the runtime, Fathom and `.rdata` that is not engine are shared.
  The engine's `.text`, its tables and its engine-typed template
  instantiations are tripled.
  - From §3's split, the estimate is about 3.5–5 MB per x86 asset against
    2.2 MB today. The audit's 5–6 MB assumed everything triples.
  - The prototype measures it. The donor's ceiling is 150 MB.
- **Hot path:** zero by design (one dispatch), checked by §4.7's pooled read.
- **Maintenance:**
  - one namespace macro in every engine file;
  - a release pipeline with three moving parts per OS;
  - two silent failure modes, made loud by §5's checks but permanent.
- **Effort, estimated:**
  - the wrap and `syzygy.cpp`: one `I1` session;
  - the build, link and verify pipeline: one or two `I2` sessions;
  - the release workflow and SDE matrix: one session;
  - the measurement reads.

## 7. Rejected alternatives

- **Function multiversioning** (`target`/`target_clones`).
  - It needs ELF IFUNC, absent on COFF, so it is unavailable on Windows with
    Clang or GCC.
  - It makes every attack lookup an indirect call that LTO cannot inline,
    forfeiting the whole-engine `popcnt`/`tzcnt`/BMI code generation the
    tier flags give. Rarog measured that loss at 4.59% (RAR-P20, a prior).
- **macOS universal2.** It reverses `CMakeLists.txt:69`'s Intel refusal and
  needs an x86 slice and `lipo` on a macOS job. macOS 26 is the last Intel
  release. No value.
- **A launcher process with sibling executables.** It is multi-file, adds a
  process tree under GUIs (Windows job objects, signals, ponder `stop`
  latency), and nobody ships it.
- **Embedding the tiers and caching one to disk on first run.** It writes at
  run time, and is open to sandboxes, AV and cache poisoning.
- **A fourth `popcnt` tier now** (SSE4.1 + POPCNT, for Nehalem–Ivy Bridge).
  In the fat binary it costs one more object, but it is a ladder decision.
  A.15.2 can read it as a fourth build if the maintainer wants the number.
  Not part of this design.

## 8. What it buys

- **Nothing** for a user who picks the right asset today.
- **The ladder**, for a user on a lower tier than their CPU allows. A.15.2
  measures it. Rarog's prior: `pext` over `base` +6.65% [+6.48, +6.89],
  about 13 Elo at PROCESS's 2 Elo per 1%.
- **The `pext` trap on AMD Excavator and Zen 1/2**, which no ladder read on
  the 5950X (Zen 3) can price. A Zen 2 user picking the asset named for
  the fastest tier gets the slowest engine. The advisory (A.15.3) tells a
  user who reads stdout. The universal binary fixes it for an unattended
  tester, who does not read stdout.
- **Five assets instead of nine**, and README text that names no tiers.
- **The foundation Phase F's SIMD tiers will need anyway.** NNUE inference
  is why Stockfish has eight levels. Basilisk's HCE has no intrinsics, so
  its ladder is flags alone until then.

## 9. Decision rule, prediction and recommendation

**The rule as the leaf proposed it**, frozen here before A.15.2 reads unless
the maintainer amends it first: pulled into Phase A if A.15.2's direct
`pext`-over-`portable` read is at least +3% pooled NPS; otherwise G.2 builds
it when Phase F's SIMD tiers arrive.

**Prediction, frozen with it** (agent's prior, 2026-10-28; A.15.2's
registration copies it):

| Hop, read directly | Prediction | 80% interval |
|---|---|---|
| `avx2` over `portable` | +5% | [+3%, +9%] |
| `pext` over `avx2` | +2% | [0%, +4%] |
| `pext` over `portable` | +7% | [+4%, +12%] |

The basis: `portable` has no hardware `popcnt`, and the evaluation counts
mobility, king-zone and threat sets by popcount on every full evaluation.
Rarog's base→avx2 hop (+4.59%) came from the same missing instruction. The
`pext` hop replaces magic multiplies in `attacks.cpp` alone. Confidence that
`pext` over `portable` clears +3%: about 90%.

**Recommendation: amend the rule; it does not discriminate.** With a 90%
prior that the threshold is cleared, the rule nearly always schedules the
build in Phase A, before the search programme, whatever A.15.2 reads. And the
ladder measures the wrong quantity: the cost to a user who picked badly, not
how many users do. What would justify building it early is evidence of harm
from asset choice, which a ladder on this machine cannot supply.

Proposed rule:

- **Pull the build forward only on evidence of harm from asset choice.**
  Either:
  - a rating list or a recurring tester runs Basilisk on a lower tier than
    its hardware supports, or on `pext` on a slow-`pext` CPU; or
  - A.15.3's advisory is shown not to reach such a tester.
- **Otherwise G.2 builds it,** with Phase F's SIMD tiers or before E.3's
  release, whichever comes first.
- **A.15.2 still runs.** It records the ladder (D-24), sizes the advisory's
  wording and the README's asset guidance, and becomes the expected gain the
  build's own gate is checked against.

**Main counter-argument.** Nine assets are a real usability cost now. The
Zen 2 `pext` trap can cost a tester tens of percent of speed silently. The
work is bounded (§6) and the donor has shown it is free on the hot path.
If the maintainer weighs the Zen 2 trap above the search programme's
schedule, the leaf's rule (or simply "build it now") is defensible.

The refusal is falsifiable: a tester's log or a rating-list entry on the
wrong tier is the evidence that reverses it, and the cheapest way to get it
is to read the CCRL/CEGT build notes once Basilisk is listed.

## 10. Interactions with other leaves

- **A.15.2.** Its registration copies §9's rule (as finally chosen) and the
  prediction. The reads: four builds per tier, interleaved, a null pair
  first, each hop direct (`tools/nps_pool.ps1`, `tools/nps_read.ps1`).
- **A.15.3.** Its advisory must cover the slow-`pext` case as well as a
  lower tier on a capable CPU. That is A-36's half, which its text leaves
  implicit: on AMD family 15h/17h running the `pext` asset, the advice is
  the `avx2` asset. Its portable-set extension is a prerequisite of §4.6.
  Its truth table is the seed of §4.7's.
- **G.2.** Owns the build under the recommended rule. Its text also says the
  Linux release compiler is Clang 22, but `release.yml` installs Clang 19 on
  `ubuntu-24.04` (§3). That prose is wrong and should be corrected when G.2
  is next edited.
- **D.3.2.** The refusal and advisory messages on stdout are protocol output
  before `uci`. Its lifecycle tests should cover them.
- **Phase F.** NNUE inference tiers are the strongest reason to build this.
  At that point the tier list grows (AVX-512, VNNI) and §4 applies
  unchanged.

## 11. Sources

- Stockfish at `edb0d9db`, read with `git -C D:/code/stockfish show
  edb0d9db:<path>`: `src/Makefile` (Section 6 and the `universal-object-*`
  targets), `src/universal/entry_x86.cpp`,
  `src/universal/rewrite_asm_sections.awk`, `src/main.cpp`,
  `scripts/check_universal.sh`, `.github/ci/universal_matrix.json`,
  `.github/workflows/universal_compilation.yml`.
- PR https://github.com/official-stockfish/Stockfish/pull/6740 (x86
  universal, the reviewer's NPS reads), as summarised in the audit §4.
- Rarog `docs/reference/rarog/analysis/universal_binary_2026-09.md`
  (snapshot `015bccae`), §3 for the Fathom-once validation and §7 for the
  traps.
- Basilisk measurements:
  - `llvm-nm` over `build/release-pext` objects for the initialisers;
  - `llvm-size -A` on `build/release-pext/basilisk.exe`;
  - `llvm-nm -C` address deltas for the `.text` split;
  - `CMakeLists.txt`, `src/main.cpp`, `src/cpu_features.h`,
    `.github/workflows/release.yml`, `tools/diag/verify_isa.py`.
- Clang attribute documentation on `target`/`target_clones` and IFUNC:
  https://clang.llvm.org/docs/AttributeReference.html.
