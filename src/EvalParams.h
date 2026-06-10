#pragma once

#include <type_traits>

// Tunable evaluation weights for Basilisk's HCE eval.
//
// Defaults are byte-for-byte equivalent to the inline constants they replace
// in eval.cpp. init_eval_tables(p) must be called again whenever p changes
// (it bakes material + PST values into the runtime MG_TABLE / EG_TABLE).
//
// X-macro registry (for the Step 2.1 loader and Step 2.2 tuner):
//   EVAL_PARAM_LIST(X) expands X(display_name, member, length)
//   - For array members (length > 1): p.member is int[length]; iterate p.member[i].
//   - For scalar members (length == 1): p.member is int; access as (&p.member)[0].
//   - For PST sub-rows (e.g. pst_mg[0]): p.pst_mg[0] decays to int* of length 64.

struct EvalParams {

    // ---- Material values [PieceType 0..6] -----------------------------------
    int mg_val[7] = {0, 85, 323, 363, 514, 1085, 0};
    int eg_val[7] = {0, 96, 310, 319, 557,  996, 0};

    // ---- PSTs: pst_mg/eg[piece_idx][sq] -------------------------------------
    // piece_idx = PieceType - 1: 0=PAWN, 1=KNIGHT, 2=BISHOP, 3=ROOK, 4=QUEEN, 5=KING
    // sq: a1=0 .. h8=63, white perspective (init_eval_tables mirrors rank for Black)
    int pst_mg[6][64] = {
        // PAWN
        {   0,   0,   0,   0,   0,   0,   0,   0,
          -35,  -1, -20, -23, -15,  24,  38, -22,
          -26,  -4,  -4, -10,   3,   3,  33, -12,
          -27,  -2,  -5,  12,  17,   6,  10, -25,
          -14,  13,   6,  21,  23,  12,  17, -23,
           -6,   7,  26,  31,  65,  56,  25, -20,
           98, 134,  61,  95,  68, 126,  34, -11,
            0,   0,   0,   0,   0,   0,   0,   0 },
        // KNIGHT
        { -167, -89, -34, -49,  61, -97, -15,-107,
           -73, -41,  72,  36,  23,  62,   7, -17,
           -47,  60,  37,  65,  84, 129,  73,  44,
            -9,  17,  19,  53,  37,  69,  18,  22,
           -13,   4,  16,  13,  28,  19,  21,  -8,
           -23,  -9,  12,  10,  19,  17,  25, -16,
           -29, -53, -12,  -3,  -1,  18, -14, -19,
          -105, -21, -58, -33, -17, -28, -19, -23 },
        // BISHOP
        {  -29,   4, -82, -37, -25, -42,   7,  -8,
           -26,  16, -18, -13,  30,  59,  18, -47,
           -16,  37,  43,  40,  35,  50,  37,  -2,
            -4,   5,  19,  50,  37,  37,   7,  -2,
            -6,  13,  13,  26,  34,  12,  10,   4,
             0,  15,  15,  15,  14,  27,  18,  10,
             4,  15,  16,   0,   7,  21,  33,   1,
           -33,  -3, -14, -21, -13, -12, -39, -21 },
        // ROOK
        {  -19, -13,   1,  17,  16,   7, -37, -26,
           -44, -16, -20,  -9,  -1,  11,  -6, -71,
           -45, -25, -16, -17,   3,   0,  -5, -33,
           -36, -26, -12,  -1,   9,  -7,   6, -23,
           -24, -11,   7,  26,  24,  35,  -8, -20,
            -5,  19,  26,  36,  17,  45,  61,  16,
            27,  32,  58,  62,  80,  67,  26,  44,
            32,  42,  32,  51,  63,   9,  31,  43 },
        // QUEEN
        {  -28,   0,  29,  12,  59,  44,  43,  45,
           -24, -39,  -5,   1, -16,  57,  28,  54,
           -13, -17,   7,   8,  29,  56,  47,  57,
           -27, -27, -16, -16,  -1,  17,  -2,   1,
            -9, -26,  -9, -10,  -2,  -4,   3,  -3,
           -14,   2, -11,  -2,  -5,   2,  14,   5,
           -35,  -8,  11,   2,   8,  15,  -3,   1,
            -1, -18,  -9,  10, -15, -25, -31, -50 },
        // KING
        {  -15,  36,  12, -54,   8, -28,  24,  14,
             1,   7,  -8, -64, -43, -16,   9,   8,
           -14, -14, -22, -46, -44, -30, -15, -27,
           -49,  -1, -27, -39, -46, -44, -33, -51,
           -17, -20, -12, -27, -30, -25, -14, -36,
            -9,  24,   2, -16, -20,   6,  22, -22,
            29,  -1, -20,  -7,  -8,  -4, -38, -29,
           -65,  23,  16, -15, -56, -34,   2,  13 }
    };

    int pst_eg[6][64] = {
        // PAWN
        {   0,   0,   0,   0,   0,   0,   0,   0,
           -10,  -6,  10,   0,  14,   7,  -5, -19,
            -8,  -4,   7,  22,  17,  16,   3, -14,
            13,   0, -13,   1,  -1, -16,   3,  -6,
            32,  24,  13,   5,  -2,   4,  17,  17,
            56,  35,  41,  22,  26,  51,  56,  20,
           134, 108, 109, 107, 105, 104, 112, 108,
             0,   0,   0,   0,   0,   0,   0,   0 },
        // KNIGHT
        {  -58, -38, -13, -28, -31, -27, -63, -99,
           -25,  -8, -25,  -2,  -9, -25, -24, -52,
           -24, -20,  10,   9,  -1,  -9, -19, -41,
           -17,   3,  22,  22,  22,  11,   8, -18,
           -18,  -6,  16,  25,  16,  17,   4, -18,
           -23,  -3,  -1,  15,  10,  -3, -20, -22,
           -42, -20, -10,  -5,  -2, -20, -23, -44,
           -29, -51, -23, -15, -22, -18, -50, -64 },
        // BISHOP
        {  -14, -21, -11,  -8,  -7,  -9, -17, -24,
            -8,  -4,   7, -12,  -3, -13,  -4, -14,
             2,  -8,   0,  -1,  -2,   6,   0,   4,
            -3,   9,  12,   9,  14,  10,   3,   2,
            -6,   3,  13,  19,   7,  10,  -3,  -9,
           -12,  -3,   8,  10,  13,   3,  -7, -15,
           -14, -18,  -7,  -1,   4,  -9, -15, -27,
           -23,  -9, -23,  -5,  -9, -16,  -5, -17 },
        // ROOK
        {   -9,   2,   3,  -1,  -5, -13,   4, -20,
            -6,  -6,   0,   2,  -9,  -9, -11,  -3,
            -4,   0,  -5,  -1,  -7, -12,  -8, -16,
             3,   5,   8,   4,  -5,  -6,  -8, -11,
             4,   3,  13,   1,   2,   1,  -1,   2,
             7,   7,   7,   5,   4,  -3,  -5,  -3,
            11,  13,  13,  11,  -3,   3,   8,   3,
            13,  10,  18,  15,  12,  12,   8,   5 },
        // QUEEN
        {   -9,  22,  22,  27,  27,  19,  10,  20,
           -17,  20,  32,  41,  58,  25,  30,   0,
           -20,   6,   9,  49,  47,  35,  19,   9,
             3,  22,  24,  45,  57,  40,  57,  36,
           -18,  28,  19,  47,  31,  34,  39,  23,
           -16, -27,  15,   6,   9,  17,  10,   5,
           -22, -23, -30, -16, -16, -23, -36, -32,
           -33, -28, -22, -43,  -5, -32, -20, -41 },
        // KING
        {  -74, -35, -18, -18, -11,  15,   4, -17,
           -12,  17,  14,  17,  17,  38,  23,  11,
            10,  17,  23,  15,  20,  45,  44,  13,
            -8,  22,  24,  27,  26,  33,  26,   3,
           -18,  -4,  21,  24,  27,  23,   9, -11,
           -19,  -3,  11,  21,  23,  16,   7,  -9,
           -27, -11,   4,  13,  14,   4,  -5, -17,
           -53, -34, -21, -11, -28, -14, -24, -43 }
    };

    // ---- Passed pawn bonuses [rank 0..7] ------------------------------------
    int passed_mg[8] = {  0,  8,  8,   8,  25,  90,  90,   0 };
    int passed_eg[8] = {  0, 10, 10,  53,  84, 106, 124,   0 };

    // Supported passer (passed pawn defended by a friendly pawn)
    int pass_supp_mg      =  0;
    int pass_supp_eg_base =  0;
    int pass_supp_eg_rank =  0;  // multiplied by relative rank

    // Candidate passer (blocked but otherwise would be passed)
    int cand_mg =  0;
    int cand_eg =  0;

    // Dynamic passer bonuses (per-rank multipliers; evaluated outside pawn hash)
    int pass_free_mg =  2;  // stop square empty
    int pass_free_eg =  0;  // stop square empty
    int pass_safe_eg = 16;  // stop square not attacked by enemy (additional)

    // ---- Pawn structure (signed values; negative = penalty) ----------------
    int doubled_mg   =  -2;
    int doubled_eg   =  -8;
    int isolated_mg  =  -9;
    int isolated_eg  = -17;
    int connected_mg =  18;
    int connected_eg =   1;
    int backward_mg  =   0;
    int backward_eg  = -12;

    // ---- Bishop pair -------------------------------------------------------
    int bp_mg = 30;
    int bp_eg = 50;

    // ---- Rook bonuses ------------------------------------------------------
    int rook_open_mg          = 25;
    int rook_open_eg          = 10;
    int rook_semi_mg          = 12;
    int rook_semi_eg          =  8;
    int rook_7th_mg           = 20;
    int rook_7th_eg           = 40;
    int rook_behind_passer_mg = 15;
    int rook_behind_passer_eg = 25;
    int enemy_rook_passer_mg  = 10;  // subtracted (enemy rook behind our passer)
    int enemy_rook_passer_eg  = 20;  // subtracted

    // ---- Knight outpost ----------------------------------------------------
    int knight_outpost_mg = 25;
    int knight_outpost_eg = 15;

    // ---- Mobility (per safe square, indexed by PieceType 0..6) -------------
    int mob_mg[7] = {0, 0, 5, 5, 1, 2, 0};
    int mob_eg[7] = {0, 0, 5, 7, 7, 12, 0};

    // ---- Pawn threats to enemy pieces --------------------------------------
    int threat_minor_mg = 18;
    int threat_minor_eg = 12;
    int threat_rook_mg  = 28;
    int threat_rook_eg  = 18;
    int threat_queen_mg = 45;
    int threat_queen_eg = 30;

    // ---- King safety -------------------------------------------------------
    int ks_unit[7]   = {0, 0, 2, 2, 3, 5, 0};  // attacker weight by PieceType
    int ks_coord_bonus =  4;  // extra attack_units when 2+ attackers in zone
    int ks_open_file   =  2;  // extra attack_units for open file in front of king

    // Penalty lookup: index = clamped attack_units [0..24]
    int safety_table[25] = {
          0,   0,   5,  15,  25,  40,  55,  70,  85, 100,
        112, 120, 126, 130, 133, 136, 138, 140, 141, 142,
        143, 144, 145, 146, 147
    };

    // ---- King pawn shelter / storm -----------------------------------------
    int shelter_missing_center = 20;  // mg penalty, open center file near king
    int shelter_missing_flank  = 10;  // mg penalty, open flank file near king
    int shelter_close1         = 15;  // mg bonus for pawn 1 rank ahead of king
    int shelter_close2         =  7;  // mg bonus for pawn 2 ranks ahead of king
    int storm_weight_kf        =  7;  // mg penalty per rank, king file
    int storm_weight_adj       =  4;  // mg penalty per rank, adjacent files

    // ---- Hanging pieces penalty (indexed by PieceType 0..6) ----------------
    int hang_pen[7] = {0, 0, 22, 39, 33, 36, 0};

    // ---- Passed pawn king proximity (endgame) ------------------------------
    int prox_base = 11;  // constant in (prox_base + rel_r) distance multiplier

    // ---- Space evaluation --------------------------------------------------
    int space_mg = 2;

    // ---- Trapped bishop ----------------------------------------------------
    int trapped_mg = 60;
    int trapped_eg = 40;

    // ---- Tempo bonus -------------------------------------------------------
    int tempo = 10;
};

// ---- Helpers for iterating EVAL_PARAM_LIST generically --------------------
// Returns int* to the first element of a parameter field, whether it is a
// plain scalar (int) or a fixed-size array (int[N] / int[R][C] sub-row).
// Usage in a macro expansion: eval_param_ptr(p.member) or eval_param_cptr(p.member).
template<typename T>
inline int* eval_param_ptr(T& v) {
    if constexpr (std::is_array_v<T>) return v;
    else return &v;
}
template<typename T>
inline const int* eval_param_cptr(const T& v) {
    if constexpr (std::is_array_v<T>) return v;
    else return &v;
}

// ---- X-macro parameter registry -------------------------------------------
// Usage: EVAL_PARAM_LIST(X) expands X(display_name, member, length).
//   Arrays (length > 1): p.member[i] for i in [0, length).
//   Scalars (length == 1): (&p.member)[0] gives the single int.
//   PST sub-rows (e.g. pst_mg[0]): int[64] decaying to int*, p.pst_mg[0][i].
#define EVAL_PARAM_LIST(X)                                          \
    X(MgVal,                mg_val,                7)               \
    X(EgVal,                eg_val,                7)               \
    X(PstMgPawn,            pst_mg[0],            64)               \
    X(PstMgKnight,          pst_mg[1],            64)               \
    X(PstMgBishop,          pst_mg[2],            64)               \
    X(PstMgRook,            pst_mg[3],            64)               \
    X(PstMgQueen,           pst_mg[4],            64)               \
    X(PstMgKing,            pst_mg[5],            64)               \
    X(PstEgPawn,            pst_eg[0],            64)               \
    X(PstEgKnight,          pst_eg[1],            64)               \
    X(PstEgBishop,          pst_eg[2],            64)               \
    X(PstEgRook,            pst_eg[3],            64)               \
    X(PstEgQueen,           pst_eg[4],            64)               \
    X(PstEgKing,            pst_eg[5],            64)               \
    X(PassedMg,             passed_mg,             8)               \
    X(PassedEg,             passed_eg,             8)               \
    X(PassSuppMg,           pass_supp_mg,          1)               \
    X(PassSuppEgBase,       pass_supp_eg_base,     1)               \
    X(PassSuppEgRank,       pass_supp_eg_rank,     1)               \
    X(CandMg,               cand_mg,               1)               \
    X(CandEg,               cand_eg,               1)               \
    X(PassFreeMg,           pass_free_mg,          1)               \
    X(PassFreeEg,           pass_free_eg,          1)               \
    X(PassSafeEg,           pass_safe_eg,          1)               \
    X(DoubledMg,            doubled_mg,            1)               \
    X(DoubledEg,            doubled_eg,            1)               \
    X(IsolatedMg,           isolated_mg,           1)               \
    X(IsolatedEg,           isolated_eg,           1)               \
    X(ConnectedMg,          connected_mg,          1)               \
    X(ConnectedEg,          connected_eg,          1)               \
    X(BackwardMg,           backward_mg,           1)               \
    X(BackwardEg,           backward_eg,           1)               \
    X(BpMg,                 bp_mg,                 1)               \
    X(BpEg,                 bp_eg,                 1)               \
    X(RookOpenMg,           rook_open_mg,          1)               \
    X(RookOpenEg,           rook_open_eg,          1)               \
    X(RookSemiMg,           rook_semi_mg,          1)               \
    X(RookSemiEg,           rook_semi_eg,          1)               \
    X(Rook7thMg,            rook_7th_mg,           1)               \
    X(Rook7thEg,            rook_7th_eg,           1)               \
    X(RookBehindPasserMg,   rook_behind_passer_mg, 1)               \
    X(RookBehindPasserEg,   rook_behind_passer_eg, 1)               \
    X(EnemyRookPasserMg,    enemy_rook_passer_mg,  1)               \
    X(EnemyRookPasserEg,    enemy_rook_passer_eg,  1)               \
    X(KnightOutpostMg,      knight_outpost_mg,     1)               \
    X(KnightOutpostEg,      knight_outpost_eg,     1)               \
    X(MobMg,                mob_mg,                7)               \
    X(MobEg,                mob_eg,                7)               \
    X(ThreatMinorMg,        threat_minor_mg,       1)               \
    X(ThreatMinorEg,        threat_minor_eg,       1)               \
    X(ThreatRookMg,         threat_rook_mg,        1)               \
    X(ThreatRookEg,         threat_rook_eg,        1)               \
    X(ThreatQueenMg,        threat_queen_mg,       1)               \
    X(ThreatQueenEg,        threat_queen_eg,       1)               \
    X(KsUnit,               ks_unit,               7)               \
    X(KsCoordBonus,         ks_coord_bonus,        1)               \
    X(KsOpenFile,           ks_open_file,          1)               \
    X(SafetyTable,          safety_table,         25)               \
    X(ShelterMissingCenter, shelter_missing_center, 1)              \
    X(ShelterMissingFlank,  shelter_missing_flank,  1)              \
    X(ShelterClose1,        shelter_close1,        1)               \
    X(ShelterClose2,        shelter_close2,        1)               \
    X(StormWeightKf,        storm_weight_kf,       1)               \
    X(StormWeightAdj,       storm_weight_adj,      1)               \
    X(HangPen,              hang_pen,              7)               \
    X(ProxBase,             prox_base,             1)               \
    X(SpaceMg,              space_mg,              1)               \
    X(TrappedMg,            trapped_mg,            1)               \
    X(TrappedEg,            trapped_eg,            1)               \
    X(Tempo,                tempo,                 1)

// ---- Texel trace infrastructure (TEXEL_TRACE builds only) -----------------
// These definitions depend on EVAL_PARAM_LIST and therefore live after it.
#ifdef TEXEL_TRACE

// Group enum: one enumerator per EVAL_PARAM_LIST entry, in order.
enum EvalParamGroup : int {
#define X(name, member, len) EPG_##name,
    EVAL_PARAM_LIST(X)
#undef X
    EPG_COUNT
};

// Per-group lengths (same order as EVAL_PARAM_LIST).
constexpr int EVAL_PARAM_LENS[] = {
#define X(name, member, len) (len),
    EVAL_PARAM_LIST(X)
#undef X
};

// Flat offset: number of scalar elements before group g.
constexpr int eval_param_offset(int g) {
    int off = 0;
    for (int i = 0; i < g; i++) off += EVAL_PARAM_LENS[i];
    return off;
}

// Total flat element count across all groups (should be 900).
constexpr int EVAL_PARAM_FLAT_SIZE = eval_param_offset(EPG_COUNT);

// Per-trace record populated by Evaluator::evaluate() when TEXEL_TRACE is
// defined. mg[i] / eg[i] hold the signed count contribution of flat param i
// to the middlegame / endgame accumulator (positive = white favoured).
// phase is the computed game phase [0..24]. rest absorbs nonlinear/frozen
// contributions so that reconstruct(trace, w) + rest == actual_score_white
// when w == default weights (acceptance guarantee).
struct EvalTrace {
    int16_t mg[EVAL_PARAM_FLAT_SIZE];
    int16_t eg[EVAL_PARAM_FLAT_SIZE];
    int phase;
    int rest; // set by caller (tuner); not written by evaluate()
};

// Thread-local trace populated by every evaluate() call in TEXEL_TRACE builds.
// Caller must zero this before calling evaluate().
extern thread_local EvalTrace g_trace;

// Reconstruct the linear part of the eval from trace counts and weights.
// Returns (mg_dot * phase + eg_dot * (24 - phase)) / 24.
int reconstruct(const EvalTrace& tr, const EvalParams& w);

#endif // TEXEL_TRACE
