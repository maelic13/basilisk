# Rarog audit, 2026-10-24: what Rarog found and fixed since 2026-08-11, checked on Basilisk

- State / class: delivered; the source of PLAN A.9–A.12 and of the audit
  inputs added to existing leaves on 2026-10-24, `R2`
- Owner / date: agent, 2026-10-24, at maintainer request
- Decision taken: maintainer, 2026-10-24. Findings with an owner leaf extend
  that leaf; findings without one become new Phase A leaves, correctness
  repairs first, each due before the leaf it names. Each step is reviewed
  with the maintainer when reached; its resolution may be a fix or a skip.

## Scope and method

- **Rarog sources**, read from the live repository at the maintainer's
  direction (an exception to `agents/research.md`'s snapshot rule):
  - every ledger entry dated since 2026-08-11 (about 225), the four
    correctness entries RAR-C01–C04;
  - about 90 `analysis/` packets since 2026-08-11 and the long-form
    `ledger_records_2026-09-14.md`;
  - CHANGELOG 2.3.2 to `[Unreleased]`, HISTORY, PLAN (open phases D–G
    included) and the closed-phase archives;
  - tests, CI and release workflows, xtask, PROCESS and AGENTS;
  - GitHub issues and PRs, and the tournament records.
- **Window.** Basilisk's 2026-10-06 parity review (A.8) read only Rarog's
  2.4.0 → 2.5.0 changelog and commits. Rarog's Phase-4 line (2026-08-11
  to 09-09, released as 2.4.0) and everything after 2026-10-06 had never been
  checked against Basilisk.
- **Basilisk** at `ed3132f` (branch `b10-pst-orientation`, B.2 and B.10
  closed, `bench 13` 8,437,148).
- **Method.** Four read-only auditors by subsystem, each checking every
  Rarog item against Basilisk's source and records with file:line. 415
  items. About 120 are already fixed in Basilisk or not applicable.
- **Verification.** Claims marked ✔ below were re-read in code by the
  coordinator. Everything else rests on the auditors' code reading; no
  engine was built or run for the audit.
- **Correction to an auditor:** A-52 says Basilisk's quiescence is "already
  fail-soft". It is fail-soft on a fail-high (`return s`, `return
  stand_pat`) but returns `alpha` on a fail-low
  (`search_kernel_core.cpp`, the delta-prune and final returns), so a
  root fail-high echoes beta (B-19).

## Where the items are

| Area | IDs | Full text |
|---|---|---|
| Correctness, protocol, time, SMP, tablebases | A-1 … A-69 | [appendix A](rarog_audit_2026-10-24_A.md) |
| Search programme | B-1 … B-122 | below |
| Evaluation, endgames, data and fitting | C-1 … C-149 | below |
| Speed, build, measurement, tournaments, process, release | D-1 … D-75 | [appendix D](rarog_audit_2026-10-24_D.md) |

## Rarog's own open plan, read for items Basilisk lacks

- **F.2.1, classical refit on search-visited data** (added 2026-10-22):
  - The same-search meter's remaining 99 Elo is not in the evaluation
    families, which read at static parity with the donor on played
    positions (RAR-E70).
  - It lies in how the evaluation scores the positions the search visits.
  - The fix: refit the whole frozen HCE surface on F.2's search-visited
    data with deep labels, then gate it as an evaluation change.
  - → Basilisk F.2.2 (new).
- **E.4**, the king-danger index rebuilt in the donor's structure. Refitting
  the coordinates from donor seeds lost held-out loss (RAR-E73); the lead is
  structure. → C.3 input.
- **E.3.0**, originality disclosure and shipped notices. → E.3 input.
- **E.2**, contempt as the registered lever in the target gauntlet
  (`c137_contempt_card`). → E.2 input.
- **D.1.x**, the clock pieces: horizon contraction, mate stop, first-iteration
  guard. D.1.6 is the cascade cluster; its gate read H0 and it returned to
  research. → D.1 input.

## Search programme items (B)

Status as checked on Basilisk. Owner is the Basilisk leaf the item feeds.

### Defects Rarog found in its own clusters, checked on Basilisk

- **B-1 ✔ PRESENT** Rule-50 damping is baked into the static eval the TT
  stores and reuses at any clock.
  - Basilisk: `eval.cpp:1210-1211, 1985-1986`; stored at
    `search_kernel_core.cpp:491-495, 1350-1352`, qsearch `:187/208/307/320`;
    reused `:488-489, :184-185`.
  - Rarog: RAR-S73 departure 3 (`db92790`) moved the damping into the
    search after the TT read.
  - Owner: A.9.1.
- **B-2 HAS (by measured choice)** The quiet skip drops a late direct check
  (Manta's mate-in-one class).
  - Basilisk: `CoreLmpExemptChecks` 0 (+13.6, BAS-S21).
  - Owner: B.4.0 should name the exposure; Rarog's late-mate fixture is a
    documented known-failing case.
- **B-3 PARTIAL** A KBNK anchor at one 60k budget is chaotic.
  - Rarog: mated at 3 of 5 budgets on its accepted engine, so it moved to a
    five-budget majority.
  - Basilisk: `tests/test_endgames.cpp:59`. Owner: B.3.0, C.5.1.
- **B-4 ✔ PRESENT** A node whose only legal move is excluded returns mate or
  stalemate, so singular extensions chain.
  - Basilisk: `search_kernel_core.cpp:1297-1298`; Stockfish returns `alpha`.
  - Rarog: bench never finished, seldepth 127, one 39-minute spin; fix
    `a1`, counter `singular_lone_move`.
  - Owner: A.9.2.
- **B-5 PLANNED, risk unflagged** IIR was hiding the extension explosion
  in Rarog; turning IIR off or to Stockfish's form exploded. Basilisk's
  IIR is at its rail 8 and B.3 plans an IIR-off arm. Owner: B.3 (after A.9.2,
  under a watchdog).
- **B-6 PARTIAL** A ply bound on extensions does not bound depth (one
  position at depth 17: 229M nodes). Owner: B.3.
- **B-7 MISSING** Per-line extension budget: Rarog adopted it (bench
  8.10M → 7.48M; 229M → 1.33M on the worst position). Owner: B.3.
- **B-8 N/A today** A −3 negative extension at depth 4 leaves the TT move at
  depth 0. Owner: B.3, if the singular floor is lowered.
- **B-9, B-10, B-11 HAS** MultiPV defects (IIR and excluded root TT move,
  stopped-depth fill, helpers' vote) do not occur in Basilisk.
- **B-12 ✔ PRESENT** The best-scored move is stored at fail-low nodes.
  - Stockfish stores a move only when alpha rises.
  - Changes the "no TT move" populations of IIR `:729`, razoring `:614`,
    RFP `:633`, LMR `:1084`, `:1110`, `:1184`.
  - Basilisk: `:1219-1223`. Owner: B.3.
- **B-13 UNVERIFIED** Dead PVS guard `max(newDepth,1)` on the PV re-search
  (`:1194-1197`); count with a diag counter. Owner: B.8.
- **B-14 ✔ PRESENT** Correction training refuses decisive results.
  - Basilisk: `:1327` (`!is_decisive(best_score)`).
  - Rarog's refusal read −15.1 ± 9.7; the donors admit them.
  - Owner: B.3 categorical.
- **B-15 PRESENT, low stakes** Correction training refused at
  singular-excluded nodes (`:1326`); Rarog +2.3 ± 9.5, kept training.
  Owner: B.3 categorical list.
- **B-16 HAS** Qsearch stores keep the PV bit (Stockfish's form). B.4 must not
  adopt Reckless's subtree marking.
- **B-17 ✔ PRESENT** The qsearch SEE threshold's lower clamp −800 is dead:
  `alpha ≥ stand_pat` there (`:279` after `:228`). Owner: B.4 / B.8.
- **B-18 PARTIAL** Qsearch late-capture count rule nearly dead (`:287` acts
  only for SEE in [−200, −50)); Rarog replaced it with count 3 and
  exemptions. Owner: B.4.
- **B-19 ✔ PRESENT** Fail-hard quiescence makes root fail-highs echo beta.
  - Rarog: 93% of bench root fail-highs equal beta; this drives aspiration
    cascades.
  - Basilisk returns: `:225`, `:321`. Owner: B.4, with the KBNK and KQK
    fixtures.
- **B-20 PRESENT (as Stockfish)** The interior estimate reads qsearch bounds
  with no depth guard (`:511-517`); count depth-0 refinements. Owner: B.4.
- **B-21 PLANNED** Qsearch fail-high interpolation moves the estimate that
  RFP and razoring were fitted on (Rarog: depth-14 tree +41%). Put RFP,
  razoring and NMP-base coordinates on B.4's surface. Owner: B.4.
- **B-22 MISSING** Quiet-evasion pruning in check. Rarog's zero-game rule set
  it off, but its game read adopted it at +7.8 ± 9.4. Basilisk `:152-180`.
  Owner: B.4.
- **B-23 HAS** No SEE exemption for checking captures; keep it.
- **B-24** A censored refinement counter: read the total delta mass, not the
  mean. Owner: B.4.
- **B-25 MISSING** Qsearch margins are literals: futility 150, delta
  `QUEEN+200`, SEE 200, late prune 6/−50 (`:221, 271, 279, 287`). Rarog's
  futility margin was the last coordinate with gradient. Owner: B.4, then
  C.10.
- **B-26 PLANNED** SEE values against evaluation material (unit census).
  Owner: B.4.0.
- **B-27 PARTIAL** Qsearch TT cutoff with decisive scores at PV qnodes
  (`:142-149` cuts everywhere). Owner: B.4.
- **B-28 PLANNED** Re-measure the qsearch's size on the current head before
  sizing B.4. Owner: B.4.0.
- **B-29 PLANNED** First-ply quiet checks: WAC gains did not predict games.
  Owner: B.4.0.

### Proof searches and extensions (B.3)

- **B-30** NMP entry margin alone did nothing in Rarog; the donor shape took
  conversion from 22% to 70%. Owner: B.3.
- **B-31 PRESENT** NMP verification at its own node is inert (0.34%
  failures); the donors disable null move over a region. Basilisk
  `:672-678`. Owner: B.3.
- **B-32** The verification region cannot fire on bench (no node at depth
  ≥ 16): use a `go depth 20` wire check and keep the verify depth off the
  tune surface. Owner: B.3.
- **B-33** Cut-node-only NMP read +3.1 at STC and **−10.4 ± 12.0 at
  10+0.1** in Rarog. Make the population a categorical with an LTC read.
  Owner: B.3.
- **B-34 N/A today** A dead margin term behind an entry veto. Audit every
  margin term against its entry guard when porting. Owner: B.3.
- **B-35 UNVERIFIED** The RFP/NMP crossover band per depth. Owner: B.3.0.
- **B-36 HAS** Null-move return safety (`:669-684`).
- **B-37 PARTIAL** A same-ply verification search overwrites the node's
  stack record (`cutoff_cnt`, `tt_pv`). Owner: B.3.
- **B-38 HAS** ProbCut's SEE-gap filter (`:697`).
- **B-39 MISSING** ProbCut move cap and candidate ordering (`:693-720`
  iterates unsorted, uncapped). Owner: B.3.
- **B-40 N/A** ProbCut verification at depth 4 (Basilisk enters at ≥ 5).
- **B-41** ProbCut verification direction was a false premise in Rarog's
  packet. Owner: B.3.0.
- **B-42 PLANNED** ProbCut TT-value guard with a depth condition. Owner: B.3.
- **B-43 PLANNED** TT-served ProbCut: small prior (+6.4 alone, declined).
  Owner: B.3.
- **B-44 HAS** ProbCut's store feeds singular admission; name it in the
  interaction map. Owner: B.3.
- **B-45 ✔ PRESENT** The singular margin's range excludes the donor's value.
  - Basilisk: `SingularBetaMult` 4, range 1..6 per ply
    (`search_params.h:124`). Stockfish's margin, converted, is about
    0.26/ply.
  - Rarog's same misfit read +24.2 ± 9.4 once in range.
  - Owner: B.3 (finer units).
- **B-46 MISSING** Singular TT-depth margin 2 instead of 3 (+12.9 ± 9.4 in
  Rarog). Owner: B.3 categorical.
- **B-47** Singular depth floor `5 + ttPv`: no direction. Owner: B.3, prior
  only.
- **B-48 HAS** Rarog's multi-cut rule (the donor's fails KBNK). Keep it
  unless a multi-budget KBNK clears the donor's.
- **B-49 PLANNED** Graded double and triple extensions: in Rarog, triples
  were 21% of extensions. Measure the one-, two- and three-ply shares,
  including the check-extension composition. Owner: B.3.
- **B-50 PLANNED** Negative extension split (−3 against the donor's −2).
  Owner: B.3.
- **B-51** A new LMR term lands on constants fitted without it, so its LMR
  bases join the tune surface. Owner: B.3.
- **B-52 N/A** Reckless-only devices.
- **B-53 PLANNED** The in-check extension: Basilisk's removal read −10.17
  (BAS-S08). Owner: B.3.
- **B-54 UNVERIFIED** In-check share of interior nodes against the oracle.
  Owner: B.3.0.
- **B-55 PLANNED** IIR minimum depth: game-read the form and widen the range
  off its rail. Owner: B.3.
- **B-56 HAS** Hindsight and `cutoffCnt` producers.

### LMR, move loop and histories (closed in B.2)

- **B-57 … B-62 HAS:** LMR one-ply floor; move counter before pruning;
  Stockfish's statistic composite; the reduction field reset; one `do_move`
  seam; donor clamps kept with converted inputs.
- **B-63 HAS (found independently)** The shallower re-search is off at
  margins ≤ 1, and Rarog's donor value read −12.9. Owner: B.8 (delete).
- **B-64 PRESENT** A continuous coordinate with a switch boundary
  (`CoreResearchShallower` range 0..12). Floor the range or read it as a
  switch. Owner: B.6 / C.10.
- **B-65 … B-68 HAS:** full-depth branch; history update policy;
  TT-cutoff bonus; razoring guards. Lesson from B-67: confirm which code
  path is live while the legacy kernel exists.
- **B-69 PARTIAL** Correction magnitude feeds the margins and must be refit
  with the eval. Owner: C.10.

### Root, aspiration, iterative deepening (B.5)

- **B-70 PARTIAL** Aspiration cost counted in nodes, not events (Rarog: 61%
  of root-window nodes are in failed windows). Owner: B.5.0.
- **B-71, B-72** One-retry aspiration read −18.3 although every zero-game
  read favoured it; the Elo surface falls steeply toward wider windows.
  Decide by games only. Owner: B.5.
- **B-73 PRESENT** Window width is coupled to LMR (`lmr_reduction`
  `delta·577/root_delta_`). Owner: B.5.
- **B-74 HAS** Re-centring on the failing score (BAS-D17).
- **B-75 PLANNED** A score-scaled first window removes won-ending cascades.
  Rarog's four-part cluster read H0 (−7.4 ± 7.5) and was re-scoped to
  bare-king trees. Owner: B.5 (with B-19).
- **B-76** A fail-high depth reduction needs its clock-coupled half. Owner:
  B.5 / D.1.
- **B-77 HAS** Interrupted iterations discarded.
- **B-78 PARTIAL** No stop after a found mate beyond mate in 1. Owner: D.1.
- **B-79 / A-48** Basilisk's fail-high clock stop (A.8.17), as Rarog
  answered on 2026-10-24 (its RAR-R22 addendum):
  - On Rarog's replay the same-move condition would fire on 24% of
    searches, removing 11% of the time spent, so Rarog rejected it.
  - Basilisk's own firing rate has never been measured. BAS-S20
    (−0.7 ± 4.5) bounds its cost.
  - Owner: B.5 (the read).
- **B-80, B-81 PRESENT** Deep-iteration line growth. Negative LMR reductions
  (up to `new_depth+2`, `+is_pv`) escape any line budget. Owner: B.5 (with
  B-7).
- **B-82 UNVERIFIED** Correction history drives won-ending tree growth
  (Rarog: 19M → 4M nodes with the term zeroed). Owner: B.5 / D.1 probe.
- **B-83 PLANNED** Optimism read −5.6 in Rarog; it must live at read time
  and needs A.9.1 first. Owner: B.5.
- **B-84 PLANNED** Draw jitter read −1.2 in Rarog; measure draw blindness
  first. Owner: B.5.
- **B-85 PARTIAL** MultiPV lines 2+ search the full window. Owner: B.5.
- **B-86 HAS** Tablebase band in the TT.
- **B-87 UNVERIFIED** The band is reachable without tables through fail-hard
  qsearch stores. Owner: B.4.

### Cluster method, screens and instruments

- **B-88, B-91, B-92, B-96, B-99 HAS:** the paired run governs; the
  categorical rule; `[0,3]`; ablation; time to depth.
- **B-89, B-90 PRESENT risk** Zero-game reads repeatedly mispredicted the
  game sign. Fixed-node WAC and canaries favour less pruning; oracle
  agreement favours more. BAS-D11, D13, D16 and D17 were decided on WAC
  alone and need game reads inside B.3–B.5.
- **B-93** The curvature sweep stays a checklist item, not a forecast. Owner:
  B.3 / B.4.
- **B-94 UNVERIFIED** Calibrate the canary rule under neutral perturbations
  (Rarog: 0 of 12 lost). Owner: B.3.0.
- **B-95** Bench chaos: single steps moved bench 0.3–71%.
- **B-97 MISSING** Deep-iteration cost screen: the geometric mean of
  cumulative node ratios over depths 16–19 must stay ≤ 2–3×. Owner: B.3.
- **B-98 PARTIAL** Silence-watchdog driver. Owner: B.3.
- **B-100 PARTIAL** Movement is not Elo: a tail with no full-step mover was
  worth +4.43. Owner: B.6.
- **B-101 MISSING** SPSA journal z-read (net travel / √Σstep² against a
  null). Owner: B.6.
- **B-102 MISSING** Choose the tune surface by firing-rate change (≥ 10%)
  or a scale move (> 5%). Owner: B.6 / C.10.
- **B-103 PARTIAL** Rail read and range widening before a tune. Owner: B.3
  (IIR), B.6.
- **B-104 UNVERIFIED** A perturbation clipped against a coordinate floor.
  Owner: B.6.
- **B-105 PARTIAL** Relevance check: every `Core*` coordinate one step
  each way must move bench. Owner: B.6, B.8.
- **B-106 PLANNED** The joint tune buys little after fitted clusters
  (Rarog: +4.5 for 120k games). Owner: B.6.
- **B-107 UNVERIFIED** Search and eval never share a tune (`LazyMargin`).
  Owner: C.10.
- **B-108 HAS** `scale_ratio.py`; apply it after B.10 and C. Owner: C.10.
- **B-109 PRESENT** Ablation bit 6 removes the check extension as well as
  singular (`:401`, `:961`). Owner: B.3, B.9.
- **B-110** Ablation limits. Owner: B.9.
- **B-111 ✔ PRESENT** Frontier nodes are counted twice.
  - `negamax` calls `record_node()` (`:335`), then quiescence counts again
    (`:117`).
  - Inflates `nodes`, NPS and `go nodes` budgets.
  - `(int(state_.nodes) & 14)` in the move loop reads the count, so search
    behaviour depends on it too.
  - Owner: A.9.6.
- **B-112 UNVERIFIED** Counter units and levels. Owner: B.3.0.
- **B-113 HAS** Tool parsing (bound lines, CR).
- **B-114** CI covers each cluster's switch arm. Owner: B.3 / B.4.
- **B-115 HAS** Cluster-and-fit approach.

### Speed and cleanup (B.7, B.8)

- **B-116 PARTIAL** Rarog's speed pass: quiet early exit +8.85%, in-place
  list +6.93%, TT prefetch before make +1.69%. Owner: B.7.2. Detail: D-5,
  D-6, D-10, D-11.
- **B-117** Allocation guard: planned (B.7.1). Rule-50 mate test: HAS.
- **B-118, B-119 HAS** History capacity; start-up tables and ponder races.
- **B-120 PLANNED** Cleanup pitfalls: bare-name search, truncated
  inventories, re-audit after deletion. Owner: B.8.
- **B-121 N/A** Shared parameters moving the legacy fingerprint. Owner: B.4,
  check shared constants.
- **B-122 PARTIAL** Seldepth excludes qsearch plies (updated only in the move
  loop). Owner: D.3.2.

### Basilisk-only observations made during the checks

- Quiescence in check never stores to the TT and never prunes
  (`:152-180`). Owner: B.4.
- PLAN B.5's text says Basilisk has no MultiPV; A.8.9 implemented it.
  Owner: B.5 (text).
- `MAX_QSEARCH_PLY` 10 returns the stand pat; Stockfish has no cap.
  Owner: B.4.

## Evaluation, endgames, data and fitting items (C)

### Basilisk defects confirmed in code

- **C-1 ✔ PRESENT** The tuner's delta scale omits scalers the evaluator
  applies.
  - `tools/texel/tuner.cpp:423-438` mirrors only OCB, KNNK and rule 50.
  - Missing: `rook_ending_scale` (KRPKR, KRPPKRP), the pawnless-minor
    4/64 or 14/64 scale, and the KBNK/KXK/KPK override rows (scale 1).
  - OCB is applied unconditionally; the engine applies it only `if
    (!scaled)`.
  - Test: dump the engine's scale beside `linear_delta_scale` per row.
  - Owner: C.2.1.
- **C-2 ✔ PRESENT** `--verify` cannot fail.
  - `rest = score − reconstruct` at load (`tuner.cpp:592-593`); the verify
    recomputes `reconstruct + rest` (`:729-731`).
  - Test: delete one `TR_MG`; verify still passes.
  - Owner: C.2.1.
- **C-3 ✔ PRESENT** The pawnless "ahead by at most a minor" scaler breaks
  ties toward White (`eval.cpp:800`). Owner: A.9.7.
- **C-4 PRESENT** OCB scale.
  - `ocb_draw_scale` = 32+4·pawns capped at 48, so neutral from 4 pawns
    (`eval.h:100-103`).
  - No passer term; fires with any other material (`eval.cpp:835`).
  - Rarog's repair: `1 + 2·pawns + 10·passers`, +12.7 ± 9.8 nElo on
    endgame starts.
  - Owner: C.5.3.
- **C-5 PRESENT** The fit uses the full evaluation; the engine plays the
  lazy one.
  - `eval.cpp:1199-1200` `#ifndef TEXEL_TRACE`.
  - Rarog: 16% of fitting rows are above the gate; fitting the played
    function was half of +31.0.
  - Owner: C.2.1.
- **C-6 PRESENT** Terms after the lazy return never fire above 700 cp.
  - The lazy return is at `eval.cpp:1206-1214`.
  - After it: passer proximity and path terms `:1691-1728`, the mop-up
    `:1895-1933`, tempo `:1935-1940`, winnable.
  - The served score jumps by the tempo at |cheap| = 700.
  - Owner: C.1 / C.6 / C.12.
- **C-7 = B-1.**
- **C-8 PRESENT** The extractor's de-dup key assumes the clock does not
  affect the evaluation (`tools/texel/extract.py:80-82`). Owner: C.2.2.
- **C-9 PRESENT** KBNK/KXK override jumps reward shedding material and
  under-promoting.
  - KBNP-K → KBN-K jumps to a base of 15,600.
  - KBP-K =N/=B scores above =Q.
  - BAS-E49 saw KBP-K "via knight promotion".
  - Rarog's rule: a replacement score stays above every one-capture or
    one-promotion neighbour.
  - Owner: A.9.9.
- **C-10 PRESENT** Knight outposts on ranks 5–8 (donor 4–6) with current pawn
  attacks (`eval.cpp:1181`, `:1739-1742`); Rarog's fix was nil. Owner:
  C.7, low.
- **C-11 PRESENT** A rear doubled passer scores as a full passer
  (`:921`); nil in Rarog. Owner: C.6, low.
- **C-12 PRESENT** "Opposed" tests the whole file; backward pawns
  (`:973`, `:951-958`). Owner: C.6.
- **C-13 PRESENT** King blockers are own-colour only (`:1278-1280`), which
  feeds king safety and the mobility area. Owner: C.3 / C.4.
- **C-14 PRESENT** King-danger conditioning narrower than the donor's:
  - safe set without `weak & attackedBy2`;
  - no unsafe-check term;
  - popcount checks;
  - weak ring;
  - ring/zone shapes.

  Basilisk: `:1240-1243, 1494, 1506-1517`. Owner: C.3.
- **C-15 PRESENT** Storm counts blocked pawns; the king-file storm weight
  is a bonus (`:1614-1625`; `eval_params.h:334`). Owner: C.3.
- **C-16 PARTIAL** Supported and phalanx share one table; no "opposed"
  (`:965-969`). Owner: C.6.
- **C-17 PRESENT** Zero-fitted king-danger inputs are a resolution limit,
  not evidence (`eval_params.h:303, 318-325`; 25-bucket table). Owner:
  C.3 / C.9.
- **C-18 PRESENT** The king-safety pass uses one global step and refits K
  (`tuner.cpp:1327, 1349, 1385-1414`). Owner: C.2.1.
- **C-19 PRESENT** Gauge freedoms are unanchored.
  - `--tune all` frees material and every PST, and `phase911.ps1:263` runs
    it.
  - `ks_noqueen_num/den` are both free.
  - Owner: C.2.1 / C.2.6.
- **C-20 PRESENT** Imbalance slots are dead or collinear
  (`eval.cpp:1093-1102, 1138-1141, 1745-1748`). Owner: C.2.1 manifest,
  C.7.
- **C-21 PRESENT** Under `--tune texel` shelter and storm are fitted by no
  stage (`tuner.cpp:369-371`). Owner: C.2.1.
- **C-22 PRESENT (bites after C.2.6)** B.10's orientation test requires
  PeSTO proximity on every table; a correct loose refit drifts away. Limit it
  to the seeds. Owner: C.2.1 / C.2.6.
- **C-23 PARTIAL** B.10's screen had one arm and refit K per arm.
  - Rarog's form: two arms (current and all-flipped), `--l2 0`, K fixed,
    2,000 epochs, both converging.
  - Basilisk has no fixed-K option (`tuner.cpp:1101`).
  - Owner: C.0 / C.2.6.
- **C-24 PRESENT (stale text)** PLAN C.5.7 says KRPPKRP has no truth, but
  `D:/chess/tablebases/syzygy7` holds `KRPPvKRP`. Owner: C.5.1 / C.5.7.
- **C-25 PRESENT** The conversion floors use one cold search at one budget
  (`tests/test_endgames.cpp:71-80`), against Rarog's persistent searcher
  over five budgets. Owner: C.5.1.
- **C-26 PRESENT** The datagen default book is SuperGM
  (`tools/datagen.ps1:120`). Owner: C.2.2.
- **C-27 PRESENT** The book sampler fills equal phase reservoirs and keeps
  its own phase definition (`tools/texel/sample_fens.py`). Owner: C.2.2.
- **C-28 PRESENT** The train/holdout split hashes start plus moves
  (`extract.py:85-90`). Owner: C.2.1.
- **C-29 PRESENT** Any game with a Result header is labelled
  (`extract.py:168, 315`). Owner: C.2.1.
- **C-30 PRESENT** Rows carry no game id or ply, so no clustered standard
  errors are possible (Rarog design effect 1.29–1.65). Owner: C.2.1 /
  C.2.2.
- **C-31 PRESENT** Colosseum 0.2.0 does not count the setup position for
  threefold (fixed in 0.3.0). Owner: A.11.1.
- **C-32 PARTIAL** The datagen wrapper checks only the exit code, warns on
  book wrap and accepts an unregistered binary. Owner: C.2.3.

### Rarog defects Basilisk avoids or has fixed

- **C-33 … C-41 HAS:**
  - connected rooks with three rooks;
  - lazy-path imbalance order;
  - the tuner's rule-50 shared definition;
  - mop-up gating;
  - restricted squares;
  - `attacked2` and rook-behind-passer;
  - the KBNK corner pull (the KXK ratio is still open, C-39 → C.5.6);
  - truth-instrument aborts and drift;
  - symmetric-family double counting.
- **C-42 N/A today** A runtime LazyMargin must clear caches. Owner: C.10 /
  C.12 if it becomes an option.
- **C-43, C-44 N/A today** Shelter in the pawn cache needs king and
  castling in the key. A won KRP-KR guard before any endgame danger term.
  Owner: C.3.
- **C-45 N/A** The unstoppable-passer off-by-one. If one is added, use
  steps+1/steps. Owner: C.6.
- **C-46 MISSING** Trapped rook (donor form nil in Rarog). Owner: C.7, low.
- **C-47 PARTIAL** Initiative in the donor's form, but every `win_*` weight
  is 0 and it is untraced (`:1942-1977`). Owner: C.5.3 / C.9.
- **C-48 PRESENT** Queen infiltration and trapped bishop definitions
  (`:1800`, `:1880-1892`). Owner: C.7.

### Fitting contract and method (C.2)

- **C-49 PRESENT** K is refitted per run (`tuner.cpp:1101`, `:1349`). Fix it
  once on validation; report a fixed-K column. Owner: C.2.1.
- **C-50 MISSING** No frozen test split: the holdout picks K and the epoch
  and also reports. Owner: C.2.1.
- **C-51 MISSING** Residuals read on unused train rows. Owner: C.2.1.
- **C-52 PARTIAL** Manifest classing every coordinate free, fixed or
  excluded, with support-based freezing. Owner: C.2.1.
- **C-53 PLANNED** Explicit prior vectors between stages (no `--initial`).
  Owner: C.2.1.
- **C-54 PARTIAL** Stage schedule: king safety, linear, king safety again,
  polish. Owner: C.2.1.
- **C-55 MISSING** Never hand-revert one block of a jointly fitted vector.
  Owner: C.2.1 handbook.
- **C-56 UNVERIFIED** Removing adjudication multiplied natural mates 57×
  and inflated the king table: tree +12.3%, KBNK floor broken. Owner:
  C.2.3 / C.2.6.
- **C-57 PLANNED** One causal axis per corpus gate. Owner: C.2.6.
- **C-58** Symmetric bracket when neither arm is the status quo. Owner: C.0.
- **C-59 HAS** Label arms decided by games.
- **C-60 PLANNED** Syzygy relabel (+6.73 on an adjudicated corpus; may
  differ on natural termination). Owner: C.2.4.
- **C-61 HAS** Resign-threshold KR-K labels (adjudication off); 9.4% of
  KR-K rows are still ½ from failures to mate. Owner: C.2.4.
- **C-62 PARTIAL** Label quality follows search strength (20.33% → 6.86%
  clean wins not won). Re-audit on the B.9 head. Owner: C.2.3.
- **C-63 PARTIAL** Relabelling small endgames cannot repair the larger rows
  of the same failed games. Owner: C.2.4.
- **C-64 PLANNED** Tablebase adjudication of whole games is a different
  change. Owner: C.2.4.
- **C-65 PLANNED** Neutral start fits 2.38% worse in Rarog. Owner: C.2.5.
- **C-66 PLANNED** A free PST refit moves tables 11–57 cp per entry; use it
  to size C.2.6's anchor. Owner: C.2.6.
- **C-67 HAS** Pure WDL targets; blended targets lost in Rarog. Owner: C.2.4
  / C.10.
- **C-68 HAS / PLANNED** Distillation lost; anchored refit paid; on-policy
  refresh null. Owner: C.8.
- **C-69 PARTIAL** Re-dump scores after any refit. Owner: C.2.1 / C.8.
- **C-70 UNVERIFIED** Tests whose preconditions depend on fitted weights:
  fix the fixture in its own commit. Owner: C.2.1.
- **C-71 PARTIAL** Fit-driver invariants:
  - exact reconstruction;
  - byte-exact restore;
  - bench equality;
  - clean tree;
  - parse errors gated at 0.

  Owner: C.2.1.
- **C-72 PARTIAL** A refit grows the tree, so a frozen-search gate reads the
  eval gain minus the search loss. Owner: C.0.
- **C-73 UNVERIFIED** A dense coefficient matrix may be infeasible at 3.5M
  rows (`tuner.cpp:51`). Owner: C.2.1.
- **C-74 HAS** Extractor settings, deliberate.
- **C-75 PARTIAL** Size the corpus by book yield per start phase. Owner:
  C.2.2.
- **C-76 PARTIAL** Pin the realised opening sequence; a move-for-move pilot
  for a harness port. Owner: C.2.3.
- **C-77 PRESENT** The quiet filter keeps transient material; re-read static
  interactions on search-settled rows. Owner: C.0 / C.2.2.
- **C-78 UNVERIFIED** Remaining game length explains part of the phase
  slope. Owner: C.0 / C.2.4.

### Screens and instruments for C.0 and each cluster

- **C-79 … C-88 MISSING/PARTIAL:**
  - donor-direction residual screen on one `9587eeeb` build;
  - a fit's own residuals cannot see outside its features;
  - one donor build for trace and total;
  - the donor's all-zero endgame rows;
  - symmetric ensemble gain with the ≤ 6-men split;
  - re-read the screen after each unit;
  - rebuild donor sub-terms offline and verify them exactly;
  - wire controls;
  - hinge-spline comparator;
  - boosted-tree interaction read.

  Owner: C.0.
- **C-89 PRESENT gap** Dump both the played and the full score
  (`--dump-eval` gives the full score only). Owner: C.0 / C.12.
- **C-90 PARTIAL** Dominant-contributor dump; split `rest` by source.
  Owner: C.1.
- **C-91 PARTIAL** Static score against the engine's own re-search. Owner:
  C.0.
- **C-92 UNVERIFIED** Calibration by phase. Owner: C.0 / C.8.
- **C-93 PARTIAL** Inventory every term by firing share, including the
  mop-up, OCB, `LAZY_MARGIN`, `SCALE_FLOOR` and rule-50. Owner: C.0 / C.9.
- **C-94 N/A yet** Family-bucket span bug. Owner: C.0.
- **C-95 MISSING** A loss at equal time is per node: run an equal-node
  companion read. Owner: C.0.
- **C-96 PARTIAL** Fixed-depth tree read on about 72 game positions (bench
  13 under-reads). Owner: C.0.
- **C-97 MISSING** Magnitude contract (±20 cp shift; share above the lazy
  gate). Owner: C.0 / C.2.6.
- **C-98 MISSING** Margin block and gate 2 after each evaluation unit.
  `LAZY_MARGIN` 700 was never refit, including after B.10's 31.7 cp shift.
  Owner: C.0 / C.10.
- **C-99 MISSING** NPS stop and per-unit cost; a `10+0.1` direction read per
  accepted unit. Owner: C.0.
- **C-100** The "frozen search consumes static gain" prior was refuted
  (+31, +34). Owner: C.0.
- **C-101 PARTIAL** Static and game results disagree in both directions; a
  skip rule names its statistic. Owner: C.0 / C.9 / C.10.
- **C-102 PARTIAL** Re-measure the same-search meter on the current head.
  Owner: C.0.
- **C-103 PARTIAL** Corpus-wide colour-symmetry sweep with promoted material
  and the lazy gate open. Owner: A.9.7.
- **C-104 PARTIAL** Behaviour-neutral restructure: corpus score hash and the
  three running-sum reads (`eval.cpp:1206, 1897, 1975`). Owner: C.1.
- **C-105 MISSING** Cache purity: a long-lived evaluator equals a cold one.
  Owner: C.1 / C.12.
- **C-106** A material-signature table. Owner: C.5.2 / C.12.
- **C-107 … C-110 MISSING:**
  - classify each repair as correctness or definitional;
  - compare against the incumbent and size priors from inactivity;
  - a post-refit falsifier per new term;
  - a structural stop rule.

  Owner: C.0.

### Mechanism priors per cluster

- **C-111** King safety (C.3):
  - The danger map carries 92% of the king residual; shelter and storm
    about 20%.
  - Flank attack and defence, king-pawn distance and pawnless flank are
    nil: do not build them.
  - Unit result +32.7. The lead left is structure (Rarog E.4).
  - Fitted queen attacker weight was the highest (Basilisk's is 0).
- **C-112 MISSING** Donor king-danger fixture (500 positions); donor tables
  kept as test fixtures. Owner: C.3.
- **C-113 UNVERIFIED** King-dominant large scores convert worse; the index
  clamp is a live coordinate. Owner: C.3 / C.9.
- **C-114 PARTIAL** Threats (C.4):
  - Carriers: safe-square slider-on-queen and king threat.
  - KnightOnQueen, donor hanging, restricted, safe-pawn and by-minor forms
    are nil.
  - Ungated forms are worse than none.
  - Basilisk lacks SliderOnQueen. Screen KnightOnQueen before building it.
- **C-115 HAS** Mobility area; pin line and x-rays nil.
- **C-116 MISSING** WeakQueen; information moved there when an x-ray term was
  removed. Owner: C.7.
- **C-117 PARTIAL** Passers (C.6): our king's distance to the block square
  carries, weak unopposed carries, definitions nil. Basilisk has the old
  symmetric form.
- **C-118** Material, imbalance and pieces: no signal in Rarog. Run the
  screen before C.7 builds its list. Owner: C.0.
- **C-119 PARTIAL** Space: +0.137 at 4.0 SE in the opening band; no C cluster
  owns space (`space_mg = 0`). Owner: C.0.
- **C-120 PARTIAL** Zero-weight terms removed behaviour-neutrally (23 groups).
  Owner: C.0 → C.1 / C.12.
- **C-121 N/A** Contempt, draw jitter and optimism rejected in Rarog; contempt
  is the registered E.2 lever there. Owner: E.2.
- **C-122 PLANNED** Nonlinear sweep before SPSA (C.9 closed NO_CHANGE in
  Rarog).
- **C-123 PLANNED** Refit cycles diminish (+11.0, +6.7); size from earlier
  cycles' games. Owner: C.8.
- **C-124 PLANNED** Phase C budget prior: about +105 from units, +42 from
  speed. The remainder is in search-visited positions. Owner: C.0; F.2.2.

### Endgame programme (C.5)

- **C-125 PLANNED** OCB below seven men: lower factors cost KBPP-KB; free
  passer forms rejected; boundary jumps. Owner: C.5.3.
- **C-126 PARTIAL** Floor every scale (rook scale floored, OCB not). Owner:
  C.5.3.
- **C-127 PLANNED** Generic scale, complexity and rule-50 carry signal only
  at ≤ 6 men plus pure OCB. Owner: C.5.2.
- **C-128 UNVERIFIED** Rule-50 damping can push the drive weights below the
  razoring margin. Owner: C.5.3.
- **C-129 PARTIAL** KR-KN and KR-KB: the flat 4/64 may under-claim wins;
  KRP-KB is unscaled. Owner: C.5.5 / C.5.6.
- **C-130 MISSING** KNN-KP: additive drive plus a blockade term (+11 at 200k);
  a replacement score bribes. Owner: C.5.6.
- **C-131 MISSING** One-capture-neighbour unit test with a wire check.
  Owner: A.9.9.
- **C-132 PARTIAL** Missing value functions: KQ-KP fortress, KBP-KN, KRKP,
  KQKR, KNNKP. Owner: C.5.2 / C.5.5 / C.5.6.
- **C-133 MISSING** Drawn-overclaim instrument from Syzygy draws (Basilisk's
  uses game results). Owner: C.5.1.
- **C-134 MISSING** Game-level ending audit from gate PGNs. Owner: C.5.2.
- **C-135, C-136 PLANNED** Recogniser rule (local defect plus ≥ 1 Elo);
  classify families by the instrument. Owner: C.5.2.
- **C-137 PLANNED** Bracket 60k/200k/600k. Owner: C.5.1.
- **C-138 PARTIAL** Per-root concentration; tree against board occurrence.
  Owner: C.5.2.
- **C-139 HAS / PLANNED** Floors with per-game SE and a ratchet. Owner: C.5.7.
- **C-140 PARTIAL** Promotion-closure paired accounting. Owner: C.5.2.
- **C-141 PARTIAL** Measurement layers stamped on reports. Owner: C.5.1.
- **C-142 MISSING** Syzygy-verified vetoes in the unit suite. Owner: C.5.1.
- **C-143 PLANNED** Closing checkpoint discipline. Owner: C.5.7.
- **C-144 UNVERIFIED** A KBN-K drive that does not speed up with nodes;
  a KBNK distance-to-mate table option. Owner: C.5.6.
- **C-145 N/A** Aspiration-bound scores in replay tools. Owner: C.5.1.
- **C-146 PARTIAL** Conversion meter compared within one pool only. Owner:
  C.11.
- **C-147 UNVERIFIED** Gates without tablebases expose static small-endgame
  scores. Owner: C.5.7.
- **C-148 HAS** Seven-man cohorts from games.
- **C-149 PARTIAL** Originality: shared Stockfish rules disclosed; the C.2.6
  PST refit removes the verbatim-PeSTO fingerprint. Owner: C.2.6 / E.3.

## Tournaments (appendix D, Part 3)

- **No faults.** No Basilisk fault, crash, illegal move or time loss in any
  record since 2026-09-20, on x86, Windows on ARM or the M4.
- **Results:**
  - RAR-P44 (Surface, 3+0.03): 1.10.2 is 18th of 20, −395 behind Rarog
    2.6.0.
  - RAR-P43 (M4): Rarog 2.6.0 +344 ± 40 over 1.10.2.
  - 1.10.2 is not resolvably stronger than 1.9.3 on either ARM host.
- **One clock anomaly:** 1.9.3's clock reached −22 ms in one M4 game, hidden
  by a 2,000 ms margin.
- **Never measured:** Rarog's match-host pool holds Basilisk at 1.10.0's
  2993; 1.10.2 and the current head have never been played there.
- **Owner:** B.9 (pool read of the head), E.2.
