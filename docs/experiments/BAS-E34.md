# BAS-E34

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E34 — 5.9.19, the generic bare-king mate drive** (2026-09-01). The drive
was `5*lk_center + 4*(14 - king_dist)`: **5 and 4 cp/step** against futility and
razoring margins of **128-243**, so the search pruned the king walk before it
paid. Replaced for minor-piece mates by a `kxk_score` override in the shape
BAS-E29 proved on KBNK — Manhattan corner potential (Chebyshev has wide
plateaus with no gradient to follow), an explicit edge term, and weights above
the margins: edge 900, corner 250, kings 300, on a `KNOWN_WIN` base.

| | before | after |
|---|---|---|
| **KBB-K conversion** | **3/12** | **12/12** |
| KQ-K / KR-K | 12/12 | 12/12 |
| KBN-K | 14/16 | 14/16 |
| drawn-ending bias | 22/60 | 22/60 |
| **bench** | 12,709,666 | **12,709,666** |

**The finding is the scoping, not the gain.** The first version overrode every
bare-king mate, Q and R included. It converted KBB-K equally well and cost
**+20.5% bench nodes** (12,709,666 -> 15,315,269) across all positions — a
search-efficiency regression that would plausibly have outweighed the endgame
gain, and one no endgame test would ever have shown. Restricting the override to
minor-piece mates returned bench to the baseline **byte for byte**.

Why the heavy case cost so much and bought nothing: BAS-E30 had already measured
KQ-K and KR-K at **100/100**. Those mates are solved by search inside the
horizon, so they never used the gradient; overriding them only replaced ordinary
scores (~950) with `KNOWN_WIN`-scale ones (~10950), which churned aspiration
windows everywhere a deep line touched a won ending.

*Generalisation.* A recogniser is worth adding only where **search cannot
already solve the class**. "The eval term is too small" and "the eval term
matters" are different claims, and BAS-E30's conversion table is what separates
them. The same test that justified the change for KBB-K refuted it for KQ-K.

*Also confirmed:* two same-coloured bishops cannot mate, so the pair is tested
by square colour, not by count — the old inline gate used `more_than_one()`.
