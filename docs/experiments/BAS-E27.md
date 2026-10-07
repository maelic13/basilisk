# BAS-E27

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E27 — frequency was the wrong target: our endgame evaluation is fine
almost everywhere, and badly wrong in ONE class** (2026-08-31). 5.9.7 ordered
the classes by how often games reach them. This measures where we are actually
**wrong**, by bucketing the 52,632-row holdout by the same canonical classes and
comparing our static evaluation against the real game result. Fitted
K = 1.90906, global loss 0.078806.

| class | n | loss | vs global | mean eval | mean result | bias |
|---|---:|---:|---:|---:|---:|---:|
| **`KBN-K`** | 604 | **0.22558** | **2.86×** | **+8563** | **0.549** | **+0.451** |
| `KPPP-KPP` | 187 | 0.08232 | 1.04× | +219 | 0.840 | −0.121 |
| `KPP-KP` | 248 | 0.06238 | 0.79× | +220 | 0.821 | −0.096 |
| `KR-KP` | 183 | 0.05820 | 0.74× | +351 | 0.669 | +0.155 |
| `KRPP-KRP` | 260 | 0.05293 | **0.67×** | +182 | 0.660 | +0.029 |
| `KRP-KR` | 310 | 0.04084 | **0.52×** | +226 | 0.705 | +0.023 |
| `KR-K` | 601 | 0.02169 | 0.28× | +679 | 0.946 | +0.014 |

**This inverts 5.9.7's conclusion.** The rook endings that dominate by frequency
are **not** where we are wrong — `KRPP-KRP` at **0.67×** and `KRP-KR` at
**0.52×** are *better* than our global average, and `KR-KR` does not even
reach the worst sixteen. Building rook-ending recognisers would have been work
against a non-problem, and frequency alone would have sent us there.

**`KBN-K` is the finding.** We score it **+8563 on average** — verified
directly at **+10,845** and **+11,123** on two textbook positions — and the
side holding bishop and knight scores **0.549**. A theoretically forced win
converts barely better than a coin flip, and the evaluation asserts near-victory
throughout. That is a 2.86× loss ratio and a +0.451 probability bias, both far
outside anything else measured.

*Important nuance for 5.9.8's design.* This is **not a missing recogniser**. We
classify KBNK correctly — the corner drive exists and `apply_endgame` scores it
as won. The failure is **conversion**: technique inside the fifty-move limit.
PLAN's 5.9.8 says implement classification only, because MAN-E05's post-mortem
was grading conversion with no recogniser able to say whether an ending was
winnable. **Our position is the exact opposite of Manta's**, and applying
Manta's remedy unchanged would build recognisers we already have while leaving
the measured defect untouched.

*Secondary, and all minor:* multi-pawn endings are **under**-estimated
(`KPPP-KPP` −0.121, `KPP-KP` −0.096) and `KR-KP` is over-estimated (+0.155),
but every one of these sits at or below global loss.
