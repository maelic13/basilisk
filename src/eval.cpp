#include "eval.h"
#include <algorithm>
#include <cstring>

// ---- Material values -------------------------------------------------------
static constexpr int MG_VAL[PIECE_TYPE_NB] = {0, 82, 337, 365, 477, 1025, 0};
static constexpr int EG_VAL[PIECE_TYPE_NB] = {0, 94, 281, 297, 512,  936, 0};

// Phase weights: Q=4, R=2, B=1, N=1; total=24
static constexpr int PHASE_W[PIECE_TYPE_NB] = {0, 0, 1, 1, 2, 4, 0};
static constexpr int TOTAL_PHASE = 24;

// ---- PeSTO PST tables (public domain, from PeSTO / Rofchade) ---------------
// Index: square a1=0 .. h8=63 (rank 1 = indices 0-7, rank 8 = indices 56-63)

static constexpr int MG_PAWN_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
   -35, -1,-20,-23,-15, 24, 38,-22,
   -26, -4, -4,-10,  3,  3, 33,-12,
   -27, -2, -5, 12, 17,  6, 10,-25,
   -14, 13,  6, 21, 23, 12, 17,-23,
    -6,  7, 26, 31, 65, 56, 25,-20,
    98,134, 61, 95, 68,126, 34,-11,
     0,  0,  0,  0,  0,  0,  0,  0
};
static constexpr int EG_PAWN_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
   -10, -6, 10,  0, 14,  7, -5,-19,
    -8, -4,  7, 22, 17, 16,  3,-14,
    13,  0,-13,  1, -1,-16,  3, -6,
    32, 24, 13,  5, -2,  4, 17, 17,
    56, 35, 41, 22, 26, 51, 56, 20,
   134,108,109,107,105,104,112,108,
     0,  0,  0,  0,  0,  0,  0,  0
};
static constexpr int MG_KNIGHT_PST[64] = {
  -167,-89,-34,-49, 61,-97,-15,-107,
   -73,-41, 72, 36, 23, 62,  7, -17,
   -47, 60, 37, 65, 84,129, 73,  44,
    -9, 17, 19, 53, 37, 69, 18,  22,
   -13,  4, 16, 13, 28, 19, 21,  -8,
   -23, -9, 12, 10, 19, 17, 25, -16,
   -29,-53,-12, -3, -1, 18,-14, -19,
  -105,-21,-58,-33,-17,-28,-19, -23
};
static constexpr int EG_KNIGHT_PST[64] = {
   -58,-38,-13,-28,-31,-27,-63,-99,
   -25, -8,-25, -2, -9,-25,-24,-52,
   -24,-20, 10,  9, -1, -9,-19,-41,
   -17,  3, 22, 22, 22, 11,  8,-18,
   -18, -6, 16, 25, 16, 17,  4,-18,
   -23, -3, -1, 15, 10, -3,-20,-22,
   -42,-20,-10, -5, -2,-20,-23,-44,
   -29,-51,-23,-15,-22,-18,-50,-64
};
static constexpr int MG_BISHOP_PST[64] = {
   -29,  4,-82,-37,-25,-42,  7, -8,
   -26, 16,-18,-13, 30, 59, 18,-47,
   -16, 37, 43, 40, 35, 50, 37, -2,
    -4,  5, 19, 50, 37, 37,  7, -2,
    -6, 13, 13, 26, 34, 12, 10,  4,
     0, 15, 15, 15, 14, 27, 18, 10,
     4, 15, 16,  0,  7, 21, 33,  1,
   -33, -3,-14,-21,-13,-12,-39,-21
};
static constexpr int EG_BISHOP_PST[64] = {
   -14,-21,-11, -8, -7, -9,-17,-24,
    -8, -4,  7,-12, -3,-13, -4,-14,
     2, -8,  0, -1, -2,  6,  0,  4,
    -3,  9, 12,  9, 14, 10,  3,  2,
    -6,  3, 13, 19,  7, 10, -3, -9,
   -12, -3,  8, 10, 13,  3, -7,-15,
   -14,-18, -7, -1,  4, -9,-15,-27,
   -23, -9,-23, -5, -9,-16, -5,-17
};
static constexpr int MG_ROOK_PST[64] = {
   -19,-13,  1, 17, 16,  7,-37,-26,
   -44,-16,-20, -9, -1, 11, -6,-71,
   -45,-25,-16,-17,  3,  0, -5,-33,
   -36,-26,-12, -1,  9, -7,  6,-23,
   -24,-11,  7, 26, 24, 35, -8,-20,
    -5, 19, 26, 36, 17, 45, 61, 16,
    27, 32, 58, 62, 80, 67, 26, 44,
    32, 42, 32, 51, 63,  9, 31, 43
};
static constexpr int EG_ROOK_PST[64] = {
    -9,  2,  3, -1, -5,-13,  4,-20,
    -6, -6,  0,  2, -9, -9,-11, -3,
    -4,  0, -5, -1, -7,-12, -8,-16,
     3,  5,  8,  4, -5, -6, -8,-11,
     4,  3, 13,  1,  2,  1, -1,  2,
     7,  7,  7,  5,  4, -3, -5, -3,
    11, 13, 13, 11, -3,  3,  8,  3,
    13, 10, 18, 15, 12, 12,  8,  5
};
static constexpr int MG_QUEEN_PST[64] = {
   -28,  0, 29, 12, 59, 44, 43, 45,
   -24,-39, -5,  1,-16, 57, 28, 54,
   -13,-17,  7,  8, 29, 56, 47, 57,
   -27,-27,-16,-16, -1, 17, -2,  1,
    -9,-26, -9,-10, -2, -4,  3, -3,
   -14,  2,-11, -2, -5,  2, 14,  5,
   -35, -8, 11,  2,  8, 15, -3,  1,
    -1,-18, -9, 10,-15,-25,-31,-50
};
static constexpr int EG_QUEEN_PST[64] = {
    -9, 22, 22, 27, 27, 19, 10, 20,
   -17, 20, 32, 41, 58, 25, 30,  0,
   -20,  6,  9, 49, 47, 35, 19,  9,
     3, 22, 24, 45, 57, 40, 57, 36,
   -18, 28, 19, 47, 31, 34, 39, 23,
   -16,-27, 15,  6,  9, 17, 10,  5,
   -22,-23,-30,-16,-16,-23,-36,-32,
   -33,-28,-22,-43, -5,-32,-20,-41
};
static constexpr int MG_KING_PST[64] = {
   -15, 36, 12,-54,  8,-28, 24, 14,
     1,  7, -8,-64,-43,-16,  9,  8,
   -14,-14,-22,-46,-44,-30,-15,-27,
   -49, -1,-27,-39,-46,-44,-33,-51,
   -17,-20,-12,-27,-30,-25,-14,-36,
    -9, 24,  2,-16,-20,  6, 22,-22,
    29, -1,-20, -7, -8, -4,-38,-29,
   -65, 23, 16,-15,-56,-34,  2, 13
};
static constexpr int EG_KING_PST[64] = {
   -74,-35,-18,-18,-11, 15,  4,-17,
   -12, 17, 14, 17, 17, 38, 23, 11,
    10, 17, 23, 15, 20, 45, 44, 13,
    -8, 22, 24, 27, 26, 33, 26,  3,
   -18, -4, 21, 24, 27, 23,  9,-11,
   -19, -3, 11, 21, 23, 16,  7, -9,
   -27,-11,  4, 13, 14,  4, -5,-17,
   -53,-34,-21,-11,-28,-14,-24,-43
};

// Combined tables: MG/EG value for [color][piece_type][square]
static int MG_TABLE[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
static int EG_TABLE[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];

static const int* MG_PST[PIECE_TYPE_NB] = {
    nullptr, MG_PAWN_PST, MG_KNIGHT_PST, MG_BISHOP_PST,
    MG_ROOK_PST, MG_QUEEN_PST, MG_KING_PST
};
static const int* EG_PST[PIECE_TYPE_NB] = {
    nullptr, EG_PAWN_PST, EG_KNIGHT_PST, EG_BISHOP_PST,
    EG_ROOK_PST, EG_QUEEN_PST, EG_KING_PST
};

void init_eval_tables() {
    for (int pt = PAWN; pt <= KING; pt++) {
        for (int sq = 0; sq < 64; sq++) {
            MG_TABLE[WHITE][pt][sq] = MG_VAL[pt] + MG_PST[pt][sq];
            EG_TABLE[WHITE][pt][sq] = EG_VAL[pt] + EG_PST[pt][sq];
            // Black: mirror rank
            int msq = sq ^ 56;
            MG_TABLE[BLACK][pt][sq] = MG_VAL[pt] + MG_PST[pt][msq];
            EG_TABLE[BLACK][pt][sq] = EG_VAL[pt] + EG_PST[pt][msq];
        }
    }
}

Evaluator::Evaluator() {
    clear_pawn_table();
}

void Evaluator::clear_pawn_table() {
    std::memset(pawn_table_, 0, sizeof(pawn_table_));
}

// ---- Pawn structure evaluation (cached) ------------------------------------

void Evaluator::eval_pawns(const Board& b,
                           int& mg_out, int& eg_out,
                           Bitboard passed[NCOLORS],
                           Bitboard attacks[NCOLORS]) {
    Key pkey = b.pieces[WHITE][PAWN] ^ b.pieces[BLACK][PAWN] ^ (Key(b.side_to_move) << 63);
    PawnEntry& pe = pawn_table_[pkey & (PAWN_TABLE_SIZE - 1)];

    if (pe.key == pkey) {
        mg_out = pe.mg;
        eg_out = pe.eg;
        passed[WHITE]  = pe.passed[WHITE];
        passed[BLACK]  = pe.passed[BLACK];
        attacks[WHITE] = pe.attacks[WHITE];
        attacks[BLACK] = pe.attacks[BLACK];
        return;
    }

    int mg = 0, eg = 0;

    for (int c = 0; c < NCOLORS; c++) {
        Color us   = Color(c);
        Color them = ~us;
        int sign   = (us == WHITE) ? 1 : -1;

        Bitboard our_pawns   = b.pieces[us][PAWN];
        Bitboard their_pawns = b.pieces[them][PAWN];

        // Pawn attack map
        Bitboard pawn_atk = 0;
        Bitboard tmp = our_pawns;
        while (tmp) {
            int sq = pop_lsb(tmp);
            pawn_atk |= PawnAttacks[us][sq];
        }
        attacks[c] = pawn_atk;

        // Passed pawn bonus per rank: {0,5,10,20,35,60,100,0} mg; {0,10,17,35,62,100,170,0} eg
        static constexpr int PASSED_MG[8] = {0, 5, 10, 20, 35, 60,100, 0};
        static constexpr int PASSED_EG[8] = {0,10, 17, 35, 62,100,170, 0};

        passed[c] = 0;
        tmp = our_pawns;
        while (tmp) {
            int sq = pop_lsb(tmp);
            int f = file_of(Square(sq));
            int r = rank_of(Square(sq));

            // Passed pawn: no enemy pawns in front on same or adjacent files
            if (!(BB_PASSED_PAWN_MASK[us][sq] & their_pawns)) {
                passed[c] |= sq_bb(Square(sq));
                int rel_r = (us == WHITE) ? r : 7 - r;
                mg += sign * PASSED_MG[rel_r];
                eg += sign * PASSED_EG[rel_r];
            }

            Bitboard file_bb = BB_FILES[f];
            Bitboard adj_bb  = BB_ADJACENT_FILES[f];

            // Doubled pawns (more than one pawn on same file)
            if (more_than_one(our_pawns & file_bb)) {
                mg += sign * (-10);
                eg += sign * (-20);
            }

            // Isolated pawns (no friendly pawns on adjacent files)
            if (!(our_pawns & adj_bb)) {
                mg += sign * (-15);
                eg += sign * (-20);
            }

            // Connected pawns (supported by another pawn)
            if (PawnAttacks[them][sq] & our_pawns) {
                mg += sign * 7;
                eg += sign * 5;
            }

            // Backward pawn: pawn can't advance without being attacked, and not passed
            // (simplified: pawn is behind its supports on adjacent files)
            if (!(our_pawns & BB_PASSED_PAWN_MASK[them][sq] & adj_bb)) {
                // No friendly pawn behind/adjacent — if stop square attacked by enemy pawn
                int stop_sq = (us == WHITE) ? sq + 8 : sq - 8;
                if (stop_sq >= 0 && stop_sq < 64) {
                    if (PawnAttacks[us][stop_sq] & their_pawns) {
                        mg += sign * (-10);
                        eg += sign * (-15);
                    }
                }
            }
        }
    }

    pe.key       = pkey;
    pe.mg        = mg;
    pe.eg        = eg;
    pe.passed[WHITE]  = passed[WHITE];
    pe.passed[BLACK]  = passed[BLACK];
    pe.attacks[WHITE] = attacks[WHITE];
    pe.attacks[BLACK] = attacks[BLACK];

    mg_out = mg;
    eg_out = eg;
}

// ---- Main evaluation -------------------------------------------------------

int Evaluator::evaluate(const Board& b) {
    int mg = 0, eg = 0;
    int phase = 0;

    // ---- Material and PST ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = b.pieces[c][pt];
            phase += popcount(bb) * PHASE_W[pt];
            while (bb) {
                int sq = pop_lsb(bb);
                mg += sign * MG_TABLE[c][pt][sq];
                eg += sign * EG_TABLE[c][pt][sq];
            }
        }
    }
    phase = std::min(phase, TOTAL_PHASE);

    // ---- Pawn structure (cached) ----
    Bitboard passed[NCOLORS] = {};
    Bitboard pawn_atk[NCOLORS] = {};
    int pmg = 0, peg = 0;
    eval_pawns(b, pmg, peg, passed, pawn_atk);
    mg += pmg;
    eg += peg;

    // ---- Bishop pair ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        if (more_than_one(b.pieces[c][BISHOP])) {
            mg += sign * 30;
            eg += sign * 50;
        }
    }

    // ---- Rook bonuses ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard rooks = b.pieces[c][ROOK];
        while (rooks) {
            int sq = pop_lsb(rooks);
            int f = file_of(Square(sq));

            bool no_own_pawn   = !(b.pieces[us][PAWN]   & BB_FILES[f]);
            bool no_their_pawn = !(b.pieces[them][PAWN] & BB_FILES[f]);

            if (no_own_pawn && no_their_pawn) {
                mg += sign * 25; eg += sign * 10; // open file
            } else if (no_own_pawn) {
                mg += sign * 12; eg += sign *  8; // semi-open
            }

            // Rook on 7th rank (relative)
            if (relative_rank(us, Square(sq)) == RANK_7) {
                mg += sign * 20; eg += sign * 40;
            }
        }
    }

    // ---- Knight bonuses ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard knights = b.pieces[c][KNIGHT];
        while (knights) {
            int sq = pop_lsb(knights);
            // Outpost: rank >= 5 (white) / <= 4 (black), supported by pawn, not attacked by enemy pawn
            Rank r = relative_rank(us, Square(sq));
            if (r >= RANK_5) {
                if (PawnAttacks[them][sq] & b.pieces[us][PAWN]) { // supported by own pawn
                    if (!(PawnAttacks[us][sq] & b.pieces[them][PAWN])) { // not attacked by enemy pawn
                        mg += sign * 25; eg += sign * 15;
                    }
                }
            }
        }
    }

    // ---- Mobility (safe squares not attacked by enemy pawns) ----
    static constexpr int MOB_MG[PIECE_TYPE_NB] = {0, 0, 4, 5, 2, 1, 0};
    static constexpr int MOB_EG[PIECE_TYPE_NB] = {0, 0, 4, 5, 4, 2, 0};

    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        Bitboard safe = ~pawn_atk[them]; // squares not attacked by enemy pawns
        for (int pt : {KNIGHT, BISHOP, ROOK, QUEEN}) {
            Bitboard pcs = b.pieces[c][pt];
            while (pcs) {
                int sq = pop_lsb(pcs);
                Bitboard att;
                switch (pt) {
                    case KNIGHT: att = KnightAttacks[sq]; break;
                    case BISHOP: att = bishop_attacks(Square(sq), b.all_occ); break;
                    case ROOK:   att = rook_attacks(Square(sq), b.all_occ); break;
                    default:     att = queen_attacks(Square(sq), b.all_occ); break;
                }
                int mob = popcount(att & safe & ~b.occupancy[c]);
                mg += sign * mob * MOB_MG[pt];
                eg += sign * mob * MOB_EG[pt];
            }
        }
    }

    // ---- Pawn threats to enemy pieces ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        Bitboard threats = pawn_atk[c] & b.occupancy[them];
        while (threats) {
            int sq = pop_lsb(threats);
            PieceType pt = type_of(b.board_sq[sq]);
            switch (pt) {
                case KNIGHT: case BISHOP: mg += sign * 18; eg += sign * 12; break;
                case ROOK:               mg += sign * 28; eg += sign * 18; break;
                case QUEEN:              mg += sign * 45; eg += sign * 30; break;
                default: break;
            }
        }
    }

    // ---- King safety (simplified) ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        Square ksq = b.king_sq[c];

        Bitboard king_zone = KingAttacks[ksq] | sq_bb(ksq);
        // Extend one more rank toward center
        if (c == WHITE) king_zone |= shift<NORTH>(KingAttacks[ksq]);
        else            king_zone |= shift<SOUTH>(KingAttacks[ksq]);

        int attack_units = 0;
        for (int pt : {KNIGHT, BISHOP, ROOK, QUEEN}) {
            Bitboard pcs = b.pieces[them][pt];
            while (pcs) {
                int sq = pop_lsb(pcs);
                Bitboard att;
                switch (pt) {
                    case KNIGHT: att = KnightAttacks[sq]; break;
                    case BISHOP: att = bishop_attacks(Square(sq), b.all_occ); break;
                    case ROOK:   att = rook_attacks(Square(sq), b.all_occ); break;
                    default:     att = queen_attacks(Square(sq), b.all_occ); break;
                }
                if (att & king_zone) {
                    static constexpr int UNIT[PIECE_TYPE_NB] = {0,0,2,2,3,5,0};
                    attack_units += UNIT[pt];
                }
            }
        }
        attack_units = std::min(attack_units, 15);
        static constexpr int SAFETY_TABLE[16] = {
            0, 0, 10, 25, 40, 60, 80, 95, 105, 110, 112, 114, 115, 116, 117, 118
        };
        mg -= sign * SAFETY_TABLE[attack_units];
    }

    // ---- King pawn shelter -------------------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Square ksq = b.king_sq[c];
        File   kf  = file_of(ksq);
        Rank   kr  = rank_of(ksq);

        // Only apply shelter bonus when king is castled or near edge
        if (kf <= FILE_C || kf >= FILE_F) {
            // Check pawns on the 3 files around king
            for (int df = -1; df <= 1; df++) {
                int f = kf + df;
                if (f < FILE_A || f > FILE_H) continue;
                Bitboard file_pawns = b.pieces[us][PAWN] & BB_FILES[f];

                // Find closest friendly pawn on this file (in front of king)
                if (us == WHITE) {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[WHITE][kr];
                    if (!in_front) {
                        // No pawn — open file near king is very bad
                        mg -= sign * (df == 0 ? 20 : 10);
                    } else {
                        Rank pawn_rank = rank_of(Square(lsb(in_front)));
                        int dist = pawn_rank - kr;
                        // Pawn close to king is good
                        if (dist == 1) { mg += sign * 15; }
                        else if (dist == 2) { mg += sign *  7; }
                    }
                } else {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[BLACK][kr];
                    if (!in_front) {
                        mg -= sign * (df == 0 ? 20 : 10);
                    } else {
                        Rank pawn_rank = rank_of(Square(msb(in_front)));
                        int dist = kr - pawn_rank;
                        if (dist == 1) { mg += sign * 15; }
                        else if (dist == 2) { mg += sign *  7; }
                    }
                }
            }
        }
    }

    // ---- Rook behind passed pawn ----------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Color them = ~us;
        Bitboard rooks = b.pieces[us][ROOK];
        while (rooks) {
            Square rsq = Square(pop_lsb(rooks));
            int f = file_of(rsq);
            // Check if there's a passed pawn on the same file
            Bitboard file_passers = passed[us] & BB_FILES[f];
            if (file_passers) {
                // Rook is behind the passed pawn (supporting it from behind)
                if (us == WHITE) {
                    if (rank_of(rsq) < rank_of(Square(lsb(file_passers)))) {
                        mg += sign * 15; eg += sign * 25;
                    }
                } else {
                    if (rank_of(rsq) > rank_of(Square(msb(file_passers)))) {
                        mg += sign * 15; eg += sign * 25;
                    }
                }
            }
            // Enemy rook behind our passed pawn is bad
            Bitboard enemy_rooks = b.pieces[them][ROOK];
            Bitboard enemy_same_file = enemy_rooks & BB_FILES[f];
            while (enemy_same_file) {
                Square er = Square(pop_lsb(enemy_same_file));
                if (file_passers) {
                    if (us == WHITE) {
                        if (rank_of(er) < rank_of(Square(lsb(file_passers)))) {
                            mg -= sign * 10; eg -= sign * 20;
                        }
                    } else {
                        if (rank_of(er) > rank_of(Square(msb(file_passers)))) {
                            mg -= sign * 10; eg -= sign * 20;
                        }
                    }
                }
            }
        }
    }

    // ---- Hanging pieces (attacked and not defended) ----------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        // Non-pawn pieces
        Bitboard pieces_bb = b.occupancy[us] & ~b.pieces[us][PAWN] & ~b.pieces[us][KING];
        while (pieces_bb) {
            Square sq = Square(pop_lsb(pieces_bb));
            // Is it attacked by enemy?
            if (!b.attackers_to(sq, b.all_occ, them)) continue;
            // Is it defended by us?
            if ( b.attackers_to(sq, b.all_occ, us))   continue;
            // Hanging piece
            PieceType pt = type_of(b.board_sq[sq]);
            static constexpr int HANG_PEN[PIECE_TYPE_NB] = {0, 0, 45, 45, 60, 80, 0};
            mg -= sign * HANG_PEN[pt];
            eg -= sign * HANG_PEN[pt];
        }
    }
    if (phase <= 6) {
        int score_approx = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
        if (std::abs(score_approx) > 200) {
            Color winning = (score_approx > 0) ? WHITE : BLACK;
            Color losing  = ~winning;
            int sign = (winning == WHITE) ? 1 : -1;
            Square wksq = b.king_sq[winning];
            Square lksq = b.king_sq[losing];
            // Push losing king to corner (distance from center)
            int lk_center = std::max(3 - file_of(lksq), file_of(lksq) - 4)
                          + std::max(3 - rank_of(lksq), rank_of(lksq) - 4);
            // Bring kings close
            int king_dist = KING_DIST[wksq][lksq];
            eg += sign * (5 * lk_center + (14 - king_dist) * 4);
        }
    }

    // ---- Tempo bonus ----
    int tempo = (b.side_to_move == WHITE) ? 10 : -10;
    mg += tempo;

    // ---- Taper and return ----
    int score = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    // ---- Endgame scaling -----------------------------------------------
    // Scale down drawn/drawish endgames to avoid falsely confident eval.

    // Opposite-color bishops: if both sides have exactly 1 bishop of opposite color
    // (and few other pieces), the position is very drawish even with pawn advantage.
    if (b.pieces[WHITE][BISHOP] && b.pieces[BLACK][BISHOP]
        && !more_than_one(b.pieces[WHITE][BISHOP])
        && !more_than_one(b.pieces[BLACK][BISHOP])) {

        bool wb_light = b.pieces[WHITE][BISHOP] & 0x55AA55AA55AA55AAULL;
        bool bb_light = b.pieces[BLACK][BISHOP] & 0x55AA55AA55AA55AAULL;
        if (wb_light != bb_light) {
            // Opposite color bishops
            // Count non-pawn, non-bishop material on both sides
            int w_np = popcount(b.pieces[WHITE][KNIGHT]) + popcount(b.pieces[WHITE][ROOK]) + popcount(b.pieces[WHITE][QUEEN]);
            int b_np = popcount(b.pieces[BLACK][KNIGHT]) + popcount(b.pieces[BLACK][ROOK]) + popcount(b.pieces[BLACK][QUEEN]);
            if (w_np == 0 && b_np == 0) {
                // Pure OCB endgame: scale significantly
                score = score / 2;
            }
        }
    }

    // No pawns + insufficient winning material → scale toward draw
    bool w_pawns = b.pieces[WHITE][PAWN] != 0;
    bool b_pawns = b.pieces[BLACK][PAWN] != 0;
    if (!w_pawns && !b_pawns) {
        // Pure piece endgame: scale based on how much stronger the winning side is
        int w_mat = popcount(b.pieces[WHITE][KNIGHT]) * 300
                  + popcount(b.pieces[WHITE][BISHOP]) * 300
                  + popcount(b.pieces[WHITE][ROOK])   * 500
                  + popcount(b.pieces[WHITE][QUEEN])  * 900;
        int b_mat = popcount(b.pieces[BLACK][KNIGHT]) * 300
                  + popcount(b.pieces[BLACK][BISHOP]) * 300
                  + popcount(b.pieces[BLACK][ROOK])   * 500
                  + popcount(b.pieces[BLACK][QUEEN])  * 900;
        int diff = std::abs(w_mat - b_mat);
        if (diff < 400) {
            // Near-equal piece endgame with no pawns: very drawish
            score = score * diff / 400;
        }
    }

    // Return from side-to-move perspective
    return (b.side_to_move == WHITE) ? score : -score;
}
