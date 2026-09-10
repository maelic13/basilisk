# PLAN 6.8.a — what resolves the won-rook only-move defect?

**Date:** 2026-09-07
**State / class:** `RESEARCH` / `R3`.
**Prior measurement status:** BAS-E53 measured the former 6.5.a possibility
question. Instrument `tools/diag/narrow_node_probe.py`;
results `tools/results/narrow-nodes-6.5.a/` (gitignored).

## Current decision needed

Determine whether the 68 one-winning-move nodes support a falsifiable
search-, HCE- or explicit-knowledge mechanism in Basilisk, or close with
`NO_CHANGE`. BAS-E53 proves the class is locally resolvable and refutes more
depth and wrong-piece explanations; it does not identify a mechanism.

Known evidence is BAS-E32 (separate drawn-share bias) and BAS-E53 (paired node
probe below). Credible competing explanations remain evaluation authority,
search realization, explicit rook-ending knowledge and an instrument/sample
artifact. Relevant interactions include rook/pawn evaluation, search
selectivity, TT carry-over, tablebase-off deployment, rule 50 and the separation
between draw scaling and won-position move precision.

The cheapest next discriminator is the already-roadmapped exhaustive
classification of the 68 nodes with a prospectively frozen held-out split. No
substantial implementation is licensed before that classification names an
isolated mechanism and a falsifier. **No mechanism prediction is backfilled
here:** BAS-E53 is already exposed. The next research pass must freeze its own
expected diagnostic movement, probability/confidence and stopping rule before
opening its held-out result.

`READY_FOR_IMPLEMENTATION` requires exact intended semantics, a producer/state/
consumer and interaction map, promotion/material closure where relevant,
deterministic known-bad cases, cheap qualification, a maintainer-owned game
gate and explicit non-goals. Otherwise the decision remains `MORE_RESEARCH` or
`NO_CHANGE`.

## The claim under test

6.5.a's characterisation established that Basilisk throws won rook endings
almost exclusively at nodes where few legal moves preserve the win, and drew
this conclusion:

> A static evaluation supplies a gradient over many moves; it cannot reliably
> pick two exact moves out of twenty.

That is an assertion about what is *possible*, and it was carrying the leaf's
direction — it is the reason the leaf points away from a rook-ending term. It
has a decisive test: if another engine resolves those same nodes at the same
node budget, the class is resolvable and the deficit is ours.

## Design

Two engines replaying the same root diverge at the first differing move, after
which any preservation-rate difference is partly a difference of trajectory.
So the comparison is **paired on frozen nodes**, not on games:

1. **extract** — replay all 48 clean wins (24 KRP-KR, 24 KRPP-KRP) once with
   the frozen head at 60,000 nodes; freeze every White-to-move node whose WDL
   is still a clean win, with `win_moves` = how many legal moves preserve it.
   **841 nodes**, 9 dropped as unprobeable.
2. **arm** — every arm answers those same 841 positions with one move, under a
   **fresh game token per node** so no arm gets transposition-table carry-over
   another lacks, and with **`SyzygyPath` cleared** so this measures evaluators
   rather than tables (Stockfish confirmed at `tbhits 0`).
3. **summary** — bucketed by narrowness, with the **wide bucket as the
   control**: an arm that is simply stronger everywhere improves there too.

Buckets were fixed before any arm ran: narrow ≤ 3, mid 4–9, wide ≥ 10.

## Result

| arm | narrow (161) | mid (133) | wide (547) | all |
|---|---|---|---|---|
| basilisk@60k | 147 / **91.3%** | 131 / 98.5% | 545 / 99.6% | 97.9% |
| basilisk@300k | 149 / **92.5%** | 131 / 98.5% | 547 / 100.0% | 98.3% |
| basilisk@600k | 151 / **93.8%** | 132 / 99.2% | 547 / 100.0% | 98.7% |
| stockfish@60k | 160 / **99.4%** | 133 / 100.0% | 547 / 100.0% | 99.9% |

Paired McNemar against basilisk@60k on the 161 narrow nodes:

| arm | arm only | ref only | z |
|---|---|---|---|
| basilisk@300k | 4 | 2 | 0.82 |
| basilisk@600k | 6 | 2 | 1.41 |
| **stockfish@60k** | **13** | **0** | **3.61** |

Resolved by exact winning-move count, the failure is an **only-move** problem:

| winning moves | n | bas@60k | bas@600k | sf@60k |
|---|---|---|---|---|
| **1** | 68 | 58 / **85%** | 61 / 90% | 68 / **100%** |
| 2 | 58 | 57 / 98% | 56 / 97% | 57 / 98% |
| 3 | 35 | 32 / 91% | 34 / 97% | 35 / 100% |
| 4 | 31 | 31 / 100% | 31 / 100% | 31 / 100% |
| 5 | 34 | 33 / 97% | 34 / 100% | 34 / 100% |
| ≥ 6 | 615 | 612 / 100% | 614 / 100% | 615 / 100% |

## What this establishes

**1. The class is resolvable at the measured budget.** Stockfish preserves
160 of 161 narrow nodes on 60,000 nodes — the same budget at which Basilisk
preserves 147. Thirteen nodes go to Stockfish and none to Basilisk, z = 3.61.
The "no gradient can pick two moves out of twenty" conclusion is **false as
stated** and no longer licenses the leaf's direction.

**2. It is not a search-volume problem.** Ten times the nodes buys 4 of the 14
narrow errors (z = 1.41, not significant). Basilisk at 600,000 nodes is still
at 93.8% where Stockfish at 60,000 is at 99.4%. Depth is not the missing
ingredient, so this is not deferrable to Phase 8 search work.

**3. The deficit is specific, not general.** On the 680 non-narrow nodes
Basilisk loses 4 against Stockfish; on the 161 narrow nodes it loses 13. The
wide control sits at ceiling for both engines, so this is not "Stockfish is
better", it is a failure concentrated where exactly one move wins.

**4. Piece selection is not the mechanism.** At the 13 discordant nodes
Basilisk's losing moves are 8 rook, 4 king, 1 pawn; Stockfish's winning moves
are 8 rook, 5 king. **The engine reaches for the right piece and the wrong
square.** This kills two earlier candidates outright: premature passer advance
(one pawn move in thirteen) and any "wrong piece to move" account.

## What this does not establish

**Stockfish evaluates with an NNUE.** Proving the class is resolvable by *an*
evaluator does not prove it is resolvable by a hand-crafted term, and this
result must not be read as licensing an HCE rook-ending coefficient. It removes
the impossibility argument; it does not supply a mechanism.

**The 13 discordant nodes are not a mechanism either.** Inspection suggests
rook placement — rook to the seventh versus a passive square, opposition
nuances, one premature push — but thirteen cases is the same sample size that
already misled this leaf once, when five first-move throws produced a "family
of causes" that the controlled profile then overturned. No cause is claimed
here.

**Node-budget parity across engines is approximate.** A node is not the same
work in two programs; this follows the 6.0.b convention of giving both engines
60,000 and inherits its caveat. The 600k arm is what protects the conclusion:
whatever Stockfish's node is worth, Basilisk with ten times as many does not
catch it.

## Consequences for the leaf

The mechanism question is no longer "can anything resolve this" but "what
knowledge resolves it, and can that knowledge be expressed in this engine".
The failure is concentrated in only-move nodes (68 of 841), which is a small
enough population to study exhaustively and too small to fit coefficients
against without a held-out split.

BAS-E32's drawn-share bias in these same families remains real, separate and
unaddressed; nothing here bears on it.
