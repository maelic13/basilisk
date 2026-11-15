# BAS-P48

**BAS-P48 — the sort's insertion pass by rank (AVX2) (B.8.9, item B4)**

- **Registration.** 2026-11-15, B.8.9, before the read. Agent-run on the idle host.
- **Change.** after the partition pass, the prefix of length `L` (4 to 64) is placed by rank, the count of greater scores plus equal scores at an earlier index (AVX2, eight a compare, padding `INT_MIN`), applied as an in-place permutation; elsewhere and without AVX2 the insertion pass. The test compares random lists with ties, `L` 0 to 80, against the one-pass insertion order; the verify build agrees on both fingerprints; `next()`'s frame 1,368 bytes (`fc45d32`).
- **Prediction (packet §8, frozen).** Bench +0.8% [+0.2, +1.5] (80%); P(kept) 0.65. Falsifier with B3: both under +0.2% means H3 is wrong (B3 read +0.81%, so it cannot fire).
- **Arms.** `b89-b3` (the head before B4, with B1, B2 and B3: full LTO with the inline threshold and
  ext-tsp, A1 to A4 reverted, and the items kept since) against `b89-b4`, four
  final-PGO builds of the head with B4, PEXT, clang 23.1.5, large pages on.
- **Read.** The pooled `bench` read (`nps_read.ps1`), gain form; step 2 when step 1
  extends.
- **Verdict rule (standing method; packet §4.4, D-17, frozen).** It adds state, so it clears its own bar or is not kept: kept when the pooled gain contrast ACCEPTs (+0.9% at step 1, or +0.5% with the lower bound above 0 at step 2); otherwise reverted.

#### RESULT

Idle host, 2026-11-15. Exactness before the read as BAS-P47's (the verify build
agreeing at every node). Archives `tools/results/b89-b4-bench`, `b89-b4-r2-bench`.

| Read | Delta | 95% interval | Verdict |
|---|---:|---|---|
| step 1 | −1.58% | [−2.75%, −0.41%] | REPEAT_STEP_1: both arms' cycles moved about 1.3% (host drift) |
| step 1, repeated | −1.57% | [−1.95%, −1.19%] | NO_CHANGE |

- **Disposition: REVERTED** (an item that adds a vector path clears its own bar).
- Prediction +0.8% [+0.2, +1.5]: missed by 2.4 points. The first version spilled its
  copy of the prefix into a `memcpy` the hot-path scan refused; the read version
  permutes in place. The likely cause is the work's shape: the rank pass costs
  L²/8 compares where the insertion pass costs about 28 shifts on 7.5 moves, and it
  also serves every capture generation (no limit, so the whole list is the prefix).
  The packet's falsifier for H3 needs B3 and B4 both under +0.2%; B3 read +0.81%.
