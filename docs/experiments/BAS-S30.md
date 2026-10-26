# BAS-S30

**BAS-S30 — A.11.6: each node counted once, with the root kept out of the tie rule**

- **Registration.** Registered 2026-10-26, before any game; maintainer-run;
  correctness repair from the Rarog audit (PLAN A.11.6; audit B-111), with
  the precondition repair it exposed. One gate for both, at the
  maintainer's decision: the second cannot pass CTest without the first.
- **Defects.**
  - `negamax` counted a node at entry and then handed the same node to
    quiescence, at depth zero or for razoring, which counted it again.
    `nodes`, NPS, bench and `go nodes` budgets were inflated, and the
    equal-score tie rule reads the count.
  - That tie rule (a move equal to the best replaces it when
    `(nodes & 14) == 0`) also ran at the root. A later root move is
    searched with a null window at alpha, and the fail-hard quiescence
    search returns alpha exactly, so an equal score is usually a fail-low
    bound: the root played a refuted move. With the count fixed, the
    free-queen test played Qa4 (worth −6) at Qxd5's `cp 1018`.
- **Candidate.** `basilisk-a116-fix-pext-pgo.exe`, revision `07a357f`: the
  root out of the tie rule (`0eadf03`) and each node counted once
  (`07a357f`). It also carries A.11.3 to A.11.5 (an empty own clock's
  budget, upper-case move text, fatal-error reports), which cannot act
  under these conditions: every game has an increment, the harness sends
  lower-case moves, and no thread fails.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 7,233,823; SHA-256
    `EA3114242C434FA70E13E6113021C7F550DB6E0FC681137B98C765010D1DA09B`.
- **Baseline.** `basilisk-a116-base-pext-pgo.exe`, revision `2c6c4fc`
  (`master`, A.11.2 merged), built the same way with the same compiler;
  `bench 13` 8,753,339; SHA-256
  `DE7412DE3B362ED62967AF7949EE4704084B96F4CC74D80BCEF965E2BF3F159D`.
- **Mechanism.** A root move is played only when it proved itself; a
  shallow iteration no longer hands its successor a refuted best move,
  whose ordering and TT contents every later iteration starts from. The
  node count is the number of positions searched, so budgets and the tie
  rule's sampling read the tree as it is.
- **Interactions.**
  - The tie rule still runs below the root (`ply + 2 >= root_depth_`); the
    count change reshuffles which ties it takes, pseudo-randomly.
  - The stop poll runs every 2,048 counted nodes, which now span more
    positions, so it polls slightly less often in time: about every
    0.74 ms at the non-PGO bench's 2.76M nodes per second. A `go nodes`
    budget buys more search by the share of nodes that were counted
    twice; that share is not measured.
  - Correction history, history and the TT read the trees the shallow
    iterations leave; the root fix changes those trees on 39 of the 40
    bench positions.
  - The legacy kernel has neither defect's fix; it has no tie rule.
- **Competing explanations for a neutral reading:** the root rule acted
  only at iterations 1 and 2, whose best move the deeper iterations
  usually replace, and the reshuffled ties are noise by design.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +3 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 80%, lower than
  BAS-S29's because the bench moves by 17% and every position's tree
  changes.
- **Most likely failure mode:** a reading too small for the bracket.
- **Verdict rule:**
  - H1: both repairs stay, and the fingerprint record updates to
    `bench 13` 7,233,823;
  - the cap without a decision: both stay as correctness fixes with no
    measured harm, as BAS-S20, BAS-S28 and BAS-S29; report the LLR and the
    estimate, and the maintainer confirms;
  - H0: revert both engine commits and return the leaf to `RESEARCH` with
    the result; the root defect then needs its own research, since
    reverting it reinstates a refuted-move choice.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the conversion floors and the WAC floor included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 30001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs**, as BAS-S19, BAS-S26, BAS-S28 and BAS-S29.
- Launcher `tools/run_a116_gate.ps1`; results in `tools/results/a116-gate/`.
- **Dry run accepted 2026-10-26**
  (`tools/results/colosseum_sprt_a116-gate_20261026_094000.*`, dry run
  `bcc9e35a…327f`, manifest `a539140d…bda8`).

#### QUALIFICATION (before the gate)

- Release PEXT without PGO:
  - the root fix alone (`0eadf03`): `bench 13` 8,101,423 (from
    8,753,339); all 40 positions change their node count, 39 their score;
  - both (`07a357f`): `bench 13` 7,233,823, deterministic on a repeat;
    the legacy OFF arm 15,887,213, unchanged; CTest ON 24/24, OFF 21/21.
- Tests: the free-queen test plays Qxd5 at depths 1–3 (it played Qa4
  with the count fixed and the root rule in place); a depth-1 search of a
  single quiet move counts 2 nodes, where the A.11.2 binary counts 3.

#### RESULT — append after exposure

- **H1 accepted (maintainer-run 2026-10-26, 52 min):** **+4.3 ± 5.9 Elo**
  (nElo +6.7 ± 9.3), LLR +2.95 after 2,656 pairs (5,312 games); W/D/L
  1475/2427/1410, pentanomial [101, 627, 1138, 686, 104].
  - 0 faults, 0 time losses; 14 post-terminal pairs kept outside the
    sample; the independent PGN recount agrees, and the score recomputed
    from the pentanomial matches.
  - `colosseum_sprt_a116-gate_20261026_094056.*`, manifest `6cc0207d…`,
    run record `e1cfab01…`, PGN `1b526aea…`.
  - Mid-run (523 pairs) the read was −6.3 ± 13.4. Time use and depth were
    then equal (69.6 ms per move on both arms; mean depth 17.12 against
    17.09), so the leaf's unlisted channel, the time manager's root-effort
    ratio, which reads the node count, did not move the clock. Reported
    nodes per move fell 3.9% because the unit changed; the arms' NPS are
    not comparable.
- **Verdict (registered rule):** H1, so both repairs stay and the
  fingerprint record updates to `bench 13` 7,233,823.
- Disposition: **H1, +4.3 ± 5.9; both repairs kept. CLOSED 2026-10-26.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +3 Elo, most likely the cap; not H0 at about
  80%.
- **Reading:** H1, centred at +4.3, just above the predicted range, and
  well before the cap. Missed slightly high.
- **What it teaches:** a refuted root move at iterations 1 and 2 cost more
  than "the deeper iterations replace it" allowed for: every later
  iteration starts from the shallow iterations' ordering and table.
