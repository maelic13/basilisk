# PLAN 6.3.a — king-to-passed-pawn approach: derivation from Basilisk's own failures

**Date:** 2026-09-03
**Verdict:** the feature is **not justified** by Basilisk's measured truth failures.
**Status:** frozen. 6.3.b and 6.3.c have no candidate to verify or gate.

## What the leaf demanded

> Derive the feature from Basilisk truth failures, not reference constants.

That wording forbids the obvious shortcut. Every strong engine has some
king-proximity-to-passer term, so it would be easy to port one and call it
motivated. 6.3.a instead requires that Basilisk's own measured failures show a
king-approach signature. They do not.

## The measurement had to be repaired first

The evidence for this leaf could not be gathered until `endgame_truth.py` was
fixed. It ended a game the moment the strong side's piece count dropped, which
is sound for KBN-K, where losing a minor is a dead draw, and false for pawn
technique, where giving a pawn to promote another is the winning method.

Over ten pawn families at 200,000 nodes, 216 clean-win roots produced 148 such
aborts and **139 happened before the engine had played a single
non-win-preserving move**. `EG0171`, KPP-K at DTZ 3, aborted at ply 2 because
Black captured a pawn the winning line gives away.

With the rule replaced by tablebase truth, conversion over those families rose
from 156/240 to 219/240 — 65.0% to 91.2%. The original figures measured the
instrument.

## The deficit is real, and it did not shrink

The 6.0.b baseline was re-run under its own registered conditions, both binaries
verified by SHA-256, only the termination rule changed:

| arm | before | after |
|---|---:|---:|
| Basilisk `294a3e2` | 293/480 (61.04%) | **361/480 (75.21%)** |
| Stockfish `dev-20260716-ebcea3ef` | 389/480 (81.04%) | **466/480 (97.08%)** |
| gap | 96 positions (20.0pp) | **105 positions (21.9pp)** |

Both arms were contaminated and both improved, but the reference improved more,
so the endgame deficit that motivates 6.3-6.7 is genuine and marginally larger
than recorded. Nothing here lets the sub-phase off.

## But the deficit has no king-approach shape

For every clean-win root the strong side's most advanced passed pawn was
identified, and the geometry a king-approach term would act on was measured:
`own_dist` (strong king to that pawn, Chebyshev), `foe_dist` (weak king to the
same pawn), and `race = own_dist - foe_dist`, negative when the strong king is
closer.

Aggregated over all 288 roots at 60,000 nodes there is a weak apparent trend —
82.2% conversion at distance 0-1 falling to 56.8% at distance 5 — but it is
non-monotone, recovering to 70.0% at distance 6-7.

**Conditioning on family destroys it.** Comparing, inside each family,
positions where the strong king is closer against those where the enemy king is
closer:

| family | race < 0 | race > 0 | delta |
|---|---:|---:|---:|
| KRP-KB | 100.0% | 75.0% | +25.0pp |
| KNN-KP | 20.0% | 0.0% | +20.0pp |
| KBP-KB | 69.2% | 60.0% | +9.2pp |
| KR-KP | 100.0% | 90.9% | +9.1pp |
| KQ-KP | 100.0% | 100.0% | 0.0pp |
| KBP-KN | 76.5% | 80.0% | -3.5pp |
| KBPP-KB | 50.0% | 57.1% | -7.1pp |
| KRP-KR | 71.4% | 83.3% | -11.9pp |
| KBP-K | 16.7% | 43.8% | -27.1pp |
| KQ-KRP | 50.0% | 85.7% | -35.7pp |

Four families favour the closer king, six favour the further one. Mean delta
**-2.2pp**, median -1.8pp. The sign is not stable and the magnitude is
consistent with noise. The aggregate trend was family composition: the families
whose roots happen to place the king far from the passer are also the families
Basilisk is worst at, for reasons that have nothing to do with king distance.

At the 200,000-node game-representative budget the trend is absent even in
aggregate: conversion by king distance reads 92.9%, 93.8%, 88.6%, 92.5%, 76.2%,
90.5%, and by race 90.5%, 93.1%, 87.9%, 89.1%, 89.5%.

## Where the deficit actually lives

Positions behind the reference after the fix, largest first: KBP-K 15,
KNN-KP 13, KQ-KR 13, KBN-K 12, KQ-KRP 10, KBPP-KB 9, then KRP-KR, KBP-KN and
KBP-KB at 7 each. KPP-K, KP-K, KP-KP, KQ-KP, KQ-K, KR-K and KBB-K are level at
zero.

These are **material-specific technique gaps**, not a shared geometric one. The
KBN-K entry is against the pre-6.1 head and is what 6.1 addressed. KBP-K is
wrong-bishop and rook-pawn logic, already owned by 6.5.c. KNN-KP, KQ-KR and
KQ-KRP are owned by 6.7.a. A general king-approach term would not touch any of
them, and adding one anyway would be importing a reference constant — exactly
what this leaf forbids.

## Residual failures are speed, not geometry

At 200,000 nodes the 21 remaining pawn-family failures are 10 `fifty_move`,
11 `ply_limit` and 1 `insufficient_material`. Only `EG0496` and `EG0497` in
KRP-KR show a real truth loss, with discards at ply 0 and ply 4. The engine
overwhelmingly does not throw these wins; it fails to finish them in time.

## Retry trigger

Reopen only if a future instrument shows a king-approach signature that
**survives conditioning on family** — a consistent sign across families and a
mean within-family delta beyond noise. Aggregate correlation across a mixed
family set is not sufficient evidence and is the specific error this analysis
exists to prevent.
