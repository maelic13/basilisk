# BAS-S59

**BAS-S59 — B.6.6: the donor-form paired runs (TT-cutoff cap, razoring, rule-50 key, kings in the non-pawn key)**

- **Registration.** Created 2026-11-12, before any floor result was read
  and before any game; maintainer-run games, agent-run vetoes; search
  reconciliation, Elo layer. The packet is
  `analysis/b66_reconciliation_2026-11-12.md`; its §3 sorts every
  departures row and §4 gives the four rows that run, with the four
  questions answered for each. The maintainer chose all four rows
  (2026-11-12).
- **Baseline / candidate.** One revision, `1b7256a`: the B.6.5 head with
  four switches, each off by default, so `bench 13` is **3,783,135** on
  the release build and on the tune build. Arm A plays the donor's form by
  UCI option, arm B the defaults, on one PGO Tune build:
  `tools/test_engines/basilisk-b66-pext-tune-pgo.exe`, SHA-256
  `9AE3C8C98FA1653D3CDF3ED94ADBC960A8DFFD108D4387220EB8A0B0AEE1F649`,
  Clang 23.1.5, PEXT tier, portable codegen, manifest clean at
  `1b7256a`.
- **Research question.** For each row, does the engine want the donor's
  form over the one it was fitted with, by enough to adopt it before
  B.6.7 fits the whole search?
- **Hypothesis / mechanism.** Packet §4, per row.
- **Interactions.** Packet §5.
- **Competing explanations.** A run on one fitted arm is tilted toward
  Basilisk's form, against which every arm here was fitted (maintainer
  2026-11-04): a reading under +5 does not clear the donor's form.

#### CONDITIONS

- **Every run:** 2,000 games (1,000 paired reversed UHO openings), 1T,
  Hash 64, no adjudication, tablebases off, Colosseum `match-fixed`, the
  recount from the PGN; runner `tools/run_b66_games.py`, which pins the
  binary by SHA-256 and passes revision, bench and hash to
  `tools/colosseum.ps1`.

| Run | Arm A | Arm B | Control | Seed |
|---|---|---|---|---|
| `TtCap` | `CoreTtCutoffBonusCap=1` | defaults | `3+0.03` | 66001 |
| `TtCap10` | `CoreTtCutoffBonusCap=1` | defaults | `10+0.1` | 66002 |
| `Razor` | `CoreRazorDonor=1` | defaults | `3+0.03` | 66003 |
| `Key50` | `CoreTtKey50=1` | defaults | `3+0.03` | 66004 |
| `Kings` | `CoreCorrNonPawnKings=1` | defaults | `3+0.03` | 66005 |

- **The switches** (`src/search_params.h`, kernel switches):

| Option | 1, the donor's form | 0, Basilisk's (default) |
|---|---|---|
| `CoreTtCutoffBonusCap` | the TT-cutoff quiet bonus is `min(slope · depth, CoreTtCutoffBonusMax)`, 695 (the donor's cap; history units are the donor's) | `slope · depth`, no cap |
| `CoreRazorDonor` | at a non-PV node, any depth, the estimate below `alpha − CoreRazorQuadMargin · depth²`, 136 (the donor's 482 at 0.282), returns quiescence; no table-move or bound guard | depth ≤ `CoreRazorDepthCap` (1), below `alpha − CoreRazorMargin · depth`, not when the table suggests a quiet move or proves a fail high |
| `CoreTtKey50` | every table read, write and prefetch uses `tt_key50`: the key mixed with `(rule50 − 14) / 8` from a count of 14; repetition keeps the raw key. This also gives quiescence's cutoff the donor's rule-50 protection (F11) | the raw key |
| `CoreCorrNonPawnKings` | each side's non-pawn correction key includes its king | kings left out; every bare king shares one entry |

- **Vetoes, before a run plays** (`run_b66_games.py` applies them to
  `tools/results/b66-20261112/floors/summary.json`, read by
  `floors.py` on release-preset variant builds with the one default set to
  1, beside the head's own build):
  - the 25-budget conversion floor (`floors_grid.py`) PASS, with its exit
    status 0;
  - the correctness suite of `test_endgames` (mate fixtures, wrong-bishop
    draws) exit 0;
  - WAC at 100,000 nodes no more than **5** below the head's, with all 300
    records read.
  An arm that fails a veto does not play; its row stays Basilisk's, and the
  failure is recorded as its reading.
- **Decision rule, per row** (PLAN B.6.6; PROCESS *Categorical switches*):
  the donor's form is adopted at ≥ +5 Elo when the 95% interval also
  excludes 0. Otherwise Basilisk's form stays. A row that reads under +5
  is not cleared by the run, which is tilted toward the fitted form: it
  waits in H.2 for the fair re-read with both arms refitted.
- **The TT-cutoff row's two legs:** adopted when either leg adopts and the
  other is not at ≤ −5 with its interval excluding 0. One leg adopting and
  the other rejecting leaves Basilisk's form, both readings carried to H.2.
- **After the runs:** the adopted forms become the defaults together behind
  one switch and pass an SPRT `[0,3]` against the head before B.6.7
  registers its surface (PLAN B.6.6). With no adoption there is no SPRT.
- **Liveness** (the pinned binary, `go depth`, nodes): a middlegame
  (`r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP1B1PPP/R2QKB1R w KQ - 3 9`,
  depth 15) reads 176,170 at the defaults; 236,345, 255,838, 176,170 and
  135,864 with each switch on in the table's order. A rook ending at a
  rule-50 count of 40 (`8/5k2/3r4/8/3R4/4K3/5P2/8 w - - 40 80`, depth 18):
  165,063; 189,991, 149,322, 135,783 and 154,266. Every switch moves the
  tree; the rule-50 key needs a count of 14 or more to.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Frozen 2026-11-12, before the floors were read:

| # | Prediction | Probability |
|---|---|---|
| P1 | `TtCap` at `3+0.03` reads within ±5 | 65% |
| P2 | `TtCap10` reads higher than `TtCap` (the form scales with depth) | 55% |
| P3 | `TtCap10` adopts (≥ +5, interval excluding 0) | 15% |
| P4 | `Razor` reads below 0 (the arm was fitted around depth-1 razoring, and the donor's form drops the two guards) | 60% |
| P5 | `Razor` adopts | 10% |
| P6 | `Key50` reads within ±5 | 75% |
| P7 | `Kings` reads within ±5 | 75% |
| P8 | Every arm passes its vetoes; if one fails, it is `Razor`'s WAC | 70% |
| P9 | No row adopts, so no SPRT runs and B.6.7 fits Basilisk's forms | 60% |

- **Expected:** P1, P6, P7, P9. **Falsification:** an adoption (P9), and
  each row's own line.
- **Registered gate / stop rule.** Each run is a fixed 2,000-game match,
  never extended or stopped early; the decision rule above reads its final
  estimate.

#### RESULT — append after exposure

- **Vetoes, before any game** (2026-11-12, packet §7.1): every arm passes.
  WAC at 100,000 nodes reads head 259; `ttcap` 268, `razor` 259, `key50`
  257, `kings` 261. The 25-budget grid passes on all five trees (KBN-K's
  five budgets 118 to 120 against 112); `test_endgames` 191/191 on each.
  `bench 13` on the variant builds: 3,878,257, 3,422,264, 4,279,787,
  3,019,185. P8 hits.
- **Dry runs**: all five accepted (packet §7.2). The games are the
  maintainer's (packet §7.3).
- **The games** (maintainer-run overnight 2026-11-12 to 13, the chain of
  packet §7.3): all five completed 2,000 games, natural termination, 0
  time losses, 0 other faults; every move annotated. The independent PGN
  recount (`colosseum_recount.py`) agrees on all five pentanomials. Each
  run's resolved configuration gives arm A its one option at 1 and arm B
  none. Run records `tools/results/b66-<run>/`; PGNs:
  - `b66-TtCap/games.pgn` `506269C0B923734649EC35BD5422CB44D04BA15D3FAA68F36048F4D497BE3255`
  - `b66-TtCap10/games.pgn` `8D8082882A97AC507E74460AC24C22BCB23202EBF15F92135EBF7AB518F97021`
  - `b66-Razor/games.pgn` `4954CE8E69E0D752B91F608B01B32FE9B7C05A7659DC3B0C23009C17863BC19E`
  - `b66-Key50/games.pgn` `6587B9D843F6BE3F17E3948730929F79633BE0D156267F37BC1A41C8592EDCD8`
  - `b66-Kings/games.pgn` `3DF828543C5E87376BF6019258A7304406453C8DA2AF0C9F736A84B223156324`

| Run | Elo (95%) | Interval | W/D/L | Pentanomial | Rule |
|---|---:|---|---|---|---|
| `TtCap` (`3+0.03`) | +8.0 ± 9.1 | [−1.1, +17.1] | 547/952/501 | 27, 223, 458, 261, 31 | not adopted: the interval includes 0 |
| `TtCap10` (`10+0.1`) | −0.5 ± 8.6 | [−9.1, +8.1] | 497/1003/500 | 23, 233, 483, 246, 15 | not adopted |
| `Razor` | **+10.6 ± 9.4** | [+1.2, +20.0] | 566/929/505 | 37, 201, 468, 252, 42 | **adopted** |
| `Key50` | −4.5 ± 8.4 | [−12.9, +3.9] | 490/994/516 | 28, 218, 525, 210, 19 | not adopted; not rejected |
| `Kings` | **+11.6 ± 9.3** | [+2.3, +20.9] | 568/931/501 | 32, 208, 454, 273, 33 | **adopted** |

- **Diagnostics (not the verdict).** Mean depth without mate scores, arm A
  against B: `TtCap` 16.20/16.17, `TtCap10` 20.06/20.05, `Razor`
  16.27/16.08, `Key50` 16.11/16.19, `Kings` 16.34/16.19. Speed within
  1% on every pair (1.88 to 1.93M nodes per second).
- **Disposition per row.**
  - TT-cutoff cap: arm A ran the donor's cap on the fitted slope,
    `min(151·d, 695)`, not the donor's whole `min(112·d, 695)`; the
    whole form is H.6's. Neither leg adopts, so Basilisk's form stays. Not
    cleared: `3+0.03` read +8.0, `10+0.1` −0.5; both readings go to H.2.
    B.6.7 fits the slope uncapped.
  - Razoring: the donor's form is **adopted**.
  - Rule-50 key: Basilisk's form stays; under +5, so H.2.
  - Kings in the non-pawn key: the donor's form is **adopted**.
  - The two adoptions go together behind one switch to the SPRT `[0,3]`
    against the head, before B.6.7 (the registered next step). If every row were null, each run would clear
    the rule by noise about 2.4% of the time (an estimate above 9.3 at
    this width), about one chance in nine that one of five would; the
    SPRT is the guard against that.
- **Calibration.** Hits: P3, P6, P8. Misses: P1 (+8.0, outside ±5), P2
  (`10+0.1` read lower, not higher), P4 and P5 (razoring read positive and
  adopted; P5 gave it 10%), P7 (kings +11.6), P9 (two adoptions). The wrong
  part of the model: the tilt toward the fitted forms was expected to
  dominate. For razoring, the reason the depth-1 form was seeded
  (a checkless quiescence) left with B.4, and the fitted neighbours did not
  hold its place. For the kings, the bare-king pooling cost more than
  B.3.0's drift reading suggested. The TT cap's long-control argument did
  not show at `10+0.1`.
