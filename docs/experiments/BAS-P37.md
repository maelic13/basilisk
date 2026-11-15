# BAS-P37

**BAS-P37 — the loaded cohort read's null pair and its liveness (B.8.9)**

- **Registration.** 2026-11-15, B.8.9, before either read. Agent-run on the idle
  host (the maintainer left the PC to this session).
- **Question.** Does `tools/cohort_read.py run` read two pools of one source as
  level under load, and does it see a known speed difference? The memory items of
  B.8.8's packet (A1 to A4) are decided by this read when it is admitted (packet
  §4.1); otherwise they fall back to the pooled `bench` read.
- **Instrument.** `tools/cohort_read.py` (this leaf): fourteen engines at once, each
  pinned to one logical CPU of its own physical core (CPUs 4, 6, ..., 30; cores 0
  and 1 left to the harness), seven from each pool, the arms on alternate cores and
  swapped every cycle, each slot's build rotating. Each engine: a fresh process,
  `ucinewgame` once, the first 200 positions of `tools/diag/game_cohort_v1.epd` at
  `go nodes 150000`, Hash 64 (large pages, the builds' default), then uncounted
  later positions until every engine of the cycle has finished. A reading is the
  engine's counted nodes over its summed `go`-to-`bestmove` time. Six cycles; a
  build's cycle value is the mean of its readings in the cycle, its value the
  median over cycles; the estimate and 95% Welch interval are `nps_read.py`'s on
  those medians. A cycle is disturbed by foreign load over 1.6 logical CPUs, a
  foreign engine, harness or build process, or a level more than 1% from its arm's
  median cycle; two are absorbed, three repeat the read.
- **Arms.** The null: `b84-on` and `b84-on-null`, two independent four-build
  final-PGO pools of `60db617` (B.8.4's head, large pages on, `bench 13`
  4,688,537), clang 23.1.5, PEXT. The liveness: `b84-off` (large pages off, BAS-P35's
  other arm, the same tree) against `b84-on`.
- **Order.** The liveness first, then the null, each a separate read.
- **Prediction.** Null: +0.0%, half-width about 0.5%. Liveness: large pages read
  +0.91% [+0.62, +1.20] alone on `bench` (BAS-P35); under load with a 64 MiB table
  per engine the TLB's share grows, so +1.5% to +4%.
- **Verdict rule** (frozen):
  - The liveness passes when `b84-on` over `b84-off` reads above 0 with its lower
    bound above 0 and no repeat.
  - The null passes (the instrument is admitted, packet §4.1) when its delta is
    within ±0.5%, its interval holds 0, its half-width is 1.0% or less, and no
    repeat.
  - Admitted means both pass. Otherwise A1 to A4 are read by the pooled `bench`
    read at the standing rule, and the packet's loaded predictions stay unverified.
