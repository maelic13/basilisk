# PLAN 6.4.a — score resolution, saturation and interaction

**Date:** 2026-09-04
**Status:** static half complete; the measurement half is prepared and unrun.
**Method:** source audit and arithmetic only. No engine was run — a maintainer
SPSA job held the machine.

## 1. Saturation: no term collides with the mate band, but only one is guarded

Every mate comparison in the engine uses `MATE_SCORE - MAX_PLY` = **31,872**. A
static evaluation at or above it is ply-adjusted as a mate score on its way
into the TT.

| path | maximum | headroom | bounded by |
|---|---:|---:|---|
| `kbnk_score`, accepted vector | 31,660 | **212** | validator, TUNE builds only |
| `kxk_score`, KBB-K | 16,660 | 15,212 | nothing |
| `kxk_score`, KBBBB-K (triple promotion) | 17,320 | 14,552 | nothing |

`kxk_score` is `KNOWN_WIN + npm + (3-edge)*900 + (6-corner)*250 + (8-kdist)*300`.
Its material term is unbounded above in principle, since promotions can add
bishops, but even four bishops leaves 14,552 points of headroom. It is safe by
a wide margin and needs no guard.

`kbnk_score` is the opposite: 212 points from the band, and the check that
keeps it there lives in `set_kbnk_drive_weights`, which is `#ifdef
BASILISK_TUNE`. **A release build's compiled default is validated by nothing.**
The 6.1.f test exercises the validator, not the constant, so editing the
default to an unsafe vector would pass every test and silently let an
evaluation be stored as a mate score.

*Recommendation, not applied here:* lift `STATIC_MATE_FLOOR` to file scope and
`static_assert` the compiled default's maximum against it, so the guarantee
holds in both build types and at compile time rather than at option-set time.

## 2. Resolution: the drive's gradient does not survive rule-50 damping

`kxk_score` carries an explicit design claim in its comment:

> Every weight clears the razoring margin (243) so the walk survives pruning,
> which is the whole point of the step.

That is true of the raw weights and false of the scores the search actually
sees. `damp_rule50` multiplies the evaluation by `(199 - clock) / 199` **after**
`apply_endgame`, so it scales the override band along with everything else:

| term | step | clock 50 | clock 80 | clock 99 |
|---|---:|---:|---:|---:|
| kbnk diagonal | 1900 | 1422 | 1136 | 954 |
| kbnk king | 460 | 344 | 275 | **231** |
| kxk edge | 900 | 673 | 538 | 452 |
| kxk king | 300 | **224** | **179** | **150** |
| kxk corner | 250 | **187** | **149** | **125** |

Bold entries have fallen below the 243 razoring margin. The kxk king and corner
terms are already under it at clock 50 — the halfway point of the fifty-move
counter, not some pathological extreme.

The mechanism is not a bug in either component. Damping toward zero as the
clock runs is deliberate and correct for rule-50 realism, and the drive weights
were chosen sensibly against the undamped margin. The interaction is what was
never audited: **the gradient a mate drive depends on is weakest exactly when
conversion is most urgent.** Ordering within the class is preserved, because
damping is monotone; what erodes is the margin by which the gradient survives
pruning.

This is a hypothesis with a clear mechanism, not a measured effect. Section 4
registers the experiment.

## 3. Interaction: exact-material terms leak through promotion

BAS-E51 established that the KBNK vector changes results in KBN-K, KBP-K,
KBP-KB, KBP-KN and KBPP-KB — every family holding a bishop and a promotable
pawn. The dispatcher condition is exact material, but a search tree is not
bound by the root's material: a knight promotion manufactures the exact
configuration the term keys on.

The same argument applies to `kxk_score` and was never checked. KBP-K promoting
to a **bishop** yields KBB-K, which `kxk_score` owns. So kxk's weights are
reachable from bishop-pawn families by exactly the same route.

*Rule this establishes for 6.5 and 6.7:* a family term's blast radius is not
its dispatcher condition, it is that condition's **promotion closure**. Any new
family term must state which families can reach it by promotion, and its
non-regression set must include them. Choosing a set that the safety argument
already excludes tests nothing, which is the error 6.1.f had to be redone for.

## 4. Registered measurement, prepared and unrun

The resolution hypothesis is testable on machinery that already exists. The
frozen 6.0.a cohort roots all carry halfmove clock 0. Re-emitting them at
elevated clocks and re-running conversion isolates the damping interaction,
because nothing else about the position changes.

Arms: clock 0 (the existing baseline), clock 50, clock 80. Same binary, same
cohort, same nodes, bare-king mate families only (KQ-K, KR-K, KBB-K, KBN-K),
since those are the ones whose conversion depends on the drive gradient rather
than on material.

Predicted if the hypothesis holds: conversion falls with starting clock faster
than the shortened rule-50 horizon alone accounts for, and the fall is
concentrated in the kxk families, whose steps cross the margin earliest.

Predicted if it does not: conversion falls only in proportion to the reduced
ply budget, equally across families, and the interaction is a curiosity rather
than a defect.

The distinction matters because the fix differs. If damping is the cause, the
override band should be exempted from `damp_rule50` — a won position is won
regardless of the clock, and the fifty-move rule is already modelled by the
harness's own termination. If it is not, nothing should change.
