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
    int mg_val[7] = { 0, 130, 851, 923, 1333, 2564, 0 };
    int eg_val[7] = { 0, 228, 832, 880, 1424, 2692, 0 };

    // ---- Piece-square tables: [PieceType - 1][square], White's view --------
    // Squares a1 = 0 to h8 = 63; init_eval_tables mirrors them for Black. The
    // reference's tables are file-symmetric for the pieces; these hold all 64
    // squares, so the seed is a point of the larger space.
    int pst_mg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -27,  -27,  -42,  -38,   -1,  -21,   -5,    0,
           -62,  -71,  -68,  -74,  -30,  -47,  -56,  -62,
           -29,  -46,  -42,  -33,  -20,   -7,  -29,  -18,
            38,    0,   -8,  -15,    0,   34,   13,   70,
            42,   23,  -14,   28,   45,  -48,   26,   20,
           113,   65,   65,  100,  225,  -31,   10,   47,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        { -101,  -59,  -95,  -20,  -29,  -41,  -86, -130,
           -83,  -69,  -45,  -32,  -21,  -31,  -15,  -30,
           -39,  -16,  -10,  -14,   20,    2,   10,  -42,
            17,   58,   61,   36,   61,   63,   53,   22,
            31,   47,   59,  130,   69,  117,   52,   90,
             1,   -2,  102,   84,  127,  147,   52,   20,
           -16,   -5,   81,   87,   98,  128,   56,  165,
          -314, -292,  -14,  -11,  -18,  -88,  -46, -300 },
        // BISHOP
        {  -13,   66,   -7,  -22,   15,  -10,   25,  -56,
            76,  -24,   35,  -24,   -3,   24,  -10,  -27,
            17,   89,  -17,   30,   16,  -25,   38,   10,
            38,   32,   73,   38,   70,   33,   16,   26,
             9,   44,   47,   89,   67,   63,   18,  -14,
            31,   28,  -14,   66,   63,   63,   38,   51,
           -27,  -47,   45,   36,   -7,   35, -100,  -36,
           -51,  -12, -141, -211, -109, -235, -131, -164 },
        // ROOK
        {  -52,  -42,  -39,  -21,  -10,  -23,    5,  -63,
           -74,  -53,  -29,  -13,  -17,    2,   30,  -34,
           -62,  -31,  -47,  -30,  -16,   -6,   26,  -12,
           -90,  -43,  -41,  -30,  -12,  -20,   49,  -45,
           -56,   -5,    8,   32,   49,   53,   44,   35,
           -35,   17,   23,   28,  119,   97,  120,   40,
           -34,  -34,    5,   48,    1,  103,   60,   41,
             1,   16,  -12,   16,   12,  -76,   14,  118 },
        // QUEEN
        {    8,  -19,  -17,  -18,   -4,   25,   -1,  -19,
           -26,  -10,   -9,    5,    9,    7,   38,  -16,
             3,    4,   10,  -12,  -10,   43,   38,   25,
            -6,   13,    8,   -4,   14,   14,    7,   -2,
           -23,   26,   14,    0,   14,    5,  -26,    5,
            -6,    2,   11,   38,  -68,    4,  -74,    7,
           -62,  -50,   21,    2,  -43,   -2,   17,   95,
           -96,   19,   -9,    7, -105,  -15,  131,   82 },
        // KING
        {  269,  299,  264,  232,  181,  193,  297,  272,
           270,  246,  234,  233,  212,  208,  283,  262,
           270,  304,  303,  237,  205,  185,  236,  207,
           236,  264,  126,   47,   58,   31,  115,   39,
           142,  169,   60,   19,   49,   91,   81,   72,
           150,  208,   58,   -5,   57,   49,   90,  155,
           160,  129,  111,  112,   33,   72,  132,  108,
            83,  130,   53,   -9,    4,   27,   91,   57 }
    };
    int pst_eg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -22,  -24,  -30,  -39,   -2,  -16,  -34,  -51,
           -22,  -25,  -31,  -30,  -23,  -19,  -34,  -42,
            -9,  -16,  -34,  -27,  -29,  -25,  -25,  -36,
            15,   -5,  -25,  -25,  -23,  -22,  -15,  -14,
            70,   48,   28,   26,   58,   56,   56,   42,
            28,   40,   56,   64,   77,   56,   39,   15,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -33,  -49,  -26,   -3,  -28,  -43,  -52,  -45,
           -27,   -6,   -9,  -11,  -23,  -23,  -25,  -34,
             2,    1,   -4,   13,   14,  -12,  -18,  -19,
            15,    4,   19,   32,   40,   25,   13,    4,
            24,   16,   23,   49,   36,   31,   23,   -7,
           -23,   19,   33,   36,   36,   37,   20,    1,
           -17,    7,   12,   70,   36,    8,    8,  -26,
           -71,   11,    1,   12,   17,  -21,  -41,  -60 },
        // BISHOP
        {  -37,  -24,  -15,  -16,   -9,  -13,  -45,  -71,
             4,  -39,  -12,    1,   -6,   -5,  -57,  -70,
            -1,   17,  -16,   10,   18,  -26,  -20,  -18,
            -3,    3,    9,   13,   10,    9,    4,  -25,
             2,    8,    3,   29,   24,    9,   19,   -4,
            -1,   19,   -5,   20,   36,   15,   33,    7,
            21,   -7,    8,   25,   29,   32,   -4,  -34,
             3,  -13,   33,   32,   20,   21,   34,    5 },
        // ROOK
        {  -21,  -22,  -18,  -27,  -41,  -27,  -49,  -41,
           -28,  -25,  -18,  -28,  -38,  -35,  -56,  -72,
           -16,  -12,   -7,  -26,  -31,  -25,  -35,  -53,
            18,   10,   17,   12,    8,    7,    0,  -23,
            20,   27,   18,   16,   12,   11,   14,   -3,
            16,   21,   16,   14,    5,    8,    9,    3,
            24,   27,   31,   25,   16,   37,   37,   27,
            19,   30,   47,   41,   37,   55,   37,   34 },
        // QUEEN
        { -123,  -81,  -82,  -46,  -83, -163, -185, -169,
           -32,  -65,  -17,  -22,  -17,  -53, -111,  -80,
           -66,  -18,  -12,   28,   32,    0,  -26,  -46,
            20,    7,   37,   46,   59,   26,   47,   27,
             0,   25,   15,   66,   65,   57,  117,   46,
           -37,    1,   36,   25,  104,   64,   85,   -2,
            -9,  -23,   30,   54,   87,   57,   44,  -14,
            16,   -9,   42,   37,  109,   66,   16,   -9 },
        // KING
        {   26,   48,   86,   66,   68,   74,   44,   14,
            81,   98,  105,  108,  111,  101,   91,   73,
            95,  110,  141,  149,  145,  130,  119,  103,
           118,  143,  177,  183,  184,  174,  154,  124,
           129,  170,  209,  211,  221,  208,  188,  138,
           105,  166,  197,  213,  218,  213,  192,  135,
           -37,   86,  157,  158,  174,  155,  152,   86,
          -173,   -5,   17,   51,   95,  124,   96,  -39 }
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
    int doubled_mg = -6;
    int doubled_eg = 54;
    int isolated_mg = 8;
    int isolated_eg = 15;
    int backward_mg = 11;
    int backward_eg = 16;
    int weak_lever_mg = -32;
    int weak_lever_eg = 71;
    int weak_unopposed_mg = 11;
    int weak_unopposed_eg = 10;
    // A blocked pawn on its fifth or sixth rank.
    int blocked_pawn_mg[2] = { -46, -13 };
    int blocked_pawn_eg[2] = { -12, -54 };
    // A connected pawn by rank, phalanx, opposition and supporters
    // (rb::connected_index). Seeded with the reference's products: the
    // middlegame value Connected[r] * (2 + phalanx - opposed) + 21 * support,
    // the endgame value that times (r - 2) / 4, truncated.
    int connected_mg[72] = { 14, 35, 56, 7, 28, 49, 12, 42, 63, 13, 35, 56, 16, 72, 157, 8, 59, 139, 36, 117, 3, 25, 98, 199, 24, 80, 106, 12, 47, 62, 45, 113, -146, 25, 73, 16, 58, 108, 176, 29, 43, 71, 73, 186, 129, 39, 96, 100, 96, 216, 125, 48, 71, -24, 237, -63, 186, -86, 117, 138, 172, 185, 214, 86, 107, 128, 56, 279, 300, 172, 193, 214 };
    int connected_eg[72] = { -3, -8, -14, -1, -7, -12, -7, -10, -15, -5, -8, -14, 0, 12, 13, 0, 13, 13, 0, 16, -55, 0, 20, -92, 6, 20, 23, 3, 6, 6, 15, 44, -59, 0, 19, 39, 29, 54, 89, 14, 20, 16, 27, 76, 64, 27, 24, 50, 72, 111, 133, 36, 60, 98, 142, 169, 139, -131, 87, 103, 172, 303, 214, 86, 107, 128, 230, 279, 300, 172, 193, 214 };

    // ---- The king's pawn shelter and storm (ticket 2; the king family) ------
    // [distance from the edge * 7 + relative rank]; rank 0 is a file without
    // a pawn, or with it behind the king.
    int shelter_strength[28] = { -31, -5, 49, 9, -24, 86, 106, -66, 83, 35, -28, -43, 50, 181, -49, 54, -15, -39, -53, 41, 136, -86, -40, -63, -77, -66, -149, -128 };
    int unblocked_storm[28] = { 116, -187, -57, 60, 75, 72, 59, 70, -210, -6, 23, 11, 8, 13, 17, -76, 133, 3, 1, 6, 21, 9, 49, 103, 49, 19, -11, -9 };
    // A storming pawn blocked by one of ours, by its relative rank.
    int blocked_storm_mg[7] = { 0, 0, 138, 8, 0, 15, 51 };
    int blocked_storm_eg[7] = { 0, 0, 61, 34, 26, 18, 13 };
    // Per step from the king to its nearest pawn.
    int king_pawn_distance_eg = 12;

    // ---- Attack maps, pieces and mobility (ticket 3) -------------------------
    // King danger's attacker weights by PieceType (knobs; the king family
    // reads their sum).
    int king_attack_weight[7] = { 0, 0, 51, 23, 10, 42, 0 };
    // Mobility by the count of attacked squares in the mobility area.
    int mob_n_mg[9] = { -58, -91, -34, -9, 6, 21, 33, 46, 54 };
    int mob_n_eg[9] = { -53, -66, -33, -13, 2, 16, 15, 21, 6 };
    int mob_b_mg[14] = { -34, -14, 24, 42, 51, 63, 67, 69, 69, 76, 70, 87, 57, 37 };
    int mob_b_eg[14] = { -28, -35, 4, 25, 38, 46, 52, 60, 68, 66, 72, 68, 85, 73 };
    int mob_r_mg[15] = { -54, -61, -1, -1, 4, 13, 20, 29, 40, 45, 44, 50, 60, 67, 83 };
    int mob_r_eg[15] = { -111, 27, 58, 86, 99, 104, 112, 117, 122, 129, 137, 144, 145, 147, 146 };
    int mob_q_mg[28] = { 161, 25, -33, -14, 20, 24, 24, 40, 48, 54, 57, 59, 68, 59, 69, 68, 57, 68, 62, 60, 59, 84, 83, 77, 97, 167, 121, 36 };
    int mob_q_eg[28] = { 146, 121, 99, 6, -67, 42, 73, 67, 86, 106, 115, 118, 125, 141, 129, 137, 155, 146, 159, 162, 167, 153, 146, 155, 139, 92, 88, 42 };
    // The piece terms; penalties are subtracted where they apply.
    int rook_on_king_ring_mg = 19;
    int rook_on_king_ring_eg = -16;
    int bishop_on_king_ring_mg = 34;
    int bishop_on_king_ring_eg = 16;
    int outpost_mg[2] = { 62, 69 };  // knight, bishop
    int outpost_eg[2] = { 29, 14 };
    int bad_outpost_mg = -12;
    int bad_outpost_eg = 15;
    int reachable_outpost_mg = 58;
    int reachable_outpost_eg = 15;
    int minor_behind_pawn_mg = 22;
    int minor_behind_pawn_eg = 15;
    int king_protector_mg[2] = { 11, 16 };  // knight, bishop; per step from our king
    int king_protector_eg[2] = { 7, 5 };
    int bishop_pawns_mg = 2;
    int bishop_pawns_eg = 6;
    int bishop_xray_pawns_mg = 16;
    int bishop_xray_pawns_eg = 3;
    int long_diagonal_bishop_mg = 79;
    int long_diagonal_bishop_eg = 30;
    int rook_on_queen_file_mg = 17;
    int rook_on_queen_file_eg = -10;
    int rook_on_file_mg[2] = { 28, 67 };  // semi-open, open
    int rook_on_file_eg[2] = { 6, 18 };
    int trapped_rook_mg = 45;
    int trapped_rook_eg = 20;
    int weak_queen_mg = 61;
    int weak_queen_eg = 13;
    int queen_infiltration_mg = 9;
    int queen_infiltration_eg = 23;

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
    int pawnless_flank_mg = -74;
    int pawnless_flank_eg = 108;
    int flank_attacks_mg = 6;      // per attack on our flank
    int flank_attacks_eg = -4;

    // ---- Threats (ticket 5) ---------------------------------------------------
    // By the attacked piece's PieceType: a minor attacking a weak or
    // defended piece, a rook a weak one.
    int threat_by_minor_mg[7] = { 0, 3, 73, 113, 192, 164, 480 };
    int threat_by_minor_eg[7] = { 0, 38, 55, 60, 63, 53, 408 };
    int threat_by_rook_mg[7] = { 0, -1, 72, 81, 15, 81, 481 };
    int threat_by_rook_eg[7] = { 0, 37, 56, 61, 7, 66, 584 };
    int threat_by_king_mg = 49;
    int threat_by_king_eg = 43;
    int hanging_mg = 18;
    int hanging_eg = 38;
    int weak_queen_protection_mg = 19;
    int weak_queen_protection_eg = 16;
    int restricted_piece_mg = 12;
    int restricted_piece_eg = 3;
    int threat_by_safe_pawn_mg = 190;
    int threat_by_safe_pawn_eg = 76;
    int threat_by_pawn_push_mg = 48;
    int threat_by_pawn_push_eg = 31;
    int knight_on_queen_mg = 26;
    int knight_on_queen_eg = -7;
    int slider_on_queen_mg = 41;
    int slider_on_queen_eg = 6;

    // ---- Passed pawns (ticket 5) ----------------------------------------------
    int passed_rank_mg[8] = { 0, -3, 12, 34, 64, 161, 276, 0 };
    int passed_rank_eg[8] = { 0, 44, 53, 93, 135, 173, 260, 0 };
    int passed_file_mg = 8;  // per file from the edge, subtracted
    int passed_file_eg = 11;
    // By relative rank 3 to 6 and the king's distance to the block square
    // (or to the square after it), capped at 5 (rb::passer_distance_index).
    // Seeded with the reference's truncated products, the rank weight
    // w = 5r - 13 times (d * 19 / 4), 2d and d.
    int passed_their_king_eg[24] = { 20, 9, 0, 7, 35, 68, 16, 26, 29, 73, 147, 193, -18, 13, 54, 173, 281, 326, -39, 12, 68, 297, 383, 451 };
    int passed_our_king_eg[24] = { 21, -5, 0, 11, 18, 14, 28, -7, 2, 32, 64, 93, -13, -44, 24, 84, 142, 166, -257, 34, 48, 201, 235, 249 };
    int passed_our_king_next_eg[24] = { -32, -42, 3, 19, 27, 54, -38, -83, 26, 67, 66, 65, 91, -90, -4, 68, 65, 50, 0, 17, 34, 51, 68, 85 };
    // A passer with a free block square, by relative rank 3 to 6 and the
    // path's case * 2 + whether we hold the block square
    // (rb::passer_path_index). Seeded with k * w, k = 35, 20, 9, 0, plus 5
    // when held.
    int passed_path_mg[32] = { -218, -171, -215, -136, -19, 12, -25, 9, -253, -185, -79, -34, -32, 46, -24, 36, -71, 85, 244, 338, -2, 185, -5, 57, 370, 532, 334, 528, 153, 238, 211, 172 };
    int passed_path_eg[32] = { 98, 55, 66, 61, 18, 30, -3, 10, 239, 196, 93, 131, 57, 92, 5, 39, 541, 384, 172, 253, 101, 145, 14, 85, 791, 743, 599, 887, 153, 238, 70, 303 };

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
