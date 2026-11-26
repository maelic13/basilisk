#pragma once

// The rebuilt evaluation's registry: every value its score reads, in the
// classical form's own units (a pawn is 124 in the middlegame and 206 in the
// endgame), with the reference's constants as the seed vector. Included by
// eval_params.h alone, in place of the accepted evaluation's registry, when
// BASILISK_EVAL_REBUILD is defined; the loader, the dumper and the trace read
// whichever registry the build has through EVAL_PARAM_LIST.
//
// A slot the score is linear in is a fitted value. A knob enters through
// arithmetic the reference truncates (the imbalance's sixteenth), so the
// score is not linear in it. The structure constants no static fit prices
// (the phase limits, the lazy thresholds, the fixed non-pawn values) are not
// here: they live beside the code that reads them (eval_rb_common.h).

struct EvalParams {

    // ---- Material values [PieceType 0..6] -----------------------------------
    int mg_val[7] = { 0, 81, 633, 678, 997, 2384, 0 };
    int eg_val[7] = { 0, 300, 1047, 1106, 1757, 3080, 0 };

    // ---- Piece-square tables: [PieceType - 1][square], White's view --------
    // Squares a1 = 0 to h8 = 63; init_eval_tables mirrors them for Black. The
    // reference's tables are file-symmetric for the pieces; these hold all 64
    // squares, so the seed is a point of the larger space.
    int pst_mg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
            -5,   -3,  -14,  -11,   10,   -3,   14,   21,
           -28,  -32,  -31,  -34,   -4,  -15,  -15,  -26,
            -5,  -15,   -8,    0,   10,   18,    1,    6,
            34,   13,   11,   15,   28,   43,   28,   61,
            61,   56,   40,   80,   91,   25,   57,   60,
           -61,  -88,  -76,  -37,   80, -128, -114, -127,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -56,  -38,  -68,  -22,  -25,  -42,  -60,  -87,
           -53,  -46,  -36,  -31,  -25,  -29,  -25,  -37,
           -22,  -14,  -15,  -20,   -2,  -13,   -6,  -32,
             9,   33,   31,   12,   31,   32,   33,    6,
            17,   29,   34,   79,   40,   75,   26,   58,
             2,   -4,   63,   55,   96,   94,   38,    4,
            -8,  -10,   54,   54,   68,   97,   38,  130,
          -249, -250,    3,    5,   20,  -34,  203, -218 },
        // BISHOP
        {    6,   49,   -6,  -14,    4,  -19,   12,  -30,
            53,   -8,   21,  -21,  -13,    5,  -12,  -17,
            18,   56,  -10,   13,    1,  -21,   18,    1,
            31,   23,   43,   22,   40,   17,    6,   20,
             7,   28,   33,   55,   43,   43,   12,   -9,
            21,   18,   -8,   49,   47,   52,   21,   33,
           -15,  -20,   37,   26,    1,   20,  -62,  -20,
           -24,    9, -103, -166,  -75, -191,  -65,  -72 },
        // ROOK
        {  -40,  -32,  -30,  -17,   -7,  -20,    9,  -36,
           -51,  -39,  -23,  -10,  -10,    3,   28,  -14,
           -43,  -25,  -34,  -25,   -9,   -7,   22,   -3,
           -70,  -34,  -32,  -24,  -10,  -16,   36,  -20,
           -43,  -14,   -2,   17,   38,   38,   29,   29,
           -32,    2,    8,   19,  107,   76,   87,   20,
           -35,  -35,    0,   33,    5,   83,   31,   13,
             2,   16,    0,   18,   23,  -60,   22,  115 },
        // QUEEN
        {   -6,  -27,  -25,  -30,  -15,    9,   -1,   13,
           -35,  -22,  -23,  -14,  -11,   -8,   15,  -14,
           -11,  -15,   -9,  -25,  -24,    9,   10,   -2,
           -20,   -7,  -14,  -23,   -9,   -7,  -14,  -22,
             0,   22,   21,    9,    3,    3,  -37,   -2,
            21,   27,   22,   54,  -31,   -3,  -65,   19,
           -19,  -10,   36,   17,  -18,    0,   22,   70,
           -51,   48,   12,   36,  -81,   31,  158,   72 },
        // KING
        {  322,  319,  292,  261,  232,  240,  324,  325,
           305,  273,  253,  248,  227,  231,  302,  306,
           323,  299,  285,  223,  184,  177,  242,  252,
           262,  239,   98,  -13,   -2,  -21,   83,   62,
           137,  133,    3,  -58,   -4,    6,  -22,   26,
           177,  250,   11,  -53,   40,  -19,   12,  181,
           216,  134,  153,  166,   27,   85,  147,  138,
           119,  163,   71,  -20,    8,   24,   92,   62 }
    };
    int pst_eg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -47,  -49,  -56,  -69,  -19,  -31,  -56,  -89,
           -49,  -56,  -61,  -61,  -50,  -46,  -66,  -74,
           -33,  -43,  -66,  -58,  -61,  -54,  -56,  -66,
             4,  -25,  -50,  -54,  -51,  -46,  -38,  -34,
            83,   50,   18,   15,   59,   52,   63,   46,
           127,  140,  154,  167,  193,  168,  140,  116,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -61,  -68,  -36,   -1,  -34,  -44,  -74,  -70,
           -38,  -16,  -15,  -12,  -25,  -33,  -31,  -30,
           -12,   -4,   -6,   17,   23,   -8,  -15,  -29,
            18,   13,   30,   43,   57,   39,   23,   14,
            32,   21,   28,   75,   46,   47,   36,    2,
           -25,   21,   48,   46,   45,   55,   25,    7,
           -20,    7,   21,   87,   46,   13,    9,  -24,
           -89,    9,    4,   16,   15,  -32,  -69,  -93 },
        // BISHOP
        {  -55,  -27,  -18,  -21,   -6,   -7,  -52, -104,
            10,  -55,   -9,    0,   -1,    2,  -63,  -85,
            -5,   29,  -22,   17,   26,  -32,  -15,  -15,
            -5,    5,   21,   18,   22,   14,    6,  -30,
             4,   13,    6,   45,   31,   17,   21,   -7,
             3,   23,  -10,   27,   45,   11,   43,   15,
            18,  -24,   11,   30,   32,   46,  -21,  -50,
            -9,  -18,   39,   34,   24,   21,   34,  -16 },
        // ROOK
        {  -25,  -25,  -21,  -27,  -44,  -26,  -55,  -61,
           -35,  -28,  -19,  -31,  -45,  -41,  -66,  -93,
           -21,  -12,  -10,  -27,  -37,  -29,  -41,  -68,
            19,   11,   16,    9,    4,    2,   -1,  -39,
            23,   37,   24,   21,   10,    9,   17,   -6,
            20,   29,   21,   16,    0,    3,   15,    8,
            31,   34,   35,   32,   13,   41,   48,   36,
            26,   39,   53,   48,   40,   67,   50,   44 },
        // QUEEN
        {  -77,  -43,  -47,  -11,  -51, -124, -164, -201,
             8,  -26,   14,   13,   15,  -28,  -78,  -67,
           -27,   18,   20,   52,   60,   46,   14,    1,
            50,   41,   70,   79,   91,   64,   84,   72,
           -43,    3,  -19,   25,   66,   53,  129,   44,
           -87,  -54,   -7,  -21,   61,   42,   55,  -41,
           -65,  -77,  -12,   16,   46,   32,   27,  -20,
           -34,  -47,    4,   -1,   77,   31,  -15,  -15 },
        // KING
        {   -2,   39,   76,   51,   43,   57,   29,  -15,
            70,   88,   99,  103,  109,   96,   81,   55,
            76,   99,  141,  154,  150,  127,  109,   84,
           108,  144,  187,  197,  197,  182,  153,  115,
           132,  184,  231,  234,  246,  229,  203,  141,
           105,  184,  221,  243,  247,  242,  215,  147,
           -69,   86,  170,  176,  194,  169,  165,   80,
          -224,  -17,    4,   44,   97,  138,  103,  -70 }
    };
    // ---- Material imbalance (knobs) ----------------------------------------
    // Piece index 0 = the bishop pair, then pawn, knight, bishop, rook, queen.
    // imb_ours[i*(i+1)/2 + j], j <= i: our pieces i against our pieces j;
    // imb_theirs[i*(i-1)/2 + j], j < i: our pieces i against theirs j. Each
    // side's polynomial is summed, then (White's - Black's) / 16, truncated.
    int imb_ours[21] = { 1699, 40, 97, -32, 217, -98, -84, 66, -113, 1, -28, -75, 64, 143, -160, -208, -126, -670, -804, -937, 304 };
    int imb_theirs[15] = { 16, 90, 63, 301, 65, 79, -194, 172, 17, -66, 470, 371, 182, 436, 989 };

    // ---- Pawn structure (ticket 2) ------------------------------------------
    // Penalties are subtracted where they apply.
    int doubled_mg = -7;
    int doubled_eg = 63;
    int isolated_mg = 5;
    int isolated_eg = 19;
    int backward_mg = 5;
    int backward_eg = 20;
    int weak_lever_mg = -20;
    int weak_lever_eg = 65;
    int weak_unopposed_mg = 6;
    int weak_unopposed_eg = 12;
    // A blocked pawn on its fifth or sixth rank.
    int blocked_pawn_mg[2] = { -26, -38 };
    int blocked_pawn_eg[2] = { -20, -74 };
    // A connected pawn by rank, phalanx, opposition and supporters
    // (rb::connected_index). Seeded with the reference's products: the
    // middlegame value Connected[r] * (2 + phalanx - opposed) + 21 * support,
    // the endgame value that times (r - 2) / 4, truncated.
    int connected_mg[72] = { 14, 35, 56, 7, 28, 49, 10, 42, 63, 8, 35, 56, 16, 48, 108, 8, 36, 88, 27, 79, 177, 19, 61, 164, 24, 52, 73, 12, 31, 46, 30, 76, -1, 16, 48, 38, 58, 71, 121, 29, 28, 55, 54, 138, 129, 20, 65, 100, 96, 146, 258, 48, 43, 16, 237, 22, 186, 12, 117, 138, 172, 189, 214, 86, 107, 128, 152, 279, 300, 172, 193, 214 };
    int connected_eg[72] = { -3, -8, -14, -1, -7, -12, -6, -10, -15, -3, -8, -14, 0, 22, 30, 0, 23, 37, 0, 30, -174, 1, 37, -103, 6, 31, 35, 3, 11, 7, 21, 57, -167, 4, 29, 10, 29, 71, 114, 14, 29, 22, 32, 88, 64, 38, 36, 50, 72, 140, 112, 36, 86, 111, 175, 151, 139, -262, 87, 103, 172, 360, 214, 86, 107, 128, 252, 279, 300, 172, 193, 214 };

    // ---- The king's pawn shelter and storm (ticket 2; the king family) ------
    // [distance from the edge * 7 + relative rank]; rank 0 is a file without
    // a pawn, or with it behind the king.
    int shelter_strength[28] = { -31, -4, 33, 1, -22, 46, 47, -48, 68, 31, -16, -25, 29, 176, -33, 43, -6, -23, -32, 26, 100, -68, -36, -47, -57, -49, -118, -116 };
    int unblocked_storm[28] = { 80, -84, -39, 43, 49, 45, 34, 54, -125, 32, 17, 9, 2, 6, 9, -39, 123, 0, -4, 0, 9, 4, 31, 98, 35, 7, -14, -19 };
    // A storming pawn blocked by one of ours, by its relative rank.
    int blocked_storm_mg[7] = { 0, 0, 83, -3, -8, 5, 23 };
    int blocked_storm_eg[7] = { 0, 0, 86, 40, 32, 18, 8 };
    // Per step from the king to its nearest pawn.
    int king_pawn_distance_eg = 17;

    // ---- Attack maps, pieces and mobility (ticket 3) -------------------------
    // King danger's attacker weights by PieceType (knobs; the king family
    // reads their sum).
    int king_attack_weight[7] = { 0, 0, 36, 18, 5, 45, 0 };
    // Mobility by the count of attacked squares in the mobility area.
    int mob_n_mg[9] = { -40, -58, -24, -8, 1, 10, 19, 29, 40 };
    int mob_n_eg[9] = { -72, -92, -41, -13, 8, 28, 28, 34, 14 };
    int mob_b_mg[14] = { -9, 2, 25, 36, 42, 50, 52, 53, 53, 57, 56, 71, 65, 110 };
    int mob_b_eg[14] = { -47, -52, -3, 25, 42, 52, 60, 69, 76, 75, 79, 74, 84, 60 };
    int mob_r_mg[15] = { -32, -29, 7, 8, 9, 15, 20, 26, 33, 36, 35, 40, 48, 56, 65 };
    int mob_r_eg[15] = { -119, 0, 50, 80, 98, 104, 113, 119, 127, 134, 144, 152, 153, 152, 155 };
    int mob_q_mg[28] = { 430, 5, -56, -50, -34, -35, -37, -28, -24, -21, -18, -18, -12, -19, -14, -14, -24, -15, -19, -19, -20, -1, 11, 39, 173, 458, 597, 467 };
    int mob_q_eg[28] = { -198, 85, 71, -11, -44, 68, 106, 116, 135, 158, 165, 171, 181, 193, 191, 198, 217, 215, 226, 227, 232, 224, 202, 182, 83, -35, -117, -188 };
    // The piece terms; penalties are subtracted where they apply.
    int rook_on_king_ring_mg = 12;
    int rook_on_king_ring_eg = -12;
    int bishop_on_king_ring_mg = 17;
    int bishop_on_king_ring_eg = 30;
    int outpost_mg[2] = { 33, 42 };  // knight, bishop
    int outpost_eg[2] = { 46, 24 };
    int bad_outpost_mg = -22;
    int bad_outpost_eg = 20;
    int reachable_outpost_mg = 33;
    int reachable_outpost_eg = 27;
    int minor_behind_pawn_mg = 11;
    int minor_behind_pawn_eg = 24;
    int king_protector_mg[2] = { 10, 13 };  // knight, bishop; per step from our king
    int king_protector_eg[2] = { 8, 6 };
    int bishop_pawns_mg = 2;
    int bishop_pawns_eg = 7;
    int bishop_xray_pawns_mg = 9;
    int bishop_xray_pawns_eg = 6;
    int long_diagonal_bishop_mg = 45;
    int long_diagonal_bishop_eg = 50;
    int rook_on_queen_file_mg = 11;
    int rook_on_queen_file_eg = -4;
    int rook_on_file_mg[2] = { 17, 41 };  // semi-open, open
    int rook_on_file_eg[2] = { 15, 35 };
    int trapped_rook_mg = 26;
    int trapped_rook_eg = 34;
    int weak_queen_mg = 39;
    int weak_queen_eg = 24;
    int queen_infiltration_mg = -37;
    int queen_infiltration_eg = 114;

    // ---- King danger (ticket 4; knobs: the index is squared) ---------------
    // A safe check by PieceType * 2 + whether there are several.
    int safe_check[14] = { 0, 0, 0, 0, 553, 909, 637, 617, 672, 884, 502, 839, 0, 0 };
    int kd_weak_ring = 120;        // per weak square in the king ring
    int kd_unsafe_checks = 125;    // per square an unsafe check could come from
    int kd_blockers = 33;          // per piece shielding our king from a slider
    int kd_king_attacks = 94;      // per attack on a square next to our king
    int kd_flank_attack = 4;       // times the flank attack's square, over 8
    int kd_no_queen = 693;         // subtracted when they have no queen
    int kd_knight_defender = 163;  // subtracted when our knight defends next to our king
    int kd_shelter = 12;            // times the shelter's middlegame value, over 8, subtracted
    int kd_flank_defence = 26;      // per square we defend in our flank, subtracted
    int kd_offset = 132;
    // ---- The king's flank (ticket 4; linear) ----------------------------------
    int pawnless_flank_mg = -97;
    int pawnless_flank_eg = 140;
    int flank_attacks_mg = 3;      // per attack on our flank
    int flank_attacks_eg = -3;

    // ---- Threats (ticket 5) ---------------------------------------------------
    // By the attacked piece's PieceType: a minor attacking a weak or
    // defended piece, a rook a weak one.
    int threat_by_minor_mg[7] = { 0, 0, 41, 71, 125, 105, 703 };
    int threat_by_minor_eg[7] = { 0, 46, 74, 86, 102, 87, 686 };
    int threat_by_rook_mg[7] = { 0, -5, 41, 47, 7, 57, 623 };
    int threat_by_rook_eg[7] = { 0, 45, 73, 82, 14, 70, 867 };
    int threat_by_king_mg = 29;
    int threat_by_king_eg = 59;
    int hanging_mg = 18;
    int hanging_eg = 39;
    int weak_queen_protection_mg = 10;
    int weak_queen_protection_eg = 21;
    int restricted_piece_mg = 8;
    int restricted_piece_eg = 4;
    int threat_by_safe_pawn_mg = 127;
    int threat_by_safe_pawn_eg = 108;
    int threat_by_pawn_push_mg = 29;
    int threat_by_pawn_push_eg = 40;
    int knight_on_queen_mg = 17;
    int knight_on_queen_eg = -4;
    int slider_on_queen_mg = 24;
    int slider_on_queen_eg = 20;

    // ---- Passed pawns (ticket 5) ----------------------------------------------
    int passed_rank_mg[8] = { 0, -5, 3, 17, 33, 53, 276, 0 };
    int passed_rank_eg[8] = { 0, 54, 68, 127, 181, 231, 260, 0 };
    int passed_file_mg = 3;  // per file from the edge, subtracted
    int passed_file_eg = 14;
    // By relative rank 3 to 6 and the king's distance to the block square
    // (or to the square after it), capped at 5 (rb::passer_distance_index).
    // Seeded with the reference's truncated products, the rank weight
    // w = 5r - 13 times (d * 19 / 4), 2d and d.
    int passed_their_king_eg[24] = { 20, 11, -1, 3, 34, 70, 3, 19, 27, 75, 156, 204, -57, -9, 48, 183, 306, 358, -98, -28, 48, 315, 427, 509 };
    int passed_our_king_eg[24] = { 31, -1, 2, 10, 12, 7, 34, -10, 1, 30, 62, 94, -16, -67, 17, 85, 154, 187, -338, 18, 40, 230, 270, 291 };
    int passed_our_king_next_eg[24] = { -44, -64, -4, 22, 41, 78, -51, -116, 22, 76, 82, 91, 119, -127, -20, 72, 76, 60, 0, 17, 34, 51, 68, 85 };
    // A passer with a free block square, by relative rank 3 to 6 and the
    // path's case * 2 + whether we hold the block square
    // (rb::passer_path_index). Seeded with k * w, k = 35, 20, 9, 0, plus 5
    // when held.
    int passed_path_mg[32] = { -371, -266, -260, -165, -22, 3, -16, 4, -586, -446, -145, -77, -33, 15, -20, 15, -488, -246, 170, 225, -23, 99, -6, 20, 100, 271, 241, 344, 153, 238, 119, 62 };
    int passed_path_eg[32] = { 139, 86, 87, 86, 19, 34, -8, 11, 322, 268, 123, 169, 66, 115, 5, 52, 711, 500, 214, 314, 117, 187, 10, 104, 1034, 931, 724, 1073, 153, 238, 91, 373 };

    // ---- Space (ticket 5; a knob) ---------------------------------------------
    // The space count times the weight squared, times this, over 256.
    int space_weight = 4;

    // ---- Tempo, in units; converted with the value --------------------------
    int tempo = 36;
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
    X(Tempo,                tempo,                 1)
