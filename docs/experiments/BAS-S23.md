# BAS-S23

**BAS-S23 — B.2.4: Cluster 1 gates**

- Date / owner / calibration category: registered 2026-10-11 (Amendment 3,
  packet §22.7), before any game; maintainer-run; playing-strength gate.
- Baseline source revision / candidate source revision / dirty-diff identity:
  (a) the unfitted ON arm against the umbrella-off arm at the same revision,
  once its PGO binaries exist; (b) the fitted ON arm (BAS-S22's baked theta)
  against the unfitted ON arm.
- Binary / compiler / flags / PGO identity and hashes: recorded at run time;
  PGO pairs per arm.
- Research question: does Cluster 1 gain Elo on its own evidence, and does
  the fit add Elo on its own evidence, so that a weak tune cannot ride on the
  cluster's margin?
- Hypothesis / proposed mechanism: packet §13 and P2 (BAS-S21).
- Interacting mechanisms / consumers: the default flips to the accepted arm
  after (a) and (b); the legacy search stays compilable until B.8.
- Credible competing hypotheses: (a) passes and (b) fails (the fit is noise);
  (a) fails after B.2.2's screens held (the fixed-node layer does not carry
  Elo, as BAS-S17 showed for razoring alone).

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected diagnostic movement: none new; the screens are B.2.2's.
- Expected Elo sign/range: (a) + (b) together +40 at STC, 90% interval
  [+5, +90] (P2); each gate SPRT `[0,10]`.
- Probability candidate is positive/useful: 65% (P2).
- Confidence and basis: moderate.
- Most likely failure mode: (b) H0.
- Falsification criteria: H0 at `[0,10]` on either gate; a stop under +10
  refutes the coherent-core share (packet §11's stop rule).
- Cheapest prior falsifier: BAS-S21's paired run.
- Registered gate / stop rule: SPRT `[0,10]` at `3+0.03`, each gate accepting
  one thing on its own evidence (PLAN B.2.4).
- Full conditions / provenance: as BAS-S21; the paired-opening SPRT recipe of
  PROCESS; P1–P6 calibration appended here, as packet §11 says.

#### RESULT — append after exposure

- Disposition: registered, not run.

#### PREDICTION CALIBRATION — preserve the original prediction above

- (P1–P6 of BAS-S21 are calibrated here after both gates)
