# BAS-E51

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E51 - 6.1.f redone: the KBNK vector reaches every bishop-plus-pawn
family, and the original test could not have seen it** (2026-09-04). The first
6.1.f accounting verified byte-identical records on KQ-K, KR-K and KBB-K and
argued they were unreachable because `g_kbnk_drive` feeds only `kbnk_score`.
The argument is correct, which is exactly why the test was worthless: those
three families were chosen *because* they are provably unreachable, so passing
proved only that the reasoning was self-consistent.

Redone without pre-selection. One binary `81EF7A81...`, the frozen 770-position
cohort, 60,000 nodes, all 21 families, only the UCI vector changed between
arms.

**Exactly five families' per-position records differ**, and the set is
principled rather than arbitrary: KBN-K, KBP-K, KBP-KB, KBP-KN, KBPP-KB. Every
one contains a bishop and a promotable pawn, or is KBN-K itself. That is the
knight-promotion reachability closure -- the only way a KBN-K node can arise in
a tree whose root has no knight. The other sixteen families, including every
pawnless one and KP-K, KPP-K and KP-KP, are byte-identical, which is where the
original claim survives.

| budget | KBN-K | KBP-K | KBP-KB | KBP-KN | KBPP-KB | net |
|---|---|---|---|---|---|---|
| 60,000 | 12 -> 20 | 9 -> 17 | 16 -> 15 | 17 -> 18 | 14 -> 14 | **+16** |
| 200,000 | 11 -> 23 | 19 -> 21 | 21 -> 22 | 19 -> 20 | 18 -> 17 | **+15** |

At 60,000 the churn is 15 positions gained and 8 lost; at 200,000 it is 22
gained and 5 lost. **Every regression is `ply_limit` with `first_discard_ply`
null** -- for instance `EG0573`, KBP-KB at DTZ 21, which legacy mates on ply 53
and the accepted vector does not finish inside the 100-ply cap. The engine
never left a won position in any of them. These are conversion-speed
regressions, not correctness regressions.

*Truth is equal or better at both budgets.* Live discards before ply 80 are
identical position-for-position in every affected family at 60,000, and at
200,000 the accepted vector has one FEWER in KBPP-KB (`EG0763` disappears).
Win-preservation rises in KBN-K 0.9955 to 0.9987 at 60,000 and 0.9935 to
1.0000 at 200,000.

*Verdict: accepted, with the leakage documented rather than denied.* KBN-K at a
game-representative budget converts 23/24 against the legacy 11/24, 12 gained
and none lost. The bishop-pawn families net +3 at 200,000. No new truth failure
appears at either budget. What the original 6.1.f asserted -- that the change is
confined to KBN-K -- is false, and the corrected claim is narrower and testable:
it is confined to families that can reach KBN-K by knight promotion, it is net
positive in all of them together, and its regressions are cap-limited
conversions rather than lost wins.

*Method note for future gates.* A non-regression set chosen by the same
argument that predicts safety cannot test that argument. Pick the set that the
mechanism could plausibly reach, or test everything and let the data name the
affected families.
