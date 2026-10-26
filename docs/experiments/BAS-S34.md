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
