#pragma once

// Search tuning parameters. Under the BASILISK_TUNE build flag each field is
// exposed as a UCI spin option so an external SPSA tool can tune it. In release
// builds the struct is a plain bag of compile-time constants.
//
// SINGLE SOURCE OF TRUTH (Phase 8.6.1, 2026-07-20): the X-macro table below
// generates all three hand-synced sites from one line per tunable —
//   (1) the struct field with its compiled-in default   (this header),
//   (2) the TUNE-build UCI option advertisement          (Parameters.cpp),
//   (3) the TUNE-build setoption clamp arm               (Parameters.cpp),
// so a default or range can never again drift between them. The 8.6.1 audit
// found exactly that drift: PostLmrHistScale advertised its reverted SPSA
// value (104) while compiling 0, and tm_instability — the +10.79 knob — was
// registered nowhere, i.e. permanently untunable. Rarog fixed the same
// disease (12 stale defaults) with its `params!` macro; this is the C++
// equivalent. External tune vectors necessarily stay separate files —
// regenerate them FROM THIS TABLE before a new tune.
//
// ============================== FIELD NOTES ==============================
// Rationale & history per group; the table itself stays scannable. Do not
// delete these when editing values — they are the record of what was tried.
//
// Removed mechanisms: capture futility, SEE pruning of quiets, qsearch quiet
//   checks and the post-LMR continuation-history nudge were dead or inert at
//   their defaults (capture futility's `lmr_depth < 1` cannot hold under the
//   default LMR table) and were deleted with their coordinates. A donor form
//   of any of them is a new mechanism, not a re-enabled knob.
//
// Singular extension / double-extension cap:
//   double_ext_max caps stacked 2-ply singular extensions (Weiss-style) so a
//   pathological line can't chain unbounded double-extensions. The default
//   16 is a live bound that the bench suite never reaches; at 1 it shrinks
//   the tree by about 9%, so it is not inert. Weiss's seed (5) broke the
//   KBNK/KQK mate-resolution CTests, and caps of 6 and 12 grew the bench by
//   18.5% and 30%: the double extensions are productive.
//
// LMR base table: lmr_base/lmr_divisor are stored x100 (60 == 0.60,
//   209 == 2.09) and divided by 100.0 in init_lmr.
//
// LMR per-move adjustments (Phase 6.7: in 1024ths of a ply):
//   The *_adj knobs and lmr_tt_capture are fractional (1024 == 1 ply); the
//   reduction is accumulated in 1024ths and shifted `>> 10` at the end.
//   lmr_cut_node_adj 401 / lmr_tt_capture 301 / lmr_not_improving_adj 89:
//   hcefinal SPSA 2026-07-14 -- hand seeds 1024/512 broke canaries eight
//   times; the JOINT tune landed values hand-seeding never could.
//   lmr_tt_pv_adj 23 is near-noise (the reconstructed tt_pv signal is weak
//   until the TT-PV bit lands; re-check at 10.7 -- the 8.5.7 re-test showed
//   the persisted bit has NO good operating point through the LMR route).
//   lmr_hist_div: history still integer-quantised; see search.cpp.
//
// History updates (Phase 6.3):
//   bonus = min((quad*d*d)/64 + lin*d, max); malus mirrored with its own
//   knobs. The references prove the *shape family* (SF: linear asymmetric
//   134d-79/1572 vs 1005d-205/2218; Weiss: 251d-267/2418 vs 532d-163/693)
//   but transplanting their constants destabilised the mate CTests because
//   every consumer (hist pruning, LMR hist div) was tuned for our scale.
//   The live asymmetric-linear values are the hcefinal SPSA vector
//   (BonusLin 120 / MalusLin 143 / MalusMax 1304 + rescaled consumers
//   HistPruneCoeff 14004, LmrHistDiv 5683). hist_ttmove_bonus 29: extra
//   when best == tt_move (SF-style).
//
// Time management (Phase 5 + 8.5.12):
//   Hand-tuned defaults. The 5.8 SPSA bake was REVERTED after the 5.9
//   validation wash (+0.88 +/- 4.03 over 12,262 games) -- the TM was at its
//   ceiling FOR THE EXISTING SIGNALS. 8.5.12 then added the missing signal:
//   tm_instability (2026-07-17, SPRT +10.79 +/- 6.13, the largest single
//   pre-1.9.0 gain) -- a decaying best_move_changes (SF totBestMoveChanges,
//   x0.5/iter, +1 per root flip, capped at 2) scales the soft-limit
//   threshold by 1 + changes * tm_instability/100, so a thrashing root buys
//   up to +70% time at the default. tm_opt_mult/tm_max_mult are overall
//   budget multipliers x100; tm_stability is the 0.060-per-stable-iter
//   shrink x1000; the scoredrop pair extends on falling eval (cp threshold
//   + ramp divisor); the effort quartet scales by best-move node-effort %.
// =========================================================================

// X(field, UciName, default, min, max) — one line per tunable, the single
// source for the struct default, the UCI advertisement and the clamp range.
#define BASILISK_SEARCH_PARAMS(X)                                        \
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
    /* Aspiration window */                                              \
    X(aspiration_delta,      AspirationDelta,       19,   10,    60)     \
    /* LMR base table (x100; see FIELD NOTES) */                         \
    X(lmr_base,              LmrBase,               60,    0,   150)     \
    X(lmr_divisor,           LmrDivisor,           209,  150,   350)     \
    /* LMR per-move adjustments (1024ths; see FIELD NOTES) */            \
    X(lmr_hist_div,          LmrHistDiv,          5683, 4096, 16384)     \
    X(lmr_non_pv_adj,        LmrNonPvAdj,         1024,    0,  3072)     \
    X(lmr_cut_node_adj,      LmrCutNodeAdj,        401,    0,  3072)     \
    X(lmr_tt_pv_adj,         LmrTtPvAdj,            23,    0,  3072)     \
    X(lmr_not_improving_adj, LmrNotImprovingAdj,    89,    0,  3072)     \
    X(lmr_tt_capture,        LmrTtCapture,         301,    0,  3072)     \
    X(lmr_singular_quiet,    LmrSingularQuiet,     401,    0,  3072)     \
    /* 5.7.6 removed check_ext_path_cap and lmr_allow_check. Both were    \
       added inert for 5.4.4, which closed REJECTED (BAS-S16, −3.48       \
       ±3.32), so they were the residue of a failed trial rather than an  \
       avenue still open. 5.7.3 then measured that reducing extension at  \
       checking nodes fails our WAC floor (137 → 124, floor 130), which   \
       is independent evidence for keeping today's policy: checking moves \
       are extended AND unreducible, deliberately. */                     \
    /* History updates (see FIELD NOTES) */                              \
    X(hist_bonus_quad,       HistBonusQuad,         62,    0,   128)     \
    X(hist_bonus_lin,        HistBonusLin,         120,    0,   400)     \
    X(hist_bonus_max,        HistBonusMax,        1863,  512,  4096)     \
    X(hist_malus_quad,       HistMalusQuad,         62,    0,   128)     \
    X(hist_malus_lin,        HistMalusLin,         143,    0,   400)     \
    X(hist_malus_max,        HistMalusMax,        1304,  512,  4096)     \
    X(hist_ttmove_bonus,     HistTtMoveBonus,       29,    0,  1024)     \
    /* Time management (see FIELD NOTES) */                              \
    X(tm_opt_mult,           TmOptMult,            100,   50,   200)     \
    X(tm_max_mult,           TmMaxMult,            100,   50,   200)     \
    X(tm_stability,          TmStability,           60,    0,   150)     \
    X(tm_scoredrop_thr,      TmScoreDropThr,        30,    5,   120)     \
    X(tm_scoredrop_div,      TmScoreDropDiv,       100,   30,   400)     \
    X(tm_effort_hi,          TmEffortHi,            80,   50,    99)     \
    X(tm_effort_lo,          TmEffortLo,            25,    1,    50)     \
    X(tm_effort_hi_mult,     TmEffortHiMult,        80,   50,   100)     \
    X(tm_effort_lo_mult,     TmEffortLoMult,       120,  100,   200)     \
    /* 8.5.12 instability-TM (+10.79; registered via 8.6.1) */           \
    X(tm_instability,        TmInstability,         35,    0,   100)

struct SearchParams {
#define BASILISK_SEARCH_PARAM_FIELD(field, uci, def, lo, hi) int field = def;
    BASILISK_SEARCH_PARAMS(BASILISK_SEARCH_PARAM_FIELD)
#undef BASILISK_SEARCH_PARAM_FIELD
};
