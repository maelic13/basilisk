<div align="center">

<picture><source media="(prefers-color-scheme: dark)" srcset="../images/logo_mark_dark.svg"><img src="../images/logo_mark.svg" alt="Triumviratus" width="130"></picture>

# Triumviratus: future directions

**What the network gets wrong, how it divides the positions, and the proposals that follow**

**by Francesco Torsello**

<sub>in collaboration with Maurizio Platino</sub>

</div>

---

<div align="center">

[What the network misses](#1-what-the-network-misses) · [How the network divides positions](#2-how-the-network-divides-the-positions) ·
[Shared and private lanes](#3-shared-and-private-lanes-per-expert) · [Overlapping bands](#4-overlapping-material-bands-in-training) ·
[Positions we lose](#5-training-on-the-positions-we-lose) · [Graded surprise](#6-a-graded-surprise-rule) ·
[Open engine measurements](#7-open-engine-measurements) · [Tuning again](#8-tuning-again-with-the-corrected-schedule) ·
[Speed](#9-speed) · [8-bit HalfKA](#10-8-bit-halfka-weights) · [A search shaped by the network](#11-a-search-shaped-by-the-network) ·
[Status](#status)

</div>

---

Written on 9 October 2026, during the development of 8.0, and extended on 14 October with the work left open at the
release. The two studies below were made on the 8.0 network, Consilium, with instrumentation added to the development
source; their purpose is to decide the shape of the next network before spending GPU time on it. Sections 3 to 6 are
proposals, recorded so that they are not lost. Sections 7 to 11 list what 8.0 leaves open on the engine side: measurements
stopped before a decision, tuning to redo, speed work and the direction of the search.

**Plan.** Work that needs no new network (sections 6 to 9 and 11) is meant for 8.1. Work that needs a new training
(sections 1 to 5 and 10) is meant for 9.0, together with the next network. The techniques already in the engine, and
what is new among them, are described in [`NOVELTIES.md`](NOVELTIES.md).

Both studies are first measurements on a few thousand positions. Each will be confirmed on a larger and more varied
set before any training is started.

## 1. What the network misses

**Method.** The search corrects the static evaluation where the network is wrong. For 3,888 positions taken from
about 7,000 games against many different engines, the score of a depth-1 search (network plus quiescence, so pending
captures are resolved) was compared with the score of a 300,000-node search. The difference, the residual, is what
the network does not see and the search finds. Around thirty chess concepts were then computed for each position
(our side minus the opponent's), and for each one the effect on the residual was estimated by least squares, holding
the starting score, the material balance and the number of pieces fixed. A concept with a strong and stable effect is
a candidate input for the next network, to be added as a zero-initialised graft, the method that produced the
`PassedPawns` block.

**Result.** Effect of one unit of the concept on the residual, in centipawns, with its t statistic (|t| > 3 is a
signal). A positive effect means the network underestimates the concept.

| concept | endgames (15 pieces or fewer) | middlegames (more than 15) |
|---|---:|---:|
| passed pawn, per rank of advance | **+11.1** (t 10.3) | +2.8 (t 2.8) |
| passed pawn, per pawn | **+31.7** (t 8.4) | +4.4 (t 1.3) |
| passed pawn the enemy king cannot catch | **+44.8** (t 6.2) | +15.5 (t 2.4) |
| connected passed pawns | **+23.0** (t 4.8) | **+20.1** (t 3.6) |
| space, per square | **+12.0** (t 6.6) | **+5.7** (t 6.0) |
| open files next to our own king | −12.2 (t −3.4) | **−15.4** (t −5.1) |
| enemy pieces attacking our king zone | −6.4 (t −1.9) | **−11.1** (t −5.2) |
| locked pawns, as an amplifier of the score (per 100 cp) | +6.5 (t 2.6) | **+11.2** (t 8.0) |
| opposite-coloured bishops, rook endgames (drawing factors) | no effect (t 0.7, −0.4) | |

**Reading.**

- **Passed pawns are still underestimated, mostly in endgames, even with the `PassedPawns` block.** The block encodes
  where a passer stands; what the residual points to is relational: whether the enemy king can catch it, whether it is
  connected, whether its path is free. A second version of the block with these relations is the first candidate.
- **The danger to the own king is underestimated in the middlegame:** open files next to the king and the number of
  enemy pieces attacking its zone. The threat inputs of the network describe attacks on pieces, not on the squares
  around the king. A king-zone block is the second candidate.
- **Space is underestimated in every phase**, by about 6 to 12 centipawns per square. Third candidate.
- The network handles the classical drawing factors well: opposite-coloured bishops and rook endgames leave no
  residual.

The residual also contains dynamic effects that a static input cannot capture, so these are candidates to be tested
by a graft, not conclusions.

## 2. How the network divides the positions

**Method.** Consilium chooses its expert by the number of pieces on the board. Here the network was asked which
regimes it sees itself. The outputs of its first layer (1,024 values per evaluation) were recorded with the position
they came from, 92,442 evaluations in all, and grouped without labels (principal components and k-means). The groups
were then compared with criteria that could choose the expert of a future network, by normalised mutual information
(0 = the criterion says nothing about the groups, 1 = it explains them completely).

| criterion | explains the network's groups (6 groups) |
|---|---:|
| **material band × queens on the board** | **0.43** |
| queens on the board (0, 1, 2) | 0.39 |
| material band (the criterion of Consilium) | 0.34 |
| number of pawns | 0.29 |
| rooks and minor pieces | 0.20 |
| closed pawn structure | 0.07 |
| opposite castling | 0.01 |

**Reading.** The presence of queens explains the network's own division of the positions better than the material
bands it was built with, and the two together explain it best. Inside each of the three lower bands the network still
separates positions with and without queens; inside the opening band it separates closed from open structures. The
largest group, 37% of the evaluations, is queenless positions with few pieces.

**Proposal: experts by material band and queens.** A next network with six to eight experts, chosen by material band
and by whether queens are on the board. Queens change only with a queen capture or a promotion, so the expert would
change as rarely as today and the incremental update would keep its cost. The choice would be measured on the network
rather than guessed.

## 3. Shared and private lanes per expert

The neuron study of [`NOVELTIES.md`](NOVELTIES.md) §5 found that every expert of Consilium uses nearly every pair of
the first layer, with different intensity: the endgame expert has 85 pairs it uses at least twice as often as the
others. A next network could give each expert a shared part of the first layer and a private part, wider where the
evaluation is harder. This is a mixture of experts with capacity where it is needed, at the same cost per position.

## 4. Overlapping material bands in training

When a capture moves a position from one expert to the next, the evaluation can jump. In training, each expert could
also learn from the positions of the neighbouring band, with a lower weight, so that neighbouring experts agree near
the boundary. The engine would not change at all. Earlier attempts to handle the boundary inside the search did not
gain, and neither did averaging the two experts at evaluation time (+5 at fixed nodes, but 15 to 18% slower:
[`archive/EXPERT_BLEND_8.0.md`](../development/archive/EXPERT_BLEND_8.0.md)); this addresses it where it arises, in the network.

## 5. Training on the positions we lose

Fine-tuning that gives more weight to positions like those where the engine loses against stronger opponents, the
"hard examples" of machine learning, rarely used for NNUE. A few thousand lost positions are not enough to move a
network trained on hundreds of billions; this needs a dedicated data generation that produces billions of such
positions, for example games against stronger engines, or positions where a deep search contradicts the static
evaluation (the residual of section 1, collected at scale).

## 6. A graded surprise rule

The surprise rule of 8.0 gives 20% more time when the opponent does not play the predicted reply. The graded form,
already in the development source and off by default (`TmSurpriseGrade`), weighs that extra time by how much the unexpected reply
changed the score against the expected one: no extra time when the opponent has given something away, up to twice the
extra when the reply was stronger than predicted. It will be measured at 40+0.4, where the surprise rule gained.

## 7. Open engine measurements

During the last week of 8.0 several changes were measured and stopped before the SPRT reached a decision, because the
release came first. Their code is in the source, off by default. Each will be taken up again one at a time, as fishtest
does: a fixed-bound SPRT run until it accepts or rejects, first at a short time control and then, for the changes that
pass, at a long one. Results below are pentanomial Elo with 95% intervals; none of them excludes zero.

| change | measured so far | next step |
|---|---|---|
| Time per network expert (`TmExp0`–`TmExp3`) | +1.9 ± 5.9 at 30+0.3 (2,930 games) | SPRT to a decision at 30+0.3 |
| Reduction scale per expert (`LmrScaleExp*`) | +1.1 ± 3.7 at 8+0.08 (9,118) | SPRT to a decision, then 20+0.2 |
| Long time control ramp (nine search deltas that grow with depth) | +1.0 ± 6.5 at 30+0.3 (2,742) | SPSA at 30+0.3 (section 8), then SPRT |
| Draw value in defence (`DrawDefend`) | +0.3 ± 4.0 at 8+0.08 (7,296) | SPRT on an endgame book without adjudication |
| Low-depth hash-move extension (LDSE) | off in the release: with it, the 40+0.4 non-regression test of the time rules read −9.4 ± 10.9 (890) and that side searched 0.1–0.25 ply shallower | SPRT of LDSE alone at 30+0.3 |
| Separate checks of the two defence time rules (`TmEarlySave`, `TmDefendMul`) | adopted together on 11 October | one SPRT each at 10+0.1 and 40+0.4 |
| Fifty-move scaling of the hash and of the correction (`HashFiftyStep`, `CorrFiftyStep`) | +0.3 ± 3.7 (3,172) and +1.3 ± 2.9 (5,294) on endgames | the hash at 40+0.4 with a larger table; the correction in a different form (per band, non-pawn material only) |
| Per-expert search margins at a long time control | adopted at 10+0.1 on 12 October; at 20+0.2 −0.9 ± 4.7 (4,668 games, Maurizio Platino) | re-tune in the SPSA P2 (section 8) |

## 8. Tuning again with the corrected schedule

On 13 October a defect was found in the project's SPSA tool: the perturbation `c` decayed from the value meant as the
final one, so every SPSA run of 8.0 explored with between a half and two fifths of the intended step. The tool now uses
the schedule of fishtest (`c` reaches its final value at the last iteration, `a` scaled to match). The parameter vectors
of 8.0 are therefore a rough tuning. The runs to repeat, in order of priority, all at 5,000 iterations:

| run | parameters | time control |
|---|---|---|
| P1 | 20 search constants | 20+0.2 |
| P2 | per-expert margins, reduction scales and the causal reduction (17) | 10+0.1 |
| P3 | move ordering (17) | 10+0.1 |
| P4 | pruning and reductions (46) | 10+0.1 |
| P5 | the long time control ramp (9) | 30+0.3 |
| P6 | deep-search parameters (4) | 40+0.4 |

The first corrected run, on the 15 evaluation-correction weights, moved several of them clearly; its mean vector was
neutral in an SPRT (+0.7 ± 2.9 over 14,386 games at 8+0.08), so nothing was adopted. Each vector will enter the engine
only through an SPRT against the release.

## 9. Speed

8.0 is measured with hardware counters (cycles per node at a fixed search tree, `xperf`, pinned cores). The candidates
left are small and tree-identical:

- the templated add and subtract loops of the accumulator update, from Stockfish's development after version 19;
- a branchless form of the correction lookup;
- the measurements of 12 to 14 October to repeat with the corrected analysis script (the per-thread rule did not
  handle persistent search threads).

Changes that were measured and found within the noise have been removed from the source; the record is in the
development log.

## 10. 8-bit HalfKA weights

Reading the king-piece (HalfKA) rows of the first layer as 8-bit instead of 16-bit integers halves the memory read at
each incremental update. With the current network the conversion loses precision: +2.8 ± 6.5 over 3,124 games, which
does not show a gain. The network has to be trained with its HalfKA weights clipped to the 8-bit range; it is planned
with the next training.

## 11. A search shaped by the network

The 8.0 search was reorganised on the algorithmic base of Stockfish 19's search and re-tuned on Consilium, with the
project's own techniques on top (time management, per-expert margins, the causal reduction, the passed-pawn rules in
endgames). The next step is to go through its components one at a time: an audit of what each one does with a
mixture-of-experts network, a rewrite where the network calls for a different design, and an SPRT for each rewrite.
Per-expert margins were the first example: the right pruning margin depends on which expert evaluates the position.

## 12. Analysis ideas worth a game test

The analysis build (Triumviratus Pos Solver) added search components that act only in analysis. Most of them spend
depth to keep rare candidate moves alive, which is the opposite of what a game needs, but one may also help in games:

- a null-move guard near the solution: from root depth 11, for the side opposite to the root, the null move is tried
  only with more than five legal moves (an idea from Crystal by Joseph Ellis, our implementation). It changes the
  search in a few positions out of a hundred, so its effect in games is expected to be small; it needs a long SPRT on a
  book with many endgames.

The two-line search (always searching the second best root move with a full window) clearly helps on hard positions
but costs two to three plies of average depth, so it stays an analysis feature.

## 13. Playing strength levels

Triumviratus always plays at full strength, so a human cannot play a useful game against it. 8.1 should add playing
levels through the standard UCI options, `UCI_LimitStrength` and `UCI_Elo`, which most interfaces show as a strength
slider. A weaker level should play like a weaker player, with plausible moves and occasional errors, not like a full
strength engine that blunders at random. The plan is to limit the search (depth and nodes per move) and to choose
among the best root lines with a chance that grows with the level's distance from full strength and shrinks with the
score loss. Each level is then calibrated in games against engines with a known rating, so that the number in
`UCI_Elo` means what it says.

## Status

| direction | status |
|---|---|
| Inputs from the residual (section 1) | grafts on Consilium closed (§37–38 of the development log); engine-side use without retraining (constant terms, learned corrections, search rules) closed on 11 October (§44); open only for a new network |
| Experts by material and queens (section 2) | first measurement done; to be confirmed on a larger collection |
| Shared and private lanes (section 3) | proposal, needs a full training |
| Overlapping bands (section 4) | proposal, needs a full training; the engine-side blend was measured and removed |
| Positions we lose (section 5) | proposal, needs a dedicated data generation |
| Graded surprise (section 6) | implemented, off; game test queued |
| Open engine measurements (section 7) | code in the source, off; one SPRT each, to a decision, for 8.1 |
| Tuning with the corrected schedule (section 8) | tool fixed, six runs prepared; for 8.1 |
| Speed (section 9) | three small candidates; for 8.1 |
| 8-bit HalfKA weights (section 10) | engine path ready behind a build flag; needs clipped training, for 9.0 |
| A search shaped by the network (section 11) | direction for 8.1 and later |
| Analysis ideas for games (section 12) | null-move guard implemented for analysis; one game SPRT for 8.1 |
| Playing strength levels (section 13) | planned for 8.1 (`UCI_LimitStrength`, `UCI_Elo`), calibrated in games |
