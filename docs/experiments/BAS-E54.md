# BAS-E54

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E54 - 6.5.a: the reference's rook-ending scaling ports, but only with a
floor on the discount** (2026-09-07). KRPKR and KRPPKRP draw scaling implemented
in `apply_endgame`, gated by a few bitboard ORs rather than a popcount census
(BAS-E34's lesson). Measured on the 25k holdout's drawn subset with
`tools/diag/endgame_drawn_bias.py`, which reproduces BAS-E32's eight recorded
drawn shares exactly and identified the dataset as `armC_basilisk25k`.

*The baseline is worse than BAS-E32 recorded, and the roadmap's quoted figures
were stale.* At the current head the drawn-subset bias is **+0.302** in KRP-KR
and **+0.262** in KRPP-KRP, against the +0.171 and +0.138 of 2026-08-31, while
the symmetric controls KRP-KRP and KPP-KPP are unchanged at +0.003 and -0.021.
Whatever moved between those dates moved the up-a-pawn classes only.

*Unfloored, a faithful port is disqualifying.* Cost decomposition, one bench
each, every binary hash distinct:

| arm | bench | vs base |
|---|---|---|
| gate only, rules disabled | 12,709,666 | **0.0%** |
| KRPKR rules 1-7 | 12,525,875 | -1.4% |
| **KRPKR rule 8 alone** | **22,750,756** | **+79.0%** |
| KRPPKRP alone | 26,402,512 | +107.7% |

The gate is free. **One rule -- the loosest condition with the most aggressive
scale, 10 of 64 -- is the entire KRPKR explosion**, and KRPPKRP's 9-21 range
costs more still. These flatten the evaluation across large regions of
rook-ending search space; two bench positions lost mate scores outright.

*A floor fixes both problems at once, and the correctness argument is the one
that matters.* Checked against Syzygy on 3,000 random KRPKR positions, the
unfloored draw rules call **18 genuinely WON positions dead draws**. With
`SCALE_FLOOR = 24` that count is **zero**: no rule here can assert a draw it
cannot prove, only discount. Accepted candidate:

| | bench | KRP-KR bias | KRPP-KRP bias |
|---|---|---|---|
| baseline | 12,709,666 | +0.302 | +0.262 |
| **floor 24** | **12,568,898 (-1.1%)** | **+0.242** | **+0.231** |

NPS +1.24% (ABBA, 12 runs each, idle machine), full CTest 12/12.

*The floor was NOT selected by benching floors, and that is deliberate.* That
curve is chaotic -- floor 32 costs +28.5% while 24 and 40 sit within 9% of
baseline -- so its minimum is noise, and picking it would be fitting to the
instrument. 24 is the largest discount that keeps a clear majority of each
rule's effect; the correctness argument above is what justifies having a floor
at all.

*Two bugs found in my own port, both by measurement rather than review.* The
Lucena rules return `SCALE_FACTOR_MAX` in the reference, which is 128 -- twice
normal, an amplification for a won position. Reading them against
`SCALE_NORMAL` turned won rook endings into near-draws. And the first version
of the deterministic test passed with the rules disabled: the KRPP-KRP
threshold was 150 where the position scores 19 with the rule and 53 without.
Both guards were then re-checked by neutering the gate and confirming they
fail.

*Strength gate and disposition.* The maintainer-returned no-adjudication SPRT
used the prepared `rookscale` candidate against `base` at `3+0.03`, 1 thread,
Hash 64, paired `UHO_Lichess_4852_v1.epd`, tablebases off and natural
termination. It accepted H1 for the registered `[-5,+3]` bounds after **6,332
games: 1,641 wins, 1,581 losses, 3,110 draws**, +3.29 +/- 4.61 Elo, +6.11 +/-
8.56 nElo, LOS 91.91%, draw ratio 56.10%, pentanomial
`[67,606,1776,634,83]`, LLR **2.98** against `(-2.94,2.94)`. **Accepted and
retained.** This establishes the registered strength verdict under these
conditions; it does not claim that the residual rook-ending bias is solved.
The returned PGN/log paths were
`tools/results/sprt_rookscale_vs_base_20260907_094516.{pgn,log}`. The path was
first recorded as `results/sprt/_rookscale...`, which does not exist, and the
artifacts were reported unverified for that reason. **They were present all
along and are now verified** (2026-09-07): the run manifest records
`repo_revision 489c33130c`, engineA bench 12,568,898 / SHA-256 `6637B574...`,
engineB bench 12,709,666 / SHA-256 `0AE15B45...`, `elo0=-5 elo1=3
alpha=0.05 beta=0.05 model=normalized`, `tc=3+0.03`, Hash 64 both sides,
Threads 1 both sides, concurrency 14 on 16 physical cores with one logical CPU
per core, book `UHO_Lichess_4852_v1.epd` SHA-256 `7A7F6470...`, opening seed
1089228159, and **adjudication NONE**. The PGN holds 6,334 `[Event ]` tags
against the 6,332 games the SPRT counted, the ordinary two-game overrun of a
concurrent run stopping on its boundary.

*Read the verdict correctly.* H1 at `[-5,+3]` is a decision that the evidence
favours "not a regression" over "a real loss". The measured effect is
**+3.29 +/- 4.61 Elo, an interval that contains zero**, so this is an accepted
non-regression with a positive point estimate, not a demonstrated +3 Elo gain.
The mode used was `simplify` with the upper bound widened to +3.

Retry only if the evaluator/search
surface or rook-ending occurrence materially changes.
