# BAS-S22

**BAS-S22 — B.2.3: Cluster 1 curvature sweep, then SPSA in blocks**

- Date / owner / calibration category: registered 2026-10-11 (Amendment 3,
  packet §22.7), before any game; maintainer-run; parameter fit.
- Baseline source revision / candidate source revision / dirty-diff identity:
  the B.2.2-accepted head (BAS-S21 proceeds); the fit runs on the ON arm's
  Tune build; theta is baked in one engine commit.
- Binary / compiler / flags / PGO identity and hashes: recorded at run time.
- Research question: which of the cluster's 50 coordinates (review §5.10,
  Amendments 1 and 3) carry curvature at STC, and what theta does SPSA in
  blocks (PLAN rule 7c) reach from the §7.3 seeds?
- Hypothesis / proposed mechanism: the seeds are unit-correct (§7.2 rules 1–4)
  but column-uncertain; the ranges span the Stockfish-converted, the
  oracle-converted and the Basilisk-fitted columns, so the sweep locates the
  column and SPSA refines inside it.
- Interacting mechanisms / consumers: every coordinate of the cluster; the
  categorical switches are fixed before the fit (BAS-S21's paired runs).
- Credible competing hypotheses: the surface is flat at STC for most
  coordinates (BAS-E18's saturation lesson), in which case the fit is noise
  and B.2.4(b) reads H0.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected diagnostic movement: curvature on the node-margin and move-loop
  groups (the ×4 family, packet §2.6); little on the LMR 1024ths terms.
- Expected Elo sign/range: the fitted arm against the unfitted one is
  B.2.4(b)'s gate; P2 (BAS-S21) expects +40 at STC for the fitted cluster in
  all.
- Probability candidate is positive/useful: 65% (P2).
- Confidence and basis: moderate; Rarog's method (docs/reference) and the
  B.2.2 screens.
- Most likely failure mode: a flat surface at the harness noise floor;
  over-fitting to the UHO book.
- Falsification criteria: B.2.4(b) H0 at `[0,10]`.
- Cheapest prior falsifier: the curvature sweep itself (a coordinate with no
  curvature is dropped from the SPSA block).
- Registered gate / stop rule: B.2.4(b).
- Full conditions / provenance: as BAS-S21; SPSA overnight per the compute
  schedule; blocks and iteration counts recorded here when registered in
  detail (PLAN rule 7c).

#### RESULT — append after exposure

- Disposition: registered, not run.
