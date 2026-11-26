# BAS-E67

**BAS-E67 — C.3.6: Basilisk's own forms, the screen's candidate against the base, one cluster SPRT `[0,3]` and the endgame bracket veto**

- Date / owner / category: registered 2026-11-26, before any arm or
  game; agent-run overnight with the maintainer's leave;
  evaluation/HCE.
- Base: fixed by BAS-E66. It is R on corpus C if R was kept, otherwise
  the shipped vector on corpus A.
- Candidate, by the rule frozen now: from the base's screen (BAS-O20 if
  C, BAS-O18 if A), arm P if Δ(P) is at least the best single Δ,
  otherwise the best single form; the one form if one passed. If none
  passed, there is no game read, and this entry closes with that.
  The candidate's bits, fitted values, commit and `bench 13` are filled
  in here before the first game.
- Research question: do Basilisk's own forms, fitted, gain strength on
  the accepted evaluation?
- Pre-game checks: BAS-E66's list.
- Conditions: BAS-E65's (BAS-E66's list), its own seed. **Cap 20,000
  pairs.** Nothing else runs on the host while it plays. The maintainer
  may stop it early.
- Then the C.2.7 endgame bracket veto against the base.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-26

- A game read happens at all, so at least one form passes the base's
  screen: 0.75.
- Given a read: H1 0.35, H0 0.3, the cap or the maintainer's stop 0.35.
- Given H1, the first-crossing estimate is +5 Elo or less: 0.7.
- Power at this cap: H1 with about 0.36 at +2 Elo, 0.7 at +3, 0.98 at
  +5; H0 with about 0.3 at a true 0.

#### Registered rule

- **The veto, the maintainer's ruling for this run only** (2026-11-26):
  "H1 alone is enough whatever the check says, because we have the C.4
  in plan. It should not be general rule, just rule for this C.3 work."
  Clean-win discards and conversion counts neither block nor hold. The
  bracket is recorded in full and its vetoed positions go to C.4.
- H1 with no hard failure (`new_rule50_failure`, `engine_anomaly`,
  `malformed_record`): the candidate is kept and baked. A cluster
  returns its members without attribution.
- H1 with a hard failure: not baked; it returns to research as an R3
  return, and the cause decides what is redone, by the packet's
  Amendment 1 (the maintainer's ruling of 2026-11-26). If research
  cannot fix it: held for the maintainer.
- H0: not baked; for a cluster, no member is closed individually.
- The cap, or a maintainer's stop before a boundary: not baked, recorded
  as unresolved at the games played and routed to Phase H with its
  estimate. Never extended or restarted on the same pair.
- The estimate at a stop is a first-crossing estimate.

#### RESULT — append after exposure
