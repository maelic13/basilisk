# BAS-E44

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E44 - 6.2.a: the 6.1 KBNK change is indistinguishable in play; stopped
undecided at 7,720 games** (2026-09-03). Single-variable A/B isolating 6.1:
candidate `a3078cf` (`DF251C66...`) against baseline `6accfe6` (`E6E555D0...`),
the last revision carrying pre-6.1 KBNK scoring. SPRT elo0=-5 elo1=0
alpha=beta=0.05 on the normalized model, tc=3+0.03, 1 thread, 64 MB,
UHO_Lichess_4852_v1, concurrency 14, opening seed 910337551, **adjudication
NONE**, both benches 12,709,666.

| games | Elo | nElo | LOS | PairsRatio | LLR |
|---:|---|---|---:|---:|---|
| 3,484 | -4.29 +/- 6.06 | -8.17 +/- 11.54 | 8.26% | 0.89 | -0.82 (-27.7%) |
| 7,720 | **-1.40 +/- 4.07** | **-2.66 +/- 7.75** | 25.09% | 0.97 | **-0.05 (-1.7%)** |

Ptnml at the stop: [88, 736, 2236, 719, 81] -- 824 losing pairs against 800
winning, near-symmetric. The interim 3,484-game reading was noise and returned
to the origin exactly as a true zero does. It should not have been read as a
signal; an SPRT 28% of the way to its bound is not evidence of anything.

**The maintainer stopped the run; it did not reach a bound.** What that
licenses is bounded and must not be overstated: at 7,720 games the candidate is
not distinguishable from the baseline, and a regression worse than about -5.5
Elo is excluded at 95%. The interval still spans -5.47 to +2.67 Elo, so a real
regression of two or three Elo remains inside it. This is practical
equivalence, not a demonstrated null, and it is not a pass of the registered
bound.

*The PGO speed confound is ruled out, and it does not flatter the candidate.*
The binaries came from separate PGO runs, so a layout-driven speed difference
could have manufactured Elo with no behavioural cause. A first measurement was
discarded because a maintainer job was running concurrently. Re-measured on an
idle machine with five interleaved `bench 13` repeats per side, so any drift
hits both equally: candidate mean 3,604,125 nps against baseline 3,565,130, or
+1.09% (+1.34% on medians), t = +1.70 and therefore not significant against a
within-engine spread of 1.6-3.0%. The candidate is if anything marginally
faster, so speed cannot explain a negative result. If the ~1% edge is real it
is worth roughly one to two Elo at this time control, which would put the
behavioural effect nearer -3 than -1.4 -- still inside noise, but the
correction runs against the change, not for it.

*This is what BAS-E43 predicted.* KBNK occurs zero times in search trees from
real roots, so a KBNK-only evaluation change has almost no surface on which to
move a 3+0.03 result. Equivalence is the expected outcome. 6.1's value remains
where BAS-E41 measured it: conversion 95 to 144 of 198 when the family is
actually reached on the board.
