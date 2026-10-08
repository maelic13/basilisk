# BAS-E32

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E32 — CORRECTION to BAS-E27: the per-class mean hid a systematic error
on the drawn subset** (2026-08-31). BAS-E27 compared our evaluation against the
game result per endgame class, found the rook endings *better* than global loss,
and concluded they needed nothing. **That conclusion was wrong, and the method
was the reason.**

A scaling function exists to recognise that a materially-winning position is in
fact **drawn**. A per-class mean cannot see that: if a class is 40% decisive and
we score those correctly, the mean looks healthy while every drawn position is
called a win. Splitting each class by actual game result:

| class | drawn share | we predicted | bias | reference has? |
|---|---:|---:|---:|---|
| **`KBN-K`** | 90.2% | **1.000** | **+0.500** | `KBNK` |
| `KRPP-KR` | 25.1% | 0.848 | +0.348 | — |
| **`KR-KP`** | 66.1% | **0.780** | **+0.280** | **`KRKP`** |
| **`KRP-KR`** | 59.0% | **0.671** | **+0.171** | **`KRPKR`** |
| `KBPP-KBP` | 84.7% | 0.639 | +0.139 | `KBPKB` family |
| **`KRPP-KRP`** | 61.9% | **0.638** | **+0.138** | **`KRPPKRP`** |
| `KRP-KRP` | 90.1% | 0.498 | ~0 | — |
| `KPP-KPP` | 65.7% | 0.489 | ~0 | — |

**We systematically score drawn rook endings as won**, and the classes where we
are wrong are precisely the ones the reference implements scaling functions for.
Where material is symmetric — `KRP-KRP`, `KPP-KPP` — we are accurate. It is
the **up-a-pawn** cases that fail: our evaluation sees +1 pawn and has no notion
that Philidor exists.

*Recorded as a method lesson, because it is the second time in this phase.*
BAS-E27 replaced a frequency ordering with an error ordering and that was an
improvement; it then used a **mean** where the quantity of interest lived in a
**subset**. Frequency → mean error → error on the sub-population the mechanism
targets. When evaluating whether a *recogniser* is needed, measure the
population that recogniser would fire on.
