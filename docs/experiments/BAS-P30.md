# BAS-P30

**BAS-P30 — the board rebuild's set in the whole engine (B.7.6)**

- **Registration.** 2026-11-14, B.7.6, before any read. Agent-built pools; the read
  runs on the idle host the maintainer gives. Behaviour-neutral: `bench 13`
  4,247,965 on both arms, and the `bench 13` board trace of the head is byte-identical
  to the base's (SHA-256 4C6643CD9CB43460…).
- **Question.** Do B.7.6's four board components together make the engine faster?
- **Arms.** Four-build final-PGO pools, clang 23.1.5, PEXT (`tools/nps_pool.ps1`):
  `b76-base` at `fb91be6`, the commit before the first component, and `b76-head` at
  `0c978c2`.
- **Changes.** `1097ee8` (C1): unmake without the key work it discards. `eccf9b7`
  (C3′): SEE pins read from aligned sliders. `5e40371` (C5): repetition scans from four
  plies back. `0c978c2` (C8): `is_in_check` and `has_non_pawn_material` in `board.h`.
- **Prediction.** Frozen by the frontier review (`analysis/b7_frontier_review_2026-11-14.md`
  §3) for C1 and C3′ together: +2.0%, 80% interval [+0.8%, +3.2%]. C5 and C8 are
  expected cost-neutral.
- **Method.** `tools/nps_read.ps1 -Base b76-base -Cand b76-head`, the gain form
  (PROCESS *NPS*): step 1, two cycles, accepts at +0.9% or more and is NO_CHANGE at
  +0.1% or less; otherwise step 2, six cycles, accepts at +0.5% or more with the lower
  bound above 0.
- **Verdict rule** (PLAN B.7 and B.7.6, frozen before the read):
  - At or above +0.5%, the set stays and B.7.6 closes.
  - Under +0.5% falsifies the engine screen as a predictor of pooled NPS: B.7.6
    returns to research (R3), and its members are read alone (D-16); a member that
    does not pay returns to the original form.
  - Below zero is a red flag: the research reads the search's side as well as the
    board's (profiles of both arms by region, the board's callers included, and the
    search's board calls on both) before anything is kept or reverted.
- **Screens before it** (PLAN B.7.6): C1 against the base and C3′ against C1 on the
  search replay (`tools/diag/board_replay_run.py`) and the engine screen
  (`tools/diag/engine_ab.py`, 16 rounds). A component stays when its replay class is
  faster by more than twice the spread of the untouched classes and the screen is
  faster in at least 13 of 16 rounds.
- **Status.** Registered; pools built, the read waits for the idle host.
