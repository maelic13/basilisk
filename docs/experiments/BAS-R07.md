# BAS-R07

**BAS-R07 — the fail-high clock stop's sink probes: B.10 ended rec1's sink
by a mate proof; the cascade, the tree growth and the stop's hole remain**
(observation, 2026-10-25, agent; exploratory diagnostics with no frozen
prediction; packet `analysis/b5_failhigh_stop_2026-10-25.md`; raw runs,
arm patches, scripts and hashes in `tools/results/fhstop-20261025/`).

Clang 23.1.3 `release-pext` builds without PGO; each arm reproduces its
revision's bench. Rec1 (`7r/5R2/8/2k1PB2/8/4K3/8/8 w - - 0 86`) at
`60000+600` with A.8.17's stop removed:
- `fac5064` (bench 14,978,465): to the maximum, 32,305 ms;
- `71a612b` (11,099,210): to the maximum, 32,305 ms;
- the current head (8,437,148): 16,720 ms, `mate 25`, after nine
  fail-highs and a give-up window.

So B.10's PST repair, not B.2, ends this position's sink, through a mate
proof.

Fixed depth on the head, with the reduction's correction term removed or the
negative-reduction ceiling at `new_depth`, nodes to depth fall:
- rec1, depth 26: 72.9M → 2.4M and 3.2M;
- KR v K, depth 34: 30.8M → 1.9M and 8.1M;
- bench: −25% and −21%.

Extended LMR children are 10.90% of the suite's LMR-path searches (Rarog
10.5%) and 12–39% in won endings. Yet at `60000+600` without the stop both
arms still run rec1 to the maximum (depths 30 and 38).

With the stop present, at `180000+2000` the arms take 2.79× and 4.07× the
optimum without firing. A give-up full window that began just before the
optimum ran 28 s and 47 s.

From the stop-free timelines, a 2× threshold would have guarded four of the
five trees at `60000+600` and missed the B.2 tree. In A.9.2's `10+0.1` games
the head used at least 0.7× its clock on 98 of 66,390 moves (median clock
564 ms). Those were mostly fail-low searches in lost or level positions.

*Lesson.* "The sink is gone" was one position's mate proof. The cascade is
the aspiration loop's and survives a 30× smaller tree. The stop's firing rate
in games is still unmeasured; M1 in the packet registers that read.
