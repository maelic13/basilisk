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
    int mg_val[7] = { 0, 83, 323, 363, 514, 1085, 0 };
    int eg_val[7] = { 0, 94, 310, 321, 560, 998, 0 };

    // ---- PSTs: pst_mg/eg[piece_idx][sq] -------------------------------------
    // piece_idx = PieceType - 1: 0=PAWN, 1=KNIGHT, 2=BISHOP, 3=ROOK, 4=QUEEN, 5=KING
    // sq: a1=0 .. h8=63, white perspective (init_eval_tables mirrors rank for Black)
            int pst_mg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -33,   -1,  -20,  -23,  -15,   26,   39,  -23,
           -24,   -4,   -5,  -10,    3,    3,   30,  -11,
           -27,   -2,   -5,   10,   17,    6,    7,  -25,
           -14,   13,    6,   20,   22,   12,   16,  -23,
            -6,    7,   26,   31,   65,   56,   25,  -20,
            98,  134,   61,   95,   68,  126,   34,  -11,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        { -167,  -89,  -34,  -49,   61,  -97,  -15, -107,
           -73,  -41,   72,   36,   23,   62,    7,  -17,
           -47,   60,   37,   65,   84,  125,   73,   44,
            -9,   17,   19,   53,   37,   69,   18,   22,
           -13,    4,   16,   13,   28,   19,   21,   -8,
           -23,   -9,   12,   10,   19,   17,   25,  -16,
           -29,  -53,  -12,   -3,   -1,   18,  -14,  -19,
          -105,  -21,  -58,  -33,  -17,  -28,  -19,  -23 },
        // BISHOP
        {  -29,    4,  -82,  -37,  -25,  -42,    7,   -8,
           -26,   16,  -18,  -13,   30,   59,   18,  -47,
           -16,   37,   43,   40,   35,   50,   37,   -2,
            -4,    5,   19,   50,   37,   37,    7,   -2,
            -6,   13,   13,   26,   34,   12,   10,    4,
             0,   15,   15,   15,   14,   27,   18,   10,
             4,   15,   16,    0,    7,   21,   33,    1,
           -33,   -3,  -14,  -21,  -13,  -12,  -39,  -21 },
        // ROOK
        {  -19,  -13,    1,   17,   16,    7,  -37,  -26,
           -44,  -16,  -20,   -9,   -1,   11,   -6,  -71,
           -45,  -25,  -16,  -17,    3,    0,   -5,  -33,
           -36,  -26,  -12,   -1,    9,   -7,    6,  -23,
           -24,  -11,    7,   26,   24,   35,   -8,  -20,
            -5,   19,   26,   36,   17,   45,   61,   16,
            27,   32,   58,   62,   80,   67,   26,   44,
            32,   42,   32,   51,   63,    9,   31,   43 },
        // QUEEN
        {  -28,    0,   29,   12,   56,   44,   43,   45,
           -24,  -39,   -5,    1,  -16,   57,   28,   54,
           -13,  -17,    7,    8,   29,   56,   47,   57,
           -27,  -27,  -16,  -16,   -1,   17,   -2,    1,
            -9,  -26,   -9,  -10,   -2,   -4,    3,   -3,
           -14,    2,  -11,   -2,   -5,    2,   14,    5,
           -35,   -8,   11,    2,    8,   15,   -3,    1,
            -1,  -18,   -9,   10,  -15,  -25,  -31,  -50 },
        // KING
        {  -15,   36,   12,  -54,    8,  -28,   24,   12,
             1,    7,   -8,  -64,  -43,  -16,    9,    8,
           -14,  -14,  -22,  -46,  -44,  -30,  -15,  -27,
           -49,   -1,  -27,  -39,  -46,  -44,  -33,  -51,
           -17,  -20,  -12,  -27,  -30,  -25,  -14,  -36,
            -9,   24,    2,  -16,  -20,    6,   22,  -22,
            29,   -1,  -20,   -7,   -8,   -4,  -38,  -29,
           -65,   23,   16,  -15,  -56,  -34,    2,   13 }
    };

            int pst_eg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
            -7,   -4,   10,    0,   14,    6,   -5,  -19,
            -5,   -2,    7,   20,   13,   14,    0,  -13,
            15,    1,  -13,   -2,   -1,  -14,    2,   -6,
            32,   24,   13,    4,   -2,    4,   17,   16,
            56,   35,   41,   22,   26,   51,   56,   21,
           134,  108,  109,  107,  105,  104,  112,  108,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -58,  -38,  -13,  -28,  -31,  -27,  -63,  -99,
           -25,   -8,  -25,   -2,   -9,  -25,  -24,  -52,
           -24,  -20,   10,    9,   -1,  -12,  -19,  -41,
           -17,    3,   22,   22,   22,   11,    8,  -18,
           -18,   -6,   16,   25,   16,   17,    4,  -18,
           -23,   -3,   -1,   15,   10,   -3,  -20,  -22,
           -42,  -20,  -10,   -5,   -2,  -20,  -23,  -44,
           -29,  -51,  -23,  -15,  -22,  -18,  -50,  -64 },
        // BISHOP
        {  -14,  -21,  -11,   -8,   -7,   -9,  -17,  -24,
            -8,   -4,    7,  -12,   -3,  -13,   -4,  -14,
             2,   -8,    0,   -1,   -2,    6,    0,    4,
            -3,    9,   12,    9,   14,   10,    3,    2,
            -6,    3,   13,   19,    7,   10,   -3,   -9,
           -12,   -3,    8,   10,   13,    3,   -7,  -15,
           -14,  -18,   -7,   -1,    4,   -9,  -15,  -27,
           -23,   -9,  -23,   -5,   -9,  -16,   -5,  -17 },
        // ROOK
        {   -9,    2,    3,   -1,   -5,  -13,    4,  -20,
            -6,   -6,    0,    2,   -9,   -9,  -11,   -3,
            -4,    0,   -5,   -1,   -7,  -12,   -8,  -16,
             3,    5,    8,    4,   -5,   -6,   -8,  -11,
             4,    3,   13,    1,    2,    1,   -1,    2,
             7,    7,    7,    5,    4,   -3,   -5,   -3,
            11,   11,   11,    9,   -3,    3,    8,    3,
            13,   10,   18,   15,   12,   12,    8,    5 },
        // QUEEN
        {   -9,   22,   22,   27,   27,   19,   10,   20,
           -17,   20,   32,   41,   58,   25,   30,    0,
           -20,    6,    9,   49,   47,   35,   19,    9,
             3,   22,   24,   45,   57,   40,   57,   36,
           -18,   28,   19,   47,   31,   34,   39,   23,
           -16,  -27,   15,    6,    9,   17,   10,    5,
           -22,  -23,  -30,  -16,  -16,  -23,  -36,  -32,
           -33,  -28,  -22,  -43,   -5,  -32,  -20,  -41 },
        // KING
        {  -74,  -35,  -18,  -18,  -11,   15,    1,  -17,
           -12,   17,   14,   17,   17,   35,   21,   11,
            10,   17,   23,   15,   20,   44,   41,   13,
            -8,   22,   24,   27,   26,   33,   26,    3,
           -18,   -4,   21,   24,   27,   23,    9,  -11,
           -19,   -3,   11,   21,   23,   16,    7,   -9,
           -27,  -11,    4,   13,   14,    4,   -5,  -17,
           -53,  -34,  -21,  -11,  -28,  -14,  -24,  -43 }
    };

    // ---- Passed pawn bonuses [rank 0..7] ------------------------------------
    int passed_mg[8] = {  0,  8,  8,   8,  25,  90,  90,   0 };
    int passed_eg[8] = { 0, 9, 9, 53, 84, 107, 122, 0 };


    // Dynamic passer bonuses (per-rank multipliers; evaluated outside pawn hash)
    int pass_free_mg = 0;  // stop square empty
    int pass_free_eg = 0;  // stop square empty
    int pass_safe_eg = 12;  // stop square not attacked by enemy (additional)

    // ---- Pawn structure (signed values; negative = penalty) ----------------
    int doubled_mg = 0;
    int doubled_eg   =  -8;
    int isolated_mg = -7;
    int isolated_eg = -14;
    int connected_mg = 15;
    int connected_eg = 0;
    int backward_mg  =   0;
    int backward_eg = -11;

    // ---- Pawn-structure refinement (Step 3.4; seeded INERT, tuned Phase 4.5) ----
    // Pawn-cache-safe (depend only on pawns); seeded 0 so bench is unchanged. The
    // existing flat doubled/isolated/connected/backward terms stay active.
    int connected_rank_mg[8] = { 0, 2, 2, -2, -1, 0, 0, 0 };  // connected/phalanx by rel rank
    int connected_rank_eg[8] = { 0, 1, 0, 0, 1, 1, 0, 0 };
    int weak_unopposed_mg = -2;  // weak (isolated/backward) pawn on a half-open file
    int weak_unopposed_eg = -1;
    int blocked_pawn_mg[2] = { -3, 0 };  // own pawn rammed by an enemy pawn, rel rank 5 / 6
    int blocked_pawn_eg[2] = { -2, 0 };
    int pawn_majority_mg = 1;  // own pawn majority on a flank (breakthrough potential)
    int pawn_majority_eg = 4;

    // ---- Passed-pawn promotion-path safety (Step 3.4; piece-dependent, seeded 0) ----
    int passed_path_safe_eg = 3;  // promotion path free of enemy attack (x rel rank)
    int passed_block_defended_eg = 2;  // immediate block square defended by us
    int passed_king_block_eg = 13;  // (enemy - own) king distance to the block square

    // ---- Small positional terms (Step 3.7; seeded INERT, tuned Phase 4.5) ----
    // All seeded 0 so bench is unchanged; traced one-hot/linear, tuner decides.
    int reachable_outpost_mg = 2;  // per outpost square a knight can hop to
    int reachable_outpost_eg = 2;
    int bad_bishop_mg = -1;  // per own pawn on the bishop's colour
    int bad_bishop_eg = -4;
    int minor_king_ring_mg = 1;  // our minor attacking the enemy king ring
    int minor_king_ring_eg = -2;
    int rook_king_ring_mg = -2;  // our rook attacking the enemy king ring
    int rook_king_ring_eg = -2;
    int rook_closed_mg = -1;  // rook on a closed file (own + enemy pawn present)
    int rook_closed_eg = 1;
    int rook_queen_file_mg = 2;  // our rook shares a file with the enemy queen
    int rook_queen_file_eg    = 0;
    int connected_rooks_mg = 2;  // our rooks defend each other along an open line
    int connected_rooks_eg = 1;
    int bishop_pair_pawns_mg = 1;  // bishop pair scaled by own pawn count
    int bishop_pair_pawns_eg = 1;

    // ---- Material imbalance (Step 3.8; SF-style quadratic, seeded INERT) ----
    // Piece index: 0 = bishop pair, 1 = pawn, 2 = knight, 3 = bishop, 4 = rook,
    // 5 = queen. `imb_our[t]`/`imb_their[t]` are lower-triangular (t = i*(i+1)/2+j,
    // j <= i) coefficients of own-vs-own and own-vs-enemy piece-count products;
    // `imb_linear[i]` is the per-piece linear term. The eval is LINEAR in these
    // coefficients (feature = count products), so they are traced normally and
    // fit by the linear tuner in Phase 4. All seeded 0 -> contributes 0 today.
    int imb_linear[6] = { 0, 0, 0, 2, 2, 2 };
    int imb_our[21] = { 0, 0, 3, 0, 5, 0, 0, 3, 1, 2, 0, 4, 1, 2, 2, 0, 7, 0, 1, 2, 2 };
    int imb_their[21] = { 0, 0, 0, 0, 4, 0, 0, 6, -1, 0, 0, 9, 2, -2, 0, 0, 13, 1, 2, 2, 0 };

    // ---- HCE survey additions (Step 3.9; SF11-classical, seeded INERT) ------
    // All seeded 0 so bench is unchanged; traced, tuned in Phase 4.5.
    int minor_behind_pawn_mg = 2;  // minor shielded by a friendly pawn directly ahead
    int minor_behind_pawn_eg  = 0;
    int king_protector_mg = -1;  // per square of own-minor distance to our king (penalty)
    int king_protector_eg = 1;
    int queen_infiltration_mg = 4;  // our queen safely deep in the enemy half
    int queen_infiltration_eg = 3;
    int pawn_islands_mg       = 0;  // per disconnected own-pawn group (penalty)
    int pawn_islands_eg = 4;

    // ---- Bishop pair -------------------------------------------------------
    int bp_mg = 30;
    int bp_eg = 50;

    // ---- Rook bonuses ------------------------------------------------------
    int rook_open_mg          = 25;
    int rook_open_eg = 8;
    int rook_semi_mg = 13;
    int rook_semi_eg = 10;
    int rook_7th_mg = 16;
    int rook_7th_eg = 33;
    int rook_behind_passer_mg = 15;
    int rook_behind_passer_eg = 25;
    int enemy_rook_passer_mg  = 10;  // subtracted (enemy rook behind our passer)
    int enemy_rook_passer_eg  = 20;  // subtracted

    // ---- Knight outpost ----------------------------------------------------
    int knight_outpost_mg = 25;
    int knight_outpost_eg = 15;

    // ---- Mobility: per-count one-hot tables (Step 3.3) ---------------------
    // Indexed by the safe-mobility square count. Seeded table[i] = i * old linear
    // weight (knight/bishop/rook/queen mg = 5/5/1/2, eg = 5/7/7/12) so the eval is
    // byte-identical to the previous linear `mob * w` form; tuned in Phase 4.4.
    // (The mobility *area* is unchanged here — its SF-style refinement, excluding
    // own K/Q, blocked pawns, and pinned pieces via blockers_for_king, is a
    // behaviour change deferred to Phase 4.4 where it interacts with the fit.)
    int mob_n_mg[9]  = { 0, 5, 10, 15, 20, 25, 30, 35, 40 };
    int mob_n_eg[9] = { 0, 5, 10, 16, 21, 26, 30, 35, 38 };
    int mob_b_mg[14] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65 };
    int mob_b_eg[14] = { 0, 7, 14, 21, 28, 37, 43, 50, 56, 63, 69, 76, 84, 91 };
    int mob_r_mg[15] = { -1, 1, 2, 3, 4, 5, 7, 8, 9, 9, 10, 11, 12, 13, 14 };
    int mob_r_eg[15] = { 0, 7, 14, 21, 28, 36, 44, 50, 57, 64, 70, 77, 83, 90, 96 };
    int mob_q_mg[28] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54 };
    int mob_q_eg[28] = { 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276, 288, 300, 312, 324 };

    // ---- Pawn threats to enemy pieces (re-tuned Phase 4.2) -----------------
    int threat_minor_mg = 58;
    int threat_minor_eg = 25;
    int threat_rook_mg  = 58;
    int threat_rook_eg  = 25;
    int threat_queen_mg = 58;
    int threat_queen_eg = 25;

    // ---- Threats package (Step 3.1; SF-style, seeded INERT, tuned in Phase 4.2) ----
    // Every weight seeded 0 so the structure computes + traces but does not change
    // eval (bench identical); the Phase-4.2 fit activates them. Per-type arrays are
    // indexed by the attacked PieceType [0..6].
    // (Phase 4.2 fit; the new package absorbed the old flat hang_pen, now zeroed.)
    int threat_by_minor_mg[7] = {0, -1, 32, 47, 56, 51, 0};  // weak/defended enemy attacked by N/B
    int threat_by_minor_eg[7] = {0, 25,  2, 11, 22, 30, 0};
    int threat_by_rook_mg[7]  = {0, -6, 15, 21,  4, 46, 0};  // weak enemy attacked by rook
    int threat_by_rook_eg[7] = { 0, 21, 18, 19, 8, 33, 0 };
    int threat_by_king_mg     = 20;  // weak enemy attacked by our king
    int threat_by_king_eg     = 40;
    int threat_hanging_mg     = 22;  // per weak enemy that is undefended or doubly attacked
    int threat_hanging_eg     = 25;
    int weak_queen_prot_mg = 17;  // per weak enemy whose only defender is its queen
    int weak_queen_prot_eg    =  7;
    int restricted_mg = 3;  // per square the enemy attacks but cannot safely hold
    int restricted_eg         = -5;
    int threat_push_mg        = 18;  // per enemy non-pawn attackable by a safe pawn push
    int threat_push_eg        = 14;

    // ---- King safety (Phase 4.1 fit via --tune-kingsafety) -----------------
    int ks_unit[7]   = {0, 0, 4, 0, 1, 0, 0};  // attacker weight by PieceType
    int ks_coord_bonus =  4;  // extra attack_units when 2+ attackers in zone
    int ks_open_file   =  0;  // extra attack_units for open file in front of king

    // Penalty lookup: index = clamped attack_units [0..24]
    int safety_table[25] = { -1, 0, 21, 22, 34, 37, 38, 75, 75, 81, 89, 120, 120, 129, 148, 173, 187, 202, 202, 209, 214, 243, 253, 296, 296 };

    // ---- King safety v2 danger inputs (Step 3.2; seeded INERT, tuned Phase 4.3) ----
    // These feed the attack_units -> safety_table funnel as extra index
    // contributions. All seeded 0 (and the no-queen scaling seeded to reproduce
    // the old frozen *2/3) so bench is unchanged; the Phase-4.3 nonlinear
    // (finite-difference) tuner activates them. They are composite/index inputs,
    // not pure-linear, so they carry no linear trace (only SafetyTable is traced).
    int ks_safe_check[7]  = {0, 0, 7, 6, 8, 6, 0};  // per safe check, by checker type (Phase 4.1)
    int ks_weak_ring      = 0;  // per king-ring square attacked and not solidly defended
    int ks_ring_pressure  = 0;  // per king-ring square the enemy attacks
    int ks_flank_attack   = 0;  // per enemy attack in the king's flank
    int ks_flank_defense  = 0;  // per friendly defense in the king's flank (subtracted)
    int ks_pawnless_flank = 0;  // king on a flank with no friendly pawns
    int ks_king_blockers  = 4;  // per own piece pinned/blocking in front of the king (Phase 4.1)
    int ks_central_king   = 0;  // king central with castling rights gone
    int ks_shelter_storm  = 0;  // king pawn-shelter exposure folded into danger
    int ks_noqueen_num    = 2;  // no-enemy-queen danger scaling numerator (old 2/3)
    int ks_noqueen_den    = 3;  // ... denominator

    // ---- King pawn shelter / storm -----------------------------------------
    int shelter_missing_center = 19;  // mg penalty, open center file near king
    int shelter_missing_flank = 9;  // mg penalty, open flank file near king
    int shelter_close1         = 15;  // mg bonus for pawn 1 rank ahead of king
    int shelter_close2         =  7;  // mg bonus for pawn 2 ranks ahead of king
    int storm_weight_kf = 4;  // mg penalty per rank, king file
    int storm_weight_adj = 2;  // mg penalty per rank, adjacent files

    // ---- Hanging pieces penalty (indexed by PieceType 0..6) ----------------
    int hang_pen[7] = {0, 0, 0, 0, 0, 0, 0};  // dropped Phase 4.2 (absorbed by threats package)

    // ---- Passed pawn king proximity (endgame) ------------------------------
    int prox_base = 1;  // constant in (prox_base + rel_r) distance multiplier

    // ---- Space evaluation --------------------------------------------------
    int space_mg = 0;
    // Step 3.6 space refinement (seeded 0; tuned in Phase 4.5):
    int space_piece_mg = 0;  // space weighted by own non-pawn piece count
    int space_blocked_mg = -2;  // space weighted by own blocked-pawn count

    // ---- Trapped bishop ----------------------------------------------------
    int trapped_mg = 60;
    int trapped_eg = 40;

    // ---- Tempo bonus -------------------------------------------------------
    int tempo = 15;

    // ---- Winnable / complexity coupling (Step 3.6) -------------------------
    // Frozen (not linearly traced); all seeded 0 so the eval/bench are
    // unchanged. Tuned via the finite-difference path in Phase 4.5.
    int win_outflanking  = 0;  // king file-distance minus rank-distance
    int win_both_flanks  = 0;  // pawns present on both flanks
    int win_infiltration = 0;  // a king advanced into enemy territory
    int win_pawn_endgame = 0;  // no non-pawn material on the board
    int win_passed       = 0;  // total passed-pawn count
    int win_total_pawns  = 0;  // total pawn count
    int win_bias         = 0;  // constant complexity offset
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
    X(ConnectedRankMg,      connected_rank_mg,     8)               \
    X(ConnectedRankEg,      connected_rank_eg,     8)               \
    X(WeakUnopposedMg,      weak_unopposed_mg,     1)               \
    X(WeakUnopposedEg,      weak_unopposed_eg,     1)               \
    X(BlockedPawnMg,        blocked_pawn_mg,       2)               \
    X(BlockedPawnEg,        blocked_pawn_eg,       2)               \
    X(PawnMajorityMg,       pawn_majority_mg,      1)               \
    X(PawnMajorityEg,       pawn_majority_eg,      1)               \
    X(PassedPathSafeEg,     passed_path_safe_eg,   1)               \
    X(PassedBlockDefendedEg, passed_block_defended_eg, 1)           \
    X(PassedKingBlockEg,    passed_king_block_eg,  1)               \
    X(ReachableOutpostMg,   reachable_outpost_mg,  1)               \
    X(ReachableOutpostEg,   reachable_outpost_eg,  1)               \
    X(BadBishopMg,          bad_bishop_mg,         1)               \
    X(BadBishopEg,          bad_bishop_eg,         1)               \
    X(MinorKingRingMg,      minor_king_ring_mg,    1)               \
    X(MinorKingRingEg,      minor_king_ring_eg,    1)               \
    X(RookKingRingMg,       rook_king_ring_mg,     1)               \
    X(RookKingRingEg,       rook_king_ring_eg,     1)               \
    X(RookClosedMg,         rook_closed_mg,        1)               \
    X(RookClosedEg,         rook_closed_eg,        1)               \
    X(RookQueenFileMg,      rook_queen_file_mg,    1)               \
    X(RookQueenFileEg,      rook_queen_file_eg,    1)               \
    X(ConnectedRooksMg,     connected_rooks_mg,    1)               \
    X(ConnectedRooksEg,     connected_rooks_eg,    1)               \
    X(BishopPairPawnsMg,    bishop_pair_pawns_mg,  1)               \
    X(BishopPairPawnsEg,    bishop_pair_pawns_eg,  1)               \
    X(ImbLinear,            imb_linear,            6)               \
    X(ImbOur,               imb_our,              21)               \
    X(ImbTheir,             imb_their,            21)               \
    X(MinorBehindPawnMg,    minor_behind_pawn_mg,  1)               \
    X(MinorBehindPawnEg,    minor_behind_pawn_eg,  1)               \
    X(KingProtectorMg,      king_protector_mg,     1)               \
    X(KingProtectorEg,      king_protector_eg,     1)               \
    X(QueenInfiltrationMg,  queen_infiltration_mg, 1)               \
    X(QueenInfiltrationEg,  queen_infiltration_eg, 1)               \
    X(PawnIslandsMg,        pawn_islands_mg,       1)               \
    X(PawnIslandsEg,        pawn_islands_eg,       1)               \
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
    X(MobNMg,               mob_n_mg,              9)               \
    X(MobNEg,               mob_n_eg,              9)               \
    X(MobBMg,               mob_b_mg,             14)               \
    X(MobBEg,               mob_b_eg,             14)               \
    X(MobRMg,               mob_r_mg,             15)               \
    X(MobREg,               mob_r_eg,             15)               \
    X(MobQMg,               mob_q_mg,             28)               \
    X(MobQEg,               mob_q_eg,             28)               \
    X(ThreatMinorMg,        threat_minor_mg,       1)               \
    X(ThreatMinorEg,        threat_minor_eg,       1)               \
    X(ThreatRookMg,         threat_rook_mg,        1)               \
    X(ThreatRookEg,         threat_rook_eg,        1)               \
    X(ThreatQueenMg,        threat_queen_mg,       1)               \
    X(ThreatQueenEg,        threat_queen_eg,       1)               \
    X(ThreatByMinorMg,      threat_by_minor_mg,    7)               \
    X(ThreatByMinorEg,      threat_by_minor_eg,    7)               \
    X(ThreatByRookMg,       threat_by_rook_mg,     7)               \
    X(ThreatByRookEg,       threat_by_rook_eg,     7)               \
    X(ThreatByKingMg,       threat_by_king_mg,     1)               \
    X(ThreatByKingEg,       threat_by_king_eg,     1)               \
    X(ThreatHangingMg,      threat_hanging_mg,     1)               \
    X(ThreatHangingEg,      threat_hanging_eg,     1)               \
    X(WeakQueenProtMg,      weak_queen_prot_mg,    1)               \
    X(WeakQueenProtEg,      weak_queen_prot_eg,    1)               \
    X(RestrictedMg,         restricted_mg,         1)               \
    X(RestrictedEg,         restricted_eg,         1)               \
    X(ThreatPushMg,         threat_push_mg,        1)               \
    X(ThreatPushEg,         threat_push_eg,        1)               \
    X(KsUnit,               ks_unit,               7)               \
    X(KsCoordBonus,         ks_coord_bonus,        1)               \
    X(KsOpenFile,           ks_open_file,          1)               \
    X(SafetyTable,          safety_table,         25)               \
    X(KsSafeCheck,          ks_safe_check,         7)               \
    X(KsWeakRing,           ks_weak_ring,          1)               \
    X(KsRingPressure,       ks_ring_pressure,      1)               \
    X(KsFlankAttack,        ks_flank_attack,       1)               \
    X(KsFlankDefense,       ks_flank_defense,      1)               \
    X(KsPawnlessFlank,      ks_pawnless_flank,     1)               \
    X(KsKingBlockers,       ks_king_blockers,      1)               \
    X(KsCentralKing,        ks_central_king,       1)               \
    X(KsShelterStorm,       ks_shelter_storm,      1)               \
    X(KsNoqueenNum,         ks_noqueen_num,        1)               \
    X(KsNoqueenDen,         ks_noqueen_den,        1)               \
    X(ShelterMissingCenter, shelter_missing_center, 1)              \
    X(ShelterMissingFlank,  shelter_missing_flank,  1)              \
    X(ShelterClose1,        shelter_close1,        1)               \
    X(ShelterClose2,        shelter_close2,        1)               \
    X(StormWeightKf,        storm_weight_kf,       1)               \
    X(StormWeightAdj,       storm_weight_adj,      1)               \
    X(HangPen,              hang_pen,              7)               \
    X(ProxBase,             prox_base,             1)               \
    X(SpaceMg,              space_mg,              1)               \
    X(SpacePieceMg,         space_piece_mg,        1)               \
    X(SpaceBlockedMg,       space_blocked_mg,      1)               \
    X(TrappedMg,            trapped_mg,            1)               \
    X(TrappedEg,            trapped_eg,            1)               \
    X(Tempo,                tempo,                 1)               \
    X(WinOutflanking,       win_outflanking,       1)               \
    X(WinBothFlanks,        win_both_flanks,       1)               \
    X(WinInfiltration,      win_infiltration,      1)               \
    X(WinPawnEndgame,       win_pawn_endgame,      1)               \
    X(WinPassed,            win_passed,            1)               \
    X(WinTotalPawns,        win_total_pawns,       1)               \
    X(WinBias,              win_bias,              1)

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
