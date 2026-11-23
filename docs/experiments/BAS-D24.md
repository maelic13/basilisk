# BAS-D24

**BAS-D24 — C.2.8.3, Amendment 2: the KQ-K and KR-K conversion grid on the top-tier repair**

- Date / owner / calibration category: registered 2026-11-23 in the C.2.8
  registration's §7.3, before the repair was built; agent, PLAN C.2.8.3;
  deterministic qualification (no playing verdict).
- Change: `cd4e453`. KQ-K and KR-K score their tier plus frozen material
  and classical Stockfish's `Endgame<KXK>` drive, in place of the winner's
  ordinary score.
- Builds:
  - the repaired head (`build/c283/head`);
  - A2's vector baked over it (`build/c283/A2r`).
  - Release preset, Clang 23.1.5, non-PGO; the instrument is node-limited
    and single-threaded per playout, so it is deterministic.
- Question: does the repair convert KQ-K and KR-K at every budget, and is
  its conversion independent of the fitted tables?
- Instrument: `test_endgames --conversion --nodes N --list`, the 24
  seeded positions per family, at 40,000, 50,000, 60,000, 70,000, 80,000,
  100,000 and 120,000 nodes (`tools/results/c283/amend2/`).

#### PRE-REGISTERED PREDICTION — frozen 2026-11-23 (the registration's §7.3)

- The grid passes, 24 of 24 for KQ-K and KR-K at every budget, on both
  builds: 0.8.
- The two builds give identical ply lists for KQ-K and KR-K: 0.95.
- Falsifier: any KQ-K or KR-K failure in the grid.

#### RESULT — 2026-11-23

- **The grid passes:** KQ-K 24/24 and KR-K 24/24 at all seven budgets, on
  both builds.
- **The ply lists are identical** between the two builds at every budget.
- At the registered 60,000 nodes every family passes on the repaired head:
  KQ-K mean 11.1 plies, KR-K 21.3, KBB-K 23.9, KBN-K 48.2.
- At 80,000 and 120,000 nodes the run exits 1 on KQ-K's staleness check
  alone. It mates faster than two-thirds of its ceiling (means 10.8 and
  10.2 against 10.87). Those bounds are calibrated at 60,000 nodes; this is
  not a conversion failure.
- Run 2's unconverted position `4K3/8/8/8/8/5k2/8/1R6 w` is converted at
  every budget.
- Also:
  - the fit is untouched: `--verify` passes on validation, and A2's
    realized loss reproduces 0.09612177;
  - `test_endgames` 193/193, `test_search` 792/792, `test_eval` 134/134;
  - `bench 13` on the repaired head is 4,392,040.
- Calibration: both predictions hit; the falsifier did not trigger.
- Disposition: COMPLETE. The repair qualifies for the bake.
