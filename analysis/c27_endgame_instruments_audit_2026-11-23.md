# C.2.7 endgame instruments: audit against the Rarog snapshot (2026-11-23)

- State / class: the audit (C.2.7's first bullet) is done, `R2`. No
  instrument was changed. The repairs, the bracket and the re-runs wait on
  the maintainer's sign-off of the split in §5.
- Owner / date: agent, PLAN C.2.7.
- Sources:
  - Rarog: the snapshot at `015bccae` (`docs/reference/rarog/tools/diag/`),
    and for held-out tooling its archived PLAN 4.10.7, since `holdout.py`
    is not in the snapshot.
  - Basilisk: `tools/diag/endgame_*.py` at `f9b14cb`, and the engine's
    `ucinewgame` path.
- Why now: PLAN §3 gives every evaluation gate its endgame truth vetoes,
  and C.2.8 is the first evaluation gate.

**Recommendation.** Split the rest of C.2.7 into four sub-steps (§5),
because the leaf holds two implementation passes, one read on an idle
host and long re-runs. Fix the truth contract first (C.2.7.2), because
C.2.8's veto needs it.

The main counter-argument: no historical gate result depends on the
defects found (§3). It is still the vetoes' first use since 6.0.e, and
one of the defects (a thin family) can block a sound candidate.

## 1. The frozen prediction, and the result

The archived 6.6.a card froze this before any audit: most generic
validation and provenance guards are directly portable or already
covered, and runtime adapters need local design (70%).

**Supported.** Of 24 deltas:
- 16 are portable or covered;
- the 6 adapter-specific ones are runtime and compatibility contracts:
  the schema change, cohort identity, sharding, the clock path, the veto
  under a bracket, and the colour-mirror test;
- 2 are prioritisers that belong to C.7.2.

## 2. The deltas

Classes: **P** directly portable; **A** adapter-specific; **C** already
covered; **N** inapplicable to gate integrity. Owners use the sub-steps
of §5.

### Truth runner (`endgame_truth.py` against Rarog's)

| # | Rarog | Basilisk | Class | Owner |
|---|---|---|---|---|
| D1 | The termination change (play on after a material shed) bumped the schema to `rarog-endgame-truth-v2`, and consumers refuse v1 | The same change was made in place under `basilisk-endgame-truth-v1`. Pre-fix reports (6.0.b: 193 and 174 `material_lost`) and post-fix ones (`-refixed`, BAS-D22) share the string | A: v2, plus a loader that accepts a v1 report only when every record carries `shed_material_ply` (the post-fix marker) and none ends `material_lost` | C.2.7.2 |
| D2 | — | `endgame_vetoes.py` defaults `--baseline` to the pre-fix 6.0.b report, and its outcome list still accepts `material_lost` | P: no default baseline; refuse `material_lost` | C.2.7.2 |
| D3 | A SHA-256 over each family's FEN sequence, folded into one cohort id; comparison per family | The frozen manifest's and EPD's SHA-256, the position count and IDs; compare and ceilings require identical family lists | A: a per-family digest over `(id, fen)` in order, so a subset or bracket run compares family by family | C.2.7.2 |
| D4 | The report stamps its four `layers` and disclaims strength | No stamp, though DESIGN defines the four layers | P | C.2.7.2 |
| D5 | Sharded by process, results reassembled by index, the serial run is the reference; relies on `ucinewgame` | Persistent one-thread engines in threads, results reordered by token; also relies on `ucinewgame`. The engine clears its table and every thread's search state on it (`Engine::apply_table_state`). Serial and parallel smoke reports agree, but only at 1 node, which cannot show carried state | A: a serial-against-sharded identity test at a real budget, and a known-bad engine that keeps state across games and must be caught | C.2.7.2 |
| D6 | Refuses a partial family | `map` raises on any worker error; the join is `strict` | C | — |
| D7 | Records `no_move` only | Records crash, engine error, illegal move and no move as anomalies | C (Basilisk ahead) | — |
| D8 | — | The cohort EPD and manifest are hashed byte for byte, but `.gitattributes` does not pin them. On this host they check out CRLF, so BAS-D22 had to read LF exports | P: `-text` for the cohort files | C.2.7.2 |

### Comparison and floors (`endgame_floors.py` against `endgame_compare.py` and `endgame_vetoes.py`)

| # | Rarog | Basilisk | Class | Owner |
|---|---|---|---|---|
| D9 | Thin denominators get no rate, and are reported as thin | A family whose pairs all move the same way has zero variance, so `sigma` is ±∞. **Proved live:** three positions that each lose a conversion read −∞ and `block` | P | C.2.7.2 |
| D10 | McNemar on discordant positions only; INDETERMINATE below 6 | A paired mean difference with no discordant minimum | P | C.2.7.2 |
| D11 | The aggregate fails beyond 2 SE; a family fails beyond 3 SE and is reported beyond 2 | Paired per position. The aggregate reports only (6.0.d registered it so), and a family blocks beyond 3 SE | C (a registered policy; pairing is stronger) | — |
| D12 | Superseded floors are kept and refused | `endgame_ceilings_v1.json`, derived from the contaminated 6.0.b arms, is tracked and unmarked | P (low: nothing reads it) | C.2.7.2 |
| D13 | Renamed to "attained reference result" after BAS-E50 | The field is still `attained_single_engine_ceiling`, with a definition block saying it is no maximum and no gate | C (no consumer) | — |
| D14 | Hard vetoes in the unit suite (`endgames.rs`) | `tests/endgames.epd`, `test_eval`'s symmetry sweep, and B.9.3's `tests/endgame_verdicts.cpp` | C; the colour-mirror extension to the endgame EPDs is A (a C++ test) | C.2.7.2 |

### Budgets and the veto

| # | Rarog | Basilisk | Class | Owner |
|---|---|---|---|---|
| D15 | `endgame_budget_bracket.py`: truth at several budgets over one cohort; refuses differing cohorts | None. Every truth read is at one budget (C-25) | P | C.2.7.3 |
| D16 | `nodes_per_move.py`: nodes per search under a real clock | None | A: Basilisk's time path, Python, an idle host | C.2.7.3 |
| D17 | A verdict seen at one budget only is provisional (BAS-E45) | `endgame_vetoes.py` vetoes any new clean-win discard at one budget, position by position | A: the rule in §4 | C.2.7.3 |

### Other instruments

| # | Rarog | Basilisk | Class | Owner |
|---|---|---|---|---|
| D18 | `endgame_drawn.py`: searched overclaim on Syzygy draws | B.9.3's per-site enumeration (`tests/endgame_verdicts.cpp`) and `endgame_drawn_bias.py` | C; the read on the C head is a re-run (C-133, C-142) | C.2.7.4 |
| D19 | Bound scores never read as evaluations | `fixed_budget_probe.py`, `deep_iteration_screen.py` and `loss_by_cell.py` skip bounds; the truth tools read no score | C (C-145) | — |
| D20 | `endgame_occurrence.py`: occurrence split by root | `endgame_search_occurrence.py` over a chosen suite | N (a prioritiser) | C.7.2 |
| D21 | Board occurrence and the family ranking | None | N (a prioritiser) | C.7.2 |
| D22 | `conversion_audit.py`: wins thrown in games | `tb_gate_audit.py conversion` | C | — |
| D23 | `endgame_book.py` | `endgame_cohort.py`: frozen, re-probed before every run | C (Basilisk ahead) | — |
| D24 | Held-out tooling: FEN-hash halves, a write-once registration with a runner-up, McNemar, plateaus, a spent cohort | McNemar in `narrow_node_probe.py` only | P by contract (no source in the snapshot) | §5: C.7.3, or C.2.7.5 |

Outside the leaf's list: the gate-provenance refusals and build matrix
(archived 6.6.f and 6.6.g). Basilisk records `engine_sha256` but no bench
or build manifest in a truth report.

## 3. What depends on which semantics

| Artifact | Semantics | Consumers | After C.2.7.2 |
|---|---|---|---|
| `tools/results/endgame-truth-6.0.b/` | pre-fix (aborts on a shed) | BAS-E08's ceilings v1, superseded by BAS-E47 and BAS-E50; `endgame_vetoes.py`'s default | refused |
| `tools/results/endgame-truth-6.0.b-refixed/` | post-fix | BAS-E47, BAS-E50 (ceilings v2), 6.0.e's self-check | accepted by the marker |
| `tools/results/b111-checkpoint/truth.json` | post-fix, 60,000 nodes | BAS-D22 (458/480), C.0 | accepted; the candidate veto baseline (§4) |
| BAS-E52's clock arms (2026-09-04) | post-fix | BAS-E52 | unaffected |

- **No gate since 6.0.e has run the veto or compare tools.** No ledger
  entry, analysis or PROCESS rule cites them. Changing their rules
  therefore invalidates no gate.
- **The stale default cannot pass a bad candidate.** A pre-fix baseline
  stops early, so it under-counts its own discards and hides its own
  fifty-move failures: it can only add false vetoes. On the same engine's
  refixed report it adds none, and none is recorded.
- **The engine moved since BAS-D22's head** (`4d2742f`: the evaluator's
  module split and 1.11.0) at the same fingerprint, 4,777,412. Bench
  identity is not behavioural identity, so the report is reused only after
  a family subset of the current head reproduces its records byte for
  byte.

## 4. Handoffs (exact semantics)

**C.2.7.2, truth contract v2 and guards** `[I2]`
- `endgame_truth.py` writes `basilisk-endgame-truth-v2`.
  - It stamps the four layers, and writes per-family digests: SHA-256
    over `id\tfen\n` in cohort order, folded with the family names.
  - Consumers accept v2, and v1 only with the post-fix marker (D1).
- `endgame_vetoes.py` has no default baseline, and refuses
  `material_lost`.
- `endgame_compare.py`:
  - conversion uses McNemar on discordant positions;
  - a family under 6 discordant positions is INDETERMINATE and never
    blocks;
  - a zero-variance family never yields ±∞;
  - the 2 SE and 3 SE policy is otherwise unchanged.
- The cohort files become `-text`, and `ceilings_v1` is refused as
  superseded.
- Known-bad tests, each failing on its bad input and passing on a good
  one:
  - a pre-fix report;
  - a changed cohort FEN;
  - a thin family;
  - a 1-node serial/sharded pair against a real-budget pair;
  - a fake UCI engine that keeps state across `ucinewgame`.
- The colour-mirror sweep extended to `tests/endgames.epd`,
  `endgame_draws.epd` and the recogniser positions, in its own engine-test
  commit.
- Non-goals: no change to a verdict rule beyond D9 and D10, and no new
  instrument.
- Bench is unchanged; the test changes are tooling and tests.

**C.2.7.3, deployed nodes and the bracket** `[V]`
- Ports: `nodes_per_move.py`, which uses Basilisk's real clock path; and
  `endgame_budget_bracket.py`, which refuses differing family digests.
- The read is on an idle host, at the gate's time control: nodes per
  search by game phase.
- The bracket is three budgets that straddle the deployed median, the
  low one at most the median divided by 3. Rarog's 60k, 200k and 600k
  are not inherited.
- **The veto rule, frozen now before any bracket data:**
  - a new clean-win discard or rule-50 failure vetoes only if it appears
    at two of the three budgets;
  - KBN-K is judged the same way, as the multi-budget majority (B-3);
  - engine anomalies veto at any budget.

**C.2.7.4, the C head's baselines** `[V]`
- First, the current head on two families, at 60,000 nodes, against
  BAS-D22's records. Byte identity lets BAS-D22's report stand as the
  60k arm; any difference re-runs the arm.
- Then the head at the bracket's other budgets, under v2: the
  maintainer's commands.
- B.9.3's enumeration re-read on the C head: the drawn overclaim (C-133)
  and the Syzygy-verified vetoes (C-142).
- Nothing else is re-run (§3).

## 5. The split (for sign-off)

| Sub-step | Class | Content |
|---|---|---|
| C.2.7.1 | `R2` | This audit: done |
| C.2.7.2 | `I2` | D1–D5, D8–D10, D12, D14 |
| C.2.7.3 | `V` | D15–D17: the ports, the read, the bracket |
| C.2.7.4 | `V` | §4's re-runs |

D24 (held-out tooling) has no consumer before C.7's first selection
among endgame candidates. C.2.8 uses the cohort only as a veto, which a
spent cohort keeps. "Every measurement sits in the step that reads it"
puts D24 in C.7.3, which I recommend. Otherwise it stays here as C.2.7.5.

## 6. Closure (2026-11-23)

The maintainer agreed the split as proposed, with D24 moved to C.7.3.
- **C.2.7.2** built the v2 contract and its guards.
- **C.2.7.3** set the bracket from BAS-D23's kept run: 21,000, 64,000
  and 190,000 nodes.
- **C.2.7.4** measured the head (`tools/results/c274/`). It ran the
  maintainer's command, `run_c274.py`, plus one check of mine at 60,000
  nodes.

**The C head's truth baselines** (`tools/results/c274/head/`,
`truth-<budget>.json`, v2; binary `EDDBA36E…`, bench 4,777,412) convert
443, 448 and 463 of 480 clean wins at the three budgets.

| Family | 21k | 64k | 190k |
|---|---:|---:|---:|
| KR-KN | 21 | 23 | 22 |
| KNN-KP | 4 | 4 | 14 |
| KRP-KR | 21 | 22 | 24 |
| KRP-KB | 23 | 22 | 23 |
| KBP-KB | 23 | 22 | 24 |
| KBP-KN | 22 | 22 | 23 |
| KBPP-KB | 20 | 24 | 23 |
| the 13 others | 23–24 each | 23–24 each | 23–24 each |

KNN-K has no clean wins in the cohort, only its 16 draws.

**The single-budget reads are noisy at the position level.**
- The head at 60,000 nodes reproduces BAS-D22's 770 records exactly. So
  the evaluator's module split changed nothing in these endgames, and
  every difference below is the budget's alone.
- From 60,000 to 64,000 nodes (7%), 22 positions change their conversion,
  and KNN-KP alone goes from 10 to 4 of 24.
- That is the case the two-of-three rule exists for. A position read at
  one budget is a sample of a search's chaos, not a property of the
  evaluation.

**B.9.3's enumeration on the C head** (C-133, C-142; `compare_b93.py`,
`compare_b93.json`), class for class against B.9.3's records:
- **The exhaustive classes** (every three- and four-man class, KBPP-K,
  KRP-KR) change only where B.9.5 repaired a site, and nothing new is
  wrong.
  - Same-coloured bishops are now a dead position: KBB-K's 765,152
    positions that `--estimates` read as overclaimed (F6) now have 0.
  - The pawnless scale of 0 no longer fires with pawns on the board:
    KPP-K, KN-KP and KB-KP lose their defect lines, 2,395, 218,092 and
    149,399 wrong before.
  - KRP-KR's exhaustive lines are identical to B.9.3's.
- **The random classes** move within sampling noise.
  - The exception is KBB-KB's pawnless scale 4. Its wrong count is the
    same (8,626 against 8,655), but the dead-position rule now takes 17%
    of its positions. The rate therefore rises from 0.43% to 0.52%,
    over the 0.5% line. It is one of V6's scale-4 classes, already
    routed to C.7.5.
- **The defects that remain** are B.9.3's kept ones, at their recorded
  rates:
  - V3, KRP-KR's draw rules;
  - V4, the KRPP-KRP table (5.07% on this sample);
  - V5, KBN-K's known win (0.6885%);
  - V6, the pawnless scales 4 and 14.

  None blocks a gate, since each has its leaf and retry trigger.

**For C.2.8's gate:** the endgame veto is

    python tools/diag/endgame_budget_bracket.py veto --baseline-dir tools/results/c274/head --candidate-dir <candidate dir> --budgets 21000,64000,190000

with the candidate measured by `endgame_budget_bracket.py run` at the same
budgets.
