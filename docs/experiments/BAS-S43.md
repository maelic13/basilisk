# BAS-S43

**BAS-S43 — B.3.5: the Cluster 2 gates**

- **Registration.** Created 2026-11-03 before any of its games, while
  BAS-S45 (B.3.4) is prepared. Maintainer-run games; the agent builds,
  pins and dry-runs. Assembled from packet §11's BAS-S43 text
  (`analysis/b30_cluster2_research_2026-10-30.md`, the packet's B.3.4),
  the *Roadmap audit*'s replaced clause for (b), and PLAN B.3.5 with its
  2026-11-02 clause on P9.
- **Candidate.** B.3.4's: the fitted ON arm with whatever BAS-S45's rule
  adopts, as Release final-PGO builds of one revision. The candidate is
  fixed by that rule, written before any game here. Its build identity is
  recorded below before (a)'s first game.
- **Research question.** Does the fitted Cluster 2 arm gain against the
  OFF arm at `[0,3]`, and is the fit itself worth anything against the
  unfitted arm?

#### GATES

- **(a)** The candidate ON arm against the OFF arm at the candidate's
  revision.
  - SPRT `[0,3]` nElo (`sprt-default.toml`) at `3+0.03`, 1T, Hash 64,
    paired reversed UHO, no adjudication, tablebases off, clean final-PGO
    builds and manifests. Seed 43001.
  - **Cap 2,000 pairs.** The drift model at the expected value: B.3.2's
    unfitted paired run read +67.5 Elo, which is +108.7 ± 15.2 nElo at
    1,000 pairs, so σ ≈ 245 nElo per pair. The expected stop is
    2.94 / (3 · 107 / 245²) ≈ 550 pairs; the cap is about 3.6×.
  - **H1:** the B3_PROOF default flips ON and the fingerprint record
    moves to the ON arm.
  - **H0:** the default stays OFF, the evidence is kept and the
    fingerprint record is unchanged.
  - A cap without a decision is recorded and goes to the maintainer.
- **(b)** The fitted arm at B.3.3's bake revision (engine of `f02a9e4`,
  `bench 13` 5,369,042), before B.3.4's adoptions, against the unfitted
  arm (`basilisk-b32-on-pgo1-pext-pgo.exe`, `0B5D9DC6…`, engine of
  `c6f7151`, 4,923,507).
  - SPRT `[0,3]`, the same conditions. Seed 43002.
  - **Cap 12,000 pairs.** The expected value is about +5 Elo (+8 nElo):
    the fit was mostly flat (one full-step mover in 29; B.3.3's analysis,
    finding 4). The expected stop is 2.94 / (3 · 6.5 / 245²) ≈ 9,000
    pairs, so the cap is 1.3×.
  - **The packet registers no consequence for (b).** Its verdict and its
    first-crossing estimate are recorded. A reversion of theta to the
    seeds, if any, is the maintainer's decision; it is not taken by this
    rule.
- **(c)** A direction read, not a gate: 2,000 games at `10+0.1` 1T of
  (a)'s pair, on `match-fixed-ltc.toml`. Seed 43003. It is recorded with
  its interval beside `B3NmpCutOnly`'s LTC read (B-33) and scores P14.
- **Before the gates (PLAN, 2026-11-02): P9's consequence on the fitted
  arm.**
  - First, the share of search time in the upcoming-repetition check, from
    the Release ON build's profile (`tools/profile.ps1`, a Profile build of
    the candidate, on an idle host).
  - Only if that share is material: pooled NPS of two Release pools that
    differ only in `B3Cuckoo`'s default.
  - The maintainer then chooses B.7 or keeping the donor's form at its
    cost.
  - The gates (a) and (c) wait for that choice. (b) does not involve it,
    since both of its arms carry the check.
- **Order:** BAS-S45's games, the P9 profile, (b), then the maintainer's
  P9 choice, then (a) and (c).
- **Reject (packet):** H0 on (a) reverts the default, keeps the evidence
  and restores the fingerprint record.
- No candidate, bound, cap, book or adjudication changes after a game is
  observed.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Carried from the packet (§11 and the predictions table), verbatim:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P2 | After B.3.3 and the gate, B.3 reads **+20 Elo at STC**, 90% interval [0, +45]; H1 probability 55% | moderate | H0 at `[0,3]` or an estimate under +5 refutes the coherent-cluster share for proof searches; the residual then lies in B.4/B.5 |
| P9 | Speed: pooled NPS **0.98–1.01×** the OFF arm (the cuckoo loop and `follow_pv` are the only per-node additions) | moderate-high | below 0.97 sends the cuckoo loop to B.7 before the gate |
| P14 | B.3.4's `10+0.1` direction read of the fitted arm shows no conversion regression against the OFF arm: the won-position share of its decisive games (a win from a position the oracle scores ≥ +564 at move 40) within 3 points of the OFF arm's | low-moderate | a drop says `seekMate` at 12 costs conversion at length and the sweep's 16 is re-read at `10+0.1` |

- **P2's reading.** "B.3" is (a)'s pair. B.3.2's unfitted read was
  already +67.5, far above P2's band, so P2's estimate leg is scored on
  (a)'s first-crossing estimate, with that caveat recorded.
- **P9's reading.** It is scored on game NPS and the profile, as PLAN's
  clause says, never on `bench 13` pools of different trees.

Added before exposure:

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| G1 | (a) accepts H1 | 95% | H0 |
| G2 | (b) accepts H1 | 40% | — (scored either way) |
| G3 | The upcoming-repetition check takes under 1% of search time in the Release ON profile | 70% | ≥ 1% |

#### RESULT — append after exposure

- **The arms (2026-11-04).** B.3.4 adopted nothing, so the candidate is
  the fitted arm. Every build is a clean final-PGO PEXT build at
  `5f1cdf3`, whose engine is `f02a9e4`'s:

  | Build | Bench 13 | SHA-256 |
  |---|---:|---|
  | ON Release (`basilisk-b35-on-pext-pgo.exe`) | 5,369,042 | `3DFF7B64…A810` |
  | OFF Release (`basilisk-b35-off-pext-pgo.exe`) | 11,880,564 | `4127849D…CCE9` |
  | ON Profile (`basilisk-b35-on-prof-pext-profile-pgo.exe`) | 5,369,042 | `389FD285…3F06` |

  - (b)'s fitted arm is the ON Release build; its unfitted arm is
    `0B5D9DC6…`.
  - `tools/run_b35_gates.ps1` pins them. Its dry runs of (a), (b) and (c)
    are accepted, with `[0,3]` normalized, the caps 2,000 and 12,000
    pairs, `3+0.03`, (c) at `10+0.1` for 2,000 games, and the seeds
    43001-43003.
- **P9's profile (2026-11-04).** `tools/profile.ps1` on the ON Profile
  build, cohort `bench 13` × 5, 1 ms sampling on an idle host
  (`tools/results/b35-profile-on/`, `report.json` `289940bb…`,
  `samples.json` `57dc58cd…`). 11,225 samples, inline chains on 61.8%,
  none unresolved.
  - **`Board::upcoming_repetition` appears in the inline chain of 0.30%
    of in-module samples** (32 of 10,737; Wilson 95% [0.21%, 0.42%]). It
    is the innermost frame in 0.15%.
  - The first run was refused because the region map lacked
    `search_b3.h` and `cpu_detect.h`. They are mapped in `ec9d6de`, and
    the map's test passes again.
  - **The share is not material**, so the pooled NPS read of two
    `B3Cuckoo` pools is not owed (PLAN's clause).
  - **G3 hit** (under 1%).
  - **P9** is scored on game NPS at (a), as its reading says.
- **Waiting on the maintainer:** the P9 choice, either B.7 or keeping the
  donor's form at its cost. Item 9 read the check at 0.0 ± 9.3 Elo, and
  it costs 0.3% of search time on this profile. (a) and (c) wait for the
  choice; (b) does not.
- **The maintainer's P9 choice (2026-11-04): keep the check** (the
  donor's form) at its 0.3%. It does not go to B.7. The candidate is
  unchanged, and (a), (b) and (c) are released to the maintainer.
- **A reference read, not part of any gate (2026-11-04):** our 40 bench
  positions replayed at depth 13, 1T, Hash 16, after one `ucinewgame`
  (`bench_replay.py`, the sum of each search's final node count; all 40
  answered):
  - Stockfish 19 (the donor, NNUE): **1,911,294**.
  - Stockfish 11 (the last classical evaluation): **4,797,242**.
  - The ON arm: 5,369,042. The OFF arm: 11,880,564.
  - A node count to a nominal depth is not comparable across engines,
    because depth means a different tree in each. It says how hard each
    engine prunes to reach "depth 13", not how strong it is.
