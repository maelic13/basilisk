# BAS-S49

**BAS-S49 — B.4.2: SEE follows pins opened during the exchange, `[-5,5]`**

- **Registration.** Registered 2026-11-05, before any game; maintainer-run;
  correctness repair A-2 of PLAN B.4.2 (BAS-C09's created-pin half, under
  the known-defects rule of 2026-10-26). Its own gate, as the leaf
  requires; it plays on top of A-3 (BAS-S48), the order the repairs were
  committed in.
- **Defect.** Both SEE kernels computed pins once, on the occupancy after
  the first move. A pin opened by the mover's own departure was honoured;
  one opened by a later capturer's departure was not, so the pinned piece
  still recaptured: `see_ge` read −400 for the fixture's −300
  (`2k5/2n5/2B1p3/3p4/8/8/3R4/2R4K w`, Rxd5).
- **Candidate.** `basilisk-b42-pins-pext-pgo.exe`, revision `d3add9c`: a
  departure that shares a line with either king and an enemy slider of
  that line's kind recomputes the pins (`Board::see_departure_opens_line`,
  a cheap superset test, so the recompute runs only where a pin can
  appear).
  - Release PEXT final PGO, clean manifest; `bench 13` 6,090,091;
    SHA-256 `ABF97BE1…`.
- **Baseline.** `basilisk-b42-promo-pext-pgo.exe`, revision `9aaa528`
  (BAS-S48's candidate); `bench 13` 8,532,389; SHA-256 `416A153C…`.
- **Mechanism.** A missed pin lets an illegal recapture into the exchange,
  so `see_ge` reads a capture as worse for the mover than it is (or a
  quiet move as more exposed) and the pruning rules that read it drop a
  move that wins material. The repair only removes illegal recapturers.
- **Invariant that fails on the old behaviour.** `tests/test_board.cpp`
  "created pin: opened mid-exchange" reads −400 on the old `see_ge` and
  −300, the oracle's value, on the candidate. The quiet corpus's
  legality-filtered geometry falls from 11 to 7.
- **Measured exposure.**
  - On the 104,000-move random-walk corpus, 16 verdict boundaries change,
    all wrong before and equal to the oracle after. Every one of the 138
    divergences left (3 bench roots, 135 walk moves) has a check on the
    oracle's exchange: no pin geometry remains.
  - In search, the candidate's `bench 13` tree with the old kernel run
    beside the new one: 100 of 7,660,016 `see_ge` calls change verdict
    away from a back rank (0.0013%), where only a pin can change it; the
    other 5,126 are on a back rank.
  - Each bench position searched alone from fresh tables at depth 13: no
    tree changes. `bench 13` moves −29% only through the shared table and
    histories.
  - The exposure is small, so the cap is the one `agents/measurement.md`
    gives a change that touches few games.
- **Competing explanation for a neutral reading:** the geometry needs two
  of one side's pieces on a king's line with an enemy slider behind them
  and an exchange that moves the nearer one, so it is too rare to read in
  1,500 pairs; a null is the expected reading.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 Elo**; most likely the cap without a decision.
  Confidence that the result is not H0: about 90%.
- **Verdict rule:**
  - H1 or the cap without a decision: the repair stays (the cap as a
    correctness fix with no measured harm); report the LLR and the
    estimate, and the maintainer confirms;
  - H0: the wrong form is not restored (PLAN *Known defects*); the cost is
    first sought in the recompute's speed, then the leaf returns to
    `RESEARCH`; the maintainer decides.
- **Correctness floors, read before the result is acted on:** BAS-S48's
  (§QUALIFICATION there), which were read on this candidate.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 49001; concurrency as the wrapper resolves it.
- **Cap: 1,500 pairs.**
- Launcher `tools/run_b42_pins_gate.ps1`; results in
  `tools/results/b42-pins-gate/`.
- **Dry run:** DRYRUN_PINS

#### RESULT — append after exposure

#### PREDICTION CALIBRATION — preserve the original prediction above
