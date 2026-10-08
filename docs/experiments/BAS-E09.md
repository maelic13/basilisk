# BAS-E09

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E09 — the +31% bench does not buy a proportional depth loss** (2026-08-26,
diagnostic, no Elo claim). Paired depth-at-equal-nodes over the 107-position
`suite_v1.epd`, candidate `41797a7` against pre-bake baseline `8c5d7cc`, both
built plain Release from the same toolchain, **Hash 64 on both arms**.

| nodes/position | base mean depth | cand mean depth | paired Δ (mean) | Δ excl. mate runaways | deeper / shallower / equal |
|---:|---:|---:|---:|---:|---|
| 300,000 | 21.47 | 21.49 | **+0.019** | −0.010 | 27 / 32 / 48 |
| 1,000,000 | 26.29 | 26.12 | **−0.168** | −0.250 | 25 / 37 / 45 |

*Why the bench number misled.* Bench counts nodes to a **fixed depth 13** on a
different 40-position set. Two evaluators that disagree score a position
differently — the candidate returns 251 where the baseline returns 281 — so
aspiration windows, fail-high/low patterns and TT behaviour all diverge. A large
bench-node delta between two *different* evaluators is expected and is not by
itself evidence of a search-efficiency regression. NPS is unchanged (3.23M vs
3.31M), so the extra nodes are not a more expensive evaluator either.

*What the cost actually is.* Roughly **0.17–0.25 ply at 1M nodes**, and
indistinguishable from zero at 300k. That is a small single-digit Elo headwind
into 5.9.6, not the ~2.5× deficit first recorded. It is a real headwind and it
is the right size to state, but it does not on its own predict rejection.

*Method note, carried from BAS-O04.* The mean is reported alongside a
mate-runaway exclusion because BAS-O04's "12.07-ply gap" was a mean dominated by
ten positions running past depth 100. Here the two agree, so the conclusion does
not rest on the choice.

*Standing caution.* Depth at fixed nodes is a coarse instrument: one iteration
costs ~1.9× the previous (BAS-D08), so a 31% node difference is about a third of
an iteration and rounds to zero in most single positions. The median is 0 at
both operating points for that reason; the mean over 107 paired positions is
what carries the signal.
