#pragma once

// The search kernel's own literals: the numbers the node kernel, quiescence,
// the history updates and the root record read that are no search option.
// Most are the donor's values for its own network and tree. This table is
// their one source: a release or tune build reads each as a compile-time
// constant, exactly the literal it replaced; the literal-screen build
// (CMake LITERAL_SCREEN, on a Tune build) advertises each as a UCI option
// `Lit<Name>` so that a zero-game read can move it (PLAN B.6.7). The list,
// each entry's donor line and its read are
// `analysis/b67_kernel_literals.md`; a literal that joins the tune surface
// leaves this table for a search parameter.
//
// L(field, UciName, value)

#define BASILISK_KERNEL_LITERALS(L)                                         \
    /* LMR and the move loop */                                             \
    L(lmr_window_pv,          LitLmrWindowPv,            611)               \
    L(lmr_window_null,        LitLmrWindowNull,          164)               \
    L(lmr_hist_div_scale,     LitLmrHistDivScale,        988)               \
    L(prune_main_hist,        LitPruneMainHist,           73)               \
    L(lmr_tt_pv_cut,          LitLmrTtPvCut,             914)               \
    L(lmr_base,               LitLmrBase,                695)               \
    L(lmr_move_count,         LitLmrMoveCount,            64)               \
    L(lmr_corr_div,           LitLmrCorrDiv,            7235)               \
    L(lmr_corr_cap,           LitLmrCorrCap,            2649)               \
    L(lmr_bias,               LitLmrBias,               2244)               \
    L(stat_capture,           LitStatCapture,            883)               \
    L(stat_main,              LitStatMain,              1940)               \
    L(stat_cont1,             LitStatCont1,             1182)               \
    L(stat_cont2,             LitStatCont2,             1096)               \
    L(lmr_alpha_lo,           LitLmrAlphaLo,             -67)               \
    L(lmr_alpha_hi,           LitLmrAlphaHi,              85)               \
    L(lmr_all_slope,          LitLmrAllSlope,            256)               \
    L(lmr_all_offset,         LitLmrAllOffset,           283)               \
    L(lmr_no_tt_step,         LitLmrNoTtStep,           1105)               \
    L(lmr_step1,              LitLmrStep1,              5073)               \
    L(lmr_step2,              LitLmrStep2,              5559)               \
    L(lmr_step2_depth,        LitLmrStep2Depth,            2)               \
    L(lmr_min_depth,          LitLmrMinDepth,              2)               \
    L(pv_tt_move_depth,       LitPvTtMoveDepth,            1)               \
    L(research_cont_bonus,    LitResearchContBonus,     1323)               \
    L(root_underpromo,        LitRootUnderpromo,         200)               \
    L(tie_promo_plies,        LitTiePromoPlies,            2)               \
    /* Hindsight depth adjustment */                                        \
    L(hind_up_reduction,      LitHindUpReduction,          3)               \
    L(hind_down_reduction,    LitHindDownReduction,        2)               \
    L(hind_down_depth,        LitHindDownDepth,            2)               \
    L(hind_down_eval,         LitHindDownEval,            49)               \
    /* The table's cutoff and its neighbours */                             \
    L(tt_node_typed_depth,    LitTtNodeTypedDepth,         4)               \
    L(tt_prev_move_count,     LitTtPrevMoveCount,          5)               \
    L(tt_prev_malus,          LitTtPrevMalus,          -2325)               \
    L(tt_graph_depth,         LitTtGraphDepth,             7)               \
    L(tt_rule50,              LitTtRule50,                96)               \
    L(tt_penalty_depth,       LitTtPenaltyDepth,           5)               \
    /* The parent's eval difference */                                      \
    L(eval_diff_lo,           LitEvalDiffLo,            -184)               \
    L(eval_diff_hi,           LitEvalDiffHi,             185)               \
    L(eval_diff_offset,       LitEvalDiffOffset,          57)               \
    L(eval_diff_main,         LitEvalDiffMain,            11)               \
    L(eval_diff_pawn,         LitEvalDiffPawn,            11)               \
    /* Null move, singular extension, the fail-high depth cut */            \
    L(nmp_min_ply_num,        LitNmpMinPlyNum,             3)               \
    L(sing_tt_depth,          LitSingTtDepth,              3)               \
    L(sing_beta_den,          LitSingBetaDen,             63)               \
    L(sing_tmh_den,           LitSingTmhDen,          116906)               \
    L(shuffle_rule50,         LitShuffleRule50,           10)               \
    L(shuffle_null_plies,     LitShuffleNullPlies,         6)               \
    L(shuffle_ply,            LitShufflePly,              20)               \
    L(fh_red_min,             LitFhRedMin,                 3)               \
    L(fh_red_max,             LitFhRedMax,                11)               \
    L(fh_red_by,              LitFhRedBy,                  3)               \
    /* The TT-move history */                                               \
    L(tt_move_bonus,          LitTtMoveBonus,            919)               \
    L(tt_move_malus,          LitTtMoveMalus,           -767)               \
    /* Correction history: its value, its update */                        \
    L(corr_no_move_scale,     LitCorrNoMoveScale,       1066)               \
    L(corr_share,             LitCorrShare,             1061)               \
    L(corr_pawn_w,            LitCorrPawnW,            15909)               \
    L(corr_minor_w,           LitCorrMinorW,           10327)               \
    L(corr_nonpawn_w,         LitCorrNonPawnW,         12143)               \
    L(corr_cont_none,         LitCorrContNone,         69413)               \
    L(corr_minor_share,       LitCorrMinorShare,         151)               \
    L(corr_nonpawn_share,     LitCorrNonPawnShare,       188)               \
    L(corr_cont2_share,       LitCorrCont2Share,         141)               \
    L(corr_cont4_share,       LitCorrCont4Share,          72)               \
    /* Continuation-history update */                                      \
    L(cont_w1,                LitContW1,                 499)               \
    L(cont_w2,                LitContW2,                 396)               \
    L(cont_w3,                LitContW3,                 156)               \
    L(cont_w4,                LitContW4,                 251)               \
    L(cont_w5,                LitContW5,                  66)               \
    L(cont_w6,                LitContW6,                 208)               \
    L(cont_consistency_scale, LitContConsistencyScale,  1024)               \
    L(cont_near_bonus,        LitContNearBonus,           75)               \
    /* Quiet, capture and low-ply updates after a best move */              \
    L(low_ply_share,          LitLowPlyShare,            710)               \
    L(cont_share,             LitContShare,              756)               \
    L(pawn_share_up,          LitPawnShareUp,           1031)               \
    L(pawn_share_down,        LitPawnShareDown,          435)               \
    L(pawn_share_threshold,   LitPawnShareThreshold,      -4)               \
    L(quiet_bonus_offset,     LitQuietBonusOffset,       -87)               \
    L(quiet_bonus_tt,         LitQuietBonusTt,           357)               \
    L(quiet_bonus_stat_den,   LitQuietBonusStatDen,       28)               \
    L(malus_offset,           LitMalusOffset,           -230)               \
    L(bonus_count_den,        LitBonusCountDen,          263)               \
    L(quiet_bonus_share,      LitQuietBonusShare,        919)               \
    L(malus_share,            LitMalusShare,            1335)               \
    L(capt_bonus_share,       LitCaptBonusShare,        1340)               \
    L(cont_malus_share,       LitContMalusShare,         749)               \
    L(capt_malus_share,       LitCaptMalusShare,        1536)               \
    /* The parent's update after a fail low */                              \
    L(fl_capture,             LitFlCapture,              917)               \
    L(fl_base,                LitFlBase,                -240)               \
    L(fl_stat_div,            LitFlStatDiv,               95)               \
    L(fl_depth_slope,         LitFlDepthSlope,            59)               \
    L(fl_depth_cap,           LitFlDepthCap,             404)               \
    L(fl_late_move,           LitFlLateMove,             181)               \
    L(fl_late_count,          LitFlLateCount,              9)               \
    L(fl_below_own,           LitFlBelowOwn,             134)               \
    L(fl_own_margin,          LitFlOwnMargin,             29)               \
    L(fl_below_parent,        LitFlBelowParent,          166)               \
    L(fl_parent_margin,       LitFlParentMargin,          20)               \
    L(fl_size_slope,          LitFlSizeSlope,            152)               \
    L(fl_size_offset,         LitFlSizeOffset,            88)               \
    L(fl_size_cap,            LitFlSizeCap,             1220)               \
    L(fl_cont_num,            LitFlContNum,              266)               \
    L(fl_main_share,          LitFlMainShare,            226)               \
    L(fl_pawn_share,          LitFlPawnShare,            320)               \
    /* History bounds (gravity) and the low-ply refill */                   \
    L(bound_main,             LitBoundMain,             6837)               \
    L(bound_low_ply,          LitBoundLowPly,           7746)               \
    L(bound_capture,          LitBoundCapture,         10685)               \
    L(bound_cont,             LitBoundCont,            30169)               \
    L(bound_pawn,             LitBoundPawn,             7558)               \
    L(bound_tt_move,          LitBoundTtMove,           8021)               \
    L(bound_corr,             LitBoundCorr,             1019)               \
    L(bound_cont_corr,        LitBoundContCorr,         1052)               \
    L(low_ply_refill,         LitLowPlyRefill,           111)               \
    /* The move picker's ordering (move_picker_core.cpp) */                  \
    L(pick_main_scale,        LitPickMainScale,         1089)               \
    L(pick_pawn_scale,        LitPickPawnScale,         1003)               \
    L(pick_cont_far_scale,    LitPickContFarScale,       986)               \
    L(pick_check_bonus,       LitPickCheckBonus,       17113)               \
    L(pick_threat_scale,      LitPickThreatScale,        971)               \
    L(pick_low_ply_scale,     LitPickLowPlyScale,       1023)               \
    L(pick_good_quiet,        LitPickGoodQuiet,       -12781)               \
    L(pick_sort_limit,        LitPickSortLimit,        -3612)               \
    L(pick_capture_scale,     LitPickCaptureScale,      1043)               \
    L(pick_good_capture_div,  LitPickGoodCaptureDiv,      44)               \
    /* The root record and the full-window iterations */                    \
    L(ema_min,                LitEmaMin,                  12)               \
    L(ema_max,                LitEmaMax,                  23)               \
    L(ema_msq_max,            LitEmaMsqMax,               16)               \
    L(ema_chi,                LitEmaChi,                   3)               \
    L(asp_min_depth,          LitAspMinDepth,              3)

struct KernelLiterals {
#define BASILISK_KERNEL_LITERAL_FIELD(field, uci, value) int field = value;
    BASILISK_KERNEL_LITERALS(BASILISK_KERNEL_LITERAL_FIELD)
#undef BASILISK_KERNEL_LITERAL_FIELD
};

// The compiled values, for compile-time proofs in every build.
inline constexpr KernelLiterals kKernelLiteralDefaults{};

#ifdef BASILISK_LITERAL_SCREEN
// The screen's values, set by UCI options before a search; one per process.
extern KernelLiterals g_kernel_literals;
#define KLIT(field) (::g_kernel_literals.field)
// A product of screened values is taken in 64 bits: the release build proves
// its 32-bit widths at the compiled values, which a moved option leaves.
#define KLIT_WIDE(x) (static_cast<long long>(x))
#else
// Each literal is a namespace-scope constant read as a prvalue, so the compiler
// sees exactly the number it replaced (a `const int&` parameter, as
// std::clamp's, binds a temporary, not the variable).
#define BASILISK_KERNEL_LITERAL_CONST(field, uci, value) inline constexpr int klit_##field = value;
BASILISK_KERNEL_LITERALS(BASILISK_KERNEL_LITERAL_CONST)
#undef BASILISK_KERNEL_LITERAL_CONST
#define KLIT(field) (int(::klit_##field))
#define KLIT_WIDE(x) (x)
#endif

// A whole table or ratio scaled by a literal in 1/1024, the unit every scale
// literal was seeded at (1024, the form it replaced). The same in every build:
// a baked scale is applied at its value, as the tune read it.
#define KLIT_SCALED(x, field) ((x) * KLIT(field) / 1024)

// A screen option's range: about half to twice the value (every divisor here
// is at least 2, so none reaches 0). A history bound is read no further than a
// signed 16-bit entry holds (`HistoryBound`).
constexpr int kernel_literal_min(int value) { return value > 0 ? value / 2 : value * 2 - 2; }
constexpr int kernel_literal_max(int value) { return value > 0 ? value * 2 + 2 : (value - 1) / 2; }
