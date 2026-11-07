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
