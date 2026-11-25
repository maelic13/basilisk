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
    X(Tempo,                tempo,                 1)
