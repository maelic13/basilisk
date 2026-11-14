# BAS-P28

**BAS-P28 — speed at an identical tree from hardware counters, validated on
BAS-P25's patch**

- **Registration.** Registered 2026-11-05, after the pools were built and
  before any counted run; agent; measurement instrument (PLAN B.16.4,
  Triumviratus TM-7).
- **Question.** Does the search thread's cycles per node, read from the
  CPU's counters on one pinned core, find a known speed change at an
  identical tree, and how small a change can it see?
- **Instrument.** `tools/run_b164_pmc.ps1` (elevated) traces `bench 13` with
  xperf's PMC-on-CSWITCH: at every context switch the CPU's retired
  instructions, cycles and branch mispredictions are logged, and
  `tools/diag/pmc_read.py` gives each thread of the engine the difference
  from the previous log on its CPU. The search thread is the one with the
  most cycles; the others are start-up and are reported, not counted. Per
  run: cycles, instructions and branch mispredictions per bench node. Per
  arm: each build's median over three cycles; a contrast is the difference
  of the arms' means in percent of the base with its Welch 95% interval
  over builds (over runs for the null, one binary against its copy).
- **Tool choice.** ETW counters through the Windows Performance Toolkit's
  xperf, which lists InstructionRetired, TotalCycles and BranchMispredictions
  on the 5950X; AMD uProf is not installed. A probe trace (2026-11-05,
  `tools/diag/pmc_probe.ps1`, the master build `D5E67D0A…`, bench 5,361,058)
  paired all 13,241 counter logs with their switches, no negative delta, no
  counter corruption: the search thread ran 9.585 billion cycles in the
  bench's 1.984 s (4.83 GHz), 3,656 instructions, 1,788 cycles and 19.5
  branch mispredictions a node; start-up 2.2% of the engine's cycles.
- **Arms**, Release PEXT final-PGO, clang 23.1.4, `bench 13` 19,470,325,
  built 2026-11-05 by `tools/nps_pool.ps1 -Revision` as four-build pools:
  `p25-base` (`0680393`, `tools/test_engines/b164-p25-base.pool.json`) and
  `p25-cand` (`fcb3819`, `b164-p25-cand.pool.json`); `copy`, a byte-identical
  copy of `p25-base`'s first build. The tree is identical: BAS-P25 removed
  computations whose weight was zero.
- **Reference.** BAS-P25's pooled NPS read: +2.35% [+2.01%, +2.70%], that is
  −2.30% [−2.63%, −1.97%] in time per node.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- `null` (the copy against its source, runs): cycles per node within
  ±0.3% with an interval holding 0 (probability 0.9); instructions per node
  equal to 0.01% (0.9).
- `p25`: cycles per node −2.3%, its interval excluding 0 (0.75);
  instructions per node 1–4% lower (0.6); branch mispredictions per node
  within ±1% (0.6).
- **Acceptance rule.** The instrument is accepted when (a) the null's
  cycles-per-node interval holds 0 with a half-width of at most 0.5%, and
  (b) `p25`'s interval lies below 0 and overlaps BAS-P25's
  [−2.63%, −1.97%]. Otherwise the leaf returns to research with the read.
- **Stop rule.** One read; a read whose host check fails before or after is
  repeated whole.
- **Conditions.** An idle host, elevated, CPU = the first game core, about
  ten minutes; archive `tools/results/b164-pmc/` (`runs.jsonl`,
  `report.json`, `host.json`).

**Scheduled 2026-11-05 (maintainer):** the read runs as B.15.4's first
step, the first leaf that reads the instrument, not with B.16.4. Its
prediction stays frozen and unexposed. If the pools are gone by then they
are rebuilt from the same revisions with the same recipe; their hashes
change, and the read records the new pool files.

#### RESULT — append after exposure

Read 2026-11-14 by the maintainer, elevated, CPU 2, host at 0.3% before and after
(`tools/results/b164-pmc/`), 27 runs of 19,470,325 nodes, start-up 0.75% of the
engine's cycles.

| Contrast | Cycles per node | Instructions per node | Branch misses per node |
|---|---|---|---|
| `p25` (builds) | −2.233% [−2.796%, −1.671%] | −2.623% [−2.726%, −2.520%] | −1.487% [−1.940%, −1.034%] |
| `null` (runs) | +0.096% [−0.666%, +0.858%] | −0.053% [−0.199%, +0.093%] | −0.240% [−0.897%, +0.416%] |

- Diagnostics (not the verdict): the p25 arms' builds read 1,425–1,431 and 1,388–1,401
  cycles per node; the copy reads 1,429.5, inside the base's builds.
- Deciding result: (b) holds: p25's interval lies below 0 and overlaps BAS-P25's
  [−2.63%, −1.97%]. (a) fails: the null holds 0, but its half-width is 0.76%, above the
  0.5% the rule allows.
- Disposition: **NOT ACCEPTED** by the frozen rule. The null is three runs of one
  binary against three of another, and its t-interval on two degrees of freedom is
  wider than the instrument's real noise suggests: the copy sits inside the base's
  four builds. The instrument goes back to research with this read (PLAN B.7.9).

#### PREDICTION CALIBRATION — preserve the original prediction above

- Original prediction summary: null within ±0.3% holding 0, instructions equal to
  0.01%; p25 −2.3% excluding 0, instructions 1–4% lower, branch misses within ±1%.
- Observed result: cycles as predicted on both contrasts (+0.10% and −2.23%), and
  p25's instructions −2.62% inside 1–4%. Missed: the null's instructions read −0.05%,
  not 0.01% (a different path through the same code at start-up is the likely cause),
  p25's branch misses fell 1.5%, and the null's interval was wider than the rule. The
  miss is in the instrument's design (a three-run null), not in the counters.
