# B.5.6 — how Basilisk runs SPSA: the investigation and the proposed redesign

**Status:** research packet, written 2026-11-12. Zero games. It proposes a redesign of
PLAN rule 7c and PROCESS *SPSA go/no-go*; neither is edited yet. The rule text in §8 is
a draft for the maintainer's sign-off, and B.5.6 stays open until it is signed off and
recorded.

**Recommendation.** Run a tune as one registered run with twice today's perturbation and
half its gain ratio, read every 1,000 iterations by a per-coordinate gradient test on the
tune's own games, and stop when that test reads quiet. Drop the blocks and the count of
full-step movers. Keep matches for the gate.

**Main counter-argument.** The evidence is five blocks on one host at one time control,
every read here is retrospective, and the Elo forecast (§5) has no usable test yet. So
the stop rule rests on the gradient test, whose null is exact, and the forecast
is recorded and scored but decides nothing until it has earned it (§8, §10).

## 1. The question and the evidence

PLAN B.5.6 asks: blocks or one long schedule; what the stop should be; how movement is
read; and the steps, the gain ratio, the games per iteration and the estimator.

The evidence is the five SPSA blocks on disk, each 2,000 iterations of 30 games at
`3+0.03`, and the matches that measured four of them:

| Block | Run directory | Coordinates | Rule 7c | Theta against its seeds |
|---|---|---:|---|---|
| B.2.3 block 1 | `b23-spsa-block1` | 49 | CONTINUE, 6 movers | +57.0 ± 9.8, fixed 2,000 games (BAS-S24) |
| B.2.3 block 2 | `b23-spsa-block2` | 49 | STOP, 2 movers | not measured usefully (below) |
| B.3.3 | `b33-spsa-block1` | 29 | STOP, 1 mover | +17.6 ± 6.4, an SPRT's stop (BAS-S43 b) |
| B.4.6 | `b46-spsa-block1` | 15 | STOP, 1 mover | +6.5 ± 3.6, an SPRT's stop (BAS-S52 b) |
| B.5.4 | `b54-spsa-block1` | 8 | CONTINUE, 3 movers | +10.8 ± 5.3, fixed 6,000 games (BAS-S56) |

An SPRT's estimate is biased upward by where it stopped. BAS-S27's chain carries "block
2's +2", which is B.2.4's gate (+58.7 ± 20.9) less block 1's match (+57.0 ± 9.8): a
difference known to about ±23, so block 2's gain has no match of its own.

Every read below is rebuilt from each block's `result.json` (the centre after every
iteration and that iteration's wins, losses and draws). The reads were not registered
before they ran: PLAN named them as reads to consider, and no prediction was frozen. The
scripts and outputs are in `tools/results/b56-20261112/` (§13).

## 2. What one iteration does

Colosseum's schedule is fishtest's (§6). At iteration *k* each coordinate *i* is perturbed
by `±c_k,i` with an independent random sign `δ_i`; the plus vector plays the minus vector
30 games; `D` is plus wins minus plus losses; and every centre moves by
`r_k · c_k,i · D · δ_i`.

Two facts follow, and both were checked on the journals.

- **In step units every coordinate moves by the same amount, with its own sign.** The
  largest spread within an iteration is 2.8e-15 on the two blocks without a clamp (B.4.6
  and B.5.4). The
  step is the generator's (`range / 16`), and it is the perturbation at iteration 0.
- **The gain ratio is flat and the perturbation decays.** `r_k` runs from 0.00277 to
  0.0031 over a block. The perturbation falls from 1.00 step to 0.62 at iteration 100,
  0.53 at 500 and 0.46 at 2,000. So the move per unit of `D` falls from 0.0028 to 0.0014
  steps, and nearly all of that fall is the perturbation's.

## 3. The gradient test: a tune is a match per coordinate

For a coordinate with no gradient, its sign `δ_i` is independent of `D`. So

`z_i = Σ δ_i,k · D_k / √(Σ D_k²)`

over any window is a sum of symmetric terms with known sizes: mean 0, variance 1,
whatever the other coordinates do, whatever the draw rate. The tuner drew the signs
itself, so this null is exact, not modelled. Re-drawing the signs 4,000 times against
B.5.4's own results gives SD 1.000 and 4.5% beyond |z| = 2 (`stop.txt`).

`z_i` is the result of a match between "this coordinate one perturbation up" and "one
perturbation down", played inside the tune at no extra cost. In Elo:

- one unit of mean `D` over 30 games is 11.58 Elo between the plus and minus sides;
- over a 2,000-iteration block the standard error of a coordinate's
  `Elo(+c) − Elo(−c)` is **0.86 to 0.95 Elo**, the same for every coordinate and
  independent of how many coordinates the surface holds (`noise.txt`);
- over 1,000 iterations it is about 1.3 Elo.

This is BAS-S54's sign-randomisation read, taken on the results directly. BAS-S54 read
net travel over its noise; the two agree (the SD of z over a block's coordinates differs
by under 0.2 between them), and the gradient form weights every iteration equally where
travel weights the early ones.

**What the blocks show** (`noise.txt`, `gain.txt`):

- Over 150 coordinate-blocks, 23 read |z| above 2 where 7 are expected by chance, and 7
  read above 3 where 0.4 are expected. Most coordinates show no gradient above the floor.
- Every gradient detected was worth 2 to 5.6 Elo across one perturbation. A coordinate
  worth less than about 2 Elo across ±0.55 steps cannot be seen in 60,000 games.
- The random walk of a coordinate with no gradient is 0.33 to 0.36 steps over a block
  (1 SD), so rule 7c's "one full step" is about a 3 SD event. As a noise line it was
  sound; the faults are elsewhere (§7).

## 4. The unlock premise

Blocks were justified by a premise: moving some coordinates early unlocks others, so a
coordinate that starts late needs a fresh schedule. B.2.3's two blocks test it on 49
coordinates (`unlock.txt`).

- Of the 27 coordinates idle in block 1 (|z| under 1), **two** read |z| above 2 in block
  2 (`CoreQuietBonusSlope` +2.50, `CoreMainHistDecay` +2.08). Chance alone gives 1.2.
  The SD of their z in block 2 is 1.24 against a null of 1.00 ± 0.14.
- Of the 9 coordinates live in block 1 (|z| at least 2), **seven** kept their sign in
  block 2, and the two strongest readings of block 2 are block 1's movers
  (`CoreCorrUpdateScale` +2.82, `CoreRfpPerPly` −2.63).
- z in block 1 and z in block 2 correlate at +0.34, the same as block 1's two halves.

**Reading.** Block 2 continued block 1's unfinished movers. It did not unlock idle
coordinates, beyond what chance gives. The premise is not supported. One pair of blocks,
read after the fact.

BAS-S54 noted that no coordinate passed |z| = 2 in B.5.4's first third. Read at 1,000
iterations, `B5AspBase` passes its line in the first half (+2.75) and `B3SingMargin` in
the second (−3.63), and B.2.3's block 1 read strongest in its first third. A window of
667 iterations resolves a coordinate to about 1.6 Elo (1 SD), so one quiet third is within
chance.

## 5. How far a block can travel, and what it gained

**Convergence capacity.** Take a coordinate with curvature *h* Elo per step², so that
standing *x* steps from its optimum costs `h·x²/2` and its gradient is `h·x`. The expected
move per iteration is proportional to the gradient, so the distance decays as
`exp(−h·Λ)`, where Λ sums `2 · (move per unit D) · (perturbation in steps) / 11.58` over
the run. On the journals **Λ is 0.34 per block**, the same in all five (`forecast.txt`).

| Curvature (one step off costs) | Distance left after 2,000 / 4,000 / 6,000 iterations | With twice the perturbation and half the ratio |
|---|---|---|
| 0.5 Elo/step² (0.25 Elo) | 84% / 71% / 60% | 71% / 51% / 36% |
| 1 (0.5 Elo) | 71% / 51% / 36% | 51% / 26% / 13% |
| 2 (1 Elo) | 51% / 26% / 13% | 26% / 7% / 2% |
| 3 (1.5 Elo) | 36% / 13% / 5% | 13% / 2% / 0% |

Two curvatures can be read from the journals. `CoreIirMinDepth` in B.2.3 block 1: its
gradient fell from 8.0 to 3.5 Elo per step between the first and last third while its
centre moved 1.3 steps, so *h* is about 3.3. That is the strongest coordinate in the
record. `B3NmpPerPly`: 3.9 Elo per step over B.4.6 and 2.5 over B.5.4, 1.4 steps apart,
so *h* is about 1.

**Reading.** At a curvature near 1, one block covers about 30% of the distance and three
blocks 64%. This is the plain explanation for the coordinates that keep moving one way
fit after fit (`B3NmpPerPly` in three fits, `B3SingMargin` in two, `CoreCorrUpdateScale`
in five, `B3IirMinDepth` in three; `analysis/b54_constants_2026-11-12.md` findings 3 to
5): each tune stopped short. It also explains Rarog's RAR-S78, where a restart gained
+13.1: more schedule, not a fresh one.

**The tune's own read of its gain.** The gain of a run is the sum over iterations of
gradient times move. Estimating the gradient and the move from the same iterations is
biased upward (a noise coordinate's travel correlates with its own noise), so the read
is cross-fitted: the gradient from the even iterations against the move of the odd ones,
and the reverse, within each quarter (`gain.txt`).

| Block | Tune's own read | Its null SD | Measured |
|---|---:|---:|---|
| B.2.3 block 1 | +33.2 | 9.3 | +57.0 ± 9.8 |
| B.2.3 block 2 | +3.1 | 8.6 | – |
| B.3.3 | +13.3 | 6.9 | +17.6 ± 6.4 (SPRT) |
| B.4.6 | +8.0 | 5.2 | +6.5 ± 3.6 (SPRT) |
| B.5.4 | +11.3 | 3.8 | +10.8 ± 5.3 |

It agrees with the match within both errors on three blocks and reads low on B.2.3's
block 1, where two integer coordinates made large discrete moves (`CoreIirMinDepth`
4 → 8, `CoreResearchShallower` 2 → 0). Its noise grows with the square root of the
coordinate count: at 8 coordinates it is as sharp as a 3,000-game match, at 49 it is
not. It is a free cross-check on the gate, never the gate.

**The forecast.** The expected gain of the next segment is proportional to the sum of
squared gradients, and `Σ z² − n` over a window estimates that sum without bias, with
SD `√(2n)`. Holding the last 1,000 iterations' gradients constant over a further 2,000
gives an upper reading (`forecast.txt`):

| Block | Excess, iterations 1,000 to 2,000 | Next 2,000 iterations, at most |
|---|---:|---:|
| B.2.3 block 1 | +15.5 ± 9.9 | +10.5 ± 6.7 |
| B.2.3 block 2 | +1.3 ± 9.9 | +0.9 ± 6.6 |
| B.3.3 | −5.7 ± 7.6 | −3.3 ± 4.4 |
| B.4.6 | +17.2 ± 5.5 | +10.0 ± 3.2 |
| B.5.4 | +26.3 ± 4.0 | +15.1 ± 2.3 |

It has no usable test yet. B.2.3 block 1 forecast +10.5 ± 6.7 at most, and block 2's
gain is known only from the tune's own read, +3.1 ± 8.6: consistent with the forecast and
with zero. By its own error it can separate "about ten left" from "nothing left" on a
small surface. It cannot separate 3 from 6, and at 49 coordinates it separates little.

## 6. How others run it

Read as practice, not authority. Sources are in §13.

- **Stockfish (fishtest).** Plain SPSA: `c_k = c / k^γ`, `a_k = a / (A + k)^α`,
  α 0.602, γ 0.101, A a tenth of the iterations, one iteration a game pair, the move
  `r · c · (wins − losses)`. The game count is fixed at the start and is the only stop.
  The wiki's advice: `c_end` "large enough to have a measurable Elo difference", and stop
  a tune whose values barely change after a few thousand games because `c` is too low.
  Recent tunes in the commit log ran 60,000 games at STC or LTC. The tuned values are
  then gated by SPRT at STC and LTC.
- **OpenBench.** The same schedule and constants. Its wiki calls `r_end` 0.002 ideal and
  `c_end` about a twentieth of a coordinate's reasonable range, eight game pairs a
  point, and "hundreds of thousands of games". No stop rule and no convergence check.
- **Triumviratus** (`docs/reference/triumviratus/docs/development/archive/DEVELOPMENT_8.0.md`
  §13, §17, §38, §45, §47.3, §48.1). The same family, up to about 6,000 iterations a tune.
  It reads each lever's net displacement against a random walk with the same steps, as
  BAS-S54 does. Its theta is the mean of the last 50 to 600 iterations. Its tool had
  decayed the perturbation from `c_end` instead of to it, so runs explored with half the
  intended step; the first run after the repair moved several levers clearly. Its later
  tunes on a settled search read about nothing (+3.4 ± 3.9, +1.6 ± 4.3, +0.2 ± 2.9,
  +0.7 ± 2.9).
- **Rarog** (BAS-X31, BAS-X36). Blocks after RAR-S78; then RAR-S104, one run of 4,000
  iterations with twice the perturbation and half the ratio, which settled in its first
  half and passed its gate at +4.5 ± 2.9.

**Basilisk against this.** Colosseum's schedule is fishtest's (α 0.601, γ 0.102, A 200
of 2,000). Our gain ratio is 0.0031 against the usual 0.002. Our perturbation ends at
`range / 35`, against OpenBench's `range / 20`. Our 60,000 games a block match Stockfish's
recent tunes. Nobody in this list has a stop rule beyond the game count, and nobody
reads the gradient test of §3.

**Is there a better method?** Not one with evidence at 8 to 50 coordinates.

- **CLOP** and **Bayesian optimisation** (`chess-tuning-tools`, used for Leela's few
  search constants) fit a model of the whole surface. They suit a handful of coordinates
  and are not used by any strong engine at our counts.
- **RSPSA** (Kocsis and Szepesvári 2006) adapts each coordinate's step from the sign
  agreement of successive gradient estimates. It beat plain SPSA in their poker program.
  A search summary reports that Coulom's own trials of it were poor; I have not read
  that thread.
- **BSPSA** (Ivec and Vojnović 2022) replaces the decaying gain with a posterior-variance
  update. One paper: on Stockfish with 29 detuned parameters it ended 14 Elo nearer the
  original than SPSA after 100,000 pairs. Its gain shrinks faster than ours, which is
  the opposite of what §5 asks for.
- **Second-order SPSA** estimates the curvature from four evaluations an iteration. Its
  curvature estimate is far noisier than the gradient's, and our gradient is already at
  the noise floor.

The strong engines all use plain SPSA, a fixed budget and an SPRT afterwards. What they
do not have, and what our journals can supply, is a read of whether the budget was
enough.

## 7. Answers to the leaf's questions

1. **Blocks or one schedule.** One schedule. The unlock premise is not supported (§4).
   A restart changes almost nothing but the first hundred iterations' perturbation: three
   blocks and one run of 6,000 have nearly the same gain, because the ratio is flat and
   the perturbation is nearly flat after iteration 500 (§2). A restart also rounds every
   centre, which jolts a coordinate with a step of 2 by up to a quarter of a step.
2. **The stop.** The gradient test on the last 1,000 iterations (§8). Applied to the five
   blocks at iteration 2,000 (`stop.txt`):

   | Block | Rule 7c | Gradient test | What happened next |
   |---|---|---|---|
   | B.2.3 block 1 | CONTINUE | quiet: stop | block 2's own read +3.1 ± 8.6, its last 1,000 iterations quiet |
   | B.2.3 block 2 | STOP | quiet: stop | – |
   | B.3.3 | STOP | quiet: stop | – |
   | B.4.6 | STOP | live (`B3NmpPerPly` −3.20) | B.5.4 moved `B3NmpPerPly` again, z −2.50 |
   | B.5.4 | CONTINUE | live (`B3SingMargin` −3.63) | block 2 withdrawn by the maintainer |

   The two disagree twice. On B.4.6 what followed sides with the gradient test. On
   B.2.3 it is not decided: block 2 has no match of its own (§1). Two cases, read after
   the fact.

   A match between checkpoints cannot do this job. Deciding a 5 Elo question needs about
   25,000 games (±2.6 at 95%), nearly the cost of the 1,000 iterations it would decide.
3. **How movement is read.** By the gradient z of §3 over registered windows, against
   its exact null, with a familywise line. Net travel and "moved a full step" are
   dropped: travel is gain times noise, and a count of movers ignores how many
   coordinates there are.
4. **Steps and perturbation.** Twice today's perturbation (§8). Every z doubles for the
   same games, so coordinates now at z 1.5 to 2 become readable. Whether steps should be
   sized by effect instead of `range / 16` is not settled here (§11).
5. **Gain ratio.** Halved with the doubled perturbation, 0.0016. The move per unit of
   `D` in raw units is then today's, so the random walk is unchanged, and Λ doubles (§5).
   Keeping 0.0031 would quadruple Λ and double the walk; at a curvature of 1 the walk
   would then cost about 0.2 Elo a coordinate, which is 6 Elo on a surface of 30.
6. **Games per iteration.** 30 stays: one pair a slot on 15 slots. The other coordinates'
   true gradients add 9.6% to an iteration's variance in the worst window (B.2.3 block
   1's first third), 5.3% in B.5.4's second third and nothing measurable elsewhere, so
   fewer games an iteration would buy almost nothing.
7. **Estimator.** The final centre stays. An average of the last iterations lags a
   coordinate that is still moving, and at this gain the walk never reaches the
   stationary state where averaging pays. This changes if Colosseum gains a held gain
   (§9).

## 8. The proposed rule (draft, for sign-off)

**PLAN rule 7c, replacing the block text and the 2026-10-27 paragraph:**

> 7c. **A tune is one run, read at checkpoints, and stopped when its gradients are
> gone.**
> - *Shape.* 30 games an iteration on 15 slots. A registered ceiling, 6,000 iterations
>   (180,000 games) unless the registration argues another. The perturbation is twice
>   the generator's step schedule and `r_end` is 0.0016. A coordinate whose seed plus or
>   minus two steps leaves its range keeps the single step.
> - *Checkpoints.* Iteration 2,000 and every 1,000 after it. At each, `tools/spsa_read.py`
>   reads the last 1,000 iterations.
> - *The test.* For each coordinate, `z = Σ δ·D / √(Σ D²)`. The window is **live** if any
>   |z| passes the familywise 5% line for the surface's coordinate count, or if
>   `Σ z² − n` is at least `2·√(2n)`. Otherwise it is **quiet**.
> - *The rule.* Quiet: the tune stops, and theta is that checkpoint's rounded centres.
>   Live: the next 1,000 iterations may run. The maintainer may stop at any checkpoint;
>   theta is then that checkpoint's centres. A run stopped between checkpoints is not
>   theta: it resumes.
> - *Rails.* A centre within one perturbation of a bound, with its z pointing outward,
>   stops the tune for a new registration of the range.
> - *The record.* Each read appends the per-coordinate z, the tune's own read of its
>   gain, and a frozen forecast for the next 2,000 iterations. The forecast decides
>   nothing until three scored forecasts have each fallen within twice their SD.
> - A tune is judged by its gate, not by its read.

The familywise line is 2.73 at 8 coordinates, 2.93 at 15, 3.13 at 29 and 3.28 at 49. With
both clauses a tune with no gradient at all reads live under 10% of the time, which costs
one more checkpoint.

**The minimum of 2,000 iterations is there for a reason.** B.4.6 read quiet over its
first 1,000 iterations and live over its second: `B3NmpPerPly` read 2.57 against a line
of 2.93, then 3.20. A first read at 1,000 would have stopped it. With twice the
perturbation that first reading would have been about 5.

**The floor is stated with every stop.** "Quiet" does not mean converged. It means two
things, both reported with the stop:

- no single coordinate is worth more than about 3.5 to 4 Elo across one perturbation
  (the familywise line times the 1.3 Elo of a 1,000-iteration window);
- the forecast's upper reading for the next 2,000 iterations is under twice its SD:

  | Coordinates | Today's perturbation | Twice the perturbation, half the ratio |
  |---:|---:|---:|
  | 8 | 4.6 Elo | 2.3 Elo |
  | 15 | 6.4 | 3.2 |
  | 29 | 8.8 | 4.4 |
  | 49 | 13.4 | 6.7 |

  At 49 coordinates today's perturbation cannot tell a tune with 13 Elo left from a
  finished one. B.2.3's block 2 shows it: no 1,000-iteration window reads live, yet over
  its whole 2,000 iterations `Σ z² − n` is +3.1 SD. The doubled perturbation halves the
  floor, and a smaller surface lowers it further.

**PROCESS *SPSA go/no-go*, items 5 and 6:**

> 5. Register the tune as one run (PLAN rule 7c): the surface, the fixed values, the
>    binary hash and fingerprint, the slots, the ceiling in games, the perturbation and
>    `r_end`, the checkpoints and the estimator, before launch. The test and the ceiling
>    never change after the first game.
> 6. Theta is the rounded centres at the checkpoint where the tune stopped, without
>    selection among checkpoints. The fit is gated once, theta against the seeds on the
>    tune binary. Where the tune's own read is +10 Elo or more, the gate is a fixed
>    6,000 games and passes when its 95% interval excludes 0 (BAS-S56's rule). Otherwise
>    it is the `[0,3]` SPRT. Bake into a fresh clean PGO binary whose `bench 13`
>    reproduces the tune binary's count with theta set.

**Why the gate has two forms.** A fixed 6,000 games reads ±5.3, so it passes a true +10
with 96% and a true +5 with 46%. B.5.4's +10.8 passed it where the SPRT would have needed
about 7,200 games; B.4.6's fit took 12,632 games by SPRT and would have been a coin flip
at 6,000. The tune's own read chooses the instrument before any gate game, so the choice
cannot follow the gate's result.

**What it needs built** (an `I1` leaf, after sign-off):

- `tools/spsa_read.py`, replacing `tools/spsa_block_rule.py`, with tests on synthetic
  journals of both verdicts and on the five real blocks (`stop.txt` is its fixture).
- `tools/generate_spsa_surface.py`: the doubled perturbation for a registered ceiling,
  with the range exception.
- `tools/spsa_colosseum.ps1`: the ceiling, `r_end` and checkpoints as arguments in place
  of the fixed 2,000; a loop that runs to each checkpoint with Colosseum's
  `--stop-after-iteration`, applies the read, and resumes or stops. Colosseum needs no
  change for this.

## 9. Should Colosseum's SPSA change?

The maintainer asked whether Colosseum should gain a variable step size and stop rules:
a maximum of iterations, no moving parameters, no gradients. I have read Colosseum's
command line only, not its source.

**Stop rules.**

- *Maximum iterations.* It exists: the horizon, and `--stop-after-iteration`.
- *No moving parameters.* Not sound as a stop. Movement is gain times result. Under a
  decaying gain every coordinate stops moving whether or not it has arrived, and under a
  held gain the noise keeps every coordinate moving for ever. Movement means something
  only against its null, and that is the gradient test.
- *No gradients.* Sound, and it is the right criterion: at an optimum the gradient is
  zero, so "no gradient above a stated floor" is what convergence means. The mathematics
  holds because the test's null is exact (§3) and because the rule reads only past
  iterations. A stop chosen from past data does not bias where the centres are; it is
  not a selection on Elo. Its weakness is power, not validity: quiet means "under the
  floor", so the floor is registered and reported.

  The textbook version is Pflug's diagnostic (the running inner product of successive
  gradient estimates turns negative at stationarity), on which Chee and Toulis built a
  stopping test for a held gain. Later work found its variance too large to be reliable
  even on a quadratic. The windowed test of §3 is stronger here for one reason: SPSA
  knows the signs it drew, so each coordinate gets its own exact test.

**Variable step size.** Three different things carry that name.

- *A step per coordinate.* It exists: each coordinate has its own `c_end`, and the move
  scales with it. Sizing those steps better is §11's open read.
- *A held gain.* Today the perturbation and the move decay on a schedule fixed by the
  horizon. Holding both constant until the stop test fires is supported by theory: a
  held gain converges to a neighbourhood of the optimum whose size is set by the gain,
  and averaging the centres over the stationary stretch recovers the best attainable
  rate (Polyak and Juditsky). Colosseum already has the averaging estimator
  (`--final-window-percent`). A held gain makes every window comparable and removes the
  horizon from the schedule, which a run with a stop rule no longer knows in advance.
  This is the change I would make first.
- *A gain adapted per coordinate.* Raising the gain of a coordinate whose gradient is
  established and decaying the rest is Kesten's rule, and its convergence is proved: a
  gain that depends only on past data keeps each update unbiased. The risk is variance,
  not bias: a coordinate boosted on a chance reading walks further. Per iteration our
  signal is a few percent of the noise, so adapting on single iterations, as RSPSA does,
  would adapt on noise. Adapting on a window's z past the familywise line is safe. It
  would roughly halve the iterations a starved coordinate needs. It is a real saving and
  a real mechanism to get wrong, so it comes second.

**Does it make sense to put this in Colosseum?** Yes, in two stages.

1. **Now, outside Colosseum:** §8's read tool and wrapper loop. It needs nothing from
   Colosseum, and it lets one or two tunes score the test and the forecast first.
2. **After the read has held on a real tune:** move the test into Colosseum's driver and
   its `spsa status`, add the held gain with the windowed average, and then consider the
   per-coordinate boost. The reasons to move it: an unattended run stops itself, the
   rule and its record have one implementation, and Rarog shares the runner.

Putting it in Colosseum first would fix a test in the shared runner before any tune has
used it. Colosseum is pinned here (`tools/colosseum/colosseum.pin.json`), so stage 2 is a
change in that repository and a re-pin, on the maintainer's decision.

## 10. Falsifiers

- **The stop test.** A tune stopped as quiet, then a later tune that takes +8 Elo or
  more by its gate chiefly from the same coordinates, says the floor is too high: the
  window lengthens or the perturbation grows.
- **The doubled perturbation.** Clamped perturbations on more than 1% of iterations for
  any coordinate, or a correctness floor failing at seed plus or minus two steps, sends
  that coordinate back to the single step. A gate that reads below the tune's own read
  by more than twice their combined error says the central difference is biased at that
  width.
- **The forecast.** Scored at every checkpoint against the tune's own read of the next
  segment, and at the gate against the match. Two of the first three outside twice
  their SD and it is dropped from the record.
- **The convergence model (§5).** It predicts that the coordinates named there keep
  their direction in the next tune that holds them, and that a tune run to a quiet read
  ends that drift. A quiet read followed by the same drift in the next tune contradicts
  it.

## 11. Not run, and what is owed

- **Steps sized by effect.** Whether the change in `bench 13` at seed plus or minus one
  step predicts which coordinates carry gradient, tested against the 150 z values in
  hand. Zero games, minutes on the tune binaries. If it holds, the generator can size a
  step by tree effect; if not, `range / 16` stays. Its prediction is frozen before it
  runs.
- **The doubled perturbation's bounds.** For B.6.7's surface: which coordinates leave
  their range at two steps, and `bench 13` and WAC at both ends. Zero games; it belongs
  to that tune's registration.
- **A games test of old method against new** is not proposed. It costs two tunes and
  resolves only a difference of about 8 Elo. The next tune's scored reads are the test.

## 12. Separate findings

- **B.5.4 and B.4.6 both ended with live gradient.** `B3SingMargin` read z −3.63 over
  B.5.4's last 1,000 iterations and `B3NmpPerPly` −3.20 over B.4.6's. The forecast puts
  B.5.4's withdrawn block 2 at +15.1 ± 2.3 at most; halved, about +7. The decision to
  withdraw it stands. **Owner:** B.6.7's surface, which starts with both known live.
- **Rule 7c stopped B.4.6 one block early** by this read. No action: B.5.4 carried
  `B3NmpPerPly` further.

## 13. Reproduction and sources

`tools/results/b56-20261112/`, Python 3, run in that directory, reading
`tools/results/b*-spsa-block*/result.json`:

| Script | Hash | Output | Hash | Reads |
|---|---|---|---|---|
| `load.py` | 1f46618f… | – | – | the journals |
| `noise.py` | 1422ad14… | `noise.txt` | e3b5bae1… | §2, §3 |
| `unlock.py` | 9a63110c… | `unlock.txt` | 027b0ad3… | §4 |
| `gain.py` | e3236c09… | `gain.txt` | deb9f45d… | §5, the tune's own read |
| `forecast.py` | c9f7bcb1… | `forecast.txt` | 3b00c78b… | §5, capacity and forecast |
| `stop.py` | 296af29a… | `stop.txt` | fecf3b59… | §7, §8, the null's check |

Outside sources, read 2026-11-12 through summaries of the pages, not the pages
themselves:

- fishtest wiki, *Creating my first test* and *Fishtest FAQ*; fishtest
  `server/fishtest/spsa_workflow.py` for the constants and the update.
- OpenBench wiki, *SPSA Tuning Workloads*.
- chessprogramming.org, *SPSA* and *Stockfish's Tuning Method*.
- Kocsis and Szepesvári, "Universal parameter optimisation in games based on SPSA",
  *Machine Learning* 63 (2006): abstract only.
- Ivec and Vojnović, "Bayesian statistics approach to chess engines optimization",
  arXiv 2205.15602.
- Chee and Toulis, "Convergence diagnostics for stochastic gradient descent with
  constant step size", arXiv 1710.06382: abstract, and later abstracts on the Pflug
  statistic's variance.
- Kesten's rule and Polyak–Juditsky averaging are cited from memory, not re-read.
