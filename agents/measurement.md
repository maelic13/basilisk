# Measurement, gates and long runs

Read this for `V` leaves and whenever a task builds a binary to measure,
reads bench, NPS or `Diag` counters, plays games, runs or prepares an SPRT,
SPSA, gauntlet, datagen or fit, or reads returned artifacts. Register an
experiment by `agents/research.md`, *Experiment design*, before it runs.

## Measurement

- After PLAN A.3, Colosseum CLI is the main harness for gates, fixed matches,
  tunes, null pairs and gauntlets, driven by `tools/colosseum.ps1` from the
  committed run files; fastchess and weather-factory (`tools/sprt.ps1`,
  `tools/spsa.ps1`) stay installed and working as the backup and the second
  opinion. PROCESS's *Harness* section names the cross-check triggers. A
  registered experiment names its runner and never changes it mid-way.
- Never measure strength or NPS on a `TEXEL` or diagnostic build. If a number
  you are not changing changes, check the binary.
- Measure only on an idle host: check CPU use and running engine, harness or
  build processes first, and if the machine is busy stop and ask rather than
  measure. Keep builds and profiling off a match host while it plays.
- Rebuild before measuring, with the exact preset and options. For a
  multi-run study, build once, verify the fingerprint, hash and archive that
  executable, and measure the copy; rebuild only when source, options,
  toolchain or build settings change.
- `bench` and `Diag` counters are per position; sum them with the committed
  tools, never a hand-rolled parser. Before differencing two counters, confirm
  they are in the same unit; a passing invariant does not prove comparability.
  Counter ratios are valid only at sampling stride 1.
- A binary entered in a rated pool is a tagged release or carries its bench
  fingerprint or build flag in its version string, and its ledger row names
  the fingerprint. The harness runner is pinned the same way.
- The current fingerprint is declared once, in GUIDE's *Now* table;
  `check_roadmap.py` fails when the restatement in `AGENTS.md` or DESIGN
  disagrees.

## Acceptance

- A playing change needs deterministic regression evidence, the relevant
  tactical/endgame tests, bench accounting and an appropriately registered
  strength gate. Reasoning, node counts and static fit loss do not prove Elo.
- Build the actual candidate configuration that will be tested. Keep compiler,
  PGO, binary, book, seed, time control, hash, threads, affinity, adjudication
  and data provenance comparable and recorded.
- Score-based game adjudication is off by default. Use it only for an explicitly
  registered compatibility experiment.
- Never accept a candidate that fails a hard correctness, mate, rule-50,
  tablebase or time-forfeit gate even if its strength estimate is positive.
- Prove a guard or harness wire live before trusting a null from it: a known-bad
  input must make it fail, an absurd option value must move the numbers.
  Proving the engine responds is not proving the instrument reports it.

## Gating

- The strength unit is one dependency-complete, locally fitted cluster;
  internal sub-steps get no gates of their own. Register it in
  `docs/EXPERIMENTS.md` before any games, and never change bounds, cap, book or
  adjudication after seeing games.
- `[0,3]` nElo is the default bracket; `sprt.ps1`'s own default upper bound is
  5, so pass the registered bounds explicitly. Widen only for a genuinely large
  prior and say why. A removal or simplification uses `[-5,0]`; a repair of
  unknown sign uses a symmetric bracket. Size the cap from the drift model at
  the expected value first.
- High bounds reject small gains. That is overnight compute: budget the games,
  do not widen the bounds or poll.
- Bench and counter screens choose candidates and never accept strength. Do
  not invent an acceptance rule after seeing a result.
- An unresolved stop is not "probably fine": a high LOS on a point estimate is
  not evidence the mechanism works.
- One gate, one read: an SPRT's verdict and its estimate at the stop are the
  record; no fixed match of the same pair runs beside it.
- SPSA is conditional: first show activation, interaction and curvature with a
  zero-game sweep; a flat or monotone surface is evidence against the tune. A
  tune runs in registered blocks with a movement stop rule (PLAN rule 7c),
  never on a horizon chosen to fit the answer.

## Long-running work

- Agents may run short builds, benches, focused tests and targeted diagnostics
  when they are the appropriate verification for the current leaf.
- Do not start long SPRTs, SPSAs, tournaments, large datagen jobs, long fits or
  comparable machine-saturating work unless the maintainer explicitly asks the
  agent to run it.
- For a required long run, prepare and validate the runnable state, commit that
  state with a clear Prepare <step> subject, provide exactly one copy-pasteable
  single-line command, and stop. Keep the checklist item open.
- After the maintainer returns the artifacts, analyze them, apply the
  pre-registered verdict, finish the checklist/docs, commit with a clear
  Complete <step> subject, report the outcome and next leaf, then stop.
- Respect temporary resource reservations stated in the conversation. Do not
  compete with an active engine job merely because a command is normally short.
- The host is shared with Rarog. Never start a pinned harness while another
  pinned harness runs, from either repository: both compute the same core
  list, and two at once oversubscribe every core and forfeit games.
