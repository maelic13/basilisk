# BAS-S34

**BAS-S34 — the root underpromotion rule re-tested where it acts**

- **Registration.** Registered 2026-10-26, before any game; maintainer-run;
  the retry trigger of BAS-S33 (maintainer decision 2026-10-26).
- **Question.** Does the root rule BAS-S33 tested cost Elo in the positions
  where it changes the move? BAS-S33's `[-5,5]` SPRT on UHO openings ended
  at H0 (−2.7 ± 4.1), but the rule acts in only a few percent of those
  games, so that gate could not tell a small loss from 0.
- **Arms.** BAS-S33's, unchanged and pinned by SHA-256:
  - candidate `basilisk-a119-up-pext-pgo.exe`, revision `1641e64`,
    `0D80B72FB5B2196283F70CA2C8E25F97F7CA4B603460AA5C579860D587DBBCEF`;
  - baseline `basilisk-a119-fix-pext-pgo.exe`, revision `fdf4277`,
    `FE85A3EF8AA2BD784D87861601AAD0DDF122A219E321FA3B4807B6F3EBD10CE0`;
  - both `bench 13` 7,857,713. The source is reverted (`c1791b3`); a
    passing verdict re-applies `1641e64`'s engine change.
- **Book.** `tools/diag/promotion_choices_v1.epd`, 3,159 positions, built
  by `tools/diag/promotion_book.py` from the A.11.x gate games (`a111`,
  `a112`, `a116`, `a117`, `a119`, `a119-up` and `a818-sink` under
  `tools/results/`): of the 95,637 positions where the side to move may
  underpromote, those where the two arms' best moves differ at 100,000
  nodes, one thread, Hash 64. 1,404 are a queen against a rook or bishop;
  1,755 are other moves, because searching the queen first reorders the
  root wherever a promotion is legal. 5.95% of BAS-S33's games pass
  through one of these positions (in sample: the book includes them).
- **Mechanism.** As BAS-S33: the root plays the queen unless a rook or
  bishop promotion wins clearly more.
- **Competing explanations.** The positions are chosen where the arms
  differ at 100,000 nodes; at `3+0.03` a move gets about 50,000 to
  300,000, so some games start without a difference. Many positions are
  already decided, and their pairs are 1–1.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +10 Elo** on this book (Stockfish 18 rated
  BAS-S33's differing choices 21 better, 148 equal, 8 worse); H1 or the
  cap at about 80%.
- **Verdict rule:**
  - H1 (no worse): re-apply the rule, with its tests;
  - the cap without a decision: the loss on this book is then within
    about 5 nElo, under 0.2 Elo in ordinary play at 5.95% exposure, so
    re-apply it as a correctness fix and the maintainer confirms;
  - H0: the rule stays reverted, closed `NOT_WORTH_PURSUING`.
- **Correctness floors before re-applying:** CTest on both arms, the
  promotion sections of `test_search` included; `bench 13` 7,857,713.

#### CONDITIONS

- Colosseum SPRT, `sprt-removal.toml`: normalized `[-5,0]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, each book position once with colours reversed,
  random order, no adjudication, tablebases off; seed 34001; concurrency
  as the wrapper resolves it.
- **Cap: 3,159 pairs**, the whole book, so no opening repeats.
- Launcher `tools/run_a119_up_retest.ps1`; results in
  `tools/results/a119-up-retest/`.
- `tools/colosseum.ps1` checked `-Book` against the resolved run but never
  passed it, so the first dry run resolved to the UHO book and the policy
  check refused it. The wrapper now passes `--book` when a launcher names
  one; launchers that do not resolve as before (BAS-S33's dry run is
  byte-identical, `3980489f…`).
- **Dry run accepted 2026-10-26**
  (`tools/results/colosseum_sprt_a119-up-retest_20261026_220917.*`, dry
  run `2b0fcf2b…dad0`, manifest `7f78201d…8eca`): 3,159 openings
  scheduled, none reused. The book's SHA-256 is `084be1ad…5abe`, and
  `promotion_book.py` rebuilds it byte for byte.

#### RESULT — append after exposure

- **Cap without a decision (maintainer-run 2026-10-26):** **−0.3 ± 1.1
  Elo** on the book (nElo −2.2 ± 8.6), LLR +0.09 in [−2.94, 2.94] at the
  3,159-pair cap (6,318 games); W/D/L 2601/1111/2606, pentanomial [0, 53,
  3059, 46, 1].
  - 0 faults, 0 time losses; the independent PGN recount agrees. Both
    arms 3.23M nps, 252,000 nodes and 78 ms a move.
  - The run took a fresh dry run with the registered hash
    (`colosseum_sprt_a119-up-retest_20261026_221029.dry-run.json`,
    `2b0fcf2b…`); manifest `2c0f822e…`, run record `50703c05…`, PGN
    `2c58754a…`.
  - 3,059 of the 3,159 pairs went 1–1: the position, not the promotion,
    decided them. The other 100 split 47 to the rule, 53 against.
- **Verdict (registered rule):** the cap, so the rule is re-applied as a
  correctness fix, on the maintainer's confirmation (2026-10-26):
  `d7d2c34`, which restores `1641e64`'s engine and tests exactly; `bench
  13` 7,857,713, the legacy arm 23,696,970; CTest ON 24/24, OFF 21/21.
- Disposition: **cap, −0.3 ± 1.1; the rule is re-applied. CLOSED
  2026-10-26.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +10 Elo on this book; H1 or the cap at about
  80%.
- **Reading:** the cap, centred at −0.3, just below the predicted range.
  Hit on the outcome, slightly low on the centre: Stockfish's 21-to-8
  preference for the rule's choices did not show in results.
- **What it teaches:** even where the rule changes the move, the result
  changes in about 3% of pairs, evenly both ways; its value is correct
  moves, not Elo. Measured where it acts, the harm bound is ±0.07 Elo in
  ordinary play, which the UHO gate's ±4.1 could never give.
