#pragma once

// The rebuilt evaluation's registry: every value its score reads, in the
// classical form's own units (a pawn is 124 in the middlegame and 206 in the
// endgame), with the reference's constants as the seed vector. Included by
// eval_params.h alone; the loader, the dumper and the trace read it through
// EVAL_PARAM_LIST.
//
// A slot the score is linear in is a fitted value. A knob enters through
// arithmetic the reference truncates (the imbalance's sixteenth), so the
// score is not linear in it. The structure constants no static fit prices
// (the phase limits, the lazy thresholds, the fixed non-pawn values) are not
// here: they live beside the code that reads them (eval_rb_common.h).

struct EvalParams {

    // ---- Material values [PieceType 0..6] -----------------------------------
    int mg_val[7] = { 0, 139, 852, 927, 1333, 2565, 0 };
    int eg_val[7] = { 0, 234, 829, 877, 1419, 2696, 0 };

    // ---- Piece-square tables: [PieceType - 1][square], White's view --------
    // Squares a1 = 0 to h8 = 63; init_eval_tables mirrors them for Black. The
    // reference's tables are file-symmetric for the pieces; these hold all 64
    // squares, so the seed is a point of the larger space.
    int pst_mg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -27,  -39,  -40,  -40,   -1,  -17,  -24,  -17,
           -58,  -66,  -66,  -61,  -21,  -26,  -52,  -58,
           -19,  -46,  -39,  -21,   -5,   -4,  -41,  -20,
            35,   14,    1,   -6,   20,   37,   14,   77,
            54,  -12,  -11,   42,   57,  -66,    1,   12,
           128,   68,   37,   71,  182,  -23,    9,   53,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        { -141,  -62,  -76,  -44,  -33,  -55,  -88, -159,
           -79,  -92,  -30,  -32,  -24,  -37,  -16,  -55,
           -64,  -29,   -6,  -17,   13,    5,    5,  -43,
             4,   52,   63,   46,   62,   73,   90,   24,
            45,   42,   58,  124,   69,  102,   58,   90,
           -19,   41,   89,   95,  126,  144,   49,   39,
           -45,  -21,   72,  106,  126,  141,   60,  126,
          -305, -280,   -6,    3,   -5,  -81,  -47, -281 },
        // BISHOP
        {   18,   66,  -16,  -23,  -20,  -24,   26,  -68,
            76,  -17,   39,  -26,   -6,   16,   -8,  -30,
            13,   84,   -2,   37,   11,  -23,   18,   11,
             5,    5,   64,   48,   69,   40,   22,   30,
            10,   31,   51,   94,   48,   88,   25,  -10,
            21,   40,   -7,   49,   46,   42,   49,   71,
           -45,  -40,   62,   16,   22,   17,  -85,  -11,
           -62,  -10, -119, -205, -121, -245, -123, -156 },
        // ROOK
        {  -53,  -46,  -33,  -14,   -6,  -24,   -1,  -60,
           -61,  -45,  -25,   -9,    5,   12,   35,  -48,
           -80,  -46,  -25,  -32,   -2,  -10,   37,  -14,
           -60,  -50,  -24,  -33,  -28,  -40,   40,  -55,
           -52,   -6,   -5,   24,   17,   60,   49,   21,
           -26,   10,   42,   53,  107,  104,  110,   13,
           -20,  -18,   22,   48,    4,   66,   70,   24,
           -26,   22,  -15,    3,   -4,  -50,   37,   93 },
        // QUEEN
        {   17,  -18,  -10,    0,    1,   -8,  -11,  -13,
           -19,   -3,   -6,    0,    3,   16,   36,   -7,
           -10,    8,    0,  -22,  -21,   25,   30,   26,
            -3,  -23,    2,   -8,   -2,    9,   28,   12,
           -14,   13,   -6,  -12,   -1,   15,  -16,    2,
             0,   21,   33,    8,  -36,    6,  -41,   10,
           -33,  -14,   10,    7,  -55,  -18,   18,   85,
           -89,   27,  -30,  -13, -113,    3,  133,  102 },
        // KING
        {  258,  322,  251,  208,  156,  167,  287,  262,
           309,  279,  188,  224,  197,  198,  280,  257,
           266,  318,  279,  230,  198,  213,  268,  223,
           242,  259,  126,   59,   67,   43,  129,   36,
           143,  171,   54,   13,   43,   92,   85,   70,
           153,  212,   60,   -4,   56,   48,   98,  157,
           159,  135,  114,  112,   36,   74,  132,  108,
            83,  130,   52,   -6,    3,   27,   90,   56 }
    };
    int pst_eg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -24,  -28,  -26,   -6,   -3,  -15,  -30,  -49,
           -24,  -31,  -31,  -22,  -20,  -15,  -29,  -40,
           -18,  -23,  -35,  -29,  -26,  -25,  -25,  -36,
            10,  -18,  -26,  -27,  -23,  -25,  -20,  -13,
            57,   39,   32,   22,   46,   48,   53,   41,
            38,   43,   61,   72,   79,   62,   51,   30,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -67,  -31,  -26,  -18,  -27,  -50,  -48,  -65,
           -27,    2,  -21,  -15,  -22,  -18,  -34,  -45,
           -25,   -7,   -6,   11,   19,  -17,  -25,  -31,
             6,   10,   24,   29,   45,   21,   21,    3,
            18,   16,   34,   56,   49,   41,   23,    0,
            -9,   19,   28,   39,   35,   41,   25,   -9,
           -29,    8,   20,   56,   34,    1,    4,   -7,
           -55,   18,   12,   21,   28,   13,   -9,  -61 },
        // BISHOP
        {  -41,  -26,  -14,  -14,  -15,  -22,  -45,  -67,
            -1,  -43,  -20,    3,  -14,  -18,  -62,  -63,
             3,    2,  -14,    5,   17,  -29,  -21,  -20,
            -8,    9,   14,   14,    9,    8,    5,  -34,
             9,   12,    7,   28,   31,   14,   19,  -13,
             5,   17,    1,   23,   34,   28,   27,   18,
             9,   -5,   25,   12,   37,   28,   -2,  -17,
             0,   18,   40,   32,   15,   24,   23,    5 },
        // ROOK
        {  -33,  -37,  -26,  -40,  -45,  -34,  -49,  -43,
           -26,  -32,  -23,  -36,  -40,  -40,  -63,  -65,
           -17,  -17,  -17,  -16,  -25,  -21,  -32,  -56,
             7,   11,   11,    2,    7,   16,   -7,   -9,
            26,   20,   28,   23,   19,   14,    5,   -5,
            18,   19,   13,   15,    9,    8,    1,   11,
            17,   25,   25,   17,   23,   35,   40,   21,
            17,   37,   43,   47,   51,   57,   53,   32 },
        // QUEEN
        { -132,  -85,  -76,  -65,  -89, -134, -194, -154,
           -50,  -60,  -29,  -29,  -25,  -68,  -99,  -97,
           -46,   -6,   -4,   28,   35,    2,  -28,  -47,
            13,   35,   31,   44,   63,   34,   30,   17,
             2,   41,   47,   66,   62,   66,   88,   52,
           -35,  -10,   28,   52,  100,   74,   97,    9,
           -22,   -5,   23,   55,   75,   43,   25,    1,
            12,   -7,   28,   47,   78,   80,   15,    6 },
        // KING
        {   14,   39,   70,   59,   58,   72,   47,   14,
            81,   90,  108,  106,  107,  102,   91,   79,
            90,  115,  138,  143,  148,  140,  125,  104,
           109,  155,  177,  185,  185,  182,  165,  138,
           140,  174,  212,  214,  224,  216,  194,  147,
           115,  168,  186,  201,  220,  218,  181,  137,
           -30,   91,  148,  133,  176,  171,  153,   57,
          -158,   19,   32,   69,   82,   99,   92,  -65 }
    };
    // ---- Material imbalance (knobs) ----------------------------------------
    // Piece index 0 = the bishop pair, then pawn, knight, bishop, rook, queen.
    // imb_ours[i*(i+1)/2 + j], j <= i: our pieces i against our pieces j;
    // imb_theirs[i*(i-1)/2 + j], j < i: our pieces i against theirs j. Each
    // side's polynomial is summed, then (White's - Black's) / 16, truncated.
    int imb_ours[21] = { 1401, 40, 65, -155, 237, -101, -75, 86, 4, -37, 11, -56, 158, 216, -181, 73, 269, 118, 115, 878, 69 };
    int imb_theirs[15] = { 36, 27, 59, 171, 56, -15, -366, 66, 90, -15, 247, 154, -6, 128, 99 };

    // ---- Pawn structure (ticket 2) ------------------------------------------
    // Penalties are subtracted where they apply.
    int doubled_mg = -16;
    int doubled_eg = 57;
    int isolated_mg = 7;
    int isolated_eg = 18;
    int backward_mg = 14;
    int backward_eg = 16;
    int weak_lever_mg = -22;
    int weak_lever_eg = 68;
    int weak_unopposed_mg = 12;
    int weak_unopposed_eg = 12;
    // A blocked pawn on its fifth or sixth rank.
    int blocked_pawn_mg[2] = { -43, -5 };
    int blocked_pawn_eg[2] = { -10, -54 };
    // A connected pawn by rank, phalanx, opposition and supporters
    // (rb::connected_index). Seeded with the reference's products: the
    // middlegame value Connected[r] * (2 + phalanx - opposed) + 21 * support,
    // the endgame value that times (r - 2) / 4, truncated.
    int connected_mg[72] = { 14, 35, 56, 7, 28, 49, 22, 42, 63, 16, 35, 56, 16, 76, 146, 8, 61, 126, 37, 117, 9, 26, 84, 210, 24, 81, 132, 12, 46, 73, 49, 108, -146, 30, 76, -7, 58, 108, 197, 29, 49, 66, 63, 190, 129, 49, 82, 100, 96, 190, 125, 48, 70, -8, 233, -67, 186, -82, 117, 138, 172, 173, 214, 86, 107, 128, 53, 279, 300, 172, 193, 214 };
    int connected_eg[72] = { -3, -8, -14, -1, -7, -12, -9, -10, -15, -3, -8, -14, 0, 9, 9, 0, 8, 16, -4, 14, -59, -4, 22, -70, 6, 16, 13, 3, 7, -12, 19, 49, -59, -2, 15, 12, 29, 56, 68, 14, 15, 22, 36, 88, 64, 11, 36, 50, 72, 124, 143, 36, 51, 106, 165, 173, 139, -127, 87, 103, 172, 295, 214, 86, 107, 128, 239, 279, 300, 172, 193, 214 };

    // ---- The king's pawn shelter and storm (ticket 2; the king family) ------
    // [distance from the edge * 7 + relative rank]; rank 0 is a file without
    // a pawn, or with it behind the king.
    int shelter_strength[28] = { -41, 8, 40, 3, -17, 52, 100, -78, 81, 32, -33, -46, 62, 172, -48, 65, -16, -19, -46, 28, 113, -86, -9, -60, -81, -43, -123, -139 };
    int unblocked_storm[28] = { 120, -204, -69, 63, 73, 79, 68, 58, -191, 9, 39, 25, -7, 13, 8, -63, 96, 25, -10, 2, 12, 19, 26, 81, 80, 22, -2, -12 };
    // A storming pawn blocked by one of ours, by its relative rank.
    int blocked_storm_mg[7] = { 0, 0, 120, 9, -7, 21, 43 };
    int blocked_storm_eg[7] = { 0, 0, 56, 34, 22, 19, 6 };
    // Per step from the king to its nearest pawn.
    int king_pawn_distance_eg = 8;

    // ---- Attack maps, pieces and mobility (ticket 3) -------------------------
    // King danger's attacker weights by PieceType (knobs; the king family
    // reads their sum).
    int king_attack_weight[7] = { 0, 0, 51, 23, 10, 42, 0 };
    // Mobility by the count of attacked squares in the mobility area.
    int mob_n_mg[9] = { -48, -80, -37, -14, 2, 18, 29, 44, 53 };
    int mob_n_eg[9] = { -54, -86, -34, -11, 4, 17, 21, 23, 15 };
    int mob_b_mg[14] = { -53, -17, 16, 31, 45, 55, 61, 64, 71, 75, 89, 95, 74, 56 };
    int mob_b_eg[14] = { -20, -35, -2, 27, 39, 53, 61, 68, 72, 74, 69, 64, 69, 57 };
    int mob_r_mg[15] = { -43, -45, -3, 3, -2, 14, 21, 28, 40, 42, 41, 49, 59, 53, 80 };
    int mob_r_eg[15] = { -128, 3, 54, 75, 99, 101, 111, 122, 130, 137, 144, 149, 154, 160, 153 };
    int mob_q_mg[28] = { 133, -5, -49, -8, 26, 20, 30, 37, 46, 54, 59, 66, 66, 69, 75, 69, 75, 78, 76, 79, 66, 89, 70, 70, 95, 161, 115, 37 };
    int mob_q_eg[28] = { 122, 111, 81, 3, -50, 35, 73, 96, 102, 109, 126, 126, 134, 139, 142, 151, 150, 146, 152, 155, 154, 154, 146, 136, 133, 86, 88, 48 };
    // The piece terms; penalties are subtracted where they apply.
    int rook_on_king_ring_mg = 19;
    int rook_on_king_ring_eg = -18;
    int bishop_on_king_ring_mg = 41;
    int bishop_on_king_ring_eg = 13;
    int outpost_mg[2] = { 64, 64 };  // knight, bishop
    int outpost_eg[2] = { 24, 7 };
    int bad_outpost_mg = -5;
    int bad_outpost_eg = 23;
    int reachable_outpost_mg = 58;
    int reachable_outpost_eg = 18;
    int minor_behind_pawn_mg = 19;
    int minor_behind_pawn_eg = 17;
    int king_protector_mg[2] = { 11, 15 };  // knight, bishop; per step from our king
    int king_protector_eg[2] = { 8, 6 };
    int bishop_pawns_mg = 3;
    int bishop_pawns_eg = 6;
    int bishop_xray_pawns_mg = 12;
    int bishop_xray_pawns_eg = 1;
    int long_diagonal_bishop_mg = 68;
    int long_diagonal_bishop_eg = 29;
    int rook_on_queen_file_mg = 12;
    int rook_on_queen_file_eg = 1;
    int rook_on_file_mg[2] = { 32, 65 };  // semi-open, open
    int rook_on_file_eg[2] = { 2, 18 };
    int trapped_rook_mg = 37;
    int trapped_rook_eg = 23;
    int weak_queen_mg = 56;
    int weak_queen_eg = 14;
    int queen_infiltration_mg = 12;
    int queen_infiltration_eg = 19;

    // ---- King danger (ticket 4; knobs: the index is squared) ---------------
    // A safe check by PieceType * 2 + whether there are several.
    int safe_check[14] = { 0, 0, 0, 0, 574, 1027, 669, 436, 733, 948, 541, 856, 0, 0 };
    int kd_weak_ring = 104;        // per weak square in the king ring
    int kd_unsafe_checks = 163;    // per square an unsafe check could come from
    int kd_blockers = 0;          // per piece shielding our king from a slider
    int kd_king_attacks = 100;      // per attack on a square next to our king
    int kd_flank_attack = 4;       // times the flank attack's square, over 8
    int kd_no_queen = 1097;         // subtracted when they have no queen
    int kd_knight_defender = 225;  // subtracted when our knight defends next to our king
    int kd_shelter = 11;            // times the shelter's middlegame value, over 8, subtracted
    int kd_flank_defence = 19;      // per square we defend in our flank, subtracted
    int kd_offset = 99;
    // ---- The king's flank (ticket 4; linear) ----------------------------------
    int pawnless_flank_mg = -93;
    int pawnless_flank_eg = 116;
    int flank_attacks_mg = 8;      // per attack on our flank
    int flank_attacks_eg = -5;

    // ---- Threats (ticket 5) ---------------------------------------------------
    // By the attacked piece's PieceType: a minor attacking a weak or
    // defended piece, a rook a weak one.
    int threat_by_minor_mg[7] = { 0, 9, 70, 111, 191, 160, 507 };
    int threat_by_minor_eg[7] = { 0, 38, 63, 62, 70, 31, 437 };
    int threat_by_rook_mg[7] = { 0, 4, 66, 66, 0, 85, 512 };
    int threat_by_rook_eg[7] = { 0, 38, 46, 58, 19, 45, 622 };
    int threat_by_king_mg = 68;
    int threat_by_king_eg = 41;
    int hanging_mg = 20;
    int hanging_eg = 38;
    int weak_queen_protection_mg = 20;
    int weak_queen_protection_eg = 19;
    int restricted_piece_mg = 12;
    int restricted_piece_eg = 3;
    int threat_by_safe_pawn_mg = 179;
    int threat_by_safe_pawn_eg = 77;
    int threat_by_pawn_push_mg = 42;
    int threat_by_pawn_push_eg = 33;
    int knight_on_queen_mg = 27;
    int knight_on_queen_eg = -13;
    int slider_on_queen_mg = 43;
    int slider_on_queen_eg = 5;

    // ---- Passed pawns (ticket 5) ----------------------------------------------
    int passed_rank_mg[8] = { 0, 0, 7, 31, 58, 163, 276, 0 };
    int passed_rank_eg[8] = { 0, 49, 50, 96, 134, 182, 260, 0 };
    int passed_file_mg = 10;  // per file from the edge, subtracted
    int passed_file_eg = 12;
    // By relative rank 3 to 6 and the king's distance to the block square
    // (or to the square after it), capped at 5 (rb::passer_distance_index).
    // Seeded with the reference's truncated products, the rank weight
    // w = 5r - 13 times (d * 19 / 4), 2d and d.
    int passed_their_king_eg[24] = { 25, 10, 2, 3, 31, 68, 27, 22, 33, 78, 141, 184, -16, 18, 71, 170, 276, 311, -35, 15, 82, 301, 382, 427 };
    int passed_our_king_eg[24] = { 25, -4, -2, 7, 17, 16, 13, -4, -1, 36, 69, 97, -16, -54, 25, 87, 147, 170, -278, 37, 20, 204, 264, 263 };
    int passed_our_king_next_eg[24] = { -44, -50, 4, 22, 40, 57, -43, -78, 33, 65, 65, 61, 91, -80, -23, 71, 70, 51, 0, 17, 34, 51, 68, 85 };
    // A passer with a free block square, by relative rank 3 to 6 and the
    // path's case * 2 + whether we hold the block square
    // (rb::passer_path_index). Seeded with k * w, k = 35, 20, 9, 0, plus 5
    // when held.
    int passed_path_mg[32] = { -219, -168, -200, -142, -13, 18, -19, 10, -280, -197, -90, -35, -20, 37, -16, 36, -106, 61, 226, 322, 0, 160, 1, 73, 358, 518, 322, 537, 153, 238, 181, 183 };
    int passed_path_eg[32] = { 88, 53, 65, 49, 16, 30, 0, 9, 215, 170, 95, 118, 50, 87, 9, 38, 513, 361, 147, 234, 89, 140, 9, 78, 812, 729, 602, 905, 153, 238, 90, 337 };

    // ---- Space (ticket 5; a knob) ---------------------------------------------
    // The space count times the weight squared, times this, over 256.
    int space_weight = 15;

    // ---- Tempo, in units; converted with the value --------------------------
    int tempo = 36;

    // ---- Basilisk's own forms (src/eval.h, OwnForm) -------------------------
    // Read only when their bit is compiled in. Each value is added where its
    // form applies, so a penalty is negative. A form that replaces the
    // classical one is seeded with the retired evaluation's value converted
    // to these units (cp * 1024 / 620, rounded); one added beside it is
    // seeded at 0, so the bit alone at its seed serves the same value.
    int own_tempo_mg = 45;
    int own_space_mg = 0;
    int own_space_piece_mg = 2;
    int own_space_blocked_mg = -2;
    int own_minor_king_ring_mg = 5;
    int own_minor_king_ring_eg = 2;
    int own_rook_king_ring_mg = -66;
    int own_rook_king_ring_eg = -10;
    int own_bad_bishop_mg = -3;
    int own_bad_bishop_eg = -20;
    int own_knight_outpost_mg = 31;
    int own_knight_outpost_eg = 33;
    int own_reachable_outpost_mg = 17;
    int own_reachable_outpost_eg = 21;
    int own_minor_behind_pawn_mg = 17;
    int own_minor_behind_pawn_eg = 2;
    int own_rook_queen_file_mg = 30;
    int own_rook_queen_file_eg = -3;
    int own_queen_infiltration_mg = -74;
    int own_queen_infiltration_eg = 307;
    int own_threat_minor_mg = 144;
    int own_threat_minor_eg = 94;
    int own_threat_rook_mg = 144;
    int own_threat_rook_eg = 94;
    int own_threat_queen_mg = 144;
    int own_threat_queen_eg = 94;
    int own_doubled_mg = -2;
    int own_doubled_eg = -23;
    int own_backward_mg = 0;
    int own_backward_eg = -10;
    int own_pawn_majority_mg = 0;
    int own_pawn_majority_eg = 0;
    int own_pawn_islands_mg = 0;
    int own_pawn_islands_eg = 0;
    int own_connected_rooks_mg = 0;
    int own_connected_rooks_eg = 0;
    int own_rook_closed_mg = 0;
    int own_rook_closed_eg = 0;
    int own_trapped_bishop_mg = 0;
    int own_trapped_bishop_eg = 0;
    int own_rook_behind_passer_mg = 0;
    int own_rook_behind_passer_eg = 0;
    int own_enemy_rook_passer_mg = 0;
    int own_enemy_rook_passer_eg = 0;
};

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
    X(ImbOurs,              imb_ours,             21)               \
    X(ImbTheirs,            imb_theirs,           15)               \
    X(DoubledMg,            doubled_mg,            1)               \
    X(DoubledEg,            doubled_eg,            1)               \
    X(IsolatedMg,           isolated_mg,           1)               \
    X(IsolatedEg,           isolated_eg,           1)               \
    X(BackwardMg,           backward_mg,           1)               \
    X(BackwardEg,           backward_eg,           1)               \
    X(WeakLeverMg,          weak_lever_mg,         1)               \
    X(WeakLeverEg,          weak_lever_eg,         1)               \
    X(WeakUnopposedMg,      weak_unopposed_mg,     1)               \
    X(WeakUnopposedEg,      weak_unopposed_eg,     1)               \
    X(BlockedPawnMg,        blocked_pawn_mg,       2)               \
    X(BlockedPawnEg,        blocked_pawn_eg,       2)               \
    X(ConnectedMg,          connected_mg,         72)               \
    X(ConnectedEg,          connected_eg,         72)               \
    X(ShelterStrength,      shelter_strength,     28)               \
    X(UnblockedStorm,       unblocked_storm,      28)               \
    X(BlockedStormMg,       blocked_storm_mg,      7)               \
    X(BlockedStormEg,       blocked_storm_eg,      7)               \
    X(KingPawnDistanceEg,   king_pawn_distance_eg, 1)               \
    X(KingAttackWeight,     king_attack_weight,    7)               \
    X(MobNMg,               mob_n_mg,              9)               \
    X(MobNEg,               mob_n_eg,              9)               \
    X(MobBMg,               mob_b_mg,             14)               \
    X(MobBEg,               mob_b_eg,             14)               \
    X(MobRMg,               mob_r_mg,             15)               \
    X(MobREg,               mob_r_eg,             15)               \
    X(MobQMg,               mob_q_mg,             28)               \
    X(MobQEg,               mob_q_eg,             28)               \
    X(RookOnKingRingMg,     rook_on_king_ring_mg,  1)               \
    X(RookOnKingRingEg,     rook_on_king_ring_eg,  1)               \
    X(BishopOnKingRingMg,   bishop_on_king_ring_mg, 1)              \
    X(BishopOnKingRingEg,   bishop_on_king_ring_eg, 1)              \
    X(OutpostMg,            outpost_mg,            2)               \
    X(OutpostEg,            outpost_eg,            2)               \
    X(BadOutpostMg,         bad_outpost_mg,        1)               \
    X(BadOutpostEg,         bad_outpost_eg,        1)               \
    X(ReachableOutpostMg,   reachable_outpost_mg,  1)               \
    X(ReachableOutpostEg,   reachable_outpost_eg,  1)               \
    X(MinorBehindPawnMg,    minor_behind_pawn_mg,  1)               \
    X(MinorBehindPawnEg,    minor_behind_pawn_eg,  1)               \
    X(KingProtectorMg,      king_protector_mg,     2)               \
    X(KingProtectorEg,      king_protector_eg,     2)               \
    X(BishopPawnsMg,        bishop_pawns_mg,       1)               \
    X(BishopPawnsEg,        bishop_pawns_eg,       1)               \
    X(BishopXrayPawnsMg,    bishop_xray_pawns_mg,  1)               \
    X(BishopXrayPawnsEg,    bishop_xray_pawns_eg,  1)               \
    X(LongDiagonalBishopMg, long_diagonal_bishop_mg, 1)             \
    X(LongDiagonalBishopEg, long_diagonal_bishop_eg, 1)             \
    X(RookOnQueenFileMg,    rook_on_queen_file_mg, 1)               \
    X(RookOnQueenFileEg,    rook_on_queen_file_eg, 1)               \
    X(RookOnFileMg,         rook_on_file_mg,       2)               \
    X(RookOnFileEg,         rook_on_file_eg,       2)               \
    X(TrappedRookMg,        trapped_rook_mg,       1)               \
    X(TrappedRookEg,        trapped_rook_eg,       1)               \
    X(WeakQueenMg,          weak_queen_mg,         1)               \
    X(WeakQueenEg,          weak_queen_eg,         1)               \
    X(QueenInfiltrationMg,  queen_infiltration_mg, 1)               \
    X(QueenInfiltrationEg,  queen_infiltration_eg, 1)               \
    X(SafeCheck,            safe_check,           14)               \
    X(KdWeakRing,           kd_weak_ring,          1)               \
    X(KdUnsafeChecks,       kd_unsafe_checks,      1)               \
    X(KdBlockers,           kd_blockers,           1)               \
    X(KdKingAttacks,        kd_king_attacks,       1)               \
    X(KdFlankAttack,        kd_flank_attack,       1)               \
    X(KdNoQueen,            kd_no_queen,           1)               \
    X(KdKnightDefender,     kd_knight_defender,    1)               \
    X(KdShelter,            kd_shelter,            1)               \
    X(KdFlankDefence,       kd_flank_defence,      1)               \
    X(KdOffset,             kd_offset,             1)               \
    X(PawnlessFlankMg,      pawnless_flank_mg,     1)               \
    X(PawnlessFlankEg,      pawnless_flank_eg,     1)               \
    X(FlankAttacksMg,       flank_attacks_mg,      1)               \
    X(FlankAttacksEg,       flank_attacks_eg,      1)               \
    X(ThreatByMinorMg,      threat_by_minor_mg,    7)               \
    X(ThreatByMinorEg,      threat_by_minor_eg,    7)               \
    X(ThreatByRookMg,       threat_by_rook_mg,     7)               \
    X(ThreatByRookEg,       threat_by_rook_eg,     7)               \
    X(ThreatByKingMg,       threat_by_king_mg,     1)               \
    X(ThreatByKingEg,       threat_by_king_eg,     1)               \
    X(HangingMg,            hanging_mg,            1)               \
    X(HangingEg,            hanging_eg,            1)               \
    X(WeakQueenProtectionMg, weak_queen_protection_mg, 1)           \
    X(WeakQueenProtectionEg, weak_queen_protection_eg, 1)           \
    X(RestrictedPieceMg,    restricted_piece_mg,   1)               \
    X(RestrictedPieceEg,    restricted_piece_eg,   1)               \
    X(ThreatBySafePawnMg,   threat_by_safe_pawn_mg, 1)              \
    X(ThreatBySafePawnEg,   threat_by_safe_pawn_eg, 1)              \
    X(ThreatByPawnPushMg,   threat_by_pawn_push_mg, 1)              \
    X(ThreatByPawnPushEg,   threat_by_pawn_push_eg, 1)              \
    X(KnightOnQueenMg,      knight_on_queen_mg,    1)               \
    X(KnightOnQueenEg,      knight_on_queen_eg,    1)               \
    X(SliderOnQueenMg,      slider_on_queen_mg,    1)               \
    X(SliderOnQueenEg,      slider_on_queen_eg,    1)               \
    X(PassedRankMg,         passed_rank_mg,        8)               \
    X(PassedRankEg,         passed_rank_eg,        8)               \
    X(PassedFileMg,         passed_file_mg,        1)               \
    X(PassedFileEg,         passed_file_eg,        1)               \
    X(PassedTheirKingEg,    passed_their_king_eg, 24)               \
    X(PassedOurKingEg,      passed_our_king_eg,   24)               \
    X(PassedOurKingNextEg,  passed_our_king_next_eg, 24)            \
    X(PassedPathMg,         passed_path_mg,       32)               \
    X(PassedPathEg,         passed_path_eg,       32)               \
    X(SpaceWeight,          space_weight,          1)               \
    X(Tempo,                tempo,                 1)               \
    X(OwnTempoMg,           own_tempo_mg,          1)               \
    X(OwnSpaceMg,           own_space_mg,          1)               \
    X(OwnSpacePieceMg,      own_space_piece_mg,    1)               \
    X(OwnSpaceBlockedMg,    own_space_blocked_mg,  1)               \
    X(OwnMinorKingRingMg,   own_minor_king_ring_mg, 1)              \
    X(OwnMinorKingRingEg,   own_minor_king_ring_eg, 1)              \
    X(OwnRookKingRingMg,    own_rook_king_ring_mg, 1)               \
    X(OwnRookKingRingEg,    own_rook_king_ring_eg, 1)               \
    X(OwnBadBishopMg,       own_bad_bishop_mg,     1)               \
    X(OwnBadBishopEg,       own_bad_bishop_eg,     1)               \
    X(OwnKnightOutpostMg,   own_knight_outpost_mg, 1)               \
    X(OwnKnightOutpostEg,   own_knight_outpost_eg, 1)               \
    X(OwnReachableOutpostMg, own_reachable_outpost_mg, 1)           \
    X(OwnReachableOutpostEg, own_reachable_outpost_eg, 1)           \
    X(OwnMinorBehindPawnMg, own_minor_behind_pawn_mg, 1)            \
    X(OwnMinorBehindPawnEg, own_minor_behind_pawn_eg, 1)            \
    X(OwnRookQueenFileMg,   own_rook_queen_file_mg, 1)              \
    X(OwnRookQueenFileEg,   own_rook_queen_file_eg, 1)              \
    X(OwnQueenInfiltrationMg, own_queen_infiltration_mg, 1)         \
    X(OwnQueenInfiltrationEg, own_queen_infiltration_eg, 1)         \
    X(OwnThreatMinorMg,     own_threat_minor_mg,   1)               \
    X(OwnThreatMinorEg,     own_threat_minor_eg,   1)               \
    X(OwnThreatRookMg,      own_threat_rook_mg,    1)               \
    X(OwnThreatRookEg,      own_threat_rook_eg,    1)               \
    X(OwnThreatQueenMg,     own_threat_queen_mg,   1)               \
    X(OwnThreatQueenEg,     own_threat_queen_eg,   1)               \
    X(OwnDoubledMg,         own_doubled_mg,        1)               \
    X(OwnDoubledEg,         own_doubled_eg,        1)               \
    X(OwnBackwardMg,        own_backward_mg,       1)               \
    X(OwnBackwardEg,        own_backward_eg,       1)               \
    X(OwnPawnMajorityMg,    own_pawn_majority_mg,  1)               \
    X(OwnPawnMajorityEg,    own_pawn_majority_eg,  1)               \
    X(OwnPawnIslandsMg,     own_pawn_islands_mg,   1)               \
    X(OwnPawnIslandsEg,     own_pawn_islands_eg,   1)               \
    X(OwnConnectedRooksMg,  own_connected_rooks_mg, 1)              \
    X(OwnConnectedRooksEg,  own_connected_rooks_eg, 1)              \
    X(OwnRookClosedMg,      own_rook_closed_mg,    1)               \
    X(OwnRookClosedEg,      own_rook_closed_eg,    1)               \
    X(OwnTrappedBishopMg,   own_trapped_bishop_mg, 1)               \
    X(OwnTrappedBishopEg,   own_trapped_bishop_eg, 1)               \
    X(OwnRookBehindPasserMg, own_rook_behind_passer_mg, 1)          \
    X(OwnRookBehindPasserEg, own_rook_behind_passer_eg, 1)          \
    X(OwnEnemyRookPasserMg, own_enemy_rook_passer_mg, 1)            \
    X(OwnEnemyRookPasserEg, own_enemy_rook_passer_eg, 1)
