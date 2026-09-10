# Basilisk development plan

Phase 15 is complete and **1.10.0 is released**: the board-correctness defects
a peer audit demonstrated are repaired or documented, the release gate passed,
and `master` carries the `Version 1.10.0` commit tagged `v1.10.0`.
**The next phase is not yet planned** — see section 4.

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
| Latest release | Basilisk **1.10.0**, tagged `v1.10.0` on `master` |
| Development branch | `dev`, level with the release |
| Bench-13 fingerprint | **14,978,465**; CTest 12/12 release and sanitizer |
| Previous release | Basilisk 1.9.3; bench 11,941,440 |
| Strength | **+19.18 ± 6.76 Elo** over 1.9.3 at `3+0.03` 1T, H1 accepted at 4,224 games (BAS-E55) |
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
