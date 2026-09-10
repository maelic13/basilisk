# Basilisk development plan

Basilisk is being **frozen** after one more release. The maintainer's
development effort moves to Rarog (`D:/code/rarog`), which shares Basilisk's
evaluation lineage and now carries the joint battle plan. Basilisk stays in
the maintainer's tournament pool as a reference engine and as the C++ donor
for Rarog's classical work.

This roadmap therefore has one phase: fix the board correctness defects that
a peer audit demonstrated, release **1.9.4** from the current `dev` line, and
freeze. The previous roadmap (Phases 5–14, endgame maturity through NNUE) is
archived verbatim at
[docs/archive/PLAN-2026-09-09.md](docs/archive/PLAN-2026-09-09.md) with its
GUIDE at [docs/archive/GUIDE-2026-09-09.md](docs/archive/GUIDE-2026-09-09.md);
[HISTORY.md](HISTORY.md) records what that roadmap established and the state
it froze in. `EXPERIMENTS.md`, `DESIGN.md` and `AGENTS.md` are unchanged.

## 1. Current checkpoint

| Item | State |
|---|---|
| Branch | `dev` |
| Released baseline | Basilisk 1.9.3 at `d737123`; bench-13 fingerprint 11,941,440 |
| Accepted engine head | `dev` at `d0f2627`: Group A endgames plus 6.5.a rook scaling; bench 12,568,898; CTest 12/12; 170 commits since v1.9.3 |
| Strength baseline | 6.5.a accepted at +3.29 ± 4.61 Elo over frozen Group A; the accepted HCE line previously about +12 Elo over 1.9.3 |
| Pool position, `3+0.03` 1T (2026-09-04) | Houdini 1.5a −197, Critter 1.6a −187, Fritz 16 −178, Rybka 4 −84; Rarog 2.4.0-dev +26 |
| Current phase | Phase 15, step 15.0.a |
| Long job | None |
| Release target | **1.9.4**, then freeze |

## 2. Operating contract

`DESIGN.md` holds the engine invariants and the four questions every mechanism
must answer before it is implemented. `AGENTS.md` makes reasoned refusal and
refutation deliverables of equal standing to a diff.

- Work strictly in numbered order. A later step may be prepared, but may not
  change engine policy or consume experimental budget before its dependencies
  close.
- Commit each completed step with PLAN and GUIDE synchronized, and run
  `python tools/diag/check_roadmap.py` before committing either roadmap file.
- Consult and update `EXPERIMENTS.md` before retrying a mechanism; freeze the
  prediction before exposure and append the calibration after.
- Preserve source, compiler, binary, book, corpus, split, seed, tablebase and
  command provenance. Hash immutable inputs and outputs.
- A behavior-neutral change needs the relevant static checks, CTest and exact
  bench. A playing change additionally needs its registered game gate. A
  correctness repair needs an independent invariant that fails on the old
  behaviour, and a strength gate when deployed play changes materially.
- Strength tests use paired UHO openings (`tools/books/UHO_Lichess_4852_v1.epd`),
  normalized Elo, natural termination (score-based adjudication off), and
  record the exact bracket used. `[0,3]` nElo is the default; an unknown-sign
  repair uses a symmetric bracket; non-regression uses `[-5,0]` as
  `tools/sprt.ps1 -Mode simplify` runs it. Accepting H1 is a decision, not an
  effect size: report the point estimate and interval beside the verdict.
- Long jobs are run by the maintainer after the agent prepares and verifies
  the instrument and hands over one command; the agent analyzes the returned
  artifacts and applies the registered verdict.
- Reference engines teach mechanisms and experimental design. Reimplement in
  Basilisk's idiom; never copy constants as acceptance evidence.
- Do not run competing CPU-heavy work while a long tournament, tune or fit
  occupies the machine.

### Development states and capability classes

`RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPLEMENTED -> LOCAL_QUALIFIED -> GAME_GATE -> CLOSED`

`READY_FOR_IMPLEMENTATION` is the boundary: measured defect, mechanism and
semantics, interactions, invariants, cheapest falsifier, qualification and
deciding gate are concrete. A false premise returns the leaf to `RESEARCH`.

| Class | Use |
|---|---|
| `R3` | frontier research; unresolved causal or architecture work |
| `R2` | bounded but correctness-sensitive architecture/reasoning |
| `I2` | difficult implementation requiring strong reasoning |
| `I1` | well-specified implementation |
| `M` | mechanical documentation, manifests or provenance |
| `V` | verification or measurement work |

GUIDE owns the editable mapping from classes to current models.

## 3. Required evidence

| Change | Minimum gate |
|---|---|
| Tool/docs/refactor | Syntax/static checks; focused tests; full tests/bench when execution semantics can change |
| Correctness repair | Independent invariant that fails on the old behaviour; strength gate when deployed play changes materially |
| Search or SEE change | Deterministic regression/telemetry, 1T STC SPRT, relevant LTC/4T confirmation |
| Release | Reproducible PGO assets, correctness matrix, prior-release games at STC and a 4T direction check |
| Behavior-neutral hot path | Exact immediate fingerprint, targeted parity, pooled/interleaved NPS on an idle-enough host |

## Phase 15 — Board correctness, release 1.9.4, freeze

**Why this phase and only this phase.** The peer audit of the shared board
lineage found three real SEE defects in Rarog (BAS-X22; Rarog RAR-M27/M28):
selected-king legality, pins created during the exchange, and recapture
promotions. Basilisk's `see_ge` (`src/board.cpp`, the loop after
`see_pins`) has **no king rule at all**: when the king is the least valuable
remaining attacker it recaptures regardless of whether the target square is
still attacked, so an exchange that ends with an illegal king capture is
scored as if the capture stood. Basilisk's pins come from `see_pins` on the
exchange's starting occupancy, so a pin created after a piece leaves its
square is invisible, and a pawn recapture on the last rank is scored as a
pawn. All three are correctness defects that ordinary tests and NPS cannot
see; Rarog's repaired cluster measured +12.12 ± 10.17 Elo and changed its bench
tree by 10%. Fixing them is cheap, and a frozen engine should not carry a known
defect into its last release.

**What this phase does not do.** No throughput work, no endgame or HCE work,
no search work. Every open leaf of the archived roadmap stays archived; the
retry map in `EXPERIMENTS.md` section 9 remains valid for whoever reopens
Basilisk.

- [ ] **15.0** Board correctness repairs, before the release gate
    - [x] **15.0.a** `[I2]` **SEE king legality.** In `see_ge` and `see`, when
      the side to move's least valuable remaining attacker is the king, the
      king may capture only if the opponent has no attackers left on the
      target under the current exchange occupancy; otherwise the exchange
      ends before the king move (the Stockfish rule, two lines in the loop).
      Apply it to both kernels identically so `see == see_ge` parity holds at
      every threshold. Tests: port the king-legality cases from Rarog's
      independent fixtures (`D:/code/rarog/tests/data/see-repair-v1.tsv` and
      `see-contract-v1.tsv`; verdicts at threshold zero are value-vector
      independent for the cases that involve no promotion), add them to
      `tests/test_board.cpp` so they **fail on the current kernel first**, and
      re-run the movegen oracle. Record the new bench fingerprint; this is a
      playing change and is gated at 15.1.a. Done criteria: fixtures pass,
      CTest passes, bench recorded with its delta explained by SEE consumers.
      **Done 2026-09-09.** Rule added to both kernels (`src/board.cpp`
      `see()`/`see_ge()`, 12 lines). Nine fixtures in `tests/test_board.cpp`;
      the seven ported from Rarog all **passed on the unrepaired kernel**, so
      the two that fail first are new: the defender of the king's destination
      is itself absolutely pinned. Mechanism correction to BAS-X29: `see_ge`
      was not missing a king rule -- the `KING=20000` sentinel plus the
      minimax fold already implemented it, but it derived the opponent's
      reply from the 8.2 **pin-filtered** attacker set. A pinned piece still
      controls squares against the enemy king, so a pinned defender was
      dropped and the king recaptured illegally. The rule therefore reads the
      **unfiltered** opponent set; pin-filtering it would reintroduce the bug.
      Evidence: 1,896,743 captures over a seeded 4,000-game random-walk
      corpus, kernel diffed against itself. 6,919 `see()` values and 333
      `see_ge` threshold verdicts changed; adjudicated against an independent
      make/unmake legality oracle the repair is correct **6,481-0** on the
      `see()` cases and **301-0** on the `see_ge` cases, with 0 regressions.
      `see`/`see_ge` disagreement fell 4.579% -> 4.236% of captures (the
      residual is the deliberate, SPRT-backed X-ray truncation in `see()`;
      exact parity is not an engine invariant, contrary to this leaf's
      original wording). CTest 12/12 release. **Cost: bench 12,568,898 ->
      14,978,465 nodes, +19.17%**; NPS 3,495,244 -> 3,450,464 (-1.28%).
      Accounted for by the SEE consumers: every call site
      (`search.cpp:1443`, `:1446`, `:1475`, `:1712`, `:1874`, `:1899`, and
      the bad-tactical classification at `:898`/`:1908`/`:2146`) prunes or
      deprioritizes when `see_ge` is FALSE, and the repair moves verdicts
      overwhelmingly the other way -- `see()` rose in 5,946 of 6,919 changed
      cases and `see_ge(m,0)` flipped FALSE->TRUE 119 times against 15 the
      other way -- so less is pruned. The aggregate is tail-driven, not
      uniform: per-position median +3.6%, 15 of 40 positions shrank, p75
      +36.9%, max +207.8%. +19.17% is large enough to be a real risk to
      BAS-E55; it is carried into 15.1.a as a correctness repair, not as a
      strength claim. **Acceptance is provisional pending BAS-E56**, an
      isolated fixed-N cost gate registered 2026-09-09 against this leaf's own
      parent (`ed8db0fc94`); its frozen verdict rule may revert the repair and
      accept the pre-repair behaviour as documented incorrectness. BAS-E55's
      "does not license reverting the repairs" clause was explicitly amended
      for 15.0.a before any exposure.
    - [x] **15.0.b** **Time-forfeit residual: harness reserve sweep,
      BAS-E56 - CLOSED 2026-09-10, rejected at -64.81 Elo, 0 forfeits in
      10,000 games either arm; `Move Overhead` stays 10.** Checked 2026-09-09 against Rarog's A.3.3 diagnosis: Basilisk
      already starts its clock at `go` receipt (`go_recv_time`, Step 5.4) and
      polls it every 2,048 nodes, so the clock-origin repair Rarog needed does
      not apply here, yet the 15.0.a gate forfeited 4 games in 2,927 and
      earlier runs 3 in 24,989 and 1 in 21,994. Those are 50-500 ms stalls
      of a saturated host that no engine can see mid-search; the only lever
      is the reserve. BAS-E56 plays the 15.0.a candidate against itself with
      `Move Overhead` 40 on one side and 10 on the other, 10,000 games fixed,
      in the maintainer's 2026-09-09 night run. Adopt 40 in the harness
      profiles only if the 40 arm forfeits at most a quarter of the 10 arm and
      the paired interval excludes -3 Elo; either way the verdict is recorded
      and no engine source changes.
    - [ ] **15.0.c** `[R2]` **Created pins and recapture promotions.** Port the
      remaining Rarog fixtures for pins created during the exchange and for
      promotion recaptures. Decide with evidence, not by analogy: (1) if the
      failing fixtures are reachable from a production SEE caller (capture
      pruning, qsearch floor, ordering) and the king rule alone does not fix
      them, implement the current-occupancy king-safety recapturer as Rarog
      did (RAR-M28) and accept the measured SEE kernel cost; (2) if they are
      not reachable, or the cost measured by `tests/board_performance.cpp`
      exceeds 10% of the SEE column, record the approximation with its
      fixtures marked expected-fail and a written reason. Either outcome
      closes the leaf. Promotion recaptures add the promotion gain and the
      promoted piece's value; queen suffices for a material-only exchange.
    - [ ] **15.0.d** `[I1]` **Malformed input and counter boundaries.** Add
      tests that fail first for: a non-ASCII move token in `position ...
      moves` (Rarog panicked slicing UTF-8, RAR-M26), a `moves` list longer
      than any history reservation, `go` with absurd or missing values,
      `setoption` with an unknown name, and the FEN fullmove bound already
      enforced at 100000. Malformed input must produce a diagnostic and a
      legal engine state, never a crash. Deterministic; no gate.
    - [ ] **15.0.e** `[V]` **Deterministic qualification.** Debug and release
      CTest, the sanitizer preset, random-walk board invariants
      (`test_invariants`), perft, `bench` recorded on the fixed head, the
      `board_performance_test` SEE column against the 1.9.3 binary (a cost
      report, not a gate), and the ISA check on the PGO asset.
- [ ] **15.1** Release 1.9.4 and freeze
    - [ ] **15.1.a** `[V]` **Registered release gate — BAS-E55.** The 15.0 head
      as a fresh PGO pext build against the 1.9.3 release binary, `3+0.03`,
      1T, Hash 64, paired UHO, no adjudication, `[0,3]` nElo, cap 20,000
      games, plus a 4T `10+0.1` 400-game direction check with zero forfeits.
      Prediction frozen in the ledger row before any game. H0 does not
      license reverting the repairs: it returns 15.0 to research with the
      diagnostics and delays the release.
    - [ ] **15.1.b** `[M]` **Release 1.9.4.** User-facing CHANGELOG entry
      covering everything since 1.9.3 (the accepted endgame and HCE line,
      the SEE repairs, tooling), version strings, README download table,
      `release.yml` PGO assets smoke-tested per `docs/release_tiers.md`, tag
      `v1.9.4` and publish on maintainer instruction only.
    - [ ] **15.1.c** `[M]` **Freeze.** Record the frozen state in HISTORY
      (revision, bench, asset hashes, pool position), merge `dev` into
      `master`, and note the reopening rule. Branch disposition was done early
      on 2026-09-09 at the maintainer's request: `backup` (merged), `nnue`
      (local and origin heads differed; both kept as `archive/nnue-local` and
      `archive/nnue-origin`), `arm_fix` (`archive/arm_fix`), `hybrid` and
      `hybrid-diag` (`oracle/hybrid`, `oracle/hybrid-diag`, cited by BAS-X rows
      and two cluster audits) are tags now and the branches are deleted locally
      and on origin; only `master` and `dev` remain. Reopening rule: any later work starts by
      reading HISTORY and the archived roadmap, not by resuming 6.6.a.

## 4. Number map

Phases 1–4 (closed releases 1.0.0–1.9.3) and Phase 5 (completed foundation)
are history. Phases 6–14 of the archived roadmap are **not continued**; their
open leaves, evidence and retry triggers stay in the archive and in
`EXPERIMENTS.md`. Phase 15 is new and the only active phase.
