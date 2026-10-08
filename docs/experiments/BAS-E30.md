# BAS-E30

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E30 — which forced-win endings actually need work, and why KBBK is worse
than KBNK** (2026-08-31). Conversion measured across every bare-king family that
is a theoretical win. Rook-and-pawn endings are excluded: they are not forced
wins, so a conversion percentage there is meaningless and BAS-E27's
eval-versus-result comparison is the right instrument for them.

*Python harness, 100 positions per family, 60k nodes, persistent TT:*

| family | mated | fifty-move | stalemate/other |
|---|---:|---:|---:|
| **`KQ-K`** | **100.0%** | 0% | 0% |
| **`KR-K`** | **100.0%** | 0% | 0% |
| `KBB-K` | 87.0% | 10% | 3% |
| `KBN-K` | 54.0% | 37% | 9% |

**The two most frequent bare-king endings are already perfect.** `KQ-K` (5.88%
of games) and `KR-K` (6.64%) convert 100/100, consistent with BAS-E27 putting
them at 0.57× and 0.28× global loss. **There is no Elo waiting in "all the
endgames"** — only KBNK and KBBK need anything.

*Then the CTest harness (5.9.18) disagreed sharply, and the disagreement is the
finding:*

| family | python (persistent TT) | CTest (fresh TT per move) |
|---|---:|---:|
| `KBB-K` | 87% | **3/12 = 25%** |
| `KBN-K` | 54% | 14/16 = 88% |

Two causes, both worth recording. The harnesses differ because `best_move()`
builds a **fresh transposition table for every move**, where a real game keeps
one across the playout — so the CTest instrument is deliberately harsher and
its numbers are not comparable to the python ones.

**But the ordering inversion is real and is not a harness artifact.** Two
bishops is a far simpler mate than bishop-and-knight, so KBBK converting *worse*
points at something structural: **KBBK has no recogniser at all.** It falls
through to the generic mate-drive, `5 × lk_center + (14 − king_dist) × 4`
— **5 and 4 centipawns per step**, which is the exact sub-pruning-margin defect
BAS-E29 diagnosed in KBNK, an order of magnitude worse. KQK and KRK survive it
only because the material is overwhelming enough that the search finds mates
directly.
