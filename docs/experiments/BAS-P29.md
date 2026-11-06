# BAS-P29

**BAS-P29 — the evaluation's material, SEE's exchange and the quiet stage made cheaper**

- **Registration.** 2026-11-06, B.4.5, on the maintainer's instruction after BAS-S50's
  P9 falsifier fired. Agent, on the idle host the maintainer gave. Behaviour-neutral:
  `bench 13` OFF 6,090,091, arm 4,003,560, unchanged.
- **Question.** How much speed can be recovered at an identical tree?
- **Arms.** Four-build final-PGO pools, clang 23.1.5, PEXT: `b45-on` and `b45-off`
  at `abc3b0f`, `b45s-on` and `b45s-off` at `516d422`.
- **Changes.** `1ee67b7`: the material imbalance from a table keyed on the ten piece
  counts, and material with piece-square values in one walk of the occupied squares.
  `1990343`: `score_quiets`' attack maps wait for the first piece that uses them, and
  the sort's scan does not branch on the score. `516d422`: `see_ge` computes its
  attackers once at entry and then only the line a capture opens.

#### RESULT

`tools/nps_read.ps1`, four pools, two cycles, host at 1% or less, no cycle disturbed
(`tools/results/b45s-nps`):

| Contrast | Delta | 95% interval | Verdict |
|---|---:|---|---|
| The arm, `b45s-on` over `b45-on`, same tree | +3.99% | [+3.84%, +4.14%] | ACCEPT |
| The OFF arm, `b45s-off` over `b45-off`, same tree | +4.16% | [+4.00%, +4.33%] | ACCEPT |
| The arm over the OFF arm at `516d422` | −5.49% | [−5.61%, −5.37%] | two trees, not pure speed |

Game-like at Hash 64 over 40 positions, the arm reads +3.66% with identical node
totals. Eight other candidates read within noise or below it; the packet's *B.4.5 —
the speed work* lists all thirteen.

- **Disposition.** ACCEPTED: kept on both arms.
- **Lesson.** The gain was recomputation the profile showed by line, a value that
  depends on piece counts alone. Skipping PEXT lookups behind a branch loses.
- **Artifacts.** `tools/results/b45s-nps`, `tools/results/b45-20261106/prof/`, `speed/`.
