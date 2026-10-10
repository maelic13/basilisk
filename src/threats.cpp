#include "threats.h"

#include "attacks.h"
#include "bitboard.h"

Threats threats_against(const Board& b, Color us) {
    const Color them = ~us;
    const Bitboard occ = b.all_pieces();

    const Bitboard pawns = b.piece_bb(them, PAWN);
    Threats t;
    t.by_pawn = them == WHITE
              ? shift<NORTH_EAST>(pawns) | shift<NORTH_WEST>(pawns)
              : shift<SOUTH_EAST>(pawns) | shift<SOUTH_WEST>(pawns);

    Bitboard minor = 0;
    for (Bitboard bb = b.piece_bb(them, KNIGHT); bb; )
        minor |= KnightAttacks[pop_lsb(bb)];
    for (Bitboard bb = b.piece_bb(them, BISHOP); bb; )
        minor |= bishop_attacks(Square(pop_lsb(bb)), occ);
    t.by_minor = minor | t.by_pawn;

    Bitboard rook = 0;
    for (Bitboard bb = b.piece_bb(them, ROOK); bb; )
        rook |= rook_attacks(Square(pop_lsb(bb)), occ);
    t.by_rook = rook | t.by_minor;

    Bitboard rest = KingAttacks[b.king_square(them)];
    for (Bitboard bb = b.piece_bb(them, QUEEN); bb; )
        rest |= queen_attacks(Square(pop_lsb(bb)), occ);
    t.all = rest | t.by_rook;
    return t;
}
