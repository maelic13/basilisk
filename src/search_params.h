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
#define BASILISK_LEGACY_KERNEL_PARAMS(X)                                 \
    /* Reverse futility pruning */                                       \
    X(rfp_coeff,             RfpCoeff,             160,   60,   240)     \
    X(rfp_improving,         RfpImproving,          72,    0,   140)     \
    /* Razoring */                                                       \
    X(razor_coeff,           RazorCoeff,           243,  120,   500)     \
    /* Null-move pruning */                                              \
    X(null_base,             NullBase,               3,    2,     6)     \
    X(null_eval_div,         NullEvalDiv,          192,   80,   400)     \
    /* ProbCut */                                                        \
    X(probcut_margin,        ProbCutMargin,        189,   80,   360)     \
    /* Move-loop futility */                                             \
    X(futility_base,         FutilityBase,         180,   40,   280)     \
    X(futility_coeff,        FutilityCoeff,        128,   40,   200)     \
    /* History pruning */                                                \
    X(hist_prune_coeff,      HistPruneCoeff,     14004, 1000, 28000)     \
    /* SEE pruning (bad captures) */                                     \
    X(see_prune_coeff,       SeePruneCoeff,         73,   30,   160)     \
    /* Singular extension */                                             \
    X(singular_min_depth,    SingularMinDepth,       5,    4,     8)     \
    X(singular_beta_mult,    SingularBetaMult,       4,    1,     6)     \
    X(singular_double_margin, SingularDoubleMargin,  4,    0,    60)     \
    X(double_ext_max,        DoubleExtMax,          16,    1,   200)     \
    /* LMR base table (x100) */                                          \
    X(lmr_base,              LmrBase,               60,    0,   150)     \
    X(lmr_divisor,           LmrDivisor,           209,  150,   350)     \
    /* LMR per-move adjustments (1024ths) */                             \
    X(lmr_hist_div,          LmrHistDiv,          5683, 4096, 16384)     \
    X(lmr_non_pv_adj,        LmrNonPvAdj,         1024,    0,  3072)     \
    X(lmr_cut_node_adj,      LmrCutNodeAdj,        401,    0,  3072)     \
    X(lmr_tt_pv_adj,         LmrTtPvAdj,            23,    0,  3072)     \
    X(lmr_not_improving_adj, LmrNotImprovingAdj,    89,    0,  3072)     \
    X(lmr_tt_capture,        LmrTtCapture,         301,    0,  3072)     \
    X(lmr_singular_quiet,    LmrSingularQuiet,     401,    0,  3072)     \
    /* History updates */                                                \
    X(hist_bonus_quad,       HistBonusQuad,         62,    0,   128)     \
    X(hist_bonus_lin,        HistBonusLin,         120,    0,   400)     \
    X(hist_bonus_max,        HistBonusMax,        1863,  512,  4096)     \
    X(hist_malus_quad,       HistMalusQuad,         62,    0,   128)     \
    X(hist_malus_lin,        HistMalusLin,         143,    0,   400)     \
    X(hist_malus_max,        HistMalusMax,        1304,  512,  4096)     \
    X(hist_ttmove_bonus,     HistTtMoveBonus,       29,    0,  1024)

// ---- Cluster 1's kernel (the B2_CORE arm) -------------------------------------
//
// Filled by the cluster's implementation; empty until then, so the ON arm
// compiles with the root table alone.
#define BASILISK_CORE_KERNEL_PARAMS(X)

#if defined(BASILISK_B2_CORE)
#define BASILISK_KERNEL_PARAMS(X) BASILISK_CORE_KERNEL_PARAMS(X)
#else
#define BASILISK_KERNEL_PARAMS(X) BASILISK_LEGACY_KERNEL_PARAMS(X)
#endif

#define BASILISK_SEARCH_PARAMS(X) \
    BASILISK_KERNEL_PARAMS(X)     \
    BASILISK_ROOT_PARAMS(X)

struct SearchParams {
#define BASILISK_SEARCH_PARAM_FIELD(field, uci, def, lo, hi) int field = def;
    BASILISK_SEARCH_PARAMS(BASILISK_SEARCH_PARAM_FIELD)
#undef BASILISK_SEARCH_PARAM_FIELD
};
