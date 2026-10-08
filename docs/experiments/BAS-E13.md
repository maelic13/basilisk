# BAS-E13

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E13 — the revert is provably the 1.9.3 engine** (2026-08-26). An SPRT can
only *fail to detect* a difference; this proves there is none. At fixed depth
the search is deterministic, so identical best moves **and** identical node
counts imply the same engine. Over the 107-position `suite_v1.epd` at depth 12,
Hash 64 both arms, against `basilisk-1.9.3-baseline-pext-pgo`: **0 best-move
mismatches, 0 node-count mismatches**, and bench **11,941,440** exactly.

*Conditional lesson.* A confirmation SPRT here would spend ~20k games failing to
detect a difference already proven absent. Where a change is claimed
behaviour-neutral, the deterministic identity check is both cheaper and
**strictly stronger** than a null SPRT, and should be preferred. Only the
−1.37% NPS delta is real, and at roughly 1 Elo it sits far below what this
harness resolves (BAS-M06; BAS-M01's ±10 Elo placement floor).
