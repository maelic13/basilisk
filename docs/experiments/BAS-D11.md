# BAS-D11

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D11 — 5.7.3 REFUTED: the reference's extension exclusivity fails our
tactical floor** (2026-08-30). Three measurements, one implemented change
reverted, no games spent.

*First, how often the stack the audit flagged actually happens* — instrumented
with three new diag counters, 107-position suite, depth 12:

| | count | share |
|---|---:|---:|
| interior nodes | 14,154,475 | |
| singular extension fired | 70,790 | 0.50% of interior |
| …of which **+2 double** | 37,884 | **53.52% of fired** |
| …at an in-check node | 22,940 | 32.41% of fired |
| …**both → the full 3-ply stack** | 17,412 | 24.60% of fired = **0.123% of interior** |

**The double-extension rate is the surprise: more than half of all singular
extensions take +2.** `singular_double_margin` is **4** on a range of 0–60, so
the double fires when `s_val < s_beta - 4` — barely stricter than the singular
test itself. A mechanism meant to mark "one move is overwhelmingly best" is the
common case.

*But tightening it buys nothing.* Swept on `sweep.py` (107 positions, 300k
nodes, paired): margin 12 → **−0.178** ply, 25 → −0.009, 40 → +0.000. The
permissive setting looks wrong and does not measurably cost depth. Not pursued.

**`double_ext_max` is dead code.** Capping it at 16 instead of the 200 default
gives **0 better, 0 worse, 107 same** — `ss->double_exts` never reaches 16, so
the Phase 6.4 path cap has never once bound. Recorded for 5.7.6.

*Then the audit's actual candidate*, landed behind an inert knob and swept:

| `SingCheckMaxExt` | paired Δ depth | WAC @ depth 6 | floor 130 |
|---|---:|---:|---|
| **2 — compose (current)** | — | **137** | pass |
| 1 — no double when in check | +0.009 | 132 | pass |
| 0 — exclusive (the reference) | **+0.065** | **124** | **FAIL** |

**The reference's semantics do not transfer, and the knob was removed.**
Exclusivity fails the WAC floor outright; the intermediate setting costs 5
solved positions to buy noise. The reason is structural: our check extension is
**per-node and unconditional**, the reference's is **per-move and gated on
discovery-or-SEE**. Removing our composition therefore removes strictly more
extension than removing theirs would. Composition stays.

**Durable lesson — depth-at-fixed-nodes is not sufficient for extension work.**
That instrument ranked exclusivity **best** at +0.065 ply. WAC caught it as a
tactical regression that fails a correctness floor. Extensions exist to find
forcing lines, and a metric that averages depth over quiet and tactical
positions alike cannot see that. **Any future extension candidate must clear WAC
as well as the depth sweep before it is considered for a gate.** The two
instruments disagreeing is itself the finding.

*Kept:* the three probe counters (`sing_fired`, `sing_double`, `sing_in_check`,
`sing_triple`), as the evidence base for anything revisiting this interaction.
Bench restored to 12,709,666, CTest 12/12, WAC 137/300.
