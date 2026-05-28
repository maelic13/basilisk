// Board.cpp — full board implementation
#include "Board.h"
#include "Constants.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <limits>
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
    if (pt == PAWN) pawn_key ^= Zobrist::PieceKeys[c][PAWN][sq];
    if (pt == KNIGHT || pt == BISHOP) minor_key ^= Zobrist::PieceKeys[c][pt][sq];
    if (pt != PAWN && pt != KING) nonpawn_key[c] ^= Zobrist::PieceKeys[c][pt][sq];
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
    if (pt == PAWN) pawn_key ^= Zobrist::PieceKeys[c][PAWN][sq];
    if (pt == KNIGHT || pt == BISHOP) minor_key ^= Zobrist::PieceKeys[c][pt][sq];
    if (pt != PAWN && pt != KING) nonpawn_key[c] ^= Zobrist::PieceKeys[c][pt][sq];
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
    if (pt == PAWN)
        pawn_key ^= Zobrist::PieceKeys[c][PAWN][from]
                  ^ Zobrist::PieceKeys[c][PAWN][to];
    if (pt == KNIGHT || pt == BISHOP)
        minor_key ^= Zobrist::PieceKeys[c][pt][from]
                   ^ Zobrist::PieceKeys[c][pt][to];
    if (pt != PAWN && pt != KING)
        nonpawn_key[c] ^= Zobrist::PieceKeys[c][pt][from]
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

static bool has_piece_on(const Board& b, Square sq, Color color, PieceType pt) {
    const Piece p = b.board_sq[sq];
    return p != NO_PIECE && color_of(p) == color && type_of(p) == pt;
}

static bool can_castle_kingside(const Board& b, Color us) {
    const Rank back = us == WHITE ? RANK_1 : RANK_8;
    const Square king_from = make_square(FILE_E, back);
    const Square rook_from = make_square(FILE_H, back);
    const int right = us == WHITE ? WK_CASTLE : BK_CASTLE;
    return (b.castling_rights & right)
        && has_piece_on(b, king_from, us, KING)
        && has_piece_on(b, rook_from, us, ROOK);
}

static bool can_castle_queenside(const Board& b, Color us) {
    const Rank back = us == WHITE ? RANK_1 : RANK_8;
    const Square king_from = make_square(FILE_E, back);
    const Square rook_from = make_square(FILE_A, back);
    const int right = us == WHITE ? WQ_CASTLE : BQ_CASTLE;
    return (b.castling_rights & right)
        && has_piece_on(b, king_from, us, KING)
        && has_piece_on(b, rook_from, us, ROOK);
}

// ---- Constructor & FEN parsing ---------------------------------------------

Board::Board()
    : history(std::make_unique<UndoInfo[]>(MAX_HISTORY))
    , history_size(0) {
    set_fen(std::string(startPosition));
}

Board::Board(const Board& other)
    : history(std::make_unique<UndoInfo[]>(MAX_HISTORY))
    , history_size(0) {
    *this = other;
}

Board& Board::operator=(const Board& other) {
    if (this == &other) return *this;

    if (!history)
        history = std::make_unique<UndoInfo[]>(MAX_HISTORY);

    for (int c = 0; c < NCOLORS; ++c) {
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            pieces[c][pt] = other.pieces[c][pt];
        occupancy[c] = other.occupancy[c];
        king_sq[c] = other.king_sq[c];
    }

    all_occ = other.all_occ;
    for (int s = 0; s < SQUARE_NB; ++s)
        board_sq[s] = other.board_sq[s];

    side_to_move = other.side_to_move;
    fullmove_number = other.fullmove_number;
    ply = other.ply;
    hash = other.hash;
    pawn_key = other.pawn_key;
    minor_key = other.minor_key;
    nonpawn_key[WHITE] = other.nonpawn_key[WHITE];
    nonpawn_key[BLACK] = other.nonpawn_key[BLACK];
    ep_sq = other.ep_sq;
    castling_rights = other.castling_rights;
    halfmove_clock = other.halfmove_clock;
    plies_from_null = other.plies_from_null;
    checkers = other.checkers;

    history_size = other.history_size;
    for (int i = 0; i < history_size; ++i)
        history[static_cast<size_t>(i)] = other.history[static_cast<size_t>(i)];

    return *this;
}

void Board::set_fen(const std::string& fen) {
    std::string error;
    if (!try_set_fen(fen, &error))
        throw std::invalid_argument(error);
}

bool Board::try_set_fen(const std::string& fen, std::string* error,
                        bool validate_legal_position) {
    auto fail = [&](const std::string& message) {
        if (error)
            *error = message;
        return false;
    };

    auto parse_nonnegative_int = [](const std::string& token, int max_value, int& out) {
        if (token.empty())
            return false;
        long long value = 0;
        for (char ch : token) {
            if (!std::isdigit(static_cast<unsigned char>(ch)))
                return false;
            value = value * 10 + (ch - '0');
            if (value > max_value)
                return false;
        }
        out = static_cast<int>(value);
        return true;
    };

    auto decode_piece = [](char ch, Color& color, PieceType& pt) {
        switch (ch) {
            case 'P': color = WHITE; pt = PAWN;   return true;
            case 'N': color = WHITE; pt = KNIGHT; return true;
            case 'B': color = WHITE; pt = BISHOP; return true;
            case 'R': color = WHITE; pt = ROOK;   return true;
            case 'Q': color = WHITE; pt = QUEEN;  return true;
            case 'K': color = WHITE; pt = KING;   return true;
            case 'p': color = BLACK; pt = PAWN;   return true;
            case 'n': color = BLACK; pt = KNIGHT; return true;
            case 'b': color = BLACK; pt = BISHOP; return true;
            case 'r': color = BLACK; pt = ROOK;   return true;
            case 'q': color = BLACK; pt = QUEEN;  return true;
            case 'k': color = BLACK; pt = KING;   return true;
            default: return false;
        }
    };

    std::istringstream ss(fen);
    std::string placement, side_token, castling_token, ep_token;
    if (!(ss >> placement >> side_token >> castling_token >> ep_token))
        return fail("Invalid FEN. Expected at least four fields.");

    int parsed_halfmove = 0;
    int parsed_fullmove = 1;
    std::string token;
    if (ss >> token) {
        if (!parse_nonnegative_int(token, 32767, parsed_halfmove))
            return fail("Invalid FEN. Invalid halfmove clock.");
    }
    if (ss >> token) {
        if (!parse_nonnegative_int(token, 100000, parsed_fullmove))
            return fail("Invalid FEN. Invalid fullmove number.");
    }
    if (ss >> token)
        return fail("Invalid FEN. Too many fields.");

    Bitboard new_pieces[NCOLORS][PIECE_TYPE_NB] = {};
    Bitboard new_occupancy[NCOLORS] = {};
    Bitboard new_all_occ = 0;
    Piece    new_board_sq[SQUARE_NB];
    for (int s = 0; s < SQUARE_NB; ++s)
        new_board_sq[s] = NO_PIECE;

    Key new_pawn_key = 0;
    Key new_minor_key = 0;
    Key new_nonpawn_key[NCOLORS] = {0, 0};

    int rank = 7;
    int file = 0;
    int num_pieces = 0;
    for (char ch : placement) {
        if (ch == '/') {
            if (file != 8)
                return fail("Invalid FEN. Rank ended before eight squares.");
            if (rank == 0)
                return fail("Invalid FEN. Too many ranks.");
            --rank;
            file = 0;
            continue;
        }

        if (ch >= '1' && ch <= '8') {
            file += ch - '0';
            if (file > 8)
                return fail("Invalid FEN. Rank has too many squares.");
            continue;
        }

        Color c = WHITE;
        PieceType pt = NO_PIECE_TYPE;
        if (!decode_piece(ch, c, pt))
            return fail(std::string("Invalid FEN. Invalid piece: ") + ch);
        if (file >= 8)
            return fail("Invalid FEN. Rank has too many squares.");
        if (++num_pieces > 32)
            return fail("Invalid FEN. More than 32 pieces on the board.");

        Square sq = make_square(File(file), Rank(rank));
        Bitboard bb = sq_bb(sq);
        new_pieces[c][pt] |= bb;
        new_occupancy[c]  |= bb;
        new_all_occ       |= bb;
        new_board_sq[sq]   = make_piece(c, pt);
        if (pt == PAWN) new_pawn_key ^= Zobrist::PieceKeys[c][PAWN][sq];
        if (pt == KNIGHT || pt == BISHOP) new_minor_key ^= Zobrist::PieceKeys[c][pt][sq];
        if (pt != PAWN && pt != KING) new_nonpawn_key[c] ^= Zobrist::PieceKeys[c][pt][sq];
        ++file;
    }

    if (rank != 0 || file != 8)
        return fail("Invalid FEN. Board state encoding ended before eight ranks.");

    const Bitboard back_ranks = BB_RANKS[RANK_1] | BB_RANKS[RANK_8];
    if ((new_pieces[WHITE][PAWN] | new_pieces[BLACK][PAWN]) & back_ranks)
        return fail("Unsupported position. Pawns on the first or eighth rank.");

    if (popcount(new_pieces[WHITE][KING]) != 1 || popcount(new_pieces[BLACK][KING]) != 1)
        return fail("Unsupported position. Incorrect number of kings.");

    if (popcount(new_pieces[WHITE][PAWN]) > 8)
        return fail("Unsupported position. WHITE has more than 8 pawns.");
    if (popcount(new_pieces[BLACK][PAWN]) > 8)
        return fail("Unsupported position. BLACK has more than 8 pawns.");

    auto promoted_piece_count = [&](Color c) {
        return std::max(popcount(new_pieces[c][KNIGHT]) - 2, 0)
             + std::max(popcount(new_pieces[c][BISHOP]) - 2, 0)
             + std::max(popcount(new_pieces[c][ROOK])   - 2, 0)
             + std::max(popcount(new_pieces[c][QUEEN])  - 1, 0);
    };
    if (promoted_piece_count(WHITE) > 8 - popcount(new_pieces[WHITE][PAWN]))
        return fail("Unsupported position. Too many pieces for WHITE.");
    if (promoted_piece_count(BLACK) > 8 - popcount(new_pieces[BLACK][PAWN]))
        return fail("Unsupported position. Too many pieces for BLACK.");

    Color new_side_to_move;
    if (side_token == "w")
        new_side_to_move = WHITE;
    else if (side_token == "b")
        new_side_to_move = BLACK;
    else
        return fail("Invalid FEN. Invalid side to move.");

    int new_castling_rights = NO_CASTLING;
    if (castling_token != "-") {
        for (char ch : castling_token) {
            switch (ch) {
                case 'K': new_castling_rights |= WK_CASTLE; break;
                case 'Q': new_castling_rights |= WQ_CASTLE; break;
                case 'k': new_castling_rights |= BK_CASTLE; break;
                case 'q': new_castling_rights |= BQ_CASTLE; break;
                default:
                    return fail("Invalid FEN. Invalid castling rights.");
            }
        }
    }

    auto has_piece = [&](Square sq, Color color, PieceType pt) {
        Piece p = new_board_sq[sq];
        return p != NO_PIECE && color_of(p) == color && type_of(p) == pt;
    };
    if (!has_piece(E1, WHITE, KING) || !has_piece(H1, WHITE, ROOK))
        new_castling_rights &= ~WK_CASTLE;
    if (!has_piece(E1, WHITE, KING) || !has_piece(A1, WHITE, ROOK))
        new_castling_rights &= ~WQ_CASTLE;
    if (!has_piece(E8, BLACK, KING) || !has_piece(H8, BLACK, ROOK))
        new_castling_rights &= ~BK_CASTLE;
    if (!has_piece(E8, BLACK, KING) || !has_piece(A8, BLACK, ROOK))
        new_castling_rights &= ~BQ_CASTLE;

    Square new_ep_sq = SQ_NONE;
    if (ep_token != "-") {
        if (ep_token.size() != 2
            || ep_token[0] < 'a' || ep_token[0] > 'h'
            || ep_token[1] < '1' || ep_token[1] > '8') {
            return fail("Invalid FEN. Invalid en-passant square.");
        }

        File ep_file = File(ep_token[0] - 'a');
        Rank ep_rank = Rank(ep_token[1] - '1');
        if ((new_side_to_move == WHITE && ep_rank != RANK_6)
            || (new_side_to_move == BLACK && ep_rank != RANK_3)) {
            return fail("Invalid FEN. Invalid en-passant square.");
        }

        new_ep_sq = make_square(ep_file, ep_rank);
        Square pushed_pawn = Square(int(new_ep_sq) + int(pawn_push(~new_side_to_move)));
        Square origin_sq   = Square(int(new_ep_sq) + int(pawn_push(new_side_to_move)));
        if (new_board_sq[new_ep_sq] != NO_PIECE
            || new_board_sq[origin_sq] != NO_PIECE
            || !has_piece(pushed_pawn, ~new_side_to_move, PAWN)) {
            return fail("Invalid FEN. Invalid en-passant square.");
        }
    }

    Square new_king_sq[NCOLORS] = {
        Square(lsb(new_pieces[WHITE][KING])),
        Square(lsb(new_pieces[BLACK][KING]))
    };

    auto attackers_to_local = [&](Square sq, Color by) {
        return (PawnAttacks[~by][sq]     & new_pieces[by][PAWN])
             | (KnightAttacks[sq]        & new_pieces[by][KNIGHT])
             | (bishop_attacks(sq, new_all_occ) & (new_pieces[by][BISHOP] | new_pieces[by][QUEEN]))
             | (rook_attacks(sq, new_all_occ)   & (new_pieces[by][ROOK]   | new_pieces[by][QUEEN]))
             | (KingAttacks[sq]          & new_pieces[by][KING]);
    };

    if (validate_legal_position
        && attackers_to_local(new_king_sq[~new_side_to_move], new_side_to_move))
        return fail("Unsupported position. King can be captured.");

    Key new_hash = 0;
    for (int c = 0; c < NCOLORS; ++c) {
        for (int pt = PAWN; pt <= KING; ++pt) {
            Bitboard bb = new_pieces[c][pt];
            while (bb) {
                int sq = pop_lsb(bb);
                new_hash ^= Zobrist::PieceKeys[c][pt][sq];
            }
        }
    }
    if (new_side_to_move == BLACK) new_hash ^= Zobrist::SideKey;
    new_hash ^= Zobrist::CastlingKeys[new_castling_rights];
    if (new_ep_sq != SQ_NONE) new_hash ^= Zobrist::EpKeys[file_of(new_ep_sq)];

    for (int c = 0; c < NCOLORS; ++c) {
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            pieces[c][pt] = new_pieces[c][pt];
        occupancy[c] = new_occupancy[c];
        king_sq[c] = new_king_sq[c];
    }
    all_occ = new_all_occ;
    for (int s = 0; s < SQUARE_NB; ++s)
        board_sq[s] = new_board_sq[s];

    side_to_move = new_side_to_move;
    fullmove_number = parsed_fullmove;
    ply = 0;
    hash = new_hash;
    pawn_key = new_pawn_key;
    minor_key = new_minor_key;
    nonpawn_key[WHITE] = new_nonpawn_key[WHITE];
    nonpawn_key[BLACK] = new_nonpawn_key[BLACK];
    ep_sq = new_ep_sq;
    castling_rights = new_castling_rights;
    halfmove_clock = parsed_halfmove;
    plies_from_null = 0;
    checkers = attackers_to_local(king_sq[side_to_move], ~side_to_move);
    history_size = 0;

    return true;
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
    ui.pawn_key = pawn_key;
    ui.minor_key = minor_key;
    ui.nonpawn_key[WHITE] = nonpawn_key[WHITE];
    ui.nonpawn_key[BLACK] = nonpawn_key[BLACK];
    ui.checkers = checkers;
    ui.ep_sq    = ep_sq;
    ui.castling = castling_rights;
    ui.halfmove = halfmove_clock;
    ui.plies_from_null = plies_from_null;
    ui.captured = NO_PIECE;
    assert(history_size < MAX_HISTORY);
    history[static_cast<size_t>(history_size++)] = ui;

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
    plies_from_null++;

    if (mt == EN_PASSANT) {
        // Capture the EP pawn
        Square ep_pawn = make_square(file_of(to), rank_of(from));
        history[static_cast<size_t>(history_size - 1)].captured = board_sq[ep_pawn];
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
            history[static_cast<size_t>(history_size - 1)].captured = captured;
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
    assert(history_size > 0);
    UndoInfo ui = history[static_cast<size_t>(--history_size)];

    // Restore most state from undo info
    // (hash/checkers must be restored AFTER piece movements, since put_piece/
    //  move_piece update hash incrementally — we override them at the end)
    ep_sq           = ui.ep_sq;
    castling_rights = ui.castling;
    halfmove_clock  = ui.halfmove;
    plies_from_null = ui.plies_from_null;

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
    pawn_key = ui.pawn_key;
    minor_key = ui.minor_key;
    nonpawn_key[WHITE] = ui.nonpawn_key[WHITE];
    nonpawn_key[BLACK] = ui.nonpawn_key[BLACK];
    checkers = ui.checkers;
}

void Board::make_null_move() {
    UndoInfo ui;
    ui.hash     = hash;
    ui.pawn_key = pawn_key;
    ui.minor_key = minor_key;
    ui.nonpawn_key[WHITE] = nonpawn_key[WHITE];
    ui.nonpawn_key[BLACK] = nonpawn_key[BLACK];
    ui.checkers = checkers;
    ui.ep_sq    = ep_sq;
    ui.castling = castling_rights;
    ui.halfmove = halfmove_clock;
    ui.plies_from_null = plies_from_null;
    ui.captured = NO_PIECE;
    assert(history_size < MAX_HISTORY);
    history[static_cast<size_t>(history_size++)] = ui;

    hash ^= Zobrist::SideKey;
    if (ep_sq != SQ_NONE) {
        hash ^= Zobrist::EpKeys[file_of(ep_sq)];
        ep_sq = SQ_NONE;
    }
    halfmove_clock++;
    plies_from_null = 0;
    side_to_move = ~side_to_move;
    ply++;
    // After a null move the new side-to-move cannot be in check:
    // if they were, their previous move would have been illegal.
    checkers = 0;
}

void Board::unmake_null_move() {
    assert(history_size > 0);
    UndoInfo ui = history[static_cast<size_t>(--history_size)];

    ep_sq           = ui.ep_sq;
    castling_rights = ui.castling;
    halfmove_clock  = ui.halfmove;
    plies_from_null = ui.plies_from_null;
    hash            = ui.hash;
    pawn_key        = ui.pawn_key;
    minor_key       = ui.minor_key;
    nonpawn_key[WHITE] = ui.nonpawn_key[WHITE];
    nonpawn_key[BLACK] = ui.nonpawn_key[BLACK];
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
            if (can_castle_kingside(*this, WHITE)
                && !(all_occ & ((sq_bb(F1) | sq_bb(G1))))
                && !is_square_attacked(E1, BLACK)
                && !is_square_attacked(F1, BLACK)
                && !is_square_attacked(G1, BLACK)) {
                moves.push_back(make_castling(E1, G1));
            }
            if (can_castle_queenside(*this, WHITE)
                && !(all_occ & (sq_bb(B1) | sq_bb(C1) | sq_bb(D1)))
                && !is_square_attacked(E1, BLACK)
                && !is_square_attacked(D1, BLACK)
                && !is_square_attacked(C1, BLACK)) {
                moves.push_back(make_castling(E1, C1));
            }
        } else {
            if (can_castle_kingside(*this, BLACK)
                && !(all_occ & (sq_bb(F8) | sq_bb(G8)))
                && !is_square_attacked(E8, WHITE)
                && !is_square_attacked(F8, WHITE)
                && !is_square_attacked(G8, WHITE)) {
                moves.push_back(make_castling(E8, G8));
            }
            if (can_castle_queenside(*this, BLACK)
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

    if (int(from) < 0 || int(from) >= SQUARE_NB || int(to) < 0 || int(to) >= SQUARE_NB)
        return false;
    const Piece moving = board_sq[from];
    if (moving == NO_PIECE || color_of(moving) != us)
        return false;

    // King moves: destination must not be attacked after king leaves
    if (type_of(moving) == KING) {
        if (mt == CASTLING) {
            const bool king_side = to > from;
            const bool ok_rights = king_side ? can_castle_kingside(*this, us)
                                             : can_castle_queenside(*this, us);
            if (!ok_rights)
                return false;

            const Rank back = us == WHITE ? RANK_1 : RANK_8;
            if (us == WHITE && from != E1)
                return false;
            if (us == BLACK && from != E8)
                return false;

            if (king_side) {
                const Square f = make_square(FILE_F, back);
                const Square g = make_square(FILE_G, back);
                return !(all_occ & (sq_bb(f) | sq_bb(g)))
                    && !is_square_attacked(from, them)
                    && !is_square_attacked(f, them)
                    && !is_square_attacked(g, them);
            }

            const Square b = make_square(FILE_B, back);
            const Square c = make_square(FILE_C, back);
            const Square d = make_square(FILE_D, back);
            return !(all_occ & (sq_bb(b) | sq_bb(c) | sq_bb(d)))
                && !is_square_attacked(from, them)
                && !is_square_attacked(d, them)
                && !is_square_attacked(c, them);
        }
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
static void gen_legal_impl(const Board& b, MoveList& ml, bool caps_only, bool quiets_only) {
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
        else if (quiets_only) targets &= empty;
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
            else if (quiets_only) dests &= empty;
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
            else if (quiets_only) dests &= empty;
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
            else if (quiets_only) dests &= empty;
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
            else if (quiets_only) dests &= empty;
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
        if (!quiets_only) {
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
        if (!quiets_only) {
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
        if (!quiets_only) {
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
        if (!quiets_only) {
            Bitboard ce = cap_e(free_pawns & ~PromRank) & them_bb & check_mask;
            while (ce) {
                Square to = Square(pop_lsb(ce));
                ml.push(make_move(Square(int(to) - CapEOff), to));
            }
        }
        // Pawn captures west (non-promo)
        if (!quiets_only) {
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
                    if (is_promo && !quiets_only) {
                        ml.push(make_promotion(from, to1, QUEEN));
                        ml.push(make_promotion(from, to1, ROOK));
                        ml.push(make_promotion(from, to1, BISHOP));
                        ml.push(make_promotion(from, to1, KNIGHT));
                    } else if (!is_promo) {
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
            if (!quiets_only) {
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
        }

        // --- En passant ---
        if (!quiets_only && b.ep_sq != SQ_NONE) {
            Square ep       = b.ep_sq;
            Square ep_pawn  = Square(int(ep) - PushOff);  // captured pawn square
            // Filter: EP must either land on check_mask or capture the checking pawn
            if ((sq_bb(ep) | sq_bb(ep_pawn)) & check_mask) {
                Bitboard ep_atk = PawnAttacks[Them][ep] & pawns;
                while (ep_atk) {
                    Square   from      = Square(pop_lsb(ep_atk));
                    // Legality: removing both pawns from occ might expose the king.
                    // The captured pawn must also be excluded from pawn attacks; this
                    // matters when en passant is the only way to answer a pawn check.
                    Bitboard occ_after = (occ ^ sq_bb(from) ^ sq_bb(ep_pawn)) | sq_bb(ep);
                    Bitboard them_pawns_after = b.pieces[Them][PAWN] & ~sq_bb(ep_pawn);
                    bool king_attacked =
                        (PawnAttacks[Us][ksq] & them_pawns_after)
                     || (KnightAttacks[ksq] & b.pieces[Them][KNIGHT])
                     || (bishop_attacks(ksq, occ_after) & (b.pieces[Them][BISHOP] | b.pieces[Them][QUEEN]))
                     || (rook_attacks(ksq, occ_after) & (b.pieces[Them][ROOK] | b.pieces[Them][QUEEN]))
                     || (KingAttacks[ksq] & b.pieces[Them][KING]);
                    if (!king_attacked)
                        ml.push(make_ep(from, ep));
                }
            }
        }
    }

    // ---- Castling (only when not in check) ----
    if (!caps_only && checkers == 0) {
        if constexpr (Us == WHITE) {
            if (can_castle_kingside(b, WHITE)
                && !(occ & (sq_bb(F1) | sq_bb(G1)))
                && !b.attackers_to(F1, occ, Them)
                && !b.attackers_to(G1, occ, Them))
                ml.push(make_castling(E1, G1));
            if (can_castle_queenside(b, WHITE)
                && !(occ & (sq_bb(B1) | sq_bb(C1) | sq_bb(D1)))
                && !b.attackers_to(D1, occ, Them)
                && !b.attackers_to(C1, occ, Them))
                ml.push(make_castling(E1, C1));
        } else {
            if (can_castle_kingside(b, BLACK)
                && !(occ & (sq_bb(F8) | sq_bb(G8)))
                && !b.attackers_to(F8, occ, Them)
                && !b.attackers_to(G8, occ, Them))
                ml.push(make_castling(E8, G8));
            if (can_castle_queenside(b, BLACK)
                && !(occ & (sq_bb(B8) | sq_bb(C8) | sq_bb(D8)))
                && !b.attackers_to(D8, occ, Them)
                && !b.attackers_to(C8, occ, Them))
                ml.push(make_castling(E8, C8));
        }
    }
}

void Board::gen_legal(MoveList& ml) const {
    if (side_to_move == WHITE) gen_legal_impl<WHITE>(*this, ml, false, false);
    else                       gen_legal_impl<BLACK>(*this, ml, false, false);
}

void Board::gen_legal_captures(MoveList& ml) const {
    if (side_to_move == WHITE) gen_legal_impl<WHITE>(*this, ml, true, false);
    else                       gen_legal_impl<BLACK>(*this, ml, true, false);
}

void Board::gen_legal_quiets(MoveList& ml) const {
    if (side_to_move == WHITE) gen_legal_impl<WHITE>(*this, ml, false, true);
    else                       gen_legal_impl<BLACK>(*this, ml, false, true);
}

Bitboard Board::check_squares(PieceType pt, Color us) const {
    const Square ksq = king_sq[~us];
    switch (pt) {
        case PAWN:   return PawnAttacks[~us][ksq];
        case KNIGHT: return KnightAttacks[ksq];
        case BISHOP: return bishop_attacks(ksq, all_occ);
        case ROOK:   return rook_attacks(ksq, all_occ);
        case QUEEN:  return queen_attacks(ksq, all_occ);
        default:     return 0;
    }
}

bool Board::gives_check(Move m) const {
    Square from      = from_sq(m);
    Square to        = to_sq(m);
    Color  us        = side_to_move;
    Color  them      = ~us;
    Square their_king = king_sq[them];

    if (move_type(m) == CASTLING) {
        const bool king_side = to > from;
        const Square rook_from = king_side ? make_square(FILE_H, rank_of(from))
                                           : make_square(FILE_A, rank_of(from));
        const Square rook_to   = king_side ? make_square(FILE_F, rank_of(from))
                                           : make_square(FILE_D, rank_of(from));
        Bitboard occ_after = all_occ ^ sq_bb(from) ^ sq_bb(to)
                            ^ sq_bb(rook_from) ^ sq_bb(rook_to);
        return (rook_attacks(rook_to, occ_after) & sq_bb(their_king)) != 0;
    }

    // Piece type after the move (promotion changes the type)
    PieceType pt;
    if (move_type(m) == PROMOTION)  pt = promo_type(m);
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
    if (is_insufficient_material()) return true;

    // 50-move rule
    if (halfmove_clock >= 100) return true;
    if (halfmove_clock < 4) return false;

    // 2-fold repetition (search back through history)
    int reps = 0;
    int stop = std::max(0, history_size - halfmove_clock);
    for (int i = history_size - 2; i >= stop; i -= 2) {
        if (history[static_cast<size_t>(i)].hash == hash) {
            reps++;
            if (reps >= 1) return true; // 2-fold (current + one prior)
        }
    }
    return false;
}

bool Board::is_repetition(int search_ply) const {
    if (halfmove_clock < 4)
        return false;

    int reps = 0;
    const int reversible = std::min(halfmove_clock, plies_from_null);
    const int stop = std::max(0, history_size - reversible);

    for (int i = history_size - 2; i >= stop; i -= 2) {
        if (history[static_cast<size_t>(i)].hash != hash)
            continue;

        const int distance = history_size - i;
        if (distance <= search_ply)
            return true;

        if (++reps >= 2)
            return true;
    }

    return false;
}

bool Board::is_draw(int search_ply) const {
    if (is_insufficient_material()) return true;
    if (halfmove_clock >= 100) return true;
    return is_repetition(search_ply);
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
