# Rarog import, 2026-11-21: the reported score, the clock and the threads

- State / class: delivered; the source of the PLAN changes of 2026-11-21, `R2`
- Owner / date: agent, 2026-11-21, at the maintainer's request ("check the
  latest changes in Rarog on its branch where it rebuilt the search in
  Reckless form… port all into proper places in PLAN"; the calibration and
  win-draw-loss output "in Stockfish form, as always")
- Decision taken: maintainer, 2026-11-21, in the request. The score
  calibration and `UCI_ShowWDL` go to the end of Phase C, in Stockfish's
  form, so that 1.12.0 ships them.

**Recommendation.** Take the score calibration now. It is output only,
cheap, and makes every release from 1.12.0 print a score that means
something: +1.00 is an even chance. The other items are priors for leaves
that already own the question (D.1, D.2, C.2.10, C.11, H.2). They add no
step.

Every claim is evidence, read by the session in the live repository unless
marked otherwise. Rarog's verdicts are priors: its donor is Reckless and
its evaluation its own (AGENTS, `agents/research.md`).

## 1. Scope and method

- **Rarog**, the live repository at `c6ff7fc` (branch `d7-kernel`,
  2026-11-21), under the exception in `agents/research.md` (an import the
  maintainer asks for). The last import read `8c0e4cb` (2026-11-11,
  `analysis/rarog_import_2026-11-11.md`). What is new since is D.7, the
  search rebuilt whole as one compact kernel in Reckless's form (D.7.0 to
  D.7.15), and the cards D.13 to D.15.
- **Read:**
  - Rarog's `docs/PLAN.md` at `c6ff7fc`: D.7.2.1, D.7.8 and its table,
    D.7.8.12, D.7.9.1, D.7.12, D.13, D.14 and D.15.
  - `analysis/d712_score_2026-11-21.md`.
  - `docs/experiments/RAR-O10.md`, and the summary lines of RAR-M90.
  - The commits `78d4681` (the fit tool) and `61c9c51` (the output).
- **Stockfish**, the local checkout at `1e7e6bde` and its tag `sf_19`,
  `src/uci.cpp` (`win_rate_params`, `win_rate_model`, `to_cp`, `wdl`):
  Basilisk's donor form for the output.
- **Basilisk** at `74729cd`: PLAN, `src/search_root.cpp`
  (`format_info_line`), and a search of `src/` for a WDL or a score scale
  (none).
- No engine was built or run.

## 2. Items and where they go

| # | Rarog item | Basilisk placement |
|---|---|---|
| 1 | D.7.12: the score printed on a fitted scale, and `UCI_ShowWDL` | New leaf **C.13.1**, in Stockfish's form; F.6, G.2 and E.3 adjusted |
| 2 | D.7.9.1, RAR-R34: one donor's time manager whole, in place of a hybrid | **D.1** input |
| 3 | D.13: correction tables shared between threads, the card | **D.2** input |
| 4 | D.14: the donor's vote withdrawal, its zero-game screen | **D.2** input |
| 5 | D.7.8.12: the table prefetch moved to where the move is picked | **D.2** input, beside its prefetch question |
| 6 | D.7.2.1, RAR-O10: Basilisk's evaluation against a reference | **C.2.10** prior and a **C.11** zero-game re-read |
| 7 | D.7.8's tests: the picker's skip drops quiet checks | **H.2** input |

### 2.1 The reported score (item 1)

- **Rarog** (D.7.12, `61c9c51`): an ordinary score prints as `100 · x / a(m)`
  at the root's material, `m` = pawns + 3 minors + 5 rooks + 9 queens,
  clamped to [17, 78], with `a` and `b` cubics in `m / 58`. `UCI_ShowWDL`
  (check, off by default) adds `wdl W D L` in permille. The constants were
  fitted on its own 2,000 games (97,741 positions), and its tool
  (`tools/wdl_fit.py`) checks that the source holds the fitted constants.
  The even chance falls at +100. The largest misfit is next to zero: draws
  run 0.908 against 0.863 predicted, because the model has no term for a
  proven draw. Bench is unchanged, mates and tablebase results print as
  before, and the search never reads the printed score.
- **Stockfish** (`sf_19`, and dev `1e7e6bde` unchanged): the same form,
  material clamped to [17, 78] and anchored at 58. `to_cp` is
  `round(100 · v / a)`. The win rate is `1 / (1 + exp((a − v) / b))`, the
  loss the same at `−v`, the draw the rest. The constants come from
  fishtest's LTC games (the `WDL_model` repository). A tablebase result
  prints as `cp ±(20000 − plies)`.
- **Basilisk today:** `format_info_line` prints the internal score
  unchanged, held below the tablebase band. Tablebase results already
  print as `cp 20000 − n`, which is Stockfish's form. There is no WDL
  output.
- **Placement.** Its own leaf, C.13.1 (`I1`), the release's first sub-step.
  - It runs after C.11 freezes the evaluation, and C.12 is bit-exact, so
    the constants fitted there are the release's.
  - Phase D's clock, threads and tablebase policy move neither the
    evaluation nor its scale, so E.3 checks the constants rather than
    refitting.
  - F.6 refits them per network, as it already plans. It was until now the
    first fit.
  - Form: Stockfish's whole, the material formula, the clamp, the
    anchor, the cubics, the option and the field. Only the constants are
    Basilisk's own, fitted on its own games, as Stockfish fits its own.
  - The tool is new Basilisk Python. Rarog's `tools/wdl_fit.py` is the
    method reference, and no Rarog value crosses.

### 2.2 The clock (item 2)

- **Read** (RAR-R34). Rarog replaced its hybrid clock with Reckless's time
  manager whole: the budget, its horizon and the five factors. The guards
  stayed: the reserve under `Move Overhead`, the proved-mate stop, the
  first-iteration guard, the tablebase box, the single-move shortcut and
  the `go` semantics.
  - The port read **+30.0 ± 8.8 Elo** at `3+0.03` and **+18.6 ± 8.2** at
    `10+0.1` against the hybrid, with no fault or time loss, nor in 200
    games at four threads.
  - It was predicted at +4 and +3, a miss in magnitude. The hybrid's early
    under-spend (0.60 of the base in the first twenty moves against the
    port's 0.75) cost far more than a null horizon read had suggested.
- **The census** that found it, at zero games: 30% of the hybrid's time was
  spent past the optimum, and 6.1% of its moves took three times the
  optimum or more (`analysis/artifacts/d79-clock-2026-11-20/over_optimum.py`
  in Rarog).
- **For Basilisk.** D.1 already builds Stockfish's shape whole as one arm.
  The import adds the prior that the whole form can be worth far more than
  its parts suggest, and the census as a zero-game read before the arms.
  It also names the pieces of Stockfish's time manager a partial port tends
  to miss: its per-control scale, its cut when behind on the clock, and
  `increaseDepth`. Those are the pieces Rarog's hybrid lacked.

### 2.3 The threads (items 3 to 5)

- **D.13's card.**
  - Reckless shares its pawn and non-pawn correction tables between
    threads, one copy per NUMA node. A read is a relaxed load and an update
    a relaxed load and store, so a racing update may be lost but is never
    torn.
  - At one thread the code is the same `mov` (bench exact), and the cost
    is a pointer load a node.
  - The four-thread acceptance read is costed at 13 to 18 hours for an
    SPRT `[0,5]`, or 1 hour 45 minutes for a no-harm read.
  - Stockfish shares correction, pawn and continuation histories per NUMA
    node (`SharedHistories`, `history.h` at `1e7e6bde`). D.2 already lists
    shared correction as a design question, and the card is its prior for
    form and cost.
- **D.14's screen.** A thread that has voted to stop withdraws its vote
  when its soft limit stops being met; Rarog's thread keeps it. A `diag`
  counter at four threads, on 300 positions at `3000+30`, counts the
  searches in which a withdrawal would have happened. Under 2% means
  `NO_CHANGE`. It is D.2's soft-stop voting question at zero games.
- **D.7.8.12's prefetch.**
  - Prefetching the child's table cluster before the make, from the key the
    move will produce, read +1.09%.
  - Moving that prefetch up to where the move is picked, before the pruning
    rules, read a further +1.12% (pooled PGO NPS). The pass read +2.71% in
    all.
  - Prefetching the correction and pawn entries as well read nothing once
    the table's line was hidden.
  - Basilisk's prefetch across the make (B.8.10's X0) is the first form,
    and D.2 asks for a cheaper one. The second form is a candidate there,
    read under PROCESS's rule for a memory item.

### 2.4 Basilisk's evaluation against a reference (item 6)

RAR-O10 (read 2026-11-14, zero games) scored each evaluation alone against
Stockfish 19's expected score, `(W + D/2) / 1000` at 1,000,000 nodes, on
10,000 positions from a 52,800-game round robin. The anchors were Stockfish
11's classical evaluation and Rarog's Phase C start. Skill is
1 − cross-entropy / the constant predictor's, on the 2,428 quiet positions
of the report half:

| Evaluation | Skill | Spearman |
|---|---|---|
| Stockfish 11 classical | 0.2054 | 0.785 |
| Rarog's head | 0.1769 | 0.747 |
| Rarog's Phase C start | 0.1700 | 0.714 |
| Basilisk's head (2026-11-14) | 0.1646 | 0.713 |

- **Whole engines at 100 ms a move.** Basilisk's moves lose ×1.078 Rarog's,
  an interval that includes zero. They lose clearly more only with 45 or
  more points of non-pawn material: +0.0058 [+0.0015, +0.0103].
- **It agrees with C.0.** Most of the same-search deficit sits at phase 18
  to 24, which is the most material on the board. It is a reference loss,
  not Elo: Rarog's distillation improved its holdout loss and lost 17 Elo
  (RAR-X03).
- **Placement.** A prior for C.2.10's order, and a zero-game re-read for
  C.11. Its positions and labels are rebuilt from Basilisk's own games
  under the same recipe, because Rarog's artifacts are not imported.

### 2.5 Quiet checks after the pruning skip (item 7)

Once late-move pruning or futility fires, Reckless's picker (and
Stockfish's, `skip_quiet_moves`) skips every quiet move left, direct checks
among them. On WAC positions and their continuations, Rarog's kernel then
plays a move that allows a mate in one in 1 of 123 positions where some
move avoids one, at every depth to 7. Its old kernel kept the checks and
did not. Rarog left it as an ignored test (`no_mate_in_one_is_allowed`)
because the skip is the donor's form. For Basilisk it is a refinement
candidate behind its donor's form, so it goes to H.2.

## 3. Read and not imported

- **D.7's kernel rebuild itself** (Reckless's shape): Basilisk's Phase B is
  the Stockfish-shaped equivalent, and RAR-O11 found Stockfish dev's search
  on Rarog's evaluation 34.3 ± 12.0 below Reckless's unfitted. That is a
  verdict about Rarog's donor choice, not a Basilisk step.
- **D.7.8.1 to D.7.8.11:** Rarog's own kernel repairs, each a departure
  from Reckless. D.7.8.9's three small speed items (+0.95%) are Rarog code
  forms.
- **D.7.11's finishing fit:** Rarog's SPSA on the kernel, its method
  already Basilisk's (rule 7c's replacement, curvature sweep).
- **D.10** (Linux large pages), **D.11** (originality disclosure) and
  **D.15** (release packaging from Basilisk 1.11.0): Basilisk's own
  practice, or work travelling back from it.
- **RAR-M90:** Rarog's pre-NNUE meter, 96.6 below Stockfish 16 HCE in its
  field and −100.6 head to head. It is context for C.2.10's frontier pair,
  not an input.

## 4. Separate finding (not acted on)

G.2's TA-2 item keeps "tablebase wins printed as `cp 20000` minus the
distance". `format_info_line` (`src/search_root.cpp`) already prints a
tablebase result that way. The item may be done already; G.2's owner
checks it.
