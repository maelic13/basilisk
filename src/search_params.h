#pragma once

// Search tuning parameters. Under the BASILISK_TUNE build flag each field is
// exposed as a UCI spin option so an external SPSA tool can tune it. In release
// builds the struct is a plain bag of compile-time constants.
//
// One X-macro line per tunable generates the three sites that used to be
// hand-synced: the struct field with its compiled-in default (this header),
// the TUNE-build UCI option advertisement and the setoption clamp arm (both
// parameters.cpp). A default or range therefore cannot drift between them,
// which once left an option advertising a reverted value while compiling
// another, and a strength-bearing knob registered nowhere. External tune
// vectors are separate files, regenerated from these lines by
// tools/generate_spsa_surface.py.
//
// The lines are grouped into tables by owner:
//
//   BASILISK_ROOT_PARAMS            the driver's: aspiration and time management;
//   BASILISK_LEGACY_KERNEL_PARAMS   the legacy node kernel's;
//   BASILISK_CORE_KERNEL_PARAMS     Cluster 1's kernel, compiled on the B2_CORE
//                                   arm in place of the legacy table.
//
// BASILISK_SEARCH_PARAMS, the table every consumer expands, is the arm's
// kernel table followed by the root table.
//
// X(field, UciName, default, min, max) — one line per tunable.

// ---- The driver: aspiration window and time management ----------------------
//
// The time-management defaults are hand-tuned. An SPSA bake of the budget
// multipliers was reverted after a 12,262-game validation read +0.88 +/- 4.03:
// the manager was at its ceiling for the signals it had. tm_instability then
// added the missing signal (+10.79 +/- 6.13, the largest single gain of the
// 1.9 line): a decaying count of root best-move changes (halved per iteration,
// +1 per flip, capped at 2) scales the soft-limit threshold by
// 1 + changes * tm_instability / 100, so a thrashing root buys up to +70% time.
// tm_opt_mult / tm_max_mult are budget multipliers x100; tm_stability is the
// per-stable-iteration shrink x1000; the scoredrop pair extends on a falling
// evaluation (threshold in evaluation units, ramp divisor); the effort quartet
// scales by the best move's share of the iteration's nodes.
#define BASILISK_ROOT_PARAMS(X)                                          \
    /* Aspiration window */                                              \
    X(aspiration_delta,      AspirationDelta,       19,   10,    60)     \
    /* Time management */                                                \
    X(tm_opt_mult,           TmOptMult,            100,   50,   200)     \
    X(tm_max_mult,           TmMaxMult,            100,   50,   200)     \
    X(tm_stability,          TmStability,           60,    0,   150)     \
    X(tm_scoredrop_thr,      TmScoreDropThr,        30,    5,   120)     \
    X(tm_scoredrop_div,      TmScoreDropDiv,       100,   30,   400)     \
    X(tm_effort_hi,          TmEffortHi,            80,   50,    99)     \
    X(tm_effort_lo,          TmEffortLo,            25,    1,    50)     \
    X(tm_effort_hi_mult,     TmEffortHiMult,        80,   50,   100)     \
    X(tm_effort_lo_mult,     TmEffortLoMult,       120,  100,   200)     \
    X(tm_instability,        TmInstability,         35,    0,   100)

// ---- The legacy node kernel ---------------------------------------------------
//
// Removed mechanisms: capture futility, SEE pruning of quiets, qsearch quiet
//   checks and the post-LMR continuation-history nudge were dead or inert at
//   their defaults (capture futility's `lmr_depth < 1` cannot hold under the
//   default LMR table) and were deleted with their coordinates. A donor form
//   of any of them is a new mechanism, not a re-enabled knob.
//
// double_ext_max caps stacked 2-ply singular extensions so a pathological line
//   cannot chain them without bound. 16 is a live bound the bench suite never
//   reaches; at 1 it shrinks the tree by about 9%, so it is not inert. A seed
//   of 5 broke the KBNK/KQK mate-resolution tests, and caps of 6 and 12 grew
//   the bench by 18.5% and 30%: the double extensions are productive.
//
// LMR base table: lmr_base / lmr_divisor are stored x100 (60 == 0.60,
//   209 == 2.09) and divided by 100.0 in init_lmr.
//
// LMR per-move adjustments are in 1024ths of a ply (1024 == 1 ply); the
//   reduction is accumulated in 1024ths and shifted `>> 10` at the end.
//   lmr_cut_node_adj 401 / lmr_tt_capture 301 / lmr_not_improving_adj 89 are
//   the joint SPSA vector of 2026-07-14; hand seeds of 1024 and 512 broke the
//   canaries eight times. lmr_tt_pv_adj 23 is near noise because the
//   reconstructed tt_pv signal is weak (a persisted TT bit found no good
//   operating point through this route either). The history term stays
//   integer-quantised, divided by lmr_hist_div before the x1024; the
//   continuous form measured worse (BAS-S13).
//
// History updates: bonus = min(quad*d*d/64 + lin*d, max), malus mirrored with
//   its own knobs. The references prove the shape family, but transplanting
//   their constants destabilised the mate tests because every consumer
//   (history pruning, the LMR divisor) was tuned for this scale; the live
//   values are the same 2026-07-14 SPSA vector with its rescaled consumers
//   (HistPruneCoeff 14004, LmrHistDiv 5683). hist_ttmove_bonus is the extra
//   when the best move was the TT move.
//
// Checking moves are extended and never reduced, deliberately: a path cap on
//   the check extension was rejected in play (BAS-S16, -3.48 +/- 3.32), and
//   reducing at checking nodes failed the WAC floor (137 -> 124, floor 130).
//
// The table is split into groups so that the B2_CORE arm's table can take
// the groups whose mechanisms it still runs at their legacy values; the
// legacy table is the groups in their original order.
#define BASILISK_LEGACY_NODE_MARGINS(X)                                  \
    /* Reverse futility pruning */                                       \
    X(rfp_coeff,             RfpCoeff,             160,   60,   240)     \
    X(rfp_improving,         RfpImproving,          72,    0,   140)     \
    /* Razoring */                                                       \
    X(razor_coeff,           RazorCoeff,           243,  120,   500)

#define BASILISK_LEGACY_NULL_PROBCUT(X)                                  \
    /* Null-move pruning */                                              \
    X(null_base,             NullBase,               3,    2,     6)     \
    X(null_eval_div,         NullEvalDiv,          192,   80,   400)     \
    /* ProbCut */                                                        \
    X(probcut_margin,        ProbCutMargin,        189,   80,   360)

#define BASILISK_LEGACY_MOVE_LOOP(X)                                     \
    /* Move-loop futility */                                             \
    X(futility_base,         FutilityBase,         180,   40,   280)     \
    X(futility_coeff,        FutilityCoeff,        128,   40,   200)     \
    /* History pruning */                                                \
    X(hist_prune_coeff,      HistPruneCoeff,     14004, 1000, 28000)     \
    /* SEE pruning (bad captures) */                                     \
    X(see_prune_coeff,       SeePruneCoeff,         73,   30,   160)

#define BASILISK_LEGACY_SINGULAR(X)                                      \
    /* Singular extension */                                             \
    X(singular_min_depth,    SingularMinDepth,       5,    4,     8)     \
    X(singular_beta_mult,    SingularBetaMult,       4,    1,     6)     \
    X(singular_double_margin, SingularDoubleMargin,  4,    0,    60)     \
    X(double_ext_max,        DoubleExtMax,          16,    1,   200)

#define BASILISK_LEGACY_LMR(X)                                           \
    /* LMR base table (x100) */                                          \
    X(lmr_base,              LmrBase,               60,    0,   150)     \
    X(lmr_divisor,           LmrDivisor,           209,  150,   350)     \
    /* LMR per-move adjustments (1024ths) */                             \
    X(lmr_hist_div,          LmrHistDiv,          5683, 4096, 16384)     \
    X(lmr_non_pv_adj,        LmrNonPvAdj,         1024,    0,  3072)     \
    X(lmr_cut_node_adj,      LmrCutNodeAdj,        401,    0,  3072)     \
    X(lmr_tt_pv_adj,         LmrTtPvAdj,            23,    0,  3072)     \
    X(lmr_not_improving_adj, LmrNotImprovingAdj,    89,    0,  3072)     \
    X(lmr_tt_capture,        LmrTtCapture,         301,    0,  3072)

#define BASILISK_LEGACY_LMR_SINGULAR(X)                                  \
    X(lmr_singular_quiet,    LmrSingularQuiet,     401,    0,  3072)

#define BASILISK_LEGACY_HISTORY(X)                                       \
    /* History updates */                                                \
    X(hist_bonus_quad,       HistBonusQuad,         62,    0,   128)     \
    X(hist_bonus_lin,        HistBonusLin,         120,    0,   400)     \
    X(hist_bonus_max,        HistBonusMax,        1863,  512,  4096)     \
    X(hist_malus_quad,       HistMalusQuad,         62,    0,   128)     \
    X(hist_malus_lin,        HistMalusLin,         143,    0,   400)     \
    X(hist_malus_max,        HistMalusMax,        1304,  512,  4096)     \
    X(hist_ttmove_bonus,     HistTtMoveBonus,       29,    0,  1024)

#define BASILISK_LEGACY_KERNEL_PARAMS(X) \
    BASILISK_LEGACY_NODE_MARGINS(X)      \
    BASILISK_LEGACY_NULL_PROBCUT(X)      \
    BASILISK_LEGACY_MOVE_LOOP(X)         \
    BASILISK_LEGACY_SINGULAR(X)          \
    BASILISK_LEGACY_LMR(X)               \
    BASILISK_LEGACY_LMR_SINGULAR(X)      \
    BASILISK_LEGACY_HISTORY(X)

// ---- Cluster 1's kernel (the B2_CORE arm) -------------------------------------
//
// The core kernel reads the legacy coordinates of every mechanism it has not
// yet replaced, at their legacy values, and its own coordinates at the
// cluster's seeds. Units are evaluation units unless a line says otherwise.
//
// Node margins: razoring at depth <= CoreRazorDepthCap returns quiescence
//   when the estimate is below alpha - margin * depth. Reverse futility at
//   depth <= its cap returns (w * beta + (1024 - w) * eval) / 1024 when the
//   estimate clears beta by per_ply * depth - improving - worsening (+ the
//   correction in evaluation units times corr_mult/1024; 0 turns that term
//   off).
// Move-loop pruning reads a move's reduced depth, lmrDepth: quiets stop
//   after (base + d^2 * square_scale/16) / (2 - improving) moves; captures and
//   checks are pruned by futility (base + slope * lmrDepth + captured value +
//   capture history / div, below lmrDepth cap) and by SEE below
//   -(coeff * depth + capture history / div); quiets at non-PV nodes by
//   continuation history below -slope * depth, by futility (base + slope *
//   lmrDepth + alpha_term when the eval is above alpha, below lmrDepth cap)
//   and by SEE below -coeff * lmrDepth^2.
// LMR: the base reduction is R[d] * R[moves] with R[i] = table_scale/128 *
//   ln(i), less 577/1024 of the window's share of the root window, more by
//   not_improving/512 of the product when not improving, plus the offset;
//   per-move terms add or subtract in 1024ths (tt_pv, cut node, TT capture,
//   the children's cutoff count, the history score over 4096/stat_scale,
//   alpha above the estimate, all nodes). The alpha term is alpha_eval_scale
//   per unit of (alpha - estimate) scaled to the donor's evaluation units
//   (x 1000/282), clamped to -64..96 after scaling. A reduced search that
//   fails high is re-searched a ply deeper above the best by
//   `research_deeper`, a ply shallower below it plus `research_shallower`.
// History updates: a move that raises alpha earns min(slope * depth - 81,
//   cap) (more for the TT move and the parent's history score); the moves
//   searched before it lose min(slope * depth - 235, cap), each earlier quiet
//   decay/1024 less than the next. Between searches the main history keeps
//   main_hist_decay/1024 of each entry.
// The TT-cutoff bonus trains the quiet histories by slope * depth (history
//   units per ply) when a quiet TT move's entry cuts.
// Correction training: the bonus is clamp(error * depth * scale / 128, +/-256)
//   in 128ths, with scale * 3/2 when no move raised alpha; the continuation
//   correction's weight is in 131072ths of the summed rows.
#define BASILISK_CORE_KERNEL_PARAMS(X)                                   \
    BASILISK_LEGACY_NULL_PROBCUT(X)                                      \
    BASILISK_LEGACY_SINGULAR(X)                                          \
    BASILISK_LEGACY_LMR_SINGULAR(X)                                      \
    /* Node margins */                                                   \
    X(razor_margin,          CoreRazorMargin,       256,  128,   527)    \
    X(rfp_per_ply,           CoreRfpPerPly,         133,   40,   227)    \
    X(rfp_improving,         CoreRfpImproving,       89,   35,   160)    \
    X(rfp_worsening,         CoreRfpWorsening,       11,    0,    60)    \
    X(rfp_corr_mult,         CoreRfpCorrMult,         0,    0,  2048)    \
    X(rfp_depth_cap,         CoreRfpDepthCap,         9,    6,    19)    \
    X(rfp_beta_weight,       CoreRfpBetaWeight,     661,  512,  1024)    \
    X(iir_min_depth,         CoreIirMinDepth,         4,    3,     8)    \
    /* Move-loop pruning */                                              \
    X(lmp_base,              CoreLmpBase,             3,    1,     8)    \
    X(lmp_square_scale,      CoreLmpSquareScale,     16,    8,    32)    \
    X(quiet_fut_base,        CoreQuietFutBase,      158,   80,   300)    \
    X(quiet_fut_slope,       CoreQuietFutSlope,     108,   34,   190)    \
    X(quiet_fut_alpha_term,  CoreQuietFutAlphaTerm,  25,    0,    90)    \
    X(quiet_fut_depth_cap,   CoreQuietFutDepthCap,    8,    5,    12)    \
    X(capt_fut_base,         CoreCaptFutBase,        92,   40,   270)    \
    X(capt_fut_slope,        CoreCaptFutSlope,      115,   60,   390)    \
    X(capt_fut_hist_div,     CoreCaptFutHistDiv,     27,    8,   128)    \
    X(capt_fut_depth_cap,    CoreCaptFutDepthCap,     7,    4,     8)    \
    X(cont_hist_prune_slope, CoreContHistPruneSlope, 4136, 1500, 8000)   \
    X(quiet_see_coeff,       CoreQuietSeeCoeff,      10,    4,    30)    \
    X(capt_see_coeff,        CoreCaptSeeCoeff,       74,   30,   200)    \
    X(capt_see_hist_div,     CoreCaptSeeHistDiv,     80,   20,   200)    \
    /* Late-move reductions (1024ths of a ply) and re-search */          \
    X(lmr_table_scale,       CoreLmrTableScale,    2872, 1500,  4500)    \
    X(lmr_offset,            CoreLmrOffset,         982,    0,  2000)    \
    X(lmr_not_improving,     CoreLmrNotImproving,   197,    0,   500)    \
    X(lmr_tt_pv,             CoreLmrTtPv,           929,    0,  2000)    \
    X(lmr_tt_pv_base,        CoreLmrTtPvBase,      3023,    0,  6000)    \
    X(lmr_tt_pv_pv_node,     CoreLmrTtPvPvNode,    1004,    0,  2500)    \
    X(lmr_tt_pv_above_alpha, CoreLmrTtPvAboveAlpha, 885,    0,  2000)    \
    X(lmr_tt_pv_depth_ok,    CoreLmrTtPvDepthOk,    816,    0,  2000)    \
    X(lmr_cut_node,          CoreLmrCutNode,       4026, 1500,  7000)    \
    X(lmr_cut_node_no_tt,    CoreLmrCutNodeNoTt,    933,    0,  2500)    \
    X(lmr_tt_capture,        CoreLmrTtCapture,     1079,    0,  2500)    \
    X(lmr_cutoff_cnt,        CoreLmrCutoffCnt,      264,    0,  1000)    \
    X(lmr_cutoff_cnt2,       CoreLmrCutoffCnt2,    1095,    0,  2500)    \
    X(lmr_cutoff_cnt_all,    CoreLmrCutoffCntAll,  1138,    0,  2500)    \
    X(lmr_stat_scale,        CoreLmrStatScale,      439,  100,  1200)    \
    X(lmr_alpha_eval_scale,  CoreLmrAlphaEvalScale,   3,    0,    10)    \
    X(lmr_all_node_scale,    CoreLmrAllNodeScale,   276,    0,   800)    \
    X(research_deeper,       CoreResearchDeeper,     15,    4,    40)    \
    X(research_shallower,    CoreResearchShallower,   2,    0,    12)    \
    /* History update policy */                                          \
    X(quiet_bonus_cap,       CoreQuietBonusCap,    1487,  600,  3000)    \
    X(quiet_bonus_slope,     CoreQuietBonusSlope,   133,   60,   300)    \
    X(malus_cap,             CoreMalusCap,         2244,  900,  4500)    \
    X(malus_slope,           CoreMalusSlope,        968,  400,  2000)    \
    X(malus_decay,           CoreMalusDecay,        921,  700,  1024)    \
    X(main_hist_decay,       CoreMainHistDecay,     729,  400,  1024)    \
    /* TT cutoff training */                                             \
    X(tt_cutoff_bonus_slope, CoreTtCutoffBonusSlope, 131,    0,   300)   \
    /* Correction */                                                     \
    X(corr_update_scale,     CoreCorrUpdateScale,    12,    4,    30)     \
    X(corr_cont_weight,      CoreCorrContWeight,   8761, 2000, 16000)

// Categorical switches of Cluster 1's kernel: S(field, UciName, default, min,
// max). Tune builds advertise them as options so a paired run can settle each
// on one binary; they are never SPSA coordinates, which is why they use S and
// not X (tools/generate_spsa_surface.py reads X lines only).
//
// tt_cutoff_node_typed: 1 admits a shallow TT cutoff only where the score
//   agrees with the node's expected type (cut node and fail high, or all node
//   and fail low); 0 admits it on bound and depth alone.
// pawn_hist_buckets_log2: the pawn history's bucket count as a power of two
//   (11 = 2,048, 13 = 8,192); a size, settled by speed, not by games.
// razor_depth_cap: the deepest node razoring runs at (1 = the oracle's form,
//   3 = the legacy reach).
// lmp_exempt_checks: 1 keeps delivering quiet moves that give check from
//   their destination after the move-count rule ends quiet delivery; 0,
//   the default and the donor's form, delivers no quiet after it.
// lmr_negative_allowed: 1 lets a negative reduction extend a reduced search
//   up to two plies beyond the full depth; 0 caps it at the full depth.
// corr_admission: 1 trains the correction where the error's sign agrees with
//   the bound (above the static eval exactly when a move raised alpha); 0
//   where the node failed high or raised alpha.
#define BASILISK_CORE_KERNEL_SWITCHES(S)                                 \
    S(tt_cutoff_node_typed,  CoreTtCutoffNodeTyped,  1,    0,     1)     \
    S(pawn_hist_buckets_log2, CorePawnHistBucketsLog2, 13,  11,    13)   \
    S(corr_admission,        CoreCorrAdmission,      1,    0,     1)     \
    S(razor_depth_cap,       CoreRazorDepthCap,      1,    1,     3)     \
    S(lmp_exempt_checks,     CoreLmpExemptChecks,    0,    0,     1)     \
    S(lmr_negative_allowed,  CoreLmrNegativeAllowed, 1,    0,     1)

#if defined(BASILISK_B2_CORE)
#define BASILISK_KERNEL_PARAMS(X) BASILISK_CORE_KERNEL_PARAMS(X)
#define BASILISK_KERNEL_SWITCHES(S) BASILISK_CORE_KERNEL_SWITCHES(S)
#else
#define BASILISK_KERNEL_PARAMS(X) BASILISK_LEGACY_KERNEL_PARAMS(X)
#define BASILISK_KERNEL_SWITCHES(S)
#endif

#define BASILISK_SEARCH_PARAMS(X) \
    BASILISK_KERNEL_PARAMS(X)     \
    BASILISK_ROOT_PARAMS(X)

struct SearchParams {
#define BASILISK_SEARCH_PARAM_FIELD(field, uci, def, lo, hi) int field = def;
    BASILISK_SEARCH_PARAMS(BASILISK_SEARCH_PARAM_FIELD)
    BASILISK_KERNEL_SWITCHES(BASILISK_SEARCH_PARAM_FIELD)
#undef BASILISK_SEARCH_PARAM_FIELD
};
