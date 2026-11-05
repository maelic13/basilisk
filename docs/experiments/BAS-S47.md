# BAS-S47

**BAS-S47 — what pruning and reductions hide: each rule's decisions searched
again as the rule denied them**

- Date / owner / calibration category: registered 2026-11-05, agent;
  measurement instrument and its first reading (PLAN B.16.2, Triumviratus
  TM-9).
- Baseline source revision / candidate source revision / dirty-diff identity:
  the instrument `f2d8116` (engine) and `9bac833` (reader), both clean on
  `b16-instruments`; the engine's tree is `master`'s, bench 5,361,058.
- Binary / compiler / flags / PGO identity and hashes: Clang 23.1.4
  (MSYS2), the `release` preset without PGO, reporting
  `Basilisk dev-20261105-9bac8334`, `bench 13` 5,361,058 on both:
  - diagnostic (`-DDIAGNOSTIC=ON`),
    `tools/results/b162-pricing/bin/basilisk-b162-diag.exe`, SHA-256
    `5C1DEA87CE56223C3BA4D1DDB21A264BF11A6CB1B46663455470CD50C610F5E1`;
  - tune (`-DTUNE=ON`, for the planted rules),
    `tools/results/b162-pricing/bin/basilisk-b162-tune.exe`, SHA-256
    `2032B1C65333193FF2772DAF7D7CF53F6B4C92B25ACA43270D660DC120F9BA7E`.
- Research question: which of the search's pruning rules and reductions
  hide moves that matter, and how often? Basilisk's decision trace records
  each decision but never prices it.
- Hypothesis / proposed mechanism: none to accept. The reading screens:
  it points B.4 (quiescence pruning), B.5 (line growth), B.3.4's form
  re-reads and later fits at the rules worth a game. It never accepts.
- Interacting mechanisms / consumers: every rule priced reads the same
  transposition table and histories; the probes read them and write
  neither.

#### Instrument

`Pricing N` (tune and diagnostic builds, `src/search_pricing.h`) counts
each rule's decisions in the engine's own tree by remaining depth, node
type (PV, expected cut, expected all, null-window quiescence) and move
number, and searches every N-th one of each rule again as the rule denied
it:

- node rules (razoring, reverse futility, the null move, ProbCut, the
  table-served ProbCut, the singular multi-cut, quiescence's delta rule):
  the node from its entry without its node-level shortcuts, at its own
  depth and window. It **beats** the rule when it lands on the other side
  of the bound the rule's value claimed.
- move rules (capture futility and SEE, continuation-history pruning,
  quiet futility and SEE, the move-count rule's first denied quiet,
  quiescence's futility, SEE and late-capture rules): the move at the
  node's full depth with the node's window. It beats the rule when it
  exceeds alpha, and **changes** the node's best move when it also exceeds
  the node's final best score.
- reductions: a reduced search that failed low, searched at the move's
  full depth with the node's window; beats and changes as for a move.

A probe is isolated: it reads the table and the histories, writes neither
(its stores go to a private 16 MiB table only probes read), and its nodes,
counters and stack records are restored. A search with pricing on is
therefore the same search as with it off, which the reader checks
(`python tools/diag/pricing.py isolation`). Each probe reads the table as
the engine left it at the decision; the razoring and ProbCut probes run
before those rules' own stores at the node, the null move's after its
verification (a shallower entry than the node's).

Not priced: IIR, the hindsight depth changes, the three-ply reduction after
alpha rises, the first move's reduction step and the negative singular
extension; the Cluster 2 OFF arm's null move and ProbCut.

Validation before registration (2026-11-05, the uncommitted equivalent
of `f2d8116`):

- `bench 13` 5,361,058 on the release, diagnostic and tune builds;
  CTest 24/24 (`-LE strength`).
- Isolation: suite_v2's first 20 positions at depth 10, `Pricing 4`,
  179,335 probes: every iteration line (depth, score, nodes, PV), the
  bestmove and every search counter identical to `Pricing 0`.
- The check is live: a throwaway build whose probes wrote the quiet
  histories differed on all 20 positions.

#### PRE-REGISTERED PREDICTION — freeze before exposure

Exposure before freezing: during development, one position (an Italian
opening, depth 12, `Pricing 8`) showed reverse futility at depth 1 at
expected cut nodes beating 17 of 1,074 probes, and quiet futility cells
with 0 to 2 beats in 5 to 61 probes. A three-position timing pilot at depth
12 was run for its node totals only (167,224 search, 167,263 probe nodes);
its table was not read.

Three registered runs, all `Hash 64`, one fresh process per position,
`go depth`:

1. **Isolation, whole suite**: the diagnostic binary, suite_v2's 106
   positions, depth 12, `Pricing 4`. Rule: zero positions differ.
   Probability 0.95.
2. **Planted rules**: the tune binary, suite_v2's first 40 positions,
   depth 12, `Pricing 4`, three arms: the defaults; quiet futility planted
   loose (`CoreQuietFutBase 80`, `CoreQuietFutSlope 34`, the tune ranges'
   minimums, against 166 and 107); reverse futility planted loose
   (`CoreRfpPerPly 40` against 101). Rule: each planted rule's beat share
   lies above its honest share with disjoint 95% Wilson intervals
   (`pricing.py compare`). Expected: quiet futility 1–4% honest, 4–12%
   planted; reverse futility 0.5–3% honest, 3–10% planted. Probability
   0.85 for each.
3. **The reading**: the diagnostic binary, suite_v2's 106 positions, depth
   16, `Pricing 4`. Expected beat shares:
   - reduced fail-lows 1–4% (Triumviratus read 1.7%), higher at depth 8
     and above than at 1–3 (probability 0.6);
   - reverse futility 0.5–3%, the null move 1–5%, ProbCut and its table
     form 2–10%, the multi-cut 2–10%, razoring 3–15%, quiescence's delta
     rule under 1%;
   - quiet futility 0.5–4%, quiet SEE 2–10%, continuation-history pruning
     1–5%, the move-count rule 1–5%, capture futility 2–8%, capture SEE
     3–12%; quiescence's futility under 1%, SEE 1–6%, late captures 2–8%;
   - for the move rules, a change share at least 60% of the beat share
     (most of their decisions are at null-window nodes that fail low);
   - the three rules with the most estimated hidden changes per million
     search nodes drawn from reduced fail-lows, quiet futility, the
     move-count rule and capture SEE (probability 0.6).
   Confidence: low on the levels (no prior reading in Basilisk), moderate
   on the order.

- Most likely failure mode: an isolation difference in run 1 (a state
  the probe writes and the wrapper misses), which would void the reading
  until repaired.
- Falsification criteria: run 1 or run 2 failing its rule voids the
  instrument; the reading itself is an observation.
- Registered screening rule (the reading's use): the reading names (a) the
  three rules with the most estimated hidden changes per million search
  nodes (decisions × change share ÷ search nodes; for node rules, beats in
  place of changes) and (b) every rule whose beat share's lower 95% bound
  exceeds 5%. Those are the rules the next leaves price in games first; no
  rule changes here.
- Full conditions / provenance: `tools/diag/suite_v2.epd`, archives
  `tools/results/b162-pricing/{isolation,honest,planted-qfut,planted-rfp,reading}/`.

#### AMENDMENT 1 — registered 2026-11-05, after run 2

Run 1 passed (106 positions, 2,490,852 probes, none differ). Run 2's
reverse-futility arm passed (2.57% [2.48, 2.67] honest, 3.19% [3.09, 3.30]
planted). Its quiet-futility arm **failed its rule, in the wrong
direction**: 1.42% [1.36, 1.47] honest, 0.78% [0.73, 0.83] planted, lower
in every depth band, node type and move bucket.

The plant was not a loosening, a design error made at registration. The
futility value is `eval + base + slope × lmr_depth`, and `lmr_depth`
(reduced depth plus history / about 3,000) is negative for quiets with a
poor history. The planted margin 80 + 34 L lies above the honest 166 + 107 L
for every L ≤ −2, so the plant tightened the rule for poorly rated quiets
and loosened it only for well-rated ones; its decisions per search node
fell (0.339 against 0.360). Lowering the reverse-futility margin's slope
lowered it at every depth, which is why that arm was a valid plant.

Exposure before this amendment: runs 1 and 2 above, read in full (every
quiet-futility and reverse-futility cell of the three arms); the reading
(run 3) not run.

Amended check (2a): the tune binary, the same 40 positions, depth 12,
`Pricing 4`, quiet futility planted with `CoreQuietFutBase 80` alone (slope
107 kept): the futility value falls by 86 at every `lmr_depth`, so the rule
prunes everything it pruned before and more. Rule: the planted beat share
above the honest arm's 1.42% [1.36, 1.47] with disjoint 95% intervals.
Probability 0.8. If it fails, the instrument is not trusted and the leaf
returns to research with this record. Run 3 waits for it. Archive
`tools/results/b162-pricing/planted-qfut-base/`.

#### RESULT — append after exposure

Runs 2026-11-05 by the agent, binaries as registered.

- Run 1, isolation: **passed**. 106 positions at depth 12, `Pricing 4`,
  2,490,852 probes, no position differs
  (`tools/results/b162-pricing/isolation/output.txt`).
- Run 2, planted rules: reverse futility **passed** (2.57% [2.48, 2.67]
  honest, 3.19% [3.09, 3.30] planted); quiet futility **failed** as
  registered, a plant that tightened the rule (Amendment 1); the amended
  check 2a **passed**: 1.42% [1.36, 1.47] honest, 1.91% [1.85, 1.97]
  planted, decisions 743,042 → 799,194.
- Run 3, the reading: 106 positions at depth 16, `Pricing 4`; 31,000,309
  search and 74,809,408 probe nodes; `runs.jsonl` SHA-256 `1B75647A…`,
  `summary.json` `A9985964…`
  (`python tools/diag/pricing.py report tools/results/b162-pricing/reading`
  reproduces it). Beat share [95% interval], change share, estimated
  hidden changes per million search nodes:

| Rule | Decisions | Beat % | Change % | Hidden / Mn |
|---|---:|---:|---:|---:|
| razor | 598,718 | 12.44 [12.27, 12.61] | – | 2,402.7 |
| rfp | 4,841,269 | 2.70 [2.67, 2.73] | – | 4,210.9 |
| null_move | 807,589 | 0.41 [0.38, 0.44] | – | 106.7 |
| probcut | 915,109 | 1.49 [1.44, 1.54] | – | 438.9 |
| multicut | 46,716 | 0.70 [0.57, 0.87] | – | 10.6 |
| qs_delta | 491,043 | 3.08 [2.99, 3.18] | – | 488.0 |
| capture_futility | 2,133,690 | 2.08 [2.05, 2.12] | 1.97 | 1,356.6 |
| capture_see | 2,318,738 | 0.62 [0.60, 0.64] | 0.59 | 444.2 |
| cont_hist | 401,518 | 4.96 [4.83, 5.10] | 4.64 | 600.7 |
| quiet_futility | 11,189,650 | 1.01 [1.00, 1.02] | 0.97 | 3,504.5 |
| quiet_see | 2,784,011 | 0.46 [0.45, 0.48] | 0.41 | 369.7 |
| skip_quiets | 2,553,548 | 2.35 [2.32, 2.39] | 2.07 | 1,701.6 |
| qs_futility | 3,783,878 | 4.27 [4.23, 4.31] | 4.24 | 5,174.1 |
| qs_see | 4,650,381 | 1.53 [1.51, 1.55] | 1.35 | 2,028.7 |
| qs_late | 1,019 | 2.54 [1.17, 5.43] | 2.54 | 0.8 |
| lmr_fail_low | 7,401,145 | 2.35 [2.33, 2.37] | 2.09 | 4,994.9 |

  The table-served ProbCut made no decision: its switch is off by default.

- Diagnostics (not the verdict):
  - Razoring at expected cut nodes hides a fail high in 25.0% [24.6,
    25.5] of its probes (all nodes 7.4%); reverse futility at expected all
    nodes 10.9% [10.6, 11.3] (cut nodes 2.4%).
  - Quiescence futility hides 5.1% of the first three captures it prunes
    and under 0.7% of later ones.
  - Reduced fail-lows: 3.0% at depths 1–7, 1.0% at 8 and above; 4.1% at
    expected cut nodes against 1.2% at all nodes; 4.8% for moves 1–3
    falling to 0.8% for 17 and later.
  - A beat counts a probe one point past the bound; the margin is not
    recorded, so a beat share is an upper bound on what matters.
  - Probes cost 2.4 times the search's nodes at `Pricing 4`.
- Deciding result (the registered screen): the most estimated hidden
  changes per million search nodes are **quiescence futility (5,174),
  reduced fail-lows (4,995) and reverse futility (4,211)**; the only rule
  whose beat share's lower bound exceeds 5% is **razoring** (12.27%).
- Disposition: **observation**; the instrument is validated (runs 1, 2,
  2a). The screen points B.4 at quiescence futility and its delta rule,
  B.5 at the reduction of early moves at cut nodes, and the next fits and
  form re-reads at razoring and reverse futility by node type. No rule
  changes here.

#### PREDICTION CALIBRATION — preserve the original prediction above

- Original prediction summary: isolation clean (0.95); both plants above
  honest (0.85 each); beat shares by rule in stated ranges; reduced
  fail-lows 1–4% and rising with depth (0.6); change share at least 60% of
  beat share for move rules; the top three drawn from reduced fail-lows,
  quiet futility, the move-count rule and capture SEE (0.6).
- Observed result: isolation clean; reverse-futility plant above honest;
  the quiet-futility plant failed because the plant was misdesigned (the
  amended plant passed). Nine of the fifteen beat-share ranges with
  decisions held (razor,
  reverse futility, quiet futility, continuation history, the move-count
  rule, capture futility, quiescence SEE, late captures, reduced
  fail-lows). Misses, one direction each: the SEE rules hide far less
  than predicted (quiet SEE 0.46%, capture SEE 0.62%), as do the null move
  (0.41%), ProbCut (1.49%) and the multi-cut (0.70%); quiescence's
  gain-based rules hide far more (futility 4.27%, delta 3.08%, against
  under 1%). Reduced fail-lows fell with depth instead of rising. The
  change shares held (88–99% of beat shares). One of the predicted top
  three appeared. What the model got wrong: it treated a static material
  margin as safe in quiescence and an exchange test as risky, and the
  reverse holds; a capture's evaluation swing beyond its material is
  larger than the margins assume. The plant error: a registered plant
  must be checked to prune a superset of the honest rule's moves before
  it is registered.
