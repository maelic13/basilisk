# A.2.3 — Feature, option and parameter inventory

- State / class: CLOSED (inventory delivered), `R2`
- Owner / date: A.2.3, 2026-09-28
- Decision needed: which knobs B.1 removes behaviour-neutrally, which go to
  B.0 as inert-with-owner candidates, and which evaluation groups C.0/C.1
  inherit as inert or unfired.

**Recommendation.** B.1 removes the four failed-trial mechanisms (capture
futility, SEE-quiet pruning, qsearch quiet checks and the post-LMR history
nudge: 7 coordinates) and the drifted `KBNK Drive` option; B.0 re-adds any
of them only in the donor's form. Every other search coordinate is live. The
main counter-argument is re-work if B.0 wants a mechanism back, but none of
the four is in the donor's form today, and capture futility is not safely
parked: an `LmrBase` tune reactivates it (see *Results*).

## Registered probe (frozen 2026-09-28, before any probe result)

**Instrument.** `tools/diag/run_suite.py` internal pass: `suite_v1.epd` (107
positions), `go depth 14`, Hash 64, one thread, a fresh process per position,
`Diag` on, counters summed by the script's own `info string diag kv` parser.
Binary: `tools/results/a23/basilisk-a23-tune.exe`, SHA-256
`a0137707eb53cbace2fd2b710c751d04eadd52b3d020fb3a20498f4edb94d687`, built from
`aecbd93` with `cmake --preset release-pext -B build/a23-tune -DTUNE=ON
-DCOMP=clang` (no PGO: node counts are PGO-independent); `bench 13` =
**14,978,465**, the development fingerprint. `bench` itself ignores
`setoption` (it builds default `SearchLimits`), which is why the probe runs
through `go`.

**Arms.** Two baselines; then each of the 48 `search_params.h` coordinates
alone at its range minimum and at its range maximum, skipping an endpoint
equal to the default.

**Verdict rule.** An arm *moves the tree* when any summed counter differs from
the baseline, excluding `hist_below_half`, `hist_below_quarter` and
`hist_below_eighth`, which read `HistPruneCoeff` directly and so change without
the search changing. A coordinate is *search-live on the suite* when either
endpoint moves the tree, and *null on the suite* when neither does. A null is
then read against the code: a fixed-depth probe cannot reach time management,
so a TM null says nothing about TM liveness.

**Wire check.** At least one known-live coordinate must move the tree, or the
option path is not proven connected and every null is void.

**Predictions** (confidence in brackets):

1. The two baselines are identical in every counter (0.97).
2. All 10 TM coordinates are null (0.95): fixed depth runs no clock.
3. `QuietSeeCoeff` is null at both endpoints (0.9): its gate
   `depth <= quiet_see_depth` is closed at the default 0. `QuietSeeDepth`
   at 10 moves the tree (0.9).
4. `CapFutCoeff` is null at both endpoints (0.8): the gate
   `lmr_depth < cap_fut_depth` opens at the default 1 only when
   `lmr_depth == 0`, so `cap_fut_coeff * lmr_depth` is always 0 there.
   `CapFutDepth` at 0 moves the tree (0.8), contradicting the code comment
   that the default is 0 and never fires; `CapFutBase` moves it (0.8).
5. `DoubleExtMax` at 200 is null (0.8): the cap does not bind at depth 14.
   At 1 it moves the tree (0.9).
6. `QsearchCheckCap` at 10 and `PostLmrHistScale` at 300 move the tree
   (0.9 each).
7. Every other coordinate (30) moves the tree at one endpoint at least
   (0.85 overall; the likeliest exception is `HistPruneCoeff` at 28000,
   but its minimum should move it).

**Falsifiers.** A baseline pair that differs voids the instrument. A TM arm
that moves the tree means a fixed-depth search reads TM state, which would be
a defect. A predicted-null coordinate that moves the tree is live and cannot
be removed as inert.

*Clerical correction, before any arm ran:* the registration first said 46
coordinates; the table has 48 (the runner's count assertion caught it). The
"30 others" in prediction 7 was already computed from 48.

## Results (after exposure)

Evidence, from `tools/results/a23/probe/` (95 JSON reports; the comparison
rule is in *Artifacts*):

- The baseline pair is identical in all 65 `kv` counters, and the LMR
  accounting identity holds in every arm. Baseline: 48,889,485 nodes
  (interior + qsearch).
- 38 coordinates move the tree at an endpoint, so the option wire is live.
- **Calibration.** Predictions 1, 2, 3, 5 and 6 held. Prediction 7 held for
  all 30, including `HistPruneCoeff` at 28000 (44 counters, +5.2% nodes).
  Prediction 4 held for `CapFutCoeff` and **failed** for `CapFutDepth` at 0
  and for `CapFutBase`: both are null. The miss was reachability, not
  mechanism. `lmr_depth = depth - base_r`, with
  `base_r = floor(0.60 + ln d * ln m / 2.09)` and `m <= 63`, never reaches
  `d`: depth 1 gives 0, and for d >= 2 the maximum stays below d (1.97 at
  depth 2). So `lmr_depth < 1` cannot hold, and capture futility is provably
  dead at the default LMR table.
- **Post-registration check** (exploratory, labelled as such): at
  `LmrBase=150`, `CapFutDepth` 1 against 0 differs in 46 counters
  (41,780,934 against 35,666,902 nodes). An SPSA that moves `LmrBase` to 100
  or more silently reactivates capture futility, which was reverted after an
  SPRT of -2.78 ± 7.50.
- Time management cannot be reached at fixed depth, so its 10 nulls carry no
  information by design. The wire is shown separately: with `TM_Debug`,
  `TmOptMult` 100 against 200 gives `soft_ms` 258 against 516 on the same
  clock (`tools/results/a23/tm_*.log`).

## Search coordinates (48)

Node change at the tested endpoints (min / max), depth 14, 107 positions.

| Coordinate(s) | Default | Probe | Class | Owner / disposition |
|---|---|---|---|---|
| `RfpCoeff`, `RfpImproving` | 160, 72 | -35.8% / +21.0%; +14.1% / -0.5% | live | B.2 |
| `RazorCoeff` | 243 | -12.9% / +1.0% | live | B.2 |
| `NullBase`, `NullEvalDiv` | 3, 192 | +27.9% / -9.9%; -1.7% / +3.3% | live | B.2 |
| `ProbCutMargin` | 189 | +34.7% / +359.6% | live | B.2 |
| `FutilityBase`, `FutilityCoeff` | 180, 128 | +42.6% / +152.0%; +51.1% / +12.7% | live | B.2 |
| `HistPruneCoeff` | 14004 | +11.9% / +5.2% | live; raising it grows the tree, so history pruning is not near-dead | B.2 |
| `SeePruneCoeff` | 73 | +3.2% / +205.4% | live | B.2 |
| `CapFutDepth`, `CapFutBase`, `CapFutCoeff` | 1, 198, 283 | null at 0 / +19.6% at 10; null; null | **dead at the default LMR table** (proof above); live once `LmrBase` >= 100 | B.1 removes; B.0 re-adds in donor form if wanted |
| `QuietSeeDepth`, `QuietSeeCoeff` | 0, 25 | +47.0% at 10; null | inert: `depth <= 0` never holds in the move loop | B.1 removes (the naive form broke KBNK); B.0 decides the donor form |
| `QsearchCheckCap` | 0 | +31.1% at 10 | inert: `> 0` gates the loop | B.1 removes (its seed broke the KBNK CTest; the hcefinal SPSA pinned 0) |
| `SingularMinDepth`, `SingularBetaMult`, `SingularDoubleMargin` | 5, 4, 4 | +41.5% / +2.2%; +55.9% / +49.5%; +25.4% / +32.3% | live | per B.0 |
| `DoubleExtMax` | **16** | -8.7% at 1; null at 200 | live safety bound, never binding on the suite | keep; B.0 decides with the singular cluster |
| `AspirationDelta` | 19 | +11.4% / +20.2% | live | B.5 |
| `LmrBase`, `LmrDivisor` | 60, 209 | +63.1% / -14.5%; -12.3% / +135.7% | live; `LmrBase` also gates capture futility | B.2; no SPSA surface moves `LmrBase` while capture futility is in the tree |
| `LmrHistDiv`, `LmrNonPvAdj`, `LmrCutNodeAdj`, `LmrTtPvAdj`, `LmrNotImprovingAdj`, `LmrTtCapture`, `LmrSingularQuiet` | 5683, 1024, 401, 23, 89, 301, 401 | all live; `LmrTtPvAdj` 23 gives +15.6% at 0 | live | B.2 |
| `PostLmrHistScale` | 0 | +19.9% at 300 | inert: `> 0` gates the update | B.1 removes (a wash, reverted); B.0 decides the donor form |
| `HistBonusQuad/Lin/Max`, `HistMalusQuad/Lin/Max`, `HistTtMoveBonus` | 62, 120, 1863, 62, 143, 1304, 29 | all live | live | B.2 (histories) |
| `TmOptMult`, `TmMaxMult` | 100, 100 | wire shown by `TM_Debug` | live, identity at default | D.1 |
| `TmStability`, `TmScoreDropThr`, `TmScoreDropDiv`, `TmEffortHi`, `TmEffortLo`, `TmEffortHiMult`, `TmEffortLoMult`, `TmInstability` | 60, 30, 100, 80, 25, 80, 120, 35 | unreachable at fixed depth | live by code: one expression in the main thread's soft-limit test | D.1 |

**Premises corrected.** PLAN listed five "exposed but inert" knobs. Four
are inert, capture futility by an unreachable gate rather than by its value.
The fifth, the double-extension cap, has been a live bound at 16 since
BAS-D15, while `search_params.h`'s field notes still say 200 and "provably
inert" and the capture-futility code comment still says its default is 0.
BAS-D15's "behaviour-identical" rested on equal completed depth from
`sweep.py`, which cannot show two trees are the same; this probe's
all-counter identity at 200 now supports the claim, on this suite and depth.

## Options

| Option | Build | Class | Note |
|---|---|---|---|
| `Threads`, `Hash`, `Clear Hash`, `Ponder`, `Move Overhead`, `SyzygyPath`, `SyzygyProbeDepth`, `Syzygy50MoveRule`, `SyzygyProbeLimit` | every | live | the UCI surface |
| `TM_Debug`, `Diag` | advertised in TUNE, parsed in every build | live diagnostics | D.1; A.5.3 |
| `KBNK Drive` | TUNE | **dead, drifted** | Advertises `17000,1000,0,220,0`; the engine compiles `15600,1900,0,460,0`, so a harness sending the advertised default installs a different drive. Its sweep programme is closed and its summary tools left at A.2.1. B.1 removes it; a registered KBNK refit would regenerate it from the constant. |

## `Diag` counters (57)

- All 57 are written and printed, so none is dead.
  `hist_cutoff_updates` and `hist_reward_updates` are written through one
  ternary.
- **16 are prose-only:** `asp_*` (5), `sing_*` (5), `tt_pv_nodes`,
  `tt_stores`, `tt_stores_same_key`, `qs_evasion_nodes`,
  `hist_cutoff_updates` and `hist_reward_updates`. The committed parser reads
  only `kv` lines, so none of them can be summed. A.5.3 owns this.
- The `kv` stream adds 26 keys outside the struct: `eval_calls`, the pawn
  cache and 22 endgame-dispatcher families. Six families are zero on this
  suite, which is sparsity, not death.
- The `hist_below_*` counters observe `HistPruneCoeff` directly; they are
  B.0's history-pruning reachability probe.
- All 73 `diag_` increments run in release builds whether `Diag` is on or
  not; only printing is gated. Nobody has priced that per-node cost. A.5.3
  decides it with PROCESS's NPS method.
- Stale comments: the struct says "47 counters" where the assert says 57,
  and "check_exts must read 0 once 8.6.7 lands" names a retired step whose
  premise BAS-X01 overturned.

## Evaluation parameters (145 groups, 1,178 scalars)

Every group is read in the release translation unit (checked on the
preprocessed `eval.cpp`), so none is dead through a missing consumer. The
122 groups with a nonzero weight are live. Activation on real positions is
not measured, because the Texel tracer `tools/texel/tuner.cpp` does not
compile at `aecbd93`: it reads `Board::pieces`, `side_to_move` and
`halfmove_clock`, which are now private, so `--feature-support` cannot run.
A.4.2 repairs it and C.0 measures activation.

The 23 all-zero groups, read at their use sites (a zero is inert only when
the term vanishes with it):

| Kind | Groups | Owner |
|---|---|---|
| Whole mechanism inert, yet computed on every evaluation | Complexity: `WinOutflanking`, `WinBothFlanks`, `WinInfiltration`, `WinPawnEndgame`, `WinPassed`, `WinTotalPawns`, `WinBias` (`eg += sgn * max(0, -abs(eg))` adds 0). Hanging pieces: `HangPen` (7) | C.5.3; C.4 |
| Inert inputs to the live king-danger sum | `KsOpenFile`, `KsRingPressure`, `KsFlankAttack`, `KsFlankDefense`, `KsPawnlessFlank` | C.3 |
| Inert coordinates of a live family | `SpaceMg`, `SpacePieceMg` (`SpaceBlockedMg` is live) | C.0 assigns: no C cluster names space |
| Fitted zero half of a live MG/EG pair | `PassFreeMg`, `DoubledMg`, `ConnectedEg`, `RookQueenFileEg`, `MinorBehindPawnEg`, `KingProtectorNEg`, `KingProtectorBEg` | C.6, C.7 refits |
| Not inert | `ProxBase`: `(opp_dist - own_dist) * (prox_base + rel_r)` stays live | C.6 |

## Handoffs

- **B.1** removes, behaviour-neutrally at an exact `bench 13`, the 7
  coordinates of capture futility, SEE-quiet pruning, qsearch quiet checks
  and the post-LMR nudge, plus `KBNK Drive`, and corrects the stale comments
  named above as it moves the code. B.0 may keep one only by naming the
  donor form that replaces it.
- **B.0** reads this packet as an input: the live list, `DoubleExtMax` as a
  bound, `HistPruneCoeff` as live, and the rule that no SPSA surface moves
  `LmrBase` while capture futility is in the tree.
- **A.4.2** repairs the Texel tracer and builds the `TEXEL` target in CI so
  it cannot rot again. **A.5.3** makes every counter machine-readable and
  decides the release-build increments. **C.0** measures evaluation
  activation and places space.

## Artifacts

`tools/results/a23/` (ignored) holds:
- the binary named above;
- `probe/*.json` and `probe/*.log` (95 arms);
- `base_d12.*`, `time_d14.*`, `lb150_cf0.*` and `lb150_cf1.*`;
- `tm_100.log` and `tm_200.log`;
- `eval_groups.txt`: per group, its length, nonzero count and sum of
  absolute values, from a throwaway program compiled against
  `src/eval_params.h`;
- `eval_pp.cpp`, the preprocessed release `eval.cpp`.

Arm recipe: `python tools/diag/run_suite.py --engine
tools/results/a23/basilisk-a23-tune.exe --depth 14 --hash 64 --option
<Name>=<Value> --out <arm>.json`. An arm moves the tree when any `kv`
counter other than the three `hist_below_*` differs from `base0`.
