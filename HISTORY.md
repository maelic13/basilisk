# Basilisk history

⚠ **THIS FILE IS HISTORY. It does not tell you what to do next.** Read
`GUIDE.md` for the current step and `PLAN.md` for what it involves.

## Numbering, and how to resolve an old reference

The current roadmap uses lettered phases (`A.2.1`, `B.3`); no retired number
is reused. Retired identifiers stay valid in `EXPERIMENTS.md`, `analysis/`,
source comments and commit messages, and resolve here:

| Scheme | Where it appears | Resolve it in |
|---|---|---|
| Phases 1–4 (releases 1.0.0–1.9.3) | changelog, the oldest ledger rows | *Releases* below |
| Phases 5–14 (`5.9.22`, `6.6.a`, `8.4.b`, …) | ledger rows, analyses and commits up to 2026-09-09 | [docs/archive/PLAN-2026-09-09.md](docs/archive/PLAN-2026-09-09.md) and [docs/archive/GUIDE-2026-09-09.md](docs/archive/GUIDE-2026-09-09.md); that PLAN's §15 maps the numbers used before 2026-09-07 |
| Phase 15 and the 1.10.1 patch (`15.0.a`, `15.1.c`) | BAS-C08–C13, BAS-E55–E57, commits to 2026-09-27 | [docs/archive/PLAN-2026-09-28.md](docs/archive/PLAN-2026-09-28.md) and [docs/archive/GUIDE-2026-09-28.md](docs/archive/GUIDE-2026-09-28.md) |
| Current roadmap (`A`–`G`) | `PLAN.md`, `GUIDE.md`, ledger rows from BAS-X30 on | `PLAN.md` |

Where a retired open leaf continues in the current roadmap, the *Number map*
below says where.

## Preserved commits

Commits kept only because no tracked recipe can replace them. None is
reachable from `master` or `dev`. Branches: only `master` and `dev` exist,
locally and on `origin`. Disposition recorded 2026-09-28 (A.2.2).

| Tag | Commit | What it holds | Cited by | Retires when | Disposition |
|---|---|---|---|---|---|
| `oracle/hybrid` | `01df815` | The deficit oracle under `hybrid/`: classical Stockfish `9587eeeb`'s search with a UCI switch selecting Basilisk 1.9.3's HCE or Stockfish's own | PLAN rule 1, A.5.4, A.7.3; PROCESS *Independence boundary* | no open PLAN leaf or PROCESS rule names it | keep |
| `oracle/hybrid-diag` | `324ace4` | `oracle/hybrid` plus a behaviour-neutral qsearch counter in `hybrid/stockfish/src/search.cpp` | BAS-D03; `analysis/cluster55_audit_v1.md` §3 | A.5.3 commits the oracle's counter set including the qsearch share, so BAS-D03 reproduces from `dev` | keep |
| `archive/nnue-local` | `1431a89` | The 2026-07 NNUE bring-up: scalar `.mnn` loader (`src/nnue.*`), `UseNNUE`/`EvalFile`, `test_nnue` with `tests/data/test_h16.mnn` vectors | PLAN F.4 | F.4 closes, or F.0 chooses a network format other than `.mnn` | keep |
| `archive/nnue-origin` | `4fa11bf` | An ancestor of `archive/nnue-local`; nothing it reaches is lost without it | none | fired: redundant | removed 2026-09-28 |
| `archive/backup` | `ae4af1a` | The pre-squash line of 2026-08-05 → 08-11, including the withdrawn Colosseum adoption (`3cbf90b`); an ancestor of `oracle/hybrid` | none | fired: redundant | removed 2026-09-28 |
| `archive/arm_fix` | `67a987b` | One commit aligning the TT to 64-byte lines for Apple (`src/tt.h`) | BAS-P07, which states the mechanism and why it was rejected | fired: hypothesis rejected, the row carries the recipe | removed 2026-09-28 |

Reviewed again 2026-10-07 (A.8.3), when `dev` began reaching `master` by
merge commits: none of the three kept tags is reachable from `dev`, so the
change retires none.

The kept tags are annotated with their reason and retirement condition. The
removed ones were deleted locally and on `origin`; their commits stay
resolvable only while unpruned, and the rows above carry what they held.
Removing a tag and pushing a tag are the maintainer's commands.

## Releases (Phases 1–4)

| Release | What it established |
|---|---|
| 1.0.0 – 1.8.0 | Board, move generation, UCI, PVS/qsearch, TT, histories, SEE, Syzygy, time management, Lazy SMP, reproducible tests, accepted HCE |
| 1.9.0 | State, repetition/rule-50, TT/mate and SEE/pin correctness; staged ordering, correction/history, root-instability timing, dense TT |
| 1.9.1 | Centralized parameters, invariants, fuzzing, CI, telemetry; behaviour-identical PGO speed pass at +4.34% NPS |
| 1.9.2 / 1.9.3 | SPSA/MT harness and helper clock/node/thread safety repaired; four-thread bundle accepted; PGO tool matching fixed |

## The 2026 development line (Phases 5–6)

Between 1.9.3 (2026-08-01) and 1.10.0 the `dev` branch accumulated the endgame
and hand-crafted-evaluation work below. What it established, each with its
ledger evidence:

| Result | Evidence |
|---|---|
| The KBNK mate drive completed and provably isolated; conversion measured on fixed-seed cohorts with deterministic CTest floors | BAS-E42 and the 6.1 record |
| Endgame Group A gated and accepted; the accepted HCE line about +12 Elo over 1.9.3 | 6.2 record, BAS-E4x |
| Rook-ending draw scaling (KRPKR, KRPPKRP) with a floored discount, accepted at +3.29 ± 4.61 Elo over frozen Group A; bench 12,568,898 | BAS-E54 |
| Passed-pawn king approach studied and dispositioned | 6.3 record |
| Magnitude and coverage audit of the endgame evaluator; reopened work recorded with its reasons | 6.4 record, "Reopened work, 2026-09-03" |
| The TT publication redesign: atomic whole-record word accepted for correctness, the coherence repair reverted on measured throughput with the risk recorded | BAS-X2x rows, commits `2bf43fb`, `c378706` |
| A peer audit of the shared board lineage found three SEE defects of the same kernel shape; Basilisk had no fixtures for them | BAS-X22 |
| Pool position at `3+0.03` 1T | **Sourced 2026-10-01 from the 2026-09-15 Super Rating Tournament PGN**: Basilisk 1.10.0 scored 15-26-159 against Houdini 3, 25-35-140 against Critter 1.6a, 25-45-130 against Fritz 16 and 44-38-118 against Rybka 4.1, 200 games per pair with 100 per colour and zero unfinished. The old 2026-09-04 gaps were unsourced and are retired. BAS-M08; PGN SHA-256 `4e87a36a030dfc696c9328f9f34f60b94784a5303af4c8be92d2db6b2c05103c` |

## Phase 15 (2026-09-09 → 2026-09-10)

Board correctness, then the 1.10.0 release. Summarised in `PLAN.md`; the
measured detail is in `EXPERIMENTS.md` (BAS-C08, BAS-C09, BAS-E55, BAS-E56,
BAS-E57).

| Result | Evidence |
|---|---|
| SEE king legality repaired: the exchange now ends before an illegal king recapture, reading the unfiltered attacker set | BAS-C08, 6,481–0 and 301–0 against an independent legality oracle |
| Created pins and promotion recaptures kept as documented approximations | BAS-C09, 0 verdict changes in 339,607 production calls; repair costs +16.8% of the SEE column |
| Malformed UCI input hardened; unknown `setoption` names now diagnosed | 15.0.d, 17 test sections |
| Deterministic qualification clean | 15.0.e: CTest 12/12 release and sanitizer, 6/6 perft exact, invariants across four seeds |
| 1.10.0 accepted against 1.9.3 at +19.18 ± 6.76 Elo, 1T `3+0.03` | BAS-E55 |

## Patch 1.10.1 (2026-09-27)

Two ponder-on time forfeits in a Colosseum `120+1` 4T tournament had one
cause: a `ponderhit` that arrived during search setup was reset by that setup
(BAS-C10). The first ponder-on gate then failed on a second, older defect:
setup work charged to the clock, up to 0.3 s per move from tablebase line
extension in 5–6-man positions (BAS-C11). A rejected `position` became fatal
instead of searching the previous board (BAS-C12), and tablebase PV lines were
restored the Stockfish way, time-boxed at half of Move Overhead (BAS-C13).
The second ponder-on gate passed: 1,005 games, 0 failures for 1.10.1-rc3
against 263 time losses for 1.10.0, all in positions of six men or fewer. No
Elo SPRT: search and evaluation are bit-identical, and fastchess cannot
ponder. The full record, with both gate registrations and calibrations, is in
the archived PLAN of 2026-09-28.

## What the Phase 5–15 line established, and why the roadmap was rewritten

Phase 5 measured where the strength was. Stockfish `9587eeeb`'s search
driving Basilisk's own HCE beat Basilisk by +322.7, and its HCE beat Basilisk's
under the identical search by +232.8 (BAS-O01, BAS-O02): the search was the
larger deficit, and at fixed nodes 98.4% of the width gap followed the search
(BAS-O04). The search track then attacked it one mechanism at a time.
Reduction magnitude was refuted on the harness before any game (BAS-S13–S15),
and the check-move depth bundle lost −3.48 ± 3.32 (BAS-S16). Cluster 5.4
closed on the hypothesis that width is a symptom of the weaker evaluator, and
the roadmap turned to endgame maturity (Phase 6: Group A and the 6.5.a rook
scaling, +3.29 ± 4.61) and then board correctness and the 1.10.0 release
(Phase 15, +19.18 ± 6.76 over 1.9.3).

Over the same weeks Rarog, starting from the same oracle finding and a weaker
HCE, rebuilt its search as a donor-shaped architecture adopted as a unit,
fitted it by SPSA in blocks and gated it in clusters. It gained about 230 pool
Elo in fifteen days (3001 → 3233, RAR-M57, RAR-M63) and passed Basilisk by
+200. The 2026-09-28 rewrite adopts that method and its order of steps, with
modern Stockfish as the architecture donor and classical Stockfish `9587eeeb`
as the evaluation donor and oracle. Every open leaf of the archived roadmaps
continues under a new identifier except archived Phase 14 (an optional HCE
fallback), which is redundant because the HCE stays in the tree as the NNUE's
datagen baseline and fallback.

## Number map: retired open leaves that continue in the current roadmap

Every identifier below is retired. Completed retired leaves stay history; this
map covers only open work.

| Retired (archive 2026-09-09) | Continues as | Note |
|---|---|---|
| 6.6.a–6.6.e, 6.6.h | C.5.1 | endgame instruments and gate integrity |
| 6.6.f | A.3.1 | harness provenance refusals, shared guards |
| 6.6.g | A.4.2 | build option and ISA combination matrix |
| 6.7.a–6.7.d | C.5.2 | occurrence, classification, closure, ranking |
| 6.8.a | C.5.4 | the BAS-E53 only-move defect |
| 6.8.b–6.8.e | C.5.5 | Group B families by measured kind |
| 6.9.a–6.9.b | C.5.7 | occurrence-tiered endgame gates |
| 6.10.a–6.10.d | C.5.6 | lower-yield remainder |
| 6.10.e, 6.11.a–6.11.d | C.5.7 | gate and closure |
| 7.0 | A.4.1 | toolchain refresh and freeze |
| 7.1, 7.7, 7.9–7.12 | D.3.1 | board contract audit, a defect hunt |
| 7.2–7.8 | B.7.2 | board speed, only if a profile makes it hot |
| 8.0.a–8.0.d | C.0 | HCE surface audit |
| 8.0.e | C.1 and each C cluster | categorical tests and the architecture freeze |
| 8.1.a–8.1.h | C.2.1 | fit-tooling contract, Texel handbook |
| 8.2.a–8.2.e | C.2.2 | corpus design |
| 8.3.a–8.3.c | C.2.3 | corpus A |
| 8.4.a–8.4.f, 8.5.a–8.5.d | C.2.4 | matched label arms; whole-game tablebase adjudication |
| 8.6.a–8.6.c | C.2.5 | initialization control |
| 8.7.a–8.7.e, 8.8.a–8.8.d | C.2.6 | matched fits and the label-contract gate |
| 8.9.a–8.9.e | C.8 | refit cycles |
| 8.10.a–8.10.e | C.9 | nonlinear HCE SPSA |
| 8.11.a–8.11.d | C.11 | checkpoint and freeze |
| 8.12.a–8.12.d | C.12 | evaluation throughput, bit-exact |
| 9.0.a–9.0.c | B.0 | search audit, interactions, oracle profile |
| 9.1.a–9.1.b | B.3 | extension authority |
| 9.2.a–9.2.c | B.4 (research card), C.10 | SEE and move-ordering value scale |
| 9.3 | B.0, B.2 | TT and caches, inside cluster 1 |
| 9.4 | D.3.2, D.4 | lifecycle, protocol, tablebases |
| 9.5 | A.3, A.5, B.8 | harness and diagnostics audit |
| 9.6.a–9.6.e | B.2.3, B.6 and each cluster's SPSA | search SPSA |
| 9.7.a–9.7.d | D.1 | time management |
| 9.8.a–9.8.c | B.8, B.9 | cleanup and checkpoint |
| 9.9 | E.1 | attribution checkpoint |
| 9.10.a, 9.10.c, 9.10.d | E.3.2 | classical release |
| 9.10.b | D.2 | SMP quality |
| 9.11.a–9.11.d | G.2 | optional universal binary |
| 10.0–10.4 | F.0, F.1 | NNUE runway |
| 11.0–11.5 | F.2–F.6, F.9 | baseline NNUE and its release |
| 12.0.a–12.0.c | F.7, F.8 | architecture ladder, data frontier |
| 12.1.a–12.1.b | G.3 | selective search after NNUE |
| 13.0 | G.1 | high-thread and NUMA |
| 13.1 | G.2 | platforms and delivery |
| 14.0 | dropped | the HCE stays in the tree as datagen baseline and fallback |
| "Phase 16" (named by the archived Phase-15 PLAN) | never opened | superseded by the lettered roadmap |

Moved within the current roadmap on 2026-10-06 (maintainer decision; A.8):

| Was | Now | Note |
|---|---|---|
| E.3.1 | A.8.6 | tag-driven release flow, done now rather than before E.3 |
| D.3.3 | A.8.19 | displayed-score normalisation |
| E.3.2 | E.3 | the release, a leaf once E.3.1 left |

## Completed current-roadmap work (dated records; PLAN owns IDs)

- **2026-09-28 — PLAN A.1 CLOSED: document reset.** The roadmap was rewritten
  on Rarog's model with Stockfish as the donor: new PLAN, GUIDE and PROCESS;
  AGENTS merged with Rarog's rules; DESIGN, HISTORY and the ledger's live
  sections brought up to date. The Phase-15 roadmap was archived verbatim
  (`6543ccf`). `check_roadmap.py` learned lettered IDs, the `(ANY TIME)`
  ordering exemption, the register check and the fingerprint check
  (`01cc84a`). `dev` was recreated from `master` at `38c42e6`. Maintainer
  decisions taken with it: the donor roles, Rarog's four target engines at 1T
  and 4T, one release at E.3 (2.0.0 if the target gate is met, else 1.11.0),
  and Colosseum CLI as the main harness from Phase A. Amended the same day at
  the maintainer's request, so the plan would survive Rarog's deletion:
  everything it takes from Rarog was pinned as a verbatim snapshot in
  `docs/reference/rarog/` (Rarog `015bccae`, 274 files byte-identical to
  their blobs, plus the two oracle tags as patches; `e5ad70b`). The checker
  verifies its manifest (`e948979`), Rarog's measurements of Basilisk were
  imported as BAS-X34, and two bundles of Rarog's full history were kept in
  ignored storage with a tested restore recipe.

## Release record — 1.10.1

| Item | Value |
|---|---|
| Version | **1.10.1** (2026-09-27) |
| Scope | Time-forfeit fixes only: a `ponderhit` arriving during search setup was discarded (BAS-C10); setup work charged to the clock -- tablebase root lines (95-300 ms per move in 5-6-man positions), hash allocation/clearing on the first move, KPK table build (BAS-C11); a rejected `position` now exits with status 1 instead of searching the previous position, and triple check is rejected (BAS-C12); tablebase PV lines restored the Stockfish way, time-boxed at half of Move Overhead (BAS-C13) |
| Bench-13 fingerprint | **14,978,465**, unchanged from 1.10.0 — search and evaluation bit-identical |
| Strength gate | No Elo SPRT (bench-identical; fastchess cannot ponder). Ponder-on game gate with `tools/ponder_match.py` against 1.10.0 as positive control: **PASS**. 1,005 games at `3+0.03` 1T: 0 failures for 1.10.1-rc3, 263 time losses for 1.10.0 (PLAN "Patch 1.10.1"). BAS-C13, added after the gate: a 200-game ponder-on smoke run and 263 games of a gate rerun on rc4 (stopped by the maintainer), 0 failures of any kind, while 1.10.0 lost 63 on time |
| Qualification | Regression tests fail on 1.10.0 and pass; release CTest 12/12; ASan/UBSan CTest 12/12 at BAS-C11, and on the six affected test binaries after BAS-C12; black-box zero-delay UCI stress; TSan on the ponder/threading/protocol tests (BAS-C10 repair) |

The release revision is the `Version 1.10.1` commit on `master`, tagged
**`v1.10.1`**. As for 1.10.0, the published binaries are the assets built by
`release.yml` and attached to that release.

## Release record — 1.10.0

| Item | Value |
|---|---|
| Version | **1.10.0** (2026-09-10) |
| Bench-13 fingerprint | **14,978,465** |
| Previous release | 1.9.3, bench 11,941,440 |
| Strength | **+19.18 ± 6.76 Elo** over 1.9.3, `3+0.03` 1T, H1 accepted at 4,224 games, LOS 100% (BAS-E55) |
| 4T smoke gate | Clean: zero crashes, zero time forfeits, 95% lower bound −1.17 Elo |
| Qualification | CTest 12/12 release and 12/12 under ASan/UBSan; perft exact on all six standard positions; `test_invariants` 18/18 across four seeds |
| Pool position, `3+0.03` 1T (2026-09-15 tournament, recounted 2026-10-01) | Houdini 3 15-26-159, Critter 1.6a 25-35-140, Fritz 16 25-45-130, Rybka 4.1 44-38-118; 200 games per pair, 100 per colour, zero unfinished (BAS-M08) |
| Accepted risks carried | TT publication coherence (BAS-C05); SEE created-pin and promotion-recapture approximations (BAS-C09) |

The release revision is the `Version 1.10.0` commit on `master`, tagged
**`v1.10.0`** — the durable identifier, since the squash SHA is not stable
across a re-merge. Published binaries are the nine assets built by
`release.yml` and attached to that release; per-asset hashes are not recorded
here, following the 8.6.5 local-only-manifest decision that keeps per-asset
data out of the repository (see `docs/release_tiers.md`). The locally built
reference asset used for the tier smoke tests was
`basilisk-v1.10.0-windows-x86_64-pext-pgo.exe`, sha256
`542247ca44a90d74abc793eb6cb121171e2b1224a78310c0cbb4e9915b522913`; the
published assets are CI-built and will differ.
