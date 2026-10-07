# BAS-E08

Two different experiments were registered under this ID before the
ledger was split; both are kept, in ledger order.

<!-- part 1 of 2: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E08 — 5.9.4 joint Texel refit of the enlarged surface** (2026-08-25,
`--tune scalars`, 348 active params, 1,520,109 train / 79,891 holdout rows from
`beast_sf_*`, `--l2 1e-6`, 200 epochs, K = 1.41868).

| | initial | tuned | delta |
|---|---:|---:|---:|
| holdout loss | 0.0703086 | **0.0659483** | **−6.2%** |
| opening (n=29,421) | 0.06477 | 0.05676 | −12.4% |
| early-mid (n=17,117) | 0.08900 | 0.08441 | −5.2% |
| middlegame (n=16,313) | 0.07211 | 0.07038 | −2.4% |
| **endgame** (n=13,932) | 0.05976 | 0.05951 | **−0.4%** |
| deep endgame (n=3,108) | 0.05761 | 0.05689 | −1.2% |

**Ablation — the new structure contributes essentially nothing.** Applying only
the twenty new-term values while leaving the 348 existing parameters at their
shipped values gives holdout **0.0703113** against the baseline **0.0703086** —
marginally *worse*, and inside noise. The entire 6.2% therefore comes from
refitting the pre-existing surface.

*Conditional lesson, and it is not comfortable.* That refit is close to what HCE
cycle 6 already did, and cycle 6 **washed at +1.37 ±5.21 over 8,100 games**. The
distinction this program relied on — that an enlarged surface is not the surface
that washed — is weakened by the ablation: the enlargement measures inert, so
what remains is largely cycle 7. Two recorded results say holdout loss cannot
rescue that read: durable lesson "holdout-MSE-delta does not predict Elo", and
BAS-X02, where Stockfish distillation improved holdout by 4.9% and lost −17.11
Elo in Rarog.

*Two individual terms are suspect.* `SliderOnQueen` fitted to exactly **0**
despite firing on 2,260 of 20,000 positions, and `LongDiagonalBishop` fitted
**negative** where the concept predicts a bonus. Both are the signature BAS-X11
describes of a term whose relations duplicate signal already priced elsewhere —
mobility and `bad_bishop` in this case.

*One clear success.* Splitting `king_protector` at 5.9.2 was justified: the fit
separated the pieces it could not previously distinguish, to `(−1, +4)` for
knights against `(−2, 0)` for bishops. The endgame divergence is the whole point
— a knight near its own king in the endgame is worth something a bishop is not.

*Cost carried into the gate — measured, after a first reading that was wrong.*
Bench moves **11,941,440 → 15,655,764**, +31% nodes at the bench's fixed depth
13, and this was first recorded here as a ~2.5× per-iteration deficit that the
evaluation gain would have to outrun. **BAS-E09 measured it directly and that
reading does not hold.** The real cost at realistic depths is about a quarter of
a ply. See BAS-E09.

*Correctness.* CTest 12/12 including the KBNK/KQK mate canaries — notable, since
this class of change tripped them eight consecutive times historically.

<!-- part 2 of 2: from docs/EXPERIMENTS.md, 5. Evaluation and data experiments -->

| Field | Value |
|---|---|
| ID | BAS-E08 |
| Experiment and conditions | Frozen 770-position Syzygy truth baseline: accepted head `294a3e2` versus Stockfish `dev-20260716-ebcea3ef`, both at 60k nodes, 1T, 16 MB, engine TB disabled and no score adjudication. |
| Result / disposition | **Baseline observation:** Basilisk converted 293/480 clean wins (61.04%) versus 389/480 (81.04%); paired matrix 277 both, 16 Basilisk-only, 112 reference-only, 75 neither. |
| Conditional lesson and retry trigger | The 81.04% is an attained reference result, not a theoretical or empirical ceiling: Basilisk may surpass it, and need not equal it to be accepted. The 405/480 paired union is stretch evidence, not one engine's performance. Largest deficits were KBP-K, KQ-KR, KNN-KP, KBN-K and KQ-KRP. |
| Source | `tools/diag/endgame_ceilings_v1.json`; PLAN 6.0.b–c |
