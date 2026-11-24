# C.2.8: the refitted piece-square tables' provenance, 2026-11-24

The refit's acceptance retires the PeSTO disclosure (PLAN C.2.5, the
maintainer's 2026-10-27 rule; `analysis/licence_audit_2026-11-23.md` item
3). This note is the similarity screen re-run on the accepted head.

**Recommendation:** the disclosure is retired. The tables are fitted from
`sf_16`'s, with no PeSTO value carried. README keeps one line saying
releases up to 1.11.0 used PeSTO-derived tables.

## 1. Inputs

- **Candidate:** the accepted head's `src/` (the refit `cbc3676`, restored
  over its revert; bench 3,359,329), exported by `git archive`.
- **Donors**, exported the same way from `D:/code/stockfish`: `sf_19`
  (`edb0d9db`), classical `9587eeeb`, and `sf_16`, the seed's source.
- **PeSTO's tables** as published: the fixture `PESTO_PUBLISHED` of the
  retired orientation test (`git show b912051^:tests/test_eval.cpp`).
- **The seed:** `tools/results/c283/A2/00-seed.txt`.
- Outputs and the comparison script: `tools/results/c28-provenance/`
  (ignored).

## 2. The originality scan (`tools/diag/originality_scan.py`, B.11.3's method)

| Against | Numeric runs | Scaled tables | Raw token runs | Normalised runs |
|---|---:|---:|---:|---:|
| `sf_19` | 120 | 6 | 18 | 33 |
| `9587eeeb` | 58 | 0 | 12 | 30 |
| `sf_16` | 132 | 3 | 15 | 30 |

- **No match inside the piece-square tables** against any donor. B.11.3's
  match, six values of one pawn row against `9587eeeb`'s `psqt.cpp`, is
  gone.
- **One new donor match:** `src/eval_endgame.h:463`, the lone-king drive's
  edge table, is classical Stockfish's `PushToEdges`
  (`9587eeeb` `src/endgame.cpp:33`), unchanged. Amendment 2 ported the
  drive on purpose (BAS-D24). README now names it.
- The KRP-KR scaling rules match as at B.11.3, now in `src/eval_endgame.h`.
  The rest is the coincidence and convention B.11.3 read.

## 3. Distance to PeSTO

Each table's level (its mean over live squares) is removed first, since
levels absorb material. Cells: mean |difference| in cp / correlation.

| Pooled over 12 tables | old header | refit | SF16 seed |
|---|---:|---:|---:|
| against PeSTO | 1.8 / 0.98 | 23.8 / 0.62 | 19.4 / 0.44 |
| entries equal to PeSTO's | 604 of 736 | 4 of 736 | 22 of 736 |

- The refit sits further from PeSTO than its own seed did, and its 4
  equal entries are chance.
- **The eg pawn table converges:** correlation 0.98 with PeSTO's, at
  18.7 cp an entry and no equal entry. Both are dominated by the same rank
  gradient, which the fit found in the data from a seed correlated 0.47.
  This is a shared fact about pawns, not a carried value.
- Per table: `tools/results/c28-provenance/pesto_distance.md`.
