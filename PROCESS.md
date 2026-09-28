# Basilisk recurring procedures

The recurring procedures: how a leaf is researched, registered, implemented,
reviewed, gated and closed, and how the build, fit, tune and gate instruments
are run. `AGENTS.md` holds the rules that stop wrong results, `PLAN.md` the
roadmap and `DESIGN.md` the engine invariants. This file also owns the
independence boundary with the donor engines.

## Research packet and implementation handoff

Use a packet under `analysis/` when the decision would make PLAN unwieldy;
trivial work stays in PLAN. The packet format is defined in
[analysis/README.md](analysis/README.md). A packet is a live decision record,
not a second roadmap; PLAN owns its state and links it.

The handoff fixes the following. If any central field is still a design
choice, the leaf remains `RESEARCH`.

- the goal and exact intended semantics;
- why local evidence says it should work here;
- the producer → stored state → consumer map where relevant;
- files and subsystems, interactions and invariants;
- instrumentation, deterministic tests and cheap qualification;
- the maintainer-owned expensive gate and the acceptance/rejection rule;
- explicit non-goals and adjacent mechanisms not to change.

During research, inspect PLAN, EXPERIMENTS, the retry map, linked analyses and
the relevant source. Prefer the cheapest test that separates causal
explanations. For important interactions use a bounded baseline/A/B/A+B
screen when it can distinguish independence from masking; do not require it
for every small change. Freeze predictions in EXPERIMENTS before exposure. A
later explanation is calibration, never proof that the outcome was predicted.

**Returns and amendments.** When implementation finds a false premise, the
implementer appends to the packet a *Return N* section: the premise, the
evidence against it (counters, traces, failing tests), what was built and
kept, and the options the implementer can see. The researcher answers with an
*Amendment N* section that picks an option, re-registers any prediction the
change affects (explicitly, as a clerical or substantive amendment, before any
game), and updates the handoff. The leaf's PLAN text records the return and
its resolution in one line each. Implementation never resolves a return by
itself.

**Implementation record.** When a cluster is implemented, the packet gains an
*Implementation record*: commits per ticket, the fingerprints of both arms,
the tests added, every deviation from the handoff with its reason, and what
the reviewer must check. The reviewer's acceptance is appended below it.

## Experiment registration

Register an experiment as one entry in the `EXPERIMENTS.md` section that owns
it, before any games. The template is EXPERIMENTS §11. When the registration
is longer than an entry, write it in an `analysis/` packet with the template's
fields and cite the packet; append the result and calibration there without
rewriting the prediction.

## Step lifecycle

Before selecting a leaf, review GUIDE's checkpoint and holds and PLAN's
register. Select the earliest unblocked leaf; keep skipped work visible and
return when its unblock condition holds. After one leaf, record its result
and status, commit the verified work, report the next executable leaf and
the relevant holds, then stop.

Analysis-only leaves deliver findings, an interaction and cost map, derived
numbered implementation leaves, or a justified no-change. They do not
implement speculative improvements during the audit. Before any subsystem
optimization, inventory producer → stored state → consumer → invalidation,
undo and reset, profile realistic search, and classify each finding. A
faster operation can still build a worse tree.

For every behavioural cluster, **gate the fitted dependency-complete cluster,
not each feature and not the whole phase at once.** Internal sub-steps may be
too sparse or coupled to win before their consumers and weights move together;
postponing all games to the end destroys attribution and lets losing
structures hide.

1. **Audit** — name the problem, its C++ owner, all interacting consumers and
   the local diagnostic population. Update PLAN first if the evidence
   contradicts the planned order.
2. **Register** — an EXPERIMENTS entry with hypothesis, baseline revision,
   candidate scope, expected direction, gate, cap and stop rule, before
   games. Bounds default to `[0,3]` nElo; size the cap from the drift model at
   the expected value.
3. **Implement** — the smallest dependency-complete cluster, behind its
   umbrella switch (below).
4. **Prove correctness** — release and ASan/UBSan CTest on both arms, the
   invariant tests, `bench 13` exact on the off arm at every commit, and
   ThreadSanitizer when threading or shared state is touched.
5. **Review** — a separate session accepts it against the handoff.
6. **Explain** — the rule-8 screens at fixed depth and nodes against the
   oracle: nodes, qnodes, move source, cutoff index, TT use, reductions and
   re-searches, pruning, extensions, aspiration. Counters explain a candidate
   and cannot accept it. A true correctness canary may veto; disagreement
   between aggregate depth and tactical counts is otherwise inconclusive until
   the per-position and equal-cost results explain it.
7. **Paired run** — the unfitted 2,000-game paired run, which governs.
8. **Fit** — categorical semantics held fixed, the curvature sweep, then SPSA
   in blocks over the live surface; theta is the last completed block's
   rounded centres, never a selected checkpoint.
9. **Gate** — prepare and verify the fitted candidate, the revision-matched
   baseline, clean final-PGO builds, manifests and the registered command. The
   maintainer starts it. Do not change the candidate, bounds, cap, book or
   adjudication after observing games.
10. **Close** — accept and flip the default only on a passing result.
    Otherwise revert the behaviour, keep the evidence row and restore the prior
    fingerprint. Ablate a surprising integrated result before crediting a
    subcomponent.
11. **Advance** — the next item starts only after this one is accepted,
    rejected, explicitly closed, or visibly held.

A separable categorical alternative may have a preliminary paired run, which
never replaces the fitted integrated cluster's SPRT. The programme checkpoints
(B.9, C.11, E.1) own the combined confirmation runs; none may rescue an
earlier losing cluster. Two failed coherent clusters in one programme trigger
a return to evidence (PLAN rule 6), not silent closure.

## Cluster delivery shape

Taken from Rarog's B.2 and B.3, which took it from Manta's 6.5.10.

- **One umbrella switch per cluster.** A CMake `option()` defaulting OFF
  compiles the cluster in. The off arm reproduces the accepted `bench 13`
  fingerprint exactly at every commit, so the baseline is always the same
  tree as the candidate minus the cluster. The switch flips to ON in one
  commit after the gate; the old path stays compilable until the programme's
  cleanup leaf deletes it.
- **Tickets.** The cluster lands in ordered tickets, one mechanism each, every
  one keeping the off arm exact. Ticket 0 installs the decision trace (a
  diag-only printout, bounded to plies one and two under `searchmoves`, of
  every prune, reduction and extension with its inputs). The trace finds
  defects that counters cannot see.
- **Categorical switches.** A handoff choice that the research could not
  settle becomes a tune-build UCI option with values `0..1`, its default equal
  to the handoff's choice. It is settled by a 2,000-game paired run on one
  tune build (no build offset between the arms), adopted at ≥ +5 Elo,
  rejected at ≤ −5, and left at default between. It is never an SPSA
  coordinate.
- **Canaries are anchored to the reference.** A tactical canary must be
  solved at the depth classical Stockfish solves it, not at whatever depth
  the current build manages. A changed canary is recorded with its cause and
  never re-blessed.
- **Implementer and reviewer are separate roles.** The reviewer's acceptance
  is recorded before diagnostics start.

## The independence boundary

Donors (maintainer decision 2026-09-28; PLAN rule 1 points here):
- **Modern Stockfish** (pinned by B.0; default official master `0a215d6c`)
  for search, TT, histories, move ordering, time management, threads and the
  later NNUE runtime.
- **Classical Stockfish `9587eeeb`** for the evaluation families, as the
  deficit oracle (`oracle/hybrid`), and as a seed column for eval-coupled
  margins.

Rarog and Manta are worked examples of the same method; Reckless is Rarog's
donor. None of them donates to Basilisk, and their verdicts are priors, never
acceptance. Rarog is read only through the pinned snapshot in
`docs/reference/rarog/`, whose README says how to resolve a reference and how
a newer Rarog finding enters: a new dated snapshot plus ledger import rows. No
Basilisk leaf may depend on the live Rarog repository.

- **May cross:** architecture, mechanisms, population choices, contracts,
  failure modes and constants. A constant is a seed on the donor's scale: it
  is converted through the measured scale ratio, fitted locally and gated
  (PLAN rule 2).
- **Written by us:** code in Basilisk's own structure. Line-for-line
  transcription only where an algorithm has one natural form, or a different
  form provably loses throughput. Stockfish and Basilisk are both GPL-3, so
  the licence permits more than this boundary does. The boundary is about
  understanding, not law.
- **How:** read the donor, close the file, then design from Basilisk's own
  code and its measured evidence. If a change cannot be justified without
  pointing at the donor, it is not understood well enough to ship.
- `README.md`'s posture stays accurate: an independent engine, with thanks
  for the inspiration.
- Do not merge the `oracle/hybrid` package into Basilisk or read the oracle as
  permission for an unmeasured rewrite.
- Similarity is never a reason to accept anything, and a counter that diverges
  from the oracle is a question, not a defect. Games decide.
- Deciding that a donor mechanism does not apply here is a first-class result;
  record it with its reason.

## Adjudication

Every instrument plays games out: `sprt.ps1`, `gauntlet.ps1`, SPSA on either
path, the Colosseum run files after A.3, and `datagen.ps1` (whose default is
`-Adjudication none`). Adjudication destroys endgames before they are reached
and is an evaluator-dependent confounder across engines: it moved one
cross-engine estimate by 74 Elo (BAS-X10). `-Adjudicate` opts back in only
with a registered reason, and its results are not comparable with
natural-termination ones. Syzygy truth may label training positions (C.2.4);
it never adjudicates a strength game. Use fixed movetime or nodes only for
deterministic diagnostics.

## Harness

**Until A.3 closes, `tools/sprt.ps1` (fastchess) and `tools/spsa.ps1`
(weather-factory) are the gate and tune path.** After A.3, Colosseum CLI is
the main path for gates, fixed matches, tunes, null pairs and gauntlets. It is
driven by `tools/colosseum.ps1` from committed run files, with the runner
pinned by revision and SHA-256. It is qualified in its own repository, with
Rarog as the validation engine, and Basilisk repeats none of that
qualification. fastchess and weather-factory stay installed, working and
documented as the backup and the second opinion until at least the classical
release. Every guard has one implementation in `harness_common.ps1`, shared
by both paths.

Run the backup path as a cross-check when:
- the runner, the scheduler or the CPU topology on this host changes (the
  same trigger that owes a null pair, BAS-M02);
- a result is surprising: a sign nobody predicted, a magnitude well outside
  the registered band, or a gate resolving far faster or slower than the
  drift model predicts;
- the Colosseum pin changes.

A cross-check is a fixed match or a replayed gate on the same arms, read as
"do the two instruments agree inside their intervals", never as a second
chance at acceptance. A registered experiment names its runner and never
changes it mid-way.

Shared conditions: `3+0.03`, Hash 64, one thread, the UHO book in random order
and paired, no adjudication, a 20 ms time margin, and fourteen concurrent games
on pinned physical cores (Windows services most interrupts on CPU 0; the
harness leaves two physical cores free). A tune runs fifteen slots, because
both perturbation arms share a slot. fastchess pins one core per game and
starves `Threads > 1`, so multi-thread runs on that path drop affinity.
fastchess ≥ 1.7 pins through Windows CPU sets, which are invisible to process
affinity masks: verify pinning by per-CPU load, not by querying masks
(BAS-M01). **Never run two pinned harnesses at once, from Basilisk and Rarog
together included.** A null pair (the same executable on both arms, judged on
the full 95% nElo interval inside ±5, never on LLR) is owed only after a
runner, scheduler or topology change.

`sprt.ps1` defaults: `-Mode gainer` is `[0, Elo1]` with **Elo1 = 5 by default,
so a registered `[0,3]` gate passes `-Elo1 3`**. `-Mode simplify` is `[-5,0]`.
`-Mode fixed -Games N` is a measurement that decides nothing. `-OptionsA` and
`-OptionsB` test a UCI option on one binary without a rebuild.

## Build, bench and NPS

- Presets: `release`, `release-pext`, `release-avx2`, `debug`,
  `relwithdebinfo`, `msvc-release-pext`. PGO: `cmake --build --preset
  release-pext --target pgo`, which trains on the bench suite and writes to
  `build/dist/`. `TUNE=ON` exposes the search constants as UCI options;
  `TEXEL=ON` builds the Texel target and is never measured for strength.
- `bench 13` is the fingerprint: 40 positions, single-threaded, identical on
  every platform. `bench 13 1 N` is for multi-thread speed only.
- `tools/build_test.ps1 -Suffix <s>` builds a PEXT+PGO tune binary into
  `tools/test_engines/`; A.4.2 extends it to arm flavors with manifests.
- **NPS:** `tools/nps_ab.ps1` validates on a self pair first (it must read
  about 0.00%), pools at least two PGO builds per arm (two PGO builds of
  identical source differ by about a third of a percent), interleaves arms,
  and runs on an idle host.
- **Converting NPS to Elo:** about 2 Elo per 1% NPS at `3+0.03` holds for
  small deltas only (BAS-P01). Above a few percent convert through doublings,
  and say which conversion was used.
- **Converting nodes to depth:** a node-count change is converted to plies
  (divide by log EBF) before it is called expensive. +19% bench nodes was
  0.165 ply, and strength-neutral.

## Matched ablation (deficit decomposition)

One shared bit mask on Basilisk and on the oracle build, so the same number
ablates the same mechanism on each side (A.5.4 fixes the order), compiled
away in a shipped build.

1. The harness refuses an option an engine does not expose. A run that sets a
   missing option plays the whole match at the default and measures nothing.
2. Prove every bit live before trusting any number from it: nodes to a fixed
   depth must move for each bit. A guard that reads ×1.00 is dead.
3. Prefer matched cross-engine runs to self-play deltas: Basilisk against the
   oracle at the same mask, reading G(0) − G(mask) as the Elo that mechanism
   explains.
4. Keep the ablated arm inside roughly 20–80% score; outside it the Elo curve
   saturates and small score differences read as large Elo.
5. About 2,000 games is enough for these 40–250 Elo effects; mechanisms under
   about 10 Elo are not measurable this way at `3+0.03`.
6. Net out NPS before comparing engines; it cancels in G(0) − G(mask) but not
   in any absolute statement.

## Texel procedure

C.2.1 writes the Basilisk Texel handbook; until then the tools are
`basilisk-texel` (`TEXEL=ON`), `tools/texel/extract_parallel.py` and
`tools/texel/bake.py`. The policy:

1. Mechanically verify the label domain and source before fitting:
   self-play WDL means exactly `0`, `0.5`, `1` from White's perspective; a
   filename or a prose summary is not proof.
2. Keep stable by-game train/validation/test splits; claim the test split once
   after model selection.
3. Enumerate every evaluation coordinate with its status — free, fixed or
   excluded (nonlinear, capped or bucket-selecting terms a linear model would
   misprice) — and its reason. The inventory sums exactly to the parameter
   registry.
4. Trace every linear term exactly and verify full-evaluation reconstruction.
5. Smoke the complete vector → bake → source → rebuild chain with absurd
   values in every instrument class; unchanged behaviour after a broad fit is
   a failed wire.
6. **Sequential joint bake.** `--tune all` and `--tune-kingsafety` each write
   a full parameter dump with the other group at baseline, so baking one after
   the other reverts the first. The order is: bake the linear fit, rebuild
   `basilisk-texel` against it, re-fit king safety on the baked evaluation,
   then bake king safety.
7. Fit loss is a screen and a falsifier, never acceptance. Bake the fit into
   clean PGO and gate it.
8. Keep search parameters fixed during an evaluation fit; re-measure their
   populations at C.10 rather than co-tuning.

## SPSA go/no-go procedure

SPSA is owed where a cluster's registration names its live coordinates (PLAN
B.2.3, B.6, C.9, C.10, F.6). An undirected broad tune is forbidden, and
evaluation and search coordinates never share a run.

1. Name the strength-bearing mechanism and show local evidence that its
   consumers are misfit. Estimate the plausible Elo and cancel if it is inside
   the gate's dead zone.
2. Gate categorical switches separately and freeze the winner in both arms;
   never pin a binary knob as an SPSA constant.
3. Run the curvature sweep (PLAN rule 8); flat or monotone on every registered
   coordinate skips the tune.
4. Select continuous coordinates from activation and interaction evidence,
   never from a historical coordinate count. Generate the surface from the
   X-macro table in `src/search_params.h` so defaults and ranges cannot drift.
5. Register the tune in blocks (PLAN rule 7c): 2,000 iterations × 30 games,
   later blocks seeded from the previous block's rounded centres with a fresh
   schedule and the same steps, the movement stop rule, a ceiling of three
   blocks; block size, rule and ceiling never change after the first game.
   Register the surface, fixed values, binary hash and fingerprint, slots,
   the budget in games, gain (`r_end` 0.0031) and estimator before launch.
6. Theta is the last completed block's rounded centres, without checkpoint
   selection; bake it into a fresh clean PGO binary and run a paired SPRT.
7. Verify at bake time that every correctness floor still passes under
   generous limits, not only the fixed-depth test: a tuned optimum once broke
   the KBNK conversion core that the fast-TC games never reached, and was
   excluded on that evidence.

## Opening books

SPRT and SPSA both use `tools/books/UHO_Lichess_4852_v1.epd`, paired and
reversed, at `3+0.03`, so the optimizer and the gate see the same openings and
clock. Datagen uses a diverse start set (`tools/texel/data/beast_seed.epd`,
`-BookFormat epd`): identical engines at fixed nodes are deterministic, and a
small book collapsed 200,000 games into 31,880 unique positions.

## Decision rules

- One item open at a time; each candidate gates against the current accepted
  head, never against a stale baseline or another unresolved candidate.
- Categorical architecture is gated before its constants are fitted.
- A touched dormant switch is removed, kept inert with a named owner, or
  separately gated; it is never activated opportunistically.
- Borderline results are not accumulated as hidden debt. Accept or revert.
- One gate, one read. Only when an SPRT stops under 2,000 games may a longer
  match be discussed, and it is registered then, not in advance.
- Tune and non-PGO results are diagnostics; final-PGO games decide promotion.
- A correctness exception names the invariant, the tests and the incomplete
  strength evidence.

## Common commands

```powershell
cmake --preset release-pext -DCOMP=clang
cmake --build --preset release-pext
ctest --test-dir build/release-pext --output-on-failure
cmake --build --preset release-pext --target pgo
python tools/diag/check_roadmap.py
python tools/diag/check_roadmap.py --self-test
```

```powershell
# Gate on the backup path until A.3 closes. Pass the registered bounds: the
# gainer default upper bound is 5, not the registered 3.
./tools/sprt.ps1 -EngineA <candidate.exe> -EngineB <baseline.exe> `
  -NameA candidate -NameB baseline -Mode gainer -Elo0 0 -Elo1 3

# A measurement with an interval, which decides nothing
./tools/sprt.ps1 -EngineA <a.exe> -EngineB <b.exe> -Mode fixed -Games 2000

# Non-regression / simplification, [-5,0]
./tools/sprt.ps1 -EngineA <candidate.exe> -EngineB <baseline.exe> -Mode simplify

# Pooled NPS, validated on a self pair first
./tools/nps_ab.ps1 -EnginesA <a1.exe>,<a2.exe> -EnginesB <b1.exe>,<b2.exe>
./tools/nps_ab.ps1 -EnginesA <a1.exe>,<a2.exe> -SelfPair

# Tune binary for SPSA and categorical runs
./tools/build_test.ps1 -Suffix <s>
```
