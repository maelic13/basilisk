# B.11.3 — originality scan and disclosure for 1.11.0 (2026-11-18)

The question is what of the donors' material is in the 1.11.0 release
candidate, and what the release must name.

This is the lean scope the maintainer chose on 2026-11-18 (PLAN B.11.3):

- one scan, for shared code and numeric tables;
- a disclosure by category in README;
- the licences shipped with every asset.

The constants census, the evaluation-table scan and the move-similarity
screen are not run here; C.13, E.3 and F.9 decide again for their
candidates.

This file is also the donor list a tournament registration asks for (§4).

## 1. Method

- **The tool.** `tools/diag/originality_scan.py`, ported from Rarog's tool
  of the same name (the live repository at `042e2b9`, read at the
  maintainer's request) with 14 tests.
  - The method and its positive control are Rarog's
    (`analysis/originality_review_2.6.0_2026-10-22.md` there).
  - The port extends the token runs to C++.
  - It reports:
    - runs of identical numbers;
    - numeric tables equal up to a scale factor;
    - runs of identical tokens, both raw (20 or more) and with identifiers
      normalised (40 or more).
- **The candidate** is the frozen search head on `b11-checkpoint-release`,
  `src/` exported by `git archive`: 76 files, 127,626 tokens. Its engine is
  `4d2742f`'s less the ablation flavour, at `bench 13` 4,777,412.
- **The donors**, `src/` exported the same way from `D:/code/stockfish`:
  - `sf_19` (`edb0d9db`): 72 files, 129,774 tokens;
  - `9587eeeb`: 39 files, 68,042 tokens.
- **Command:**
  `python tools/diag/originality_scan.py <out> <cand> sf19=<sf19> sfclassical=<sfclassical>`.
  It takes seconds.

| Against | Non-trivial numeric runs | Scaled tables | Raw token runs (≥ 20) | Normalised token runs (≥ 40) |
|---|---:|---:|---:|---:|
| `sf_19` | 121 | 6 | 17 | 31 |
| `9587eeeb` | 60 | 0 | 10 | 27 |

Every match of note was read side by side with the donor's lines.

## 2. What the matches are

**Coincidence or convention, nothing to name.**

- Most numeric runs:
  - runs of zeros, `0..7` counters, bit masks and file and rank constants;
  - also `{1, 1, 9, 7, 7, 9}`, the shift amounts of the eight directions.
- Most normalised token runs: `#include` lists at the top of files, and
  the square enumeration.
- All six scaled tables: short runs of small integers, such as tablebase
  and SIMD tables against unrelated tables here.
- Generic C++, such as a thread's deleted copy constructors and
  `steady_clock` timing.
- **The bench positions** are Basilisk's own suite, kept identical to
  Rarog's; the standard perft positions open it. Stockfish's bench list
  differs. The match is only an array of strings.
- **The magic numbers and Zobrist keys** are Basilisk's own: the output
  of a seeded splitmix64 search (`src/magics.h`, `src/zobrist.cpp`).
  splitmix64's three constants are the public-domain generator's.

**Data from the donors, used unchanged.**

| What | Basilisk | Donor | Note |
|---|---|---|---|
| The time manager's formula and its constants | `src/search_worker.cpp:97` | `sf_19` `src/timeman.cpp:117` | known since 1.10.2's audit; Phase D rebuilds the time manager |
| The 16-entry LMR history divisor table | `src/search_kernel_core.cpp:140` (`LMR_HISTORY_DIVISOR`) | `sf_19` `src/search.cpp:55` (`lmrDivisor`) | B.6.7 fitted one scale on it, `LitLmrHistDivScale`; the entries are the donor's |
| The continuation-history consistency multipliers | `src/search_history_core.cpp:30` (`CONT_CONSISTENCY`) | `sf_19` `src/search.cpp:2022` (`CMHCMultipliers`) | seven values |
| Stockfish's piece values inside the histories | `src/move_picker_core.h:33` (`HIST_PIECE_VALUE`) | `sf_19` `src/types.h:199` | by design: the histories keep the donor's limits, so their material terms are the donor's units |
| Several endgame scaling rules, around KRP-KR | `src/eval.cpp:635–701` | `9587eeeb` `src/endgame.cpp:432–497` | the rules' conditions and constants as classical Stockfish states them |
| Part of a pawn piece-square row | `src/eval_params.h:88` | `9587eeeb` `src/psqt.cpp` (pawn, endgame) | six values; the rest of the tables are PeSTO's (1.10.2's audit, `analysis/pst_orientation_2026-10-23.md`) |

- **Fitted from the donor's seeds, not unchanged.** B.6.7's joint fit
  moved 93 of the 118 screened kernel literals and 61 of the search lines.
  Literals converted at B.0's scale ratio (0.282) were never equal to the
  donor's numbers.
- The literals the fit left out, or rounded back to their seed, keep the
  donor's value. The disclosure names them as a category: "some are still
  Stockfish's". `analysis/b67_kernel_literals.md` lists each literal with
  its donor line.

**Code close to the donor's.** These are small routines, each with one
natural form, which PROCESS's independence boundary allows:

| What | Basilisk | Donor |
|---|---|---|
| Windows large-page privilege set-up | `src/large_pages.cpp:59–80` | `9587eeeb` `src/misc.cpp:329–360` |
| The cuckoo table of reversible moves, for upcoming-repetition detection | `src/zobrist.cpp:68–79` | `9587eeeb` `src/position.cpp:137–149` |
| The compile date parsed from `__DATE__` | `src/version.cpp:26–35` | `sf_19` `src/misc.cpp:150–159` |

No longer stretch of code matched. The search kernel's largest match is the
six-entry continuation-history array (55 normalised tokens). The code is
Basilisk's own, in its own structure.

## 3. Verdict and disclosure

- **Verdict.** 1.11.0 is an independently written engine: Stockfish's
  design, reimplemented. It carries donor data unchanged (§2) and a few
  small routines close to Stockfish's. Both are named in README's
  acknowledgements by category.
- **Licences.**
  - Basilisk and Stockfish are GPL-3, and Fathom is MIT.
  - PeSTO's tables were published under CC BY-SA 3.0, which asks for
    attribution; README gives it.
  - From this release, every release ships the GPL text and Fathom's
    notice as assets beside the executables (`.github/workflows/release.yml`).
    No earlier release carried either.
  - This is not legal advice. The maintainer confirms that nothing found
    needs more than naming.
- **Not disclosed, by maintainer decision (2026-11-18):**
  - Rarog, the maintainer's own engine;
  - the oracle, which never ships;
  - Triumviratus, which gave mechanisms and measured lessons only.

## 4. Donor list for a registration

- Stockfish 19 (`sf_19`, GPL-3):
  - search design;
  - time manager;
  - search constants as seeds, some unchanged;
  - three tables unchanged.
- Classical Stockfish (`9587eeeb`, GPL-3): a few endgame scaling rules and
  part of one piece-square row.
- PeSTO (Ronald Friederich, CC BY-SA 3.0): the piece-square tables.
- Fathom (MIT): Syzygy probing.

Repeat the scan at each release check (C.13, E.3, F.9). Phase C brings in
classical evaluation families, so C.13 adds the evaluation-table scan.
