# BAS-E21

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E21 — 5.9.14 ACCEPTED: +2.64 Elo, the first gain of Phase 5.9**
(2026-08-28). `basilisk-5914-ks-pext-pgo` (`a534b1ab87`, bench 12,844,350)
against `basilisk-5911-base-pext-pgo` (`8e8f681ddb`, bench 11,941,440).
`3+0.03`, 1T, Hash 64 both arms, UHO, concurrency 14.

**Elo +2.64 ±2.05, nElo +4.05 ±3.15, LOS 99.42%, LLR 2.97 → H1 accepted** over
46,864 games in 8h44m. Ptnml [1036, 5599, 9849, 5869, 1079], PairsRatio 1.05.

*The LLR drifted rather than wandered, and that was the tell throughout.*

| games | Elo | LLR |
|---:|---:|---:|
| 10,636 | +2.74 ±4.30 | +0.72 |
| 22,998 | +2.99 ±2.93 | +1.76 |
| 46,864 | +2.64 ±2.05 | **+2.97 → H1** |

The point estimate held across a 4× increase in N while the interval tightened
by half. Contrast arm C, which sat at LLR +0.03 for 44,800 games: a hovering LLR
means neutral, a drifting one means real, and the distinction was visible long
before either finished.

**Time-forfeit warning investigated and cleared.** The harness flagged 9 lines
reporting losses on time and said to investigate before trusting the result.
Seven actual forfeits: **candidate 6, baseline 1**.

Two findings, both of which clear it:

1. **They are clustered, not systematic.** Every forfeit falls between games
   10,700 and 15,138 — a 4,438-game span, **9.5% of the run, containing 100% of
   the forfeits**, with none before and none after. Under a uniform rate the
   probability of that is ≈ 0.095⁷ ≈ **7×10⁻⁸**. This is a transient host
   condition, not a property of the engine.
2. **They penalise the CANDIDATE, so the result is conservative.** Removing all
   seven moves the estimate from **+2.639 to +2.713** Elo — the forfeits cost
   the winning side 0.074 Elo. A defect that biases *against* the accepted arm
   cannot manufacture the pass.

*Watch item, not a blocker.* The candidate searches ~7.6% more nodes per depth,
so under host load it may sit marginally closer to the clock than the baseline.
Worth re-checking if forfeits appear in a future run of this binary; nothing here
supports it as a real effect.

**What this result is, and what it is not.**

It is the **only accepted change in Phase 5.9**, and it came from the **capped
non-linear king-danger funnel** — precisely where BAS-E07 located the reference's
advantage, and precisely the surface a linear Texel fit structurally cannot
reach. Both halves of that prediction held.

It is **not** a new feature. Nothing was added. The gain came from two
corrections: fixing the corpus (5.9.11) and re-running a fit that had been
discarded for the wrong reason (5.9.5 → 5.9.14). The candidate had existed since
5.9.5 and was blocked by a canary failure caused by corpus blindness, not by
evidence against it.

*Conditional lesson.* A candidate withdrawn because it broke a canary deserves
re-examination once the canary's cause is understood. 5.9.5's withdrawal was
correct on the evidence available; keeping it as an open retry rather than a
closed rejection is what made this gain reachable. Retry trigger: any candidate
withdrawn for a reason later shown to be an artifact of its inputs.
