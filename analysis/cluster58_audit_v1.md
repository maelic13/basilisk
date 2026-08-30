# Cluster 5.8 E — root search and clock handoff: contract inventory

Audit date 2026-08-30. Basilisk head `dbfd47f`; reference Stockfish `9587eeeb`
(`hybrid/stockfish/src/search.cpp`).

Method follows `cluster57_audit_v1.md`. No engine change is proposed here; the
output is the candidate list 5.8 will choose from.

**Precondition check.** 5.8 requires clusters C and D. 5.6 (C) closed 2026-08-13
with its contracts audited and one defect *recorded rather than gated* —
history pruning fires 142 times in 5.36M tested quiets, provably unsatisfiable
at depth 6 (BAS-D04). 5.7 (D) closed 2026-08-30: one change kept as a reading
(5.7.2), three refuted, one undecided. Both clusters are closed with their
contracts measured, which is the bar the precondition rule sets.

**Carry-in caution.** PLAN states total time allocation must not move until the
root evidence is coherent, and that any real-clock change gates separately under
the time/root/SMP evidence rule. This audit therefore separates **root search**
(§1–§3) from **clock** (§4), and only the former is proposed for 5.8's early
sub-steps.

---

## 1. Aspiration window — three material divergences

| Contract | Reference | Basilisk | Verdict |
|---|---|---|---|
| Gate | `rootDepth >= 4` | `depth > 3` | Equivalent |
| Initial delta | 19 | `aspiration_delta` = 19 | Equivalent |
| Fail-high | `beta = bestValue + delta` | `asp_b = score + delta` | Equivalent |
| **Fail-low** | `beta = (alpha + beta) / 2` **and** `alpha = bestValue - delta` | `asp_a = score - delta` **only** | **Different** |
| **Re-search depth** | `rootDepth - failedHighCnt - searchAgainCounter` | full depth, always | **Absent** |
| Delta growth | `delta += delta / 4 + 5` | `delta += delta / 2` | **Different** |
| Give-up | none — widens until it contains the score | `delta >= 900` → full-width re-search | Ours only |

### 1a. Fail-low does not narrow beta — the clearest gap

On a fail-low the reference pulls **beta down to the window midpoint** as well as
dropping alpha. We only drop alpha, leaving beta where it was, so our re-search
window is strictly wider than the reference's for the same event.

The reference's reasoning is sound and cheap: a fail-low says the true score is
*below* the window, so the old beta is now known to be far too generous. Keeping
it wastes the re-search. This is a two-line change with a clear rationale, and it
is the strongest candidate in this cluster.

### 1b. No depth reduction on repeated fail-highs

The reference re-searches **shallower** each time the root fails high
(`failedHighCnt`), on the argument that a root thrashing high is better resolved
cheaply than exactly. We always re-search at full depth.

Note `searchAgainCounter` in the same expression is an SMP mechanism for helper
threads and is *not* in scope here — Basilisk's 1T gate is the reference point,
and PLAN routes SMP changes through the time/root/SMP evidence rule.

### 1c. Delta growth is faster in ours

`delta += delta/2` (×1.5) against `delta += delta/4 + 5` (×1.25, +5). Ours
escalates faster, so it needs fewer re-searches but reaches a very wide window
sooner. Combined with 1a, our fail-low path is doubly wide. Interacts with 1a and
should not be tested independently of it.

### 1d. The `delta >= 900` give-up is ours alone

The reference has no escape hatch; it widens until the window contains the score.
Ours bails to full width at delta 900. Recorded as intentionally different — a
bounded worst case is defensible — but it means our window sequence is
`19, 28, 43, 64, 96, 145, 217, 325, 488, 733, ∞`, so the hatch fires on the 11th
re-search at the latest. Whether it ever fires is not currently counted.

## 2. Root move authority and PV ownership

| Contract | Basilisk | Verdict |
|---|---|---|
| Best move source | `pv_table_[0][0]`, guarded by `pv_len_[0] > 0` | Sound |
| Ponder move | PV[1], overridden by tablebase PV, else `ponder_from_tt` | Sound; TB override is deliberate |
| Fallback | `RootMoveTable::fallback_move()` — first entry, mutex-guarded | Present |
| Result publication | only when `pv_len_[0] > 0`; `result.depth = depth` | Sound |
| Stability input | `best_stability` counter plus decaying `best_move_changes` | Present, 8.5.12 |

No divergence found worth a candidate. The one observation: `result.score` is
`reported_score` (tablebase-corrected) while `root_table_->update(...)` is passed
the raw `score`. That is almost certainly deliberate — the table wants search
scores for ordering, not TB-corrected ones — but it is not stated in place, and
a future reader could reasonably take it for a bug. **Documentation item, not a
behaviour item.**

## 3. What is NOT here

Completed-root authority across iterations (the "keep the previous iteration's
move if this one was aborted" contract) is handled by `if (stopped_ && depth > 1)
break;` before the result is published, so an aborted iteration cannot overwrite
a completed one. Equivalent in effect to the reference. No candidate.

## 4. Clock — deliberately not opened

Our time management carries `best_stability`, `best_move_changes`, score-drop
extension, and effort-based scaling (`root_best_effort_`). The reference's
`9587eeeb` time policy is materially older than ours here, so this is another
blend-of-eras surface rather than a gap.

PLAN forbids moving total time allocation until the root evidence is coherent.
**No clock candidate is proposed until §1's aspiration items have readings.**

---

## Candidate list for 5.8, ordered by expected value

1. **5.8.2 Fail-low narrows beta** (§1a). Two lines, clear rationale, strictly
   narrower re-search window. The strongest candidate in the cluster.
2. **5.8.3 Delta growth rate** (§1c). Interacts with 1; test after, not
   independently.
3. **5.8.4 Fail-high depth reduction** (§1b). Larger change, and it alters how
   much work an unstable root consumes — measure the fail-high distribution
   first, as 5.7.3 did for the extension stack.
4. **5.8.5 Instrument the aspiration path.** None of the above can be sized
   without counting fail-lows, fail-highs, re-searches per iteration and whether
   the `delta >= 900` hatch ever fires. **This should run first in practice** —
   5.7 showed repeatedly that measuring reach before implementing is what keeps
   candidates cheap.
5. **5.8.6 Documentation**: state the `reported_score` / `score` split at the
   `root_table_->update` call site (§2).

Method carried from 5.7, and it is not optional here: report **mean, median and
the directional split** for any depth measurement (BAS-D13), and treat WAC as a
**floor rather than a comparator** (BAS-D14).
