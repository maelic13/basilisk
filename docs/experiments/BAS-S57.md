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
