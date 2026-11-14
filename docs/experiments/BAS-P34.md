# BAS-P34

**BAS-P34 — B.8.2's deletions and idioms in the whole engine (B.8.2)**

- **Registration.** 2026-11-15, B.8.2, before any read. Agent-built pools; the read
  runs on the idle host the maintainer gives. Behaviour-neutral: `bench 13`
  4,247,965 on both arms; the head's `bench 13` board trace is byte-identical to the
  base's (SHA-256 4C6643CD9CB43460…), its decision trace is byte-identical over 8,273
  lines, and a Tune build advertises every option exactly as the base does.
- **Question.** Do B.8.2's deletions and idioms leave the engine at least as fast?
- **Arms.** Four-build final-PGO pools, clang 23.1.5, PEXT (`tools/nps_pool.ps1`):
  - base `b76-head`, built at `0c978c2`, reused: its engine sources and build
    files are identical to `55e3010` on `master` (`git diff 0c978c2 55e3010 --
    src CMakeLists.txt CMakePresets.json build_test.ps1` is empty), so it is
    master's engine, and BAS-P30 read it;
  - candidate `b82-head`, built at `01aa08e`, B.8.2's head on `b8-cleanup-speed`.
- **Changes** (`analysis/b82_switch_inventory_2026-11-14.md` §2–§4, §7): about forty
  runtime switches and their other arms; `Board::see`, the cross-check build and the
  board's dead paths; the counters, trace events and price rules nothing writes;
  release telemetry counted only in Tune and Diag; `allow_null`, `moved_piece` and
  the shallower re-search (B-63); the override tables folded into their lines; B.6.4's
  behaviour-neutral idioms (one unit conversion, a scoped `TTFlag`, `DIAG_COUNT`
  under `if constexpr`, a designated-initialiser trace record, a `SearchedMoves`
  list).
- **Prediction.** The inventory's §6, frozen before the deletions: a gain of a few
  tenths of a percent, not Rarog's +1–6% (D-18), since the deleted branches are
  well predicted; the release telemetry's increments are the likeliest source of
  any measurable gain.
- **Method.** `tools/nps_read.ps1 -Base b76-head -Cand b82-head -NoRegression`, the
  no-regression form (PROCESS *NPS*): step 1, two cycles, passes at −0.1% or above and
  fails at −0.9% or below; otherwise step 2, six cycles in all, fails at −0.5% or
  below with the upper bound under 0. The estimate and its interval are recorded
  either way.
- **Verdict rule** (PLAN B.8.2, frozen before the read):
  - A pass keeps everything and closes B.8.2; the estimate is the record of the
    cleanup's speed.
  - A fail is a red flag (an item that costs NPS is dropped, not traded): the
    idioms and the telemetry guard are read alone against the deletions without
    them, and an item that costs speed returns to its earlier form before B.8.2
    closes.

#### RESULT

Idle host, 2026-11-15. Pooled read (`tools/nps_read.ps1 -NoRegression`,
`tools/results/b82-nps-20261115`), two cycles, host at 1.1% or less, no cycle
disturbed; per-build medians `b76-head` 2.503M, 2.487M, 2.497M, 2.491M and
`b82-head` 2.543M, 2.542M, 2.538M, 2.545M.

| Contrast | Delta | 95% interval | Verdict |
|---|---:|---|---|
| `b82-head` over `b76-head` | +1.89% | [+1.47%, +2.31%] | PASS at step 1 |

- **Disposition.** PASSED: every deletion and idiom stays, and B.8.2 closes. The read
  is a gain as well as a pass: its lower bound clears the gain form's +0.5% step-2
  bar, though only the no-regression form was registered.
- **Calibration.** The prediction (a few tenths of a percent) was low by a factor of
  about five; the result sits inside Rarog's +1–6% (D-18). The deleted switches were
  branches the release build took at every node they guarded, and the release
  telemetry's increments ran on every evaluation, SEE and check test; the read does
  not apportion the gain between them.
- **Note on the candidate.** The three commits after `01aa08e` (the profile-region map
  and the records) change no engine source, so `b82-head` is the head's engine.
- **Artifacts.** `tools/results/b82-nps-20261115` (`read.json`, `runs.jsonl`,
  `verdict-2.json`); pools `tools/test_engines/b76-head.pool.json` and
  `b82-head.pool.json`.
