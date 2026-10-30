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
//   BASILISK_CORE_KERNEL_PARAMS     the node kernel's, which includes the
//                                   groups it inherited from the kernel it
//                                   replaced (the BASILISK_LEGACY_* groups).
//
// BASILISK_SEARCH_PARAMS, the table every consumer expands, is the kernel
// table followed by the root table.
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

// ---- Groups inherited from the replaced kernel ------------------------------
//
// The node kernel still runs three groups of the kernel it replaced at their
// old values; they keep their old names.
//
// double_ext_max caps stacked 2-ply singular extensions so a pathological line
//   cannot chain them without bound. 16 is a live bound the bench suite never
//   reaches; at 1 it shrinks the tree by about 9%, so it is not inert. A seed
//   of 5 broke the KBNK/KQK mate-resolution tests, and caps of 6 and 12 grew
//   the bench by 18.5% and 30%: the double extensions are productive.
//
// Checking moves are extended and never reduced, deliberately: a path cap on
//   the check extension was rejected in play (BAS-S16, -3.48 +/- 3.32), and
//   reducing at checking nodes failed the WAC floor (137 -> 124, floor 130).
#define BASILISK_LEGACY_NULL_PROBCUT(X)                                  \
    /* Null-move pruning */                                              \
    X(null_base,             NullBase,               3,    2,     6)     \
    X(null_eval_div,         NullEvalDiv,          192,   80,   400)     \
    /* ProbCut */                                                        \
    X(probcut_margin,        ProbCutMargin,        189,   80,   360)

#define BASILISK_LEGACY_SINGULAR(X)                                      \
    /* Singular extension */                                             \
    X(singular_min_depth,    SingularMinDepth,       5,    4,     8)     \
    X(singular_beta_mult,    SingularBetaMult,       4,    1,     6)     \
    X(singular_double_margin, SingularDoubleMargin,  4,    0,    60)     \
    X(double_ext_max,        DoubleExtMax,          16,    1,   200)

#define BASILISK_LEGACY_LMR_SINGULAR(X)                                  \
    X(lmr_singular_quiet,    LmrSingularQuiet,     401,    0,  3072)

// ---- The node kernel (Cluster 1, the selectivity core) ----------------------
//
// The kernel reads the inherited groups above for the mechanisms it has not
// replaced, and its own coordinates at the cluster's seeds. Units are evaluation units unless a line says otherwise.
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
//
// F lines are coordinates the kernel reads and a tune build advertises but no
// tune fits: their response is a staircase a tune's step cannot resolve.
// Every consumer expands them as X (BASILISK_KERNEL_PARAMS passes X twice);
// tools/generate_spsa_surface.py reads X lines only, so they stay off the
// SPSA surface. quiet_see_coeff: SEE values here are multiples of 100, so
// neighbouring coefficients prune the same quiets at every common lmrDepth.
#define BASILISK_CORE_KERNEL_PARAMS(X, F)                                \
    BASILISK_LEGACY_NULL_PROBCUT(X)                                      \
    BASILISK_LEGACY_SINGULAR(X)                                          \
    BASILISK_LEGACY_LMR_SINGULAR(X)                                      \
    /* Node margins */                                                   \
    X(razor_margin,          CoreRazorMargin,       269,  128,   527)    \
    X(rfp_per_ply,           CoreRfpPerPly,         101,   40,   227)    \
    X(rfp_improving,         CoreRfpImproving,       98,   35,   160)    \
    X(rfp_worsening,         CoreRfpWorsening,       13,    0,    60)    \
    X(rfp_corr_mult,         CoreRfpCorrMult,       538,    0,  2048)    \
    X(rfp_depth_cap,         CoreRfpDepthCap,         10,    6,    19)    \
    X(rfp_beta_weight,       CoreRfpBetaWeight,     660,  512,  1024)    \
    X(iir_min_depth,         CoreIirMinDepth,         8,    3,     8)    \
    /* Move-loop pruning */                                              \
    X(lmp_base,              CoreLmpBase,             3,    1,     8)    \
    X(lmp_square_scale,      CoreLmpSquareScale,     16,    8,    32)    \
    X(quiet_fut_base,        CoreQuietFutBase,      166,   80,   300)    \
    X(quiet_fut_slope,       CoreQuietFutSlope,     107,   34,   190)    \
    X(quiet_fut_alpha_term,  CoreQuietFutAlphaTerm,  29,    0,    90)    \
    X(quiet_fut_depth_cap,   CoreQuietFutDepthCap,    8,    5,    12)    \
    X(capt_fut_base,         CoreCaptFutBase,        79,   40,   270)    \
    X(capt_fut_slope,        CoreCaptFutSlope,      105,   60,   390)    \
    X(capt_fut_hist_div,     CoreCaptFutHistDiv,     15,    8,   128)    \
    X(capt_fut_depth_cap,    CoreCaptFutDepthCap,     6,    4,     8)    \
    X(cont_hist_prune_slope, CoreContHistPruneSlope, 3825, 1500, 8000)   \
    F(quiet_see_coeff,       CoreQuietSeeCoeff,      10,    4,    30)    \
    X(capt_see_coeff,        CoreCaptSeeCoeff,       68,   30,   200)    \
    X(capt_see_hist_div,     CoreCaptSeeHistDiv,     76,   20,   200)    \
    /* Late-move reductions (1024ths of a ply) and re-search */          \
    X(lmr_table_scale,       CoreLmrTableScale,    2862, 1500,  4500)    \
    X(lmr_offset,            CoreLmrOffset,         1000,    0,  2000)    \
    X(lmr_not_improving,     CoreLmrNotImproving,   201,    0,   500)    \
    X(lmr_tt_pv,             CoreLmrTtPv,           879,    0,  2000)    \
    X(lmr_tt_pv_base,        CoreLmrTtPvBase,      3105,    0,  6000)    \
    X(lmr_tt_pv_pv_node,     CoreLmrTtPvPvNode,     915,    0,  2500)    \
    X(lmr_tt_pv_above_alpha, CoreLmrTtPvAboveAlpha, 775,    0,  2000)    \
    X(lmr_tt_pv_depth_ok,    CoreLmrTtPvDepthOk,    841,    0,  2000)    \
    X(lmr_cut_node,          CoreLmrCutNode,       3997, 1500,  7000)    \
    X(lmr_cut_node_no_tt,    CoreLmrCutNodeNoTt,    864,    0,  2500)    \
    X(lmr_tt_capture,        CoreLmrTtCapture,      964,    0,  2500)    \
    X(lmr_cutoff_cnt,        CoreLmrCutoffCnt,      241,    0,  1000)    \
    X(lmr_cutoff_cnt2,       CoreLmrCutoffCnt2,     996,    0,  2500)    \
    X(lmr_cutoff_cnt_all,    CoreLmrCutoffCntAll,   910,    0,  2500)    \
    X(lmr_stat_scale,        CoreLmrStatScale,      526,  100,  1200)    \
    X(lmr_alpha_eval_scale,  CoreLmrAlphaEvalScale,   2,    0,    10)    \
    X(lmr_all_node_scale,    CoreLmrAllNodeScale,   247,    0,   800)    \
    X(research_deeper,       CoreResearchDeeper,     13,    4,    40)    \
    X(research_shallower,    CoreResearchShallower,   1,    0,    12)    \
    /* History update policy */                                          \
    X(quiet_bonus_cap,       CoreQuietBonusCap,    1394,  600,  3000)    \
    X(quiet_bonus_slope,     CoreQuietBonusSlope,   151,   60,   300)    \
    X(malus_cap,             CoreMalusCap,         2253,  900,  4500)    \
    X(malus_slope,           CoreMalusSlope,        829,  400,  2000)    \
    X(malus_decay,           CoreMalusDecay,        916,  700,  1024)    \
    X(main_hist_decay,       CoreMainHistDecay,     749,  400,  1024)    \
    /* TT cutoff training */                                             \
    X(tt_cutoff_bonus_slope, CoreTtCutoffBonusSlope, 151,    0,   300)   \
    /* Correction */                                                     \
    X(corr_update_scale,     CoreCorrUpdateScale,    16,    4,    30)     \
    X(corr_cont_weight,      CoreCorrContWeight,   9649, 2000, 16000)

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

// ---- Cluster 2 (the B3_PROOF arm) -------------------------------------------
//
// B(X, F, S): X the coordinates at the packet's seeds, F the donor's fixed
// terms (read and advertised, never on a surface), S the categoricals and
// attribution switches. Every arm compiles the fields so the kernel's
// `if constexpr` branches compile on both; only the B3_PROOF arm reads them,
// and tune builds advertise them there alone.
//
// b3_check_ext: 1 extends every in-check node by a ply (the legacy policy);
//   0, the donor's, extends none.
// b3_check_prune: 1 admits in-check nodes to LMR, the first-move step-down,
//   the depth step after alpha rises and move-loop pruning (quiet futility
//   excepted), as the donor does; 0 reduces and prunes none in check.
#define BASILISK_B3_PARAMS(X, F, S)                                      \
    S(b3_check_ext,          B3CheckExt,             0,    0,     1)     \
    S(b3_check_prune,        B3CheckPrune,           1,    0,     1)

#define BASILISK_KERNEL_PARAMS(X) BASILISK_CORE_KERNEL_PARAMS(X, X)
#define BASILISK_KERNEL_SWITCHES(S) BASILISK_CORE_KERNEL_SWITCHES(S)

#define BASILISK_SEARCH_PARAMS(X) \
    BASILISK_KERNEL_PARAMS(X)     \
    BASILISK_ROOT_PARAMS(X)

struct SearchParams {
#define BASILISK_SEARCH_PARAM_FIELD(field, uci, def, lo, hi) int field = def;
    BASILISK_SEARCH_PARAMS(BASILISK_SEARCH_PARAM_FIELD)
    BASILISK_KERNEL_SWITCHES(BASILISK_SEARCH_PARAM_FIELD)
    BASILISK_B3_PARAMS(BASILISK_SEARCH_PARAM_FIELD, BASILISK_SEARCH_PARAM_FIELD,
                       BASILISK_SEARCH_PARAM_FIELD)
#undef BASILISK_SEARCH_PARAM_FIELD
};
