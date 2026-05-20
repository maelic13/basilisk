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
    hash     = compute_hash();
    checkers = attackers_to(king_sq[side_to_move], all_occ, ~side_to_move);
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
    ui.checkers = checkers;
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
    checkers = attackers_to(king_sq[side_to_move], all_occ, ~side_to_move);
}

void Board::unmake_move(Move m) {
    assert(!history.empty());
    UndoInfo ui = history.back();
    history.pop_back();

    // Restore most state from undo info
    // (hash/checkers must be restored AFTER piece movements, since put_piece/
    //  move_piece update hash incrementally — we override them at the end)
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

    // Restore hash and checkers from undo info (overrides incremental updates
    // that were applied by put_piece/move_piece/remove_piece above)
    hash     = ui.hash;
    checkers = ui.checkers;
}

void Board::make_null_move() {
    UndoInfo ui;
    ui.hash     = hash;
    ui.checkers = checkers;
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
    // After a null move the new side-to-move cannot be in check:
    // if they were, their previous move would have been illegal.
    checkers = 0;
}

void Board::unmake_null_move() {
    assert(!history.empty());
    UndoInfo ui = history.back();
    history.pop_back();

    ep_sq           = ui.ep_sq;
    castling_rights = ui.castling;
    halfmove_clock  = ui.halfmove;
    hash            = ui.hash;
    checkers        = ui.checkers;
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
    return checkers != 0;
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

// ---- Legal move generation -------------------------------------------------
//
// gen_legal_impl<Us> generates all strictly legal moves for Us in a single pass.
// Algorithm:
//   1. Compute king moves (safe squares via x-ray with king removed)
//   2. Detect double check → only king moves legal, early return
//   3. Build check_mask: squares that resolve the single check (block or capture)
//   4. Detect pinned pieces via x-ray from king through our own pieces
//   5. Generate each piece type, applying pin-ray constraints and check_mask
//
template<Color Us>
static void gen_legal_impl(const Board& b, MoveList& ml, bool caps_only) {
    constexpr Color Them    = (Us == WHITE) ? BLACK : WHITE;
    constexpr int   PushOff = (Us == WHITE) ?  8 : -8;
    constexpr int   PushOff2= (Us == WHITE) ? 16 : -16;
    constexpr int   CapEOff = (Us == WHITE) ?  9 : -7;
    constexpr int   CapWOff = (Us == WHITE) ?  7 : -9;

    const Square   ksq    = b.king_sq[Us];
    const Bitboard us_bb  = b.occupancy[Us];
    const Bitboard them_bb= b.occupancy[Them];
    const Bitboard occ    = b.all_occ;
    const Bitboard empty  = ~occ;

    // Rank constants (can't be constexpr since BB_RANKS is a runtime array)
    const Bitboard PromRank  = (Us == WHITE) ? BB_RANKS[RANK_7] : BB_RANKS[RANK_2];
    const Bitboard StartRank = (Us == WHITE) ? BB_RANKS[RANK_2] : BB_RANKS[RANK_7];

    // Direction-dispatched pawn shift helpers
    auto push_one = [](Bitboard bb) -> Bitboard {
        if constexpr (Us == WHITE) return shift<NORTH>(bb);
        else                       return shift<SOUTH>(bb);
    };
    auto cap_e = [](Bitboard bb) -> Bitboard {
        if constexpr (Us == WHITE) return shift<NORTH_EAST>(bb);
        else                       return shift<SOUTH_EAST>(bb);
    };
    auto cap_w = [](Bitboard bb) -> Bitboard {
        if constexpr (Us == WHITE) return shift<NORTH_WEST>(bb);
        else                       return shift<SOUTH_WEST>(bb);
    };

    // ---- King moves (always generate regardless of check count) ----
    {
        Bitboard occ_no_king = occ ^ sq_bb(ksq);
        Bitboard targets = KingAttacks[ksq] & ~us_bb;
        if (caps_only) targets &= them_bb;
        while (targets) {
            Square to = Square(pop_lsb(targets));
            if (!b.attackers_to(to, occ_no_king, Them))
                ml.push(make_move(ksq, to));
        }
    }

    // Double check → only king can move
    Bitboard checkers = b.checkers;
    if (popcount(checkers) > 1) return;

    // ---- Check mask (all squares if no check; block/capture ray if single check) ----
    const Bitboard check_mask = (checkers == 0)
        ? ~Bitboard(0)
        : (checkers | BB_BETWEEN[ksq][Square(lsb(checkers))]);

    // ---- Pin detection via x-ray ----
    Bitboard pinned = 0;
    {
        // Diagonal pinners (bishops & queens)
        Bitboard bv  = bishop_attacks(ksq, occ);
        Bitboard xrb = bishop_attacks(ksq, occ ^ (bv & us_bb));
        Bitboard dp  = (b.pieces[Them][BISHOP] | b.pieces[Them][QUEEN]) & xrb;
        while (dp) {
            Square   pinner  = Square(pop_lsb(dp));
            Bitboard blocker = BB_BETWEEN[ksq][pinner] & us_bb;
            if (blocker && !more_than_one(blocker)) pinned |= blocker;
        }
        // Orthogonal pinners (rooks & queens)
        Bitboard rv  = rook_attacks(ksq, occ);
        Bitboard xrr = rook_attacks(ksq, occ ^ (rv & us_bb));
        Bitboard op  = (b.pieces[Them][ROOK] | b.pieces[Them][QUEEN]) & xrr;
        while (op) {
            Square   pinner  = Square(pop_lsb(op));
            Bitboard blocker = BB_BETWEEN[ksq][pinner] & us_bb;
            if (blocker && !more_than_one(blocker)) pinned |= blocker;
        }
    }

    // ---- Knights (absolutely pinned knights can never move) ----
    {
        Bitboard knights = b.pieces[Us][KNIGHT] & ~pinned;
        while (knights) {
            Square   from  = Square(pop_lsb(knights));
            Bitboard dests = KnightAttacks[from] & ~us_bb & check_mask;
            if (caps_only) dests &= them_bb;
            while (dests) ml.push(make_move(from, Square(pop_lsb(dests))));
        }
    }

    // ---- Bishops ----
    {
        Bitboard bishops = b.pieces[Us][BISHOP];
        while (bishops) {
            Square   from  = Square(pop_lsb(bishops));
            Bitboard dests = bishop_attacks(from, occ) & ~us_bb & check_mask;
            if (sq_bb(from) & pinned) dests &= BB_LINE[ksq][from];
            if (caps_only) dests &= them_bb;
            while (dests) ml.push(make_move(from, Square(pop_lsb(dests))));
        }
    }

    // ---- Rooks ----
    {
        Bitboard rooks = b.pieces[Us][ROOK];
        while (rooks) {
            Square   from  = Square(pop_lsb(rooks));
            Bitboard dests = rook_attacks(from, occ) & ~us_bb & check_mask;
            if (sq_bb(from) & pinned) dests &= BB_LINE[ksq][from];
            if (caps_only) dests &= them_bb;
            while (dests) ml.push(make_move(from, Square(pop_lsb(dests))));
        }
    }

    // ---- Queens ----
    {
        Bitboard queens = b.pieces[Us][QUEEN];
        while (queens) {
            Square   from  = Square(pop_lsb(queens));
            Bitboard dests = queen_attacks(from, occ) & ~us_bb & check_mask;
            if (sq_bb(from) & pinned) dests &= BB_LINE[ksq][from];
            if (caps_only) dests &= them_bb;
            while (dests) ml.push(make_move(from, Square(pop_lsb(dests))));
        }
    }

    // ---- Pawns ----
    {
        Bitboard pawns        = b.pieces[Us][PAWN];
        Bitboard free_pawns   = pawns & ~pinned;
        Bitboard pinned_pawns = pawns & pinned;

        // --- Free pawns (bulk bitboard ops) ---

        // Promotion pushes
        {
            Bitboard pp = push_one(free_pawns & PromRank) & empty & check_mask;
            while (pp) {
                Square to   = Square(pop_lsb(pp));
                Square from = Square(int(to) - PushOff);
                ml.push(make_promotion(from, to, QUEEN));
                ml.push(make_promotion(from, to, ROOK));
                ml.push(make_promotion(from, to, BISHOP));
                ml.push(make_promotion(from, to, KNIGHT));
            }
        }
        // Promotion captures (east)
        {
            Bitboard pp = cap_e(free_pawns & PromRank) & them_bb & check_mask;
            while (pp) {
                Square to   = Square(pop_lsb(pp));
                Square from = Square(int(to) - CapEOff);
                ml.push(make_promotion(from, to, QUEEN));
                ml.push(make_promotion(from, to, ROOK));
                ml.push(make_promotion(from, to, BISHOP));
                ml.push(make_promotion(from, to, KNIGHT));
            }
        }
        // Promotion captures (west)
        {
            Bitboard pp = cap_w(free_pawns & PromRank) & them_bb & check_mask;
            while (pp) {
                Square to   = Square(pop_lsb(pp));
                Square from = Square(int(to) - CapWOff);
                ml.push(make_promotion(from, to, QUEEN));
                ml.push(make_promotion(from, to, ROOK));
                ml.push(make_promotion(from, to, BISHOP));
                ml.push(make_promotion(from, to, KNIGHT));
            }
        }

        if (!caps_only) {
            // Single push (non-promo)
            Bitboard p1 = push_one(free_pawns & ~PromRank) & empty & check_mask;
            while (p1) {
                Square to = Square(pop_lsb(p1));
                ml.push(make_move(Square(int(to) - PushOff), to));
            }
            // Double push
            Bitboard p2 = push_one(push_one(free_pawns & StartRank) & empty) & empty & check_mask;
            while (p2) {
                Square to = Square(pop_lsb(p2));
                ml.push(make_move(Square(int(to) - PushOff2), to));
            }
        }

        // Pawn captures east (non-promo)
        {
            Bitboard ce = cap_e(free_pawns & ~PromRank) & them_bb & check_mask;
            while (ce) {
                Square to = Square(pop_lsb(ce));
                ml.push(make_move(Square(int(to) - CapEOff), to));
            }
        }
        // Pawn captures west (non-promo)
        {
            Bitboard cw = cap_w(free_pawns & ~PromRank) & them_bb & check_mask;
            while (cw) {
                Square to = Square(pop_lsb(cw));
                ml.push(make_move(Square(int(to) - CapWOff), to));
            }
        }

        // --- Pinned pawns (per-pawn with pin-ray constraint) ---
        while (pinned_pawns) {
            Square   from    = Square(pop_lsb(pinned_pawns));
            Bitboard pin_ray = BB_LINE[ksq][from];

            bool is_promo = (sq_bb(from) & PromRank) != 0;

            // Pushes (along pin ray only)
            if (!caps_only) {
                Square to1 = Square(int(from) + PushOff);
                if (sq_bb(to1) & empty & pin_ray & check_mask) {
                    if (is_promo) {
                        ml.push(make_promotion(from, to1, QUEEN));
                        ml.push(make_promotion(from, to1, ROOK));
                        ml.push(make_promotion(from, to1, BISHOP));
                        ml.push(make_promotion(from, to1, KNIGHT));
                    } else {
                        ml.push(make_move(from, to1));
                        // Double push
                        if (sq_bb(from) & StartRank) {
                            Square to2 = Square(int(from) + PushOff2);
                            if (sq_bb(to2) & empty & pin_ray & check_mask)
                                ml.push(make_move(from, to2));
                        }
                    }
                }
            } else if (is_promo) {
                // caps_only: still emit promo pushes as they are tactically important
                Square to1 = Square(int(from) + PushOff);
                if (sq_bb(to1) & empty & pin_ray & check_mask) {
                    ml.push(make_promotion(from, to1, QUEEN));
                    ml.push(make_promotion(from, to1, ROOK));
                    ml.push(make_promotion(from, to1, BISHOP));
                    ml.push(make_promotion(from, to1, KNIGHT));
                }
            }

            // Captures
            Bitboard ce1 = cap_e(sq_bb(from)) & them_bb & pin_ray & check_mask;
            if (ce1) {
                Square to = Square(lsb(ce1));
                if (is_promo) {
                    ml.push(make_promotion(from, to, QUEEN));
                    ml.push(make_promotion(from, to, ROOK));
                    ml.push(make_promotion(from, to, BISHOP));
                    ml.push(make_promotion(from, to, KNIGHT));
                } else {
                    ml.push(make_move(from, to));
                }
            }
            Bitboard cw1 = cap_w(sq_bb(from)) & them_bb & pin_ray & check_mask;
            if (cw1) {
                Square to = Square(lsb(cw1));
                if (is_promo) {
                    ml.push(make_promotion(from, to, QUEEN));
                    ml.push(make_promotion(from, to, ROOK));
                    ml.push(make_promotion(from, to, BISHOP));
                    ml.push(make_promotion(from, to, KNIGHT));
                } else {
                    ml.push(make_move(from, to));
                }
            }
        }

        // --- En passant ---
        if (b.ep_sq != SQ_NONE) {
            Square ep       = b.ep_sq;
            Square ep_pawn  = Square(int(ep) - PushOff);  // captured pawn square
            // Filter: EP must either land on check_mask or capture the checking pawn
            if ((sq_bb(ep) | sq_bb(ep_pawn)) & check_mask) {
                Bitboard ep_atk = PawnAttacks[Them][ep] & pawns;
                while (ep_atk) {
                    Square   from      = Square(pop_lsb(ep_atk));
                    // Legality: removing both pawns from occ might expose the king
                    Bitboard occ_after = (occ ^ sq_bb(from) ^ sq_bb(ep_pawn)) | sq_bb(ep);
                    if (!b.attackers_to(ksq, occ_after, Them))
                        ml.push(make_ep(from, ep));
                }
            }
        }
    }

    // ---- Castling (only when not in check) ----
    if (!caps_only && checkers == 0) {
        if constexpr (Us == WHITE) {
            if ((b.castling_rights & WK_CASTLE)
                && !(occ & (sq_bb(F1) | sq_bb(G1)))
                && !b.attackers_to(F1, occ, Them)
                && !b.attackers_to(G1, occ, Them))
                ml.push(make_castling(E1, G1));
            if ((b.castling_rights & WQ_CASTLE)
                && !(occ & (sq_bb(B1) | sq_bb(C1) | sq_bb(D1)))
                && !b.attackers_to(D1, occ, Them)
                && !b.attackers_to(C1, occ, Them))
                ml.push(make_castling(E1, C1));
        } else {
            if ((b.castling_rights & BK_CASTLE)
                && !(occ & (sq_bb(F8) | sq_bb(G8)))
                && !b.attackers_to(F8, occ, Them)
                && !b.attackers_to(G8, occ, Them))
                ml.push(make_castling(E8, G8));
            if ((b.castling_rights & BQ_CASTLE)
                && !(occ & (sq_bb(B8) | sq_bb(C8) | sq_bb(D8)))
                && !b.attackers_to(D8, occ, Them)
                && !b.attackers_to(C8, occ, Them))
                ml.push(make_castling(E8, C8));
        }
    }
}

void Board::gen_legal(MoveList& ml) const {
    if (side_to_move == WHITE) gen_legal_impl<WHITE>(*this, ml, false);
    else                       gen_legal_impl<BLACK>(*this, ml, false);
}

void Board::gen_legal_captures(MoveList& ml) const {
    if (side_to_move == WHITE) gen_legal_impl<WHITE>(*this, ml, true);
    else                       gen_legal_impl<BLACK>(*this, ml, true);
}

bool Board::gives_check(Move m) const {
    Square from      = from_sq(m);
    Square to        = to_sq(m);
    Color  us        = side_to_move;
    Color  them      = ~us;
    Square their_king = king_sq[them];

    // Piece type after the move (promotion changes the type)
    PieceType pt;
    if (move_type(m) == PROMOTION)  pt = promo_type(m);
    else if (move_type(m) == CASTLING) return false;  // skip for simplicity
    else                            pt = type_of(board_sq[from]);

    // Occupancy after removing `from` and adding `to`
    Bitboard new_occ = (all_occ ^ sq_bb(from)) | sq_bb(to);
    if (move_type(m) == EN_PASSANT) {
        Square ep_pawn = make_square(file_of(to), rank_of(from));
        new_occ ^= sq_bb(ep_pawn);
    }

    // 1. Direct check: does the moved piece at `to` attack their king?
    switch (pt) {
        case PAWN:
            if (PawnAttacks[us][to] & sq_bb(their_king)) return true;
            break;
        case KNIGHT:
            if (KnightAttacks[to] & sq_bb(their_king)) return true;
            break;
        case BISHOP:
            if (bishop_attacks(to, new_occ) & sq_bb(their_king)) return true;
            break;
        case ROOK:
            if (rook_attacks(to, new_occ) & sq_bb(their_king)) return true;
            break;
        case QUEEN:
            if (queen_attacks(to, new_occ) & sq_bb(their_king)) return true;
            break;
        default:
            break;
    }

    // 2. Discovered check: moving `from` reveals a slider attack on their king.
    //    Use `new_occ` (from removed) and original piece arrays, excluding `from`.
    Bitboard excl = ~sq_bb(from);
    if (bishop_attacks(their_king, new_occ)
            & (pieces[us][BISHOP] | pieces[us][QUEEN]) & excl) return true;
    if (rook_attacks(their_king, new_occ)
            & (pieces[us][ROOK]   | pieces[us][QUEEN]) & excl) return true;

    return false;
}

void Board::gen_quiet_checks(MoveList& ml) const {
    // Generate all legal moves, keep only quiet moves that give check.
    MoveList all;
    gen_legal(all);
    for (Move m : all) {
        // Skip captures and promotions (handled by gen_legal_captures)
        bool is_cap   = (board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);
        if (is_cap || is_promo) continue;
        if (gives_check(m)) ml.push(m);
    }
}

// ---- Draw detection --------------------------------------------------------

bool Board::is_draw() const {
    // 50-move rule
    if (halfmove_clock >= 100) return true;

    // 2-fold repetition (search back through history)
    int reps = 0;
    int stop = std::max(0, (int)history.size() - halfmove_clock);
    for (int i = (int)history.size() - 2; i >= stop; i -= 2) {
        if (history[static_cast<size_t>(i)].hash == hash) {
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
