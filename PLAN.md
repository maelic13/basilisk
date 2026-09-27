# Basilisk development plan

Phase 15 is complete and **1.10.1 is released**: `master` carries the
`Version 1.10.1` commit tagged `v1.10.1`. It is a correctness patch on the
Phase 15 strength release 1.10.0, with time-forfeit and position-handling
fixes and bench unchanged (see "Patch 1.10.1" below). **The next phase is not
yet planned** — see section 4.

The previous roadmap (Phases 5–14, endgame maturity through NNUE) is archived
verbatim at
[docs/archive/PLAN-2026-09-09.md](docs/archive/PLAN-2026-09-09.md) with its
GUIDE at [docs/archive/GUIDE-2026-09-09.md](docs/archive/GUIDE-2026-09-09.md);
[HISTORY.md](HISTORY.md) records what that roadmap established, what Phase 15
added, and the open work it did not continue. `EXPERIMENTS.md`, `DESIGN.md`
and `AGENTS.md` are unchanged.

## 1. Current checkpoint

| Item | State |
|---|---|
| Latest release | Basilisk **1.10.1**, tagged `v1.10.1` on `master` — time-forfeit and position-handling fixes (BAS-C10, BAS-C11, BAS-C12) |
| Development branch | `dev`, level with the release |
| Bench-13 fingerprint | **14,978,465** (unchanged since 1.10.0); CTest 12/12 release and sanitizer |
| Previous release | Basilisk 1.10.0; same bench |
| Strength | **+19.18 ± 6.76 Elo** for 1.10.0 over 1.9.3 at `3+0.03` 1T, H1 accepted at 4,224 games (BAS-E55); 1.10.1 is bench-identical to 1.10.0 |
| Pool position, `3+0.03` 1T (2026-09-04) | Houdini 1.5a −197, Critter 1.6a −187, Fritz 16 −178, Rybka 4 −84 |
| Current phase | **None — Phase 16 needs planning** |
| Long job | None |

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

## Phase 15 — board correctness and release 1.10.0 (complete)

A peer audit of the shared board lineage identified three SEE defects of a
kernel shape Basilisk also carried, with no fixtures covering them. Phase 15
repaired what was worth repairing, documented what was not, hardened malformed
UCI input, qualified the result deterministically, and released 1.10.0. The
measured detail lives in `EXPERIMENTS.md`; this is the summary.

| Step | Outcome |
|---|---|
| **15.0.a** SEE king legality | **Repaired.** The exchange now ends before an illegal king recapture. The old kernel was not missing a king rule — the `KING=20000` sentinel implemented one — but it read the *pin-filtered* attacker set, which drops a defender pinned against its own king even though such a piece still controls squares. The rule therefore reads the **unfiltered** set. Correct in 6,481/6,481 changed `see()` values and 301/301 changed threshold verdicts against an independent legality oracle over 1,896,743 captures. Bench 12,568,898 → 14,978,465. Strength-neutral in isolation (BAS-E56, −0.65 ± 5.25). |
| **15.0.b** Time-forfeit residual | **Closed.** The clock already starts at `go` receipt and is polled every 2,048 nodes. Forfeits are host stalls, not an engine defect; a reserve sweep was rejected at −64.81 Elo (BAS-E57) and `Move Overhead` stays 10. |
| **15.0.c** Created pins, promotion recaptures | **Kept as documented approximations** (BAS-C09). Three of four created-pin fixtures already passed; only a pin created *mid-exchange* fails. The promotion-recapture errors cancel exactly when the promoted piece is recaptured. Neither changed a verdict in 339,607 production SEE calls, and repairing the first costs +16.8% of the SEE column against a 10% ceiling. Nine fixtures pin both the truth and the approximation. |
| **15.0.d** Malformed input | **One real defect fixed:** an unknown `setoption` name fell through in silence and now reports `info string Unknown option: '<name>'`. The other four categories — non-ASCII move tokens, over-long `moves` lists, absurd `go` values, FEN counter bounds — were already correct and are now covered by 17 test sections. |
| **15.0.e** Deterministic qualification | **Clean.** CTest 12/12 release and 12/12 under ASan/UBSan; `test_invariants` 18/18 across four seeds; all six standard perft positions exact (~594M nodes); SEE column measured *faster* than 1.9.3; PGO asset ISA verified. |
| **15.1.a** Release gate (BAS-E55) | **Passed.** 1T `3+0.03` vs 1.9.3: **+19.18 ± 6.76 Elo**, H1 accepted at 4,224 games, LOS 100%. A 4T smoke gate was clean — zero crashes, zero forfeits, 95% lower bound −1.17 Elo. |
| **15.1.b** Release 1.10.0 | **Done.** CHANGELOG, version bumped in both sources of truth, README download table verified, per-tier ISA smoke tests recorded in `docs/release_tiers.md`. |
| **15.1.c** Publish | **Done.** `dev` merged into `master` as the single `Version 1.10.0` commit, tagged `v1.10.0`, and the release published — which is what triggers `release.yml` (it fires on `release: published`, not on a tag push) to build and upload the nine matrix assets. |

Two standing lessons came out of this phase and are recorded in
`EXPERIMENTS.md`:

- **Convert a node-count delta into plies before calling it expensive.** The
  king-legality repair's "+19.17% bench nodes" is **0.165 ply** at the measured
  EBF of 2.900, and it measured strength-neutral. A percentage of nodes is not
  a cost until it is divided by `log(EBF)`.
- **Pin filtering and king-move legality are different questions.** One asks
  "may this piece recapture?", the other "is this square controlled?". A single
  attacker set cannot serve both.

## Patch 1.10.1 — time forfeits and position handling (released)

Two time forfeits by 1.10.0 in a `120+1` four-thread Colosseum tournament with
ponder on — one from a position with mate in 3 — had one cause: when the
opponent replied within about a millisecond, `ponderhit` reached the UCI thread
while the engine thread was still setting up the ponder search, and that setup
reset the flag. The search then pondered without a clock, or waited forever at
its depth cap. Measured and repaired as **BAS-C10** in `EXPERIMENTS.md`.

| Step | Outcome |
|---|---|
| Diagnosis | Reproduced with the 1.10.0 binary on both incident positions (5/5 hangs each) and an ordinary middlegame (4/4); a 100 ms delay before `ponderhit` never hangs. Not position-specific. Unrelated to BAS-E57, whose forfeits were ponder-off. |
| Repair | `ponderhit` flag owned by the UCI thread: set on `ponderhit`, reset on `go`, never written by the engine thread. The pre-search stale-`stop` reset now re-checks the control epoch so a `stop` received during setup survives. |
| Qualification | Two new engine tests fail on 1.10.0 and pass on the fix; stale-ponderhit and protocol-ownership tests added. Both incident positions 0/5 hangs after the fix. Release CTest 12/12, ASan/UBSan CTest, ThreadSanitizer on the ponder/threading/protocol tests. Bench **14,978,465** unchanged. |
| Independent re-verification (2026-09-27) | Fresh PGO build of `d194888` (`basilisk-1.10.1-rc`) and the 1.10.0 release binary both bench **14,978,465**. Release CTest 12/12; ponder/threading/protocol tests 30/30 repeats. Black-box UCI stress, zero delay between commands, 20 trials per scenario: `go ponder` + `ponderhit` (pending hash resize, `ucinewgame`, 6-man Syzygy root, depth cap, plain 4T, plain 1T) **1.10.0 hangs 20/20 in every scenario, 1.10.1 0/20**; `go ponder`/`go infinite` + immediate `stop` and plain `go` 0/20 on both; an 80-move simulated ponder game with random 0–40 ms hit/miss timing 0/80 on 1.10.1. **Clerical correction (2026-09-27):** the "6-man Syzygy root" position used was illegal (the side not to move was in check), so it never exercised tablebases. Rerun on the legal `k7/8/8/8/8/2BB4/4P3/4K3 w`: 1.10.0 hangs 10/10 with a clock and 10/10 at a depth cap; rc2 0/10. |
| Release 1.10.1 | Released 2026-09-27. The version is bumped in both sources of truth, and CHANGELOG, README, HISTORY and this record are updated. `dev` is squash-merged into `master` as `Version 1.10.1`, tagged `v1.10.1`, and published by `release.yml`. |

**No Elo SPRT.** Search and evaluation are bit-identical, and every harness
game Basilisk has played is ponder-off: fastchess has no ponder support. In
ponder-off play the only changed path, the stop re-check, needs a control
command to arrive during search setup, which fastchess never sends. A
ponder-off SPRT between these binaries is therefore a null match that cannot
fail for any reason connected to this fix.

**Ponder-on game gate, run 1 (registered 2026-09-27, before any gate games).** The
deciding gate plays with Ponder on, which the fixed defect lives in.
`tools/ponder_match.py` is a clocked referee that drives the GUI ponder
protocol: `go ponder` after each move, then `ponderhit` the instant the
opponent plays the predicted move, or `stop` followed by a fresh `go`. Setup:
`basilisk-1.10.1-rc` vs `basilisk-1.10.0-release` (the positive control),
1,000 games (500 UHO openings, both colours), `10+0.1`, Threads 1, Hash 64,
Syzygy 3-4-5-6, concurrency 7 (both engines search at once, so 2 cores per
game), time margin 100 ms, seed 20260927, no adjudication.

- **Pass:** 1.10.1 records zero failures of every kind (forfeit, hang,
  stop stall, bestmove while pondering, illegal move, crash), **and** 1.10.0
  records at least one, which proves the run actually offered the race.
- **Fail:** any 1.10.1 failure blocks the release until it is diagnosed from
  `failures.txt` and the PGN.
- **Inconclusive:** both engines are clean. The race was not exercised, so the
  run is repeated at `--threads 4`; it is not counted as a pass.
- Elo is reported but does not decide anything, because 1.10.0's forfeits
  inflate it.
- **Prediction (frozen):** 1.10.1 records 0 failures (confidence 95%). 1.10.0
  records tens of forfeits, most after a ponderhit sent within about 1 ms of
  its `go ponder`: smoke runs gave 1 forfeit in 10 games at `5+0.05`. A
  1.10.1 failure after a plain `go` would point to host noise or a different
  defect, not BAS-C10.

**Run 1 result (2026-09-27): FAIL, diagnosed as a separate defect (BAS-C11).**
The run was interrupted at about 130 games (`tools/results/ponder_20260927_171421`).
1.10.0 hung about 35 times, so the positive control fired. 1.10.1-rc had no
hang, stop stall, early bestmove, illegal move or crash, but lost **4 games
on time** with 27–630 ms left. Every time loss by either engine was in a
5–6-man position. The cause is not BAS-C10: `start_search` built a tablebase
line for every winning root move before searching, costing 95–297 ms per move
on the engine's clock, and 1.10.0 pays the same cost. Calibration against the
frozen prediction: "1.10.1 records 0 failures (95%)" was wrong. The causal
model assumed search setup was negligible outside the `ponderhit` race; in
tablebase positions it was up to 0.3 s, which also widened BAS-C10's own window
far beyond ~1 ms (1.10.0 hung on ponderhits up to 132 ms late).

| Step | Outcome |
|---|---|
| Repair (BAS-C11) | Tablebase line extension removed: the searched PV and ponder move are used, and the move played is unchanged. KPK is built at start-up. Hash resize and clear run when `setoption` / `ucinewgame` / `Clear Hash` are processed, before `readyok`, not after `go`. These follow Rarog `9a7b663`. Bench **14,978,465** unchanged. The six losing positions take 3–8 ms of setup (from 95–297 ms); the first KPK search 0.2 ms (from 9.8); Hash 1024 + `ucinewgame` on the first move 0.4 ms (from ~540). |
| Qualification | Release CTest 12/12 and ASan/UBSan CTest 12/12. Ponder/threading/protocol/search tests pass 20/20 repeats. A new hash-setup test fails on the old code. Zero-delay ponder stress, including a legal 6-man tablebase root: 0 failures on `basilisk-1.10.1-rc2`. |
| Position policy (BAS-C12) | Maintainer decision 2026-09-27: a rejected `position` exits with status 1 after an `info string CRITICAL ERROR` line. This replaces 8.6.3a's reject-and-retain, under which `go` searched the previous board. Triple check is now rejected. Bench **14,978,465**; release CTest 12/12; ASan/UBSan on the six affected test binaries. Built as `basilisk-1.10.1-rc3`. |

**Ponder-on game gate, run 2 (registered 2026-09-27, before any run-2
games).** Same instrument, conditions, seed and verdict rules as run 1.
Candidate `basilisk-1.10.1-rc3` (source revision in its manifest), control
`basilisk-1.10.0-release`, 1,000 games. *Clerical amendment, 2026-09-27,
before any run-2 game:* the candidate changed from rc2 to rc3 to include
BAS-C12. rc3 plays identically to rc2 on every position the referee sends,
because it only ever sends legal positions. Pass requires zero 1.10.1-rc3
failures of every kind and at least one 1.10.0 failure.
**Prediction (frozen):** 1.10.1-rc3 records 0 failures (confidence 85%,
lowered after run 1). 1.10.0 records well over 100 failures, almost all in
5–6-man positions (confidence 90%; run 1 gave ~35 in ~130 games). If
1.10.1-rc3 fails, the per-failure context in `failures.txt` separates host
noise (overrun after a plain `go` with an ample clock) from a defect.

**Run 2 result (2026-09-27): PASS.** Deviations from the registration, all
set by the maintainer and all disclosed:
- **Time control and size, set before the run started:** `3+0.03` and a
  2,000-game schedule. This followed a 100-game `3+0.03` pilot of rc3 vs
  1.10.0 (0 rc3 failures, 22 control failures), which showed the referee's
  100 ms margin holds at that time control. The pilot is not pooled.
- **Stopping:** the run was stopped at the originally registered size, 1,000
  games, while rc3 had no failures (`tools/results/ponder_20260927_191821`).

Results:
- **1,005 completed games.** The seven games in flight at the stop (1002–1008)
  died together, both engines included. rc3 accepts all seven final
  positions, so those deaths were the interruption, not BAS-C12's fatal
  `position` path.
- **1.10.1-rc3: zero failures of every kind.** The 95% upper bound is 0.3% of
  games.
- **1.10.0: 263 time losses** (231 hangs, 32 overruns), every one of them in a
  position with 6 or fewer men.

The score, +466 =438 −108 for rc3, is inflated by those losses and decides
nothing. Calibration: both frozen predictions held. rc3 had 0 failures
(stated at 85%); 1.10.0 had "well over 100, almost all in 5–6-man
positions" (stated at 90%), and in fact all 263 were in positions of 6 men or
fewer.

## 4. The next phase is not planned

Phase 15 closed the board-correctness work and shipped 1.10.0. **No Phase 16
exists yet, and none should be started before it is planned.** Whoever picks
this up next should plan it rather than resume an old leaf from memory.

How to plan it:

1. Read [HISTORY.md](HISTORY.md) first — what the archived roadmap established,
   what Phase 15 added, and the open work that was not continued.
2. Read `EXPERIMENTS.md` **section 9**, the retry map. It records which
   mechanisms were rejected, on what evidence, and the objective trigger that
   would justify retrying each. A rejection without its trigger fired is not a
   candidate.
3. Re-measure before believing any premise carried in the archived roadmap.
   Phase 15 twice found a load-bearing claim to be wrong: BAS-C08 corrected the
   stated mechanism of the SEE king defect, and BAS-C09 found three of four
   "created pin" fixtures already passing.
4. Apply the four questions in `DESIGN.md` before writing a diff, and the
   refusal duty in `AGENTS.md` — a reasoned "do not build this" is a
   deliverable of equal standing.

Where the open work sits, from the archived roadmap: 6.6 instrument and gate
integrity; 6.7–6.11 remaining endgame families and closure; Phase 7 board
correctness beyond Phase 15's bounded repairs; Phase 8 corpus and a complete
HCE refit; Phase 9 classical search consolidation; Phases 10–14 NNUE and
platforms. Their evidence and retry triggers are in `EXPERIMENTS.md` and
[docs/archive/PLAN-2026-09-09.md](docs/archive/PLAN-2026-09-09.md).

## 5. Number map

Phases 1–4 (closed releases 1.0.0–1.9.3) and Phase 5 (completed foundation)
are history. Phases 6–14 of the archived roadmap are not continued; their open
leaves, evidence and retry triggers stay in the archive and in
`EXPERIMENTS.md`. Phase 15 is complete. The next new phase takes number 16.
