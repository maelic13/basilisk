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

#### RESULT — append after exposure
