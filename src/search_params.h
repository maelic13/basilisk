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
//                                   line it inherited from the kernel it
//                                   replaced (BASILISK_LEGACY_LMR_SINGULAR);
//   BASILISK_B3_PARAMS, _B4_PARAMS, _B5_PARAMS  Clusters 2, 3 and 4's.
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

// ---- The line inherited from the replaced kernel ----------------------------
//
// lmr_singular_quiet: the old relaxation of a singular node's quiet
//   reductions, read only when B3SingularQuietLmr restores it. It keeps its
//   old name.
#define BASILISK_LEGACY_LMR_SINGULAR(X)                                  \
    X(lmr_singular_quiet,    LmrSingularQuiet,     401,    0,  3072)

// ---- The node kernel (Cluster 1, the selectivity core) ----------------------
//
// The kernel reads the inherited line above and its own coordinates at the
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
//
// F lines are coordinates the kernel reads and a tune build advertises but no
// tune fits: their response is a staircase a tune's step cannot resolve.
// Every consumer expands them as X (BASILISK_KERNEL_PARAMS passes X twice);
// tools/generate_spsa_surface.py reads X lines only, so they stay off the
// SPSA surface. quiet_see_coeff: SEE values here are multiples of 100, so
// neighbouring coefficients prune the same quiets at every common lmrDepth.
#define BASILISK_CORE_KERNEL_PARAMS(X, F)                                \
    BASILISK_LEGACY_LMR_SINGULAR(X)                                      \
    /* Node margins */                                                   \
    X(razor_margin,          CoreRazorMargin,       269,  128,   527)    \
    F(razor_quad_margin,     CoreRazorQuadMargin,   136,   60,   300)    \
    X(rfp_per_ply,           CoreRfpPerPly,         101,   40,   227)    \
    X(rfp_improving,         CoreRfpImproving,       98,   35,   160)    \
    X(rfp_worsening,         CoreRfpWorsening,       13,    0,    60)    \
    X(rfp_corr_mult,         CoreRfpCorrMult,       538,    0,  2048)    \
    X(rfp_depth_cap,         CoreRfpDepthCap,         10,    6,    19)    \
    X(rfp_beta_weight,       CoreRfpBetaWeight,     660,  512,  1024)    \
    /* Move-loop pruning */                                              \
    X(lmp_base,              CoreLmpBase,             3,    0,     8)    \
    X(lmp_square_scale,      CoreLmpSquareScale,     16,    8,    32)    \
    X(quiet_fut_base,        CoreQuietFutBase,      166,   80,   300)    \
    X(quiet_fut_slope,       CoreQuietFutSlope,     107,   34,   190)    \
    X(quiet_fut_alpha_term,  CoreQuietFutAlphaTerm,  29,    0,    90)    \
    X(quiet_fut_depth_cap,   CoreQuietFutDepthCap,    8,    4,    12)    \
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
    X(lmr_alpha_eval_scale,  CoreLmrAlphaEvalScale,   2,    1,    10)    \
    X(lmr_all_node_scale,    CoreLmrAllNodeScale,   247,    0,   800)    \
    X(research_deeper,       CoreResearchDeeper,     13,    4,    40)    \
    X(research_shallower,    CoreResearchShallower,   1,    1,    12)    \
    /* History update policy */                                          \
    X(quiet_bonus_cap,       CoreQuietBonusCap,    1394,  600,  3000)    \
    X(quiet_bonus_slope,     CoreQuietBonusSlope,   151,   60,   300)    \
    X(malus_cap,             CoreMalusCap,         2253,  900,  4500)    \
    X(malus_slope,           CoreMalusSlope,        829,  400,  2000)    \
    X(malus_decay,           CoreMalusDecay,        916,  700,  1024)    \
    X(main_hist_decay,       CoreMainHistDecay,     749,  400,  1024)    \
    /* TT cutoff training */                                             \
    X(tt_cutoff_bonus_slope, CoreTtCutoffBonusSlope, 151,    0,   300)   \
    F(tt_cutoff_bonus_max,   CoreTtCutoffBonusMax,  695,  200,  3000)   \
    /* Correction */                                                     \
    X(corr_update_scale,     CoreCorrUpdateScale,    16,    4,    64)     \
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
// The donor's forms where Basilisk keeps its own, each read by a paired run
// against the default (0, Basilisk's form):
// tt_cutoff_bonus_cap: 1 caps the TT-cutoff quiet bonus at
//   CoreTtCutoffBonusMax (the donor's 695); 0 slope * depth uncapped.
// razor_donor: 1 razors at any depth when the estimate is below alpha -
//   CoreRazorQuadMargin * depth^2 (the donor's 482 in its units), with no
//   table-move or bound guard; 0 the form above.
// tt_key50: 1 the table's key carries the rule-50 count's eight-ply bucket
//   from a count of 14, so a bound is not shared between positions with
//   very different fifty-move horizons (repetition reads the raw key); 0
//   the raw key.
// corr_nonpawn_kings: 1 the non-pawn correction keys include each side's
//   king; 0 leaves it out, so every bare king shares one entry.
// corr_admission: 1 trains the correction where the error's sign agrees with
//   the bound (above the static eval exactly when a move raised alpha); 0
//   where the node failed high or raised alpha.
#define BASILISK_CORE_KERNEL_SWITCHES(S)                                 \
    S(tt_cutoff_node_typed,  CoreTtCutoffNodeTyped,  1,    0,     1)     \
    S(pawn_hist_buckets_log2, CorePawnHistBucketsLog2, 13,  11,    13)   \
    S(corr_admission,        CoreCorrAdmission,      1,    0,     1)     \
    S(razor_depth_cap,       CoreRazorDepthCap,      1,    1,     3)     \
    S(lmp_exempt_checks,     CoreLmpExemptChecks,    0,    0,     1)     \
    S(lmr_negative_allowed,  CoreLmrNegativeAllowed, 1,    0,     1)     \
    S(tt_cutoff_bonus_cap,   CoreTtCutoffBonusCap,   0,    0,     1)     \
    S(razor_donor,           CoreRazorDonor,         0,    0,     1)     \
    S(tt_key50,              CoreTtKey50,            0,    0,     1)     \
    S(corr_nonpawn_kings,    CoreCorrNonPawnKings,   0,    0,     1)

// ---- Cluster 2, the proof searches and extensions ---------------------------
//
// B(X, F, S): X the coordinates at the packet's seeds, F the donor's fixed
// terms (read and advertised, never on a surface), S the categoricals and
// attribution switches. Tune builds advertise them.
//
// b3_check_ext: 1 extends every in-check node by a ply (the legacy policy);
//   0, the donor's, extends none.
// b3_check_prune: 1 admits in-check nodes to LMR, the first-move step-down,
//   the depth step after alpha rises and move-loop pruning (quiet futility
//   excepted), as the donor does; 0 reduces and prunes none in check.
// b3_pv_quiet_prune: 1 prunes quiets at PV nodes off the previous PV, as the
//   donor does; 0 exempts every PV node (the legacy rule).
//
// Null move: tried when the static eval is at least beta - per_ply * depth
//   - improving + base, and beta is above the floor; R = red_base + depth /
//   red_depth_div + max((static eval - beta) / red_eval_div, 0) plies. A cut
//   at depth >= verify_depth outside a verification is verified at the same
//   ply with null moves barred for 3/4 of the remaining depth below it.
//   b3_nmp_cut_only: 1 tries it at expected cut nodes only; 0 at any non-PV
//   node. b3_nmp_verify: 1 the region; 0 no verification; 2 the legacy
//   same-ply verification at depth >= 10, kept for the counter read.
//   b3_nmp_eval_gate: 1 also requires the TT-refined estimate to reach beta
//   (the oracle's and the legacy kernel's gate); 0 the donor's bare form.
//
// IIR: a ply off a PV or cut node off the previous PV, not in check, at
//   depth >= min_depth without a TT move. b3_iir: 1 on; 0 off. b3_iir_stale:
//   1 also reduces when the TT entry is shallower than depth - 3.
//
// ProbCut: at depth >= min_depth, with beta' = beta + base - improving, a
//   capture or queen promotion whose SEE clears beta' - static eval, holding
//   beta' in quiescence and at depth - (improving ? depth_improving : depth),
//   returns its value less the margin. The small ProbCut returns beta +
//   tt_margin from a lower bound at least depth - tt_depth deep that clears
//   it. b3_probcut_pv: 1 admits PV nodes; 0 non-PV only. b3_probcut_tt: 1
//   runs the small ProbCut (in check too, the donor's form); 0 none.
//
// Singular extension, at depth >= min_depth (+1 near a PV): the bound is the
//   TT score less (margin + margin_ttpv at a non-PV node near a PV) * depth /
//   63. Below it, the extension is 1 + (below by more than the double bar) +
//   (below by more than the triple bar); each bar is base + pv (PV node) +
//   quiet (a quiet TT move) + deep (ply beyond the root depth) [+ ttpv for the
//   triple], less min(|correction| / corr_div, corr_cap) and, for the double,
//   tmh_mult * TT-move history / 114178. A multi-cut trains the TT-move
//   history by -(mc_tmh_base + mc_tmh_slope * depth) and the correction by
//   (value - static eval) * 1000/282 * singular depth * mc_corr / 1024; the
//   negative extension is -negative plies. At root depth >= seek_mate_depth
//   with a root score of at least seek_mate_score either way, no singular
//   search runs. b3_multi_cut: 0 the donor's rule (the value clears beta); 1
//   the singular bound clears beta. b3_sing_tt_depth: 1 lowers the entry's
//   required depth from depth - 3 to depth - 2. b3_singular_quiet_lmr: 1 keeps
//   the legacy relaxation of a singular node's quiet reductions.
//   b3_ext_budget: 1 bounds a line's positive singular plies by the root
//   depth; 0 leaves them unbounded.
//
// Correction: b3_corr_decisive: 1 trains the correction on decisive scores;
//   0 refuses them (the legacy rule). b3_corr_excluded: 1 trains it at
//   singular-excluded nodes except where the node's raw static eval is 0
//   (a recognised draw); 0 refuses them. core_history_fills: 1 fills
//   fresh history tables with the donor's values at the first search after a
//   clear; 0 leaves them at zero.
//
// Upcoming repetition: b3_cuckoo: 1 raises alpha to the draw at a node
//   (interior or quiescence) whose side to move has a reversible move to a
//   repetition; 0 none.
//
// TT: b3_fail_low_store: 1 stores no move after a fail low (the entry keeps
//   its own; B-12's repair, the donor's form); 0 stores the best-scored move
//   (the form before Cluster 2). b3_tt_penalty: 1 lowers by one
//   ply the depth of an entry deep enough to cut at a non-PV node of depth
//   6 or more whose bound lies on the wrong side of beta; 0 none.
#define BASILISK_B3_PARAMS(X, F, S)                                   \
    S(b3_check_ext,          B3CheckExt,             0,    0,     1)     \
    S(b3_check_prune,        B3CheckPrune,           1,    0,     1)     \
    S(b3_pv_quiet_prune,     B3PvQuietPrune,         1,    0,     1)     \
    /* Null move */                                                    \
    X(b3_nmp_per_ply,        B3NmpPerPly,           16,    2,    32)     \
    X(b3_nmp_improving,      B3NmpImproving,        15,    0,    40)     \
    X(b3_nmp_base,           B3NmpBase,            163,   50,   300)     \
    X(b3_nmp_red_base,       B3NmpRedBase,           4,    1,    10)     \
    X(b3_nmp_red_eval_div,   B3NmpRedEvalDiv,       80,   36,   256)     \
    F(b3_nmp_red_depth_div,  B3NmpRedDepthDiv,       3,    2,     6)     \
    F(b3_nmp_beta_floor,     B3NmpBetaFloor,      -564, -2000,    0)     \
    F(b3_nmp_verify_depth,   B3NmpVerifyDepth,      16,    8,    32)     \
    S(b3_nmp_cut_only,       B3NmpCutOnly,           1,    0,     1)     \
    S(b3_nmp_verify,         B3NmpVerify,            1,    0,     2)     \
    S(b3_nmp_eval_gate,      B3NmpEvalGate,          0,    0,     1)     \
    /* Internal iterative reduction */                                   \
    X(b3_iir_min_depth,      B3IirMinDepth,          5,    3,    11)     \
    S(b3_iir,                B3Iir,                  1,    0,     1)     \
    S(b3_iir_stale,          B3IirStale,             0,    0,     1)     \
    /* ProbCut */                                                        \
    X(b3_probcut_base,       B3ProbCutBase,         73,   34,   240)     \
    X(b3_probcut_improving,  B3ProbCutImproving,    19,    0,    48)     \
    F(b3_probcut_min_depth,  B3ProbCutMinDepth,      3,    2,     8)     \
    F(b3_probcut_depth,      B3ProbCutDepth,         3,    2,     6)     \
    F(b3_probcut_depth_improving, B3ProbCutDepthImproving, 5, 3,    8)     \
    X(b3_probcut_tt_margin,  B3ProbCutTtMargin,    240,  100,   400)     \
    F(b3_probcut_tt_depth,   B3ProbCutTtDepth,       4,    2,     8)     \
    S(b3_probcut_pv,         B3ProbCutPv,            1,    0,     1)     \
    S(b3_probcut_tt,         B3ProbCutTt,            0,    0,     1)     \
    /* Singular extension */                                             \
    F(b3_sing_min_depth,     B3SingMinDepth,         6,    4,    10)     \
    X(b3_sing_margin,        B3SingMargin,         232,   13,   378)     \
    X(b3_sing_margin_ttpv,   B3SingMarginTtPv,      17,    0,   126)     \
    X(b3_sing_double_base,   B3SingDoubleBase,      -1,  -40,    40)     \
    X(b3_sing_double_pv,     B3SingDoublePv,        60,    0,   120)     \
    X(b3_sing_double_quiet,  B3SingDoubleQuiet,    -41,  -90,     0)     \
    X(b3_sing_double_deep,   B3SingDoubleDeep,     -11,  -30,     0)     \
    X(b3_sing_triple_base,   B3SingTripleBase,      20,  -20,    60)     \
    X(b3_sing_triple_pv,     B3SingTriplePv,        77,    0,   160)     \
    X(b3_sing_triple_quiet,  B3SingTripleQuiet,     -2, -110,     0)     \
    X(b3_sing_triple_ttpv,   B3SingTripleTtPv,      22,    0,    60)     \
    X(b3_sing_triple_deep,   B3SingTripleDeep,     -12,  -30,     0)     \
    F(b3_sing_corr_div,      B3SingCorrDiv,     198368, 50000, 800000)   \
    F(b3_sing_corr_cap,      B3SingCorrCap,        101,    0,   300)     \
    F(b3_sing_tmh_mult,      B3SingTmhMult,        331,    0,  1000)     \
    F(b3_sing_negative,      B3SingNegative,         3,    0,     4)     \
    F(b3_sing_mc_tmh_base,   B3SingMcTmhBase,      421,    0,  1000)     \
    F(b3_sing_mc_tmh_slope,  B3SingMcTmhSlope,     110,    0,   300)     \
    F(b3_sing_mc_corr,       B3SingMcCorr,         177,    0,   400)     \
    F(b3_seek_mate_depth,    B3SeekMateDepth,       12,    8,    64)     \
    F(b3_seek_mate_score,    B3SeekMateScore,      564,  100,  2000)     \
    S(b3_multi_cut,          B3MultiCut,             0,    0,     1)     \
    S(b3_sing_tt_depth,      B3SingTtDepth,          0,    0,     1)     \
    S(b3_singular_quiet_lmr, B3SingularQuietLmr,     0,    0,     1)     \
    S(b3_ext_budget,         B3ExtBudget,            1,    0,     1)     \
    /* Correction and the fresh tables */                                \
    S(b3_corr_decisive,      B3CorrDecisive,         1,    0,     1)     \
    S(b3_corr_excluded,      B3CorrExcluded,         1,    0,     1)     \
    S(core_history_fills,    CoreHistoryFills,       0,    0,     1)     \
    /* Upcoming repetition */                                            \
    S(b3_cuckoo,             B3Cuckoo,               1,    0,     1)     \
    /* TT */                                                             \
    S(b3_fail_low_store,     B3FailLowStore,         0,    0,     1)     \
    S(b3_tt_penalty,         B3TtPenalty,            1,    0,     1)

// Cluster 2's refit of Cluster 1 coordinates, fitted on its tree beside its
// own: A(field, UciName, value). The X line keeps Cluster 1's own fit, which
// the core surface starts from; the engine compiles and advertises these
// instead, and the later clusters' surfaces start from them. Each lies in its
// X line's range and differs from its default (test_search checks both).
#define BASILISK_B3_CORE_DEFAULTS(A)                                     \
    A(rfp_per_ply,           CoreRfpPerPly,          99)                \
    A(rfp_corr_mult,         CoreRfpCorrMult,       498)                \
    A(lmp_base,              CoreLmpBase,             4)                \
    A(quiet_fut_base,        CoreQuietFutBase,      167)                \
    A(capt_fut_base,         CoreCaptFutBase,        74)                \
    A(cont_hist_prune_slope, CoreContHistPruneSlope, 3832)              \
    A(corr_update_scale,     CoreCorrUpdateScale,    17)

// ---- Cluster 3, quiescence ---------------------------------------------------
//
// B(X, F, S) as Cluster 2's.
//
// Quiescence's margins. b4_qs_fut_base is added to the corrected static eval
// (not the table-tightened stand-pat) to give the futility base. The blends
// are the value's weight in 1024ths, the rest going to beta, at a stand-pat
// cut and at any other fail high; 1024 returns the value as it is.
// b4_qs_move_count: captures after this many, in delivery order and pruned
//   ones included, are pruned unless a check, promotion or recapture; 64
//   turns the rule off. An F line: it is a count.
// b4_qs_see_margin: every capture, and an evasion capture once the node holds
//   a value that is not a loss, must clear this SEE. An F line: on the
//   default vector every value from -99 to 0 prunes the same captures.
// b4_rfp_seek_mate_cap: reverse futility's deepest node in a mate drive.
// b4_rfp_seek_mate_depth: the root depth from which that cap binds, with a
//   root score of at least b3_seek_mate_score either way. Its own bar:
//   b3_seek_mate_depth is the singular half's, set to bound a tree this cap
//   would grow.
//
// b4_evasion_prune: 1 once an evasion holds a value that is not a loss,
//   quiets and SEE-losing captures are skipped; 0 every evasion is searched.
// b4_qs_recapture: 1 a recapture on the previous move's square is exempt from
//   the count rule, futility and the alpha-relative SEE; 0 not exempt.
// b4_stalemate: 1 tests for stalemate where quiescence's cheap conditions
//   hold; 0 stands pat on it.
// b4_rfp_seek_mate: 1 applies b4_rfp_seek_mate_cap; 0 no cap, reverse
//   futility as at any root depth.
// b4_qs_delta: 1 restores the delta rule quiescence ran before Cluster 3;
//   0 none.
// b4_gain_values: the captured value quiescence's gain rules read. 0 the
//   search's PIECE_VALUE; 1 the evaluation's material.
// b4_see_values: the vector every `see` and `see_ge` call reads, on the same
//   two choices (tune builds; release builds keep the constants).
// b4_quiet_checks: quiet checks searched at the first quiescence ply; 0 none.
#define BASILISK_B4_PARAMS(X, F, S)                                      \
    X(b4_qs_fut_base,        B4QsFutBase,           82,   40,   220)     \
    X(b4_qs_stand_pat_blend, B4QsStandPatBlend,    437,  256,  1024)     \
    X(b4_qs_fail_high_blend, B4QsFailHighBlend,    466,  256,  1024)     \
    F(b4_qs_move_count,      B4QsMoveCount,          2,    1,    64)     \
    F(b4_qs_see_margin,      B4QsSeeMargin,        -28, -200,     0)     \
    F(b4_rfp_seek_mate_cap,  B4RfpSeekMateCap,       5,    1,    19)     \
    F(b4_rfp_seek_mate_depth, B4RfpSeekMateDepth,   16,    8,    64)     \
    S(b4_evasion_prune,      B4EvasionPrune,         1,    0,     1)     \
    S(b4_qs_recapture,       B4QsRecapture,          1,    0,     1)     \
    S(b4_stalemate,          B4Stalemate,            1,    0,     1)     \
    S(b4_rfp_seek_mate,      B4RfpSeekMate,          0,    0,     1)     \
    S(b4_qs_delta,           B4QsDelta,              0,    0,     1)     \
    S(b4_gain_values,        B4GainValues,           0,    0,     1)     \
    S(b4_see_values,         B4SeeValues,            0,    0,     1)     \
    S(b4_quiet_checks,       B4QuietChecks,          0,    0,     6)

// Cluster 3's refit of earlier clusters' lines, in the form of
// BASILISK_B3_CORE_DEFAULTS, applied after it, so a line here wins. The
// coordinates are fitted on Cluster 3's tree beside its own. b3_fail_low_store:
// one form of the fail-low store for both searches; 1 stores no move in
// either, and 0 stores the best-scored searched move in both.
#define BASILISK_B4_CORE_DEFAULTS(A)                                     \
    A(b3_fail_low_store,     B3FailLowStore,          1)                \
    A(razor_margin,          CoreRazorMargin,       261)                \
    A(rfp_per_ply,           CoreRfpPerPly,         107)                \
    A(rfp_improving,         CoreRfpImproving,       94)                \
    A(rfp_corr_mult,         CoreRfpCorrMult,       457)                \
    A(lmr_cut_node_no_tt,    CoreLmrCutNodeNoTt,    856)                \
    A(b3_nmp_base,           B3NmpBase,             172)                \
    A(b3_nmp_per_ply,        B3NmpPerPly,            13)                \
    A(b3_iir_min_depth,      B3IirMinDepth,           6)

// ---- Cluster 4: the root and the aspiration window ---------------------------
//
// The width of a root line's window counts thousandths of a unit:
// `b5_asp_base + (thread % 8) * 282 + |mean square of the line's scores| * 1000 /
// b5_asp_msq_div`, rounded up to a whole unit either side of the line's average
// score, and it grows by b5_asp_growth/128 per failure. These are the donor's 5
// and 10193 in this engine's evaluation units (0.282 of the donor's), its 47
// unchanged.
//
// S lines. Categoricals, each settled by a paired run:
//   b5_fail_high_reduce: 1 the re-search after a fail high runs that many plies
//     shallower, at least one deep; 0 full depth.
//   b5_neg_ext_max: extensions from a negative reduction a line may hold before
//     the ceiling stops granting them; 0 unbounded.
// Attribution switches, whose defaults are the cluster's form:
//   b5_window: 1 the loop above; 0 the driver's older loop on the record.
//   b5_root_lmr: 1 root moves after the first are reduced at depth 2 or more.
//   b5_lmr_window_units: 1 the LMR window term reads a non-PV node's window in
//     the donor's units (577 * 0.282 = 163); 0 reads 577 at every node.
// b5_fail_high_stop: the percent of the optimum past which a fail high on the
//   previous best ends the search; 0 never. The cluster's window fails high
//   more often than the older loop's, and there the stop at the optimum itself
//   lost to the stop at twice it, so its default is BASILISK_B5_CORE_DEFAULTS's.
#define BASILISK_B5_PARAMS(X, F, S)                                      \
    X(b5_asp_base,           B5AspBase,           2202,  500, 40000)     \
    X(b5_asp_msq_div,        B5AspMsqDiv,         2907, 1000, 12000)     \
    X(b5_asp_growth,         B5AspGrowth,           46,   32,    96)     \
    S(b5_fail_high_reduce,   B5FailHighReduce,       0,    0,     1)     \
    S(b5_neg_ext_max,        B5NegExtMax,            1,    0,     2)     \
    S(b5_window,             B5Window,               1,    0,     1)     \
    S(b5_root_lmr,           B5RootLmr,              1,    0,     1)     \
    S(b5_lmr_window_units,   B5LmrWindowUnits,       1,    0,     1)     \
    S(b5_fail_high_stop,     B5FailHighStop,       100,    0,   400)

// Cluster 4's own defaults where they differ from a line's, in the form of
// BASILISK_B4_CORE_DEFAULTS and applied after it. The earlier clusters'
// coordinates here were fitted on Cluster 4's tree beside the window's.
#define BASILISK_B5_CORE_DEFAULTS(A)                                     \
    A(b5_fail_high_stop,     B5FailHighStop,        200)                \
    A(cont_hist_prune_slope, CoreContHistPruneSlope, 3783)              \
    A(corr_update_scale,     CoreCorrUpdateScale,    18)                \
    A(b3_nmp_per_ply,        B3NmpPerPly,            11)                \
    A(b3_sing_margin,        B3SingMargin,          201)                \
    A(b3_iir_min_depth,      B3IirMinDepth,           7)

// B.6.7's joint fit (BAS-S61, theta at iteration 2,000) where it differs from the
// value the clusters left, in the same form and applied after them. The
// kernel's literals the fit moved are baked in src/kernel_literals.h.
#define BASILISK_B67_DEFAULTS(A)                                         \
    A(razor_margin,          CoreRazorMargin,           258)                \
    A(rfp_per_ply,           CoreRfpPerPly,             101)                \
    A(rfp_improving,         CoreRfpImproving,           93)                \
    A(rfp_worsening,         CoreRfpWorsening,           12)                \
    A(rfp_corr_mult,         CoreRfpCorrMult,           504)                \
    A(rfp_depth_cap,         CoreRfpDepthCap,             9)                \
    A(rfp_beta_weight,       CoreRfpBetaWeight,         646)                \
    A(quiet_fut_base,        CoreQuietFutBase,          171)                \
    A(quiet_fut_slope,       CoreQuietFutSlope,         110)                \
    A(quiet_fut_alpha_term,  CoreQuietFutAlphaTerm,      32)                \
    A(capt_fut_base,         CoreCaptFutBase,            68)                \
    A(capt_fut_slope,        CoreCaptFutSlope,           92)                \
    A(cont_hist_prune_slope, CoreContHistPruneSlope,   3594)                \
    A(capt_see_coeff,        CoreCaptSeeCoeff,           65)                \
    A(capt_see_hist_div,     CoreCaptSeeHistDiv,         75)                \
    A(lmr_table_scale,       CoreLmrTableScale,        3119)                \
    A(lmr_offset,            CoreLmrOffset,             947)                \
    A(lmr_not_improving,     CoreLmrNotImproving,       224)                \
    A(lmr_tt_pv,             CoreLmrTtPv,               978)                \
    A(lmr_tt_pv_base,        CoreLmrTtPvBase,          3025)                \
    A(lmr_tt_pv_pv_node,     CoreLmrTtPvPvNode,         964)                \
    A(lmr_tt_pv_above_alpha, CoreLmrTtPvAboveAlpha,     763)                \
    A(lmr_tt_pv_depth_ok,    CoreLmrTtPvDepthOk,        838)                \
    A(lmr_cut_node,          CoreLmrCutNode,           3782)                \
    A(lmr_cut_node_no_tt,    CoreLmrCutNodeNoTt,        762)                \
    A(lmr_tt_capture,        CoreLmrTtCapture,          969)                \
    A(lmr_cutoff_cnt,        CoreLmrCutoffCnt,          253)                \
    A(lmr_cutoff_cnt2,       CoreLmrCutoffCnt2,        1006)                \
    A(lmr_cutoff_cnt_all,    CoreLmrCutoffCntAll,       874)                \
    A(lmr_stat_scale,        CoreLmrStatScale,          573)                \
    A(lmr_all_node_scale,    CoreLmrAllNodeScale,       242)                \
    A(research_deeper,       CoreResearchDeeper,         14)                \
    A(quiet_bonus_cap,       CoreQuietBonusCap,        1459)                \
    A(quiet_bonus_slope,     CoreQuietBonusSlope,       144)                \
    A(malus_cap,             CoreMalusCap,             2410)                \
    A(malus_slope,           CoreMalusSlope,            931)                \
    A(malus_decay,           CoreMalusDecay,            937)                \
    A(main_hist_decay,       CoreMainHistDecay,         776)                \
    A(tt_cutoff_bonus_slope, CoreTtCutoffBonusSlope,    139)                \
    A(corr_update_scale,     CoreCorrUpdateScale,        20)                \
    A(corr_cont_weight,      CoreCorrContWeight,      10045)                \
    A(b3_nmp_base,           B3NmpBase,                 180)                \
    A(b3_nmp_red_base,       B3NmpRedBase,                5)                \
    A(b3_nmp_red_eval_div,   B3NmpRedEvalDiv,            75)                \
    A(b3_iir_min_depth,      B3IirMinDepth,               8)                \
    A(b3_probcut_base,       B3ProbCutBase,              70)                \
    A(b3_probcut_improving,  B3ProbCutImproving,         20)                \
    A(b3_sing_margin,        B3SingMargin,              160)                \
    A(b3_sing_double_base,   B3SingDoubleBase,            0)                \
    A(b3_sing_double_pv,     B3SingDoublePv,             58)                \
    A(b3_sing_double_quiet,  B3SingDoubleQuiet,         -44)                \
    A(b3_sing_triple_base,   B3SingTripleBase,           23)                \
    A(b3_sing_triple_pv,     B3SingTriplePv,             82)                \
    A(b3_sing_triple_ttpv,   B3SingTripleTtPv,           19)                \
    A(b3_sing_triple_deep,   B3SingTripleDeep,          -11)                \
    A(b4_qs_fut_base,        B4QsFutBase,                87)                \
    A(b4_qs_stand_pat_blend, B4QsStandPatBlend,         436)                \
    A(b4_qs_fail_high_blend, B4QsFailHighBlend,         469)                \
    A(b5_asp_base,           B5AspBase,                2139)                \
    A(b5_asp_msq_div,        B5AspMsqDiv,              3260)                \
    A(b5_asp_growth,         B5AspGrowth,                47)

// B.6.6's adoptions, the B6_DONOR_FORMS arm, in the same form and applied
// last: the donor's razoring and the kings in the non-pawn correction keys,
// each adopted by its paired run and gated together. Empty on the OFF arm.
#ifdef BASILISK_B6_DONOR_FORMS
#define BASILISK_B6_CORE_DEFAULTS(A)                                     \
    A(razor_donor,           CoreRazorDonor,          1)                \
    A(corr_nonpawn_kings,    CoreCorrNonPawnKings,    1)
#else
#define BASILISK_B6_CORE_DEFAULTS(A)
#endif

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
    BASILISK_B4_PARAMS(BASILISK_SEARCH_PARAM_FIELD, BASILISK_SEARCH_PARAM_FIELD,
                       BASILISK_SEARCH_PARAM_FIELD)
    BASILISK_B5_PARAMS(BASILISK_SEARCH_PARAM_FIELD, BASILISK_SEARCH_PARAM_FIELD,
                       BASILISK_SEARCH_PARAM_FIELD)
#undef BASILISK_SEARCH_PARAM_FIELD
    constexpr SearchParams() noexcept {
#define BASILISK_ARM_DEFAULT(field, uci, value) field = value;
        BASILISK_B3_CORE_DEFAULTS(BASILISK_ARM_DEFAULT)
        BASILISK_B4_CORE_DEFAULTS(BASILISK_ARM_DEFAULT)
        BASILISK_B5_CORE_DEFAULTS(BASILISK_ARM_DEFAULT)
        BASILISK_B67_DEFAULTS(BASILISK_ARM_DEFAULT)
        BASILISK_B6_CORE_DEFAULTS(BASILISK_ARM_DEFAULT)
#undef BASILISK_ARM_DEFAULT
    }
};
