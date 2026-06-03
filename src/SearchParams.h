#pragma once

// Search tuning parameters.  All defaults reproduce Basilisk 1.4.9 behaviour
// exactly (same bench-13 fingerprint).  Under the BASILISK_TUNE build flag
// each field is exposed as a UCI spin option so weather-factory can SPSA-tune
// them.  In release builds the struct is a plain bag of compile-time constants.

struct SearchParams {
    // ---- Reverse futility pruning -------------------------------------------
    int rfp_coeff           = 140;   // RfpCoeff
    int rfp_improving       = 60;    // RfpImproving

    // ---- Razoring -----------------------------------------------------------
    int razor_coeff         = 300;   // RazorCoeff

    // ---- Null-move pruning --------------------------------------------------
    int null_base           = 4;     // NullBase
    int null_eval_div       = 200;   // NullEvalDiv

    // ---- ProbCut ------------------------------------------------------------
    int probcut_margin      = 200;   // ProbCutMargin

    // ---- Move-loop futility -------------------------------------------------
    int futility_base       = 150;   // FutilityBase
    int futility_coeff      = 110;   // FutilityCoeff

    // ---- History pruning ----------------------------------------------------
    int hist_prune_coeff    = 3500;  // HistPruneCoeff

    // ---- SEE pruning (bad captures) -----------------------------------------
    int see_prune_coeff     = 80;    // SeePruneCoeff

    // ---- Singular extension -------------------------------------------------
    int singular_beta_mult      = 2;   // SingularBetaMult
    int singular_double_margin  = 20;  // SingularDoubleMargin

    // ---- Aspiration window --------------------------------------------------
    int aspiration_delta    = 25;    // AspirationDelta

    // ---- LMR base table formula (stored ×100; divided by 100.0 in init_lmr)
    int lmr_base            = 75;    // LmrBase    (represents 0.75)
    int lmr_divisor         = 225;   // LmrDivisor (represents 2.25)

    // ---- LMR per-move adjustments -------------------------------------------
    int lmr_hist_div            = 8192;  // LmrHistDiv
    int lmr_non_pv_adj          = 1;     // LmrNonPvAdj
    int lmr_cut_node_adj        = 1;     // LmrCutNodeAdj
    int lmr_tt_pv_adj           = 1;     // LmrTtPvAdj
    int lmr_not_improving_adj   = 1;     // LmrNotImprovingAdj
};
