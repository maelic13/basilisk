# Rarog audit 2026-10-24, appendix D: speed, build, measurement, tournaments, process and release

The measurement auditor's full report, verbatim (read-only audit of Rarog since 2026-08-11 against Basilisk at `ed3132f`). The index and the coordinator's verification notes are in [`rarog_audit_2026-10-24.md`](rarog_audit_2026-10-24.md).

# Rarog → Basilisk audit: speed, build/platform, measurement, tournaments, process and release (since 2026-08-11)

## Part 1 — Items

**D-1. The NPS read was rebuilt: four builds an arm, a t-interval on per-build medians, a two-step decision**
- Rarog source: RAR-P32, `analysis/nps_method_study_2026-10-02.md`, PROCESS *Toolchain and harness notes*, `tools/nps_read.py`, 2026-10-02/03
- Finding: within one reading, the three runs differ by 3.3% on average (up to 11%). The useful reduction is best of 3 (noise 0.27%). Median of 3 is much worse (1.32%). Builds of one source differ by 0.13%, so the builds set the limit, not the cycles: 3 cycles give ±0.36%, 20 give ±0.25%. Host load moves whole cycles by 2–4% rather than widening the noise. `nps_multibuild.ps1`'s bootstrap resamples readings, not builds, so it is not the uncertainty: the same pair read [+1.56, +1.89] and [+0.92, +2.56] two hours apart. The adopted read:
  - step 1: two cycles of `bench 13 3`, accept at +0.9%, close at +0.1%;
  - step 2, only in between: six cycles, +0.5% with the lower bound above 0;
  - a no-regression form mirrored around −0.5%;
  - a disturbance rule (a cycle more than 1% off its arm's median); every run archived.
- Category: instrument
- Basilisk status: MISSING
- Basilisk evidence: `tools/nps_ab.ps1:21` pools "≥2 PGO builds per arm". `tools/nps_ab.ps1:279` bootstraps "each arm's pooled samples", which is the reading-resampling interval Rarog retired. BAS-P17 used 2 builds × 16 rounds. PROCESS:378 still names `nps_ab.ps1`.
- Suggested Basilisk owner: new I1 leaf before B.7.2 (or B.7.1's sibling): port `nps_read.py` semantics into `nps_ab.ps1`.
- Confidence: high

**D-2. Two independent pools of one source are the real null, and a start-of-read idle check is not enough**
- Rarog source: RAR-P37 (2026-10-21), RAR-M48 (2026-09-11)
- Finding: pool-against-pool nulls read 0.00% and −0.20% (±0.4–0.6%). The host's level wandered 3.4% within half an hour. Two of six reads that started on a guard-idle host were disturbed, and both disturbed readings were wrong (−0.45% and −1.21% against −1.63% and 0.00%). RAR-M48 ran a 3v3 null of two independent pools: +0.04% [−0.23, +0.15].
- Category: instrument
- Basilisk status: PARTIAL
- Basilisk evidence: BAS-P17's "self pair" reuses the candidate's own two builds, not an independent pool. `nps_ab.ps1` has no per-cycle disturbance rule.
- Suggested Basilisk owner: same leaf as D-1
- Confidence: high

**D-3. A single binary is not an arm**
- Rarog source: RAR-P38, 2026-10-21
- Finding: one plain binary's processes varied 2.51–2.67 M nps (6%) from cycle to cycle, so a single-binary arm is uninterpretable. Plain builds have to be read as content-identical pools. A plain build embeds its build directory's path, so it reproduces per directory only.
- Category: instrument
- Basilisk status: UNVERIFIED
- Basilisk evidence: the `nps_ab.ps1 -OptionsA/-OptionsB` same-binary mode reads one executable per arm. Its per-process spread has never been measured on Basilisk.
- Suggested Basilisk owner: D-1 leaf
- Confidence: medium

**D-4. Fresh executables trigger antivirus scanning that contaminates a read**
- Rarog source: RAR-M48, 2026-09-11
- Finding: the first pool read 12–16% CPU (spikes to 41%) with no user process accountable, consistent with the application-control layer scanning about 40 MB of new executables. The same bytes read 2.3% slower than they did idle. An anomalous build spread is a host symptom before it is a build finding.
- Category: lesson
- Basilisk status: MISSING
- Basilisk evidence: no such caution in PROCESS:353–385 or `agents/measurement.md`. `run_b20_nps_gate.ps1` measures right after hashing.
- Suggested Basilisk owner: D-1 leaf (wait or poll after a pool build)
- Confidence: medium

**D-5. Child TT prefetch issued before the make, from a predicted key**
- Rarog source: RAR-P30/P31 (candidate A, `a42fadc`), 2026-10-02
- Finding: the TT probe's first load was 6.89% of samples, with the prefetch issued only after make. Prefetching from `key_after_hint` before make read +1.48% (screen), +1.69% [+1.56, +1.89] and +1.78%. It was accepted. A hint test fails under three planted key defects.
- Category: speed
- Basilisk status: MISSING
- Basilisk evidence: `src/search_kernel_core.cpp:1058-1059` calls `do_move(ss, m)` and then `shared_.tt.prefetch(position_key())`. No predicted-key helper exists (no `key_after` match in `src/`).
- Suggested Basilisk owner: B.7.2 candidate
- Confidence: high

**D-6. Pick-time child-TT prefetch, read under PGO**
- Rarog source: `analysis/c12_speed_pass_2026-10-17.md` (`1aaa39e`, `65f490e`); `c12_speed_review_2026-10-18.md`
- Finding: prefetching the child's TT line when the move is picked, ahead of the pruning tests, gave +0.5% and +0.3% under PGO. Moving it later or dropping it lost 1.7%. It read −0.04% without PGO. It is "the one prefetch that pays" under 4 KB pages, because it replaces a page walk.
- Category: speed
- Basilisk status: MISSING
- Basilisk evidence: as D-5
- Suggested Basilisk owner: B.7.2
- Confidence: medium

**D-7. Most other software prefetches lose under 4 KB pages**
- Rarog source: RAR-P30 (D −1.87%, I +0.14%), RAR-P31 (I +0.37%, reverted), C.12 passes (history prefetch −3.1%, correction prefetch −0.4%/−0.6%, store-side prefetch −0.26%, which became +0.32% on large pages)
- Finding: a prefetch into a table beyond the TLB's reach triggers the page walk itself. Issuing 65 prefetches per quiet generation costs more than the latency it hides. Revisit only with large pages.
- Category: lesson
- Basilisk status: NOT-APPLICABLE (prior only)
- Basilisk evidence: Basilisk has no history or correction prefetch (`grep prefetch src`: TT only)
- Suggested Basilisk owner: B.7.2 (as a prior against these candidates)
- Confidence: high

**D-8. Large pages for allocations of 2 MB and over**
- Rarog source: `c12_speed_followup_2026-10-17.md`, `c12_speed_review_2026-10-18.md` (`ac7c13b`, `34eb4bc`, `47bc1ba`); README `ad417f6`
- Finding:
  - The gain: +2.48% [+2.13, +2.84] pooled-PGO and +2.76% nodes/s in games (400-game read). The TT probe line fell from 4.3% to 1.6% of samples.
  - The mechanism: a Windows global allocator sends ≥2 MB to `VirtualAlloc(MEM_LARGE_PAGES)` when the token holds `SeLockMemoryPrivilege`, falls back silently, and `bench` prints `Large pages : on/off/partial`. Lowering the threshold to 512 KB (pawn history and pawn cache) added +0.36%.
  - The cost: four new `unsafe` sites.
  - Linux transparent huge pages (`madvise`) are noted as a separate site.
- Category: speed
- Basilisk status: PLANNED (G.1, docs/PLAN.md:1815–1817 "large pages")
- Basilisk evidence: no `VirtualAlloc`, `MEM_LARGE`, `madvise` or `SeLockMemory` anywhere in `src/` or CMakeLists. `src/tt.h:119` resize is a plain allocation.
- Suggested Basilisk owner: pull into B.7.2 (Rarog moved it from G.1 into C.12)
- Confidence: high

**D-9. Speed or large-page work can cost thread scaling, so re-read 4T**
- Rarog source: RAR-M74, RAR-M76, GUIDE holds, 2026-10-21/22
- Finding: after C.12's speed pass and large pages, the C head was +29.8% in games at 1T but −9.2% at 4T against the B.9 head. The 4T/1T in-game NPS ratio fell from 3.72 to 2.60, and the 4T pool gain fell from +53.5 to +4.9. The head-to-head release read against 2.5.0 could not see it (+149.7 at 4T), because both versions share the loss. The cheapest discriminator is `tools/nps_scaling.ps1` (bench at 1T and 4T on both heads).
- Category: lesson
- Basilisk status: PARTIAL
- Basilisk evidence: D.2 (PLAN:1717) plans self-relative scaling. B.7.2 and C.12 carry no 4T read. PROCESS:375 allows `bench 13 1 N`, but there is no scaling tool. Release rule §5 (PLAN:1846+) is head-to-head only.
- Suggested Basilisk owner: B.7.2 and C.12 done criteria (add a 1T/4T bench-scaling read); E.1
- Confidence: high

**D-10. A search frame copying a 4 KB move list per node**
- Rarog source: RAR-P27, RAR-P28 (candidate 2), 2026-10-01/02
- Finding: six 4,104-byte `memcpy` calls per node copied `ScoredMoveList` by value: 5.08% of samples in `vcruntime`. Building the list in place gave +6.93% [+6.38, +7.68], two to three times the prediction. The caller table under-attributed the copies.
- Category: speed
- Basilisk status: UNVERIFIED
- Basilisk evidence: `src/move_picker_core.cpp:149-152` generates into a local `MoveList` and then `fill(list, …)` copies it into `moves_`, one copy per stage. Its cost is unmeasured because Basilisk has no profiler (D-13).
- Suggested Basilisk owner: B.7.2 (profile first)
- Confidence: medium

**D-11. Quiet-stage early exit once quiets are skipped**
- Rarog source: RAR-P27/P28 (candidate 1, `5fe42cf`)
- Finding: under `skip_quiets`, 60% of all scanned elements and 64% of all picks produced nothing. Counting survivors (checks, promotions) at generation and exiting when none are left gave +8.85% [+8.17, +9.48]. It is exact by construction.
- Category: speed
- Basilisk status: PARTIAL
- Basilisk evidence: `src/move_picker_core.cpp:162-189`. With `keep_direct_checks_` the picker walks every remaining quiet in two stages, calling `direct_check` on each. It is a linear walk rather than Rarog's selection scan, so it is cheaper, but it has no survivor count. Without `keep_direct_checks_` the loop exits immediately.
- Suggested Basilisk owner: B.7.2 (profile decides)
- Confidence: medium

**D-12. Attack lookups through a lazy-init check plus a heap pointer; tables built at compile time**
- Rarog source: RAR-P30 (B, prototype −0.49%), C.12 `b52520d` (+2.36% screen, +1.46% read)
- Finding: a `LazyLock` state check (0.6–0.71% of samples) and the slider table's heap pointer sat on every lookup. A plain static with fixed-size tables built at compile time read +1.46%. The binary grew from 1.01 to 1.85 MB.
- Category: speed
- Basilisk status: PARTIAL
- Basilisk evidence: `src/attacks.cpp:195` fills plain global arrays at start-up, so there is no lazy check. Lookups still load `MagicEntry::attacks` (a pointer) and are defined out-of-line in `attacks.cpp`, so inlining depends on LTO.
- Suggested Basilisk owner: B.7.2 (low priority)
- Confidence: medium

**D-13. A profiler with inline-chain attribution, and its traps**
- Rarog source: RAR-P27, RAR-M30/M36, `board_search_profile_2026-09-07/08.md`, C.12 user-mode sampler, `d42fa46`
- Finding:
  - The instruments: per-region shares of process samples via ETW plus llvm-symbolizer with full inline chains, over five frozen cohorts. A user-mode sampler later did the same without administrator rights.
  - The traps: xperf with `-symbols` switches to a per-function schema, which silently charges inlined board work to `negamax`. The tool reported "100% resolved" while wrong, twice. Stale mechanism markers under-read renamed functions. The clean-up step once deleted the only PDB.
  - The fixes: the tool now validates the schema from the data and refuses the wrong one.
- Category: instrument
- Basilisk status: MISSING
- Basilisk evidence: no profiler in `tools/` (only in the reference copy `docs/reference/rarog/tools/profile_etw.ps1`). B.7.2 (PLAN:1428-1434) requires "a profile of board work inside HCE search". The Clang/MinGW toolchain needs its own symbol path, likely `-gcodeview` or DWARF.
- Suggested Basilisk owner: new I1 leaf before B.7.2
- Confidence: high

**D-14. Screen branch-shape and prefetch changes with PGO**
- Rarog source: C.12 pass (`081041c` `select_unpredictable`: +0.1% non-PGO, +4.24% PGO); RAR-P30
- Finding: non-PGO screens mis-rank in both directions. Two-build PGO screens resolve only about ±1.5%; the pawn entry read +1.06% on two builds and +0.02% on four. A plain-build screen on a host in use reads ±3% and is no evidence.
- Category: lesson
- Basilisk status: PARTIAL
- Basilisk evidence: the `tools/nps_ab.ps1` header says "Screen non-PGO, confirm under PGO", which under-weights false negatives.
- Suggested Basilisk owner: B.7.2 method note
- Confidence: high

**D-15. Software-practice speed priors from three passes**
- Rarog source: C.12 three passes, 2026-10-16..18
- Finding:
  - Gains: compile-time constant weights and parameters outside tune builds (part of e1 +5.67%); set intersection instead of square walks (+4.9% screen); an imbalance cache by piece counts (+1.6%).
  - Losses: removing a whole-eval cache that served 966 of 4.1M calls (+0.8%, i.e. the cache cost speed); `opt-level 2` −3.9%; cmov-converter off −0.34%; branch-free TT probe −2.0%; history tables re-indexed as aligned lines −2.88%; removing a `MoveList::push` bounds check −9.6% (RAR-M44).
  - Removing work on the latency chain read flat; removing two million calls of a 10–20 ns function reads about +0.2%.
- Category: lesson
- Basilisk status: NOT-APPLICABLE (priors for B.7.2 and C.12)
- Basilisk evidence: PLAN C.12 (1685–1692) is generic. Basilisk builds `-O3` (CMakeLists:419).
- Suggested Basilisk owner: B.7.2, C.12
- Confidence: medium

**D-16. A bundle can hide a loss; read members alone**
- Rarog source: `c12_speed_review_2026-10-18.md`
- Finding: a +0.15% bundle contained a −0.58% member (the quiet-SEE skip). Its +1.13% had been a two-cycle accept on a pool that happened to read low.
- Category: lesson
- Basilisk status: MISSING
- Basilisk evidence: B.7.2 says "+0.5% per change" but has no member re-read rule.
- Suggested Basilisk owner: B.7.2
- Confidence: high

**D-17. At eight builds an arm, the +0.5% bar leaves real gains unbankable**
- Rarog source: `c12_speed_review_2026-10-18.md`; maintainer rule for "safe small gains"
- Finding: eight builds an arm resolve ±0.2–0.25%. Two +0.2% changes summed to +0.41% [+0.28, +0.54], real by the interval but under the bar. The maintainer kept them as safe exact changes, and kept T and E on large pages the same way.
- Category: lesson
- Basilisk status: MISSING
- Basilisk evidence: no such decision rule in Basilisk PROCESS
- Suggested Basilisk owner: maintainer decision at B.7.2 registration
- Confidence: medium

**D-18. A behaviour-neutral restructure or cleanup is not speed-neutral when it deletes per-node work**
- Rarog source: RAR-P24 (+6.30%), RAR-P34 (+1.16%)
- Finding: deleted per-node `lmr_reduction_units`, root-confidence snapshots, and default switches that were still read per node all paid. The prediction "compiled out or never executed" held only for diagnostics.
- Category: lesson
- Basilisk status: PARTIAL
- Basilisk evidence: BAS-P16 +1.21% "unexplained". PLAN B.8 (1435–1437) is a no-speed-claim cleanup.
- Suggested Basilisk owner: B.8 (read NPS no-regression and record any gain)
- Confidence: high

**D-19. A search programme costs speed; read pooled NPS at the checkpoint**
- Rarog source: RAR-P35 (−26.23% B.9 against 2.4.0), RAR-P39 (+28.92% C head against B.9)
- Finding: the B programme cost a quarter of bench speed even after B.7 recovered about 19%. A chained estimate missed the remaining −15% because no intermediate pools existed. In-game NPS is not bench NPS (0.69× against 0.74×).
- Category: lesson
- Basilisk status: HAS (planned)
- Basilisk evidence: PLAN B.9 (1438–1446) lists pooled NPS among the meters
- Suggested Basilisk owner: B.9
- Confidence: high

**D-20. A cheap microbenchmark falsifier before a layout rewrite**
- Rarog source: RAR-P29 (scan layout `s` 1.106/1.093, below the 1.25 bar, closed); RAR-P33 (incremental SEE −2.09%); RAR-M35 (two-stage rule saved the full-search arm)
- Finding: replay recorded workloads offline and fix the go/close bound before building. Exchange-length premises (62.5% of SEE calls end at step 1) were never measured beforehand.
- Category: lesson
- Basilisk status: PARTIAL
- Basilisk evidence: Basilisk has `board_performance.cpp`. B.7.2 says archived board leaves "default to no-change". No falsifier rule.
- Suggested Basilisk owner: B.7.2
- Confidence: medium

**D-21. Lazy start-up tables charged to the first search's clock**
- Rarog source: RAR-M59, `9a7b663`, `c0e6ef7`, 2026-09-19
- Finding: Rarog's KPK bitbase was built on its first probe (34 ms). The 4T first search converted the TT (+20 ms, 32,917 page faults). Helpers held unused 64 MiB tables. Per-thread tables were first touched by the first search. All of it now happens at start-up or configure.
- Category: speed
- Basilisk status: PARTIAL
- Basilisk evidence: KPK is fixed (`src/eval.cpp:841-847`, built in `init_eval_tables`). Whether per-thread histories and the TT are touched before the first `go` (`search_thread_pool.cpp:42`, `tt.h:119`) is UNVERIFIED.
- Suggested Basilisk owner: D.3.2
- Confidence: medium

**D-22. Undo-history reallocation that `bench` cannot see**
- Rarog source: RAR-M38, 2026-09-08
- Finding: `bench` builds every position from FEN, so it never grows the history. A 64-move game reallocated on the next search's first push. Reserve once and keep capacity across clones.
- Category: speed
- Basilisk status: HAS
- Basilisk evidence: `src/board.h:108-114` (`HISTORY_RESERVE`; capacity preserved across copies), `src/board.cpp:231,236`
- Suggested Basilisk owner: —
- Confidence: high

**D-23. Allocation guard and per-node copy audit**
- Rarog source: RAR-P31/P33 ("allocation guard" in qualification), HISTORY 2026-10-06 audit (`6433760` removed the last per-node allocation, the rule-50 mate test in check)
- Finding: a test with a counting allocator asserts allocations grow per iteration, never per node. Pair it with a disassembly search for `memcpy` and allocator calls in `negamax`, `qsearch` and `evaluate`.
- Category: speed
- Basilisk status: PLANNED (B.7.1, PLAN:1420-1426)
- Basilisk evidence: B.7.1 is held ANY TIME before B.7.2. A copy audit is not planned.
- Suggested Basilisk owner: B.7.1 (add the copy audit)
- Confidence: high

**D-24. The ISA tier ladder is measured, not assumed**
- Rarog source: RAR-P20, RAR-P22, `universal_binary_2026-09.md`
- Finding: avx2 over base +4.59% [+3.79, +5.83]; pext over avx2 +2.45%; direct pext over base +6.65% [+6.48, +6.89]. Chaining the two hops overstated the composite (+7.15%), because the first hop was 2.04 pp wide. Compose ratios only when each is tighter than the difference you care about. A frozen decision rule picked the fat-binary design over multiversioned kernels.
- Category: build/platform
- Basilisk status: MISSING
- Basilisk evidence: no per-tier NPS evidence in `docs/experiments` or PLAN. Basilisk ships three x86 tiers (`release.yml` matrix).
- Suggested Basilisk owner: G.2 (or an A-style one-off measurement before E.3's README text)
- Confidence: high

**D-25. Tell the user when they run the wrong asset**
- Rarog source: A.4.2 (`cpu_advice.rs`), `eed3f6b`, 2026-09-10/10-22
- Finding: an advisory when a lower tier runs on a CPU that could run a higher one. It names no speed figure.
- Category: build/platform
- Basilisk status: MISSING
- Basilisk evidence: `src/main.cpp:66-77` refuses an incompatible tier only, and prints only to `std::cerr` (harnesses keep stdout). No downward advisory.
- Suggested Basilisk owner: D.3.2 or G.2
- Confidence: high

**D-26. The CPU guard must run in a baseline-compiled launcher**
- Rarog source: RAR-P09; `universal_binary_2026-09.md` §8
- Finding: a runtime check for a statically required feature folds to true. Rarog's guard could never fire and was removed.
- Category: build/platform
- Basilisk status: HAS
- Basilisk evidence: a separate baseline launcher object, verified by `tools/diag/verify_isa.py verify_launcher` (needs `__cpu_model` or `cpuid`) in `release.yml` and `ci.yml`
- Suggested Basilisk owner: —
- Confidence: high

**D-27. The ISA contract executes, including ARM64 and required hint instructions**
- Rarog source: RAR-P09, RAR-P10/P11, RAR-P15, RAR-P19, `xtask verify-isa`
- Finding: every asset is disassembled. Forbidden classes fail, and required classes must appear: `prfm` on arm64; sve, aes, sha2 and dotprod are forbidden on the bare-NEON tier. "For a hint-shaped optimisation, verify the instruction, not the behaviour." A missing ARM prefetch had been a silent three-release loss. CI's plain-build cells use `--default-cpu`.
- Category: build/platform
- Basilisk status: PARTIAL
- Basilisk evidence: `tools/diag/verify_isa.py:105` accepts tiers portable, avx2 and pext only. There is no ARM contract and no required prefetch class. CI and release run it on x86 cells only.
- Suggested Basilisk owner: G.2, or a small I1 leaf under E.3's "target-native ISA checks" (PLAN:1756)
- Confidence: high

**D-28. A compiler bump is a speed change: read it on pools**
- Rarog source: RAR-P18, RAR-P36, RAR-P37, RAR-P38, PROCESS toolchain notes, 2026-09-09/10-21
- Finding: rustc 1.98.1 → 1.99.1 (LLVM 22 → 23) kept the fingerprint exactly but cost −1.51% / −1.63% / −1.66% pooled-PGO. Plain builds lost −3.63%, so the loss is code generation, and PGO recovers about 2 points. An LLVM minor step was unresolved (−0.53%). Register the next major with about ±2.5%. Never bump between an SPRT's arms. The maintainer keeps the latest stable regardless of speed.
- Category: build/platform
- Basilisk status: PARTIAL
- Basilisk evidence: `analysis/toolchain_refresh_v1.md` freezes Windows Clang 22.1.8, Linux clang-19 and AppleClang 17. No rule says a bump owes a pooled read.
- Suggested Basilisk owner: PROCESS *Build, bench and NPS* (rule); G.2
- Confidence: high

**D-29. The Windows release compiler floats in CI**
- Rarog source: `rust-toolchain.toml` pin; ci.yml and release.yml ("CI builds with the exact same rustc the developer does")
- Finding: Rarog's CI and release build with the pinned compiler, and print `rustc -vV`.
- Category: build/platform
- Basilisk status: PARTIAL (likely defect)
- Basilisk evidence: `release.yml` Windows cells install `mingw-w64-clang-*-clang` from MSYS2 at run time (`update: false` does not pin a version) and never print or assert the version. Linux pins clang-19 and prints it; the frozen "22.1.8" of `toolchain_refresh_v1.md` is not enforced. A tag run could ship assets from an unmeasured LLVM major (D-28: −1.6%).
- Suggested Basilisk owner: new I1 leaf (assert or print the compiler version per cell; pin the MSYS2 package)
- Confidence: medium

**D-30. Profile tools from PATH**
- Rarog source: RAR-P36 (`da2780f`)
- Finding: xtask took MSYS2's `llvm-profdata` (LLVM 22) from PATH, and it refused LLVM 23 raw profiles. Tools now come from the toolchain first.
- Category: build/platform
- Basilisk status: HAS
- Basilisk evidence: BAS-P08; `release.yml` Linux step `clang++ --print-prog-name=llvm-profdata` ("uses this same lookup rather than PATH ordering")
- Suggested Basilisk owner: —
- Confidence: high

**D-31. Vendored C code does not receive the profile**
- Rarog source: RAR-P19, RAR-P21, `b758287`
- Finding: no platform compiled Fathom with PGO, and macOS Apple clang later refused the inherited profile entirely. "Absence of a warning was read as presence of a behaviour." Never accept a null from a log until a flag you know is present shows up in it.
- Category: lesson
- Basilisk status: NOT-APPLICABLE
- Basilisk evidence: Basilisk compiles `external/fathom` inside CMake with the same toolchain and flags. Whether it is instrumented is unverified, but probe speed is immaterial.
- Suggested Basilisk owner: —
- Confidence: medium

**D-32. LTO mode and optimisation level are measured, not inherited**
- Rarog source: `c12_speed_followup` ("opt-level 3 beats 2 by 3.9%; fat LTO, one codegen unit and PGO stay")
- Finding: Rarog verified its build flags by pooled read.
- Category: build/platform
- Basilisk status: UNVERIFIED
- Basilisk evidence: CMakeLists:572-577 uses `INTERPROCEDURAL_OPTIMIZATION_RELEASE`, i.e. CMake's default (thin LTO for Clang; RAR-M20 records Basilisk `-flto=thin`). No full-against-thin measurement appears in docs or experiments.
- Suggested Basilisk owner: B.7.2 build-flag candidate
- Confidence: medium

**D-33. Windows ABI and compiler comparison**
- Rarog source: RAR-P23
- Finding: gnullvm against msvc was unresolved (+0.84%, with signs that disagree across arms); gnu lacks `profiler_builtins` and was −1.37%. The pool was not widened after the result.
- Category: build/platform
- Basilisk status: HAS (analogue)
- Basilisk evidence: BAS-P09 (MSVC rejected as the Windows default)
- Suggested Basilisk owner: —
- Confidence: high

**D-34. Apple's 128-byte cache line and TT over-alignment**
- Rarog source: RAR-P12, RAR-P16
- Finding: no material false sharing (ARM 3.89× at 4T). macOS returns 16 MiB-granular bases, so the wrapper cannot move an address. It was rejected for the third time. Size the population before building the fix.
- Category: build/platform
- Basilisk status: HAS
- Basilisk evidence: BAS-P07 (rejected); the retry map in docs/EXPERIMENTS.md:693
- Suggested Basilisk owner: —
- Confidence: high

**D-35. Windows-on-ARM and macOS hosts: emulation makes pools incomparable**
- Rarog source: RAR-P41, RAR-P43, RAR-P44
- Finding: x86 opponents under Prism or Rosetta run a quarter to a third slower relative to Rarog. Stockfish 11 under Rosetta led 2.6.0 by only +16.5; native, it led by +91.9 (the match host reads +89.3). Native builds gain against entered ratings (Basilisk +101). Record emulation per engine from `IMAGE_FILE_HEADER.Machine`. ARM-host pools are not usable for E.2. The Rarog-against-Rarog gaps reproduce.
- Category: lesson
- Basilisk status: NOT-APPLICABLE (record-keeping)
- Basilisk evidence: no Basilisk ARM pool record exists
- Suggested Basilisk owner: E.2 (state the host and native/emulated status per engine)
- Confidence: high

**D-36. Colosseum re-pin: 0.2.0 → 0.3.0 → 0.4.0**
- Rarog source: `c5f11b1` (0.3.0, 2026-10-14), RAR-M70 cross-check, `edbd475`/`de74cd8`/`a7afa08` (0.4.0, 2026-10-22), PROCESS *Harness*
- Finding:
  - 0.3.0 keeps engine processes per slot in every mode, writes engine input from a dedicated thread and makes datagen 28% faster. Its timed games were cross-checked against fastchess: +6.9 ± 9.4 against +6.78 ± 9.07, which the frozen rule calls agreement. The null pair was waived.
  - 0.4.0 reports per-engine depth, NPS and `h=` hold percentiles, pair Elo with pentanomial, terminations, faults by engine, and tune `arm_clamps`/`rail_warnings`. A reporting-only re-pin owes no cross-check or null.
- Category: harness
- Basilisk status: MISSING
- Basilisk evidence: `tools/colosseum/colosseum.pin.json` is `colosseum-cli 0.2.0` (pinned 2026-09-25) on HEAD and on `dev`. `gauntlet.toml` unsets `engine-processes`.
- Suggested Basilisk owner: new V/I1 leaf before B.3's gate (re-pin plus RAR-M70-style cross-check)
- Confidence: high

**D-37. Never hand-parse in-game NPS from PGN comments**
- Rarog source: RAR-M73/M74 corrections, PROCESS *Harness*, `a7aa921`
- Finding: a one-off regex skipped 16.3–16.4% of search comments (a negative `h=` or a bare `{t=…}`). It gave later moves to the other engine and corrupted four ledger rows' NPS and depth. Use the runner's `stats`.
- Category: lesson
- Basilisk status: NOT-APPLICABLE (no such parser found)
- Basilisk evidence: `tools/diag/*.py` have no NPS comment parser. After D-36, use 0.4.0 `stats`.
- Suggested Basilisk owner: D-36 leaf
- Confidence: high

**D-38. Opponent faults are read under a 1% rule; the arm's faults void the run**
- Rarog source: RAR-M66, RAR-M73/M74 addenda, PROCESS ("registration adopts the runner's fault policy by reference, never a stricter clause", RAR-E29/E32)
- Finding: Houdini 3's start-up crash at 4T (about 0.125–1%) is read, with the pair also reported without the crashes. A registration-specific "void on any forfeit" could not survive.
- Category: harness
- Basilisk status: PARTIAL
- Basilisk evidence: `tools/colosseum.ps1:263,387-388` throws on any engine fault in a gauntlet. BAS-M10 was VOID on two Houdini crashes, and BAS-M11 moved to the GUI under a maintainer crash rule. BAS-M12 diagnosed the race.
- Suggested Basilisk owner: D-36 leaf (wrapper: zero tolerance for the arm, rate rule for opponents)
- Confidence: high

**D-39. Label a stopped SPRT's estimate as a first-crossing estimate**
- Rarog source: PROCESS *Harness* (2026-10-11), RAR-M60, RAR-M45
- Finding: a stopped SPRT's point estimate is selected at the boundary and is recorded as a "first-crossing estimate", never as the effect size. A stopped SPRT is the wrong instrument for parity. A fixed 600-game run read +70 against the SPRT's +54.8, so the bias caveat does not imply the truth is lower. A stopped SPRT is never extended or re-scored on a counterfactual.
- Category: lesson
- Basilisk status: MISSING
- Basilisk evidence: `agents/measurement.md` *Gating* has "One gate, one read: an SPRT's verdict and its estimate at the stop are the record", with no bias label. There is no counterfactual rule.
- Suggested Basilisk owner: agents/measurement.md (rule)
- Confidence: high

**D-40. Speed-to-Elo is measured at +18% too**
- Rarog source: PROCESS (RAR-S98: +18.4% NPS → +35.5 ± 9.0; RAR-P02: +10.35% → +20.3; about 145 Elo per doubling against the textbook 60); C.12 RAR-S99 +22.47% → +42.3 ± 9.5
- Finding: about 2 Elo per 1% held up to +22% at `3+0.03`. The textbook doubling rule under-predicts this engine class by about 2.4×.
- Category: lesson
- Basilisk status: PARTIAL
- Basilisk evidence: PROCESS:382-384 says "holds for small deltas only; above a few percent convert through doublings", which conflicts with Rarog's newer measurement.
- Suggested Basilisk owner: PROCESS (cite RAR-S98/S99 as a prior)
- Confidence: medium

**D-41. Opponents play the smallest safe clock overhead**
- Rarog source: PROCESS *Harness* (`a9599af`, `951bc2d`), `d12_tm_research_2026-10-22.md`
- Finding: shipped overhead defaults set for long controls cost tens of Elo at `3+0.03` (Stockfish 11's `Move Overhead` 30 removed 23% of the opening horizon). Every engine with such an option plays 10 ms. Ratings measured before and after the rule are not comparable.
- Category: harness
- Basilisk status: NOT-APPLICABLE today
- Basilisk evidence: Basilisk's pool (Houdini 3, Critter, Fritz 16, Rybka 4.1) has no such options. It applies the moment Stockfish, Komodo or Dragon join (E.2).
- Suggested Basilisk owner: E.2 registration
- Confidence: medium

**D-42. Rate against a field that brackets the engine; the held scale maps to CCRL**
- Rarog source: RAR-M75
- Finding: a field scored 80–92% against read the C head about 50 Elo low. On modern engines the held scale maps to CCRL Blitz as held = 163.8 + 0.954·CCRL (r 0.980), so the head's equivalent is 3,475 ± 82. An older fit through weaker engines had slope 1.19.
- Category: lesson
- Basilisk status: PARTIAL
- Basilisk evidence: PLAN §4 pool meter is the "fixed pool with Houdini 3". Basilisk scores 14–42% against it (BAS-M11), so it is bracketed from above only by the targets.
- Suggested Basilisk owner: E.2
- Confidence: medium

**D-43. A release read against the previous version cannot see a shared 4T loss**
- Rarog source: RAR-M76
- Finding: STC +153.4, LTC +149.3 and 4T +149.7 against 2.5.0, while the pool showed thread scaling fallen. "Read both before a release that claims multi-thread strength."
- Category: release
- Basilisk status: PARTIAL
- Basilisk evidence: PLAN §5 (1846+) requires an SPRT and `10+0.1`/4T head-to-head direction checks only. E.2 has a 4T pool, but the release rule does not require it.
- Suggested Basilisk owner: PLAN §5 / E.3
- Confidence: high

**D-44. Self-relative thread-scaling curve**
- Rarog source: RAR-M46 (2.4.0 at 4T is better than at 1T against old engines; Basilisk 1.10.0 scaled 5.85×; Rarog gained +48 against Basilisk); GUIDE hold; `tools/nps_scaling.ps1`
- Finding: NPS and depth do not explain 4T conversion. Measure an engine against itself at 1/2/4/8T.
- Category: plan item
- Basilisk status: PLANNED (D.2, PLAN:1717-1723)
- Basilisk evidence: D.2 text "Self-relative scaling ... before any design"
- Suggested Basilisk owner: D.2
- Confidence: high

**D-45. Hash adequacy, not equality, across thread counts**
- Rarog source: RAR-M46 decision (4)
- Finding: Hash 64 at 4T is a 9.8× overwrite. The fix sizes the table so that entries per node searched stay roughly constant (512 MB at 4T). Bench cannot answer a hash question: about 190k nodes a position against 11M nodes a game.
- Category: harness
- Basilisk status: HAS
- Basilisk evidence: BAS-M10/M11 use Hash 64 at 4T. Rarog's 4T pool uses 512, so Basilisk's 4T row is not comparable with Rarog's 4T rows.
- Suggested Basilisk owner: E.2 registration (state the hash rationale)
- Confidence: medium

**D-46. Conversion instrument: cross-engine "surplus" claims are unstable**
- Rarog source: RAR-M47/M49/M54/M64/M73
- Finding: Basilisk 1.9.3's draws-up rate on one unchanged binary moved 16.7 → 26.1 → 17.5 per 1,000 across tournaments. Compare an engine against its own prior sample, and a competitor only inside the same tournament. Export the PGN once, cite it by sha256, and track only the tool and a frozen summary.
- Category: instrument
- Basilisk status: HAS
- Basilisk evidence: `tools/diag/conversion_audit.py`, `export_tournament_pgn.py`, BAS-M08
- Suggested Basilisk owner: —
- Confidence: high

**D-47. Copy the SQLite WAL with the database**
- Rarog source: RAR-M45
- Finding: copying `.sqlite` without its `-wal` file returned 39,437 of 39,600 games and a plausible but wrong termination table.
- Category: lesson
- Basilisk status: UNVERIFIED
- Basilisk evidence: `tools/diag/export_tournament_pgn.py` (not inspected for WAL handling)
- Suggested Basilisk owner: tooling note
- Confidence: medium

**D-48. Board benchmark parity: a harness timing a return copy**
- Rarog source: RAR-M43/M44, `board_benchmark_recipe_2026-09-05.md`
- Finding: Rarog's "legal moves" column timed a 520-byte `MoveList` return copy that Basilisk's harness had removed. A board column is not search speed, and its sign is not guaranteed to survive (+12.7% bench, −0.55% search). Basilisk was faster in every comparable column.
- Category: instrument
- Basilisk status: HAS
- Basilisk evidence: BAS-X16; `tests/board_performance.cpp`
- Suggested Basilisk owner: —
- Confidence: high

**D-49. Datagen label audit per corpus, with a frozen hold rule**
- Rarog source: RAR-M67, RAR-M68, RAR-M69, RAR-M71
- Finding: the share of clean wins not won fell 20.33% → 6.86% (B.9 search) → 6.74% → 6.66%. Conversion at 8,000 nodes is the search's. A frozen threshold holds the fit.
- Category: instrument
- Basilisk status: HAS (tool)
- Basilisk evidence: `tools/diag/datagen_label_audit.py`; C.2 (PLAN:1524)
- Suggested Basilisk owner: C.2
- Confidence: high

**D-50. A harm read for a narrow behaviour change**
- Rarog source: RAR-M72
- Finding: a 2,000-game match with a frozen −13 nElo harm line and the power stated up front (it catches −20 nElo with probability 0.82).
- Category: harness
- Basilisk status: HAS (equivalent)
- Basilisk evidence: PROCESS "A measurement with an interval, which decides nothing"; repair bracket
- Suggested Basilisk owner: —
- Confidence: medium

**D-51. Prove a harness wire live with an absurd value; the anchor must be unique and executable**
- Rarog source: `analysis/answer_harness_rset_correction.md`, RAR-M29, RAR-P20, RAR-M33
- Finding: a scripted edit anchored on a string that also occurred in the docstring. `--rset` measured defaults and produced two false nulls. A deliberately absurd value must move the numbers.
- Category: lesson
- Basilisk status: HAS
- Basilisk evidence: `agents/measurement.md` *Acceptance* ("an absurd option value must move the numbers"); AGENTS *Verification* ("asserts its anchor matched once")
- Suggested Basilisk owner: —
- Confidence: high

**D-52. Measurement on a busy host is void**
- Rarog source: RAR-M32 (a Manta SPRT held the host at 50% CPU; the result was withdrawn), RAR-M33
- Finding: the runner asserts idleness rather than annotating it, and the gate is proven live with a deliberately absurd threshold.
- Category: harness
- Basilisk status: HAS
- Basilisk evidence: PROCESS:343 ("Never run two pinned harnesses at once, from Basilisk and Rarog together"); `harness_common.ps1` idle guard; `agents/measurement.md`
- Suggested Basilisk owner: —
- Confidence: high

**D-53. SPSA tune shape: 15 slots × 30 games**
- Rarog source: RAR-M62
- Finding: 1.65× the games per hour of 14 × 32, because the iterations pack into full waves.
- Category: harness
- Basilisk status: HAS
- Basilisk evidence: PROCESS *Harness* ("fifteen slots × thirty games per iteration")
- Suggested Basilisk owner: —
- Confidence: high

**D-54. Colosseum against fastchess parity, SPRT drift model and adjudication-off default**
- Rarog source: RAR-M60, RAR-M61, RAR-M10, RAR-M13–M17
- Finding: the instruments agree within their intervals. Drift per game is about 8.3e-6·(Elo1−Elo0)·(true−midpoint). Adjudication destroys 52.7% of endgames before they are reached.
- Category: harness
- Basilisk status: HAS
- Basilisk evidence: BAS-X32; PLAN §4 (RAR-M10 prior); PROCESS *Adjudication*
- Suggested Basilisk owner: —
- Confidence: high

**D-55. One branch per change, squash PR, fixed message shape**
- Rarog source: PROCESS *Branches and pull requests* (`125bff2`, 2026-10-09); PRs #5–#11
- Finding: `master` is the trunk. Branches are short-lived. The squash message has a short imperative title with no results or plan references, plain lines wrapped at 72, results one per line, then `Bench:`. The PR runs CI and Release. The branch is deleted after merge. Rarog has executed this seven times since 2026-10-09.
- Category: process
- Basilisk status: PARTIAL (adopted on paper, transition unfinished)
- Basilisk evidence:
  - The rule is adopted: PROCESS:548-591 *Delivery*, AGENTS *Commits and reporting*, PLAN §5.
  - No PR since #4 (2026-10-08). `origin/master` is still `fac5064`.
  - The Transition clause (PROCESS:552) said `dev` lands as one final squash PR once B.2 is done. B.2 is `[x]`, but `dev` (pushed 2026-10-23) has not landed.
  - `b10-pst-orientation` branches from `dev` (merge-base `24a68cc` = `dev`), not from `master`. There are 120 commits on HEAD that are not on `master`.
- Suggested Basilisk owner: maintainer (land `dev` by squash PR; new work from `master`)
- Confidence: high

**D-56. CI on pull requests to master, with the full matrix**
- Rarog source: ci.yml (PR into master plus dispatch; the identical job set on every trigger)
- Finding: Rarog runs lint, tests on 3 OS × 2 profiles, a long tier by exact name, five bench-agreement cells and feature builds.
- Category: process
- Basilisk status: HAS (exceeds Rarog)
- Basilisk evidence: `.github/workflows/ci.yml:22-24`. Basilisk adds ASan+UBSan, GCC PGO, the documents job (`check_roadmap.py`, `check_citations.py:288`, `test_release_check.py`) and build flavors.
- Suggested Basilisk owner: —
- Confidence: high

**D-57. A test filter that matches nothing must fail**
- Rarog source: ci.yml long tier ("a filter that matched nothing would pass with zero tests"; it asserts that exactly two ran)
- Finding: Rarog counts the tests its filter ran.
- Category: process
- Basilisk status: MISSING
- Basilisk evidence: `ci.yml:199,227,234` run `ctest -R test_uci_protocol` without `--no-tests=error` or a count check
- Suggested Basilisk owner: small I1 CI fix
- Confidence: high

**D-58. Required status checks and linear history on master**
- Rarog source: PROCESS *Release* step 1 ("Both workflows should be required checks on `master`"); ci.yml comment "master enforces linear history"
- Finding: neither repository enforces required checks. Rarog's CI comment's claim about linear history is also false; squash-only merging is what gives the effect.
- Category: process
- Basilisk status: MISSING (and also missing in Rarog)
- Basilisk evidence: `gh api …/branches/master/protection`: `required_status_checks: null`, `required_linear_history: false` for both repositories. Both allow squash only.
- Suggested Basilisk owner: maintainer (repository settings)
- Confidence: high

**D-59. Keep PR refs through pruning fetches with a configured refspec**
- Rarog source: `f209981` (2026-10-11), AGENTS lines 234-243
- Finding: `remote.origin.fetch` carries `+refs/pull/*/head:refs/remotes/origin/pr/*`, so a `--prune` fetch does not drop the citable PR refs.
- Category: process
- Basilisk status: PARTIAL
- Basilisk evidence: Basilisk's `remote.origin.fetch` holds only the branch refspec. `origin/pr/*` exist from a one-shot fetch (PROCESS:587); a pruning fetch would delete them.
- Suggested Basilisk owner: `agents/records.md` rule plus config
- Confidence: high

**D-60. Citation reachability and dead-path checks**
- Rarog source: `f548432`/`b6099be` (2026-10-09), `check_citations.py`, `3f7a9e1` (truncated SHA prefixes tripped the checker), `e338460` (cite a deleted arm by its recipe); `ledger_commits_2026-09-27.md` bundle
- Finding: every cited commit must stay reachable. Throwaway arms are cited by patch or recipe. A full-history bundle preserved 499 unreachable commits.
- Category: process
- Basilisk status: HAS
- Basilisk evidence: `tools/diag/check_citations.py`, `citation_exceptions.tsv`, CI documents job; `check_roadmap.py validate_local_links`; HISTORY *Preserved commits*
- Suggested Basilisk owner: —
- Confidence: high

**D-61. Ledger split into entries, with the index kept in step**
- Rarog source: HISTORY 2026-10-05; `check_guide.py`
- Finding: an index plus one entry file per experiment. The checker fails when the index and the files disagree.
- Category: process
- Basilisk status: HAS (with stale disposition text)
- Basilisk evidence: `check_roadmap.py validate_ledger` (lines 238+, 642). But `docs/EXPERIMENTS.md:126-127` still read "Prepared, not yet run" (BAS-M10) and "Registered, not yet run" (BAS-M11), while the entries say VOID and COMPLETE (2026-10-05). The checker does not compare disposition text.
- Suggested Basilisk owner: `agents/records.md` M fix
- Confidence: high

**D-62. A tools index**
- Rarog source: architecture review F21; `tools/README.md` (T9)
- Finding: one line per tool, saying what it measures and which procedure or leaf uses it.
- Category: process
- Basilisk status: MISSING
- Basilisk evidence: no `tools/README.md` in Basilisk (`ls tools`). There are 70+ diag tools and 13 `run_*.ps1` drivers.
- Suggested Basilisk owner: M leaf any time
- Confidence: high

**D-63. Comments carry no phase or ledger numbers; history-only comments are deleted**
- Rarog source: architecture review §7, RAR-M51/M53, AGENTS *Changes*
- Finding: 453 retired-number references were found in Rarog's `src/` before the clean-up.
- Category: process
- Basilisk status: UNVERIFIED / PARTIAL
- Basilisk evidence: for example, `src/search_kernel_core.cpp:664,701` carry `// 8.7.6(c)` and `src/attacks.h:23` carries "8.7.10".
- Suggested Basilisk owner: B.8 or C.1 cleanup
- Confidence: medium

**D-64. Tag-driven release workflow**
- Rarog source: `release.yml` (E.3.1, PRs #3/#4, 2026-10-04/05)
- Finding: a `vX.Y.Z` tag validates (tag = version; commit on master or on its patch line; dated CHANGELOG section; GUIDE marks the release), builds nine PGO assets read-only, runs ISA checks, smoke-tests `uci`/`id name`, benches each asset against the declared fingerprint, verifies one fingerprint across the matrix, rechecks SHA256SUMS, and only then publishes from a single write job. A PR is a candidate build, and a dispatch is a rehearsal.
- Category: release
- Basilisk status: HAS
- Basilisk evidence: `.github/workflows/release.yml` (nearly line for line); `tools/diag/release_check.py`; used for 1.10.2 (PR #4)
- Suggested Basilisk owner: —
- Confidence: high

**D-65. The release check must also match the Released baseline row's fingerprint**
- Rarog source: `xtask/src/release.rs:139-151`; 2.5.0's failed first tag run (the fingerprint read from the wrong GUIDE row)
- Finding: `release-check` refuses a Released baseline row whose fingerprint differs from the Development head's.
- Category: release
- Basilisk status: PARTIAL
- Basilisk evidence: `release_check.py:87` (`released_in_guide`) checks the version only. Basilisk's single "Bench fingerprint" row makes the wrong-row defect impossible, but the released row's own count is unchecked.
- Suggested Basilisk owner: small I1
- Confidence: medium

**D-66. CHANGELOG `[Unreleased]` grows as changes land**
- Rarog source: AGENTS *Changes* lines 249-253, CHANGELOG header, HISTORY 2026-10-05
- Finding: a user-visible change (a gate-accepted strength change, an option, a fixed defect, a build or asset change) gets its user-facing entry in the commit that records it.
- Category: release
- Basilisk status: PARTIAL
- Basilisk evidence: PROCESS:619 states it only as Release step 5. No AGENTS or records rule exists. `CHANGELOG.md:15` `[Unreleased]` is empty on HEAD and on `dev`, despite B.2's two accepted Cluster 1 gates and B.10's PST repair (fingerprint 14,978,465 → 8,437,148).
- Suggested Basilisk owner: `agents/records.md` rule plus an M catch-up
- Confidence: high

**D-67. Development reopens on a `-dev` version**
- Rarog source: PROCESS *Release* step 6 (`66afa4e`)
- Finding: the first commit of the next branch to reach master bumps to the next `-dev` version, so a changed engine never calls itself the released version.
- Category: release
- Basilisk status: HAS
- Basilisk evidence: `src/constants.h:7` `1.11.0-dev`; CMake 1.11.0; PROCESS:618
- Suggested Basilisk owner: —
- Confidence: high

**D-68. Patch releases from the line's first release**
- Rarog source: `0c1117e`; PROCESS *Release* step 7
- Finding: land the fix on master first, then cherry-pick it onto a temporary branch from `vX.Y.0`, tag there, and delete the branch.
- Category: release
- Basilisk status: HAS
- Basilisk evidence: PROCESS *Release* "A patch while master holds unreleased work"; `release_check.py line`
- Suggested Basilisk owner: —
- Confidence: high

**D-69. Originality disclosure and licence notices shipped with the binaries**
- Rarog source: PLAN E.3.0 (`abd318e`, 2026-10-23), `analysis/originality_review_2.6.0_2026-10-22.md`, `657ea8e` (scans and similarity screen), README `605edf9`
- Finding: donor material copied unchanged (time-budget constants, Reckless move-ordering and history constants) is named in the README and in the independence boundary. Fathom's MIT notice and the GPL text ship with the assets; `release.yml` "publishes bare executables today". The scans are rerun on the release candidate.
- Category: release
- Basilisk status: MISSING
- Basilisk evidence: no PLAN or GUIDE leaf mentions originality, disclosure or notices. README:229-235 "Acknowledgements" says "inspiration". `release.yml` publishes bare `bundle/basilisk-v*`. The memory note on the 1.10.2 audit (PeSTO PSTs, SF19 time-manager constants) is not in the repository.
- Suggested Basilisk owner: new leaf under E.3 (and before any CCRL or ICGA submission)
- Confidence: high

**D-70. Release-rule reads at LTC and 4T, and an interim release decision**
- Rarog source: RAR-M65, RAR-M66, RAR-M76; PLAN E.3 (2.7.0; 3.0.0 reserved for NNUE)
- Finding: Rarog cut 2.5.0 and 2.6.0 as interim releases by maintainer decision, each licensed by STC, LTC and 4T reads against the previous release.
- Category: release
- Basilisk status: HAS (deliberately differs)
- Basilisk evidence: PLAN §5: "no interim release after B" (maintainer decision 2026-09-28)
- Suggested Basilisk owner: —
- Confidence: high

**D-71. User-facing text is short**
- Rarog source: `726e2f6` (2026-10-22); CHANGELOG header
- Finding: README, changelog and release notes are short and plain, with one rounded strength line.
- Category: release
- Basilisk status: UNVERIFIED
- Basilisk evidence: CHANGELOG 1.10.2 section (CHANGELOG.md:19+) is long, technical prose
- Suggested Basilisk owner: E.3
- Confidence: low

**D-72. Universal binary: deferred design and its traps**
- Rarog source: `universal_binary_2026-09.md` (G.2, optional)
- Finding: Rarog would need a fat binary, because multiversioning loses 4.6%. C symbols resolve first-wins silently, so a baseline tier can ship popcnt. PGO profiles are matched by mangled symbol, so tier metadata must match training. Revival triggers: NNUE, rated-strength loss from asset choice, or advisories failing.
- Category: plan item
- Basilisk status: PLANNED (G.2, PLAN:1818-1821)
- Basilisk evidence: G.2 "optional universal CPU-dispatched binary, designed first"
- Suggested Basilisk owner: G.2 (cite Rarog's record as the design start)
- Confidence: high

**D-73. Engine-protocol and stop-time output need stopped-search tests**
- Rarog source: RAR-P26 (two MultiPV defects; one invisible at fixed depth)
- Finding: a feature that changes stop-time output needs a test of a stopped search.
- Category: lesson
- Basilisk status: UNVERIFIED
- Basilisk evidence: A.8.9 MultiPV is done. `tests/test_engine_threading.cpp` has stop paths; a MultiPV-specific stopped-search assertion was not located.
- Suggested Basilisk owner: D.3.2
- Confidence: low

**D-74. Count from the source, not from an inventory**
- Rarog source: RAR-P24 (the inventory declared 99 parameters; the macro held 110); RAR-M53 (four review premises false; a preservation check per move)
- Finding: counts must be read off the source, and a document restructure needs a preservation check for every move.
- Category: lesson
- Basilisk status: HAS (practice)
- Basilisk evidence: AGENTS *Verification* ("verify mechanically")
- Suggested Basilisk owner: —
- Confidence: medium

**D-75. Evaluation share of search time recorded at the checkpoint**
- Rarog source: RAR-P40 (21.06% against B.9's 24.3%; the prediction of 28% missed in sign)
- Finding: the C.11 checkpoint records evaluation, ordering and other regions as shares of samples.
- Category: instrument
- Basilisk status: PARTIAL
- Basilisk evidence: C.11 (PLAN:1677-1684) and C.12 ("Profile terms first") require profiling, but Basilisk has no profiler (D-13).
- Suggested Basilisk owner: C.11/C.12 via D-13
- Confidence: medium

## Part 2 — Delivery-system comparison

| Element | Rarog | Basilisk |
|---|---|---|
| Trunk model | `master` is the trunk since 2026-10-09 (PROCESS *Branches and pull requests*) | Adopted 2026-10-09 (PROCESS:548-591, PLAN §5). The `dev` Transition exception is still open: `dev` (2026-10-23) has not landed, `origin/master` = `fac5064` (2026-10-08) |
| One short-lived branch per coherent change | Done: `c3-king`, `c4`, `c5`, `c7`, `c9`, `release-2.6.0`, now `d1` (PRs #6–#11) | Rule present. Practice: `b10-pst-orientation` branched from `dev` (merge-base = `dev` `24a68cc`), not from master |
| Squash-only merge | Repository allows squash only (gh API) | Same (gh API: squash only) |
| Fixed squash-message shape | PROCESS step 2 (`125bff2`) | Identical (PROCESS *Delivery* step 3; AGENTS) |
| No AI attribution | AGENTS, `.claude/settings.json` (`a7ea940`) | Same (CLAUDE.md, AGENTS) |
| Branch deleted after merge; commits citable under the PR | Yes; refspec configured (`f209981`) | Rule yes; refspec not configured (one-shot fetch only) |
| Citation reachability check in CI | `check_citations.py` (local and PR) | `check_citations.py` plus CI documents job (`ci.yml:285-288`), which Rarog's CI lacks |
| CI on PRs to master | `ci.yml` (PR plus dispatch) | `ci.yml:22-24` (PR plus dispatch), plus sanitizers, GCC PGO, documents |
| Required status checks on master | None (`required_status_checks: null`) despite PROCESS's "should be" | None |
| Tag-driven release, validation step | `cargo xtask release-check`: tag = `Cargo.toml`, on master or patch line, dated CHANGELOG, Released baseline names the version **at the declared fingerprint** | `release_check.py check`: tag = CMake and `constants.h`, line, dated CHANGELOG, Released baseline names the version; **its fingerprint is not compared** (`release_check.py:87`) |
| Build PGO assets read-only | 9 cells, `contents: read` | 9 cells, `contents: read` |
| ISA verification on assets | All 9 (x86 tiers and arm64 with required `prfm`) | x86 cells only (`verify_isa.py`, tiers portable/avx2/pext); no ARM contract |
| One bench fingerprint equal to the declared one | Per cell, then the collect job | Per cell, then the collect job (identical) |
| Compiler identity in release | Pinned via `rust-toolchain.toml`; `rustc -vV` printed | Linux clang-19 pinned and printed; Windows MSYS2 clang not pinned or printed; macOS AppleClang by runner image |
| Publish | One write job; SHA256SUMS rechecked; `softprops/action-gh-release` | One write job; SHA256SUMS rechecked; `gh release create --verify-tag --latest` |
| Manual-dispatch rehearsal | Yes (PR candidate run is the rehearsal) | Yes (same) |
| CHANGELOG `[Unreleased]` collected as changes land | AGENTS *Changes* rule (lines 249-253) | PROCESS:619 only; `[Unreleased]` empty despite B.2 and B.10 |
| `-dev` reopen after release | Yes (`66afa4e`) | Yes (`1.11.0-dev`) |
| Licence and donor disclosure shipped with release | Planned E.3.0 (2026-10-23) | Absent |
| Track record | Tag-driven releases 2.5.0 (first tag run failed, repaired by PR #4) and 2.6.0 (PR #11) | 1.10.2 via PR #4 and tag (2026-10-08); no PR since |

Verdict: Basilisk has adopted Rarog's delivery system almost line for line, and in places goes further. Its CI on pull requests adds sanitizers, GCC PGO and a documents job with citation reachability. Its `release.yml` reproduces Rarog's validate, build, single-fingerprint and publish flow, including patch lines, rehearsal and the `-dev` reopen. What is missing is the practice, not the machinery:
- the `dev` Transition is overdue: B.2 is closed, but `dev` has not landed by its final squash PR, and new work (B.10) still branches from `dev`;
- `[Unreleased]` is not collected as changes land;
- the PR refspec is not configured;
- neither repository makes the two workflows required checks.

Three release-check gaps remain: the Released baseline row's fingerprint is not compared, the Windows release compiler is not pinned, and there is no ARM ISA contract. Rarog's newest release obligation, originality disclosure with notices shipped beside the assets, has no Basilisk counterpart.

## Part 3 — Tournament findings (Rarog records since 2026-09-20 that include Basilisk)

| Record | Date / host | Conditions | Basilisk version and result | Rarog | Faults and anomalies |
|---|---|---|---|---|---|
| RAR-M63, Colosseum `e91b8e63` (DB re-read) | 2026-09-27/28, 5950X | Gauntlet, 40,000 games, `3+0.03`, 1T, Hash 128, UHO seed 95, concurrency 14, no adjudication | 1.10.1 (held 2994): 476-967-2557 (24.0%, −200) against Rarog 2.5.0-dev; Basilisk in-game 2.46 M nps (DB average) | Rarog dev 3233; +200 ± 10 against Basilisk, the smallest rise over RAR-M57 of any opponent | All 40,000 games natural; no Basilisk fault |
| RAR-M64 | 2026-10-03, 5950X | Gauntlet, 2,400 games, 1T, Hash 128, margin 2,000 ms, seed 42, CLI 0.2.0 | 1.10.0 (held 2993): 29-65-306 (15.4%); B.9 head +296.3; 2.82 M nps (old hand reading, superseded) | 3286.5 ± 15.7 | 0 faults, 0 time losses |
| RAR-M66 | 2026-10-03/04, 5950X | 4T, Hash 512, 3 slots × 4 cores, 2,400 games | 1.10.0: 18-67-315 (12.9%); head +332.2 [+295, +376]; 11.66 M nps (defective hand reading) | Performance 3340.0 | 3 faults, all Houdini 3 at launch; none Basilisk's |
| BAS-M11, Colosseum `f5fb0554` (Basilisk's own record, Rarog engines in the field) | 2026-10-05, 5950X GUI | 4T, Hash 512, concurrency 3, 2,400 games, seed 7102002 | 1.10.1: Houdini 3 59-79-262 (−194), Critter 81-96-223 (−129), Fritz 16 65-82-253 (−177), Rybka 4.1 120-96-184 (−56), Rarog 2.4.0 149-118-133 (+14), Rarog 2.5.0 21-72-307 (−312); 13.3 M nps | Rarog 2.5.0 +312 over Basilisk | 5 Houdini 3 `EngineCrash`; 0 Basilisk faults. **Anomaly:** the DB engine path is `D:\chess\engines\basilisk\basilisk-v1.10.1-windows-x86_64-pext-pgo.exe` (SHA-256 `F7AAEF49…`), not the binary BAS-M11 registered (`tools/test_engines/basilisk-a42-release-pext-pgo.exe`, `751F6DFF…`). The EXPERIMENTS index rows for BAS-M10 and BAS-M11 are stale |
| RAR-M73 | 2026-10-21, 5950X | 1T gauntlet, 2,400 games, CLI 0.3.0 | 1.10.0: 9-44-347 (7.8%); C head +430.3 [+385.5, +487.6]; Basilisk 2.86 M nps, depth 15.82/13 | 3428.8 ± 19.2 | 0 faults |
| RAR-M74 | 2026-10-21, 5950X | 4T, Hash 512 | 1.10.0: 7-48-345 (7.8%); +430.3 [+386.7, +485.7]; 9.74 M nps; Basilisk's 4T−1T shift 0.0 | Performance 3433.7 | 3 Houdini 3 setup crashes; none Basilisk's |
| RAR-P41 | 2026-10-22/23, Surface (Snapdragon X Plus, WoA), GUI | (1) 120+1, 4T, 10 engines, 180 games; (2) `3+0.03`, 1T, 14 engines, stopped at 3,712 of 36,400 | Basilisk +101 against its entered rating in run 2 (native ARM; the gain is an emulation artefact of the field); no per-pair figures | 2.6.0 3481, 2.5.0 3338, 2.4.0 3131 | Run 1: every game natural. Run 2: four engine errors, all Houdini 5.01's. No Basilisk fault reported |
| RAR-P42 | 2026-10-23, MacBook Air M4, GUI | 120+1, 1T, Hash 1024, 12 engines, 264 games, Syzygy 3–5 | Basilisk 1.8.0–1.10.2 native arm64; 1.6–1.7 M nps; no per-engine Basilisk scores recorded | 2.6.0 84.1% | No forfeit, crash or illegal move for any engine |
| RAR-P43 | 2026-10-23, M4 | `3+0.03`, 1T, Hash 128, 26,400 games, 12 engines, 2,000 ms forfeit margin | 1.10.2 and 1.9.3. Rarog 2.6.0 +344 ± 40 over 1.10.2; Rarog 2.4.0 −15.6 ± 27.0 against 1.10.2 and +11.3 ± 27.4 against 1.9.3; 1.10.2 against 1.9.3 +6.9 ± 22.6 | Release gaps reproduce the match host | No forfeit for any engine. **Basilisk 1.9.3's clock went to −22 ms in one game**, hidden by the 2,000 ms margin (it would forfeit under a 20 ms margin) |
| RAR-P44 | 2026-10-23/24, Surface WoA, GUI | `3+0.03`, 1T, Hash 128, 20 engines, stopped at 24,683 of 38,000, overhead 10 ms | 1.10.2: −394.9 ± 20.5 relative to 2.6.0, 26.4%, 2.50 M nps, 18th of 20. 1.9.3: −416.0 ± 21.1, 24.3%, 2.45 M nps, 20th of 20. Roughly level with Rarog 2.4.0 (−385.3) | 2.6.0 3386.3; Stockfish 11 native +91.9 above it | 2 time forfeits, both Houdini (never answered `go`); no Basilisk fault |

Earlier than 2026-09-20, for context: RAR-M57 (09-19), Rarog 2.5.0-dev 74.4% (+186) against Basilisk 1.10.0. RAR-M54 (09-15), Basilisk 1.10.0 −12 ± 41 and 1.9.3 +7 ± 41 against Rarog 2.4.0. RAR-M45 and RAR-M46 (09-11), 1.10.0 +23 ± 20 over 2.4.0 at 1T but −25 at 4T.

Overall:
- Basilisk has had no fault, crash, illegal move or time loss in any of these records, on x86, Windows on ARM or macOS.
- Rarog's match-host pool still holds Basilisk at 1.10.0's rating (2993). Basilisk 1.10.2 has never been read on the match host.
- 1.10.2 is not resolvably stronger than 1.9.3 on either ARM host.
- The one clock anomaly is 1.9.3's −22 ms on the M4.

## Part 4 — Coverage

**Rarog experiment entries.**
- The batch file's 89 IDs (P18–P44 and M12, M13, M17–M76) were all read in full. RAR-M21 to M41 are stubs.
- Unlisted in-scope entries read: M09, M10, M11, M14, M15, M16, P09–P17 (15).
- EXPERIMENTS.md read in full except §§3 and 5 row text: the contents, §§1, 2, 4 (index), 6–10.

**Rarog ledger records** (`ledger_records_2026-09-14.md`). Read: lines 97–116 (M25) and 184–665 (M29–M41, M19 part 2). Skipped as out of scope: lines 1–96 (M21–M24, endgame and labels) and 117–183 (M26–M28, parser and SEE correctness). Both groups are already imported into Basilisk as BAS-X17–X22.

**Rarog analysis files.**
- All 20 listed files were read. Exceptions: `board_benchmark_recipe` was read to line 180 (its embedded base64 and raw rounds skipped); `feature_inventory` to line 70; `analysis/README` header and index structure only; `ledger_commits` head and tail.
- Coordinator additions: `archive/speed_profile_8_12c.md` and `manta_tooling_audit` (partial), `phase4_6_audit` (first 50 lines), `infra_analysis` (headings only; dated 2026-07-13, out of window).

**Rarog process and release.**
- Read: `release.yml` and `ci.yml` in full; `xtask/src/release.rs` by grep; PROCESS *Harness*, *Toolchain*, *Branches*, *Release* in full; AGENTS *Measurement* and the `[Unreleased]` rule; GUIDE (grep); HISTORY lines 1–200; PLAN Phase E in full and the headings of D, F and G; filtered `git log --all` from 2026-10-03; `gh pr list` (11), `gh release list`; branch-protection API.
- Not read: `tools/diag/check_guide.py` internals, the xtask PGO build code, the full text of PLAN F and G, `docs/archive/PLAN-closed-*` (the B.7 and C.12 content was covered through RAR-P27–P35 and the C.12 analyses).

**Basilisk.**
- Read: both workflows; `release_check.py`, `verify_isa.py`, `nps_ab.ps1` (header and CI method), `colosseum.ps1` (grep), the Colosseum pin; PROCESS *Harness*, *Build/NPS*, *Delivery*, *Release*; `agents/measurement.md`; GUIDE *Now*; PLAN B.7–B.9, C.11–C.12, D–G, §§4–5; BAS-M08/M10/M11/M12/P17/X35; the `[Unreleased]` state on HEAD and `dev`; README acknowledgements.
- Source checked: `tt.h`, `attacks.h`/`.cpp`, `search_kernel_core.cpp` prefetch sites, `move_picker_core.cpp`, `eval.cpp` KPK, `board.h` history, `main.cpp`, CMakeLists LTO and PGO.
- Git and GitHub: branches, tags, refspec, PRs, releases, protection.

**Colosseum DB.** Read-only query of the two tournaments created since 2026-09-19 (`e91b8e63`, `f5fb0554`). The DB holds nothing later. The macOS and Windows-on-ARM GUI databases behind RAR-P42–P44 are on other machines and were not accessible.

**Counts.** 104 experiment entries, 24 analysis files (20 listed and 4 archive), 2 workflows per repository, 2 DB tournaments, 75 items.

## Part 5 — Most valuable for Basilisk, ranked

1. Rebuild the NPS read before B.7.2 (D-1, D-2): four builds an arm, a t-interval on per-build medians, the two-step read and the disturbance rule. `nps_ab.ps1`'s bootstrap over readings is the interval Rarog showed is not the uncertainty.
2. Prefetch the child's TT cluster before the make, plus the pick-time prefetch (D-5, D-6): +1.69% and about +0.8% in Rarog, and Basilisk prefetches only after `do_move`.
3. Pull large pages forward from G.1 into B.7.2 (D-8): +2.48% bench and +2.76% in games, exact, and it is the only measured latency win. Re-read 4T scaling afterwards (D-9).
4. Re-pin Colosseum from 0.2.0 to 0.4.0 with a RAR-M70-style cross-check (D-36, D-38): kept engine processes, runner-reported per-engine stats and opponent-fault reading, before B.3's gate.
5. Finish the delivery transition (D-55, D-66, D-59, D-58): land `dev` by its squash PR, branch from `master`, fill `[Unreleased]` for B.2 and B.10, configure the PR refspec, and make CI and Release required checks.
6. Add a profiler with inline-chain attribution before B.7.2 and C.12 (D-13): both leaves require a profile, and Basilisk has none.
7. Pin and print the Windows release compiler, and treat any compiler bump as a pooled speed read (D-29, D-28): an LLVM major cost Rarog 1.6% at the same fingerprint.
8. Add an ARM64 ISA contract with a required `prfm` class (D-27): Basilisk's contract is x86-only.
9. Plan donor and licence disclosure for E.3, with Fathom's MIT notice shipped beside the assets (D-69).
10. Add a 4T pool read to the release rule (D-43): a head-to-head release read cannot see a thread-scaling loss that both versions share.
