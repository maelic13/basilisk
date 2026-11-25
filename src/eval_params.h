#pragma once

#include <cstdint>
#include <span>
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

#ifdef BASILISK_EVAL_REBUILD
#include "eval_rb_params.h"
#else
struct EvalParams {

    // ---- Material values [PieceType 0..6] -----------------------------------
    int mg_val[7] = { 0, 83, 323, 363, 514, 1085, 0 };
    int eg_val[7] = { 0, 94, 310, 323, 562, 998, 0 };

    // ---- PSTs: pst_mg/eg[piece_idx][sq] -------------------------------------
    // piece_idx = PieceType - 1: 0=PAWN, 1=KNIGHT, 2=BISHOP, 3=ROOK, 4=QUEEN, 5=KING
    // sq: a1=0 .. h8=63, white perspective (init_eval_tables mirrors rank for Black)
    int pst_mg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
          -144, -139, -134, -126, -115,  -97, -109, -155,
          -160, -160, -154, -152, -135, -148, -142, -154,
          -149, -158, -148, -154, -152, -137, -153, -146,
          -121, -141, -138, -130, -122, -121, -128, -112,
           -96,  -99,  -93,  -78,  -64,  -25,  -51,  -68,
           -92,  -72,  -45,  -31,  -15, -125, -201, -220,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -81,  -60,  -86,  -55,  -60,  -68,  -80,  -90,
           -68,  -58,  -55,  -50,  -51,  -58,  -59,  -66,
           -38,  -32,  -37,  -38,  -30,  -40,  -31,  -48,
           -12,    6,    1,  -14,   -8,   -5,   -8,  -32,
           -13,   -5,    2,   32,   -8,   16,   -3,    7,
           -10,  -26,   27,   19,   47,   79,   20,   -9,
           -24,  -33,   12,   14,   20,   41,   -1,   62,
          -181, -157,    3,    9,    5,  -36,   64, -118 },
        // BISHOP
        {  -24,  -15,  -50,  -59,  -47,  -56,  -37,  -59,
           -19,  -35,  -30,  -56,  -50,  -37,  -36,  -52,
           -26,   -5,  -31,  -35,  -41,  -34,  -15,  -31,
           -12,  -12,  -14,  -24,  -12,  -22,  -23,  -13,
           -27,   -5,   -2,   13,   -3,    3,  -20,  -34,
           -21,  -13,   -8,   14,   27,   58,   23,   -2,
           -52,  -27,  -16,   -9,  -28,   -3,  -50,  -57,
           -23,  -37,  -85, -121,  -37, -113,  -54,  -56 },
        // ROOK
        {  -86,  -81,  -82,  -72,  -61,  -58,  -51,  -80,
           -98,  -87,  -84,  -74,  -71,  -51,  -41,  -69,
           -93,  -79,  -88,  -76,  -65,  -57,  -50,  -55,
          -109,  -82,  -81,  -73,  -56,  -55,  -33,  -72,
           -89,  -65,  -57,  -41,  -21,  -20,  -27,  -31,
           -82,  -54,  -47,  -42,   26,   28,   44,  -23,
           -82,  -75,  -49,  -26,  -24,   49,    3,  -18,
           -52,  -31,  -27,   -2,   11,   27,   30,   63 },
        // QUEEN
        {   59,   55,   57,   53,   73,   77,   78,   70,
            45,   47,   46,   58,   59,   71,   77,   76,
            55,   46,   53,   45,   51,   75,   65,   71,
            46,   50,   44,   47,   63,   68,   53,   60,
            84,   95,   97,   87,  103,  114,   87,  109,
            78,   79,   80,  104,   85,  144,  104,  124,
            52,   45,   80,   76,   69,  113,   95,  155,
            47,  103,  100,  131,   71,  182,  213,  179 },
        // KING
        {    5,   28,   10,  -70,  -14,  -54,   21,   30,
            39,    3,   -6,   -8,  -16,  -16,   24,   20,
            22,   35,   11,    0,  -23,  -33,  -14,  -35,
            33,   56,   -9,  -20,  -32,  -31,  -51, -122,
           -38,   29,   11,   -7,    1,  -20,  -33, -134,
           -13,   32,   18,  -44,   -4,   50,   18,  -77,
           -41,  -27,   23,   27,  -57,  -41,   26, -154,
           -66,   35,   -5,  -13,  -27, -119,    0, -137 }
    };

    int pst_eg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
            33,   30,   14,   -7,   32,   20,   12,   -2,
            24,   13,    2,   -4,    2,    4,   -6,   -7,
            33,   26,    4,    2,    4,    8,    8,    0,
            69,   46,   24,    2,   11,   19,   31,   30,
           186,  151,  109,   63,   73,  110,  127,  134,
           270,  232,  214,  152,  154,  194,  220,  244,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  114,   50,   74,   88,   70,   63,   53,  121,
            82,   81,   72,   71,   61,   59,   71,   86,
            78,   69,   66,   78,   80,   58,   59,   75,
            95,   78,   83,   98,  104,   92,   88,  109,
           107,   77,   81,   96,   91,   90,   89,   84,
            63,   82,   81,   80,   69,   73,   74,   88,
            73,   88,   68,  105,   75,   62,   75,   66,
            70,  112,   66,   78,   73,   67,   27,   31 },
        // BISHOP
        {  100,   86,   94,   87,   93,   95,   63,   84,
           115,   86,   84,   95,   91,   88,   81,   58,
            95,  106,  108,   97,  111,   91,   82,   84,
            85,   98,  106,  108,  101,  102,   95,   70,
           100,  100,   93,  107,  112,   96,  111,   94,
            96,  106,  102,   95,  104,  108,  104,  103,
           117,   99,   91,   92,   97,  105,  111,   78,
           114,   82,  110,  114,   90,  109,  109,  126 },
        // ROOK
        {  185,  184,  188,  179,  163,  174,  158,  155,
           176,  178,  187,  172,  164,  157,  145,  133,
           181,  192,  193,  173,  164,  177,  169,  149,
           215,  207,  206,  195,  188,  199,  191,  178,
           222,  217,  205,  194,  185,  195,  199,  192,
           216,  213,  200,  197,  164,  175,  173,  185,
           200,  198,  192,  182,  171,  162,  177,  176,
           208,  211,  218,  210,  197,  198,  190,  180 },
        // QUEEN
        {  167,  179,  176,  213,  153,  106,   62,  102,
           223,  202,  229,  222,  226,  184,  134,  164,
           204,  245,  248,  262,  261,  252,  239,  234,
           263,  261,  288,  296,  288,  280,  292,  293,
            76,  105,  101,  149,  154,  143,  178,  135,
            70,  100,  122,  124,  192,  151,  171,  120,
           104,  107,  148,  167,  201,  170,  136,  101,
           136,  112,  142,  126,  209,  117,   70,   77 },
        // KING
        {  -45,  -36,   -5,    0,  -20,   -1,  -41,  -72,
           -34,   -9,    3,    4,   11,    6,  -12,  -28,
           -49,  -26,    8,   21,   22,   14,   -4,  -13,
           -49,  -18,   21,   34,   35,   28,   10,   -7,
           -17,    7,   35,   47,   47,   45,   33,   12,
           -11,   30,   46,   64,   59,   59,   56,   31,
           -88,   -3,   32,   38,   42,   48,   54,   10,
          -153,  -54,  -52,  -25,    1,   32,   22,  -45 }
    };

    // ---- Passed pawn bonuses [rank 0..7] ------------------------------------
    int passed_mg[8] = { 0, 0, 0, 8, 17, 17, 91, 0 };
    int passed_eg[8] = { 0, 0, 0, 13, 43, 70, 123, 0 };


    // Dynamic passer bonuses (per-rank multipliers; evaluated outside pawn hash)
    int pass_free_mg = 0;  // stop square empty
    int pass_free_eg = 10;  // stop square empty
    int pass_safe_eg = 15;  // stop square not attacked by enemy (additional)

    // ---- Pawn structure (signed values; negative = penalty) ----------------
    int doubled_mg = -1;
    int doubled_eg = -14;
    int isolated_mg = 0;
    int isolated_eg = -8;
    int connected_mg = 7;
    int connected_eg = 0;
    int backward_mg = 0;
    int backward_eg = -6;

    // ---- Pawn-structure refinement (Step 3.4) ----------------------------------
    // Pawn-cache-safe (depend only on pawns). The flat doubled/isolated/
    // connected/backward terms stay active beside them.
    int connected_rank_mg[8] = { 0, -1, 14, 11, 20, 55, 215, 0 };  // connected/phalanx by rel rank
    int connected_rank_eg[8] = { 0, -1, 15, 15, 30, 55, 112, 0 };
    int weak_unopposed_mg = -11;  // weak (isolated/backward) pawn on a half-open file
    int weak_unopposed_eg = -8;
    int blocked_pawn_mg[2] = { -23, -26 };  // own pawn rammed by an enemy pawn, rel rank 5 / 6
    int blocked_pawn_eg[2] = { -17, -84 };
    int pawn_majority_mg = -12;  // own pawn majority on a flank (breakthrough potential)
    int pawn_majority_eg = 37;

    // ---- Passed-pawn promotion-path safety (Step 3.4; piece-dependent) ----
    int passed_path_safe_eg = 14;  // promotion path free of enemy attack (x rel rank)
    int passed_block_defended_eg = 6;  // immediate block square defended by us
    int passed_king_block_eg = 14;  // (enemy - own) king distance to the block square

    // ---- Small positional terms (Step 3.7) ---------------------------------
    // Traced one-hot/linear and fitted by the tuner.
    int reachable_outpost_mg = 10;  // per outpost square a knight can hop to
    int reachable_outpost_eg = 13;
    int bad_bishop_mg = -2;  // per own pawn on the bishop's colour
    int bad_bishop_eg = -12;
    int minor_king_ring_mg = 3;  // our minor attacking the enemy king ring
    int minor_king_ring_eg = 1;
    int rook_king_ring_mg = -40;  // our rook attacking the enemy king ring
    int rook_king_ring_eg = -6;
    int rook_closed_mg = -17;  // rook on a closed file (own + enemy pawn present)
    int rook_closed_eg = -1;
    int rook_queen_file_mg = 18;  // our rook shares a file with the enemy queen
    int rook_queen_file_eg = -2;
    int connected_rooks_mg = -5;  // our rooks defend each other along an open line
    int connected_rooks_eg = 33;
    int bishop_pair_pawns_mg = 11;  // bishop pair scaled by own pawn count
    int bishop_pair_pawns_eg = 7;

    // ---- Material imbalance (Step 3.8; SF-style quadratic) ----------------
    // Piece index: 0 = bishop pair, 1 = pawn, 2 = knight, 3 = bishop, 4 = rook,
    // 5 = queen. `imb_our[t]`/`imb_their[t]` are lower-triangular (t = i*(i+1)/2+j,
    // j <= i) coefficients of own-vs-own and own-vs-enemy piece-count products;
    // `imb_linear[i]` is the per-piece linear term. The eval is LINEAR in these
    // coefficients (feature = count products), so they are traced normally and
    // fit by the linear tuner.
    int imb_linear[6] = { 0, 0, 0, 4, 8, 2 };
    int imb_our[21] = { 0, 2, 4, -11, 24, -15, 0, 15, -26, 3, -40, 34, -12, -9, -11, -49, 62, -52, -58, -75, 2 };
    int imb_their[21] = { 0, 8, 0, -1, 18, 0, 13, 22, -2, 0, -21, 19, -11, -13, 0, 1, 27, 5, 4, 3, 0 };

    // ---- HCE survey additions (Step 3.9; SF11-classical) --------------------
    // Traced and fitted.
    int minor_behind_pawn_mg = 10;  // minor shielded by a friendly pawn directly ahead
    int minor_behind_pawn_eg = 1;
    // ---- Simpler-form repairs (seeded to preserve behaviour) --------------
    // king_protector was ONE scalar applied to both knights and bishops, so the
    // fit could never learn that the two pieces value king proximity
    // differently. Split per piece type, so the fit can separate them.
    int king_protector_n_mg = -10;   // knight distance-to-own-king
    int king_protector_n_eg = -3;
    int king_protector_b_mg = -9;   // bishop distance-to-own-king
    int king_protector_b_eg = -1;
    // Outposts were priced for knights only. A bishop on a protected advanced
    // square the enemy cannot challenge with a pawn is the same concept and was
    // simply missing. Seeded inert.
    // ---- Coverage close-out -------------------------------------------------
    // The six concepts BAS-E07 found absent from our evaluator plus the
    // safe-square qualifier our pawn threats lacked. Values come from a joint
    // fit, never from hand-reasoning: BAS-X11 records
    // Manta losing ~-23 Elo across two gates by hand-scaling exactly this class
    // of reference-family concept.

    int queen_infiltration_mg = -45;  // our queen safely deep in the enemy half
    int queen_infiltration_eg = 186;
    int pawn_islands_mg = -14;  // per disconnected own-pawn group (penalty)
    int pawn_islands_eg = 7;

    // ---- Bishop pair -------------------------------------------------------
    int bp_mg = 87;
    int bp_eg = 26;

    // ---- Rook bonuses ------------------------------------------------------
    int rook_open_mg = 33;
    int rook_open_eg = 14;
    int rook_semi_mg = 10;
    int rook_semi_eg = 0;
    int rook_7th_mg = 4;
    int rook_7th_eg = 22;
    int rook_behind_passer_mg = 3;
    int rook_behind_passer_eg = 21;
    int enemy_rook_passer_mg = 7;  // subtracted (enemy rook behind our passer)
    int enemy_rook_passer_eg = 102;  // subtracted

    // ---- Knight outpost ----------------------------------------------------
    int knight_outpost_mg = 19;
    int knight_outpost_eg = 20;

    // ---- Mobility: per-count one-hot tables (Step 3.3) ---------------------
    // Indexed by the safe-mobility square count. First seeded as table[i] = i *
    // the old linear weight (knight/bishop/rook/queen mg = 5/5/1/2, eg =
    // 5/7/7/12), then fitted.
    // (The mobility *area* is unchanged here — its SF-style refinement, excluding
    // own K/Q, blocked pawns, and pinned pieces via blockers_for_king, is a
    // behaviour change deferred to Phase 4.4 where it interacts with the fit.)
    int mob_n_mg[9] = { -22, -12, 9, 21, 27, 32, 37, 43, 44 };
    int mob_n_eg[9] = { -177, -35, 11, 34, 52, 71, 72, 77, 77 };
    int mob_b_mg[14] = { -16, -2, 12, 20, 28, 35, 37, 38, 40, 42, 42, 50, 65, 65 };
    int mob_b_eg[14] = { -70, -14, 15, 34, 47, 55, 63, 69, 74, 74, 74, 74, 74, 74 };
    int mob_r_mg[15] = { -39, -16, 4, 4, 9, 9, 9, 11, 12, 13, 13, 18, 24, 26, 26 };
    int mob_r_eg[15] = { -127, 12, 16, 34, 41, 50, 62, 68, 73, 77, 84, 86, 86, 91, 91 };
    int mob_q_mg[28] = { 0, 0, 0, 4, 16, 16, 16, 20, 24, 26, 28, 29, 33, 33, 34, 35, 35, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37 };
    int mob_q_eg[28] = { 0, 0, 0, 0, 15, 103, 123, 143, 159, 179, 185, 195, 203, 205, 210, 212, 214, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217, 217 };

    // ---- Pawn threats to enemy pieces (re-tuned Phase 4.2) -----------------
    int threat_minor_mg = 87;
    int threat_minor_eg = 57;
    int threat_rook_mg = 87;
    int threat_rook_eg = 57;
    int threat_queen_mg = 87;
    int threat_queen_eg = 57;

    // ---- Threats package (Step 3.1; SF-style) -------------------------------
    // Per-type arrays are indexed by the attacked PieceType [0..6].
    // (Phase 4.2 fit; it absorbed the old flat hanging-piece penalty.)
    int threat_by_minor_mg[7] = { 0, -3, 32, 46, 81, 70, 0 };  // weak/defended enemy attacked by N/B
    int threat_by_minor_eg[7] = { 0, 28, 44, 31, 33, 55, 0 };
    int threat_by_rook_mg[7] = { 0, -7, 19, 25, 6, 48, 0 };  // weak enemy attacked by rook
    int threat_by_rook_eg[7] = { 0, 36, 53, 62, 11, 94, 0 };
    int threat_by_king_mg = 15;  // weak enemy attacked by our king
    int threat_by_king_eg = 47;
    int threat_hanging_mg = 18;  // per weak enemy that is undefended or doubly attacked
    int threat_hanging_eg = 8;
    int weak_queen_prot_mg = 6;  // per weak enemy whose only defender is its queen
    int weak_queen_prot_eg = 14;
    int restricted_mg = 8;  // per square the enemy attacks but cannot safely hold
    int restricted_eg = -2;
    int threat_push_mg = 24;  // per enemy non-pawn attackable by a safe pawn push
    int threat_push_eg = 29;

    // ---- King safety (Phase 4.1 fit via --tune-kingsafety) -----------------
    int ks_unit[7] = { 0, 0, 2, 0, 4, 0, 0 };  // attacker weight by PieceType
    int ks_coord_bonus = 2;  // extra attack_units when 2+ attackers in zone

    // Penalty lookup: index = clamped attack_units [0..24]
    int safety_table[25] = { -3, 0, 0, 11, 11, 20, 35, 35, 61, 69, 93, 129, 152, 189, 189, 256, 256, 271, 271, 339, 339, 422, 422, 493, 553 };

    // ---- King safety v2 danger inputs (Step 3.2) ----------------------------
    // These feed the attack_units -> safety_table funnel as extra index
    // contributions, fitted by the Phase-4.3 nonlinear (finite-difference)
    // tuner. They are composite/index inputs,
    // not pure-linear, so they carry no linear trace (only SafetyTable is traced).
    int ks_safe_check[7] = { 0, 0, 5, 3, 6, 4, 0 };  // per safe check, by checker type (Phase 4.1)
    int ks_weak_ring = 1;  // per king-ring square attacked and not solidly defended
    int ks_king_blockers = 3;  // per own piece pinned/blocking in front of the king (Phase 4.1)
    int ks_central_king = 1;  // king central with castling rights gone
    int ks_shelter_storm = 1;  // king pawn-shelter exposure folded into danger
    int ks_noqueen_num = 3;  // no-enemy-queen danger scaling numerator
    int ks_noqueen_den = 5;  // ... denominator

    // ---- King pawn shelter / storm -----------------------------------------
    int shelter_missing_center = 19;  // mg penalty, open center file near king
    int shelter_missing_flank = -8;  // mg penalty, open flank file near king
    int shelter_close1 = 21;  // mg bonus for pawn 1 rank ahead of king
    int shelter_close2 = 14;  // mg bonus for pawn 2 ranks ahead of king
    int storm_weight_kf = -4;  // mg penalty per rank, king file
    int storm_weight_adj = -1;  // mg penalty per rank, adjacent files

    // ---- Passed pawn king proximity (endgame) ------------------------------
    int prox_base = 0;  // constant in (prox_base + rel_r) distance multiplier

    // ---- Space evaluation --------------------------------------------------
    int space_mg = 0;
    // Step 3.6 space refinement:
    int space_piece_mg = 1;  // space weighted by own non-pawn piece count
    int space_blocked_mg = -1;  // space weighted by own blocked-pawn count

    // ---- Trapped bishop ----------------------------------------------------
    int trapped_mg = 0;
    int trapped_eg = 0;

    // ---- Tempo bonus -------------------------------------------------------
    int tempo = 27;
};
#endif // BASILISK_EVAL_REBUILD

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

// Span forms. The X-macro already knows each field's length, so carrying it
// alongside a bare pointer -- in a std::pair, or as a second loop bound -- is
// an invitation to pass the wrong one. A span makes the length intrinsic and
// bounds-checkable, and compiles to the same code.
template<typename T>
inline std::span<int> eval_param_span(T& v, size_t len) {
    return std::span<int>(eval_param_ptr(v), len);
}
template<typename T>
inline std::span<const int> eval_param_cspan(const T& v, size_t len) {
    return std::span<const int>(eval_param_cptr(v), len);
}

// ---- X-macro parameter registry -------------------------------------------
// Usage: EVAL_PARAM_LIST(X) expands X(display_name, member, length).
//   Arrays (length > 1): p.member[i] for i in [0, length).
//   Scalars (length == 1): (&p.member)[0] gives the single int.
//   PST sub-rows (e.g. pst_mg[0]): int[64] decaying to int*, p.pst_mg[0][i].
#ifndef BASILISK_EVAL_REBUILD
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
    X(KingProtectorNMg,     king_protector_n_mg,   1)             \
    X(KingProtectorNEg,     king_protector_n_eg,   1)             \
    X(KingProtectorBMg,     king_protector_b_mg,   1)             \
    X(KingProtectorBEg,     king_protector_b_eg,   1)             \
    X(QueenInfiltrationMg,  queen_infiltration_mg, 1)             \
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
    X(SafetyTable,          safety_table,         25)               \
    X(KsSafeCheck,          ks_safe_check,         7)               \
    X(KsWeakRing,           ks_weak_ring,          1)               \
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
    X(ProxBase,             prox_base,             1)               \
    X(SpaceMg,              space_mg,              1)               \
    X(SpacePieceMg,         space_piece_mg,        1)               \
    X(SpaceBlockedMg,       space_blocked_mg,      1)               \
    X(TrappedMg,            trapped_mg,            1)               \
    X(TrappedEg,            trapped_eg,            1)               \
    X(Tempo,                tempo,                 1)
#endif // !BASILISK_EVAL_REBUILD

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

// Total flat element count across all groups.
constexpr int EVAL_PARAM_FLAT_SIZE = eval_param_offset(EPG_COUNT);

// Per-trace record populated by Evaluator::evaluate() when TEXEL_TRACE is
// defined. mg[i] / eg[i] hold the signed count contribution of flat param i
// to the middlegame / endgame accumulator (positive = white favoured).
// phase is the computed game phase [0..24]. What the score holds beyond the
// traced terms -- the frozen additions, the endgame step, the damping and the
// lazy checkpoint -- is recorded in g_rest (EvalRestTrace below).
struct EvalTrace {
    int16_t mg[EVAL_PARAM_FLAT_SIZE];
    int16_t eg[EVAL_PARAM_FLAT_SIZE];
    int phase;
};

// Thread-local trace populated by every evaluate() call in TEXEL_TRACE builds.
// Caller must zero this before calling evaluate().
extern thread_local EvalTrace g_trace;

// Reconstruct the linear part of the eval from trace counts and weights.
// Returns (mg_dot * phase + eg_dot * (24 - phase)) / 24.
int reconstruct(const EvalTrace& tr, const EvalParams& w);

// What `rest` is made of, recorded beside the trace so that it can be
// reported by source. The untraced additions to the running sum, by the term
// that made them; the sum the final taper read; that taper; and the score
// apply_endgame returned from it. All in White's view. evaluate() writes it;
// the caller zeroes it with g_trace.
#ifdef BASILISK_EVAL_REBUILD
// The rebuilt evaluation's untraced additions: the terms its knobs enter
// through truncating or squaring arithmetic.
enum EvalRestSource : int {
    REST_IMBALANCE,    // (White's polynomial - Black's) / 16, both halves
    REST_KING_DANGER,  // the danger index's square over 4096 and sixteenth
    REST_SPACE,        // the space count times the weight squared, over 256
    REST_SOURCE_NB
};
#else
enum EvalRestSource : int {
    REST_PASSER_RANK,  // the rank half of the passer king-proximity product
    REST_MATE_DRIVE,   // the bare-material mate drive
    REST_SOURCE_NB
};
#endif
// What apply_endgame did to the tapered score it was given: left it, scaled it
// by num / den (C++ integer division), or replaced it outright with a known
// result. `verdict` is its draw verdict.
enum EvalEndgameKind : int { EG_UNCHANGED, EG_SCALED, EG_OVERRIDE };
struct EvalEndgameStep {
    int kind;
    int num, den;
    bool verdict;
};
#ifdef BASILISK_EVAL_REBUILD
// The rebuilt evaluation's record of how it served its value, in the
// classical form's units until `t`, then centipawns; White's view except
// `served`. The traced build evaluates past both lazy exits: `mg`/`eg` and
// g_trace hold every family, and `lazy_mg`/`lazy_eg` and g_lazy_trace what
// they held at the exit a release build takes (`lazy` 1 or 2), the families
// that build serves from.
struct EvalRestTrace {
    int mg[REST_SOURCE_NB];
    int eg[REST_SOURCE_NB];
    int lazy_mg[REST_SOURCE_NB];
    int lazy_eg[REST_SOURCE_NB];
    int lazy;
    int sum_mg, sum_eg;    // the sum served from, before winnable
    int complexity;        // winnable's complexity, which no slot moves
    int ph;                // the phase, 0 to 128
    int general_sf;        // the general heuristics' factor
    int t;                 // the converted taper at a factor of 64
    int drive;             // the mate drive's tapered addition to t
    int white;             // after the endgame layer
    EvalEndgameStep step;  // the layer on t + drive
    int served;            // side to move, before the rule-50 damping
};
#else
struct EvalRestTrace {
    int mg[REST_SOURCE_NB];
    int eg[REST_SOURCE_NB];
    int sum_mg, sum_eg;
    int tapered;
    int endgame;
    EvalEndgameStep step;  // apply_endgame on `tapered`
    // The lazy checkpoint, which a traced build evaluates past: whether a
    // release build would have served the cheap block's value here, that
    // block's taper, apply_endgame on it, and what that step did. The trace
    // the cheap block wrote up to the checkpoint is g_lazy_trace.
    bool lazy;
    int lazy_tapered;
    int lazy_endgame;
    EvalEndgameStep lazy_step;
};
#endif
extern thread_local EvalRestTrace g_rest;
extern thread_local EvalTrace g_lazy_trace;
// apply_endgame's last step, set by every call.
extern thread_local EvalEndgameStep g_endgame_step;

#endif // TEXEL_TRACE
