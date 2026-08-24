# Manta cross-project import, and a reframing of the Phase-5 diagnosis

**Date:** 2026-08-13. **Source:** `D:/code/manta` (Zig engine, same maintainer).
**Outcome:** one methodology import that **overturns our leading diagnosis**,
two corrections to our own records, one new tool, and several evidence rows.
No engine change.

## 1. The import that changes the picture

Manta's `tools/branching_profile.ps1` measures branching as the **ratio between
consecutive depths**, and its documentation states why:

> a single-depth `nodes^(1/depth)` estimate ... folds in the fixed cost of the
> first plies

Every EBF figure we have recorded — BAS-O03's 2.20 against 1.61, BAS-O04's
1.834 against 1.459 — used exactly that folded estimator. Measured properly,
over 16 suite positions, depths 4–11, **Hash 64 on every arm**:

| depth | Basilisk | SF search + our eval | Basilisk / oracle |
|---:|---:|---:|---:|
| 4 | 29,482 | 6,732 | **4.38×** |
| 5 | 44,326 | 13,947 | 3.18× |
| 6 | 88,653 | 25,683 | 3.45× |
| 7 | 172,074 | 50,996 | 3.37× |
| 8 | 311,338 | 97,930 | 3.18× |
| 9 | 465,659 | 191,465 | 2.43× |
| 10 | 785,548 | 348,756 | 2.25× |
| 11 | 1,170,224 | 588,190 | 1.99× |

| | Basilisk | oracle |
|---|---:|---:|
| **b(4–11) aggregate** | **1.692** | 1.894 |
| b(4–11) per-position median | 1.699 | 1.899 |

**Our per-ply growth is better than the reference search's.** The deficit is a
**constant factor** — 4.4× at depth 4, shrinking to 2.0× by depth 11, which is
exactly what a better ratio does to a worse starting point.

### Why this overturns the diagnosis

Phase 5 has spent three clusters on the premise that our tree is *too wide per
ply*. Every attempt to cut harder failed: reduction magnitude did not move the
metric (BAS-S14), check-depth capping lost −3.48 Elo (BAS-S16), history pruning
would buy no depth (BAS-D04).

Those failures now have a coherent explanation. **We were attacking a growth
rate that was already better than the reference's.** Cutting harder could not
help, because per-ply width was never the deficit.

The deficit is that a depth-4 search costs us 4.4× what it costs the reference.
That is a different subsystem entirely — it is about what a shallow subtree
costs, not about how fast the tree grows.

### What this does not overturn

The 12-ply gap at equal nodes is real and so is the ~9.6-ply gap at equal time
from the tournament (BAS-O01/O03), which is engine-agnostic and immune to node
accounting. Both engines count interior and quiescence nodes, so the ratio
comparison is sound in kind, though absolute counts across engines never are.

## 2. Two corrections to our own records

### 2.1 Hash was not held constant — BAS-O03, BAS-O04, BAS-D03

Manta MAN-S23 retracted its own branching baseline after finding it had spliced
16 MiB and 64 MiB runs; the same engine scored 171,653,746 nodes at depth 12 on
16 MiB against 159,169,542 on 64 MiB, about 8%.

**We had the identical defect.** Basilisk defaults to `Hash 64`, the
Stockfish-based oracle to `Hash 16`, and our harness never set it. Re-measured
with all arms at Hash 64 and one estimator:

| Attribution | Recorded | Corrected |
|---|---:|---:|
| Search | 95.9% | **98.4%** |
| Evaluation | 4.1% | **1.6%** |

The conclusion is unchanged and slightly strengthened. `--hash` is now explicit
in both diagnostic tools, defaulting to 64 and applied to every arm.

### 2.2 The BAS-S16 two-binary route was unnecessary — my error

I recorded that `tools/sprt.ps1` "exposes only Threads/Hash per arm, not
arbitrary UCI options", and built two binaries for the BAS-S16 gate, accepting
one PGO profile's worth of variance between arms.

That was **wrong**. `sprt.ps1` has had `-OptionsA` / `-OptionsB` since before
this phase, at lines 221–222, and its own help says they let "a single binary be
A/B-tested on a UCI knob without a rebuild". A grep for `[string]` did not match
`[string[]]` and I did not check further.

The BAS-S16 verdict itself is unaffected — H0 at −3.48 ±3.32 over 17,058 games
— but its recorded methodology note is wrong and is corrected. Future
single-binary gates should use `-OptionsA/-OptionsB`.

## 3. Tools: what was worth taking

| Manta tool | Verdict |
|---|---|
| `branching_profile.ps1` | **Adopted** as `tools/diag/branching.py` — genuinely new capability, and it overturned our diagnosis on first use |
| Hash-as-explicit-parameter | **Adopted** into `run_suite.py` and `sweep.py` |
| Per-position ratios beside the aggregate | **Adopted** — MAN-S23's `b(4-12)` was decided by one position of forty exploding 41.5%; ours reports a median guard |
| `sprt.ps1` | **Keep ours.** Manta adds `-Nodes` and `-MaxGames`; neither is needed by our clock-based gates, and ours is the harness BAS-M01/M02 calibrated |
| `harness_common.ps1` | **Keep ours.** Manta's differences are refactoring — a named adjudication-profile function — not capability |
| `hce_fit.zig`, HCE fitting substrate | **No value here.** Zig-native and bound to Manta's `hce_params.zig`; our HCE is frozen and BAS-E07 concluded the evaluation gap is NNUE's |

## 4. Evidence worth importing as priors

- **MAN-E05 and MAN-E07** lost about **−23 Elo between them**: two faithful
  reference-family evaluation concepts, implemented with hand-reasoned
  coefficients. Manta's conclusion — adopting reference concepts with reasoned
  constants "reproduces its structure without its calibration" — is a direct
  warning about our 5.9, which is currently scoped as six absent terms each
  hand-set and individually gated. That is precisely the design that lost twice.
- **MAN-S18 and MAN-S20**: two selectivity clusters that each spent a full
  12,000-game budget and *grew* the tree (755,581→772,203 and 744,899→761,703
  nodes) while losing. Converges with our BAS-S13, where a "more principled"
  history response also made reductions smaller.
- **MAN-S23**: a registered pre-gate branching filter refuted a candidate "in
  minutes of arithmetic" where the two clusters above cost 12,000 games each.
  Independent validation of the harness-before-SPRT discipline this phase has
  been using.
- **The endpoint-measure trap**: `b(4-12)` was decided by one position of forty.
  Define a filter on a shape no single position can carry, and define it before
  measuring.
- **Manta's fit catalogue** classifies coefficients *free / fixed / excluded*,
  excluding nonlinear king danger, capped winnability and truncated tables
  because "a linear count model would misrepresent their caps, squares,
  per-application truncation or dispatch". That is independent support for
  BAS-E07: our king-safety funnel is exactly that kind of term, and a static
  linear fit cannot price it.

## 5. Consequence for the pending budget decision

The Phase-5 budget clause fired after 5.5 and 5.6 closed empty, and the
recommendation on the table was to close the strength track and go to NNUE.

**That recommendation should be revisited before it is acted on.** It rested on
"three clusters found nothing", but we now know all three were aimed at a
quantity — per-ply width — that was never deficient. A concrete, unexplored and
large target exists: our shallow searches cost roughly four times what the
reference's do.

This does not reopen the clause automatically, and it is not licence to invent
a fourth pruning cluster. It is a new measurement that changes what the decision
is choosing between, and it is registered as **5.14** rather than smuggled into
a closed one.
