# BAS-P21

**BAS-P21 — the universal x86-64 binary with 64-byte function alignment, against the pext asset**

- **Registration.** Registered 2026-10-29, before any reading; PLAN A.15.6.
  Builds agent-run; the reads need the host idle and are run when the
  maintainer frees it: `pwsh tools/run_a156_nps_reads.ps1`.
- **Background.** BAS-P20 read the universal binary −0.58% [−0.82%, −0.33%]
  against the pext asset with byte-identical hot code. The cause, found
  after that read by comparing the images: lld's call-graph sort places
  `operator delete` (`_ZdlPv`, a 16-byte runtime stub) inside the pext tier
  build's hot cluster, between `Board::see_ge` and
  `update_quiet_histories`; in the universal link the three tiers share
  that stub and it lands elsewhere, so every hot function after it moves
  16 bytes back (0 to 48 modulo 64). Replaying each tier's symbol order
  with `/order` cannot place runtime symbols, and GNU ld (Linux) does not
  sort at all, so the repair makes placement irrelevant instead.
- **Candidate** (`4488e13`): `-falign-functions=64` on the universal
  binary's tiers (`BASILISK_ALIGN_FUNCTIONS`, set by the `universal`
  preset). Every function starts on a cache line in any link, so its loops
  keep their alignment. Checked statically on the first build: 554 of 559
  pext-tier functions at 0 mod 64 in the tier build and in the universal
  binary alike (the other five identical and cold); `negamax` the same
  4,713 instructions; every tier and `BASILISK_ISA` 19,470,325; the
  universal ISA contract passes; 3,557,376 bytes (+44 KB).
- **Instrument.** `tools/nps_read.ps1` with the repaired host-load measure
  (`7996e4d`), decided by `tools/diag/nps_read.py`, as in BAS-P20.
- **Builds**, clang 23.1.4, `bench 13` 19,470,325, one revision:
  `a156-pext` (four `build_test.ps1 -Flavor Release` builds, today's pext
  asset, default alignment), and four `build_test.ps1 -Universal` builds,
  giving `a156-universal` and `a156-tier-pext` (their pext tier builds,
  aligned to 64). BAS-P20's pools are not reused: their revision predates
  `a575ab8`, whose launcher change moves code too.
- **Reads**, in order, into `tools/results/a156-nps/<read>/`:
  1. **`hot` (the gate):** base `a156-pext`, cand `a156-universal`,
     `-NoRegression`.
  2. **`place`:** base `a156-tier-pext`, cand `a156-universal`: the relink
     alone, the alignment held equal.
  3. **`align`:** base `a156-pext`, cand `a156-tier-pext`: 64-byte alignment
     itself, on the single-tier build.
- **Question.** With placement made independent of the link, does the
  universal binary match the pext asset?

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **`place`:** 0.0%, 80% interval [−0.3%, +0.3%]. This is the mechanism
  test: with every function on a cache line, the relink should cost
  nothing.
- **`align`:** −0.2%, 80% interval [−1.0%, +0.6%]. Aligning every function
  to 64 bytes spends a little i-cache and could gain or lose against
  today's layout, which is a draw of its own (BAS-P20's −0.58% was such a
  draw, in the other direction).
- **`hot`:** about the sum, −0.2% [−1.0%, +0.6%]; PASS with probability
  about 0.6.
- **Decision rule.** The universal binary is released (A.15.5's cells come
  back from `c81e220`, with the alignment) if `hot` passes the no-regression
  form. If `hot` fails while `place` reads zero, the relink is free and the
  loss is the alignment's: the binary stays unshipped, and A.15.6 closes
  NOT_WORTH_PURSUING with the recommendation to remove the universal-only
  code and keep the fixes made with it. If `place` is not zero, the
  mechanism is still not understood and A.15.6 returns to research.
- **Falsifiers:** `place` outside [−0.5%, +0.5%] refutes "placement is the
  whole cause".
- **Stop rule:** as BAS-P20: step 1, step 2 only on EXTEND; a REPEAT repeats
  that read once into a fresh directory, a second stops for the maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, one
  warm-up reading per build, 120 s under 10% host load, no Rarog or other
  engine, harness or build process.

#### Registration amendment 1 — 2026-10-29, before any exposure

- **What changed.** The three reads become one read of the three pools
  with three contrasts, `hot`, `place` and `align`, each between the same
  two pools, in the same form, decided by the same rule as before
  (`nps_read.ps1 -Pools ... -Contrasts ...`). A cycle reads every build of
  every pool once, the arm order rotating; each contrast has as many
  readings of each of its pools as a two-pool read, so its precision is
  unchanged. The read extends to step 2 when any contrast says EXTEND; a
  contrast decided at step 1 keeps its step-1 verdict. A REPEAT repeats the
  whole read once into a fresh directory; a second stops for the
  maintainer.
- **Warm-up and quiet.** The warm-up is one `bench 8` per build instead of
  one full reading, and it now runs before the quiet wait, so the antivirus
  scan it sets off falls inside the wait, which holds until load has been
  under 10% for 60 s (was 120 s, a margin for the broken load measure
  repaired in `7996e4d`).
- **Why.** The three reads paid three warm-ups and three quiet waits, about
  30 to 65 minutes of idle host; the single read takes about 9 minutes at
  step 1 and 23 if it extends.
- **Exposure so far: none.** A first attempt at `hot` under the old
  launcher was stopped by the maintainer during its first cycle (the host
  was not idle). No cycle completed, nothing was printed or read from its
  partial archive, and it is kept unread in
  `tools/results/a156-nps/hot-stopped/`. The predictions above stand
  unchanged.

#### RESULT — append after exposure

- **Result (agent-run 2026-10-29, 10:23-10:48 UTC, one three-pool read
  under amendment 1, revision `c4ea96a`, pools built at `1cd784b`; every
  cycle undisturbed, host 0.6-1.2%; `tools/results/a156-nps/p21/`).**

  | Contrast | Step 1 (2 cycles) | Six cycles | Verdict |
  |---|---|---|---|
  | `hot`, universal against the pext asset | +0.00% [−0.70%, +0.70%] | −0.29% [−0.73%, +0.16%] | **PASS** (step 1) |
  | `place`, universal against its tier build | −0.14% [−0.65%, +0.37%] | −0.13% [−0.51%, +0.26%] | NO_CHANGE (step 1) |
  | `align`, aligned tier build against the pext asset | +0.14% [−0.54%, +0.82%], EXTEND | −0.16% [−0.56%, +0.24%] | NO_CHANGE (step 2) |

  The read extended for `align`; `hot` and `place` keep their step-1
  verdicts. At six cycles `hot` would also pass step 2's rule (a fail needs
  −0.5% or below with the upper bound under 0).
- **Decision (the registered rule).** `hot` passed, so the universal binary
  is released: A.15.5's universal cells come back from `c81e220`, with the
  64-byte alignment.
- **Calibration.** All three inside their 80% intervals: `place` −0.13%
  against 0.0% [−0.3, +0.3], `align` −0.16% against −0.2% [−1.0, +0.6],
  `hot` −0.29% (six cycles) against −0.2% [−1.0, +0.6]. The falsifier,
  `place` outside ±0.5%, did not fire: with every function on a cache line
  the relink costs nothing measurable, so placement was the whole of
  BAS-P20's loss. What is left of `hot` is the alignment's own cost, about
  the sum of `place` and `align`.
- **Observation, not a test.** Over cycles 3 to 6 both aligned arms fell
  about 0.5% from their step-1 level while the pext asset held; each
  stayed within the 1% disturbance band. It is the reason the six-cycle
  `hot` reads −0.29% where step 1 read zero, and it is worth watching in
  the next read that has an aligned arm.
