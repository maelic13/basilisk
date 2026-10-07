# BAS-E39

<!-- part 1 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E39 — 6.1.c upper-range screen selects `15600,1750,0,340,0`; the plateau,
not the peak, is the finding** (2026-09-03). Fifteen arms, same 60 frozen
positions, 60,000 nodes/move, 100 plies, 30 independent one-thread workers,
engine tablebases disabled, no score adjudication. Both registered controls
reproduced exactly, so the run is valid.

| vector (`base,D,E,K,N`) | converted | paired gain/loss | discards | live discard < ply 80 |
|---|---:|---:|---:|---:|
| legacy `15600,800,900,220,220` | 26/60 | — | 10 | 0 |
| control `15600,1450,0,300,0` | 47/60 | 24/3 | 5 | **1** (KBNK0039, ply 0) |
| `15600,1650,0,460,0` | 40/60 | 20/6 | 4 | **1** (KBNK0023, ply 6) |
| **`15600,1750,0,340,0`** | **51/60** | **27/2** | **3** | 0 |
| `15600,1900,0,460,0` | 46/60 | 26/6 | 2 | 0 |

The truth veto removed exactly two arms and no arm produced a hard anomaly.
Among survivors the selected vector also posts the best admissible DTZ progress
(0.6192), the best clean-win preservation among the high-conversion arms
(99.8448%), one stalemate against the legacy seven, eight fifty-move draws
against the legacy twenty-seven — the next best arm reaches only fourteen — and
it solves `KBNK0039` in 43 plies, faster than any other variant. Engine SHA-256
`34AB7B682425020D8BBD97C80924BF2140119D127A57532811D09FDC5D2963B8`; summary
SHA-256 `12D7B76C69662C1B91A01AAE02E4CD6FEAD4792C30DB73762EA083FE7E6D4B59`.

**The honest reading is the plateau, not the peak.** Paired against legacy the
winner is overwhelming (z about +4.6). Paired against its own neighbours it is
not separated at all: +1.61 versus `d1750-k460`, +1.70 versus `d1850-k340`,
+1.21 versus `d1900-k460`. The king-340 row is non-monotone across the diagonal
axis (37, 33, 41, **51**, 44, 40), a noise signature rather than a peak. These
60 positions have now selected across three rounds and 42 arms, so 51/60 is a
winner's-curse point estimate and the held-out figure should be expected to
fall. What 6.1.c establishes is that the diagonal-dominant region beats the
legacy vector decisively; it does not establish that (1750, 340) is its optimum.
6.1.e therefore decides on held-out positions 61-198 and carries
`15600,1900,0,460,0` as a second arm so a noisy peak can be told apart from the
plateau level.

<!-- part 2 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E39 removes their motivation.** The flat-maximum diagnosis was right, but
the remedy was never a bishop term: steepening the diagonal gradient and
deleting the competing edge and knight pulls fixed most of the stuckness with
the bishop still absent from the potential. Fifty-move draws fell from 27/60 to
8/60 and stalemates from 7 to 1. A bishop feature would now be paying its known
costs — capture exposure for Arm B, stalemate-adjacent confinement competing
with the corner drive for Arm C — against a much smaller residual.

*Retry triggers, both narrow and falsifiable.* **Arm B** may be retried only if
an instrument run against the accepted vector shows bishop shuffling is still
the dominant residual failure mode, and the candidate is gated on zero clean-win
loss and zero piece loss before ply 10 — the row that actually refuted it, not
the conversion row. **Arm C** may be retried only if the stalemate-adjacency
hypothesis is tested directly rather than assumed, the term is shown to
reinforce rather than compete with the corner drive, and it clears the
deterministic KBN-K floor that it previously broke. Absent either trigger, both
stay closed; a retry is a new ID with its own manifest.


**Missing and measured NOT to matter:** `KPKP` — our `KP-KP` and `KPP-KPP`
predictions are already accurate on the drawn subset. `KRKB`/`KRKN`/`KQKR`/
`KQKP`/`KNNKP` are each below 2% of games and are not proposed.
