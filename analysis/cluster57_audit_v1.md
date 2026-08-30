# Cluster 5.7 D — extensions and depth authority: contract inventory

Audit date 2026-08-30. Basilisk head `977b7cd`; reference Stockfish `9587eeeb`
as vendored on branch `hybrid` (`hybrid/stockfish/src/search.cpp`).

Method follows `cluster55_audit_v1.md`: enumerate each contract, state whether
ours is **equivalent**, **intentionally different**, or **absent**, and record
the reason in place. No engine change is proposed by this document; its output
is the candidate list 5.7 will choose from.

**Precondition check.** 5.7 requires clusters A and B. 5.4 closed with the
ordering measured healthy (BAS-D01, 89.1% first-move cutoff) and 5.5 closed with
TT provenance and eval separation **equivalent** and the TT layout mature. The
one gap 5.5 recorded — the persisted TT-PV bit — turns out to matter here
specifically, and is treated as its own row below rather than as a blocker.

---

## 1. Singular extension

| Contract | Reference `9587eeeb` | Basilisk | Verdict |
|---|---|---|---|
| Gate depth | `depth >= 6` | `depth >= 5` | **Different** — ours fires one ply earlier |
| TT depth | `tte->depth() >= depth - 3` | `tt_depth >= depth - 3` | Equivalent |
| TT bound | `bound() & BOUND_LOWER` | `TT_BETA \|\| TT_EXACT` | Equivalent |
| Score sanity | `abs(ttValue) < VALUE_KNOWN_WIN` | `abs(tt_score) < MATE_SCORE - MAX_PLY` | Equivalent |
| Recursion guard | `!excludedMove` | `ss->excluded == MOVE_NONE` | Equivalent |
| Singular beta | `ttValue - ((formerPv + 4) * depth) / 2` | `tt_score - singular_beta_mult * depth` | **Different** — no `formerPv` term |
| Singular depth | `(depth - 1 + 3 * formerPv) / 2` | `(depth - 1) / 2` | **Different** — no `formerPv` term |
| Multicut | `singularBeta >= beta` → return | identical | Equivalent |

## 2. The `ttValue >= beta` branch — materially different mechanisms

This is the sharpest divergence found.

**Reference:** runs a *second* verification search at `(depth + 3) / 2` with the
move still excluded, and **returns `beta`** if that also fails high. It is a
second cutoff opportunity.

**Basilisk:** applies `extension--`, a **negative extension** — the move is
searched one ply *shallower* instead of the node being cut.

These are not variants of one idea. One prunes a subtree; the other reshapes
depth allocation within it. Ours is the later-SF idiom, so this is not a gap to
close by copying — but the two cannot both be described as "the `ttValue >=
beta` contract", and 5.7 exists to decide which semantics we actually want.

## 3. `singularQuietLMR` — ABSENT, and it is squarely in 5.7's mandate

The reference sets `singularQuietLMR = !ttCapture` when the move extends, then
**reduces LMR for that move later in the loop** (line 1197). Basilisk has no
equivalent: our singular move receives its extension and is then reduced by LMR
on the same terms as any other move.

5.7's stated mandate is that "extension and reduction decisions must be
arbitrated against a settled LMR". This is precisely that arbitration, and we do
not perform it. **Strongest candidate on this list.**

## 4. Check extension — ours STACKS where the reference makes it exclusive

| | Reference | Basilisk |
|---|---|---|
| Condition | `givesCheck && (discovery \|\| see_ge(move))` | **unconditional** when in check |
| Placement | `else if` in the extension chain | applied to `depth` **before** the move loop |
| Interaction with singular | **mutually exclusive** — one extension per move | **both apply**: node depth +1, then TT move +1 or +2 |

So a node in check whose TT move is singular can receive up to **3 plies** of
extension in Basilisk, where the reference caps it at 1. `ss->check_exts` is
propagated (Phase 6.4 rider) but nothing consumes it as a cap.

Ownership note: 5.3 moved check extensions to **5.4.4**, which closed with
BAS-S16 rejected (−3.48 ±3.32). The *stacking semantics* were not what 5.4.4
tested — it tested a path cap — so this remains open and belongs to 5.7 by its
interaction with singular, not as a re-litigation of 5.4.4.

## 5. Extensions the reference has and we do not

| Extension | Reference condition | Basilisk |
|---|---|---|
| Passed pawn | killer move + advanced push + passed | **absent** |
| Last captures | captured > pawn && non-pawn material ≤ 2×rook | **absent** |
| Castling | `type_of(move) == CASTLING` (a separate `if`, so it stacks) | **absent** |

Each is small in the reference's own accounting. They are recorded for
completeness, not proposed: adding three unmeasured extensions to a search whose
extension *semantics* are unsettled is the ordering error PLAN's precondition
rule exists to prevent.

## 6. Extensions we have and the reference does not

| Mechanism | Basilisk | Reference `9587eeeb` |
|---|---|---|
| Double extension | `+2` when `s_val < s_beta - singular_double_margin`, non-PV, capped by `double_ext_max` | absent |
| Negative extension | `extension--` on `ttValue >= beta` | absent (uses the second search instead) |
| IIR on a *stale* TT entry | `tt_depth < depth - 3` also triggers | absent |

**We are not behind the reference here — we are a blend of eras.** Double and
negative extensions post-date `9587eeeb`; the vendored reference is the last
pure-HCE master and is *older* than parts of our search. This reframes 5.7: the
job is **internal coherence**, not catching up.

## 7. IIR vs IID — intentionally different, ours is the modern form

| | Reference | Basilisk |
|---|---|---|
| Mechanism | true IID: searches at `depth - 7` to populate the TT | IIR: `depth--` |
| Gate | `depth >= 7 && !ttMove` | `!is_pv && depth >= 4 && (no TT move \|\| stale)` |

Ours fires earlier, on non-PV only, and additionally on stale entries. Recorded
as intentionally different; no action proposed.

## 8. `formerPv` / persisted TT-PV bit — ABSENT, adjudicated at 5.5

`formerPv` widens the singular margin and deepens the verification search for
nodes that were once PV. Both of our singular formulas lack the term entirely.

5.5 adjudicated the underlying TT-PV bit as **missing but not worth an age bit**
(the 8.5.7 re-test measured +51% nodes with no operating point). That decision
stands and is not reopened here. The consequence is recorded so it is not
rediscovered: our singular margins are `formerPv = 0` everywhere, which is the
reference's *non*-PV behaviour applied uniformly.

---

## Candidate list for 5.7, ordered by expected value

1. **`singularQuietLMR`** — absent, directly named by 5.7's mandate, cheap to
   implement, and it is the extension/reduction arbitration the cluster exists
   to perform.
2. **Check-and-singular stacking** — up to 3 plies where the reference allows 1.
   `ss->check_exts` is already propagated and unused, so the instrumentation is
   in place. Requires care: 5.4.4 already lost games to a naive path cap.
3. **`ttValue >= beta` semantics** — decide between our negative extension and
   the reference's second verification search. A real design decision, not a
   port.
4. Singular gate depth 5 vs 6 — a one-constant difference, worth an A/B only
   once (1)–(3) are settled, since it interacts with all of them.

Items 5 and 6 (missing minor extensions, our extra mechanisms) are inventory
only. Per PLAN, 5.7 **gates the integrated contract, not the individual
extensions**, so the candidate that goes to SPRT should be a coherent set, not
the first item alone.
