# BAS-S64

**BAS-S64 — B.8.7's repair gate: nodes counted as Stockfish counts them, `[-5,5]`**

- **Registration.** Created 2026-11-15, before any game; maintainer-run games;
  a repair of a departure from the donor, the maintainer's decision to make it
  now (out of turn, before B.8.4).
- **Departure.** Basilisk counted a node at every `negamax` entry and at every
  quiescence entry that negamax had not handed over
  (`src/search_kernel_core.cpp:601` and `:176` at `95997a3`). So the null child,
  LMR and PVS re-searches, the singular exclusion search, the null verification,
  ProbCut's quiescence-then-search pair and each root re-entry counted again.
  Stockfish 19 counts once per move made, in `Search::Worker::do_move`
  (`search.cpp:658` at `sf_19`): not the null move, a re-search, the exclusion
  search or the graph-history check's bare `pos.do_move`. Measured with a
  temporary counter of moves made beside the old count (removed before the
  commit): 8.5% above the donor's unit on `suite_v2` at depth 12 (5,780,881
  against 5,326,230 moves made; 4.4% above moves plus null moves, Rarog's
  unit); 10.8%, 10.2% and 13.1% at 1, 4 and 8 threads (`go movetime 1500`, six
  positions, both units counted in the same searches).
- **Why it can matter.** The donor's formulas read the count where the donor
  reads its own: the equal-score tie rule's `nodes & 14`, the root averages'
  weights (`b5::ema_weights` over each root move's effort), the time manager's
  effort share of the best move, and `go nodes`, the reported NPS and the clock
  check's cadence. The old count is not a uniform rescale: a small subtree loses
  relatively more counts than a large one.
- **Candidate.** `838a3c6`: `Searcher::do_move` counts after the move (the
  graph-history check's move passes `counted = false`), then looks for a stop;
  each node reads `stopped` on entry. `test_search` pins the contract: a
  depth-1 search of one move counts 1, one from the start position its 20 root
  moves. `tools/test_engines/basilisk-b87-count-pext-pgo.exe`, SHA-256
  `A3A9EEB393F07173943074C4A64C2347D84E2DD711DEA5B48BE7950C085756EA`, `bench 13`
  **4,688,537**.
- **Baseline.** `01aa08e`, B.8.2's head, the engine before the change (the
  commits between change only tests, tools and records):
  `tools/test_engines/basilisk-b82-head-pgo1-pext-pgo.exe` (a BAS-P34 pool
  build), SHA-256
  `C9FD373C00F52FF47726DF066212D29A688BE4E2FAEF1094174859C43E99CC40`, `bench 13`
  **4,247,965**, the accepted fingerprint.
  Both final-PGO PEXT release builds by `tools/build_test.ps1 -Flavor Release`,
  Clang 23.1.5, manifests clean.
- **Research question.** Does the donor's count cost Elo? A repair kept unless it
  harms.
- **Mechanism.** The tie rule, the root averages and the effort share were
  ported with the donor's count and fitted (B.5, B.6.7) on Basilisk's; the
  donor's count restores the inputs they were designed on. On `suite_v2` at
  depth 12 the tree is 5.9% smaller in interior nodes (the root averages'
  aspiration researches +2.9%); on `bench 13` positions move both ways (×0.45
  to ×35.7 in the new unit; three positions carry the total's rise), as any
  change to the root averages moves them.
- **Competing explanations.** The literals fitted on the old count may be tuned
  to it, so the donor's count could cost Elo until a refit; the change may be
  tree-shape noise.

#### EXPOSURE

Every search: the count feeds the root averages and the effort share at every
iteration, and all 40 `bench 13` positions change their node count. The cap is
the full one, as BAS-S63's.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 87010. Arm A the candidate, arm B the baseline.
- **Cap: 4,000 pairs.**
- Launcher `python tools/run_b87_gate.py`; results in `tools/results/b87-gate/`.
- **Dry run accepted 2026-11-15**
  (`tools/results/colosseum_sprt_b87-gate_20261115_094121.manifest.txt`).

#### QUALIFICATION (before the gate)

- On `838a3c6`, CTest 33/33 on Release, Diag and Tune.
- Two test preconditions moved in their own commits before the change, each
  read on both trees: `8f77345`, `test_b5_root`'s proven mate pins the property
  (from the first exact mate line every later one reads a mate no longer)
  instead of mate in 9 at depths 19 and 20 (the head proves mate in 9 from
  depth 19; the candidate in 9 at depth 16 and in 8 from 17); `e894552`, the
  allocation guard's shallow search at depth 4, since at Threads 4 the deep
  search grew as little as 7.6 times from depth 5 (0 failures in 12 runs on
  each tree after it).
- Threads: the pool's total is each thread's own count summed at the join
  (`SearchThreadPool::merge_results`), Stockfish's `nodes_searched()`; release
  `bench 13` NPS scales 1.91, 4.06 and 8.18 times at 2, 4 and 8 threads.

#### VERDICT RULE (frozen)

- H1 or the cap with LLR above the lower bound: the count stays; the
  fingerprint record moves to 4,688,537 (GUIDE, DESIGN, AGENTS).
- H0: the old count returns, and B.8.7 goes back to research with the
  refit question (are the root averages' and effort's literals fitted to the
  old count?).

#### RESULT

Pending: the games are the maintainer's.
