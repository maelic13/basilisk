# C.2.8.1 the fit and gate registration (2026-11-23)

- State / class: closed, `R2`. Registered before any C.2.8 fit or game.
  BAS-O13 holds the fits; BAS-E59 and BAS-E60 hold the gates.
- Owner / date: agent, PLAN C.2.8.1, on Opus at the maintainer's choice.
- Inputs:
  - BAS-O12 with Corrections 1 and 2;
  - C.2.4's packet §6 (B's gate is conditional);
  - C.2.5's production rule (the seed);
  - C.2.7's bracket veto (audit §6).

**Decision.** Fit corpus A, then corpus B, with `fit.py`'s four stages
from Stockfish 16's tables at 0.35, under one frozen K.
- Gate the A fit against the head by SPRT `[0,3]`: BAS-E59, the refit's
  strength.
- Run the label gate, BAS-E60, only if the B fit predicts B's labels
  better than the A fit by more than the instrument resolves.
- The magnitude contract is read and reported, not enforced. Its reads
  explain; the SPRT decides.
- The main counter-argument is a per-phase K. It keeps scores at today's
  magnitudes, so no search margin tuned to them changes meaning. It is
  registered as the retry.

## 1. The magnitude contract (C-97, C-98)

**The measured situation** (BAS-O12, Correction 2):
- A whole-surface fit at one frozen K makes every piece worth 12% to 35%
  more in endgames. Middlegame values move −20% (the pawn) to +4%.
- The header's own K is 2.33 in endgames against 2.01 overall, and the
  fit answers by steepening the endgame's scores.
- 27% more validation rows reach the lazy checkpoint.

**Options:**
1. **One frozen K; `LAZY_MARGIN` unchanged; the contract read and reported.**
   - The protocol as C.2.1 built it.
   - C.0's +15 to +40 prior and its 2.58% Brier screen came from a fit of
     this kind.
   - The re-valuation encodes something true in chess: material converts
     more surely as the board empties.
2. **A K per phase band in the loss.** Scores keep today's magnitudes, so
   no search margin changes meaning.
   - It changes the tuner's loss and C.2.1's contract.
   - It throws away the cross-phase signal option 1 keeps: whether to
     trade into an ending.
3. **Option 1 plus a lazy-margin refit in the same candidate.** Two causal
   axes in one gate (C-57), so it is refused.

**Chosen: option 1.**
- The ±20 cp bound applies to an evaluation unit added on top of a fitted
  surface. A whole-surface refit whose prior is tens of Elo moves scores
  by more than that by design.
- So the contract is read on this candidate, not enforced. These reads
  explain a result and do not accept one:
  - the share of validation rows at the lazy checkpoint;
  - the median served-score change by phase band;
  - the marginal piece values by phase (`piece_value_read.py`'s method).
- **Retries,** in order, if BAS-E59 rejects or its equal-node companion
  (C-95) disagrees with it:
  - option 2, the per-phase K;
  - a lazy-margin refit (C-98, owned by C.10);
  - each as its own registered gate.

## 2. The schedule (8.7.b)

| Stage | Settings | Validated by |
|---|---|---|
| king safety | `fit.py`'s defaults (`--ks-positions 100000`) | C.2.1; it must stop converged |
| linear | `--lr 0.3 --epochs 20000 --patience 50 --min-delta 1e-6` | BAS-O12: four starts converged at 2,700 to 3,200 epochs, and the travel bound (6,000) never binds |
| king safety | as before | — |
| polish | rate 0.075 (a quarter), patience 100, the same tolerance and cap | `fit.py`'s rule |

- Every stage must stop by its own convergence rule. An exit 3 stops the
  run, and it is reported, never relaxed.
- **Anchors:** `--l2 0 --smooth 0`. The only anchors are the surface's:
  - material held (C-19);
  - the two king tables and the eight mobility tables mean-anchored.

  This is the looseness PLAN asks for: BAS-O12's tables moved 72 to 78 cp
  an entry on average, with no L2.
- **Corpus and K:**
  - Corpus A's train split, every row (3,547,892 used).
  - K is fitted once on A's validation at the seed and frozen for both
    arms. The B arm reuses A's K, so the arms differ only in labels
    (C.2.4's packet §6).

## 3. The seed

C.2.5's production rule, unchanged: `seed_sf16.py --scale 0.35` from the
header at the fit's root, one start. `fit.py` must seed from that file
rather than from the compiled header (C.2.8.2).

## 4. The gates

- **BAS-E59, the refit:** the A fit's baked build against the head, at
  BAS-S67's conditions.
  - `[0,3]` nElo, 3+0.03 1T, Hash 64, concurrency 15, paired reversed UHO
    openings;
  - no adjudication, tablebases off, a fault cap of zero, a cap of 4,000
    pairs.
  - One causal axis: the fitted surface, labels A.
- **BAS-E60, the label contract:** the B fit's build against the A fit's,
  under the same conditions. It runs only if, on B's validation labels at
  the frozen K:
  - the B fit's realized loss is below the A fit's by at least 2e-5,
    BAS-O12's measured resolution;
  - and the 95% interval of the paired difference, bootstrapped over
    validation games, excludes zero.

  Otherwise corpus A stands and B is recorded null at fit level, with
  C.2.4's retry trigger.
- **Before any bake** (C.2.8.3's report), all against the seed:
  - the king-safety and king-PST movement (C-56);
  - `bench 13`, where growth over 5% is a finding the gate must explain;
  - the deterministic floors (KBNK, KQK, KRK, KBBK), which must pass;
  - the per-table movement, and the endgame-passer group and E6's signs
    (C.0);
  - the magnitude reads of §1.
- **With the gate:** C.2.7's bracket veto. The candidate is measured at
  21,000, 64,000 and 190,000 nodes against `tools/results/c274/head`.
- **After selection:** corpus A's test split is claimed once
  (`claim_test.py`) and reported for the accepted vector.
- **When refitted tables land:** the PeSTO disclosure is retired and the
  similarity screen re-run (maintainer, 2026-10-27).

## 5. What C.2.8.2 implements

`tools/texel/fit.py` gains:
- `--init FILE`, a seed vector whose root is the header;
- `--K K`, to freeze a given K instead of fitting one;
- `--min-delta`, passed to the linear and polish stages;
- per-stage settings, so the registered schedule runs as registered;
- the lazy-checkpoint count in its loss records.

The magnitude reads become a committed tool, `tools/texel/piece_values.py`
(from `tools/results/c25/piece_value_read.py`), with tests, and the
contract's guards stay. Non-goals: no change to the tuner's loss, the
surface or any engine source.
