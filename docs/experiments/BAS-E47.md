# BAS-E47

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E47 - the endgame instrument aborted correct pawn play; 6.0.b corrected**
(2026-09-03). `endgame_truth.py` ended a game whenever the strong side's piece
count fell below its start value. Sound for KBN-K, where losing a minor is a
dead draw; false for pawn technique, where giving a pawn to promote another is
the winning method. Over ten pawn families at 200,000 nodes, 216 clean-win
roots produced 148 such aborts and **139 occurred before the engine had played
a single non-win-preserving move**. `EG0171`, KPP-K at DTZ 3, aborted at ply 2
because Black captured a pawn the winning line gives away.

The rule now records `shed_material_ply` and stops nothing; the tablebase
decides whether a win was thrown, which the harness already tracked as
`first_discard_ply`. Bare-king families are unaffected because hanging the
bishop in KBN-K leaves K+N versus K, which the insufficient-material test
catches on the same ply.

*No prior KBNK result moves, by construction rather than by assumption.* Every
KBNK artifact -- kbnk-upper-6.1.c, kbnk-holdout-6.1.e, kbnk-budget-200k and
-600k -- contains zero `material_lost` outcomes. BAS-E39, BAS-E41 and BAS-E45
stand unchanged.

6.0.b was re-run under its own registered conditions, both binaries verified by
SHA-256, only the termination rule changed:

| arm | before | after |
|---|---:|---:|
| Basilisk `294a3e2` | 293/480 (61.04%) | **361/480 (75.21%)** |
| Stockfish `dev-20260716-ebcea3ef` | 389/480 (81.04%) | **466/480 (97.08%)** |
| gap | 96 (20.0pp) | **105 (21.9pp)** |

Both arms were contaminated; both improved; the reference improved more. The
endgame deficit motivating the remaining Phase-6 work is real and marginally
larger than recorded,
so nothing about the sub-phase is excused. Downstream consequence to check
before reuse: 6.0.c's frozen ceilings were derived from the contaminated
reference arm, whose conversion was 389/480 and is now 466/480.
