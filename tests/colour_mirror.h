#pragma once
// The colour mirror of a position: colours swapped, ranks flipped, side to
// move flipped, castling rights and the en-passant square mirrored. Its
// evaluation from the mover's view must equal the original's, which is how
// the evaluation tests catch a term that favours one colour.

#include "bitboard.h"
#include "board.h"

#include <string>

inline std::string mirror_fen(const char* fen) {
    Board orig;
    orig.set_fen(fen);

    // Piece placement, mirrored ranks top to bottom: rank r of the mirror is
    // rank 7 - r of the original.
    std::string placement;
    for (int r = 7; r >= 0; r--) {
        int orig_rank = 7 - r;
        int empty = 0;
        for (int f = 0; f < 8; f++) {
            Square sq = make_square(File(f), Rank(orig_rank));
            Piece  p  = orig.piece_on(sq);
            if (p == NO_PIECE) { empty++; continue; }
            if (empty) { placement += static_cast<char>('0' + empty); empty = 0; }
            Color     mc = ~color_of(p);
            PieceType pt = type_of(p);
            static const char piece_chars[] = ".pnbrqk";
            char ch = piece_chars[pt];
            if (mc == WHITE) ch = static_cast<char>(ch - 'a' + 'A');
            placement += ch;
        }
        if (empty) placement += static_cast<char>('0' + empty);
        if (r > 0)  placement += '/';
    }

    char stm = (orig.turn() == WHITE) ? 'b' : 'w';

    std::string castling;
    if (orig.castling() & BK_CASTLE) castling += 'K';
    if (orig.castling() & BQ_CASTLE) castling += 'Q';
    if (orig.castling() & WK_CASTLE) castling += 'k';
    if (orig.castling() & WQ_CASTLE) castling += 'q';
    if (castling.empty()) castling = "-";

    std::string ep = "-";
    if (orig.ep_square() != SQ_NONE) {
        Square mep = flip_rank(orig.ep_square());
        static const char files[] = "abcdefgh";
        static const char ranks[] = "12345678";
        ep  = std::string() + files[file_of(mep)] + ranks[rank_of(mep)];
    }

    return placement + " " + stm + " " + castling + " " + ep
         + " " + std::to_string(orig.rule50_count())
         + " " + std::to_string(orig.fullmove());
}

inline std::string mirror_fen(const std::string& fen) { return mirror_fen(fen.c_str()); }
