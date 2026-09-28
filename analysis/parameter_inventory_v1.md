# A.2.3 — Feature, option and parameter inventory

- State / class: RESEARCH, `R2`
- Owner / date: A.2.3, 2026-09-28
- Decision needed: which knobs B.1 removes behaviour-neutrally, which go to
  B.0 as inert-with-owner candidates, and which evaluation groups C.0/C.1
  inherit as inert or unfired.

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

**Arms.** Two baselines; then each of the 46 `search_params.h` coordinates
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
