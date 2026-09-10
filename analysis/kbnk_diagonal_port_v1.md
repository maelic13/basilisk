# KBNK diagonal-potential transfer audit

This is the source and chess-mechanism audit for PLAN 6.1.a–b. It freezes what
Rarog actually demonstrated and maps only that mechanism onto Basilisk. Step
6.1.b then established that Basilisk already had the same geometric potential
in an algebraically equivalent form. It does not choose new Basilisk
coefficients; that belongs to 6.1.c.

## 6.1.b correction: the geometry was already present

The 6.1.a audit correctly mapped both diagonal formulas but missed that
Basilisk's existing Manhattan distance to the nearer correct corner is the same
function plus a constant. For a dark-squared bishop with correct corners a1/h8:

```text
corner_md = min(f + r, 14 - f - r)
14 - corner_md = 7 + abs(7 - r - f)
```

For a light-squared bishop with correct corners a8/h1:

```text
corner_md = min(f + 7 - r, 7 - f + r)
14 - corner_md = 7 + abs(r - f)
```

The added seven is constant throughout KBNK, so it cannot affect move ordering.
Step 6.1.b rewrites `kbnk_score()` in the explicit diagonal form while retaining
that constant and the existing coefficient. This is score-identical, makes the
transfer seam visible, and proves that another geometry port cannot explain the
remaining Basilisk failures.

The transferable Rarog lesson is now narrower: its conversion gain came from
the diagonal potential's **dominance over competing king pulls** after a sweep,
not from geometry absent in Basilisk. Measuring that scale and its interaction
with Basilisk's edge, friendly-king and knight terms is exactly 6.1.c.

Verification of the explicit rewrite: release `test_eval` and `test_endgames`
pass, including both bishop-colour orientations, and `bench 13` remains exactly
12,709,666 nodes. No conversion claim is made because the score is unchanged.

## Source evidence

The finding landed in Rarog commit `4aea0c7407c05eb6417f52b61e5928404306595e`
(`Drive the mate along the diagonal, at the ratio that makes it work`). The
supporting evidence is RAR-E10 and
`D:/code/rarog/analysis/endgame_conversion_audit_2026-09-01.md`.

At 60,000 nodes on Rarog's fixed Syzygy cohort:

| Rarog potential | KBN-K conversion | KBB-K conversion |
|---|---:|---:|
| Chebyshev 8/4 baseline | 19.4% | 78.0% |
| Chebyshev + Manhattan 32/16/20/10 | 57.1% | 100.0% |
| Diagonal 60, roughly level with king pull | 56.1% | 100.0% |
| Diagonal 120 | 83.7% | 100.0% |
| Diagonal 240 | 94.9% | 100.0% |
| Diagonal 360, shipped by Rarog | 96.9% | 100.0% |
| Diagonal 480 | 96.9% | 100.0% |
| Diagonal 720 | 94.9% | 100.0% |

The outcome distribution changed from 61 fifty-move failures to zero. The
remaining failures were four minor-piece losses and one stalemate. Fifteen of
nineteen other measured families were identical; the other changes were one or
two positions. Rarog's bench was unchanged, but that bench did not activate the
term, so this is endgame evidence rather than proof from bench identity.

The result demonstrates three coupled requirements:

1. A corner potential needs enough resolution to avoid the broad rings of
   Chebyshev distance.
2. Its move-to-move gradient must survive the engine's search margins.
3. The correct-corner pull must dominate the friendly-king pull. Testing the
   diagonal shape near a 1:1 balance falsely made it look ineffective.

The absolute Rarog constants are not transferable evidence. They are seed
values on Rarog's evaluation and pruning scale only.

## Exact geometry

Let `f` and `r` be the weak king's zero-based file and rank. The winning
bishop selects one of the two corner colour complexes:

| Bishop/correct corners in Basilisk | Diagonal potential `D` | `D=7` |
|---|---|---|
| Dark-squared bishop: a1, h8 | `abs(7 - r - f)` | a1, h8 |
| Light-squared bishop: a8, h1 | `abs(r - f)` | a8, h1 |

`D` is zero on the wrong-corner diagonal and increases toward either correct
corner. It removes the coarse Chebyshev rings, although it does not assign a
unique value to every square or every legal move. The measured claim is
conversion improvement, not mathematical absence of all ties.

Only the bishop's square colour is used. The bishop's location, distance from
either king, mobility and attacked squares do not enter the potential. This is
important because Basilisk already refuted two attempts aimed at its observed
bishop shuffling:

- bishop proximity lost conversion and allowed early bishop captures;
- weak-king escape-square count failed the deterministic KBN-K floor.

Rarog therefore supplies a solution to try without reopening either rejected
feature.

## Basilisk mapping

Basilisk's `kbnk_score()` already receives every required input:

- the exact K+B+N versus bare-K material class is established by
  `apply_endgame()`;
- the winning bishop square selects a1/h8 or a8/h1 correctly;
- the weak king's file and rank are directly available;
- the result is returned in White-score perspective for either winning colour.

The current Basilisk potential is:

```text
KNOWN_WIN
+ 800 * (14 - Manhattan distance to the nearer correct corner)
+ 900 * (3 - distance from an edge)
+ 220 * (8 - king distance)
+ 220 * (8 - knight distance)
```

The initial transfer seam appeared to be the corner geometry inside
`kbnk_score()`. The 6.1.b algebra above shows that geometry is already present.
What remains is coefficient and interaction work inside `kbnk_score()`, not the
material dispatcher, score perspective, bishop placement or generic KXK logic.
Unlike Rarog's broader mop-up gate, Basilisk's exact KBNK dispatch cannot leak
into middlegames, queen mates or rook mates.

## Constraints carried into 6.1.b and 6.1.c

- Keep the now-explicit colour-complex formulas; do not import `360` or another
  Rarog constant.
- Keep the exact KBNK activation and White-score sign unchanged.
- Do not add a bishop-position, bishop-proximity or flight-square term.
- Treat the existing edge, friendly-king and knight pulls as interacting
  forces; 6.1.c must measure whether to retain, reduce or remove each one.
- Bound the maximum non-mate score below Basilisk's mate-score safety region.
- Judge the mechanism on the frozen position cohort with Syzygy WDL/DTZ and
  rule-50 outcomes. Bench identity alone is not an acceptance signal.

This audit establishes that the geometric mechanism is compatible and already
present. It does not predict that Basilisk will reproduce Rarog's 96.9% result.
