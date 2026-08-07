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
// equivalent. Colosseum tune vectors under tools/colosseum/tunes/ necessarily
// stay separate files — regenerate them FROM THIS TABLE before a new tune.
//
// ============================== FIELD NOTES ==============================
// Rationale & history per group; the table itself stays scannable. Do not
// delete these when editing values — they are the record of what was tried.
//
// Capture futility pruning (Phase 6.5, EXPOSED BUT NEAR-INERT):
//   Skip a capture at shallow lmr_depth when even winning the captured piece
//   cannot lift the static eval to alpha (SF + Ethereal). Was shipped ACTIVE
//   (cap_fut_depth 7) and SPRT'd vs the 6.4 head: -2.78 +/- 7.50 Elo, LOS
//   23%, LLR drifting to H0 over 3.6k games -- a wash-to-tiny-loss, so
//   REVERTED per the pre-registered rule. cap_fut_depth 1 (hcefinal SPSA)
//   only grazes lmr_depth < 1. Real re-enable is 10.7 SPSA material.
//
// SEE-quiet pruning (Phase 6.5, EXPOSED BUT INERT):
//   Skip quiets losing material by SEE (margin -coeff * lmr_depth^2;
//   SF+Ethereal). quiet_see_depth 0 == OFF: search nodes always have
//   depth >= 1, so `depth <= 0` never fires -> provably inert. A naive port
//   (base-table lmr_depth) broke KBNK COMPLETELY (no mate in 250 plies) --
//   SF's lmr_depth here includes the history term that protects
//   good-history quiets, which the base-table estimate lacks. Enable
//   (~8) only once the history-aware lmr_depth is wired (10.1/10.7).
//
// Qsearch quiet checks (Phase 6.8, EXPOSED BUT INERT):
//   At qply==0 only, after captures fail to raise alpha: try quiet checking
//   moves (Board::gen_quiet_checks) filtered by SEE>=0, capped at
//   qsearch_check_cap. The "SF does this" claim tracked an OLDER Stockfish
//   -- current SF restricts qsearch to captures/evasions (8.1f rider), and
//   the hcefinal SPSA independently pinned the cap at 0. The seeded default
//   (6) broke the KBNK mate CTest -- the sixth mechanism in a row (6.2-6.5)
//   to trip that canary at its literature seed. 0 is PROVABLY inert
//   (`qsearch_check_cap > 0` gates the whole loop). Kept for post-NNUE.
//
// Singular extension / double-extension cap (Phase 6.4 rider):
//   double_ext_max caps stacked 2-ply singular extensions (Weiss-style) so a
//   pathological line can't chain unbounded double-extensions. Weiss's own
//   seed (5) chaotically broke the KBNK/KQK mate-resolution CTests -- the
//   same canary fragility diagnosed in 6.3 -- so the shipped default is
//   PROVABLY inert (200 > MAX_PLY=128, double_exts can never reach it).
//   2026-07-17 re-exam vs the robust canary: capping HURTS (cap 6 -> bench
//   +18.5%, cap 12 -> +30%) -- our double-extensions are productive; stays
//   inert deliberately.
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
// Post-LMR continuation-history nudge (Phase 6.4, EXPOSED BUT INERT):
//   After an LMR-reduced move's confirmation re-search, reward/punish its
//   continuation history by whether the score held up (SF/Weiss). At full
//   Weiss weight it broke the KQK mate-in-5 CTest; 0 is PROVABLY inert
//   (hist_update's bonus term is exactly 0). The hcefinal SPSA
//   joint-optimum was 104, excluded at bake on the KBNK correctness-core
//   failure; re-tested vs rule50-retry after the canary fix -> WASH
//   (+0.87 +/- 4.92, LLR ~0 @ 8k) -> reverted to 0. A marginal SPSA dim
//   contributes ~0 re-added to the baked head; re-decided at 10.7.
//   (8.6.1: the UCI advertisement wrongly said 104 until the X-macro made
//   the drift impossible.)
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
    /* Capture futility (near-inert; see FIELD NOTES) */                 \
    X(cap_fut_depth,         CapFutDepth,            1,    0,    10)     \
    X(cap_fut_base,          CapFutBase,           198,    0,   500)     \
    X(cap_fut_coeff,         CapFutCoeff,          283,    0,   500)     \
    /* SEE-quiet pruning (inert at 0; see FIELD NOTES) */                \
    X(quiet_see_depth,       QuietSeeDepth,          0,    0,    10)     \
    X(quiet_see_coeff,       QuietSeeCoeff,         25,    0,   120)     \
    /* Qsearch quiet checks (inert at 0; see FIELD NOTES) */             \
    X(qsearch_check_cap,     QsearchCheckCap,        0,    0,    10)     \
    /* Singular extension */                                             \
    X(singular_beta_mult,    SingularBetaMult,       4,    1,     6)     \
    X(singular_double_margin, SingularDoubleMargin,  4,    0,    60)     \
    X(double_ext_max,        DoubleExtMax,         200,    1,   200)     \
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
    /* Post-LMR cont-hist nudge (inert at 0; see FIELD NOTES) */         \
    X(post_lmr_hist_scale,   PostLmrHistScale,       0,    0,   300)     \
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
