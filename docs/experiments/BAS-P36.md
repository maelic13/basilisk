# BAS-P36

**BAS-P36 — the four-thread pooled NPS read's null pair (B.8.5)**

- **Registration.** 2026-11-15, B.8.5, before the read. Agent-run on the idle host.
- **Question.** Does `tools/nps_read.ps1 -Threads 4` read two pools of one source as
  level? An instrument that cannot pass its null cannot read a four-thread effect
  (PROCESS *Pooled NPS, validated on a self pair first*).
- **Instrument.** `nps_read.ps1` (`586e3bf`) with `-Threads 4`: each reading is
  `bench 13 3 4`, pinned to the last four physical cores, one logical CPU each
  (CPUs 24, 26, 28 and 30, one CCD; BAS-X41's placement), at High priority; the
  reading's value is its best run. The node count varies from run to run (each is
  archived); NPS is the bench's own exact merged total over its time.
- **Arms.** Two independent four-build final-PGO pools of `60db617` (B.8.4's head,
  large pages on), clang 23.1.5, PEXT: `b84-on` (BAS-P35's) and `b84-on-null`
  (`tools/nps_pool.ps1 -Revision 60db617 -ArmOption LARGE_PAGES -Arm On`).
- **Why this workload.** Measured before (B.8.5): `bench 13` at four pinned threads
  read 1.25% run to run over six runs of one build; `bench 16` read 4.97%, falling
  10.19M to 9.09M across consecutive runs as the clock settled. Interleaved cycles
  put that drift on every arm alike.
- **Method.** The gain form's decision (`tools/diag/nps_read.py`): step 1, two
  cycles; step 2, six, if it extends.
- **Verdict rule** (frozen): the instrument passes when the contrast is not
  ACCEPT, the six-cycle 95% interval covers 0 if the read extends, and no cycle is
  disturbed past the medians' absorption. A pass makes `-Threads 4` B.8.9's
  four-thread instrument at the half-width this null reads; an ACCEPT, or an
  interval that excludes 0, fails it, and B.8.9 reads one thread only until a
  workload passes.

#### RESULT

Idle host, 2026-11-15. Two step-1 reads, the second the method's repeat
(`tools/results/b85-nps4t-null`, `tools/results/b85-nps4t-null-repeat`), CPUs 24,
26, 28 and 30, host at 5.4% or less.

| Read | `b84-on` cycle levels | `b84-on-null` cycle levels | Delta | 95% interval | Decision |
|---|---|---|---:|---|---|
| step 1 | 9.663M, 9.461M | 9.637M, 9.440M | −0.24% | [−2.52%, +2.03%] | REPEAT_STEP_1, both arms disturbed |
| its repeat | 9.654M, 9.755M | 9.560M, 9.626M | −1.15% | [−3.25%, +0.95%] | REPEAT_STEP_1, `b84-on` disturbed |

- **Disposition: FAILED, by the frozen rule** (cycles disturbed past absorption in
  both reads). The instrument is not B.8.9's: B.8.9 reads one thread, and B.8.8
  decides whether a four-thread read is worth a second design.
- **Why it fails, read from the archives:** an arm's two cycle levels moved 2.1% and
  then 1.05% at four threads, against the method's 1% (the cores' clock settles
  under a sustained four-core load; the same drift took `bench 16` from 10.19M to
  9.09M across six runs). And the four builds of one source spread 9.40M to 9.73M
  and 9.52M to 9.83M, about 2.7% (one thread's spread is about 0.4%), so even an
  undisturbed six-cycle read would carry an interval near ±1%, twice the +0.5%
  floor's width. Four-thread node counts differ from run to run (10.3M to 13.8M per
  `bench 13`), which the decision reports, as expected.
- **What a second design would need:** the clock held steady (a heat soak before
  each cycle, or the boost disabled, a host setting that is the maintainer's), and
  more builds per arm or longer runs to shrink the build spread.
