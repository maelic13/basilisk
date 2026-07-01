#pragma once

// Search tuning parameters. Under the BASILISK_TUNE build flag each field is
// exposed as a UCI spin option so weather-factory can SPSA-tune it. In release
// builds the struct is a plain bag of compile-time constants.

struct SearchParams {
    // ---- Reverse futility pruning -------------------------------------------
    int rfp_coeff           = 160;   // RfpCoeff
    int rfp_improving       = 72;    // RfpImproving

    // ---- Razoring -----------------------------------------------------------
    int razor_coeff         = 243;   // RazorCoeff

    // ---- Null-move pruning --------------------------------------------------
    int null_base           = 3;     // NullBase
    int null_eval_div       = 192;   // NullEvalDiv

    // ---- ProbCut ------------------------------------------------------------
    int probcut_margin      = 189;   // ProbCutMargin

    // ---- Move-loop futility -------------------------------------------------
    int futility_base       = 180;   // FutilityBase
    int futility_coeff      = 128;   // FutilityCoeff

    // ---- History pruning ----------------------------------------------------
    int hist_prune_coeff    = 4210;  // HistPruneCoeff

    // ---- SEE pruning (bad captures) -----------------------------------------
    int see_prune_coeff     = 73;    // SeePruneCoeff

    // ---- Singular extension -------------------------------------------------
    int singular_beta_mult      = 4;  // SingularBetaMult
    int singular_double_margin  = 4;  // SingularDoubleMargin

    // ---- Aspiration window --------------------------------------------------
    int aspiration_delta    = 19;    // AspirationDelta

    // ---- LMR base table formula (stored ×100; divided by 100.0 in init_lmr)
    int lmr_base            = 60;    // LmrBase    (represents 0.60)
    int lmr_divisor         = 209;   // LmrDivisor (represents 2.09)

    // ---- LMR per-move adjustments -------------------------------------------
    int lmr_hist_div            = 7830;  // LmrHistDiv
    int lmr_non_pv_adj          = 1;     // LmrNonPvAdj
    int lmr_cut_node_adj        = 0;     // LmrCutNodeAdj
    int lmr_tt_pv_adj           = 0;     // LmrTtPvAdj
    int lmr_not_improving_adj   = 0;     // LmrNotImprovingAdj

    // ---- Time management (Phase 5) --------------------------------------------
    // Hand-tuned defaults. The 5.8 SPSA bake (TmOptMult 105 / TmMaxMult 85 etc.,
    // 1147 iters @ 3+0.03) was REVERTED after the 5.9 validation: the gain SPRT
    // against these defaults was a wash (+0.88 ± 4.03 Elo over 12,262 games, LLR
    // flat inside [0,3]), confirming the TM was already at its ceiling (the
    // Phase 5.3 "TM is sound" finding). The 9 knobs stay exposed under
    // BASILISK_TUNE for future re-tuning; only the baked values were dropped.
    // Overall budget multipliers applied to the computed optimum/maximum (×100).
    int tm_opt_mult         = 100;   // TmOptMult   (1.00)
    int tm_max_mult         = 100;   // TmMaxMult   (1.00)
    // Adaptive iteration-stop scaling (Searcher::search):
    int tm_stability        = 60;    // TmStability (0.060 step per stable iter, ×1000)
    int tm_scoredrop_thr    = 30;    // TmScoreDropThr   (cp drop that triggers extension)
    int tm_scoredrop_div    = 100;   // TmScoreDropDiv   (divisor of the extra-time ramp)
    int tm_effort_hi        = 80;    // TmEffortHi       (best-move node-effort %, shrink time)
    int tm_effort_lo        = 25;    // TmEffortLo       (low effort %, grow time)
    int tm_effort_hi_mult   = 80;    // TmEffortHiMult   (0.80 when effort high, ×100)
    int tm_effort_lo_mult   = 120;   // TmEffortLoMult   (1.20 when effort low, ×100)
};
