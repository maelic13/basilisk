# BAS-C08

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C08 |
| Experiment or failure mode | **SEE king legality (15.0.a).** BAS-X29 reported that Basilisk's `see_ge` "has no king rule". Measurement corrected the mechanism: the `KING=20000` sentinel plus the minimax fold already implemented the rule, but it inferred the opponent's reply from the 8.2 **pin-filtered** attacker set. A piece pinned against its own king still controls squares against the enemy king, so a pinned defender was dropped from that set and the king recaptured illegally. All seven king-legality fixtures ported from Rarog's `see-contract-v1`/`see-repair-v1` **passed on the unrepaired kernel**; only the new pinned-defender cases fail first. |
| Disposition | **Repaired and closed 2026-09-09.** Both kernels break before a king capture when the **unfiltered** opponent attacker set is non-empty. Over 1,896,743 captures from a seeded 4,000-game random-walk corpus the kernel was diffed against itself: 6,919 `see()` values and 333 `see_ge` threshold verdicts changed, and an independent make/unmake legality oracle scores the repair **6,481-0** and **301-0** with zero regressions. `see`/`see_ge` disagreement fell 4.579% -> 4.236%. CTest 12/12. **Bench 12,568,898 -> 14,978,465 nodes (+19.17%)**, NPS -1.28%. Strength undecided; carried to BAS-E55. |
| Conditional lesson / coverage | Pin filtering answers "may this piece recapture?"; king-move legality answers "is this square controlled?". They are different questions and a single attacker set cannot serve both -- reusing the recapture filter for legality silently under-defends. A sentinel that encodes a rule implicitly is also a rule nobody can see is reading the wrong input; the donor's explicit two-line form is worth more than its equivalence suggests. Note the cost direction: a SEE correctness repair that raises SEE verdicts **loosens** every consumer that prunes on `see_ge == FALSE`, so "more correct" bought a 19% larger tree, tail-driven (per-position median +3.6%, 15 of 40 positions shrank). |
| Source | 15.0.a, BAS-X22, BAS-X29 |
