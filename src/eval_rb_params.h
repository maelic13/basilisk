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
    int mg_val[7] = { 0, 124, 781, 825, 1276, 2538, 0 };
    int eg_val[7] = { 0, 206, 854, 915, 1380, 2682, 0 };

    // ---- Piece-square tables: [PieceType - 1][square], White's view --------
    // Squares a1 = 0 to h8 = 63; init_eval_tables mirrors them for Black. The
    // reference's tables are file-symmetric for the pieces; these hold all 64
    // squares, so the seed is a point of the larger space.
    int pst_mg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
             3,    3,   10,   19,   16,   19,    7,   -5,
            -9,  -15,   11,   15,   32,   22,    5,  -22,
            -4,  -23,    6,   20,   40,   17,    4,   -8,
            13,    0,  -13,    1,   11,   -2,  -13,    5,
             5,  -12,   -7,   22,   -8,   -5,  -15,   -8,
            -7,    7,   -3,  -13,    5,  -16,   10,   -8,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        { -175,  -92,  -74,  -73,  -73,  -74,  -92, -175,
           -77,  -41,  -27,  -15,  -15,  -27,  -41,  -77,
           -61,  -17,    6,   12,   12,    6,  -17,  -61,
           -35,    8,   40,   49,   49,   40,    8,  -35,
           -34,   13,   44,   51,   51,   44,   13,  -34,
            -9,   22,   58,   53,   53,   58,   22,   -9,
           -67,  -27,    4,   37,   37,    4,  -27,  -67,
          -201,  -83,  -56,  -26,  -26,  -56,  -83, -201 },
        // BISHOP
        {  -53,   -5,   -8,  -23,  -23,   -8,   -5,  -53,
           -15,    8,   19,    4,    4,   19,    8,  -15,
            -7,   21,   -5,   17,   17,   -5,   21,   -7,
            -5,   11,   25,   39,   39,   25,   11,   -5,
           -12,   29,   22,   31,   31,   22,   29,  -12,
           -16,    6,    1,   11,   11,    1,    6,  -16,
           -17,  -14,    5,    0,    0,    5,  -14,  -17,
           -48,    1,  -14,  -23,  -23,  -14,    1,  -48 },
        // ROOK
        {  -31,  -20,  -14,   -5,   -5,  -14,  -20,  -31,
           -21,  -13,   -8,    6,    6,   -8,  -13,  -21,
           -25,  -11,   -1,    3,    3,   -1,  -11,  -25,
           -13,   -5,   -4,   -6,   -6,   -4,   -5,  -13,
           -27,  -15,   -4,    3,    3,   -4,  -15,  -27,
           -22,   -2,    6,   12,   12,    6,   -2,  -22,
            -2,   12,   16,   18,   18,   16,   12,   -2,
           -17,  -19,   -1,    9,    9,   -1,  -19,  -17 },
        // QUEEN
        {    3,   -5,   -5,    4,    4,   -5,   -5,    3,
            -3,    5,    8,   12,   12,    8,    5,   -3,
            -3,    6,   13,    7,    7,   13,    6,   -3,
             4,    5,    9,    8,    8,    9,    5,    4,
             0,   14,   12,    5,    5,   12,   14,    0,
            -4,   10,    6,    8,    8,    6,   10,   -4,
            -5,    6,   10,    8,    8,   10,    6,   -5,
            -2,   -2,    1,   -2,   -2,    1,   -2,   -2 },
        // KING
        {  271,  327,  271,  198,  198,  271,  327,  271,
           278,  303,  234,  179,  179,  234,  303,  278,
           195,  258,  169,  120,  120,  169,  258,  195,
           164,  190,  138,   98,   98,  138,  190,  164,
           154,  179,  105,   70,   70,  105,  179,  154,
           123,  145,   81,   31,   31,   81,  145,  123,
            88,  120,   65,   33,   33,   65,  120,   88,
            59,   89,   45,   -1,   -1,   45,   89,   59 },
    };
    int pst_eg[6][64] = {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           -10,   -6,   10,    0,   14,    7,   -5,  -19,
           -10,  -10,  -10,    4,    4,    3,   -6,   -4,
             6,   -2,   -8,   -4,  -13,  -12,  -10,   -9,
            10,    5,    4,   -5,   -5,   -5,   14,    9,
            28,   20,   21,   28,   30,    7,    6,   13,
             0,  -11,   12,   21,   25,   19,    4,    7,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -96,  -65,  -49,  -21,  -21,  -49,  -65,  -96,
           -67,  -54,  -18,    8,    8,  -18,  -54,  -67,
           -40,  -27,   -8,   29,   29,   -8,  -27,  -40,
           -35,   -2,   13,   28,   28,   13,   -2,  -35,
           -45,  -16,    9,   39,   39,    9,  -16,  -45,
           -51,  -44,  -16,   17,   17,  -16,  -44,  -51,
           -69,  -50,  -51,   12,   12,  -51,  -50,  -69,
          -100,  -88,  -56,  -17,  -17,  -56,  -88, -100 },
        // BISHOP
        {  -57,  -30,  -37,  -12,  -12,  -37,  -30,  -57,
           -37,  -13,  -17,    1,    1,  -17,  -13,  -37,
           -16,   -1,   -2,   10,   10,   -2,   -1,  -16,
           -20,   -6,    0,   17,   17,    0,   -6,  -20,
           -17,   -1,  -14,   15,   15,  -14,   -1,  -17,
           -30,    6,    4,    6,    6,    4,    6,  -30,
           -31,  -20,   -1,    1,    1,   -1,  -20,  -31,
           -46,  -42,  -37,  -24,  -24,  -37,  -42,  -46 },
        // ROOK
        {   -9,  -13,  -10,   -9,   -9,  -10,  -13,   -9,
           -12,   -9,   -1,   -2,   -2,   -1,   -9,  -12,
             6,   -8,   -2,   -6,   -6,   -2,   -8,    6,
            -6,    1,   -9,    7,    7,   -9,    1,   -6,
            -5,    8,    7,   -6,   -6,    7,    8,   -5,
             6,    1,   -7,   10,   10,   -7,    1,    6,
             4,    5,   20,   -5,   -5,   20,    5,    4,
            18,    0,   19,   13,   13,   19,    0,   18 },
        // QUEEN
        {  -69,  -57,  -47,  -26,  -26,  -47,  -57,  -69,
           -55,  -31,  -22,   -4,   -4,  -22,  -31,  -55,
           -39,  -18,   -9,    3,    3,   -9,  -18,  -39,
           -23,   -3,   13,   24,   24,   13,   -3,  -23,
           -29,   -6,    9,   21,   21,    9,   -6,  -29,
           -38,  -18,  -12,    1,    1,  -12,  -18,  -38,
           -50,  -27,  -24,   -8,   -8,  -24,  -27,  -50,
           -75,  -52,  -43,  -36,  -36,  -43,  -52,  -75 },
        // KING
        {    1,   45,   85,   76,   76,   85,   45,    1,
            53,  100,  133,  135,  135,  133,  100,   53,
            88,  130,  169,  175,  175,  169,  130,   88,
           103,  156,  172,  172,  172,  172,  156,  103,
            96,  166,  199,  199,  199,  199,  166,   96,
            92,  172,  184,  191,  191,  184,  172,   92,
            47,  121,  116,  131,  131,  116,  121,   47,
            11,   59,   73,   78,   78,   73,   59,   11 },
    };
    // ---- Material imbalance (knobs) ----------------------------------------
    // Piece index 0 = the bishop pair, then pawn, knight, bishop, rook, queen.
    // imb_ours[i*(i+1)/2 + j], j <= i: our pieces i against our pieces j;
    // imb_theirs[i*(i-1)/2 + j], j < i: our pieces i against theirs j. Each
    // side's polynomial is summed, then (White's - Black's) / 16, truncated.
    int imb_ours[21] = {
        1438,
          40,   38,
          32,  255,  -62,
           0,  104,    4,    0,
         -26,   -2,   47,  105, -208,
        -189,   24,  117,  133, -134,   -6 };
    int imb_theirs[15] = {
          36,
           9,   63,
          59,   65,   42,
          46,   39,   24,  -24,
          97,  100,  -42,  137,  268 };

    // ---- Pawn structure (ticket 2) ------------------------------------------
    // Penalties are subtracted where they apply.
    int doubled_mg = 11;
    int doubled_eg = 56;
    int isolated_mg = 5;
    int isolated_eg = 15;
    int backward_mg = 9;
    int backward_eg = 24;
    int weak_lever_mg = 0;
    int weak_lever_eg = 56;
    int weak_unopposed_mg = 13;
    int weak_unopposed_eg = 27;
    // A blocked pawn on its fifth or sixth rank.
    int blocked_pawn_mg[2] = { -11, -3 };
    int blocked_pawn_eg[2] = { -4, 4 };
    // A connected pawn by rank, phalanx, opposition and supporters
    // (rb::connected_index). Seeded with the reference's products: the
    // middlegame value Connected[r] * (2 + phalanx - opposed) + 21 * support,
    // the endgame value that times (r - 2) / 4, truncated.
    int connected_mg[72] = {
          14,   35,   56,    7,   28,   49,   21,   42,   63,   14,   35,   56,
          16,   37,   58,    8,   29,   50,   24,   45,   66,   16,   37,   58,
          24,   45,   66,   12,   33,   54,   36,   57,   78,   24,   45,   66,
          58,   79,  100,   29,   50,   71,   87,  108,  129,   58,   79,  100,
          96,  117,  138,   48,   69,   90,  144,  165,  186,   96,  117,  138,
         172,  193,  214,   86,  107,  128,  258,  279,  300,  172,  193,  214 };
    int connected_eg[72] = {
          -3,   -8,  -14,   -1,   -7,  -12,   -5,  -10,  -15,   -3,   -8,  -14,
           0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
           6,   11,   16,    3,    8,   13,    9,   14,   19,    6,   11,   16,
          29,   39,   50,   14,   25,   35,   43,   54,   64,   29,   39,   50,
          72,   87,  103,   36,   51,   67,  108,  123,  139,   72,   87,  103,
         172,  193,  214,   86,  107,  128,  258,  279,  300,  172,  193,  214 };

    // ---- The king's pawn shelter and storm (ticket 2; the king family) ------
    // [distance from the edge * 7 + relative rank]; rank 0 is a file without
    // a pawn, or with it behind the king.
    int shelter_strength[28] = {
         -6,  81,  93,  58,  39,  18,   25,
        -43,  61,  35, -49, -29, -11,  -63,
        -10,  75,  23,  -2,  32,   3,  -45,
        -39, -13, -29, -52, -48, -67, -166 };
    int unblocked_storm[28] = {
         85, -289, -166, 97, 50,  45,  50,
         46,  -25,  122, 45, 37, -10,  20,
         -6,   51,  168, 34, -2, -22, -14,
        -15,  -11,  101,  4, 11, -15, -29 };
    // A storming pawn blocked by one of ours, by its relative rank.
    int blocked_storm_mg[7] = { 0, 0, 76, -10, -7, -4, -1 };
    int blocked_storm_eg[7] = { 0, 0, 78, 15, 10, 6, 2 };
    // Per step from the king to its nearest pawn.
    int king_pawn_distance_eg = 16;

    // ---- Attack maps, pieces and mobility (ticket 3) -------------------------
    // King danger's attacker weights by PieceType (knobs; the king family
    // reads their sum).
    int king_attack_weight[7] = { 0, 0, 81, 52, 44, 10, 0 };
    // Mobility by the count of attacked squares in the mobility area.
    int mob_n_mg[9] = { -62, -53, -12, -4, 3, 13, 22, 28, 33 };
    int mob_n_eg[9] = { -81, -56, -31, -16, 5, 11, 17, 20, 25 };
    int mob_b_mg[14] = { -48, -20, 16, 26, 38, 51, 55, 63, 63, 68, 81, 81, 91, 98 };
    int mob_b_eg[14] = { -59, -23, -3, 13, 24, 42, 54, 57, 65, 73, 78, 86, 88, 97 };
    int mob_r_mg[15] = { -60, -20, 2, 3, 3, 11, 22, 31, 40, 40, 41, 48, 57, 57, 62 };
    int mob_r_eg[15] = { -78, -17, 23, 39, 70, 99, 103, 121, 134, 139, 158, 164, 168, 169, 172 };
    int mob_q_mg[28] = { -30, -12, -8, -9, 20, 23, 23, 35, 38, 53, 64, 65, 65, 66, 67, 67, 72, 72, 77, 79, 93, 108, 108, 108, 110, 114, 114, 116 };
    int mob_q_eg[28] = { -48, -30, -7, 19, 40, 55, 59, 75, 78, 96, 96, 100, 121, 127, 131, 133, 136, 141, 147, 150, 151, 168, 168, 171, 182, 182, 192, 219 };
    // The piece terms; penalties are subtracted where they apply.
    int rook_on_king_ring_mg = 16;
    int rook_on_king_ring_eg = 0;
    int bishop_on_king_ring_mg = 24;
    int bishop_on_king_ring_eg = 0;
    int outpost_mg[2] = { 56, 30 };  // knight, bishop
    int outpost_eg[2] = { 36, 23 };
    int bad_outpost_mg = -7;
    int bad_outpost_eg = 36;
    int reachable_outpost_mg = 31;
    int reachable_outpost_eg = 22;
    int minor_behind_pawn_mg = 18;
    int minor_behind_pawn_eg = 3;
    int king_protector_mg[2] = { 8, 6 };  // knight, bishop; per step from our king
    int king_protector_eg[2] = { 9, 9 };
    int bishop_pawns_mg = 3;
    int bishop_pawns_eg = 7;
    int bishop_xray_pawns_mg = 4;
    int bishop_xray_pawns_eg = 5;
    int long_diagonal_bishop_mg = 45;
    int long_diagonal_bishop_eg = 0;
    int rook_on_queen_file_mg = 6;
    int rook_on_queen_file_eg = 11;
    int rook_on_file_mg[2] = { 19, 48 };  // semi-open, open
    int rook_on_file_eg[2] = { 7, 29 };
    int trapped_rook_mg = 55;
    int trapped_rook_eg = 13;
    int weak_queen_mg = 56;
    int weak_queen_eg = 15;
    int queen_infiltration_mg = -2;
    int queen_infiltration_eg = 14;

    // ---- King danger (ticket 4; knobs: the index is squared) ---------------
    // A safe check by PieceType * 2 + whether there are several.
    int safe_check[14] = { 0, 0, 0, 0, 792, 1283, 645, 967, 1084, 1897, 772, 1119, 0, 0 };
    int kd_weak_ring = 185;        // per weak square in the king ring
    int kd_unsafe_checks = 148;    // per square an unsafe check could come from
    int kd_blockers = 98;          // per piece shielding our king from a slider
    int kd_king_attacks = 69;      // per attack on a square next to our king
    int kd_flank_attack = 3;       // times the flank attack's square, over 8
    int kd_no_queen = 873;         // subtracted when they have no queen
    int kd_knight_defender = 100;  // subtracted when our knight defends next to our king
    int kd_shelter = 6;            // times the shelter's middlegame value, over 8, subtracted
    int kd_flank_defence = 4;      // per square we defend in our flank, subtracted
    int kd_offset = 37;
    // ---- The king's flank (ticket 4; linear) ----------------------------------
    int pawnless_flank_mg = 17;
    int pawnless_flank_eg = 95;
    int flank_attacks_mg = 8;      // per attack on our flank
    int flank_attacks_eg = 0;

    // ---- Threats (ticket 5) ---------------------------------------------------
    // By the attacked piece's PieceType: a minor attacking a weak or
    // defended piece, a rook a weak one.
    int threat_by_minor_mg[7] = { 0, 5, 57, 77, 88, 79, 0 };
    int threat_by_minor_eg[7] = { 0, 32, 41, 56, 119, 161, 0 };
    int threat_by_rook_mg[7] = { 0, 3, 37, 42, 0, 58, 0 };
    int threat_by_rook_eg[7] = { 0, 46, 68, 60, 38, 41, 0 };
    int threat_by_king_mg = 24;
    int threat_by_king_eg = 89;
    int hanging_mg = 69;
    int hanging_eg = 36;
    int weak_queen_protection_mg = 14;
    int weak_queen_protection_eg = 0;
    int restricted_piece_mg = 7;
    int restricted_piece_eg = 7;
    int threat_by_safe_pawn_mg = 173;
    int threat_by_safe_pawn_eg = 94;
    int threat_by_pawn_push_mg = 48;
    int threat_by_pawn_push_eg = 39;
    int knight_on_queen_mg = 16;
    int knight_on_queen_eg = 11;
    int slider_on_queen_mg = 60;
    int slider_on_queen_eg = 18;

    // ---- Passed pawns (ticket 5) ----------------------------------------------
    int passed_rank_mg[8] = { 0, 10, 17, 15, 62, 168, 276, 0 };
    int passed_rank_eg[8] = { 0, 28, 33, 41, 72, 177, 260, 0 };
    int passed_file_mg = 11;  // per file from the edge, subtracted
    int passed_file_eg = 8;
    // By relative rank 3 to 6 and the king's distance to the block square
    // (or to the square after it), capped at 5 (rb::passer_distance_index).
    // Seeded with the reference's truncated products, the rank weight
    // w = 5r - 13 times (d * 19 / 4), 2d and d.
    int passed_their_king_eg[24] = {
           0,    8,   18,   28,   38,   46,
           0,   28,   63,   98,  133,  161,
           0,   48,  108,  168,  228,  276,
           0,   68,  153,  238,  323,  391 };
    int passed_our_king_eg[24] = {
           0,    4,    8,   12,   16,   20,
           0,   14,   28,   42,   56,   70,
           0,   24,   48,   72,   96,  120,
           0,   34,   68,  102,  136,  170 };
    int passed_our_king_next_eg[24] = {
           0,    2,    4,    6,    8,   10,
           0,    7,   14,   21,   28,   35,
           0,   12,   24,   36,   48,   60,
           0,   17,   34,   51,   68,   85 };
    // A passer with a free block square, by relative rank 3 to 6 and the
    // path's case * 2 + whether we hold the block square
    // (rb::passer_path_index). Seeded with k * w, k = 35, 20, 9, 0, plus 5
    // when held.
    int passed_path_mg[32] = {
          70,   80,   40,   50,   18,   28,    0,   10,
         245,  280,  140,  175,   63,   98,    0,   35,
         420,  480,  240,  300,  108,  168,    0,   60,
         595,  680,  340,  425,  153,  238,    0,   85 };
    int passed_path_eg[32] = {
          70,   80,   40,   50,   18,   28,    0,   10,
         245,  280,  140,  175,   63,   98,    0,   35,
         420,  480,  240,  300,  108,  168,    0,   60,
         595,  680,  340,  425,  153,  238,    0,   85 };

    // ---- Space (ticket 5; a knob) ---------------------------------------------
    // The space count times the weight squared, times this, over 256.
    int space_weight = 16;

    // ---- Tempo, in units; converted with the value --------------------------
    int tempo = 28;
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
