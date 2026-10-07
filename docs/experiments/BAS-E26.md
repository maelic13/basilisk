# BAS-E26

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E26 — 5.9.7 endgame recogniser inventory: rook endings dominate, and
PLAN's own ordering under-ranked them** (2026-08-31). Full table in
`analysis/endgame_inventory_v1.md`. 20,000 games from `armC_basilisk25k.pgn`
(25k nodes, **adjudication off**, so endings are played out), counting games
that reach a class at least once with ≤7 pieces.

| class | % of games | covered? |
|---|---:|---|
| **`KRPP-KRP`** | **11.32%** | **no** |
| **`KRP-KR`** | **9.09%** | **no** |
| `KR-K` | 6.64% | mate-drive |
| **`KRP-KRP`** | **6.50%** | **no** |
| **`KR-KR`** | **6.11%** | **no** |
| **`KRPP-KR`** | **5.62%** | **no** |
| `KP-K` | 4.63% | KPK bitbase |
| **`KPP-KP`** | **4.04%** | **no** |
| **`KPPP-KPP`** | **3.42%** | **no** |
| **`KBPP-KBP`** | **2.74%** | partial |

**Five of the top seven classes are rook endings, and we recognise none of
them.** No rook-ending scaling, no Philidor/Lucena notion, nothing beyond the
generic rook-behind-passer positional term.

*PLAN's ordering was close but not right.* It named `KRPKR` first — correct
as far as it goes — but **`KRPP-KRP` is more frequent still**, and the drawn
**`KR-KR` at 6.11%** was not on its list at all. Measuring instead of assuming
changed the target.

Second family: **multi-pawn endings** (`KPP-KP`, `KPPP-KPP`, `KP-KP`,
`KPP-KPP`). Our only exact pawn knowledge is the single-pawn KPK bitbase;
everything with two pawns falls back to generic evaluation.

**Stated limitation.** Frequency is necessary, not sufficient: a class we reach
often but already evaluate correctly needs no recogniser. Value is
frequency × *error*, and error is **not** measured here. The refinement —
bucket the holdout by these classes and compare static evaluation against the
game result per bucket — is the right input to 5.9.8 and is cheap, since the
data exists and the classes are now defined. Recorded so the next step does not
silently assume frequency alone justifies the work.
