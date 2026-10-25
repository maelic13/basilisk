# B.5 — the fail-high clock stop: keep, move or remove

- State / class: `RESEARCH`, the investigation for B.5's fail-high stop
  item; `R2`. Its first measurement is M1 below, a `V` read.
- Owner / date: agent, 2026-10-25, at maintainer request. Evidence BAS-R07;
  raw runs in `tools/results/fhstop-20261025/` (README there gives each
  binary's recipe, bench and hash).
- Inputs: A.8.16's packet (`analysis/a816_time_sink_2026-10-07.md`), the
  2026-10-24 stall probe (`tools/results/stall-probe-20261024/`), the Rarog
  audit's items A-47, A-48 and B-79 to B-82
  (`analysis/rarog_audit_2026-10-24.md`). Rarog's numbers are leads only:
  RAR-R18 (the stop at the target, about −15 Elo there) and RAR-R25 (at twice
  the target it fired on 2 of 300 searches) reach this packet as the
  maintainer relayed them, not from a pinned snapshot.
- Decision needed: none now. M1 runs when B.5 opens; the decision rule below
  is fixed before it runs.

## 1. The mechanism under test

`Searcher::search`, the aspiration loop (`src/search_worker.cpp`, the
`score >= asp_b` branch). At a fail-high, the main thread under time
management ends the search when the fail-high move is the last completed
iteration's move and `elapsed_seconds() >= soft_limit` (the unscaled
optimum). The iteration is discarded; the move played is the last completed
iteration's, which is the same move. The condition is evaluated only when a
window returns failing high. Nothing checks it during a window, after a fail
low, or in the give-up window (`delta >= 900`, which searches
`(-INF, INF)` and breaks unconditionally).

## 2. What Basilisk's own probes show (BAS-R07)

All on clang 23.1.3 `release-pext` builds without PGO. Each arm reproduces
its revision's bench, so each is the named tree with only the stated change.
Rec1 is `7r/5R2/8/2k1PB2/8/4K3/8/8 w - - 0 86` (KRBP v KR).

### 2.1 Lead 1: the cascade was not removed; B.10 lets rec1 prove mate

Rec1 at `60000+600` (optimum 4,700 ms, maximum 32,304 ms), stop removed:

| Tree | Revision | Bench | Time | Last line |
|---|---|---:|---:|---|
| A.8 head (1.10.2) | `fac5064` | 14,978,465 | **32,305 ms (max)** | d28 `cp 2421 lowerbound`, 6 bound lines |
| B.2 head | `71a612b` | 11,099,210 | **32,305 ms (max)** | d27 `cp 2241 lowerbound`, 2 bound lines |
| B.10 head (current) | `f4ebe79` engine | 8,437,148 | 16,720 ms | d26 `mate 25`, 9 bound lines |

The A.8 arm reproduces A.8.16's sink exactly (fail-highs at the same scores,
+1185 … +2421). B.2's selectivity core did not remove it. The current head
still cascades: nine fail-highs at depth 26 between 3.0 and 10.1 s
(`cp 1840` to `cp 3209`). Then the give-up full window runs from 10.1 to
16.7 s and proves `mate 25`. **The change that ends rec1's sink is B.10's PST
repair (`f4ebe79`), and it ends it by giving the give-up window a mate to
prove. The cascade mechanism is untouched.** The premise "the stop may no
longer be needed" rests on one position whose cascade happens to end in a
mate proof. It does not hold for won endings in general.

### 2.2 Lead 4: the tree grows as Rarog's does, but tree growth does not cause the sink

The deeper re-searches. The reduced depth is
`max(1, min(new_depth − r/1024, new_depth + 2)) + is_pv`, so a reduced child
can go up to 2 plies beyond full depth, and 3 at PV nodes. The correction
term `r -= min(|correction_raw| / 7419, 2695)` removes at most 2.63 plies of
reduction. Diagnostic counters at depth 13 over `suite_v1.epd` (106 legal
positions): **`lmr_extended` 518,627 of 4,758,381 LMR-path searches
(10.90%)**, against Rarog's 10.5%. The cap does not make this rarer.

Fixed depth, the head against two single-change arms (stop present; nodes to
complete the depth):

| Position, depth | Head | No correction term | No negative reduction |
|---|---:|---:|---:|
| rec1, 26 | 72,862,676 (`mate 25`) | 2,405,489 (`cp 938`) | 3,212,764 (`cp 942`) |
| rec3, 28 | 14,930,972 | 5,763,163 | 14,384,263 |
| KRB v K, 32 | 14,883,816 | 3,120,099 | 10,463,052 |
| KR v K, 34 | 30,810,116 | 1,931,778 | 8,077,502 |
| KBN v K, 34 | 23,878,347 | 2,894,227 | 5,591,896 |
| bench 13 | 8,437,148 | 6,320,974 (−25%) | 6,661,751 (−21%) |

In won endings the extended share rises to 12–39% (KBN v K 39.3%). B-80 and
B-82 are present in Basilisk at Rarog's scale: correction-driven negative
reductions inflate won-ending trees 5–30×. Whether that costs or earns Elo
is not known. BAS-S21 read `CoreLmrNegativeAllowed 0` at −5.6 ± 9.8.

**But removing the growth does not remove the sink.** Rec1 at `60000+600`
with the stop removed: the no-correction arm reaches depth 30 and runs to the
maximum (`cp 2266 lowerbound`); the no-negative-reduction arm reaches depth
38 and runs to the maximum (`cp 2349 lowerbound`). The cascade only moves to
another depth. The sink is the aspiration loop's: the score jumps when an
iteration first sees the conversion, and windows that rise by `delta` cannot
catch it before the maximum. On Basilisk, tree growth is a separate
question, not the sink's cause.

### 2.3 Lead 2: a window that never returns is the stop's hole

Rec1 with the stop present (diagnostic builds; `fired` is `asp_fail_high +
asp_fail_low + asp_giveup − asp_researches`):

| Tree | `60000+600` | `180000+2000` (optimum 15,189, maximum 104,396 ms) |
|---|---|---|
| head | 4,894 ms, fired | 17,825 ms (1.17× opt), not fired: give-up window 11.1 → 17.8 s |
| no correction term | 6,479 ms, fired | **42,393 ms (2.79× opt)**, not fired: give-up window 14.3 → 42.4 s |
| no negative reduction | 5,238 ms, fired | **61,875 ms (4.07× opt, 0.59× max)**, not fired: give-up window 14.5 → 61.9 s |

In both arms the give-up window began just before the optimum, and the
condition never came up again. A window that starts before the optimum and
does not return before the maximum is a sink the stop cannot reach. The
stop-free timelines show the same for an ordinary window: on the B.2 tree the
window after the 6.0 s fail-high was still running at 32.3 s.

### 2.4 The threshold at twice the target, derived from the same timelines

The stop acts only at a fail-high return, and the search before that return is
the same with or without it. A threshold of 2× the optimum (9,400 ms at
`60000+600`) therefore fires at the first fail-high after 9.4 s on each
stop-free timeline:
- A.8 tree at 10.8 s;
- current head at 10.1 s;
- no-correction arm at 11.6 s;
- no-negative-reduction arm at 12.8 s;
- **the B.2 tree never**: its last fail-high is at 6.0 s, and it sinks to the
  maximum.

1× guards all five trees at this clock; 2× guards four. The cost of the
higher threshold is guard coverage, paid exactly on the trees B.3 to B.6
will produce.

### 2.5 Games: where long moves come from today

On A.9.2's `10+0.1` run (head against 1.10.2, 1,000 games), the head spent
at least 0.7× the clock left on 98 of 66,390 moves (0.15%; 1.10.2 on 731).
Those moves had a median of 564 ms on the clock, where the maximum is about
4.1× the optimum. Scores at those moves:
- lost (−300 or worse) 42;
- even or drawn 32;
- won 14;
- mate 10.

Replaying the 98 positions fresh at their own clocks, the stop fired on 17. Of
the 11 replays that reached the maximum, 11 had fail-lows and one a give-up.
The replays are fresh processes with no game TT, so this sample is not a
firing rate. It says that today's in-game maximum hits are sub-second
fail-low searches: A-45's territory (D.1), not the stop's.

## 3. Hypotheses

- **H1 (the stop is a sink guard).** It fires mainly in won positions with a
  rising score, on a few percent of searches, and removes clock that would
  have bought nothing. BAS-S20 (−0.7 ± 4.5) is its whole cost.
- **H2 (the stop is a general time cut, Rarog's finding).** It fires on a
  large share of ordinary searches whose iteration ran past the optimum. The
  cut iteration would often have completed and changed the move's depth or
  score, so the stop costs Elo the way Rarog's did (about −15). BAS-S20's
  interval (−8.7 to +6.5 nElo) does not exclude a cost of this size at
  `3+0.03`, and it measured the stop against no stop on the A.8 tree.
- **H3 (the stop is no longer needed).** Refuted as stated by §2.1 and §2.2:
  the sink returns on three of the five trees probed, and on the head it is
  absent from rec1 only through a mate proof. H3 can only return as an
  empirical claim about the B.5 head. That claim needs §5's battery, not one
  position.

## 4. M1 — the firing-rate read (registered here, before any exposure)

**Instrument.** A `RELEASE_DIAG_COUNTERS` PGO build of the B.5 entry head
that prints one line per search: `fired`, elapsed, optimum, maximum,
`asp_fail_high`, `asp_fail_low`, `asp_giveup`, the score, and the start and
end of its longest window. Its NPS cost against the plain PGO build is read
first and must be at most 2%; otherwise the measured times are not the
game's.

**Corpus.** 300 openings from the standard book at `3+0.03` and 100 at
`10+0.1`, 1 thread. The candidate side of a gate run on the B.5 entry head is
used if one exists, otherwise a fresh self-play run. Each game is replayed
sequentially in one process: `ucinewgame` once, then `position startpos moves
…` and `go` with that side's PGN clocks. The TT and histories then carry
across moves as in the game.

**Arms on the same corpus.** Stop at 1× (as shipped), stop at 2×, stop
removed. The time each firing removes is read on the fired positions by a
paired fresh replay of the stop-removed arm: the median of 3 runs, minus the
stop's elapsed.

**Outputs.**
- The firing rate per search, by score band (won ≥ 300, even, lost).
- The time removed, as a share of total spend.
- The rate of the §2.3 hole: a window starting before the optimum and ending
  after 2× the optimum, or at the maximum, without the stop firing.
- The same three numbers for the 2× arm.

**Frozen prediction (2026-10-25, before M1 runs).**
- At `3+0.03`, 1× fires on **5–20%** of searches (central 10%). At least
  half of the firings are in positions scoring ≥ 300. It removes **2–6%** of
  total spend.
- 2× fires on under 2%.
- The hole occurs on under 0.5% of searches.

Confidence low. Basilisk's end-of-iteration stop shrinks to 0.64× the
optimum on a stable move, so fewer iterations run past the optimum than in
Rarog. Against that, the 98-position long-move replay fired 17 times.
Falsified if the rate is outside 5–20%, or if fewer than half of the firings
are in won positions.

## 5. The decision rule and each option's falsifier

**The sink battery**, run at `60000+600` and `180000+2000` on whatever
arm is proposed:
- rec1–rec3 and the three bare-king endings;
- the two tree variants of §2.2, as stand-ins for B.3–B.6's trees;
- at least 20 won positions (≥ 300) drawn from M1's corpus.

It passes when no search exceeds 0.5× the maximum.

- **Keep at 1×** (the default unless M1 says otherwise). Chosen when 1×
  fires on under 10% of searches and removes under 3% of spend. The stop is
  then a sink guard (H1), its cost is bounded by BAS-S20, and no game run is
  owed.
  *Falsifier:* M1 at or above either threshold, or a 2× SPRT passing H1. Then
  the cost is real and the threshold moves.
- **Move to 2×.** Chosen when 1× fires on at least 10% of searches or removes
  at least 3% of spend (H2). Gate: 2× against 1×, SPRT `[0,3]` at `3+0.03`,
  plus the battery passing at 2×.
  *Falsifier:* the battery failing at 2× on any position (§2.4 shows the B.2
  tree would), or the SPRT reading H0. A battery failure at 2× is not
  repaired by lowering the threshold back. The hole is §2.3's, and a
  threshold change does not reach it. It goes to the card below.
- **Remove (a removal bracket).** Chosen only when the battery passes with
  no stop at all on the B.5 head, including the two tree variants. Gate: the
  stop-free arm against the head, `[−5, 5]` at `3+0.03`; remove on H1, keep
  on H0.
  *Falsifier:* any battery search past 0.5× the maximum without the stop
  (today: three of five trees, §2.1–2.2), or the bracket reading H0. On
  today's evidence removal is refused.

**The hole is a separate card**, owned with D.1 (A-47): the give-up window
and a window that never returns. Its rate comes from M1. Under 0.5% of
searches, with no time loss: `NO_CHANGE`. Above that, it is a research card;
a threshold change cannot repair it.

## 6. Interactions and invariants

- The end-of-iteration soft stop (stability, score-drop, effort and
  instability scales) acts between iterations. The fail-high stop acts inside
  one. The time a firing saves goes to later moves through the clock;
  nothing else reads it.
- The aspiration width and growth (`AspirationDelta` 19, `delta += delta/2`,
  give-up at 900) set how many fail-highs a cascade has before the give-up.
  Any B.5 window change (B-71, B-72, B-75) changes the stop's firing rate.
  M1 is re-read after such a change before its gate.
- LMR's `delta·577/root_delta_` term (B-73) couples the window to tree shape.
  §2.2's line-growth terms change where the cascade lands, not whether.
- The stop changes no move when it fires; that invariant is A.8.17's engine
  test and the battery's move check.
- Helpers, pondering and MultiPV are unchanged (A.8.16's interactions hold).

## 7. What failed in Rarog, and why none of it is proposed here

- A known-win evaluation offset alone harmed at `10+0.1` (−25 nElo). On
  Basilisk the sink survives a 30× smaller tree (§2.2), so an evaluation
  offset that changes where the score jumps would not remove it either.
- A squared aspiration window is a global change to every search to fix one
  ending class. B-71 and B-72 say windows are decided by games only.
- The four-part cascade cluster read H0 (−7.4). None of its parts is proposed
  here.

## 8. Findings outside this item, not acted on

- B-80 and B-82 are verified present (§2.2) and are B.5's line-growth item:
  a 21–25% bench reduction is available from either term. Their Elo is
  unmeasured, and BAS-S21's −5.6 ± 9.8 is the only game read.
- `tools/diag/suite_v1.epd` holds an illegal position,
  `1r6/8/8/8/8/8/1K6/kR6 w - - 0 1` (adjacent kings, the side not to move in
  check). The engine rejects it with exit status 1. `run_suite.py` then
  failed on the diagnostic build with "missing diagnostic counters"; the
  cause was not investigated.
