# Universal x86-64 binary — design packet (A.15.1)

- State / class: `RESEARCH`, `R2`; closes A.15.1.
- Date: 2026-10-28, at `master` `75d7fce` plus A.14.1.
- Decided 2026-10-28: the maintainer chose to build it now (§9's
  counter-argument). Amendment 1 (§12) records the form that was built,
  where it departs from §4, and the leaves it was built under (A.15.2 to
  A.15.5, renumbered in execution order).
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

## 12. Amendment 1 (2026-10-28): the form that was built

Built under A.15.2 on branch `a15-universal-binary`, after probes of the
LTO objects on both platforms. Five departures from §4, each with its
evidence.

1. **Every tier symbol renamed, no namespace wrap.** A tier's LTO output
   internalises what nothing outside it references, but not everything:
   - on Windows, about 48 libc++ template functions (`std::string`,
     `std::stringstream`, `to_chars`) and their vtables stay global COMDATs,
     because the static libc++ references them too;
   - under ThinLTO, every function one module calls in another is promoted
     to a global (407 in the Windows pext tier, 271 on Linux).

   Each would be resolved first-wins across the tiers, silently: the §5.1
   trap. `cmake/universal-tiers.cmake` therefore renames every symbol each
   tier defines to `__basilisk_<tier>_<name>`, and `run_engine` to
   `basilisk_<tier>::run_engine`. Only type identities (`_ZTI`, `_ZTS`),
   which exception matching compares, and section-named data (`.refptr`
   pointers) stay shared; both are data whose copies are identical. The
   script fails if anything else stays global.
   - This makes §4.1's 57-file wrap unnecessary. It also removes D-72's
     profile trap: each tier trains and links under unchanged names in its
     own build directory.
   - Every function in the image then names its tier, so `verify_isa.py
     --tier universal` attributes code by symbol, not by the linker map of
     §4.6. That works the same with lld and with GNU ld.
2. **ThinLTO kept.** §4.4 assumed one full-LTO module. The rename applies
   across a tier's ThinLTO objects as well (one map per tier, applied to
   all 23), so the tiers are the same ThinLTO code every measurement builds.
3. **The tier objects come from the ordinary PGO build.**
   - lld on Windows keeps the post-LTO objects with
     `-Wl,--Xlink=-lldsavetemps`; the gold plugin on Linux keeps them with
     `-Wl,-plugin-opt=save-temps`. No object extraction step and no
     `objcopy -N main`.
   - In a tier build the launcher is a bare `main` that calls
     `run_engine`. The tier's LTO objects bind to anything a native object
     of the tier's link defines. The first full build failed because the
     engine had bound `std::string(string_view)` to the launcher's copy,
     and that launcher is not in the universal link.
4. **A.15.3's premise, corrected: `tzcnt` and SSE3 stay allowed.**
   - LLVM emits `tzcnt` (`f3 0f bc`, a rep-prefixed `bsf`) for a count of
     trailing zeros on the baseline. A CPU without BMI1 runs it as `bsf`.
     Forbidding it would fail every portable build (380 in the portable
     tier). The SDE runs on pre-BMI1 models (§4.7) prove the result.
   - MSYS2 builds its static libc++ for `-march=nocona` (SSE3). Every
     x86-64 CPU since 2005 has SSE3, so the portable floor is SSE3. SSSE3
     is not on it: AMD's K10 lacks it.
   - The portable set forbids SSSE3, SSE4.1, SSE4.2, AVX, BMI1 other than
     `tzcnt`, BMI2, LZCNT and POPCNT. Checked once with planted `andn`,
     `pshufb`, `ptest`, `vpxor` and `crc32q`; each fails. The avx2 tier
     forbids BMI1, BMI2 and LZCNT.
5. **Measured on the built binaries** (non-PGO-identical probes aside):
   - Every tier reproduces `bench 13` 19,470,325, with and without
     `BASILISK_ISA`, on Windows (clang 23.1.4, lld) and on Linux (WSL,
     clang 19.1.1, GNU ld with the gold plugin, static).
   - The sizes are 3,513,344 bytes on Windows and 5,140,440 on Linux
     (static, including glibc and libstdc++).
   - The universal contract passes on both. Its four regions: shared code
     clean; the portable tier clean; the avx2 tier with popcnt and AVX2 and
     no BMI or LZCNT; the pext tier with every family.
   - Synthetic disassembly with `pext` in the portable region, `popcnt` in
     shared code or `andn` in the avx2 region fails each time.

The pooled NPS read of §4.7 and the ladder are BAS-P20 (A.15.4). The SDE
selection matrix and the five assets are A.15.5.

## 13. Amendment 2 (2026-10-29): the speed read, and why it is not shipped

BAS-P20, after a repair of the harness's host-load measure, which had
refused quiet hosts:

- The universal binary against the pext asset: **−0.58%** [−0.82%, −0.33%],
  FAIL under the no-regression form. The pext tier build against the pext
  asset: +0.04% [−0.12%, +0.19%]. So the loss is in the relink, not in the
  tier build.
- The ladder: avx2 over portable +5.04%, pext over avx2 +2.82%, pext over
  portable +8.49%, each inside §9's frozen interval. A user of a pext CPU on
  the portable asset gives up about 17 Elo.
- The pext tier's hot code is byte-identical in both binaries (`negamax`
  is the same 4,713 instructions), and the function order is almost the
  same (8 adjacent inversions in 560). But the relink places the hot
  functions 16 bytes off, modulo 64, in all four builds. §4.7 assumed that
  four builds average layout out; builds of one source differ only in
  their profile, so they share one placement.
- By the registered rule the release keeps its nine per-tier assets. The
  build, the dispatcher and the checks stay in the tree. A.15.6 owns the
  placement question: a padding sweep to confirm alignment as the cause,
  then a repair that makes the tier's placement independent of the link,
  and a new gate.

## 14. Amendment 3 (2026-10-29): every trap guarded and seen to fail

A.15.6 shipped the binary with alignment. BAS-P21 found that the relink's
placement had been a real silent failure, so the traps of §5 and of Rarog's
notes (`D:/code/rarog/analysis/universal_binary_2026-09.md` §3, §7, §9) were
checked one by one: does each check exist, and has it been watched failing?
Two were unguarded (stale tiers, the image's alignment), and one check was
switched off (unprofiled files); all are fixed. "Planted" means the fault
was made on purpose and the check was run on it.

| Trap | Check, and where it runs | Seen to fail |
|---|---|---|
| First-wins symbols | After the rename, nothing a tier defines stays global but its entry, `_ZTI`/`_ZTS` and `.refptr` data (`cmake/universal-check-tier.cmake`, every tier build). Fathom is linked once, baseline, beside the dispatcher; `verify_isa.py --tier universal` requires `tb_init`, `tb_free` and both probe functions defined once as shared code and in no tier. The final link has no `--allow-multiple-definition`. | A tier object with `run_bench` put back under its plain name: "still global after the rename". A synthetic image with `tb_init` in the pext tier: fails (unit test). |
| PGO profiles | Each tier is the ordinary PGO build of its preset, renamed only after; a tier's PGO build makes `-Wprofile-instr-unprofiled` an error, except for the four files bench never runs (move formatting, Syzygy, the search diagnostics, WAC). A profile that does not match its code already fails, as `-Wprofile-instr-out-of-date` under `-Werror`. | The pext tier built on a near-empty profile (one start-and-quit run): six files without data, the build fails. The pext tier on the portable tier's profile builds (PEXT changes no control flow the profile hashes) and read −0.4%; with no profile −3.9%, indicative single builds against its own profile. |
| Early initialisers | No `.ctors`, `.dtors`, `.init_array`, `.fini_array`, `.preinit_array` or `.CRT$X[CIL]` section in any renamed tier object (`universal-check-tier.cmake`). | A planted object with a namespace-scope constructor: "carries .ctors, a static initialiser". |
| ISA attribution | `verify_isa.py --tier universal` attributes every function by its tier label; the portable tier and shared code meet the floor, the avx2 tier has no BMI1, BMI2 or LZCNT. Runs in both release cells. | Unit tests plant `popcnt` and `pext` in shared code, `popcnt` and `andn` in the portable tier, `andn` and `pext` in the avx2 tier: each fails in its region; letting the avx2 tier accept BMI1 fails the suite. |
| Stale tiers | The source (`src/`, `external/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`) is hashed before the first tier and after each; each tier's build directory must name the dispatcher's compiler and version (`universal-tiers.cmake`). | A wrong expected version: "built by clang++ 23.1.4, not 22.1.8". An edit to `src/attacks.h`: a different hash, the original back on restoring it. |
| Alignment | `verify_isa.py --tier universal`: at least 95% of each tier's functions, and every hot-path function by name, at 0 mod 64. CMake refuses a universal configure without `BASILISK_ALIGN_FUNCTIONS=64`. | BAS-P20's unaligned binary: 134 of 563 portable functions aligned, `negamax`, `quiescence`, the move picker, `is_legal` and `see_ge` named off their line. A synthetic image with `negamax` at 16 mod 64 fails (unit test). |
| Tier choice | The release benches every forced tier for the declared fingerprint and its banner; SDE checks Intel selection; `tests/test_cpu_features.cpp` holds the slow-PEXT truth table. | The forced-tier step with an expected count off by one: "the portable tier benches 19470325, not 19470326". The rule without family 17h: the truth table fails at Zen 1/2. |

The final tree passes all of it, built with the release cells' commands:
- Windows (clang 23.1.4, lld): 558, 558 and 555 of 563, 563 and 560
  functions aligned in the portable, avx2 and pext tiers, the exceptions
  destructor thunks and the terminate handler;
- Linux (WSL Ubuntu 24.04, Clang 19.1.1, gold, static): 484, 484 and 481 of
  485, 485 and 482, the ISA contract passing with the C runtime exempt;
- every forced tier 19,470,325 on both, with its banner.

## 15. Amendment 4 (2026-10-29): Stockfish's assembly route, for Clang and GCC

A.15.9 makes GCC build the universal binary as Clang does. Until now the
route depended on LLVM's linkers keeping their post-LTO objects (lld
`-lldsavetemps`, the gold plugin's `save-temps`). The new route is
Stockfish's (`universal-object-pgo` and `rewrite_asm_sections.awk`, read at
`edb0d9db`), for both compilers:

1. **The tier's LTO output as assembly.** GCC's `-save-temps=obj` keeps each
   LTO partition's `basilisk.ltrans<N>.ltrans.s` while the tier's executable
   still links (seven partitions a tier). Clang's linker writes assembly
   *instead of* an executable, so the tier build links the same objects a
   second time (`basilisk_tier_asm`, every option of the engine's link plus
   lld `--lto-emit-asm` or the gold plugin's `emit-asm`); lld writes one
   `.lto.<module>.s` beside each input (23 a tier), the gold plugin numbers
   them after the regular LTO partition's, which is empty.
2. **Renamed in the text.** The assembly is assembled once as written, and
   `nm` on that object lists what the tier defines, exactly as the objects
   route listed it. `cmake/universal-asm.py` then renames those names
   wherever the text uses them as symbols and the result is assembled.
   Stockfish's awk folds static constructors into one section; Basilisk's
   tiers have none (§4.3), so the script renames symbols instead, which
   Stockfish does with its namespace macro.
3. **Why the text and not objcopy.** GCC's PE COMDATs (`.linkonce discard`)
   are matched by section name, not by symbol. In GCC's LTO output they are
   the `.rdata$.refptr.<symbol>` pointers, 122 a tier, 90 of them to a
   symbol the tier defines, the evaluation tables among them
   (`.refptr._ZL8EG_TABLE.lto_priv.0`). Renamed by symbol alone, ld would
   keep one tier's pointer for all four, and the chosen tier would read
   another tier's tables, which only that tier initialises; nothing would
   fail. The text rename renames those section names with the symbols, and
   `check-sections` fails if any section is still named for a renamed
   symbol. Clang's 13 `.refptr` pointers a tier all point into the runtime,
   so the objects route never had this exposure.
4. **Type identities, GCC.** GCC's LTO output defines the type identities of
   the static libstdc++ it pulls in (`std::bad_alloc`, `std::exception`,
   `std::filesystem::filesystem_error`) as ordinary global data, not COMDATs:
   the first GCC link refused them, 238 multiple definitions. GCC's tiers
   therefore rename `_ZTI`/`_ZTS` with everything else. libstdc++ compares
   `type_info` by its name string (`__GXX_MERGED_TYPEINFO_NAMES` is 0), so a
   tier still catches what it or the runtime throws: `std::stoll`'s
   `invalid_argument` and `out_of_range` are caught on every GCC tier.
   Clang's tiers keep them shared, as before.
5. **Every symbol, not only the global ones.** The first Clang build on the
   new route failed the universal ISA contract: Clang names local lambdas
   with `$` (`…E3$_1…`), the first tokenizer split names at `$`, and those
   functions kept their plain names in every tier. They were local, so
   nothing was shared, but the image check attributed the avx2 and pext
   copies to shared code (590 AVX and 2 PEXT instructions). The tier check
   had looked only at global symbols; it now requires every defined symbol,
   local or global, to carry the tier's name.
6. **GCC's alignment.** GCC leaves out of `-falign-functions` every function
   it optimises for size, which under PGO is every function the training run
   never entered: 207 to 214 of each tier's 593 to 598 functions were
   aligned, though every hot-path function was. `-fmin-function-alignment`
   (GCC 14) aligns them all; a universal configure on an older GCC is
   refused.
7. **One more PGO guard, GCC's.** A GCC tier's use build makes
   `-Wmissing-profile` an error. GCC writes a profile for every file the
   training run links, run or not, so no file is exempt, and the check finds
   a missing or misplaced profile, not an unexercised file.
8. **Two Linux-only details.** The gold plugin's assembly files have no
   extension (`basilisk_tier_asm1` to `23`), which a compiler driver takes
   for a link input and skips with exit status 0; the tiers are assembled
   with `-x assembler`, and a missing object fails. GCC on ELF splits each
   function's never-run blocks into a `<function>.cold` fragment in
   `.text.unlikely`, labelled like a function but never entered by a call;
   the alignment rule leaves those fragments out (the ISA attribution does
   not). GCC on Windows does not split.

Built from the final tree on Windows (clang 23.1.4, MSYS2 GCC 16.2.0) and
Linux (WSL Ubuntu 24.04, Clang 19.1.1 with gold, GCC 14.2, static), all four
universal binaries bench 19,470,325 on every forced tier with its banner and
pass the universal ISA contract. Functions on a cache line: Clang 558 of 562
(Windows) and 483 of 484 (Linux) a tier, the rest its stubs; GCC every one.
Sizes: Clang 4,186,112 and 5,812,832 bytes, GCC 6,221,994 and 5,135,640.

Seen to fail on the new route (§14's table, re-run):

| Trap | Planted fault | Result |
|---|---|---|
| First-wins symbols | A global symbol left out of the map (`init_eval_tables`) | "still global after the rename" |
| Unrenamed locals | A local left out (`_ZL10g_kpk_once`) | "the local … is not renamed" |
| Section-named COMDATs | GCC's seven partitions renamed by symbol only, with objcopy; one Clang file the same way | "90 sections keep a name the rename moved"; "section .bss$_ZL10g_kpk_once is still named for …" |
| Early initialisers | A `.ctors` entry assembled into a tier object | "carries .ctors, a static initialiser" |
| PGO profiles, Clang | A tier on the profile of a start-and-quit run | 10 files "no profile data available" |
| PGO profiles, GCC | A tier with an empty profile directory | "profile count data file not found [-Werror=missing-profile]" |
| Stale tiers, source | `cmake/universal-tiers.cmake` edited while GCC's tiers built (by accident) | "Tier avx2: the source changed while the tiers were building" |
| Stale tiers, compiler | GCC's tiers expected as 16.2.1 | "built by …g++.exe 16.2.0, not … 16.2.1" |
| ISA attribution | `pextq` added to the base tier's `make_move`, relinked | "portable tier contains forbidden bmi2" |
| Alignment | `negamax` moved 16 bytes off its line, relinked | "starts at 16 mod 64" |
