# BAS-S57

**BAS-S57 — B.5.6: two zero-game reads for the SPSA method (steps by effect; the doubled perturbation)**

- **Registration.** Registered 2026-11-12, before any point of either read; agent-run;
  zero games; tooling and method, no Elo layer. The reads B.5.6's packet left owed
  (`analysis/b56_spsa_method_2026-11-12.md` §11), run on the maintainer's instruction
  after rule 7c's redesign was recorded.
- **Subjects.** The three tune binaries whose blocks are on disk and which still exist,
  each at its block's seeds, which are its advertised defaults:
  - B.3.3: `basilisk-b32-on-tune-pext-tune-pgo.exe` (B757C936…), 29 coordinates;
  - B.4.6: `basilisk-b45s-on-tune-pext-tune-pgo.exe` (FDB8D28C…), 15 coordinates;
  - B.5.4: `basilisk-b54-on-tune-pext-tune-pgo.exe` (D3E1B9E9…), 8 coordinates.

  B.2.3's binary (`basilisk-b23b-on-tune-pext-tune-pgo.exe`) is no longer on disk, so
  its 49 coordinates are outside the read. 52 coordinates in all.
- **Instrument.** `bench_replay.py` (the `bench 13` positions over UCI, so options apply)
  with one option moved to its seed −2, −1, +1 and +2 generator steps, clamped to its
  range, and the defaults as control: 4 points a coordinate, 208 points and 3 controls.
  Node counts are deterministic, so the points run in parallel. A control must equal
  the binary's own `bench 13`.
- **Read E — steps sized by effect.**
  - *Question.* Does a coordinate's tree effect at one step predict whether its tune
    found a gradient? If it does, the generator can size a step by effect instead of a
    sixteenth of the range.
  - *Statistic.* Sensitivity `S` = the mean of `|ln(nodes at seed ± 1 step / nodes at
    the seed)|`. Against it, `|z|`, the coordinate's gradient z over its whole block
    (`tools/results/b56-20261112/noise.py`). Spearman's rank correlation, within each
    block and pooled over the three after ranking within blocks.
  - *Competing explanation.* Node sensitivity is not Elo sensitivity (PLAN rule 8): a
    coordinate can move the tree a great deal near its optimum and show no gradient,
    and a gradient depends on the distance from the optimum, which `S` cannot see.
- **Read B — the doubled perturbation.**
  - *Question.* Is a perturbation of two steps usable at the seeds: how many
    coordinates lack the room, and does the engine still search a sane tree at two
    steps?
  - *Statistics.* (i) On the head's 74 coordinates
    (`tools/generate_spsa_surface.py` over `src/search_params.h`, arm defaults
    applied): the count whose default ± 2 steps leaves its range. (ii) On the 52
    coordinates: the node ratio at ± 2 steps against the seed, and against the ratio
    at ± 1 step.
  - *Stated before registration:* the arithmetic of (i) was read while preparing this
    entry (10 of 74 lack the room; 3 of them lack it at one step). No prediction is
    claimed for it.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **E1.** Pooled Spearman correlation of `S` with `|z|`: expected **+0.25**, 80%
  interval [0.00, +0.50]. Probability it is +0.40 or more: 20%.
- **E2.** Of the coordinates with `|z|` of 2.5 or more over their block, at least half
  lie in their block's top third by `S`: 55%.
- **B1.** No engine fault and no rejected option at any of the 208 points: 90%.
- **B2.** At ± 2 steps the node count stays within a factor of 2 of the seed's for at
  least 90% of the points that are not clamped: 75%.
- **B3.** For coordinates with `S` of 1% or more, the mean `|ln ratio|` at ± 2 steps is
  between 1.5 and 3 times that at ± 1 step for at least 60% of them: 60%. Outside that
  band the tree's response is far from linear at this width.
- **Decision rules, fixed now.**
  - Steps are sized by effect only if E1's pooled correlation is +0.40 or more and at
    least two of the three blocks read +0.30 or more on their own. Otherwise the
    sixteenth of the range stays, and the packet's open item closes as not adopted.
  - A coordinate whose point at ± 2 steps faults, or leaves the factor of 2, is named;
    rule 7c's range exception is re-read against (i) and those names.
- **Falsifiers.** E1 below 0 says tree effect points away from gradient on these
  surfaces. B1 failing is an engine defect at a legal option value, reported at once
  and separately.
- **Conditions.** `tools/results/b56-20261112/steps.py`, eight points at a time, results
  in `steps.jsonl` and `steps.txt` beside it. No games, no NPS.

#### RESULT — append after exposure

- **Run (agent, 2026-11-12, zero games).** 211 points, eight at a time, no fault and no
  rejected option. Each control equals its binary's own `bench 13` (4,923,507,
  4,003,560, 3,920,652). `tools/results/b56-20261112/`: `steps.py` 76ce0219…,
  `steps.jsonl` 0efa7cbb…, `steps.txt` ba1a273e….
- **Read E.**

  | Block | Coordinates | Spearman of `S` with `|z|` | Live coordinates in the top third by `S` |
  |---|---:|---:|---|
  | B.3.3 | 29 | +0.02 | 0 of 1 (`B3NmpRedBase`) |
  | B.4.6 | 15 | +0.50 | 0 of 1 (`B3NmpPerPly`) |
  | B.5.4 | 8 | +0.52 | 1 of 3 (`B3SingMargin`; not `B5AspBase`, `B3NmpPerPly`) |
  | Pooled | 52 | **+0.24** | 1 of 5 |

- **Read B.**
  - No fault at any point.
  - At ± 2 steps, 96 of 97 unclamped points stay within a factor of 2 of the seed's
    nodes. The one outside: `B3NmpRedEvalDiv` at 44 (seed 72), 2.37 times the nodes. It
    is a divisor, so two steps down is a 39% cut and the reduction it sets grows by 64%.
  - 7 of the 104 points at ± 2 steps clamp, on five coordinates: `B3IirMinDepth` (in
    all three blocks), `B3NmpRedBase`, `B3SingTripleQuiet`, `CoreLmpBase`, `B5AspBase`.
  - On the head's 74 coordinates, 10 lack two steps of room at their defaults:
    - 1.5 steps: `CoreLmpBase`, `CoreQuietFutDepthCap`, `B3IirMinDepth`, `B5AspBase`;
    - 1 step: `CoreCaptFutDepthCap`, `CoreLmrAlphaEvalScale`, `B3NmpRedBase`;
    - under 1 step, so today's first perturbation already clamps:
      `CoreCaptFutHistDiv` (0.88), `CoreResearchShallower` (0.5), `B3SingTripleQuiet`
      (0.29).
  - The node ratio at two steps against one step lies between 1.5 and 3 for 8 of 52
    coordinates. Its median is near 1: `bench 13` answers a move of one step with a
    change of 2% to 34% whose size and sign barely depend on the step
    (`B3NmpPerPly` in B.3.3: −21.2%, −7.8%, +16.9%, −7.4% at −2, −1, +1, +2).
- **Decision rules applied.**
  - **Steps are not sized by effect.** The pooled correlation is +0.24 against the line
    of +0.40. A sixteenth of the range stays.
  - `B3NmpRedEvalDiv` is named. It stays inside its range and faults nothing, so rule
    7c's exception does not reach it; its two-step point is re-read with WAC in the
    registration of the next tune that holds it.
  - **Rule 7c's range exception is graded.** An all-or-nothing exception would put 10 of
    74 coordinates on the single step at half the gain ratio. Each coordinate instead
    takes the largest perturbation from one to two steps that its room allows, and a
    coordinate with under one step of room is named at registration.
- Disposition: **CLOSED 2026-11-12.** Steps by effect not adopted; the doubled
  perturbation runs clean and is graded by room.

#### PREDICTION CALIBRATION — preserve the original prediction above

- **E1** (pooled correlation +0.25, 80% interval [0.00, +0.50]): **hit**, +0.24. The
  20% given to +0.40 or more did not come in.
- **E2** (at least half the live coordinates in the top third, 55%): **miss**, 1 of 5.
  The coordinates that carried gradient are ordinary by tree effect.
- **B1** (no fault, 90%): **hit**. **B2** (within a factor of 2 for at least 90%, 75%):
  **hit**, 99%.
- **B3** (two-step response 1.5 to 3 times the one-step response for 60% of
  coordinates, 60%): **miss**, 15%. The model was wrong about the instrument, not shown
  wrong about the engine: it took `bench 13`'s node count for a smooth function of a
  constant at this scale, and it is not. Forty positions at depth 13 respond to any
  change of a pruning constant with a tree change of the same order, whatever the
  change's size. So B3 says nothing about whether the Elo response is linear at two
  steps; that stays with rule 7c's falsifier (a gate below the tune's own read).
- **What E's result means with B3's.** `S` at one step is mostly this chaos, which is
  why it predicts so little. A smoother proxy (a median over many positions, or
  move agreement) could be tried; it is not proposed, because the gradient test
  already tells a tune which coordinates matter, at no cost.
- Three of five, against an expected 3.6.
