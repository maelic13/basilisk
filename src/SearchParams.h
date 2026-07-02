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

    // ---- Capture futility pruning (Phase 6.5, EXPOSED BUT INERT) ------------
    // Skip a capture at shallow lmr_depth when even winning the captured piece
    // cannot lift the static eval to alpha (SF + Ethereal). Was shipped ACTIVE
    // (cap_fut_depth 7) and SPRT'd vs the 6.4 head: -2.78 +/- 7.50 Elo, LOS 23%,
    // LLR drifting to H0 over 3.6k games -- a wash-to-tiny-loss, so REVERTED to
    // inert per the pre-registered rule. Gated by cap_fut_depth, default 0:
    // lmr_depth >= 0 so `lmr_depth < 0` never fires -> provably inert. Re-enable
    // (cap_fut_depth ~7) + tune the margins only in 6.9 SPSA, SPRT + CTest gated.
    int cap_fut_depth    = 0;    // CapFutDepth (0 = off; enable at ~7)
    int cap_fut_base     = 200;  // CapFutBase    (capture futility base margin)
    int cap_fut_coeff    = 200;  // CapFutCoeff   (capture futility per-lmrDepth margin)

    // ---- SEE-quiet pruning (Phase 6.5, EXPOSED BUT INERT) -------------------
    // Skip quiets losing material by SEE (margin -coeff * lmr_depth²; SF+Ethereal).
    // Gated by quiet_see_depth, default 0 == OFF: search nodes always have
    // depth >= 1, so `depth <= 0` never fires -> provably inert. A naive port
    // (base-table lmr_depth) broke KBNK COMPLETELY (no mate in 250 plies), unlike
    // capture futility -- SF's lmr_depth here includes the history term that
    // protects good-history quiets, which the base-table estimate lacks. Enable
    // + tune (quiet_see_depth ~8, quiet_see_coeff) only in 6.9 SPSA once the
    // history-aware lmr_depth is wired; SPRT + CTest gate it. See PLAN §5 6.5.
    int quiet_see_depth  = 0;    // QuietSeeDepth (0 = off; enable at ~8)
    int quiet_see_coeff  = 25;   // QuietSeeCoeff (quiet SEE margin = -coeff * lmrDepth^2)

    // ---- Singular extension -------------------------------------------------
    int singular_beta_mult      = 4;  // SingularBetaMult
    int singular_double_margin  = 4;  // SingularDoubleMargin
    // Phase 6.4 rider: cap stacked 2-ply singular extensions (Weiss-style) so a
    // pathological line can't chain unbounded double-extensions. Weiss's own
    // seed (5) chaotically broke the KBNK/KQK mate-resolution CTests -- the
    // same canary fragility diagnosed in 6.3 -- so the shipped default is
    // PROVABLY inert (200 > MAX_PLY=128, so double_exts can never reach it)
    // rather than an empirically-picked value. The real cap is 6.9 SPSA
    // material, gated by SPRT + these same CTests.
    int double_ext_max          = 200;  // DoubleExtMax

    // ---- Aspiration window --------------------------------------------------
    int aspiration_delta    = 19;    // AspirationDelta

    // ---- LMR base table formula (stored ×100; divided by 100.0 in init_lmr)
    int lmr_base            = 60;    // LmrBase    (represents 0.60)
    int lmr_divisor         = 209;   // LmrDivisor (represents 2.09)

    // ---- LMR per-move adjustments (Phase 6.7: in 1024ths of a ply) ----------
    // The *_adj knobs and lmr_tt_capture are fractional (1024 == 1 ply); the
    // reduction is accumulated in 1024ths and shifted `>> 10` at the end.
    // Defaults are the old integer values ×1024 -> behaviour-identical; the
    // sub-ply resolution is headroom for the 6.9 SPSA. lmr_tt_capture (SF: ~1
    // ply when the TT move is a capture) is seeded 0 == inert.
    int lmr_hist_div            = 7830;  // LmrHistDiv (history still integer-quantised; see search.cpp)
    int lmr_non_pv_adj          = 1024;  // LmrNonPvAdj      (1.0 ply)
    int lmr_cut_node_adj        = 0;     // LmrCutNodeAdj
    int lmr_tt_pv_adj           = 0;     // LmrTtPvAdj
    int lmr_not_improving_adj   = 0;     // LmrNotImprovingAdj
    int lmr_tt_capture          = 0;     // LmrTtCapture (0 = off; SF ~1039)

    // ---- Post-LMR continuation-history nudge (Phase 6.4) ---------------------
    // After an LMR-reduced move's confirmation re-search, reward/punish its
    // continuation history by whether the score held up against the original
    // window (SF/Weiss both do this after their post-LMR re-search; a further
    // do_deeper/do_shallower depth adjustment they also apply here was tried
    // and dropped -- every flat margin tried chaotically broke the KBNK/KQK
    // mate CTests, the same canary fragility diagnosed in 6.3, with no safe
    // default in a sane range). At full (Weiss-unscaled) weight, even this
    // narrower nudge alone still broke the KQK mate-in-5 CTest -- so the
    // shipped default is PROVABLY inert (0 -> hist_update's bonus term is
    // exactly 0, leaving the table untouched) rather than empirically picked.
    // The real scale is 6.9 SPSA material, gated by SPRT + these same CTests.
    int post_lmr_hist_scale    = 0;      // PostLmrHistScale (percent, 100 = Weiss-unscaled)

    // ---- History updates (Phase 6.3) -----------------------------------------
    // bonus = min((quad*d*d)/64 + lin*d, max); malus mirrored with its own knobs.
    // Defaults (quad=64, lin=0, max=2048, ttmove=0) reproduce the legacy
    // min(d*d, 2048) EXACTLY -- behaviour-identical seed. The references prove
    // the *shape family* (SF: linear asymmetric 134d-79/1572 vs 1005d-205/2218;
    // Weiss: 251d-267/2418 vs 532d-163/693 -- both linear, asymmetric,
    // independently capped), but transplanting their constants destabilised the
    // mate-resolution CTests chaotically (KBNK/KQK canaries) because every
    // consumer (hist pruning, LMR hist div) was tuned for our scale. So the
    // Basilisk-specific constants are found by the wave2 SPSA (6.9) jointly
    // with the consumers, and the SPSA output is SPRT-gated + CTest-gated.
    int hist_bonus_quad     = 64;    // HistBonusQuad  (x d^2 / 64)
    int hist_bonus_lin      = 0;     // HistBonusLin   (x d)
    int hist_bonus_max      = 2048;  // HistBonusMax
    int hist_malus_quad     = 64;    // HistMalusQuad  (x d^2 / 64)
    int hist_malus_lin      = 0;     // HistMalusLin   (x d)
    int hist_malus_max      = 2048;  // HistMalusMax
    int hist_ttmove_bonus   = 0;     // HistTtMoveBonus (extra when best == tt_move; SF-only, off)

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
