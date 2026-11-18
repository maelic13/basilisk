#pragma once

// Search tuning parameters. Under the BASILISK_TUNE build flag each field is
// exposed as a UCI spin option so an external SPSA tool can tune it. Outside a
// Tune build the kernel's lines are compile-time constants that fold where
// they are read; the comment after `BASILISK_SEARCH_PARAMS` below names the
// lines that stay run-time fields.
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
//   BASILISK_CORE_KERNEL_PARAMS     the node kernel's;
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

// ---- The node kernel (Cluster 1, the selectivity core) ----------------------
//
// Each line's default is the value in force: the later clusters' refits and
// B.6.7's joint fit (BAS-S61) are folded into it (B.8.2), so a tune starts
// from what the engine runs. Units are evaluation units unless a line says
// otherwise.
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
//   `research_deeper`.
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
    /* Node margins */                                                   \
    X(razor_margin,          CoreRazorMargin,       258,  128,   527)    \
    X(rfp_per_ply,           CoreRfpPerPly,         101,   40,   227)    \
    X(rfp_improving,         CoreRfpImproving,       93,   35,   160)    \
    X(rfp_worsening,         CoreRfpWorsening,       12,    0,    60)    \
    X(rfp_corr_mult,         CoreRfpCorrMult,       504,    0,  2048)    \
    X(rfp_depth_cap,         CoreRfpDepthCap,          9,    6,    19)    \
    X(rfp_beta_weight,       CoreRfpBetaWeight,     646,  512,  1024)    \
    /* Move-loop pruning */                                              \
    X(lmp_base,              CoreLmpBase,             4,    0,     8)    \
    X(lmp_square_scale,      CoreLmpSquareScale,     16,    8,    32)    \
    X(quiet_fut_base,        CoreQuietFutBase,      171,   80,   300)    \
    X(quiet_fut_slope,       CoreQuietFutSlope,     110,   34,   190)    \
    X(quiet_fut_alpha_term,  CoreQuietFutAlphaTerm,  32,    0,    90)    \
    X(quiet_fut_depth_cap,   CoreQuietFutDepthCap,    8,    4,    12)    \
    X(capt_fut_base,         CoreCaptFutBase,        68,   40,   270)    \
    X(capt_fut_slope,        CoreCaptFutSlope,       92,   60,   390)    \
    X(capt_fut_hist_div,     CoreCaptFutHistDiv,     15,    8,   128)    \
    X(capt_fut_depth_cap,    CoreCaptFutDepthCap,     6,    4,     8)    \
    X(cont_hist_prune_slope, CoreContHistPruneSlope, 3594, 1500, 8000)   \
    F(quiet_see_coeff,       CoreQuietSeeCoeff,      10,    4,    30)    \
    X(capt_see_coeff,        CoreCaptSeeCoeff,       65,   30,   200)    \
    X(capt_see_hist_div,     CoreCaptSeeHistDiv,     75,   20,   200)    \
    /* Late-move reductions (1024ths of a ply) and re-search */          \
    X(lmr_table_scale,       CoreLmrTableScale,    3119, 1500,  4500)    \
    X(lmr_offset,            CoreLmrOffset,          947,    0,  2000)    \
    X(lmr_not_improving,     CoreLmrNotImproving,   224,    0,   500)    \
    X(lmr_tt_pv,             CoreLmrTtPv,           978,    0,  2000)    \
    X(lmr_tt_pv_base,        CoreLmrTtPvBase,      3025,    0,  6000)    \
    X(lmr_tt_pv_pv_node,     CoreLmrTtPvPvNode,     964,    0,  2500)    \
    X(lmr_tt_pv_above_alpha, CoreLmrTtPvAboveAlpha, 763,    0,  2000)    \
    X(lmr_tt_pv_depth_ok,    CoreLmrTtPvDepthOk,    838,    0,  2000)    \
    X(lmr_cut_node,          CoreLmrCutNode,       3782, 1500,  7000)    \
    X(lmr_cut_node_no_tt,    CoreLmrCutNodeNoTt,    762,    0,  2500)    \
    X(lmr_tt_capture,        CoreLmrTtCapture,      969,    0,  2500)    \
    X(lmr_cutoff_cnt,        CoreLmrCutoffCnt,      253,    0,  1000)    \
    X(lmr_cutoff_cnt2,       CoreLmrCutoffCnt2,    1006,    0,  2500)    \
    X(lmr_cutoff_cnt_all,    CoreLmrCutoffCntAll,   874,    0,  2500)    \
    X(lmr_stat_scale,        CoreLmrStatScale,      573,  100,  1200)    \
    X(lmr_alpha_eval_scale,  CoreLmrAlphaEvalScale,   2,    1,    10)    \
    X(lmr_all_node_scale,    CoreLmrAllNodeScale,   242,    0,   800)    \
    X(research_deeper,       CoreResearchDeeper,     14,    4,    40)    \
    /* History update policy */                                          \
    X(quiet_bonus_cap,       CoreQuietBonusCap,    1459,  600,  3000)    \
    X(quiet_bonus_slope,     CoreQuietBonusSlope,   144,   60,   300)    \
    X(malus_cap,             CoreMalusCap,         2410,  900,  4500)    \
    X(malus_slope,           CoreMalusSlope,        931,  400,  2000)    \
    X(malus_decay,           CoreMalusDecay,        937,  700,  1024)    \
    X(main_hist_decay,       CoreMainHistDecay,     776,  400,  1024)    \
    /* TT cutoff training */                                             \
    X(tt_cutoff_bonus_slope, CoreTtCutoffBonusSlope, 139,    0,   300)   \
    /* Correction */                                                     \
    X(corr_update_scale,     CoreCorrUpdateScale,    20,    4,    64)     \
    X(corr_cont_weight,      CoreCorrContWeight,  10045, 2000, 16000)

// Cluster 1's small-range lines: S(field, UciName, default, min, max). Tune
// builds advertise them as options; they are never SPSA coordinates, which is
// why they use S and not X (tools/generate_spsa_surface.py reads X lines only).
//
// razor_depth_cap: the deepest node razoring runs at (1 = the oracle's form,
//   3 = the legacy reach).
#define BASILISK_CORE_KERNEL_SWITCHES(S)                                 \
    S(razor_depth_cap,       CoreRazorDepthCap,      1,    1,     3)

// ---- Cluster 2, the proof searches and extensions ---------------------------
//
// B(X, F, S): X the coordinates at the values in force, F the donor's fixed
// terms (read and advertised, never on a surface), S small-range lines. Tune
// builds advertise them.
//
// No in-check node is extended (the donor's policy); in-check nodes are
// reduced and move-loop pruned as the donor's are (quiet futility excepted);
// quiets are pruned at PV nodes off the previous PV, as the donor does.
//
// Null move, at an expected cut node: tried when the static eval is at least beta - per_ply * depth
//   - improving + base, and beta is above the floor; R = red_base + depth /
//   red_depth_div + max((static eval - beta) / red_eval_div, 0) plies. A cut
//   at depth >= verify_depth outside a verification is verified at the same
//   ply with null moves barred for 3/4 of the remaining depth below it. The
//   donor's bare form: no gate on the TT-refined estimate.
//
// IIR: a ply off a PV or cut node off the previous PV, not in check, at
//   depth >= min_depth without a TT move.
//
// ProbCut, at PV nodes too: at depth >= min_depth, with beta' = beta + base -
//   improving, a capture or queen promotion whose SEE clears beta' - static
//   eval, holding beta' in quiescence and at depth - (improving ?
//   depth_improving : depth), returns its value less the margin.
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
//   search runs. The multi-cut is the donor's rule (the value clears beta). A
//   line's positive singular plies are bounded by the root depth.
//
// Correction is trained on decisive scores and at singular-excluded nodes,
//   except where the node's raw static eval is 0 (a recognised draw).
//
// Upcoming repetition: a node (interior or quiescence) whose side to move has
//   a reversible move to a repetition raises alpha to the draw.
//
// TT: no move is stored after a fail low (the entry keeps its own; B-12's
//   repair, the donor's form). An entry deep enough to cut at a non-PV node of
//   depth 6 or more whose bound lies on the wrong side of beta loses a ply.
#define BASILISK_B3_PARAMS(X, F, S)                                   \
    /* Null move */                                                    \
    X(b3_nmp_per_ply,        B3NmpPerPly,           11,    2,    32)     \
    X(b3_nmp_improving,      B3NmpImproving,        15,    0,    40)     \
    X(b3_nmp_base,           B3NmpBase,            180,   50,   300)     \
    X(b3_nmp_red_base,       B3NmpRedBase,           5,    1,    10)     \
    X(b3_nmp_red_eval_div,   B3NmpRedEvalDiv,       75,   36,   256)     \
    F(b3_nmp_red_depth_div,  B3NmpRedDepthDiv,       3,    2,     6)     \
    F(b3_nmp_beta_floor,     B3NmpBetaFloor,      -564, -2000,    0)     \
    F(b3_nmp_verify_depth,   B3NmpVerifyDepth,      16,    8,    32)     \
    /* Internal iterative reduction */                                   \
    X(b3_iir_min_depth,      B3IirMinDepth,          8,    3,    11)     \
    /* ProbCut */                                                        \
    X(b3_probcut_base,       B3ProbCutBase,         70,   34,   240)     \
    X(b3_probcut_improving,  B3ProbCutImproving,    20,    0,    48)     \
    F(b3_probcut_min_depth,  B3ProbCutMinDepth,      3,    2,     8)     \
    F(b3_probcut_depth,      B3ProbCutDepth,         3,    2,     6)     \
    F(b3_probcut_depth_improving, B3ProbCutDepthImproving, 5, 3,    8)     \
    /* Singular extension */                                             \
    F(b3_sing_min_depth,     B3SingMinDepth,         6,    4,    10)     \
    X(b3_sing_margin,        B3SingMargin,         160,   13,   378)     \
    X(b3_sing_margin_ttpv,   B3SingMarginTtPv,      17,    0,   126)     \
    X(b3_sing_double_base,   B3SingDoubleBase,       0,  -40,    40)     \
    X(b3_sing_double_pv,     B3SingDoublePv,        58,    0,   120)     \
    X(b3_sing_double_quiet,  B3SingDoubleQuiet,    -44,  -90,     0)     \
    X(b3_sing_double_deep,   B3SingDoubleDeep,     -11,  -30,     0)     \
    X(b3_sing_triple_base,   B3SingTripleBase,      23,  -20,    60)     \
    X(b3_sing_triple_pv,     B3SingTriplePv,        82,    0,   160)     \
    X(b3_sing_triple_quiet,  B3SingTripleQuiet,     -2, -110,     0)     \
    X(b3_sing_triple_ttpv,   B3SingTripleTtPv,      19,    0,    60)     \
    X(b3_sing_triple_deep,   B3SingTripleDeep,     -11,  -30,     0)     \
    F(b3_sing_corr_div,      B3SingCorrDiv,     198368, 50000, 800000)   \
    F(b3_sing_corr_cap,      B3SingCorrCap,        101,    0,   300)     \
    F(b3_sing_tmh_mult,      B3SingTmhMult,        331,    0,  1000)     \
    F(b3_sing_negative,      B3SingNegative,         3,    0,     4)     \
    F(b3_sing_mc_tmh_base,   B3SingMcTmhBase,      421,    0,  1000)     \
    F(b3_sing_mc_tmh_slope,  B3SingMcTmhSlope,     110,    0,   300)     \
    F(b3_sing_mc_corr,       B3SingMcCorr,         177,    0,   400)     \
    F(b3_seek_mate_depth,    B3SeekMateDepth,       12,    8,    64)     \
    F(b3_seek_mate_score,    B3SeekMateScore,      564,  100,  2000)

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
//
// Once an evasion holds a value that is not a loss, quiets and SEE-losing
// captures are skipped. A recapture on the previous move's square is exempt
// from the count rule, futility and the alpha-relative SEE. Stalemate is
// tested where quiescence's cheap conditions hold. The gain rules credit the
// search's PIECE_VALUE, and no quiet check is searched.
#define BASILISK_B4_PARAMS(X, F, S)                                      \
    X(b4_qs_fut_base,        B4QsFutBase,           87,   40,   220)     \
    X(b4_qs_stand_pat_blend, B4QsStandPatBlend,    436,  256,  1024)     \
    X(b4_qs_fail_high_blend, B4QsFailHighBlend,    469,  256,  1024)     \
    F(b4_qs_move_count,      B4QsMoveCount,          2,    1,    64)     \
    F(b4_qs_see_margin,      B4QsSeeMargin,        -28, -200,     0)

// ---- Cluster 4: the root and the aspiration window ---------------------------
//
// The width of a root line's window counts thousandths of a unit:
// `b5_asp_base + (thread % 8) * 282 + |mean square of the line's scores| * 1000 /
// b5_asp_msq_div`, rounded up to a whole unit either side of the line's average
// score, and it grows by b5_asp_growth/128 per failure. These are the donor's 5
// and 10193 in this engine's evaluation units (0.282 of the donor's), its 47
// unchanged.
//
// A fail high is re-searched at full depth. A line holds one extension from a
// negative reduction at most. Root moves after the first are reduced at depth 2
// or more, as at any PV node. The LMR window term reads a non-PV node's window
// in the donor's units (577 * 0.282 = 163). The driver's older loop serves only
// a root record left empty (a helper whose root filter leaves it no move).
// b5_fail_high_stop: the percent of the optimum past which a fail high on the
//   previous best ends the search; 0 never. The cluster's window fails high
//   more often than the older loop's, and there the stop at the optimum itself
//   lost to the stop at twice it, so its default is 200.
#define BASILISK_B5_PARAMS(X, F, S)                                      \
    X(b5_asp_base,           B5AspBase,           2139,  500, 40000)     \
    X(b5_asp_msq_div,        B5AspMsqDiv,         3260, 1000, 12000)     \
    X(b5_asp_growth,         B5AspGrowth,           47,   32,    96)     \
    S(b5_fail_high_stop,     B5FailHighStop,       200,    0,   400)

#define BASILISK_KERNEL_PARAMS(X) BASILISK_CORE_KERNEL_PARAMS(X, X)
#define BASILISK_KERNEL_SWITCHES(S) BASILISK_CORE_KERNEL_SWITCHES(S)

#define BASILISK_SEARCH_PARAMS(X) \
    BASILISK_KERNEL_PARAMS(X)     \
    BASILISK_ROOT_PARAMS(X)

// Outside a Tune build nothing sets the kernel's lines (the core kernel's,
// Cluster 2's and Cluster 3's tables), so there each is a compile-time
// constant of the struct and `kp.name` folds where it is read; the driver's
// tables (the root's and Cluster 4's) stay fields. A line a test assigns
// stays a field in every build; it is named here, one probe each.
#define BASILISK_PARAM_RUNTIME_b3_nmp_verify_depth BASILISK_PARAM_PROBE
#define BASILISK_PARAM_RUNTIME_b4_qs_fut_base BASILISK_PARAM_PROBE
#define BASILISK_PARAM_RUNTIME_b4_qs_fail_high_blend BASILISK_PARAM_PROBE
#define BASILISK_PARAM_RUNTIME_b4_qs_move_count BASILISK_PARAM_PROBE
#define BASILISK_PARAM_RUNTIME_b4_qs_see_margin BASILISK_PARAM_PROBE

#define BASILISK_PARAM_PROBE ~, 1
#define BASILISK_PARAM_SECOND(a, b, ...) b
#define BASILISK_PARAM_IS_PROBE(...) BASILISK_PARAM_SECOND(__VA_ARGS__, 0, ~)
#define BASILISK_PARAM_IS_RUNTIME(field) BASILISK_PARAM_IS_PROBE(BASILISK_PARAM_RUNTIME_##field)
#define BASILISK_PARAM_CAT(a, b) BASILISK_PARAM_CAT_I(a, b)
#define BASILISK_PARAM_CAT_I(a, b) a##b
#define BASILISK_PARAM_DECL_1(field, def) int field = def;
#define BASILISK_PARAM_DECL_0(field, def) static constexpr int field = def;

struct SearchParams {
#define BASILISK_SEARCH_PARAM_FIELD(field, uci, def, lo, hi) int field = def;
#ifdef BASILISK_TUNE
#define BASILISK_KERNEL_PARAM_FIELD(field, uci, def, lo, hi) int field = def;
#else
#define BASILISK_KERNEL_PARAM_FIELD(field, uci, def, lo, hi) \
    BASILISK_PARAM_CAT(BASILISK_PARAM_DECL_, BASILISK_PARAM_IS_RUNTIME(field))(field, def)
#endif
    BASILISK_KERNEL_PARAMS(BASILISK_KERNEL_PARAM_FIELD)
    BASILISK_ROOT_PARAMS(BASILISK_SEARCH_PARAM_FIELD)
    BASILISK_KERNEL_SWITCHES(BASILISK_SEARCH_PARAM_FIELD)
    BASILISK_B3_PARAMS(BASILISK_KERNEL_PARAM_FIELD, BASILISK_KERNEL_PARAM_FIELD,
                       BASILISK_KERNEL_PARAM_FIELD)
    BASILISK_B4_PARAMS(BASILISK_KERNEL_PARAM_FIELD, BASILISK_KERNEL_PARAM_FIELD,
                       BASILISK_KERNEL_PARAM_FIELD)
    BASILISK_B5_PARAMS(BASILISK_SEARCH_PARAM_FIELD, BASILISK_SEARCH_PARAM_FIELD,
                       BASILISK_SEARCH_PARAM_FIELD)
#undef BASILISK_KERNEL_PARAM_FIELD
#undef BASILISK_SEARCH_PARAM_FIELD
};
