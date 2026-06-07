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
};
