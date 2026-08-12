# Phase 5.3 — Idea inventory, version 1

**Reference:** Stockfish `9587eeeb` (last pure-HCE master before NNUE).
**Subject:** Basilisk 1.9.3 / `development`.
**Evidence:** BAS-O01–O03 (oracle), BAS-D01/D02 (`tools/diag/baseline_v1.json`).
**Date:** 2026-08-12.

## What this document is, and is not

It is a list of **problems the reference solves that Basilisk does not**, ranked
by our own diagnostic evidence. It is not a list of upstream functions to
reproduce, and nothing in it is a design. Per PLAN's Independence contract, each
cluster designs Basilisk's own answer and must be able to justify it without
appealing to upstream authority.

Reference constants are quoted only to show the **magnitude** of a difference we
already measured from the outside. They are not values to adopt; they were
fitted to another search and another evaluation scale.

## Classification summary

| # | Item | Class | Owner | Evidence rank |
|---|---|---|---|---|
| 1 | Reduction modulation is nearly inert | **Missing** | 5.4.3 | **1** |
| 2 | Reduction eligibility excludes captures and checking moves | **Intentionally different — suspect** | 5.4.3 | 2 |
| 3 | Check extension is unconditional | **Intentionally different — suspect** | **5.4.4 (moved)** | 3 |
| 4 | History pruning is unreachable | **Missing consumer** | 5.6 | 4 |
| 5 | Move ordering | **Equivalent** | — | none |
| 6 | Reduction context we lack entirely | **Missing** | 5.4.3 | 5 |
| 7 | TT/qsearch/eval separation, aspiration, correction | **Not yet inventoried** | 5.5 / 5.8 | deferred |

---

## 1. Reduction modulation is nearly inert — MISSING

**The problem the reference solves.** A single reduction curve over
(depth, move number) is too blunt. The amount by which a late move should be
reduced depends on node context: whether the node is expected to fail high,
whether the line has been on the PV, whether the position is improving, and how
good the move's history is. Without that modulation the search reduces the same
amount everywhere and cannot buy depth where depth is cheap.

**What we measured.** BAS-D02: only 36.1% of eligible moves are reduced, 16.2%
pass every gate and then compute a reduction of zero, and the re-search rate is
**1.744%**. Reductions that are almost never wrong are reductions that are far
too small. BAS-O03/D02: 12 plies shallower than the oracle at equal nodes.

**Where the magnitude went.** Basilisk has the right *shape* — the adjustments
exist and are wired — but their magnitudes are an order of magnitude smaller
than the reference's, so they barely move the reduction:

| Context | Reference | Basilisk (1024ths ⇒ plies) | Ratio |
|---|---:|---:|---:|
| Cut node | **+2 plies** | 401 ⇒ **+0.39** | ~5× |
| Node is/was on PV | **−2 plies** | 23 ⇒ **−0.02** | ~87× |
| Not improving | +1 | 89 ⇒ +0.09 | ~11× |
| TT move is a capture | +1 | 301 ⇒ +0.29 | ~3.4× |
| Base coefficient on log(d)·log(m) | ≈0.60 | 0.479 | ~1.25× |

The cut-node term matters most because most interior nodes are cut nodes. Ours
contributes about a third of a ply where the reference contributes two.

**Why the current values are what they are.** They were produced by the
`hcefinal` SPSA, which was the first joint tune to make these knobs live at all
(they had previously been registered nowhere). That tune accepted +35.94 Elo, so
the values are not arbitrary — but it optimised them inside a search whose other
constants were fitted around timid reductions, and it ran under the pre-2026-07-21
harness bias. This is durable lesson 2 in its usual form: the knobs and their
consumers are one system.

**What Basilisk must achieve** — not how: reductions whose magnitude actually
responds to node context, with a re-search rate that reflects reductions
occasionally being wrong, without losing the tactical safety the current timid
policy buys. The joint refit of the surrounding constants is part of the cluster,
not a follow-up.

**Note on the zero-clamp.** The 16.2% is partly structural: the history term is
integer-quantised (`(stat / 5683) * 1024`), so it contributes exactly zero until
|stat| ≥ 5683 and then a whole ply. A sub-ply history response is available for
free in the existing 1024ths representation and is the obvious first thing to
test — but it is a candidate, not a conclusion.

## 2. Reduction eligibility — INTENTIONALLY DIFFERENT, suspect

**Difference.** Basilisk reduces only quiets and losing captures, never
promotions, and never a move that gives check. The reference reduces captures
and promotions under conditions and does not exclude checking moves at all; it
requires `depth ≥ 3` where we require `depth ≥ 2`.

**Evidence.** Small on its own: `lmr_blocked_gives_check` is 2,511 of 256,707
eligible (~1%), and `lmr_blocked_movetype` 966. So this is **not** where the
width is, and it must not be sold as if it were.

**Why it is still listed.** It compounds with item 3 — checking moves are
excluded from reduction *and* their nodes are extended — and with item 1, since
the same nodes that most want more reduction are the ones the exclusion protects.
Treat as a rider on 5.4.3, never as a standalone candidate.

## 3. Check extension is unconditional — INTENTIONALLY DIFFERENT, suspect

**Difference.** Basilisk extends **every** in-check node by one ply. The
reference extends only checks that are discovered or not losing material
(`is_discovery_check_on_king || see_ge(move)`) and rates the whole mechanism at
about 2 Elo.

**Evidence.** `check_exts` fire at **15.84% of interior nodes** — 2.4M of 15.1M.
One ply added to a sixth of all interior nodes is a large, unconditional spend,
and it lands on exactly the moves reduction is forbidden from touching.

**The trap.** Experiment 8.6.7 already removed check extensions standalone and
lost **−10.17 ±6.52**, and BAS-X01 records that the same removal gained +30.75 in
Rarog. That verdict stands and must not be relitigated as a cleanup. It is,
however, the classic signature of durable lesson 2: the surrounding surface was
fitted around the extension, so removing it alone is not a fair test of it.

**What this means for the plan.** The unconditional extension and the
never-reduce-checking-moves rule are two halves of one question — *how much depth
do we spend on checks* — and they cannot be adjudicated separately. See the
cluster-order amendment below.

## 4. History pruning is unreachable — MISSING CONSUMER

**Evidence.** `hist_prunes` fired **142 times in 15.1 million interior nodes**
(0.0009%). The mechanism is present, wired, and de-facto dead. This corroborates
the pre-existing 8.6.6 observation that history pruning went dead after
`hcefinal` re-scaled the history space.

**The problem.** Not "we are missing history pruning" — we have it. Either its
threshold no longer intersects the post-`hcefinal` history distribution, or its
consumers changed scale underneath it. A pruning rule that never fires is either
a bug or dead code, and both deserve resolution rather than another year of
being counted.

**Owner.** 5.6 (main selectivity), because the fix is a margin/threshold question
against the history scale that 5.4.2 will have settled. Do not touch it before
then — its input is exactly what 5.4 is changing.

## 5. Move ordering — EQUIVALENT, no action

BAS-D01: 89.10% first-move cutoffs, mean cutoff index 0.214, cutoff sources
TT 24.6% / good captures 49.3% / quiets 25.4%. Staged generation, TT move first,
killers and counters, SEE-split captures — the contracts are in place and working.

This is a genuine **equivalent** finding and the most useful kind: it removes
move-picker rework from cluster 5.4 and prevents an expensive change being
measured against an already-good baseline.

## 6. Reduction context the reference has and we lack — MISSING (low rank)

Listed for completeness, deliberately unranked against each other, and none of
them is a commitment: opponent's move count, comparing this node's stat score
against the previous ply's, moves that escape a capture, whether the TT move was
singularly extended, and a TT-hit-rate signal. Each is a small term in the
reference and several are entangled with mechanisms we do not have.

These are candidates for **after** item 1 is settled. Adding a small context term
to a modulation surface that is itself inert would measure nothing — precisely
the premature-adoption failure the maturity preconditions exist to prevent.

## 7. Not yet inventoried — deferred, and stated as such

TT producer/consumer detail, static-eval/pruning-eval/searched-bound separation,
qsearch staging, ProbCut internals, correction-history contexts, aspiration and
root policy. These belong to clusters 5.5 and 5.8, and their preconditions are
not met yet: their inputs are what 5.4 is about to change.

Inventorying them now would produce a map of a search that is about to move.
Each cluster's audit step re-runs this exercise for its own surface against a
current baseline. This is a deliberate scope decision, not an omission.

---

## Cluster-order verdict

The provisional order **stands**, with one amendment that the evidence forces.

**Confirmed.** 5.4 first, and within it 5.4.3 (reduction/re-search contract) is
the payload rather than 5.4.1/5.4.2. Ordering is equivalent (item 5), so the
move picker is not the work.

**Amendment — check-move depth policy moves into cluster 5.4 as 5.4.4.**

The unconditional check extension (item 3) and the never-reduce-checking-moves
rule (item 2) both answer "how much depth do we spend on checks", and 8.6.7
already demonstrated that changing one of them alone loses ~10 Elo. Leaving the
extension in cluster 5.7 would mean 5.4 ships a reduction contract that is
forbidden from touching a sixth of all interior nodes, and 5.7 later ships an
extension change measured against a reduction surface refitted around the old
exclusion — each half looking worse alone than the pair is together.

5.7 retains singular, double/negative and IIR extensions, whose preconditions
genuinely are TT provenance from cluster 5.5.

This is a dependency correction, not a scope increase: no new mechanism enters
Phase 5, one moves between clusters so that it can be adjudicated jointly.

## Standing warnings for cluster 5.4

1. **Reference constants are magnitudes, not values.** Quoted here to show that
   a gap exists, and fitted to a different search and evaluation scale.
2. **8.6.7 is not reopened for relitigation.** Standalone check-extension removal
   lost −10.17 ±9.83-class margins and stays rejected. What 5.4.4 tests is a
   *joint* depth policy, registered as a new experiment against the accepted head.
3. **Durable lesson 5 cuts both ways.** A smaller tree can be worse. The 5.2
   counters localize width; they do not show that reducing more gains Elo, and
   prune recall is still uninstrumented.
4. **The maturity precondition for 5.4 is satisfied** — staged generation, TT
   move availability, live history tables and post-move check knowledge all exist
   and BAS-D01 shows them healthy.
