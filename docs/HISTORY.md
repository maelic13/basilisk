# Basilisk history

⚠ **THIS FILE IS HISTORY. It does not tell you what to do next.** Read
`GUIDE.md` for the current step and `PLAN.md` for what it involves.

## Numbering, and how to resolve an old reference

The current roadmap uses lettered phases (`A.2.1`, `B.3`); no retired number
is reused, with three declared exceptions: Phase A's 2026-10-24 leaves were
renumbered the same day to run in order; on 2026-10-27 fail-soft
quiescence moved from A.11.10 to A.11.11; and on 2026-11-05 the open steps
of Phases B to D were renumbered in the order they run. The *Number map*
below resolves the earlier numbering of all three. On 2026-11-11 open
leaves of B.6, B.9, B.11, C.2, C.7 and F moved to new numbers, none of
them reused; the map resolves those too, and C.10.2's move to C.10.5 on
2026-11-26. Retired identifiers stay valid in `EXPERIMENTS.md`,
`analysis/`, source comments and commit messages, and resolve here:

| Scheme | Where it appears | Resolve it in |
|---|---|---|
| Phases 1–4 (releases 1.0.0–1.9.3) | changelog, the oldest ledger rows | *Releases* below |
| Phases 5–14 (`5.9.22`, `6.6.a`, `8.4.b`, …) | ledger rows, analyses and commits up to 2026-09-09 | [docs/archive/PLAN-2026-09-09.md](archive/PLAN-2026-09-09.md) and [docs/archive/GUIDE-2026-09-09.md](archive/GUIDE-2026-09-09.md); that PLAN's §15 maps the numbers used before 2026-09-07 |
| Phase 15 and the 1.10.1 patch (`15.0.a`, `15.1.c`) | BAS-C08–C13, BAS-E55–E57, commits to 2026-09-27 | [docs/archive/PLAN-2026-09-28.md](archive/PLAN-2026-09-28.md) and [docs/archive/GUIDE-2026-09-28.md](archive/GUIDE-2026-09-28.md) |
| Current roadmap (`A`–`G`) | `PLAN.md`, `GUIDE.md`, ledger rows from BAS-X30 on | `PLAN.md`; a closed step's text in the `docs/archive/PLAN-closed-*.md` file its phase summary links |

Where a retired open leaf continues in the current roadmap, the *Number map*
below says where.

## Preserved commits

Commits kept only because no tracked recipe can replace them. None is
reachable from `master`. Disposition recorded 2026-09-28 (A.2.2); the
`archive/dev-*` rows 2026-10-08. Since 2026-10-09 no tag or branch is added
here without the maintainer's agreement: a branch's commits stay reachable
under its pull request (`refs/pull/<n>/head`) after the squash, and a
throwaway branch is cited by its recipe (PROCESS *Delivery*).

| Tag | Commit | What it holds | Cited by | Retires when | Disposition |
|---|---|---|---|---|---|
| `oracle/hybrid` | `01df815` | The deficit oracle under `hybrid/`: classical Stockfish `9587eeeb`'s search with a UCI switch selecting Basilisk 1.9.3's HCE or Stockfish's own | PLAN rule 1, A.5.4, A.7.3; PROCESS *Independence boundary* | no open PLAN leaf or PROCESS rule names it | keep, local only from 2026-11-23: it holds Stockfish's source, and no other engine's code reaches the remote (`analysis/licence_audit_2026-11-23.md`); backup bundle `tools/results/oracle-tags/oracle-tags-2026-11-23.bundle` |
| `oracle/hybrid-diag` | `324ace4` | `oracle/hybrid` plus a behaviour-neutral qsearch counter in `hybrid/stockfish/src/search.cpp` | BAS-D03; `analysis/cluster55_audit_v1.md` §3 | A.5.3 commits the oracle's counter set including the qsearch share, so BAS-D03 reproduces from `dev` | keep, local only from 2026-11-23, as `oracle/hybrid` |
| `archive/dev-1.10.0` | `3fe85c2` | The development line from 1.9.3 to the published 1.10.0 (Phases 5–15, 239 commits), deleted with `dev` after the 1.10.0 squash | 32 commit hashes in the ledger and `analysis/`, found unreachable from any ref on 2026-10-08 | never: the records cite it | keep |
| `archive/dev-1.10.2` | `63ac175` | The development line of 1.10.1 and 1.10.2 (the PR #4 head), squashed on `master` as `Version 1.10.2` | the ledger and analyses from BAS-X30 on; PLAN A.8 | fired: PR #4's ref (`refs/pull/4/head`, the same `63ac175`) holds the whole line, so the citations resolve without the tag | removed 2026-10-25 |
| `archive/nnue-local` | `1431a89` | The 2026-07 NNUE bring-up: scalar `.mnn` loader (`src/nnue.*`), `UseNNUE`/`EvalFile`, `test_nnue` with `tests/data/test_h16.mnn` vectors | PLAN F.2.3 (F.4 until 2026-11-11) | F.2.3 closes, or F.0 chooses a network format other than `.mnn` | keep |
| `archive/nnue-origin` | `4fa11bf` | An ancestor of `archive/nnue-local`; nothing it reaches is lost without it | none | fired: redundant | removed 2026-09-28 |
| `archive/backup` | `ae4af1a` | The pre-squash line of 2026-08-05 → 08-11, including the withdrawn Colosseum adoption (`3cbf90b`); an ancestor of `oracle/hybrid` | none | fired: redundant | removed 2026-09-28 |
| `archive/arm_fix` | `67a987b` | One commit aligning the TT to 64-byte lines for Apple (`src/tt.h`) | BAS-P07, which states the mechanism and why it was rejected | fired: hypothesis rejected, the row carries the recipe | removed 2026-09-28 |

Reviewed again 2026-10-07 (A.8.3), when `dev` began reaching `master` by
merge commits: none of the three kept tags is reachable from `dev`, so the
change retires none. On 2026-10-08 releases returned to squash merges, and
every release's development line got an `archive/dev-X.Y.Z` tag. On
2026-10-09 the long-lived `dev` branch was retired: it lands once as a final
squash PR, and from then on each piece of work has its own branch and PR, so
no further `archive/dev-*` tag is made. `archive/dev-1.10.0` stays, since
the records cite it and no other ref holds its line. On 2026-10-25
`archive/dev-1.10.2` was removed as redundant with PR #4's ref, and `dev`
itself no longer exists locally or on `origin`. The other kept tags were
re-checked the same day and stay. `oracle/hybrid` is still the oracle that
PLAN rule 1 and PROCESS name, and `hybrid/` is not on `master`.
`oracle/hybrid-diag`'s condition has not fired. `archive/nnue-local` waits
on F.2.3 (F.4 until 2026-11-11).

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
| 6.6.a–6.6.e, 6.6.h | C.2.7 | endgame instruments and gate integrity (C.7.1 until 2026-11-11) |
| 6.6.f | A.3.1 | harness provenance refusals, shared guards |
| 6.6.g | A.4.2 | build option and ISA combination matrix |
| 6.7.a–6.7.d | C.7.2 | occurrence, classification, closure, ranking |
| 6.8.a | C.7.4 | the BAS-E53 only-move defect |
| 6.8.b–6.8.e | C.7.5 | Group B families by measured kind |
| 6.9.a–6.9.b | C.7.7 | occurrence-tiered endgame gates |
| 6.10.a–6.10.d | C.7.6 | lower-yield remainder |
| 6.10.e, 6.11.a–6.11.d | C.7.7 | gate and closure |
| 7.0 | A.4.1 | toolchain refresh and freeze |
| 7.1, 7.7, 7.9–7.12 | B.7.1 | board contract audit, a defect hunt (D.3.1 until 2026-11-03, B.15.1 until 2026-11-05) |
| 7.2–7.8 | B.7.2–B.7.4 | board speed and representation, in the board review (the speed pass until 2026-11-03, B.15.2–B.15.4 until 2026-11-05) |
| 8.0.a–8.0.d | C.0 | HCE surface audit |
| 8.0.e | C.1 and each C cluster | categorical tests and the architecture freeze |
| 8.1.a–8.1.h | C.2.1 | fit-tooling contract, Texel handbook |
| 8.2.a–8.2.e | C.2.2 | corpus design |
| 8.3.a–8.3.c | C.2.3 | corpus A |
| 8.4.a–8.4.f, 8.5.a–8.5.d | C.2.4 | matched label arms; whole-game tablebase adjudication |
| 8.6.a–8.6.c | C.2.5 | initialization control |
| 8.7.a–8.7.e, 8.8.a–8.8.d | C.2.8 | matched fits and the label-contract gate (C.2.6 until 2026-11-11) |
| 8.9.a–8.9.e | C.8 | refit cycles |
| 8.10.a–8.10.e | C.9 | nonlinear HCE SPSA |
| 8.11.a–8.11.d | C.11 | checkpoint and freeze |
| 8.12.a–8.12.d | C.12 | evaluation throughput, bit-exact |
| 9.0.a–9.0.c | B.0 | search audit, interactions, oracle profile |
| 9.1.a–9.1.b | B.3 | extension authority |
| 9.2.a–9.2.c | B.4 (research card), C.10 | SEE and move-ordering value scale |
| 9.3 | B.0, B.2 | TT and caches, inside cluster 1 |
| 9.4 | B.9.1, D.3 | lifecycle, protocol, tablebases |
| 9.5 | A.3, A.5, B.8 | harness and diagnostics audit |
| 9.6.a–9.6.e | B.2.3, B.6 and each cluster's SPSA | search SPSA |
| 9.7.a–9.7.d | D.1 | time management |
| 9.8.a–9.8.c | B.8, B.11.1 | cleanup and checkpoint |
| 9.9 | E.1 | attribution checkpoint |
| 9.10.a, 9.10.c, 9.10.d | E.3.2 | classical release |
| 9.10.b | D.2 | SMP quality |
| 9.11.a–9.11.d | G.2 | optional universal binary |
| 10.0–10.4 | F.0, F.1 | NNUE runway |
| 11.0–11.5 | F.2, F.5, F.6, F.9 | baseline NNUE and its release (F.3 and F.4 are F.2's sub-steps since 2026-11-11) |
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

Renumbered within Phase A on 2026-10-24 (maintainer decision): the Rarog
audit's leaves, first numbered in `4fa68b4` (PR #7), now run in order as one
sequence, and Phase A is marked `(EXTENDED)`. A reference dated before this
renumbering, commit messages on `head-reads-1102` up to `80865c0` included,
uses the first numbering. A.10.1–A.10.6 and A.12 kept their numbers.

| Was | Now | Note |
|---|---|---|
| A.11 | A.9 | harness and measurement instruments, now first |
| A.11.1 | A.9.1 | Colosseum re-pinned to 0.4.0 |
| A.11.5 | A.9.2 | direction reads against 1.10.2 (BAS-S27) |
| A.11.4 | A.9.3 | measurement rules from the audit |
| A.11.2 | A.9.4 | NPS read on pooled builds |
| A.11.3 | A.9.5 | profiler with inline attribution |
| A.13 | A.10.7 | dev builds name their commit |
| A.9 | A.11 | correctness repairs, after the instruments and delivery |
| A.9.1–A.9.9 | A.11.1–A.11.9 | the same repairs in the same order |

Renumbered within Phase A on 2026-10-27 (maintainer decision): the KR-K
conversion repair found by fail-soft quiescence's research runs first, as
A.11.10. A reference dated before this renumbering, the code audit
(`analysis/code_audit_2026-10-27.md`) and the commit messages of branch
`a11-10-fail-soft-quiescence` included, uses the first numbering.

| Was | Now | Note |
|---|---|---|
| A.11.10 | A.11.11 | fail-soft quiescence, after the KR-K repair |

Moved into Phase B on 2026-11-03 (maintainer decision): the board contract
audit becomes the first leaf of the board review and rebuild (B.15), which
Triumviratus's registration as the third donor opened. References dated
before this, the Rarog audit (`analysis/rarog_audit_2026-10-24.md`), the
code audit and the B.2.0 architecture review included, use D.3.1.

| Was | Now | Note |
|---|---|---|
| D.3.1 | B.15.1 | board contract audit, now B.15's safety net and defect hunt |

Renumbered in Phases B to D on 2026-11-05 (maintainer decision;
`analysis/rarog_sweep_2026-11-04.md`): the open steps run in ascending
order, each measurement sits in the step that reads it, and no step depends
on a later one. B.7, B.9, B.11 and C.5 to C.7 now name different work. A
reference dated before this, the Rarog, code and Triumviratus audits, the
B.3 and B.16 records and their commits included, uses the earlier numbering.

| Was | Now | Note |
|---|---|---|
| B.15, B.15.1–B.15.4 | B.7, B.7.1–B.7.4 | board review and rebuild, after B.6 |
| B.4.0 | B.4.3 | quiescence research, never a leaf; B.4 split into leaves the same day, its two repairs first |
| B.7 | B.8 | the speed pass joins the cleanup step |
| B.7.1 | B.8.3 | allocation guard, no longer `(ANY TIME)` |
| B.7.2 | B.8.4, B.8.5 | large pages split out, before the speed pass |
| D.3.2 | B.9.1, B.9.2 | lifecycle and protocol, before the release |
| B.9 | B.11.1 | search programme checkpoint |
| B.11 | B.11.2 | interim release 1.11.0 |
| C.5, C.5.1–C.5.7 | C.7, C.7.1–C.7.7 | endgame cluster, after pawns and material |
| C.6 | C.5 | pawns and passers |
| C.7 | C.6 | material, imbalance and pieces; its phase and tempo went to C.3 |
| D.4 | D.3 | tablebase policy |
| D.3 | retired | its one open leaf moved to B.9 |

Moved between leaves the same day: B.9's floor-legality patch and B.15.1's
SEE repairs (A-2, A-3) to B.4's opening (B.4.1, B.4.2); D.3.2's clean child frame (TR-4)
and deep-line stack test (A-25) to B.5; D.4's A-57 and A-58 and D.1's
A-50 and TT-4 tests to B.9.1; C.12's lazy-threshold audit and C.5.2's E7
read to C.10; D.1's thread-instability time factor to D.2; B.5's 4T S6
read to D.2.

Renumbered on 2026-11-11 (at the maintainer's request for a roadmap that
runs in ascending order with every correctness fix before the release;
`analysis/rarog_import_2026-11-11.md` §4). Each retired number below is
retired for good and none is reused, so a reference dated before this
resolves here without ambiguity.

| Was | Now | Note |
|---|---|---|
| B.6.1 | B.6.6 | donor-form reconciliation, after the audit's repairs |
| B.6.2 | B.6.7 | joint search SPSA, last in B.6 |
| B.9.2 | B.9.5 | repairs and the repair gate, now for B.9.1, B.9.3 and B.9.4 |
| B.11.2 | B.11.4 | interim release 1.11.0, after the disclosure |
| C.7.1 | C.2.7 | endgame instruments and gate integrity, before the first evaluation gate |
| C.2.6 | C.2.8 | matched fits and the label-contract gate |
| F.4 | F.2.3 | scalar integration, before any training |
| F.3 | F.2.4 | trainer hardening and the learning curve; its baseline nets went to F.6 |
| F.2.1 | F.2.5 | data generation at scale, sized by F.2.4's curve |
| F.2.2 | F.2.6 | classical refit on search-visited data |

New the same day: B.6.3 (search-path perft and kernel pins), B.6.4 (engine
audit), B.6.5 (audit repairs), B.9.3 (hard verdicts against Syzygy), B.9.4
(deferred defects), B.11.3 (originality and licence disclosure) and H.4
(throughput on the network head). Moved between leaves: B.7.1's
search-board perft (TB-1) to B.6.3; C.7.1's drawn-overclaim instrument and
Syzygy vetoes (C-133, C-142) are built in B.9.3 and read again in C.2.7;
G.2's printed-score calibration (TA-2) to F.6.

Renumbered in B.7 on 2026-11-14 (maintainer decision, after
`analysis/b7_frontier_review_2026-11-14.md`): the board's instruments come
before its rebuild, and a second round of candidates before its gate. The
two retired numbers are retired for good. A reference dated before this,
B.7.1's and B.7.2's packets included, uses the earlier numbering.

| Was | Now | Note |
|---|---|---|
| B.7.3 | B.7.6 | rebuild behind the board contract, on the amended handoff |
| B.7.4 | B.7.9 | board speed read and gate |

New the same day: B.7.5 (search replay and engine screen), B.7.7 (second
round of board candidates) and B.7.8 (their rebuild). Withdrawn the same
day: Chess960 (handoff item C7, TB-9, G.2's bullet).

Renumbered in Phase C on 2026-11-25 (C.2.10's order, from games;
`analysis/c210_family_order_2026-11-24.md` §11): the four family clusters
run in the order of the Elo each explains. C.3, C.5 and C.6 now name
different work, and C.4 keeps its number. A reference dated before this,
C.0's, C.2.8's and C.2.10's records and packets and BAS-E62 included,
uses the earlier numbering.

| Was | Now | Note |
|---|---|---|
| C.5 | C.3 | pawns and passers, first; it opens C.3.1 to C.3.5 |
| C.6 | C.5 | material, imbalance and pieces |
| C.3, C.3.1–C.3.5 | C.6 | mobility, space and threats, last; its sub-steps were never started and open again when reached |

Retired and renumbered in Phase C on 2026-11-25 (the rebuild;
`analysis/c31_rebuild_2026-11-25.md` §13, signed by the maintainer): the
king-safety, material and mobility clusters are built inside C.3, the
evaluation rebuilt whole, and the leaves after them move up. C.4 to
C.10 now name different work. A reference dated before this, the
rebuild packet and BAS-M25 included, uses the earlier numbering.

| Was | Now | Note |
|---|---|---|
| C.4 | none | king safety, phase and tempo: retired, built in C.3 |
| C.5 | none | material, imbalance and pieces: retired, built in C.3 |
| C.6 | none | mobility, space and threats: retired, built in C.3 |
| C.7, C.7.2–C.7.7 | C.4, C.4.2–C.4.7 | endgame handling and winnability |
| C.8, C.8.1–C.8.2 | C.5, C.5.1–C.5.2 | residual and refit cycles |
| C.9 | C.6 | nonlinear HCE SPSA; its surface is the rebuild's structure constants |
| C.10, C.10.1–C.10.2 | C.7, C.7.1–C.7.2 | joint re-fit on the new evaluation |
| C.11 | C.8 | freeze the classical evaluation |
| C.12 | C.9 | evaluation throughput, bit-exact |
| C.13, C.13.1–C.13.2 | C.10, C.10.1–C.10.2 | interim release |

The retired entries as they stood, with their inputs. Each input is a
defect or a prior of a form the rebuild displaces, and is re-read only
if C.3.6 returns that form.

- **C.4, retired 2026-11-25** `[I2]` King safety, phase and tempo cluster
  Folded into C.3 on 2026-11-25 (maintainer): built there as part of the
  whole rebuild. This entry keeps the family's inputs and is retired at
  C.3.1's handoff.
  King safety cluster in the donor's shape: attacker units and weights, safe
  and unsafe checks by piece, weak ring squares, flank attack and defence,
  shelter and storm with the castling alternative, the queenless reduction and
  the nonlinear danger map; refit; gate.
  Opens as sub-steps (research, implementation, review, refit, gate), as
  C.3 has, when C.3 closes; the same holds for C.5 and C.6.
  It was C.3 until 2026-11-18, when the mobility and space cluster moved
  ahead of it.
  The phase and tempo join it (from C.7, 2026-11-05): E2's phase and E5's
  tempo change together with E1's eg king danger, never one at a time, and
  before the other families, so that each later refit reads the final
  phase. Since 2026-11-18 the mobility and space cluster precedes it:
  the donor's space is gated by material, not by the phase, and the one
  refit the phase then costs that cluster is run anyway (C.0's packet
  §7.3).
  Since 2026-11-25 the pawns and passers cluster precedes it instead
  (C.2.10), at the same cost: its values are refitted with the phase
  here. The mobility and space cluster is last.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Definitions: king blockers of either colour (C-13, `eval.cpp:1278-1280`);
    the donor's safe and unsafe checks, check presence, the weak ring and the
    ring and zone shapes (C-14); storm without blocked pawns and the king-file
    sign (C-15).
  - Priors: the danger map carries the king residual and shelter and storm
    about 20%, while flank attack and defence, king-pawn distance and the
    pawnless flank were nil in Rarog (C-111); Rarog's open E.4: the remaining
    lead is the index's structure, since refitting its coefficients from donor
    seeds failed (RAR-E73); zero-fitted inputs are a resolution limit (C-17);
    the clamp is live (C-113); a donor fixture (C-112).
  - Guards: shelter in the pawn cache needs the king square and castling in
    the key (C-43); a won KRP-KR static guard before any endgame danger term
    (C-44).
  Code audit inputs (2026-10-27 second pass, §6.3):
  - E1: king danger is middlegame-only (`eval.cpp:1594`, `mg -=
    safety_table[units]`; the donor also subtracts `kingDanger/16` from
    eg) and lacks the donor's inputs: the mobility difference, the
    knight-defender relief, `kingAttacksCount`, pawns attacking the
    ring, the shelter coupling and the attacker-count × weight product.
    Bishop and queen attacker units are fitted 0 (C-111). Double
    counting: the open-file count enters the danger units
    (`:1576-1582`) on the files `shelter_missing_*` already penalises
    (`:1615-1627`), and the king-ring bonuses fire beside the attack
    units where the donor's `else if` keeps them exclusive. Measure
    first: the donor-direction residual screen on these inputs by phase
    and king danger's firing share at phase ≤ 12; the lead is index
    structure (RAR-E73).
  - E2's phase: pieces count N = B = 1, R = 2, Q = 4 of 24 against the
    donor's npm between its endgame and middlegame limits; changed with
    the eg king danger here, never alone, and with tempo (E5: tempo is
    mg-only, 22, so it tapers to 0 in pawn endings and is scaled with
    the score, where the donor adds a constant 28 after scaling; the
    lazy path omits it, C-6).
  Rarog sweep input (2026-11-04, `analysis/rarog_sweep_2026-11-04.md`;
  Rarog E.4's first step, EVAL-3):
  - In the donor's king-danger inputs bishops see through queens, rooks
    through queens and own rooks, and a pinned piece's attacks are cut to
    the king line (`sf9587-ks` `evaluate.cpp`); Basilisk's maps are plain
    occupancy (`eval.cpp`, `all_pieces()`). C-115's x-ray nil was
    mobility's alone. Rarog's C.3 fixture came from a donor patched to
    plain maps, so a donor fixture here is taken from the unpatched donor.
  C.0 inputs (2026-11-18, its packet §7.2, §7.3 and §8.2): the donor's
  king row adds 15.2 at phase 10–17 and 11.1 at 18–24; the endgame
  bands' static gap is not one family's; tempo exists only in
  `9587eeeb`'s evaluation; Basilisk's rule-50 curve stays. The mobility
  and space cluster goes ahead of this one (maintainer, 2026-11-18).
  C.2.10 inputs (2026-11-25, BAS-O14; the packet's §10 and §11):
  - The king's meter reads +43.1 ± 13.2. It is a lower bound: the phase
    and tempo are outside every bit.
  - It is the second cluster in games, 51.8 ± 14.2 behind pawns and
    passers and 80.9 ± 13.9 ahead of the piece terms. It is also the
    largest donor gain in the screen on corpus A (BAS-E62).
- **C.5, retired 2026-11-25** `[I2]` Material, imbalance and pieces cluster
  Folded into C.3 on 2026-11-25 (maintainer): built there as part of the
  whole rebuild. This entry keeps the family's inputs and is retired at
  C.3.1's handoff.
  Imbalance in the donor's quadratic form, bishop pair and bishop-pawn
  colour, outposts (including BAS-E07's absent `BadOutpost`), long diagonal,
  rook files, trapped rook, queen weakness and king-protector distance;
  refit; gate.
  Outposts, `reachable_outpost`, mobility and `queen_infiltration` were
  fitted around seven rank-mirrored piece-square tables (B.10). Their trace
  reads them for compensation the flip left behind.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - Outposts on ranks 5-8 against the donor's 4-6 (C-10); dead and collinear
    imbalance slots (C-20); queen infiltration and the trapped bishop (C-48);
    no trapped rook (C-46); no WeakQueen (C-116); screen first (C-118).
  Code audit inputs (2026-10-27 second pass, §6.3):
  - E2's phase and E5's tempo are the king-safety cluster's (C.4; C.3
    from 2026-11-05 to 2026-11-18), changed with its eg king danger.
  - Bishop terms: the bishop outpost is absent although comments at
    `eval.cpp:1783` and `eval_params.h:222` say it exists; bishop x-ray
    on pawns is absent; bishop pawns is a linear count without the
    donor's factor for unprotected and blocked central pawns; the minor
    behind a pawn counts own pawns only. Screen first (C-118).
  - Doubled penalises every pawn on a file with two or more, supported
    or not (`:965`; the donor only the rear pawn when unsupported);
    Basilisk-only terms with no donor prior: pawn majority, pawn
    islands, closed file, seventh rank, connected rooks, the trapped
    bishop.
  C.2.8 inputs (2026-11-23, the fit registration's Amendment 1, its §6.1
  and §6.7; inference until C.2.8.3's run 2 reads it):
  - A PST's level against the imbalance's piece-times-pawn products is
    weakly identified, and the split moves with the fit's surface. With
    every entry free the eg rook table rose 180 cp (BAS-O12). With 242
    slots held it rose 52 and the products carried the rest (C.2.8.3's
    run 1). The endgame piece values stayed within 8% of each other.
  - So an imbalance term's fitted value is read with the table levels of
    the same fit, never alone (C-118's screen).
  It was C.6 until 2026-11-25, and C.7 until 2026-11-05, when the phase
  moved to the king-safety cluster (C.3 then, C.4 since 2026-11-18).
  C.2.10 inputs (2026-11-25, BAS-O14; the packet's §10 to §12):
  - The piece terms' meter reads −37.7 ± 13.1: the deficit is not in
    them. It is a lower bound for the cluster, since material and the
    imbalance are outside every bit.
  - With all seven families removed, Basilisk's remainder (material,
    tables, imbalance, the bishop pair, tempo and endgame knowledge)
    leads the donor's by 16.6 [5.8, 27.4]. That read is flagged: its
    arm ran 17% faster than at mask 0. So the remainder is about level
    (inference).
  - C-110 closes the donor's queen terms without a port (WeakQueen,
    C-116): the pieces' meter lies below zero and their gain includes
    zero in every band on both samples. The rook terms sit on the edge
    ([0.0, 2.0] at phase 10–17 on gate games) and stay with C-118's
    screen.
  - The imbalance keeps a static case: its donor gain is 4.5 [2.9, 5.8]
    over all gate-game rows and 9.9 [5.7, 14.2] at phase 10–17
    (BAS-E62's control).
- **C.6, retired 2026-11-25** `[I2]` Mobility, space and threats cluster
  Folded into C.3 on 2026-11-25 (maintainer): built there as part of the
  whole rebuild. This entry keeps the family's inputs and is retired at
  C.3.1's handoff.
  The last family since 2026-11-25: C.2.10's games moved it from first
  (maintainer, 2026-11-18, on C.0's evidence).
  The donor's space, which Basilisk does
  not have in any working form; the mobility area with x-rays and the pin
  line; and of the threat terms (weak enemies, hanging pieces, restricted
  squares, pawn-push threats, and BAS-E07's absent `KnightOnQueen` and
  `SliderOnQueen`) those their own screen supports. Shared attack maps
  from C.1; refit; gate.
  It was C.3 from 2026-11-18 to 2026-11-25, with sub-steps C.3.1 to
  C.3.5 that were never started, and C.4, "Threats and mobility", before
  that. Space had no owner (C-119); C.0 placed it here. It opens as
  sub-steps when reached, as C.3 has.
  Rarog audit inputs (2026-10-24, `analysis/rarog_audit_2026-10-24.md`; IDs
  are its items):
  - SliderOnQueen is missing; screen KnightOnQueen before building it (nil in
    Rarog); ungated forms were worse than none (C-114); mobility priors
    (C-115); the dead `hang_pen` loop (C-37; A.16 removed it).
  - Code audit 2026-10-27 (§6.3): the safe-pawn threat is a flat 58/25
    whatever the target and counts pawns that are themselves unsafe
    (`eval.cpp:1403-1426`), overlapping hanging, restricted and
    weak-queen on the same target.
  C.0 inputs (2026-11-18, its packet §7.2 and §8.2): C.0 places space
  here (C-119, E8). At phase 18–24, where 68 to 69% of the same-search
  deficit is realised, the donor's space adds 47.0 and its mobility 29.6
  to Basilisk's whole score; the threat rows add 4.4 or less, so a
  threat term is built only if its own screen excludes zero.
  C.2.8 inputs (2026-11-23, the fit registration's Amendment 1, its §6.2
  and §6.7):
  - Queen mobility 1 is what a queen on its home square counts before its
    own pawns move: the mobility area leaves out the king and the low
    pawns (`src/eval_attacks.h`). Corpus A starts at book exits, so it
    holds 10 rows at count 0 and 224 to 278 at count 1, mostly trapped
    queens.
  - A free fit puts both entries on the tuner's −200 clamp (BAS-O12,
    every arm). That is about 230 cp for the first pawn move from the
    starting position (inference, not measured).
  - C.2.8's fit holds count 0 at the seed, and the tuner's monotone chain
    then stops count 1 at it. This cluster's research settles the two
    entries: a prior, a merged low-count entry, or rows that reach them.
  C.2.10 inputs (2026-11-24, BAS-E62's control):
  - On C.0's gate-game rows (UHO, `3+0.03`) the donor's space adds 40.4
    [32.7, 49.1] to the refitted head's score at phase 18–24, and its
    mobility 13.4 [9.7, 17.4]. Before the refit they read 50.8 and 29.6.
  - On corpus A's validation rows the same reads are 3.4 and 2.1, and
    5.7 and 7.1 on the header before the refit. Corpus A is self-play at
    8,000 nodes from a phase-balanced start pool.
  - So a space term fitted on corpus A alone may be fitted to nothing.
    This cluster's research decides which rows its refit reads before
    any value is fitted, and separates the two candidate causes: the
    starts and the search depth.
  C.2.10 inputs (2026-11-25, BAS-O14; the packet's §10 to §12):
  - The cluster's meter reads −52.0 ± 13.5: mobility alone −46.0 ± 13.3,
    space −2.1 ± 12.5 and threats −9.9 ± 12.9. The deficit is not here.
    Basilisk's mobility is worth more to it than the donor's is to the
    donor, and level does not explain it: both terms carry the same
    (slope on own material 0.019 and 0.021).
  - Space leads the static screen on gate games (40.4 at phase 18–24)
    and is worth the same to both evaluations in games. So the screen's
    donor gain does not rank what games return, and C.0's prior,
    mobility and space first, is refuted.
  - Its research starts from whether anything is ported at all. It
    answers C.0's three competing readings first (its packet §8.2):
    whether space still reads once the tables are refitted in their
    right orientation, whether the gain is the unbalanced book's, and
    whether mobility's gain is the area's values or the x-rays. It ends
    in a frozen handoff, or a `NO_CHANGE`, for each term.

Moved in Phase C on 2026-11-26 (at the maintainer's request for a speed
pass and a clean-up of the remote before the release): C.9 opens as
C.9.1 to C.9.3 and keeps its number, and the remote clean-up, C.10.3 and
C.10.4, runs before the cut. No number is reused.

| Was | Now | Note |
|---|---|---|
| C.9 | C.9, C.9.1–C.9.3 | throughput, widened from the evaluation to the whole engine |
| C.10.2 | C.10.5 | the 1.12.0 cut |

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
- **2026-11-18 — PLAN Phase B CLOSED: the search programme and 1.11.0.**
  The search was rebuilt in modern Stockfish's design (`sf_19`) in four
  clusters, each fitted and gated, then audited, tuned jointly, cleaned of
  its arms and switches, and made faster. B.11.1's checkpoint attributed
  +441.4 Elo of gates since 1.10.2 against +422 measured on the four
  targets (`analysis/b111_checkpoint_2026-11-18.md`). PLAN keeps the
  phase's summary; the steps' text is in
  `docs/archive/PLAN-closed-2026-10-30.md` (B.0–B.2, B.10) and
  `docs/archive/PLAN-closed-2026-11-18.md` (the rest). `bench 13` was
  14,978,465 for 1.10.0 to 1.10.2, then 11,099,210 at B.2.4, 8,437,148 at
  B.10, 9,260,429 at A.11.1, 8,753,339 at A.11.2, 7,233,823 at A.11.6,
  7,384,101 at A.11.7, 7,857,713 at A.11.9, 8,913,394 at A.11.10,
  16,092,165 at A.11.11, 19,108,490 at A.13.1, 19,470,325 at A.13.2,
  16,615,757 at B.12, 11,880,564 at B.14, 5,369,042 at B.3.5, 5,361,058 at
  B.3.6, 6,090,091 at B.4.2, 4,144,790 at B.4.7, 3,783,135 at B.5.5,
  4,278,307 at B.6.7, 4,247,965 at B.7.1, 4,688,537 at B.8.7 and
  4,777,412 since B.9.5. Older still: 12,709,666 before 6.5.a, and
  12,568,898 before the 15.0.a SEE repair. The legacy kernel, deleted at
  B.8.1, read 23,696,970 at A.11.9.
- **2026-11-25 — No other project's file is tracked.** At the maintainer's
  request the remote holds Basilisk and nothing else. The 54 Triumviratus
  documents under `docs/reference/` moved to the ignored local store beside
  that engine's source (`tools/results/reference/`, verified by the
  `.local-sha256` manifests), and the copy of the UCI specification left the
  root. Rarog is pinned by commit:
  `docs/reference/rarog/` (273 files, Rarog `015bccae`) and
  `docs/reference/rarog-2026-10-06/` (19 files, `dcf15c51`) left the tree
  after every file was compared with Rarog's blob at its commit and found
  identical. `docs/reference/README.md` says where the two commits are and
  how an older reference resolves. The plan's protection against Rarog's
  deletion, the reason for the copy on 2026-09-28, is now the three bundles
  in ignored storage.

## Release record — 1.11.0

| Item | Value |
|---|---|
| Version | **1.11.0** (2026-11-18) |
| Scope | PLAN Phase B: the search rebuilt in Stockfish's design (Clusters 1–4, B.2–B.5), the joint search SPSA (B.6), the board rebuild (B.7), the cleanup and speed pass (B.8), the robustness, verdict and protocol repairs (B.9), and the evaluation repairs of B.10, B.12 and B.14. Releases now ship as archives with the GPL and Fathom licences (B.11.3) |
| Bench-13 fingerprint | **4,777,412** (1.10.2: 14,978,465) |
| Strength | BAS-S67, SPRT `[0,3]` over 1.10.2 at `3+0.03` 1T: H1 at 536 games, +406.9 ± 39.2. BAS-M19's direction checks, 1,000 games each: `10+0.1` +371.3 [+348.8, +396.4]; 4T sibling-free +386.6 [+363.7, +412.1]. On the four targets' scale the head rates 3416.3 at 1T (BAS-M16); it is ninth of fourteen in the HCE frontier field, 202 below Stockfish 16 HCE (BAS-M18) |
| Qualification | PR #30's CI (the test matrix, the sanitizers, GCC PGO and bench agreement) and its release candidate run: all five assets and every x86 tier at 4,777,412, tier selection under SDE, `release_check.py check v1.11.0`; then the tag run's rebuild |

The release revision is the `Version 1.11.0` commit on `master`
(`f1e31f4`, squashed from PR #30), tagged **`v1.11.0`**. Its assets are
five archives, each holding the binary, the GPL, Fathom's licence, the
README and the changelog, with `SHA256SUMS`.

## Release record — 1.10.2

| Item | Value |
|---|---|
| Version | **1.10.2** (2026-10-08) |
| Scope | PLAN A.8: the Rarog 2.4.0→2.5.0 parity work. Tablebase band and bound-correct in-search probes (A.8.14, BAS-S19); the won-endgame time sink (A.8.17, BAS-S20); MultiPV (A.8.9); command-line commands and `help` (A.8.10); the tablebase extension's start rule (A.8.11); UCI `info` conformance and the threaded `bestmove` line (A.8.7–A.8.8); the C++23 output pass (A.8.12) |
| Bench-13 fingerprint | **14,978,465**, unchanged from 1.10.1 |
| Strength | With tables, BAS-S19 +31.7 ± 13.4 against the pre-A.8.14 head (an early-stopped SPRT); it is the release figure. The planned verification match against 1.10.1 (BAS-M13) was withdrawn unplayed by maintainer decision. Without tables, BAS-S20 −0.7 ± 4.5 (the only change to play without tables) |
| Qualification | Release and ASan/UBSan CTest 17/17; `release_check.py check v1.10.2`; NPS 4.189M pooled (BAS-P15, +1.5% over 1.10.1); the ponder-on, Threads 4 smoke run against the published 1.10.1 (BAS-C15): stopped by the maintainer at 121 of 200 games and accepted on his judgement, with 1.10.2 at zero failures; the two "process exited" games on 1.10.1's side coincide with the stop |

The release revision is the `Version 1.10.2` commit on `master`: the PR was
merged with a merge commit and then squashed on `master` by maintainer
decision. Its tree is the PR head's (`63ac175`, kept as `archive/dev-1.10.2`)
plus the documents recording the return to squash merges. It is tagged
**`v1.10.2`**, the first release through the tag-driven flow (PROCESS
*Release*); its assets are the nine `release.yml` builds.

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
