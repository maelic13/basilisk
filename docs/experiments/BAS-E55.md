# BAS-E55

<!-- part 1 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E55 - 15.1.a release gate: the SEE-repaired dev head against
the 1.9.3 release binary - CLOSED, BOTH LEGS PASS** (2026-09-09).

- Date / owner / calibration category: 2026-09-09 / maintainer-run / release gate.
- **Clerical correction, 2026-09-10, after the result:** the release this gate
  qualified was renamed from **1.9.4 to 1.10.0**. The text below still says
  "1.9.4" and is left as frozen -- that was its name when the prediction was
  registered. Nothing measured changed; the rename follows this project's
  bench-fingerprint rule for minor versus patch.
- Baseline: `basilisk-v1.9.3-windows-x86_64-pext-pgo.exe` (bench 11,941,440).
  Candidate: the Phase 15.0 head (dev `d0f2627` plus 15.0.a-15.0.d), fresh
  PGO pext build, hash recorded in the run manifest.
- Research question: does the 1.9.4 line (accepted Group A endgames, 6.5.a
  rook scaling, SEE king legality and any 15.0.c repair) beat 1.9.3 at STC,
  and does it transfer at 4T?
- Hypothesis: yes; the accepted line measured about +12 Elo over 1.9.3 before
  6.5.a, and the SEE king rule removes illegal king recaptures from every SEE
  consumer (capture pruning, qsearch floor, ordering) as Rarog's repaired
  cluster did.
- Interacting mechanisms: SEE consumers in search; the repair changes the
  bench tree (Rarog: +10% nodes) so fixed-depth comparisons are not
  comparable and only games decide.
- Competing hypotheses: the king rule prunes fewer captures and costs time to
  depth with no accuracy gain at `3+0.03`; the 4T line has an untuned
  interaction with the repaired SEE.

PRE-REGISTERED PREDICTION, frozen before exposure:

- Expected Elo: **+10 to +20** at STC 1T against 1.9.3; 4T direction positive.
- Probability the candidate is positive: 0.85. Confidence moderate; the
  6.5.a and Group A gates were paired against each other, not against 1.9.3.
- Most likely failure mode: the SEE repair's node increase costs more at
  `3+0.03` than its accuracy returns, reading as a null instead of a loss.
- Registered gate: `tools/sprt.ps1` STC `3+0.03`, 1T, Hash 64, paired UHO
  (`tools/books/UHO_Lichess_4852_v1.epd`), no adjudication, `[0,3]` nElo,
  cap 20,000 games; plus a 4T `10+0.1` 400-game direction check, zero
  forfeits required. H0 returns 15.0 to research; it does not license
  reverting the repairs.
- **AMENDED 2026-09-10, BEFORE ANY EXPOSURE** (maintainer decision; no game of
  this gate has been played). **The 4T direction check moves from `10+0.1` to
  `3+0.03`.** Rationale: `3+0.03` is the deciding time control, and holding TC
  constant across the two legs makes THREAD COUNT the only variable, removing
  the TC/threads confound the original pairing carried. The 1T leg is
  unchanged. Three consequences are recorded here rather than discovered later:
  - **Cost of the change, stated up front:** shorter clocks raise time-forfeit
    exposure, and at `Threads>1` `tools/sprt.ps1` treats ANY nonzero forfeit
    count as invalidating (`sprt.ps1:581`). The forfeits characterised by
    15.0.b/BAS-C09 are 50-500 ms host stalls, which a 3-second clock absorbs
    far worse than a 10-second one; the script's own header records Rarog
    measuring 10 forfeits in 240 games at `Threads=4` in this configuration.
    The 4T leg is therefore at material risk of being voided by forfeits, and
    that risk is the price of removing the TC confound.
  - **The registered 400-game size cannot support a direction claim.** This is
    a defect in the original registration, not in the amendment.
    `tools/sprt.ps1:473` warns for every `Threads>1` run: "budget ~10k games
    (at 4T nothing under that separates 0 from +3 -- the same run read +1.78
    @1.5k and -0.81 @28.4k elsewhere)". A 400-game 4T sample is a crash and
    forfeit smoke test, and must be reported as one unless it is resized.
  - **No 4T `-Mode calibrate` null exists.** `tools/results/` still contains
    none, as BAS-C05 recorded on 2026-09-07. Affinity is dropped at
    `Threads>1`, so placement bias is unpinned and, per both BAS-C05 and the
    harness warning, no 4T verdict is trustworthy until that thread count has
    its own null. This is unchanged by the TC amendment and applies equally at
    `10+0.1`.
- **Runnable state validated 2026-09-10.** Baseline
  `tools/test_engines/basilisk-1.9.3-baseline-pext-pgo.exe`, bench
  **11,941,440** (matches the registered figure), built from dev `16eff20`;
  that commit differs from the `v1.9.3` tag `d737123` only in COMMENT text in
  `src/search_params.h`, so it is functionally the 1.9.3 release (clerical
  correction 2026-09-28: first written as `61e6f23`, a pre-release commit a
  stale local tag named; re-checked against `d737123`, where `src/` and the
  CMake files differ only in those comments). Candidate
  `tools/test_engines/basilisk-15.0-head-pext-pgo.exe`, revision `4aafddb`
  (the closed 15.0 head), bench **14,978,465**, clean tree. Both arms
  `release-pext (USE_PEXT=ON, TUNE=ON, PGO=USE)` and compiler-identical
  (clang 22.1.8), which is what `sprt.ps1:419` refuses to run without.
- **Hazard, do NOT use for either arm:**
  `build/dist/basilisk-v1.9.3-windows-x86_64-pext-pgo.exe` no longer contains
  1.9.3. The local `release-pext` PGO target republishes into that
  version-named path, so 15.0.e's asset rebuild overwrote it with the 15.0
  head. The registration originally named that filename as the baseline; it
  now names the `tools/test_engines/` copy above, whose bench was verified
  against the registered value.
- **AMENDED 2026-09-10 (second), BEFORE ANY 4T EXPOSURE** (maintainer
  decisions; the 1T leg was already running, the 4T leg had not started):
  - **4T size fixed at 2,000 games, not the ~10k the harness advises.**
    Maintainer decision on wall-clock grounds. Recorded honestly: at 2,000
    games the interval is roughly **+/-9 to +/-10 Elo** (scaling the +/-4.2 at
    10,000 by sqrt(5), before the extra variance that unpinned placement adds
    above 1T), so this leg **cannot support a direction claim** and is not
    reported as one. What it CAN do, and what it is registered as, is a
    **4T smoke gate**: it detects a catastrophic SMP-specific regression
    (worse than about -10 Elo), a crash, or a time-forfeit cluster. That is a
    real and sufficient purpose for the release decision -- the 1T leg is what
    decides strength -- but the word "direction check" in the original
    registration overstates what 2,000 games at 4T can measure, and the
    verdict must be written as pass/no-pass on the smoke criteria only.
  - **The 4T `-Mode calibrate` null is attested by the maintainer as already
    run elsewhere, and the harness is accepted as sound at this thread count.**
    Recorded as a maintainer attestation, not as an artifact in
    `tools/results/`; the earlier BAS-C05 objection that no 4T null existed is
    resolved by that attestation for the purposes of this gate.
  - Registered 4T command shape: `-Mode fixed -Games 2000 -Threads 4`, TC
    `3+0.03`, Hash derived per side (256 MB at 4T), same paired UHO book, no
    adjudication. Zero time forfeits are still required: `sprt.ps1:581` voids
    a `Threads>1` run on any nonzero count, and the shorter clock raises that
    exposure, so a forfeit-voided 4T leg is a foreseeable outcome rather than
    a surprise.

RESULT, 1T LEG (2026-09-10). Run
`sprt_15.0-head_vs_1.9.3_20260910_103009`, 51:37 wall.

**Elo +19.18 +/- 6.76, nElo +29.80 +/- 10.48, LOS 100.00%, LLR 2.95 ->
H1 ACCEPTED at 4,224 games.** W 1,237 / L 1,004 / D 1,983, 52.76%.
DrawRatio 44.60%, PairsRatio 1.35, Ptnml(0-2) [72, 425, 942, 544, 129],
WL/DD 0.86.

- **VERDICT on the deciding leg: PASS.** The 15.0 head beats the 1.9.3
  release by a clear margin at STC. The 1.9.4 line is justified; 15.1.b may
  proceed once the 4T smoke gate reports.
- **Time forfeits: 2 in 4,226 games = 0.047%, exactly one per engine.**
  Admissible under BAS-E56's amended rule (ceiling 0.5%; a 1-1 split is
  maximally symmetric). They are ALSO a single transient host event rather
  than two independent ones: the games ended at 11:11:52 and 11:12:08 local,
  **16 seconds apart** in a 3,097-second run -- rounds 1717 and 1729. Two
  independent draws landing that close have probability about 1%. One engine
  lost each. This is direct positive evidence for 15.0.b's diagnosis (50-500
  ms stalls of a saturated host, invisible to a mid-search engine) and
  against any engine-side clock defect, and it supersedes the 3-0
  candidate-side skew seen in BAS-E56, which was never significant at n=3.
- **Harness defect found while reading this, reported not fixed:**
  `tools/sprt.ps1`'s forfeit counter greps
  `loses on time|timeouts:\s*[1-9]`, which also matches fastchess's own
  per-player `Timeouts: N` SUMMARY lines. It reported "4 log line(s)" for 2
  real forfeits -- 2 events plus 2 summary lines. The comment at that site
  shows the author anticipated the summary-line hazard and excluded
  `Timeouts: 0`, but `Timeouts: 1..9` still double-counts. The inflation is
  +1 per player with a nonzero count, so it never turns a clean run dirty,
  but it overstates magnitude on any run that has forfeits at all -- and at
  `Threads>1`, where any nonzero count voids the run, a reader chasing a
  phantom extra pair wastes the investigation.

PREDICTION CALIBRATION (against the record frozen 2026-09-09):

- Predicted **+10 to +20 Elo**, P(positive) **0.85**. Observed **+19.18 +/-
  6.76**, inside the interval and near its top, sign called correctly, LOS
  100%. The interval, the sign and the confidence were all sound.
- The registered "most likely failure mode" -- the SEE repair's node
  increase costing more at `3+0.03` than its accuracy returns, reading as a
  null -- **did not occur**, and BAS-C08 explains why it was never likely:
  +19.17% bench nodes is only **0.165 ply** at the measured EBF of 2.900, so
  the cost was roughly a sixth of what its headline percentage suggested.
  BAS-E56 had already measured that leg in isolation at -0.65 +/- 5.25, i.e.
  free; this gate measures the whole 1.9.3->15.0 line, whose gain comes from
  the accepted Group A endgame and 6.5.a rook work rather than from 15.0.a.
- Nothing in the causal model needs revising. The one correction already
  recorded stands: convert a node delta into plies via the EBF before calling
  it expensive.

RESULT, 4T SMOKE GATE (2026-09-10). Run
`sprt_15.0-head_vs_1.9.3_20260910_112713`, `-Mode fixed -Threads 4`,
`3+0.03`, Hash 256 MB per side, concurrency 3 (12 of 16 physical cores).

**Elo +26.11 +/- 27.28, nElo +39.18 +/- 40.70, LOS 97.04%, 280 games**
(305 written to the PGN before the maintainer stopped it). W 86 / L 65 /
D 129, 53.75%. DrawRatio 43.57%, PairsRatio 1.47,
Ptnml(0-2) [5, 27, 61, 36, 11], WL/DD 0.85.

- **VERDICT: PASS on all three registered smoke criteria.**
  1. **Crashes: zero.** No crash, disconnect, illegal move or engine
     termination in the log.
  2. **Time forfeits: ZERO in 305 games at 4T.** Any nonzero count would have
     voided the run.
  3. **No catastrophic SMP regression.** The 95% interval's lower bound is
     **-1.17 Elo**, comfortably above the registered -10 threshold.
- **Stopped at 280 of the registered 2,000 games** on maintainer judgement.
  Recorded as a data-dependent stop. It does not undermine the registered
  purpose: the smoke criteria are coarse, and 280 games already bound a
  catastrophic regression, which is all this leg was ever asked to do.
- **The point estimate must NOT be read as 4T gaining more than 1T.** At
  +/-27.28 the interval spans -1.17 to +53.39; +26.11 and the 1T leg's +19.18
  are statistically indistinguishable. This is precisely why the leg was
  relabelled a smoke gate rather than a direction check.
- **A concern registered in this experiment's own amendment was FALSIFIED.**
  The second amendment warned that moving the 4T leg from `10+0.1` to
  `3+0.03` would raise forfeit exposure and that "a forfeit-voided 4T leg is
  a foreseeable outcome". It measured **zero** forfeits, against the script
  header's citation of Rarog seeing 10 forfeits in 240 games at `Threads=4`.
  The concern was wrong in this configuration, and the reason is informative:
  the 4T run occupies **12 of 16 physical cores** while the 1T run occupies
  **14 of 16**. The LESS saturated run had zero forfeits and the MORE
  saturated one had two. That is a third independent line of evidence for
  15.0.b's host-saturation diagnosis -- forfeit incidence tracks host
  headroom, not thread count and not clock length -- and it is the opposite
  of what a per-thread clock-handling defect would produce.

<!-- part 2 of 2: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E55 CLOSED 2026-09-10: BOTH LEGS PASS.** The deciding 1T leg accepted
H1 at +19.18 +/- 6.76 Elo; the 4T smoke gate is clean on crashes, forfeits
and catastrophic regression. The 1.9.4 release line is justified and 15.1.b
may proceed.
- **AMENDED 2026-09-09, BEFORE ANY EXPOSURE** (maintainer decision, no
  result of any kind observed): the clause above is superseded for 15.0.a
  only. 15.0.a now carries its own isolated gate, BAS-E56, whose verdict
  rule may revert the king-legality repair and accept the pre-repair
  behaviour as a documented risk, on the BAS-C05 precedent. BAS-E55 is
  otherwise unchanged and still decides the 15.0 head against 1.9.3.
  Recorded as an explicit clerical amendment to a frozen registration.
