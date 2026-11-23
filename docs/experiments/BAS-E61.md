# BAS-E61

**BAS-E61 — C.2.8.4 companion: the refit against the head before the top-tier repair, SPRT `[0,3]`**

- Registration: created 2026-11-23, before any game, at the review
  session's request after BAS-E59. Agent-run under the maintainer's
  authority for the night.
- Candidate: BAS-E59's candidate, `cbc3676`,
  `basilisk-c284-refit-pext-pgo.exe` (`B2495083…`, bench 3,359,329).
- Baseline: the head before Amendment 2's top-tier repair, `901e519`,
  `basilisk-c273-head-pext-pgo.exe` (`EDDBA36E…`, bench 4,777,412).
- Question: did the repair cost the baseline anything? It had no game gate
  of its own, and BAS-E59 played against the repaired head. If the refit
  scores about the same against the old head, the repaired baseline is
  sound.
- Conditions: BAS-E59's, with seed 280403. SPRT `[0,3]` nElo
  (`sprt-default.toml`), 3+0.03 with a 20 ms margin, 1T, Hash 64, paired
  reversed UHO in random order, no adjudication, tablebases off, cap 4,000
  pairs. Launcher: `python tools/run_c284_gates.py --run companion`.
- It decides no acceptance. It is evidence for the maintainer's ruling on
  BAS-E59.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-23 (the review session's)

- H1 with a first-crossing estimate of +60 Elo or more: 0.8.
- Contradiction, reported to the review session: H0, or an estimate under
  +50.

#### RESULT — append after exposure
