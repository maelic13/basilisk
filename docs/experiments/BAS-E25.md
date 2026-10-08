# BAS-E25

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E25 — term removal ACCEPTED as a non-regression; Phase 5.9 closes**
(2026-08-29). `5912-slim` (`eb717a7033`, bench 13,981,020) against `5912-full`
(`e7e6b61211`, bench 20,005,943). `-Mode simplify`, H0 elo ≤ −5, H1 elo ≥ 0.

**Elo +0.49 ±2.96, nElo +0.76 ±4.59, LLR 2.97 → H1 accepted** in 21,990 games /
4h06m. Ptnml [470, 2682, 4661, 2711, 471], PairsRatio 1.01.

The eight refuted terms are gone with no measurable cost. The LLR drifted
steadily (+0.07 → +1.22 → +2.97) rather than hovering, and the point estimate
moved *toward* zero as the interval halved — the profile of a genuinely neutral
change.

*Forfeit note, and it points the other way this time.* One game (4111) was lost
on time by **`5912-full`, the baseline** — so it handed the candidate a free
win rather than penalising it. Worth ~0.03 Elo on one game in 21,990, and the
verdict holds without it, but unlike BAS-E21 this forfeit is **not**
conservative and should not be dismissed with the same reasoning.
