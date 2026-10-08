# BAS-E14

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E14 — the resign threshold is why the corpus had no mating material**
(2026-08-26, 2 × 300 self-play games, `5.9.11-datagen` binary, 8,000 nodes/move,
Hash 16, SuperGM_4mvs, seed 42; the only variable is adjudication).

BAS-E10 and BAS-E11 both blamed corpus coverage. This measures the mechanism
directly rather than inferring it.

| | standard (resign 600/3) | **none** |
|---|---:|---:|
| games reaching **checkmate** | 4 (**1.3%**) | 187 (**62.3%**) |
| reaching bare-officer material | 75 (25.0%) | 194 (**64.7%**) |
| reaching phase ≤ 2 | 79 (26.3%) | 125 (41.7%) |
| median plies | 116 | 138 |
| ended by adjudication | **209 / 300** | **0** |

**Resign-at-600 truncates the game roughly 22 plies before the material the fit
needs ever appears.** 209 of 300 games never finished. A corpus built this way
cannot contain bare-king endings, so a static fit is free to destroy mate-drive
and endgame behaviour at zero measured cost — which is exactly what BAS-E10
caught in `safety_table` and what BAS-E11 could not otherwise explain.

*Cost of the fix:* ~19% more plies and ~14% more wall time. `datagen.ps1` now
takes `-Adjudication standard|none`; "none" omits the flags entirely rather than
passing disabled ones.

*Caveat, so this is not over-claimed.* Positions immediately before mate are
checks and will still be dropped by the quiet filter — correctly, since a static
eval cannot price a position with a forced tactic pending. What this recovers is
the **approach** to mate: quiet K+Q-v-K and K+R-v-K positions, which is the class
mate-drive and endgame knowledge actually need. It also does **not** by itself
fix the sharp-middlegame gap; that is what the UHO slice is for.
