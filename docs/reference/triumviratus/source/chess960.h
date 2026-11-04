#pragma once
// Chess960 (Fischer Random). See docs/audit_8.0/I_CHESS960.md.
//
// Stage 1 (27/09/2026): data model read by parse_fen (X-FEN and Shredder-FEN).
// Stage 2 (27/09/2026): connected to the engine. The engine encodes castling as before (king towards g1/c1/g8/c8 with the
// castling flag; in 960 the king may also not move, source == destination) and takes the rook's square
// from castle_rook_sq[], which in normal chess is always h1/a1/h8/a8. With g_chess960 off (default, and
// the only state reachable in release) the generation uses the usual code and the tree is identical.
#include "defs.h"

extern bool g_chess960;              // UCI_Chess960

struct Castling960 {
    int rights;                      // bits wk/wq/bk/bq read from the FEN
    int king_from[2];                // king square [white, black]
    int rook_from[4];                // rook square for wk, wq, bk, bq (no_sq if the right is missing)
    int rights_mask[64];             // castle &= rights_mask[sq] for origin and destination of the move
};
extern Castling960 g_c960;

// Castling squares used by the engine (make/unmake, generation, move printing). Normal chess: h1 a1 h8 a8
// and e1 e8. Rewritten by parse_fen: from g_c960 with 960 on, to the normal values otherwise.
extern int castle_rook_sq[4];
extern int castle_king_sq[2];

// Reads the castling field of a FEN (X-FEN: K/Q/k/q = outermost rook on that side; Shredder-FEN:
// A-H/a-h = rook's file) on the board ALREADY loaded. Inconsistent rights (no king on the
// rank, no rook on the file) are discarded. Returns the number of rights discarded.
int c960_parse_castling(const char* field_begin, const char* field_end);

// Carries the squares and the rights mask into the engine: from g_c960 (960) or the normal values (standard).
void c960_apply_to_engine();
void c960_reset_standard();

// Index 0..3 of the right (wk, wq, bk, bq) -> bit.
static inline int c960_bit(int idx) { return 1 << idx; }

// Index of the right from the KING's arrival square (g1, c1, g8, c8), the internal encoding of castling.
static inline int castle_index(int king_to) {
    return king_to == g1 ? 0 : king_to == c1 ? 1 : king_to == g8 ? 2 : 3;
}
static inline int castle_rook_to(int idx) {
    return idx == 0 ? f1 : idx == 1 ? d1 : idx == 2 ? f8 : d8;
}
static inline int castle_king_to(int idx) {
    return idx == 0 ? g1 : idx == 1 ? c1 : idx == 2 ? g8 : c8;
}

// Squares between a and b inclusive, on the same rank (contiguous indices: a8 = 0 ... h1 = 63).
static inline U64 c960_span(int a, int b) {
    const int lo = a < b ? a : b, hi = a < b ? b : a;
    return ((hi == 63 ? 0ULL : (1ULL << (hi + 1))) - (1ULL << lo));
}

// 960 castling rule (as SF). Right idx present; free the squares between the king and its destination and between
// the rook and its destination, king and rook themselves excluded; no square of the king's path attacked, starting
// square included (no castling out of check). The arrival square is checked by the legality test after make, with
// the rook already moved: it also covers the case of the rook that screened an attack along the rank.
template <typename Attacked>
static inline bool c960_can_castle(int castle, int idx, U64 occ, Attacked attacked) {
    if (!(castle & c960_bit(idx))) return false;
    const int ksq = castle_king_sq[idx >> 1], rsq = castle_rook_sq[idx];
    const int kto = castle_king_to(idx), rto = castle_rook_to(idx);
    const U64 movers = (1ULL << ksq) | (1ULL << rsq);
    if (occ & (c960_span(ksq, kto) | c960_span(rsq, rto)) & ~movers) return false;
    U64 path = c960_span(ksq, kto) & ~(1ULL << kto);
    if (ksq == kto) path = 1ULL << ksq;
    while (path) {
        const int sq = get_ls1b_index(path);
        if (attacked(sq)) return false;
        path &= path - 1;
    }
    return true;
}
