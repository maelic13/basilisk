// Board.cpp — full board implementation
#include "Board.h"
#include "Constants.h"
#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>

// ---- Internal helpers -------------------------------------------------------

void Board::put_piece(Color c, PieceType pt, Square sq) {
    Bitboard bb = sq_bb(sq);
    pieces[c][pt]  |= bb;
    occupancy[c]   |= bb;
    all_occ        |= bb;
    board_sq[sq]    = make_piece(c, pt);
    hash           ^= Zobrist::PieceKeys[c][pt][sq];
    if (pt == KING) king_sq[c] = sq;
}

void Board::remove_piece(Square sq) {
    Piece p = board_sq[sq];
    if (p == NO_PIECE) return;
    Color c     = color_of(p);
    PieceType pt = type_of(p);
    Bitboard bb  = sq_bb(sq);
    pieces[c][pt]  &= ~bb;
    occupancy[c]   &= ~bb;
    all_occ        &= ~bb;
    board_sq[sq]    = NO_PIECE;
    hash           ^= Zobrist::PieceKeys[c][pt][sq];
}

void Board::move_piece(Square from, Square to) {
    Piece p = board_sq[from];
    Color c     = color_of(p);
    PieceType pt = type_of(p);
    Bitboard mask = sq_bb(from) | sq_bb(to);
    pieces[c][pt]  ^= mask;
    occupancy[c]   ^= mask;
    all_occ        ^= mask;
    board_sq[from]  = NO_PIECE;
    board_sq[to]    = p;
    hash           ^= Zobrist::PieceKeys[c][pt][from]
                    ^ Zobrist::PieceKeys[c][pt][to];
    if (pt == KING) king_sq[c] = to;
}

Key Board::compute_hash() const {
    Key h = 0;
    for (int c = 0; c < NCOLORS; c++)
        for (int pt = PAWN; pt <= KING; pt++) {
            Bitboard bb = pieces[c][pt];
            while (bb) {
                int sq = pop_lsb(bb);
                h ^= Zobrist::PieceKeys[c][pt][sq];
            }
        }
    if (side_to_move == BLACK) h ^= Zobrist::SideKey;
    h ^= Zobrist::CastlingKeys[castling_rights];
    if (ep_sq != SQ_NONE) h ^= Zobrist::EpKeys[file_of(ep_sq)];
    return h;
}

// ---- Constructor & FEN parsing ---------------------------------------------

Board::Board() {
    set_fen(std::string(startPosition));
}

void Board::set_fen(const std::string& fen) {
    // Clear board
    for (int c = 0; c < NCOLORS; c++)
        for (int pt = 0; pt < PIECE_TYPE_NB; pt++)
            pieces[c][pt] = 0;
    for (int c = 0; c < NCOLORS; c++) occupancy[c] = 0;
    all_occ = 0;
    for (int s = 0; s < SQUARE_NB; s++) board_sq[s] = NO_PIECE;
    history.clear();
    ply = 0;

    std::istringstream ss(fen);
    std::string token;

    // Piece placement
    ss >> token;
    int rank = 7, file = 0;
    for (char ch : token) {
        if (ch == '/') { rank--; file = 0; }
        else if (ch >= '1' && ch <= '8') { file += ch - '0'; }
        else {
            Color c;
            PieceType pt;
            switch (ch) {
                case 'P': c=WHITE; pt=PAWN;   break;
                case 'N': c=WHITE; pt=KNIGHT; break;
                case 'B': c=WHITE; pt=BISHOP; break;
                case 'R': c=WHITE; pt=ROOK;   break;
                case 'Q': c=WHITE; pt=QUEEN;  break;
                case 'K': c=WHITE; pt=KING;   break;
                case 'p': c=BLACK; pt=PAWN;   break;
                case 'n': c=BLACK; pt=KNIGHT; break;
                case 'b': c=BLACK; pt=BISHOP; break;
                case 'r': c=BLACK; pt=ROOK;   break;
                case 'q': c=BLACK; pt=QUEEN;  break;
                case 'k': c=BLACK; pt=KING;   break;
                default: file++; continue;
            }
            Square sq = make_square(File(file), Rank(rank));
            // Put piece without hash (we compute hash from scratch at end)
            Bitboard bb = sq_bb(sq);
            pieces[c][pt] |= bb;
            occupancy[c]  |= bb;
            all_occ       |= bb;
            board_sq[sq]   = make_piece(c, pt);
            if (pt == KING) king_sq[c] = sq;
            file++;
        }
    }

    // Side to move
    ss >> token;
    side_to_move = (token == "w") ? WHITE : BLACK;

    // Castling rights
    ss >> token;
    castling_rights = NO_CASTLING;
    for (char ch : token) {
        switch (ch) {
            case 'K': castling_rights |= WK_CASTLE; break;
            case 'Q': castling_rights |= WQ_CASTLE; break;
            case 'k': castling_rights |= BK_CASTLE; break;
            case 'q': castling_rights |= BQ_CASTLE; break;
        }
    }

    // En passant
    ss >> token;
    ep_sq = SQ_NONE;
    if (token != "-" && token.size() >= 2) {
        int f = token[0] - 'a';
        int r = token[1] - '1';
        if (f >= 0 && f < 8 && r >= 0 && r < 8)
            ep_sq = make_square(File(f), Rank(r));
    }

    // Halfmove clock
    halfmove_clock = 0;
    ss >> halfmove_clock;

    // Fullmove number
    fullmove_number = 1;
    ss >> fullmove_number;

    // Compute hash from scratch
    hash = compute_hash();
}

std::string Board::get_fen() const {
    static const char piece_chars[] = ".PNBRQK..pnbrqk";
    std::string fen;

    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Square sq = make_square(File(file), Rank(rank));
            Piece p = board_sq[sq];
            if (p == NO_PIECE) {
                empty++;
            } else {
                if (empty) { fen += char('0'+empty); empty = 0; }
                fen += piece_chars[p];
            }
        }
        if (empty) fen += char('0'+empty);
        if (rank > 0) fen += '/';
    }

    fen += ' ';
    fen += (side_to_move == WHITE) ? 'w' : 'b';
    fen += ' ';

    std::string castle;
    if (castling_rights & WK_CASTLE) castle += 'K';
    if (castling_rights & WQ_CASTLE) castle += 'Q';
    if (castling_rights & BK_CASTLE) castle += 'k';
    if (castling_rights & BQ_CASTLE) castle += 'q';
    fen += castle.empty() ? "-" : castle;
    fen += ' ';

    if (ep_sq == SQ_NONE) {
        fen += '-';
    } else {
        fen += char('a' + file_of(ep_sq));
        fen += char('1' + rank_of(ep_sq));
    }

    fen += ' ';
    fen += std::to_string(halfmove_clock);
    fen += ' ';
    fen += std::to_string(fullmove_number);

    return fen;
}

// ---- make_move / unmake_move ------------------------------------------------

void Board::make_move(Move m) {
    UndoInfo ui;
    ui.hash     = hash;
    ui.ep_sq    = ep_sq;
    ui.castling = castling_rights;
    ui.halfmove = halfmove_clock;
    ui.captured = NO_PIECE;
    history.push_back(ui);

    Square from = from_sq(m);
    Square to   = to_sq(m);
    MoveType mt = move_type(m);
    Color us    = side_to_move;
    Color them  = ~us;

    // Remove old castling and ep from hash
    hash ^= Zobrist::CastlingKeys[castling_rights];
    if (ep_sq != SQ_NONE) hash ^= Zobrist::EpKeys[file_of(ep_sq)];
    ep_sq = SQ_NONE;

    halfmove_clock++;

    if (mt == EN_PASSANT) {
        // Capture the EP pawn
        Square ep_pawn = make_square(file_of(to), rank_of(from));
        history.back().captured = board_sq[ep_pawn];
        remove_piece(ep_pawn);
        move_piece(from, to);
        halfmove_clock = 0;
    }
    else if (mt == CASTLING) {
        // King moves, then rook
        // Determine rook squares
        bool king_side = (to > from);
        Square rook_from = king_side ? make_square(FILE_H, rank_of(from))
                                     : make_square(FILE_A, rank_of(from));
        Square rook_to   = king_side ? make_square(FILE_F, rank_of(from))
                                     : make_square(FILE_D, rank_of(from));
        move_piece(from, to);
        move_piece(rook_from, rook_to);
    }
    else {
        Piece captured = board_sq[to];
        if (captured != NO_PIECE) {
            history.back().captured = captured;
            remove_piece(to);
            halfmove_clock = 0;
        }

        if (mt == PROMOTION) {
            remove_piece(from);
            put_piece(us, promo_type(m), to);
            halfmove_clock = 0;
        } else {
            move_piece(from, to);
            // Pawn-specific
            PieceType pt = type_of(board_sq[to]);
            if (pt == PAWN) {
                halfmove_clock = 0;
                // Double push: set EP square
                int diff = int(to) - int(from);
                if (diff == 16 || diff == -16) {
                    Square ep = Square(int(from) + (diff / 2));
                    // Only set EP if opponent pawn can actually capture
                    if (PawnAttacks[us][ep] & pieces[them][PAWN]) {
                        ep_sq = ep;
                        hash ^= Zobrist::EpKeys[file_of(ep_sq)];
                    }
                }
            }
        }
    }

    // Update castling rights
    castling_rights &= CASTLING_MASK[from] & CASTLING_MASK[to];

    if (side_to_move == BLACK) fullmove_number++;
    side_to_move = them;
    hash ^= Zobrist::SideKey;
    hash ^= Zobrist::CastlingKeys[castling_rights];

    ply++;
}

void Board::unmake_move(Move m) {
    assert(!history.empty());
    UndoInfo ui = history.back();
    history.pop_back();

    // Restore hash-tracked state from undo info
    // We'll just re-derive the hash instead of tracking incremental
    ep_sq           = ui.ep_sq;
    castling_rights = ui.castling;
    halfmove_clock  = ui.halfmove;

    ply--;
    if (side_to_move == WHITE) fullmove_number--;
    side_to_move = ~side_to_move;
    Color us   = side_to_move;
    Color them = ~us;

    Square from = from_sq(m);
    Square to   = to_sq(m);
    MoveType mt = move_type(m);

    if (mt == EN_PASSANT) {
        move_piece(to, from);
        Square ep_pawn = make_square(file_of(to), rank_of(from));
        put_piece(them, PAWN, ep_pawn);
    }
    else if (mt == CASTLING) {
        bool king_side = (to > from);
        Square rook_from = king_side ? make_square(FILE_H, rank_of(from))
                                     : make_square(FILE_A, rank_of(from));
        Square rook_to   = king_side ? make_square(FILE_F, rank_of(from))
                                     : make_square(FILE_D, rank_of(from));
        move_piece(to, from);
        move_piece(rook_to, rook_from);
    }
    else if (mt == PROMOTION) {
        remove_piece(to);
        put_piece(us, PAWN, from);
        if (ui.captured != NO_PIECE)
            put_piece(them, type_of(ui.captured), to);
    }
    else {
        move_piece(to, from);
        if (ui.captured != NO_PIECE)
            put_piece(them, type_of(ui.captured), to);
    }

    // Recompute hash from scratch (avoids complex incremental undo)
    hash = compute_hash();
}

void Board::make_null_move() {
    UndoInfo ui;
    ui.hash     = hash;
    ui.ep_sq    = ep_sq;
    ui.castling = castling_rights;
    ui.halfmove = halfmove_clock;
    ui.captured = NO_PIECE;
    history.push_back(ui);

    hash ^= Zobrist::SideKey;
    if (ep_sq != SQ_NONE) {
        hash ^= Zobrist::EpKeys[file_of(ep_sq)];
        ep_sq = SQ_NONE;
    }
    halfmove_clock++;
    side_to_move = ~side_to_move;
    ply++;
}

void Board::unmake_null_move() {
    assert(!history.empty());
    UndoInfo ui = history.back();
    history.pop_back();

    ep_sq           = ui.ep_sq;
    castling_rights = ui.castling;
    halfmove_clock  = ui.halfmove;
    hash            = ui.hash;
    side_to_move    = ~side_to_move;
    ply--;
}

// ---- Attack & check --------------------------------------------------------

Bitboard Board::attackers_to(Square sq, Bitboard occ) const {
    return (PawnAttacks[WHITE][sq]   & pieces[BLACK][PAWN])
         | (PawnAttacks[BLACK][sq]   & pieces[WHITE][PAWN])
         | (KnightAttacks[sq]        & (pieces[WHITE][KNIGHT] | pieces[BLACK][KNIGHT]))
         | (bishop_attacks(sq, occ)  & (pieces[WHITE][BISHOP] | pieces[WHITE][QUEEN]
                                      | pieces[BLACK][BISHOP] | pieces[BLACK][QUEEN]))
         | (rook_attacks(sq, occ)    & (pieces[WHITE][ROOK]   | pieces[WHITE][QUEEN]
                                      | pieces[BLACK][ROOK]   | pieces[BLACK][QUEEN]))
         | (KingAttacks[sq]          & (pieces[WHITE][KING]   | pieces[BLACK][KING]));
}

Bitboard Board::attackers_to(Square sq, Bitboard occ, Color by) const {
    return (PawnAttacks[~by][sq]     & pieces[by][PAWN])
         | (KnightAttacks[sq]        & pieces[by][KNIGHT])
         | (bishop_attacks(sq, occ)  & (pieces[by][BISHOP] | pieces[by][QUEEN]))
         | (rook_attacks(sq, occ)    & (pieces[by][ROOK]   | pieces[by][QUEEN]))
         | (KingAttacks[sq]          & pieces[by][KING]);
}

bool Board::is_square_attacked(Square sq, Color by) const {
    return attackers_to(sq, all_occ, by) != 0;
}

bool Board::is_in_check() const {
    return is_square_attacked(king_sq[side_to_move], ~side_to_move);
}

// ---- Move generation -------------------------------------------------------

static void add_promotions(std::vector<Move>& moves, Square from, Square to) {
    moves.push_back(make_promotion(from, to, QUEEN));
    moves.push_back(make_promotion(from, to, ROOK));
    moves.push_back(make_promotion(from, to, BISHOP));
    moves.push_back(make_promotion(from, to, KNIGHT));
}

void Board::gen_pseudo_legal(std::vector<Move>& moves) const {
    Color us   = side_to_move;
    Color them = ~us;
    Bitboard friendly = occupancy[us];
    Bitboard enemy    = occupancy[them];
    Bitboard empty    = ~all_occ;

    // ---- Pawns ----
    {
        Bitboard pawns = pieces[us][PAWN];
        Bitboard promo_rank = (us == WHITE) ? BB_RANKS[RANK_7] : BB_RANKS[RANK_2];
        Bitboard push1 = (us == WHITE) ? shift<NORTH>(pawns & ~promo_rank)
                                       : shift<SOUTH>(pawns & ~promo_rank);
        push1 &= empty;
        Bitboard push2 = (us == WHITE) ? (shift<NORTH>(push1) & empty & BB_RANKS[RANK_4])
                                       : (shift<SOUTH>(push1) & empty & BB_RANKS[RANK_5]);

        // Single push
        Bitboard tmp = push1;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 8 : to + 8;
            moves.push_back(::make_move(Square(from), Square(to)));
        }
        // Double push
        tmp = push2;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 16 : to + 16;
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // Pawn promotions (from promotion rank)
        Bitboard promo_pawns = pawns & promo_rank;
        if (promo_pawns) {
            Bitboard promo_push = (us == WHITE) ? shift<NORTH>(promo_pawns)
                                                : shift<SOUTH>(promo_pawns);
            promo_push &= empty;
            tmp = promo_push;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 8 : to + 8;
                add_promotions(moves, Square(from), Square(to));
            }

            // Promotion captures
            Bitboard pcap_e = (us == WHITE) ? shift<NORTH_EAST>(promo_pawns)
                                            : shift<SOUTH_EAST>(promo_pawns);
            Bitboard pcap_w = (us == WHITE) ? shift<NORTH_WEST>(promo_pawns)
                                            : shift<SOUTH_WEST>(promo_pawns);
            pcap_e &= enemy;
            pcap_w &= enemy;
            tmp = pcap_e;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 9 : to + 7;
                add_promotions(moves, Square(from), Square(to));
            }
            tmp = pcap_w;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 7 : to + 9;
                add_promotions(moves, Square(from), Square(to));
            }
        }

        // Normal captures
        Bitboard cap_e = (us == WHITE) ? shift<NORTH_EAST>(pawns & ~promo_rank)
                                       : shift<SOUTH_EAST>(pawns & ~promo_rank);
        Bitboard cap_w = (us == WHITE) ? shift<NORTH_WEST>(pawns & ~promo_rank)
                                       : shift<SOUTH_WEST>(pawns & ~promo_rank);
        cap_e &= enemy;
        cap_w &= enemy;
        tmp = cap_e;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 9 : to + 7;
            moves.push_back(::make_move(Square(from), Square(to)));
        }
        tmp = cap_w;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 7 : to + 9;
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // En passant
        if (ep_sq != SQ_NONE) {
            Bitboard ep_attackers = PawnAttacks[them][ep_sq] & pawns;
            tmp = ep_attackers;
            while (tmp) {
                int from = pop_lsb(tmp);
                moves.push_back(make_ep(Square(from), ep_sq));
            }
        }
    }

    // ---- Knights ----
    {
        Bitboard knights = pieces[us][KNIGHT];
        while (knights) {
            int from = pop_lsb(knights);
            Bitboard att = KnightAttacks[from] & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- Bishops ----
    {
        Bitboard bishops = pieces[us][BISHOP];
        while (bishops) {
            int from = pop_lsb(bishops);
            Bitboard att = bishop_attacks(Square(from), all_occ) & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- Rooks ----
    {
        Bitboard rooks = pieces[us][ROOK];
        while (rooks) {
            int from = pop_lsb(rooks);
            Bitboard att = rook_attacks(Square(from), all_occ) & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- Queens ----
    {
        Bitboard queens = pieces[us][QUEEN];
        while (queens) {
            int from = pop_lsb(queens);
            Bitboard att = queen_attacks(Square(from), all_occ) & ~friendly;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    }

    // ---- King ----
    {
        int from = king_sq[us];
        Bitboard att = KingAttacks[from] & ~friendly;
        while (att) {
            int to = pop_lsb(att);
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // Castling
        if (us == WHITE) {
            if ((castling_rights & WK_CASTLE)
                && !(all_occ & ((sq_bb(F1) | sq_bb(G1))))
                && !is_square_attacked(E1, BLACK)
                && !is_square_attacked(F1, BLACK)
                && !is_square_attacked(G1, BLACK)) {
                moves.push_back(make_castling(E1, G1));
            }
            if ((castling_rights & WQ_CASTLE)
                && !(all_occ & (sq_bb(B1) | sq_bb(C1) | sq_bb(D1)))
                && !is_square_attacked(E1, BLACK)
                && !is_square_attacked(D1, BLACK)
                && !is_square_attacked(C1, BLACK)) {
                moves.push_back(make_castling(E1, C1));
            }
        } else {
            if ((castling_rights & BK_CASTLE)
                && !(all_occ & (sq_bb(F8) | sq_bb(G8)))
                && !is_square_attacked(E8, WHITE)
                && !is_square_attacked(F8, WHITE)
                && !is_square_attacked(G8, WHITE)) {
                moves.push_back(make_castling(E8, G8));
            }
            if ((castling_rights & BQ_CASTLE)
                && !(all_occ & (sq_bb(B8) | sq_bb(C8) | sq_bb(D8)))
                && !is_square_attacked(E8, WHITE)
                && !is_square_attacked(D8, WHITE)
                && !is_square_attacked(C8, WHITE)) {
                moves.push_back(make_castling(E8, C8));
            }
        }
    }
}

void Board::gen_pseudo_legal_captures(std::vector<Move>& moves) const {
    Color us   = side_to_move;
    Color them = ~us;
    Bitboard enemy    = occupancy[them];

    // ---- Pawn captures & promotions ----
    {
        Bitboard pawns = pieces[us][PAWN];
        Bitboard promo_rank = (us == WHITE) ? BB_RANKS[RANK_7] : BB_RANKS[RANK_2];

        // Promotion pushes (queen only for now — queen captures ordering in qsearch)
        Bitboard promo_push = (us == WHITE) ? shift<NORTH>(pawns & promo_rank) & ~all_occ
                                            : shift<SOUTH>(pawns & promo_rank) & ~all_occ;
        Bitboard tmp = promo_push;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 8 : to + 8;
            add_promotions(moves, Square(from), Square(to));
        }

        // Promotion captures
        Bitboard promo_pawns = pawns & promo_rank;
        if (promo_pawns) {
            Bitboard pcap_e = (us == WHITE) ? shift<NORTH_EAST>(promo_pawns)
                                            : shift<SOUTH_EAST>(promo_pawns);
            Bitboard pcap_w = (us == WHITE) ? shift<NORTH_WEST>(promo_pawns)
                                            : shift<SOUTH_WEST>(promo_pawns);
            pcap_e &= enemy;
            pcap_w &= enemy;
            tmp = pcap_e;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 9 : to + 7;
                add_promotions(moves, Square(from), Square(to));
            }
            tmp = pcap_w;
            while (tmp) {
                int to = pop_lsb(tmp);
                int from = us == WHITE ? to - 7 : to + 9;
                add_promotions(moves, Square(from), Square(to));
            }
        }

        // Normal pawn captures (not promo rank)
        Bitboard cap_e = (us == WHITE) ? shift<NORTH_EAST>(pawns & ~promo_rank)
                                       : shift<SOUTH_EAST>(pawns & ~promo_rank);
        Bitboard cap_w = (us == WHITE) ? shift<NORTH_WEST>(pawns & ~promo_rank)
                                       : shift<SOUTH_WEST>(pawns & ~promo_rank);
        cap_e &= enemy;
        cap_w &= enemy;
        tmp = cap_e;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 9 : to + 7;
            moves.push_back(::make_move(Square(from), Square(to)));
        }
        tmp = cap_w;
        while (tmp) {
            int to = pop_lsb(tmp);
            int from = us == WHITE ? to - 7 : to + 9;
            moves.push_back(::make_move(Square(from), Square(to)));
        }

        // En passant
        if (ep_sq != SQ_NONE) {
            Bitboard ep_atk = PawnAttacks[them][ep_sq] & pawns;
            tmp = ep_atk;
            while (tmp) {
                int from = pop_lsb(tmp);
                moves.push_back(make_ep(Square(from), ep_sq));
            }
        }
    }

    // ---- Other pieces: captures only ----
    auto gen_piece_caps = [&](PieceType pt) {
        Bitboard pcs = pieces[us][pt];
        while (pcs) {
            int from = pop_lsb(pcs);
            Bitboard att;
            switch (pt) {
                case KNIGHT: att = KnightAttacks[from]; break;
                case BISHOP: att = bishop_attacks(Square(from), all_occ); break;
                case ROOK:   att = rook_attacks(Square(from), all_occ); break;
                case QUEEN:  att = queen_attacks(Square(from), all_occ); break;
                case KING:   att = KingAttacks[from]; break;
                default:     att = 0; break;
            }
            att &= enemy;
            while (att) {
                int to = pop_lsb(att);
                moves.push_back(::make_move(Square(from), Square(to)));
            }
        }
    };

    gen_piece_caps(KNIGHT);
    gen_piece_caps(BISHOP);
    gen_piece_caps(ROOK);
    gen_piece_caps(QUEEN);
    gen_piece_caps(KING);
}

bool Board::is_legal(Move m) const {
    Color us   = side_to_move;
    Color them = ~us;
    Square from = from_sq(m);
    Square to   = to_sq(m);
    MoveType mt = move_type(m);

    // King moves: destination must not be attacked after king leaves
    if (type_of(board_sq[from]) == KING) {
        if (mt == CASTLING) return true; // castling legality checked in gen
        Bitboard occ_after = (all_occ ^ sq_bb(from)) | sq_bb(to);
        return !attackers_to(to, occ_after, them);
    }

    // En passant: complex — do make/unmake
    if (mt == EN_PASSANT) {
        Board tmp = *this;
        tmp.make_move(m);
        return !tmp.is_square_attacked(tmp.king_sq[us], them);
    }

    // Regular moves: ensure king is not in check after the move
    Square ksq = king_sq[us];
    Bitboard occ_after = (all_occ ^ sq_bb(from)) | sq_bb(to);
    Bitboard not_captured = ~sq_bb(to);

    // Sliding attacks (depend on occupancy after move)
    Bitboard bishop_sliders = (pieces[them][BISHOP] | pieces[them][QUEEN]) & not_captured;
    if (bishop_attacks(ksq, occ_after) & bishop_sliders) return false;

    Bitboard rook_sliders = (pieces[them][ROOK] | pieces[them][QUEEN]) & not_captured;
    if (rook_attacks(ksq, occ_after) & rook_sliders) return false;

    // Knight and pawn attacks are not affected by occupancy
    if (KnightAttacks[ksq] & pieces[them][KNIGHT] & not_captured) return false;
    if (PawnAttacks[us][ksq] & pieces[them][PAWN]  & not_captured) return false;

    return true;
}

// ---- Draw detection --------------------------------------------------------

bool Board::is_draw() const {
    // 50-move rule
    if (halfmove_clock >= 100) return true;

    // 2-fold repetition (search back through history)
    int reps = 0;
    int stop = (int)history.size() - halfmove_clock;
    for (int i = (int)history.size() - 2; i >= stop; i -= 2) {
        if (history[i].hash == hash) {
            reps++;
            if (reps >= 1) return true; // 2-fold (current + one prior)
        }
    }
    return false;
}

bool Board::is_insufficient_material() const {
    int total = popcount(all_occ);
    if (total == 2) return true; // K vs K
    if (total == 3) {
        if (pieces[WHITE][KNIGHT] || pieces[BLACK][KNIGHT]) return true;
        if (pieces[WHITE][BISHOP] || pieces[BLACK][BISHOP]) return true;
    }
    if (total == 4) {
        // KB vs KB — same color bishops
        Bitboard wb = pieces[WHITE][BISHOP], bb2 = pieces[BLACK][BISHOP];
        if (wb && bb2) {
            // Both light or both dark
            bool wl = wb & 0x55AA55AA55AA55AAULL;
            bool bl = bb2 & 0x55AA55AA55AA55AAULL;
            if (wl == bl) return true;
        }
    }
    return false;
}

bool Board::has_non_pawn_material(Color c) const {
    return (pieces[c][KNIGHT] | pieces[c][BISHOP] | pieces[c][ROOK] | pieces[c][QUEEN]) != 0;
}

// ---- SEE -------------------------------------------------------------------

int Board::see(Move m) const {
    static constexpr int SEE_VALUES[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};

    Square from = from_sq(m);
    Square to   = to_sq(m);
    MoveType mt = move_type(m);

    int gain0;
    Piece target = board_sq[to];

    if (mt == EN_PASSANT) {
        gain0 = SEE_VALUES[PAWN];
    } else if (target != NO_PIECE) {
        gain0 = SEE_VALUES[type_of(target)];
    } else {
        gain0 = 0;
    }

    if (mt == PROMOTION)
        gain0 += SEE_VALUES[promo_type(m)] - SEE_VALUES[PAWN];

    int gain[32];
    gain[0] = gain0;
    int depth = 0;

    PieceType piece_on_sq = type_of(board_sq[from]);
    if (mt == PROMOTION) piece_on_sq = promo_type(m);

    Bitboard occ = all_occ ^ sq_bb(from);
    if (mt == EN_PASSANT) {
        Square ep_pawn = make_square(file_of(to), rank_of(from));
        occ ^= sq_bb(ep_pawn);
    }

    Color side = ~side_to_move; // first recapture is by opponent

    Bitboard attackers = attackers_to(to, occ) & occ;

    while (true) {
        depth++;
        gain[depth] = SEE_VALUES[piece_on_sq] - gain[depth-1];

        Bitboard side_att = attackers & occupancy[side];
        if (!side_att) break;

        // Find least valuable attacker
        PieceType attacker_type = NO_PIECE_TYPE;
        Bitboard attacker_bb = 0;
        for (int pt = PAWN; pt <= KING; pt++) {
            attacker_bb = side_att & pieces[side][pt];
            if (attacker_bb) { attacker_type = PieceType(pt); break; }
        }
        if (attacker_type == NO_PIECE_TYPE) break;

        // Early exit: even capturing for free doesn't help
        if (std::max(-gain[depth-1], gain[depth]) < 0) break;

        // Remove attacker (reveals X-ray)
        Bitboard lva_bit = attacker_bb & (-attacker_bb); // lowest set bit
        occ ^= lva_bit;
        piece_on_sq = attacker_type;
        side = ~side;

        // Recompute attackers (X-ray sliders revealed)
        attackers = attackers_to(to, occ) & occ;
    }

    // Minimax fold
    for (int i = depth - 1; i > 0; i--)
        gain[i-1] = std::max(gain[i-1], -gain[i]);

    return gain[0];
}
