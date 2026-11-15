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

#### RESULT

Idle host, 2026-11-15, CPUs 4 to 30 (even), harness on 0 to 3; foreign load 0.3 to
0.9 logical CPUs every cycle (`tools/results/b89-cohort-live`, `b89-cohort-null`).

| Read | Base arm | Cand arm | Delta | 95% interval | Disturbed cycles | Verdict |
|---|---:|---:|---:|---|---|---|
| liveness, `b84-off` → `b84-on` | 2.0185M | 2.0691M | +2.38% | [+1.89%, +2.87%] | one each, absorbed | PASS |
| null, `b84-on` → `b84-on-null` | 2.0835M | 2.0808M | −0.13% | [−0.35%, +0.10%] | two and one, absorbed | PASS |

- **Disposition: ADMITTED.** The loaded read decides A1 to A4 by the packet's §4.1.
  Its half-width at the null is 0.23%; the builds of one pool spread 0.3%.
- Large pages read +2.38% under load against BAS-P35's +0.91% alone on `bench`:
  the load multiplies a memory item about 2.6 times, as B.8.5 inferred.
- **Instrument change before these reads (deviation, recorded):** a five- and a
  forty-position smoke on `b84-off` and `b84-on`, before this registration's reads,
  timed each search from `go` to `bestmove` in the harness; the harness's reply
  latency ran 4.7% over the engines' own time on average and up to 20%, and two
  cycles of the five-position smoke halved. The reading's time is the engine
  process's own CPU time over its counted searches (`cohort_read.py`'s docstring);
  the wall time is archived beside it. The smokes' archives were deleted; neither
  is a read.
- Cycle levels move 2.4% between cycles in one arm (the arms swap cores and the
  cycle's host); both arms move together, which the concurrent design is for.
