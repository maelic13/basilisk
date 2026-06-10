#include "eval.h"
#include <algorithm>
#include <cstring>

// Global evaluation parameters (defaults in EvalParams.h).
// Call init_eval_tables(g_eval_params) again after changing these.
EvalParams g_eval_params;

// Phase weights (frozen; not in EvalParams)
static constexpr int PHASE_W[PIECE_TYPE_NB] = {0, 0, 1, 1, 2, 4, 0};
static constexpr int TOTAL_PHASE = 24;

// Runtime tables baked from g_eval_params in init_eval_tables().
static int MG_TABLE[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];
static int EG_TABLE[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];

static Square forward_square(Color c, Square sq) {
    const int to = int(sq) + (c == WHITE ? 8 : -8);
    return (to >= 0 && to < 64) ? Square(to) : SQ_NONE;
}

void init_eval_tables(const EvalParams& p) {
    for (int pt = PAWN; pt <= KING; pt++) {
        for (int sq = 0; sq < 64; sq++) {
            MG_TABLE[WHITE][pt][sq] = p.mg_val[pt] + p.pst_mg[pt - 1][sq];
            EG_TABLE[WHITE][pt][sq] = p.eg_val[pt] + p.pst_eg[pt - 1][sq];
            // Black: mirror rank
            int msq = sq ^ 56;
            MG_TABLE[BLACK][pt][sq] = p.mg_val[pt] + p.pst_mg[pt - 1][msq];
            EG_TABLE[BLACK][pt][sq] = p.eg_val[pt] + p.pst_eg[pt - 1][msq];
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
    Key pkey = b.pawn_key;
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

    const EvalParams& p = g_eval_params;
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
                mg += sign * p.passed_mg[rel_r];
                eg += sign * p.passed_eg[rel_r];
                if (PawnAttacks[them][sq] & our_pawns) {
                    mg += sign * p.pass_supp_mg;
                    eg += sign * (p.pass_supp_eg_base + rel_r * p.pass_supp_eg_rank);
                }
            } else {
                const int rel_r = (us == WHITE) ? r : 7 - r;
                Bitboard adj_bb = BB_ADJACENT_FILES[f];
                if (rel_r >= 3
                    && (PawnAttacks[them][sq] & our_pawns)
                    && !(their_pawns & adj_bb & BB_FORWARD_RANKS[us][r])) {
                    mg += sign * p.cand_mg;
                    eg += sign * p.cand_eg;
                }
            }

            Bitboard file_bb = BB_FILES[f];
            Bitboard adj_bb  = BB_ADJACENT_FILES[f];

            // Doubled pawns
            if (more_than_one(our_pawns & file_bb)) {
                mg += sign * p.doubled_mg;
                eg += sign * p.doubled_eg;
            }

            // Isolated pawns
            if (!(our_pawns & adj_bb)) {
                mg += sign * p.isolated_mg;
                eg += sign * p.isolated_eg;
            }

            // Connected pawns (supported by another pawn)
            if (PawnAttacks[them][sq] & our_pawns) {
                mg += sign * p.connected_mg;
                eg += sign * p.connected_eg;
            }

            // Backward pawn
            if (!(our_pawns & BB_PASSED_PAWN_MASK[them][sq] & adj_bb)) {
                int stop_sq = (us == WHITE) ? sq + 8 : sq - 8;
                if (stop_sq >= 0 && stop_sq < 64) {
                    if (PawnAttacks[us][stop_sq] & their_pawns) {
                        mg += sign * p.backward_mg;
                        eg += sign * p.backward_eg;
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
    const EvalParams& p = g_eval_params;
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

    // ---- Dynamic passed-pawn terms (non-pawn occupancy; outside pawn hash) --
    for (int c = 0; c < NCOLORS; c++) {
        Color us   = Color(c);
        Color them = ~us;
        int sign   = (us == WHITE) ? 1 : -1;
        Bitboard pp = passed[c];
        while (pp) {
            Square psq = Square(pop_lsb(pp));
            Square stop = forward_square(us, psq);
            if (stop == SQ_NONE || (b.all_occ & sq_bb(stop)))
                continue;

            int rel_r = relative_rank(us, psq);
            mg += sign * (rel_r * p.pass_free_mg);
            eg += sign * (rel_r * p.pass_free_eg);

            if (!b.is_attacked_by(stop, b.all_occ, them))
                eg += sign * (rel_r * p.pass_safe_eg);
        }
    }

    // ---- Bishop pair ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        if (more_than_one(b.pieces[c][BISHOP])) {
            mg += sign * p.bp_mg;
            eg += sign * p.bp_eg;
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
                mg += sign * p.rook_open_mg;
                eg += sign * p.rook_open_eg;
            } else if (no_own_pawn) {
                mg += sign * p.rook_semi_mg;
                eg += sign * p.rook_semi_eg;
            }

            // Rook on 7th rank (relative)
            if (relative_rank(us, Square(sq)) == RANK_7) {
                mg += sign * p.rook_7th_mg;
                eg += sign * p.rook_7th_eg;
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
            Rank r = relative_rank(us, Square(sq));
            if (r >= RANK_5) {
                if (PawnAttacks[them][sq] & b.pieces[us][PAWN]) {
                    if (!(PawnAttacks[us][sq] & b.pieces[them][PAWN])) {
                        mg += sign * p.knight_outpost_mg;
                        eg += sign * p.knight_outpost_eg;
                    }
                }
            }
        }
    }

    // ---- Mobility (safe squares not attacked by enemy pawns) ----
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        Bitboard safe = ~pawn_atk[them];
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
                mg += sign * mob * p.mob_mg[pt];
                eg += sign * mob * p.mob_eg[pt];
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
                case KNIGHT: case BISHOP:
                    mg += sign * p.threat_minor_mg;
                    eg += sign * p.threat_minor_eg;
                    break;
                case ROOK:
                    mg += sign * p.threat_rook_mg;
                    eg += sign * p.threat_rook_eg;
                    break;
                case QUEEN:
                    mg += sign * p.threat_queen_mg;
                    eg += sign * p.threat_queen_eg;
                    break;
                default: break;
            }
        }
    }

    // ---- King safety -------------------------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color them = ~Color(c);
        Square ksq = b.king_sq[c];

        Bitboard king_zone = KingAttacks[ksq] | sq_bb(ksq);
        if (c == WHITE) king_zone |= shift<NORTH>(KingAttacks[ksq]);
        else            king_zone |= shift<SOUTH>(KingAttacks[ksq]);

        int attack_units = 0;
        int n_attackers  = 0;
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
                Bitboard zone_hits = att & king_zone;
                if (zone_hits) {
                    n_attackers++;
                    attack_units += p.ks_unit[pt] + popcount(zone_hits) / 2;
                }
            }
        }
        if (n_attackers >= 2) attack_units += p.ks_coord_bonus;
        if (n_attackers == 0) attack_units = 0;

        if (!b.pieces[them][QUEEN]) attack_units = attack_units * 2 / 3;

        {
            Bitboard king_files = BB_FILES[file_of(ksq)];
            if (!(b.pieces[c][PAWN] & king_files))
                attack_units += p.ks_open_file;
        }

        attack_units = std::min(attack_units, 24);
        mg -= sign * p.safety_table[attack_units];
    }

    // ---- King pawn shelter -------------------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Square ksq = b.king_sq[c];
        File   kf  = file_of(ksq);
        Rank   kr  = rank_of(ksq);

        if (kf <= FILE_C || kf >= FILE_F) {
            for (int df = -1; df <= 1; df++) {
                int f = kf + df;
                if (f < FILE_A || f > FILE_H) continue;
                Bitboard file_pawns = b.pieces[us][PAWN] & BB_FILES[f];

                if (us == WHITE) {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[WHITE][kr];
                    if (!in_front) {
                        mg -= sign * (df == 0 ? p.shelter_missing_center
                                              : p.shelter_missing_flank);
                    } else {
                        Rank pawn_rank = rank_of(Square(lsb(in_front)));
                        int dist = pawn_rank - kr;
                        if (dist == 1)      { mg += sign * p.shelter_close1; }
                        else if (dist == 2) { mg += sign * p.shelter_close2; }
                    }
                } else {
                    Bitboard in_front = file_pawns & BB_FORWARD_RANKS[BLACK][kr];
                    if (!in_front) {
                        mg -= sign * (df == 0 ? p.shelter_missing_center
                                              : p.shelter_missing_flank);
                    } else {
                        Rank pawn_rank = rank_of(Square(msb(in_front)));
                        int dist = kr - pawn_rank;
                        if (dist == 1)      { mg += sign * p.shelter_close1; }
                        else if (dist == 2) { mg += sign * p.shelter_close2; }
                    }
                }
            }
        }

        Bitboard storm_files = 0;
        for (int df = -1; df <= 1; df++) {
            int f = kf + df;
            if (f >= FILE_A && f <= FILE_H)
                storm_files |= BB_FILES[f];
        }

        Bitboard storm = b.pieces[~us][PAWN] & storm_files;
        while (storm) {
            Square psq = Square(pop_lsb(storm));
            int rel_r = relative_rank(~us, psq);
            if (rel_r >= 3) {
                int weight = (file_of(psq) == kf) ? p.storm_weight_kf
                                                   : p.storm_weight_adj;
                mg -= sign * rel_r * weight;
            }
        }
    }

    // ---- Rook behind passed pawn -------------------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us = Color(c);
        Color them = ~us;
        Bitboard rooks = b.pieces[us][ROOK];
        while (rooks) {
            Square rsq = Square(pop_lsb(rooks));
            int f = file_of(rsq);
            Bitboard file_passers = passed[us] & BB_FILES[f];
            if (file_passers) {
                if (us == WHITE) {
                    if (rank_of(rsq) < rank_of(Square(lsb(file_passers)))) {
                        mg += sign * p.rook_behind_passer_mg;
                        eg += sign * p.rook_behind_passer_eg;
                    }
                } else {
                    if (rank_of(rsq) > rank_of(Square(msb(file_passers)))) {
                        mg += sign * p.rook_behind_passer_mg;
                        eg += sign * p.rook_behind_passer_eg;
                    }
                }
            }
            Bitboard enemy_rooks = b.pieces[them][ROOK];
            Bitboard enemy_same_file = enemy_rooks & BB_FILES[f];
            while (enemy_same_file) {
                Square er = Square(pop_lsb(enemy_same_file));
                if (file_passers) {
                    if (us == WHITE) {
                        if (rank_of(er) < rank_of(Square(lsb(file_passers)))) {
                            mg -= sign * p.enemy_rook_passer_mg;
                            eg -= sign * p.enemy_rook_passer_eg;
                        }
                    } else {
                        if (rank_of(er) > rank_of(Square(msb(file_passers)))) {
                            mg -= sign * p.enemy_rook_passer_mg;
                            eg -= sign * p.enemy_rook_passer_eg;
                        }
                    }
                }
            }
        }
    }

    // ---- Hanging pieces (attacked and not defended) ------------------------
    for (int c = 0; c < NCOLORS; c++) {
        int sign = (c == WHITE) ? 1 : -1;
        Color us   = Color(c);
        Color them = ~us;
        Bitboard pieces_bb = b.occupancy[us] & ~b.pieces[us][PAWN] & ~b.pieces[us][KING];
        while (pieces_bb) {
            Square sq = Square(pop_lsb(pieces_bb));
            if (!b.is_attacked_by(sq, b.all_occ, them)) continue;
            if ( b.is_attacked_by(sq, b.all_occ, us))   continue;
            PieceType pt = type_of(b.board_sq[sq]);
            mg -= sign * p.hang_pen[pt];
            eg -= sign * p.hang_pen[pt];
        }
    }

    // ---- Passed pawn king proximity (endgame) ------------------------------
    for (int c = 0; c < NCOLORS; c++) {
        Color us = Color(c);
        Color them = ~us;
        int sign = (us == WHITE) ? 1 : -1;
        Bitboard pp = passed[us];
        while (pp) {
            Square psq = Square(pop_lsb(pp));
            int rel_r = (us == WHITE) ? rank_of(psq) : (RANK_8 - rank_of(psq));
            int own_dist = KING_DIST[b.king_sq[us]][psq];
            int opp_dist = KING_DIST[b.king_sq[them]][psq];
            eg += sign * (opp_dist - own_dist) * (p.prox_base + rel_r);
        }
    }

    // ---- Space evaluation (center control in middlegame) -------------------
    {
        const Bitboard center_files =
            BB_FILES[FILE_C] | BB_FILES[FILE_D] | BB_FILES[FILE_E] | BB_FILES[FILE_F];
        const Bitboard white_space_ranks =
            BB_RANKS[RANK_2] | BB_RANKS[RANK_3] | BB_RANKS[RANK_4];
        const Bitboard black_space_ranks =
            BB_RANKS[RANK_5] | BB_RANKS[RANK_6] | BB_RANKS[RANK_7];

        Bitboard wspace = (center_files & white_space_ranks) & ~b.pieces[WHITE][PAWN] & ~pawn_atk[BLACK];
        Bitboard bspace = (center_files & black_space_ranks) & ~b.pieces[BLACK][PAWN] & ~pawn_atk[WHITE];
        mg += (popcount(wspace) - popcount(bspace)) * p.space_mg;
    }

    // ---- Trapped bishop detection ------------------------------------------
    if (phase < TOTAL_PHASE / 2) {
        for (int c = 0; c < NCOLORS; c++) {
            int sign = (c == WHITE) ? 1 : -1;
            Bitboard bbs = b.pieces[c][BISHOP];
            while (bbs) {
                Square bsq = Square(pop_lsb(bbs));
                Bitboard moves = bishop_attacks(bsq, b.all_occ) & ~b.occupancy[c];
                if (moves == 0) {
                    mg -= sign * p.trapped_mg;
                    eg -= sign * p.trapped_eg;
                }
            }
        }
    }

    // ---- Endgame mate-drive (frozen) ---------------------------------------
    if (phase <= 6) {
        int score_approx = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;
        if (std::abs(score_approx) > 200) {
            Color winning = (score_approx > 0) ? WHITE : BLACK;
            Color losing  = ~winning;
            int sign = (winning == WHITE) ? 1 : -1;
            Square wksq = b.king_sq[winning];
            Square lksq = b.king_sq[losing];
            int lk_center = std::max(3 - file_of(lksq), file_of(lksq) - 4)
                          + std::max(3 - rank_of(lksq), rank_of(lksq) - 4);
            int king_dist = KING_DIST[wksq][lksq];
            eg += sign * (5 * lk_center + (14 - king_dist) * 4);
        }
    }

    // ---- Tempo bonus -------------------------------------------------------
    mg += (b.side_to_move == WHITE) ? p.tempo : -p.tempo;

    // ---- Taper and return --------------------------------------------------
    int score = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    // ---- Draw scaling (frozen) ---------------------------------------------
    {
        constexpr Bitboard DARK_SQ = 0x55AA55AA55AA55AAULL;
        bool wb1 = !more_than_one(b.pieces[WHITE][BISHOP]) && b.pieces[WHITE][BISHOP];
        bool bb1 = !more_than_one(b.pieces[BLACK][BISHOP]) && b.pieces[BLACK][BISHOP];
        if (wb1 && bb1) {
            bool wb_dark = (b.pieces[WHITE][BISHOP] & DARK_SQ) != 0;
            bool bb_dark = (b.pieces[BLACK][BISHOP] & DARK_SQ) != 0;
            if (wb_dark != bb_dark) {
                int total_pawns = popcount(b.pieces[WHITE][PAWN] | b.pieces[BLACK][PAWN]);
                int scale = 32 + total_pawns * 4;
                score = score * scale / 48;
            }
        }
    }
    {
        auto only_king = [&](Color c) {
            return b.occupancy[c] == sq_bb(b.king_sq[c]);
        };
        auto only_knights = [&](Color c, int n) {
            return !b.pieces[c][PAWN] && !b.pieces[c][BISHOP]
                && !b.pieces[c][ROOK] && !b.pieces[c][QUEEN]
                && popcount(b.pieces[c][KNIGHT]) == n;
        };
        if (only_king(WHITE) && only_knights(BLACK, 2)) score = 0;
        if (only_king(BLACK) && only_knights(WHITE, 2)) score = 0;
    }

    if (b.halfmove_clock > 0)
        score = score * std::max(0, 100 - b.halfmove_clock) / 100;

    return (b.side_to_move == WHITE) ? score : -score;
}
