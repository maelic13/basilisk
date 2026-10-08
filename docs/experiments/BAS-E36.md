# BAS-E36

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E36 — 5.9.22: the KBNK failures are STUCK, not slow; two remedies
refuted** (2026-09-01). Tablebase DTZ at every white-to-move node, over the 198
Syzygy-confirmed wins.

| outcome | n | median optimal DTZ | median plies spent | median DTZ left | moves losing ground |
|---|---:|---:|---:|---:|---:|
| mated | 94 | 47 | **47** | 1 | 22.1% |
| fifty-move | 77 | 52 | 100 | **46** | **47.7%** |

**The result is bimodal, and that kills the framing the step was written with.**
When we convert we do it in **1.0x optimal** — the technique is not slow. When we
fail we spend 100 plies to travel 6, and still have 46 to go. We are not slow,
we are **stuck**, and on those games nearly half our moves lose ground.

*Mechanism, from the move traces.* Progress is real at first, then the engine
shuffles the bishop — `Ba4 Bd7 Be8 Bb5 Be2 Bg4 Bd3 Bc4` — while DTZ oscillates
in a band and the white king barely moves. Every term in `kbnk_score` is a
function of the weak king, our king or the knight; **nothing depends on the
bishop**, so all bishop moves score *exactly* alike. The potential has a flat
maximum that is not mate, and escaping it needs a multi-move plan whose
intermediate steps do not raise the potential.

**Arm B — bishop proximity to the weak king, weight 300. REFUTED.**

| | baseline | arm B |
|---|---:|---:|
| mated | 94 (47.5%) | 88 (44.4%) |
| stalemate | 16 | 20 |
| piece lost at ply < 10 | **0** | **2** |

94 -> 88 is inside one SE (±3.6pp), so this is "no gain", not a proven loss. It
is rejected on the second row: pulling a long-range piece next to a bare king
gets it captured, at ply 6 twice, which never happened before.

**Arm C — the weak king's escape-square count, weight 400. REFUTED, and harder.**
Chosen because it is what a mating net actually tightens: it moves with every
piece including the bishop, reaches zero exactly at mate, and cannot be gamed by
parking the bishop next to the king (an undefended bishop is not in our attack
set, so its square *counts as a flight square* and the term gets worse). KBN-K
conversion fell **14/16 -> 9/16**, failing the CTest floor — about 3.8 SE, a real
regression, so it was rejected without spending the 198-position run.

*Hypothesis for why, not a claim:* minimising flight squares rewards confinement
that is stalemate-adjacent rather than mating, and it competes with the corner
drive instead of reinforcing it.

**5.9.22 stays open.** The diagnosis is solid and reusable; the two obvious
bishop features are now closed off. Bench was byte-identical on both arms — the
bench suite contains no KBNK position — so only the endgame instruments can see
this work at all, which is exactly why it needs `tools/diag/kbnk_outcomes.py`
rather than an SPRT.
