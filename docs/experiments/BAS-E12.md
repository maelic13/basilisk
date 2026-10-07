# BAS-E12

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E12 — recovering the NPS the new terms cost** (2026-08-26, speed only,
behaviour-identical). Bench stays **18,228,447** across the change and CTest is
12/12, so this is a pure throughput measurement, not a behaviour change.

The 4.1% NPS that BAS-E11 attributes to the 5.9.1/5.9.2 term code was paid from
the moment those terms landed and was **never measured** — 5.9.1 and 5.9.2 both
recorded "bench unchanged", which is a *node-count* identity and says nothing
about speed. Three fixes, all exact rather than approximate:

1. `bishop_xray_pawn` called `bishop_attacks(sq, 0ULL)` per bishop per eval. The
   empty-board ray set is a per-square **constant**; tabled once in
   `init_eval_tables`.
2. `trapped_rook` recomputed `rook_attacks(rsq, b.all_occ)` for a square whose
   attack set the attack-map substrate already caches in `slider_att[]` — the
   same 8.7.7(a) substitution already made for the king-ring and connected-rook
   probes.
3. `slider_on_queen` did four magic lookups whenever the enemy held a queen,
   including when we held no slider that could be counted. Now gated on holding
   the piece, each half independently. Exact: the popcount was zero regardless.

NPS **3.208M → 3.299M, +2.8%**, leaving 1.8% against pre-5.9.1's 3.358M. The
tuner still reconstructs all 10,000 verification positions exactly.

*Not done, and why.* `bishop_outpost` sits **before** the lazy checkpoint, so it
is paid on every eval including skips. Moving it after would recover more, but
it would change the lazy score and therefore behaviour — that is an SPRT-gated
change, not a free one.

*Second pass: zero-guards, after reverting the values.* With 5.9.6 rejected and
every new coefficient back at 0, an **interleaved** measurement (BAS-M06
protocol; the earlier non-interleaved reading understated this) put the retained
term code at **−3.73%** NPS against pre-5.9.1 — speed spent on arithmetic
multiplied by zero. Six terms now skip when both their coefficients are zero,
via `EVAL_TERM_ACTIVE`. The macro is **always true under `TEXEL_TRACE`**: the
tuner needs a term's feature counts precisely when its coefficient is 0, since
that is the state it fits from, so gating the trace would break `--verify` and
the fit itself. Deficit **−3.73% → −1.37%**; `--verify` still exact on 10,000
positions.

The residual −1.37% is the standing price of retaining the structure for
5.9.12's ablation. If the terms still measure inert once the PSTs are free, they
should be removed and that comes back.
