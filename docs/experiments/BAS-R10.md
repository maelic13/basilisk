# BAS-R10

**BAS-R10 — B.5.3: M1, the fail-high stop's firing read, on the arm**

- **Registration.** Created 2026-11-09 at B.5.3's opening, before any of its
  replays; maintainer-run replays, about five hours. Assembled from the stop
  packet (`analysis/b5_failhigh_stop_2026-10-25.md` §4 and §5) and the Cluster 4
  packet (`analysis/b5_cluster4_research_2026-11-08.md` §11, §A1.5, §A2.5). The
  frozen predictions and rules are copied; where a clause replaced earlier
  words, the new words are given and the source named.
- **Binaries.** One revision, the accepted implementation head; pinned in
  BAS-S53's *RESULT* and in `tools/run_b53_m1.ps1`. The arm's and the OFF arm's
  Tune builds and Diag builds, each reading its arm's `bench 13` (ON 3,920,652,
  OFF 4,144,790).
- **Research question.** On the arm, is the fail-high stop at the optimum a sink
  guard (H1 of the stop packet) or a general time cut (H2), and what does its
  threshold have to be?
- **Interactions.** The stop and the hole are opposite problems on the arm
  (§A2.4 b): the stop cuts ordinary searches short, and the hole runs decided ones
  long. With narrower windows more fail highs land on the previous best (§5.4).
  The time manager's effort measure moves with the windows (§A2.4 e).

#### INSTRUMENT

`tools/diag/replay_games.py`, each game replayed in one process, `ucinewgame`
once, the game's own clocks, one thread, Hash 64 (§A1.5 replaces §11's "Diag
build" and the stop packet's PGO build):

| Column | Build | Option |
|---|---|---|
| the arm at 1× | the arm's Tune | `B5FailHighStop` 100 |
| the arm at 2× | the arm's Tune | `B5FailHighStop` 200 |
| the arm, stop removed | the arm's Tune | `B5FailHighStop` 0 |
| the OFF arm's reference | the OFF arm's Tune | `B5FailHighStop` 100 |
| Diag, the arm | the arm's Diag | the default stop |
| Diag, the OFF arm | the OFF arm's Diag | the default stop |

Read by `tools/diag/replay_read.py`.

- **Corpus.** The first 300 games in file order of the B.4.7 gate (a) PGN at
  `3+0.03` (`tools/results/b47-gate-a/games.pgn`, SHA-256 `18aea0cd…`), candidate
  side, and the first 100 of its `10+0.1` direction run
  (`tools/results/b47-direction-ltc/games.pgn`, `6931997c…`), candidate side.

#### OUTPUTS

- The firing rate per search, by score band (won at 300 or more, even, lost).
- The time removed, as a share of spend, by score band.
- **The hole (§A2.5).** A root search that ends after twice the optimum, or at the
  maximum, without the stop firing; split by whether it began before the optimum
  or after it; reported for every stop arm and for the OFF arm's reference column,
  by score band, with the time it took past the optimum.
- The same for the 2× arm.
- Added by §A2.5, same replays: root best-move changes a completed iteration; from
  the Diag replays, the shares of iterations at or over `TmEffortHi` and at or
  under `TmEffortLo` on both arms.
- The ply-cap counters per game (P9), from the Diag replays.

**How the time removed is read.** The sum over the 1× (or 2×) column's positions
of the stop-removed column's elapsed less its own, over the stop-removed column's
spend. That is the method behind §A2.4 b's 10.1% and 6.6% and A6. A position's
band is the stop-removed row's. The stop packet's §4 also describes a paired fresh
replay of the fired positions, the median of three runs; §A2.5 and this leaf's
list do not carry it and it is not run. A fresh replay has no game table, so it is
not the same search.

#### THE RULE

Stop packet §5 as §A2.5 amends it:

- **Keep at 1×** when 1× fires on under 10% of searches and removes under 3% of
  spend. No gate and no game run are owed.
- **Move to 2×** when 1× fires on at least 10% of searches or removes at least 3%
  of spend. Gate: 2× against 1×, SPRT `[0,3]` at `3+0.03` (BAS-S55 (c)), with the
  differential battery gate below.
- **Remove** only if the battery passes with no stop on the arm, then `[−5,5]` at
  `3+0.03`; remove on H1, keep on H0.
- **The gate of a stop option (§A2.5; replaces the stop packet's battery clause
  for the arm).** On BAS-S53's battery screen an option (2×, or removal) passes
  when, against the 1× column of the same binary, its searches over 0.5× the
  maximum are no more than 2 above the larger of the two 1× runs and its spend no
  more than 10% above theirs, in each stratum at `60000+600`. Keeping 1× has no
  gate. The tolerance is R2's (§A2.3): runs at one setting differ by one search and
  4.1% of spend at most. A search over the bound at 1× is the hole's and gates no
  option.
- **The hole's card.** Under 0.5% of searches with no time loss, `NO_CHANGE`.
  Above that a research card, D.1's, read on the arm's rate with the OFF arm's
  beside it. §A2.4 c: D.1's clock test inside an iteration is pulled forward only
  if the flip's long-clock condition fails with `B5NegExtMax` 1 not available, or
  M1 reads the hole on more than 0.5% of the arm's searches **with** unconverted
  games to show for it.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Frozen 2026-11-08 and 2026-11-09 (packet §11, §A2.6); seen before freezing, for
A6 to A8: the review's 60 positions, R1, R2 and twenty replayed games.

| # | Prediction | Confidence | Falsifier |
|---|---|---|---|
| P8 | M1 on the arm at `3+0.03`: the stop fires on **1 to 6%** of searches (the stop packet registered 5 to 20% on the old head; the mean-square width opens won-tier windows), removes **≤ 2%** of spend; the hole on **≤ 0.3%** of searches; 2× fires on ≤ 1% | low | outside the ranges the stop packet's §5 rule decides as registered; nothing here changes it |
| P9 | The ply cap in games: **≤ 0.05** cap returns per game at `3+0.03`, ≤ 1 per game at `60+0.6`, with the in-check share ≤ 30% | low | a rate above 1 per game at `3+0.03` makes the raise a Phase C item with the bands; none here |
| P11 | Time: the share of iterations whose best effort reaches `TmEffortHi` rises by **≤ 10 points** on the arm | low-moderate | above 10 points is recorded for D.1 and the direction run (d) is read beside it |
| A6 | M1 on the arm at `3+0.03`: the stop ends **18 to 30%** of searches and removes **7 to 13%** of spend; the OFF arm's column 8 to 14% and 4 to 8% | moderate (twenty of its games are seen) | outside the ranges, the twenty games misled, and the first hundred are read against the rest |
| A7 | The registered rule sends the arm to 2× against 1× (85%); both options pass the differential gate (85%) | moderate | an option failing the gate is a stop that still guards something on the arm |
| A8 | The hole on the arm at `10+0.1`: **2 to 8%** of won-band searches, at least a third beginning after the optimum; the OFF arm 0.5 to 3%; over all searches the arm is above 0.5% (60%) | low | under 1% of won-band searches says the battery overstates what games meet |

The stop packet's own §4 prediction (1× fires on 5 to 20%, at least half in
positions at 300 or more, removes 2 to 6%; 2× under 2%; the hole under 0.5%) is
on the old head and is kept there.

**Calibration owed, at this entry's result (§A2.6).** P8 is already contradicted:
the stop ends 25.1% of searches on twenty games, with 10.1% of spend removed
(§A2.4 b says why: about twice as many root fail highs a search). P11's high share
holds at fixed depth (8.8% and 8.9%), and its low share, unpredicted, rises 12
points. §5.2's mechanism for the effort share was wrong (§A2.4 e).

- **A column the frozen table reads and the list does not hold.** A6's "4 to 8%"
  for the OFF arm's time removed needs the OFF arm at stop 0, which §11, §A2.5 and
  this leaf's list do not name; the review's twenty games had it. The launcher's
  `-OffStop0` plays it when the maintainer agrees; without it that clause is
  scored "not read".

#### RESULT — append after exposure

- **Dry run** (2026-11-09, `tools/results/b53-m1-dry/`, three games a column, seven
  columns with `-OffStop0`, both corpora; `b5-20261109/b53/dry-m1.log`): exit 0, the
  reader's rule reached its report. Three games read: the gate corpus's arm at 1× ended
  24.72% of searches with 8.13% of spend removed, the OFF arm 12.36% and 8.67%, rule "2x
  against 1x". These are three games and score nothing.
- **Guards** (`b5-20261109/b53/guard_test.ps1`, `guard_test.txt`): 16 launcher cases and
  the reader's rule on synthetic results of both signs, 0 failures. `-CheckOnly` passes
  on the real pins.
- **The gate of a stop option is read** (BAS-S53 *RESULT*): stop 200 passes; removal
  fails on W's spend, so the Remove branch is closed. The rule and the hole remain
  M1's.
- **`Diag` runs in every column.** `replay_games.py` turns `Diag` on, as it did for the
  review's twenty games, because the firing rate needs `fhstop_fired`. BAS-S53's anchor
  measured what that costs a search: about 8% slower, which moves where the clock stops
  it. The registered instrument is kept, and it matches A6's source. But M1's firing
  rate and time removed are of a search about 8% slower than the one that played, and
  are read with that stated.
- **The replays** (2026-11-09, maintainer-run with `-OffStop0`, seven columns on both
  corpora, every exit 0; `tools/results/b53-m1/gate/` and `direction/`, `m1.json` and
  `m1.txt` in each).

  | Gate corpus, `3+0.03`, 300 games, 19,205 searches | ended by the stop | spend removed | hole | over half the maximum |
  |---|---:|---:|---:|---:|
  | the arm at 1× | 23.42% | 9.89% | 6.28% | 693 |
  | the arm at 2× | 2.86% | 1.53% | 8.95% | 802 |
  | the arm, stop removed | 0 | | 11.96% | 947 |
  | the OFF arm at 1× | 9.70% | 5.40% | 7.63% | 751 |
  | the OFF arm, stop removed | 0 | | 11.24% | 1,048 |

  By band at 1× (fired; hole; removed of the band's spend): the arm won 5.46%, 13.48%,
  5.84%; even 32.20%, 2.93%, 11.81%; lost 3.81%, 13.33%, 0.88%. The OFF arm won 20.92%,
  3.45%, 17.66%; even 7.25%, 5.99%, 2.38%; lost 6.17%, 22.06%, 6.03%. So the arm's stop
  cuts even positions and the OFF arm's cuts won ones; the arm's hole is in won
  positions and the OFF arm's in lost ones. The first 100 games and the other 200 agree:
  the arm 24.48% and 22.90% fired, 10.70% and 9.49% removed.

  The direction corpus at `10+0.1`, 100 games, 6,524 searches: the arm at 1× 18.27%
  fired, 7.71% removed, hole 4.80% (won band 10.90%: 37 before the optimum, 94 after, 13
  other); the OFF arm 7.31%, 3.54%, 4.63% (won band 2.21%).

  The Diag replays (gate corpus): the shares at or over `TmEffortHi` 23.35% on the arm and
  24.37% on the OFF arm; at or under `TmEffortLo` 32.03% and 20.96%. Root best-move
  changes a completed iteration 0.0973 and 0.0945. Ply-cap returns 17 on the arm, all in
  game 33, none in check; 0 on the OFF arm.

- **The rule: 2× against 1×** (1× fires on 23.42%, at least 10%). The battery's gate is
  passed (BAS-S53 *RESULT*), so what is owed is BAS-S55 (c)'s SPRT `[0,3]` at `3+0.03`,
  in B.5.5.
- **The hole's card: a research card for D.1.** The arm's hole is 6.28% of searches,
  above 0.5%, with the OFF arm's 7.63% beside it. §A2.4 c's pull-forward does not apply:
  the whole-arm run leaves the arm fewer unconverted games at 300 than the OFF arm (38
  against 48; BAS-S53 *RESULT*).
- **Scored as frozen:**

  | # | Read | Verdict |
  |---|---|---|
  | P8 | 23.42% fired against 1 to 6%; 9.89% removed against at most 2%; hole 6.28% against at most 0.3%; 2× fires 2.86% against at most 1% | **miss** on every clause, as §A2.6 said; the rule decides as registered |
  | P9 | 0.057 ply-cap returns a game at `3+0.03` against at most 0.05, none in check | **miss** by one game's 17 returns; the falsifier (above 1 a game) does not fire; `60+0.6` is not read here |
  | P11 | the high-effort share −1.0 point on the arm | **hit**; the low share rises 11.1 points, unpredicted, as §A2.4 e says |
  | A6 | the arm 23.42% and 9.89%; the OFF arm 9.70% and 5.40% | **hit** on all four; the hundred and the rest agree |
  | A7 | the rule sends the arm to 2× against 1× | **hit**; the second clause missed at the battery (removal fails the gate) |
  | A8 | the arm's won-band hole 10.90% at `10+0.1`, 65% of it after the optimum; the OFF arm 2.21%; the arm 4.80% over all searches | **miss** on the first clause, high; **hit** on the other three |

- **The gate's place (2026-11-10, maintainer):** 2× against 1× is played before B.5.4's
  fit, on the seeds, as BAS-S55 (c)'s stop bracket, since the stop's rate moves with the
  window the fit moves (BAS-S54 *AUDIT READ*). The rule, the bracket and the battery's
  gate are unchanged.
- **Not in the predictions:** the OFF arm hit the ply cap 384 times in one game of the
  direction corpus (game 96), 3.84 a game; it is the OFF arm's, outside P9 and this
  leaf, and is reported to the maintainer.

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Before any game:** P8 missed every clause (the stop ended 23.42% of searches
  against 1 to 6%, removed 9.89% of spend against at most 2%, and left a 6.28% hole
  against at most 0.3%). P9 missed by one game's 17 ply-cap returns. P11 hit.
- **After twenty games were seen:** A6 hit on all four ranges and A7's rule hit; A7's
  gate clause missed at the battery. A8 missed high on the won band (10.90% against 2 to
  8%) and hit on its other three clauses.
- **What it teaches:** the stop's rate was sized from twenty replayed games and not
  from the mechanism's argument, which put it at a quarter of what it is. The hole
  sits in won positions on the arm, larger than predicted, which is why D.1 owes it a
  research card.
