# 5.9.7 — Endgame recogniser inventory and risk order

Measured 2026-08-31 from `armC_basilisk25k.pgn`: 20,000 self-play games at
25k nodes with **adjudication off**, so endings are played out rather than
resigned at 600cp. Counts **games reaching a class at least once**, with ≤7
pieces on the board — the quantity PLAN asks for, since a class reached in 2% of
games matters more than one reached 500 times inside a single drawn ending.

Classes are canonicalised so `KRP-KR` and `KR-KRP` are one entry.

## What we already recognise

| Class | Mechanism |
|---|---|
| `KP-K` | KPK bitbase (exact) |
| `KBN-K` | KBNK corner drive |
| `KNN-K` | forced draw |
| `K-K`, K+minor-K | insufficient-material scaling |
| bishop-pawn | `KBP` scaling |
| opposite bishops | OCB draw scaling |
| bare-king generic | mate-drive (`5*lk_center + (14-king_dist)*4`) |

## Measured frequency — absent classes in bold

| Class | Games reaching | % | Covered? |
|---|---:|---:|---|
| **`KRPP-KRP`** | 2,264 | **11.32%** | **no** |
| **`KRP-KR`** | 1,818 | **9.09%** | **no** |
| `KR-K` | 1,329 | 6.64% | mate-drive |
| **`KRP-KRP`** | 1,301 | **6.50%** | **no** |
| **`KR-KR`** | 1,221 | **6.11%** | **no** |
| `KQ-K` | 1,175 | 5.88% | mate-drive |
| **`KRPP-KR`** | 1,124 | **5.62%** | **no** |
| `KP-K` | 926 | 4.63% | KPK bitbase |
| `KQP-K` | 891 | 4.46% | mate-drive |
| **`KPP-KP`** | 808 | **4.04%** | **no** |
| `K-K` | 770 | 3.85% | insufficient |
| `KRP-K` | 756 | 3.78% | mate-drive |
| **`KPPP-KPP`** | 685 | **3.42%** | **no** |
| **`KBPP-KBP`** | 548 | **2.74%** | partial (`KBP`, OCB) |
| `KBN-K` | 533 | 2.67% | KBNK |
| **`KRP-KP`** | 529 | **2.65%** | **no** |
| **`KP-KP`** | 511 | **2.56%** | **no** |
| **`KPP-KPP`** | 494 | **2.47%** | **no** |
| **`KR-KP`** | 475 | **2.38%** | **no** |
| **`KQR-KR`** | 419 | **2.10%** | **no** |

## Risk order

**1. Rook endings — overwhelmingly first.** `KRPP-KRP`, `KRP-KR`, `KRP-KRP`,
`KRPP-KR` and `KR-KR` together are reached by a large fraction of all games and
occupy five of the top seven rows. Nothing in our evaluation knows anything
about them: no rook-ending scaling, no Philidor/Lucena notion, no
rook-behind-passer scaling beyond the generic positional term.

**PLAN's own ordering under-ranked this.** It named `KRPKR` first, which is
correct as far as it goes — but `KRPP-KRP` is *more* frequent still, and the
drawn `KR-KR` case at 6.11% was not on the list at all.

**2. Multi-pawn endings.** `KPP-KP`, `KPPP-KPP`, `KP-KP`, `KPP-KPP` — reached
often, and our only exact knowledge is the single-pawn KPK bitbase. Everything
with two pawns falls back to generic evaluation.

**3. Rook vs pawn** (`KRP-KP`, `KR-KP`) — the class where a tempo decides the
result and a generic evaluation is least likely to be right.

**4. Bishop endings** (`KBPP-KBP`) — partially covered by `KBP` scaling and OCB.

**5. Queen vs rook** (`KQR-KR`) — 2.10%, and usually a straightforward win the
existing material terms already price roughly.

## What this inventory does NOT establish

Frequency is **necessary but not sufficient**. A class we reach often but
already evaluate correctly needs no recogniser. The value of a recogniser is
frequency × *error*, and error is not measured here.

The natural refinement — and the right input to 5.9.8's design — is to take the
holdout corpus, bucket its positions by these classes, and compare our static
evaluation against the actual game result per bucket. That identifies where we
are *wrong*, not merely where we *are*. It is cheap: the data exists and the
classes are already defined above.

Recorded so the next step starts from it rather than from the assumption that
frequency alone justifies the work.
