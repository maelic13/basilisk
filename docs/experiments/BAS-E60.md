# BAS-E60

**BAS-E60 — C.2.8.4: the Syzygy label contract, B's fit against A's, conditional SPRT `[0,3]`**

- Registration: created 2026-11-23 (C.2.8.1), before any fit or game. It
  runs only if its condition holds; otherwise it closes unplayed.
- **Condition**, read from BAS-O13 on corpus B's validation labels at the
  frozen K. Both must hold:
  - the B fit's realized loss is below the A fit's by at least 2e-5
    (BAS-O12's measured resolution);
  - the 95% interval of the paired per-row difference, bootstrapped over
    validation games, excludes zero.
- Candidate: the B fit's baked build. Baseline: the A fit's baked build,
  the BAS-E59 candidate. Both are clean final PGO PEXT, with the same
  compiler.
- Question: do Syzygy-corrected labels (`basilisk-texel-labels-syzygy-v1`)
  make the fitted surface stronger? One causal axis: the labels. K, seed,
  schedule, rows and surface are identical.
- Conditions: BAS-E59's, with seed 280402 and a cap of 4,000 pairs.
- If the condition fails, corpus A stands and B is null at fit level. The
  retry trigger is C.2.4's: a corpus whose contradicted share of
  adjudicable games exceeds 10%, or Phase H's small-gain programme.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- The condition fails: 0.75.
- If it runs: H0 or a cap without a decision, 0.7. B's labels touch 0.28%
  of rows, symmetrically.
- Falsifier: an accepted H1 with an estimate above +5, which would mean
  the corrected rows carry more than C.2.4's numbers allow.

#### RESULT — append after exposure

- **Condition read on run 1, 2026-11-23** (BAS-O13; the report's §5).
  On B's validation labels at K 1.960321:
  - the B fit's realized loss is 0.09634110;
  - the A fit's is 0.09632117.

  B is worse by 2.0e-5. The condition needs it better by at least 2e-5,
  so it fails without the bootstrap, and no game is played. Run-to-run
  fit noise is the size of B's whole label effect.
- Run 1's surface defect affects both arms identically. The registration's
  amendment decides whether B is re-run and the condition re-read.
