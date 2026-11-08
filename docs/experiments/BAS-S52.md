# BAS-S52

**BAS-S52 — B.4.7: the Cluster 3 gates**

- **Registration.** Created 2026-11-07 before any of its games. Maintainer-run games;
  the agent builds, pins and dry-runs. Assembled from packet §11's BAS-S52 text
  (`analysis/b43_cluster3_research_2026-11-06.md`), PLAN B.4.7 and B.4.4's review.
- **Candidate.** The fitted ON arm as B.4.6 baked it, `916618c`'s engine, ON `bench 13`
  4,144,790, as Release final-PGO builds of one revision. Build identities are recorded
  below before the first game.
- **Research question.** Does the fitted Cluster 3 arm gain against the OFF arm at
  `[0,3]`, is the fit worth anything against the unfitted arm, and can the delta rule go?

#### GATES

All at 1T, Hash 64, paired reversed UHO, no adjudication, tablebases off, clean
final-PGO builds and manifests. σ ≈ 245 nElo per pair, from B.4.5's paired run (+47.4 ±
15.2 nElo at 1,000 pairs).

- **(a)** The candidate ON arm against the OFF arm at the candidate's revision.
  - SPRT `[0,3]` nElo (`sprt-default.toml`) at `3+0.03`. Seed 52001.
  - **Cap 4,500 pairs.** The expected value is B.4.5's unfitted +47.4 nElo, so the
    expected stop is 2.94 / (3 · 45.9 / 245²) ≈ 1,280 pairs; the cap is about 3.5×.
  - **H1:** the default flips ON and the fingerprint record moves to the ON arm. Before
    the flip, the ten clauses of the arm's quiescence that no test binds get tests in
    their own commit, each shown to fail on its mutant (packet *Review — B.4.4 accepted
    with named follow-ups*, §5).
  - **H0:** the default stays off, the evidence is kept and the fingerprint record is
    unchanged.
  - A cap without a decision is recorded and goes to the maintainer.
- **(b)** The fitted arm against the unfitted arm: the candidate against
  `basilisk-b45s-on-pgo1-pext-pgo.exe` (9C9E463E…, engine of `516d422`, 4,003,560).
  - SPRT `[0,3]` at `3+0.03`. Seed 52002.
  - **Cap 12,000 pairs.** The expected value is about +5 Elo (+8 nElo): one full-step
    mover in 15 and a mostly flat surface (B.4.6's analysis, finding 7). The expected
    stop is 2.94 / (3 · 6.5 / 245²) ≈ 9,000 pairs; the cap is 1.3×.
  - **The packet registers no consequence for (b).** Its verdict and first-crossing
    estimate are recorded. A reversion of theta to the seeds, if any, is the maintainer's
    decision; it is not taken by this rule.
- **(c)** The delta rule's deletion: the fitted arm (`B4QsDelta` 0) against itself with
  `B4QsDelta` 1, one Tune build.
  - SPRT `[-5,0]` nElo (`sprt-removal.toml`) at `3+0.03`. Seed 52003. Arm A is the
    deletion and arm B the rule, so H1 says removal is not worse.
  - `B4QsDelta` 1 restores the delta rule under the arm's band guard, not the OFF arm's
    (B.4.4's review).
  - **Cap 18,000 pairs.** The expected value is 0: the rule is live at 3.8% of quiescence
    nodes. The expected stop is 2.94 / (5 · 2.5 / 245²) ≈ 14,100 pairs; the cap is 1.3×.
  - **H1:** the deletion stands. **H0:** the rule is restored on the arm as `B4QsDelta`
    1 and its margin becomes a coordinate (P11's falsifier); the candidate is rebuilt and
    re-pinned before (a).
  - A cap without a decision is recorded and goes to the maintainer.
- **(d)** A direction read, not a gate: 2,000 games of (a)'s pair at `10+0.1`, on
  `match-fixed-ltc.toml`. Seed 52004. Recorded with its interval.
- **Before (a): P9's falsifier.** P9 is a miss and its falsifier fires: it returns ticket
  1's evasion delivery to implementation before the gate. BAS-S50 answered it with
  measurement and left the handling to the maintainer, who deferred it on 2026-11-06.
  Neither (b) nor (c) involves it, since both arms of each carry the delivery.
- **Order:** (b), (c), the maintainer's P9 choice, then (a) and (d). (c) comes before (a)
  because its H0 changes the candidate. `tools/run_b47_gates.ps1` refuses (a) and (d)
  until that is recorded.
- **Reject (packet):** H0 on (a) leaves the default off, keeps the evidence and the
  fingerprint record.
- No candidate, bound, cap, book or adjudication changes after a game is observed.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Carried from the packet, verbatim:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P2 | After B.4.6 and the gate, B.4 reads **+10 Elo** at STC, 90% interval [−3, +28]; H1 at `[0,3]` probability 50% | low-moderate | H0 with P3 to P5 held says a donor-shaped quiescence carries no Elo on this HCE; the arm stays off and B.5 proceeds |
| P11 | The delta rule's deletion: B.4.7 (c) accepts H1, removal is not worse (75%) | moderate | H0 restores the rule on the arm as `B4QsDelta` 1 and its margin becomes a coordinate |

- **P2's reading.** "B.4" is (a)'s pair. B.4.5's unfitted +29.3 ± 9.5 already sits above
  P2's point, so its estimate leg is scored on (a)'s first-crossing estimate with that
  caveat recorded.

Added before exposure:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| G1 | (a) accepts H1 | 90% | H0 |
| G2 | (b) accepts H1 | 35% | — (scored either way) |
| G3 | (d) reads above 0 | 80% | at or below 0 |

#### RESULT — append after exposure

- **The arms (2026-11-07).** Every build is a clean final-PGO PEXT build at `507fc7b`,
  whose engine is `916618c`'s (clang 23.1.5):

  | Build | Bench 13 | SHA-256 |
  |---|---:|---|
  | ON Release (`basilisk-b47-on-pext-pgo.exe`) | 4,144,790 | AB97234E… |
  | OFF Release (`basilisk-b47-off-pext-pgo.exe`) | 6,090,091 | DB15A497… |
  | ON Tune (`basilisk-b47-on-tune-pext-tune-pgo.exe`) | 4,144,790 | AA404AB3… |
  | Unfitted ON (`basilisk-b45s-on-pgo1-pext-pgo.exe`, `516d422`) | 4,003,560 | 9C9E463E… |

  - `tools/run_b47_gates.ps1` pins them. Its dry runs of all four are accepted, and the
    resolved seeds, caps and clocks were checked against this entry. (a) and (d) refuse a
    real run until the order allows it, as both refusals showed.
  - `B4QsDelta` 1 is live on the Tune build: the `bench 13` replay reads 4,069,419
    against the control's 4,144,790.
  - WAC at 100,000 nodes on the Release builds: fitted 254, OFF 247.
- **The maintainer's P9 choice (2026-11-07): closed, ticket 1 not returned.** The evasion
  delivery stays as built. It measured about 0.8 of the 5.5-point `bench 13` gap, and in
  play the unfitted arm ran level with the OFF arm (0.99 to 1.01× by game phase). P9 stays
  a miss. The fitted arm's speed has not been read. This clears P9 from the order; (a) and
  (d) still wait on (c)'s verdict.
- **(b): H1 (2026-11-07).** LLR +2.95 at 6,316 pairs, 12,632 games, about 2 hours (cap
  12,000 pairs). First-crossing estimate **+6.5 ± 3.6 Elo, +10.9 ± 6.1 nElo**;
  pentanomial [164, 1444, 2911, 1585, 212]. No fault, no time loss, no anomaly.
  `tools/results/b47-gate-b/` (`result.json` 3a88a240…). The fit stands, and G2 is a hit at
  35%.
- **(c): H1 (2026-11-07).** LLR +2.95 at 1,297 pairs, 2,594 games, 26 minutes (cap 18,000
  pairs). First-crossing estimate **+15.0 ± 8.0 Elo, +25.1 ± 13.4 nElo** for the deletion;
  pentanomial [31, 271, 599, 347, 49]. No fault, no time loss, no anomaly.
  `tools/results/b47-gate-c/` (`result.json` 4d62c266…). The deletion stands, the candidate is
  unchanged, and P11 is a hit.
  - The stop is early because the reading is far from the bracket: at +25 nElo the drift
    model gives 2.94 / (5 · 27.6 / 245²) ≈ 1,290 pairs, against about 14,100 at the
    expected 0.
  - The expected value of 0 was wrong in sign, and this does not price the rule. Theta was
    fitted with the rule off, so arm B carries it unfitted, and the estimate is a
    first-crossing one. Only the verdict is the record.
- **Both verdicts were recounted** from `games.jsonl`: pentanomials equal the run records,
  every pair has one opening with colours reversed, the 30 post-terminal games of each
  run are outside the sample, and a normal-approximation LLR reads +2.95 and +2.96. All
  three binaries still hash to their pins.
- **Speed as (b)'s games read it**, recorded and carrying no rule: the fitted arm 1.97M
  nodes a second against the unfitted arm's 2.01M, 0.977×, at 137,284 against 140,356
  nodes a move, and mean depth without mate scores 16.48 against 16.74. (a) reads the
  fitted arm against the OFF arm directly.
- **(a) and (d) are unlocked:** P9 is closed and (c) left the candidate as pinned, so
  `$candidateFixed` is set and no binary changes.
- **(a): H1 (2026-11-07).** LLR +2.96 at 1,230 pairs, 2,460 games, 25 minutes (cap 4,500
  pairs). First-crossing estimate **+31.7 ± 8.6 Elo, +50.8 ± 13.7 nElo**; pentanomial
  [28, 224, 551, 350, 77]. No fault, no time loss, no anomaly. `tools/results/b47-gate-a/`
  (`result.json` bbb0371d…).
- **(d) (2026-11-07).** **+38.4 ± 9.2 Elo, +64.3 ± 15.2 nElo** at `10+0.1`, 2,000 games,
  68 minutes; W/D/L 622/976/402, pentanomial [10, 183, 438, 315, 54]. No fault, no time
  loss, no anomaly. `tools/results/b47-direction-ltc/` (`result.json` 6cbefbb0…). A
  direction read: it carries no rule.
- **Both were recounted** from `games.jsonl`: (a)'s pentanomial equals its run record,
  (d)'s equals the harness's closing line (a match's run record holds none), every pair
  has one opening with colours reversed, and (a)'s 28 post-terminal games are outside its
  sample. Seeds, clocks and adjudication match this entry, and both binaries still hash to
  their pins.
- **Speed and depth as the games read them**, recorded and carrying no rule:

  | Read | ON nodes a second | OFF | Ratio | ON depth without mates | OFF |
  |---|---:|---:|---:|---:|---:|
  | (a), `3+0.03` | 1.99M | 2.01M | 0.989× | 16.57 | 15.60 |
  | (d), `10+0.1` | 2.02M | 2.06M | 0.978× | 20.60 | 19.04 |

- **Predictions.**

  | # | Result | Score |
  |---|---|---|
  | P2 | H1 at `[0,3]`, against 50%; +31.7 first-crossing against +10, [−3, +28] | H1 **hit**; the estimate is above the interval, with the caveat of its reading above |
  | P11 | (c) accepted H1 | **hit** |
  | G1 | (a) accepted H1 | **hit** |
  | G2 | (b) accepted H1, against 35% | **hit** |
  | G3 | (d) read above 0 | **hit** |

- **The H1 clause, in its order.**
  - `a346d22` binds the ten clauses (M1, M2, M3, M5, M6, M7, M8, M11, M13, M16) with nine
    sections of `test_b4_qsearch`; M2 and M16 share one. Each mutant fails its section on
    a Diag and a Release tree, and the controls pass before and after
    (`tools/results/b47-20261107/clause-mutants.json`). The sections pass on the ON and
    OFF arms in Release, Tune and Diag builds.
  - `312dd5b` flips `B4_QSEARCH` to ON, and the fingerprint record moves to **4,144,790**.
    `9ef253d` makes CI's arm job the OFF arm and pins `B4_QSEARCH=OFF` in its pre-Cluster-2
    job.
  - On fresh configures (`tools/results/b47-20261107/flip/`): the default build `bench 13`
    4,144,790, CTest 27/27; `B4_QSEARCH=OFF` 6,090,091, CTest 27/27; the default Tune build
    4,144,790, with `test_search`, `test_b3_proof` and `test_b4_qsearch` passing; the Texel
    build's verify passes.
- **Found, not repaired: CI's pre-Cluster-2 job has one failing section.** With
  `B3_PROOF=OFF` (`bench 13` 15,198,763), `test_b4_qsearch`'s "the exit store in check"
  fails at `EXPECT(!run.node_stored)`, CTest 26/27.
  - It fails the same way at `507fc7b`, so neither the flip nor the new tests caused it.
    The section dates from `d9b11ab`, and this branch has not yet run in CI.
  - On that arm every node in check takes the check extension
    (`src/search_kernel_core.cpp:842`), so the section's child is a depth-1 negamax node
    that stores, not a quiescence node. The section's premise fails there; the search does
    not.
  - **Settled (maintainer, 2026-11-07):** tests bind what the engine uses, not what it is
    replacing. `test_b4_qsearch`, `test_b3_proof`, `test_search` and `test_wac` lose their
    old-arm branches, and CI builds the two OFF arms without testing them. The ON arm's
    section counts are unchanged, so nothing that binds the default path went. The OFF
    arms' tests are no longer expected to pass.
- Disposition: **CLOSED 2026-11-07.** All three SPRTs H1: `B4_QSEARCH` defaults ON at
  **4,144,790**, and the OFF arm stays compilable at 6,090,091 until the search programme's
  cleanup.
