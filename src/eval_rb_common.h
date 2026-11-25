#pragma once

// The rebuilt evaluation's shared ground: its units and structure constants,
// the bitboard vocabulary its families are written in, and the evaluation's
// working state. Every quantity here is in the classical form's units (a
// pawn is 124 in the middlegame, 206 in the endgame) until the value is
// converted to centipawns once, at the end (eval_rb.h).
// Part of the evaluator's single translation unit: included by src/eval.cpp
// alone when BASILISK_EVAL_REBUILD is defined. Include it nowhere else.

#include "eval_common.h"

// ---- Structure constants ------------------------------------------------------
// Fixed, not fitted: no static fit prices them. The non-pawn values below are
// the reference's middlegame piece values, read wherever the form counts
// non-pawn material (the phase, the lazy thresholds, the space threshold, the
// general scale heuristics, complexity); the material slots never move them.
// ---- The Texel trace ------------------------------------------------------------
// A traced build (TEXEL_TRACE) counts every linear slot's coefficient, White
// minus Black, as it enters the score: RB_TR2 for a slot pair XMg/XEg read
// together, TR_MG or TR_EG for a single half. The knobs' terms go to g_rest
// by source. It evaluates past the lazy exits (rb::FULL).
#ifdef TEXEL_TRACE
#define RB_TR2(group, idx, n) do { TR_MG(group##Mg, (idx), (n)); TR_EG(group##Eg, (idx), (n)); } while (0)
#define RB_REST(source, mg_, eg_) do { g_rest.mg[REST_##source] += (mg_); g_rest.eg[REST_##source] += (eg_); } while (0)
#else
#define RB_TR2(group, idx, n) (void)0
#define RB_REST(source, mg_, eg_) (void)0
#endif

namespace rb {

#ifdef TEXEL_TRACE
inline constexpr bool TRACED = true;
#else
inline constexpr bool TRACED = false;
#endif

// The sign a colour's coefficients enter the trace with.
template<Color Us> inline constexpr int SIGN = Us == WHITE ? 1 : -1;

inline constexpr int NPM_KNIGHT = 781;
inline constexpr int NPM_BISHOP = 825;
inline constexpr int NPM_ROOK   = 1276;
inline constexpr int NPM_QUEEN  = 2538;

inline constexpr int MIDGAME_LIMIT = 15258;
inline constexpr int ENDGAME_LIMIT = 3915;
inline constexpr int PHASE_MIDGAME = 128;

// ---- The score pair ---------------------------------------------------------------
// A family's contribution in White's view, as the reference's packed Score
// but held as two ints: the game positions never reach the 16-bit halves'
// range, where the packed form would wrap.
struct Score {
    int mg = 0;
    int eg = 0;
    constexpr Score() = default;
    constexpr Score(int m, int e) : mg(m), eg(e) {}
    constexpr Score& operator+=(Score o) { mg += o.mg; eg += o.eg; return *this; }
    constexpr Score& operator-=(Score o) { mg -= o.mg; eg -= o.eg; return *this; }
    friend constexpr Score operator+(Score a, Score b) { return {a.mg + b.mg, a.eg + b.eg}; }
    friend constexpr Score operator-(Score a, Score b) { return {a.mg - b.mg, a.eg - b.eg}; }
    friend constexpr Score operator-(Score a) { return {-a.mg, -a.eg}; }
    friend constexpr Score operator*(Score a, int k) { return {a.mg * k, a.eg * k}; }
    friend constexpr bool operator==(Score a, Score b) = default;
};

// ---- Bitboard vocabulary ------------------------------------------------------------
inline constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
inline constexpr Bitboard RANK_1_BB = 0xFFULL;
inline constexpr Bitboard file_bb(int f) { return FILE_A_BB << f; }
inline constexpr Bitboard rank_bb(int r) { return RANK_1_BB << (8 * r); }
inline constexpr Bitboard file_bb_of(Square s) { return file_bb(file_of(s)); }
inline constexpr Bitboard rank_bb_of(Square s) { return rank_bb(rank_of(s)); }

inline constexpr Bitboard QUEEN_SIDE   = file_bb(0) | file_bb(1) | file_bb(2) | file_bb(3);
inline constexpr Bitboard CENTER_FILES = file_bb(2) | file_bb(3) | file_bb(4) | file_bb(5);
inline constexpr Bitboard KING_SIDE    = file_bb(4) | file_bb(5) | file_bb(6) | file_bb(7);
inline constexpr Bitboard CENTER       = (file_bb(3) | file_bb(4)) & (rank_bb(3) | rank_bb(4));
inline constexpr Bitboard DARK_SQUARES = EVAL_DARK_SQUARES;

// The files a king's flank attack and defence are counted on.
inline constexpr Bitboard KING_FLANK[FILE_NB] = {
    QUEEN_SIDE ^ file_bb(3), QUEEN_SIDE, QUEEN_SIDE,
    CENTER_FILES, CENTER_FILES,
    KING_SIDE, KING_SIDE, KING_SIDE ^ file_bb(4)
};

inline constexpr int edge_distance(int f) { return f < 7 - f ? f : 7 - f; }
inline int file_distance(Square a, Square b) { return std::abs(int(file_of(a)) - int(file_of(b))); }
inline int rank_distance(Square a, Square b) { return std::abs(int(rank_of(a)) - int(rank_of(b))); }
inline int distance(Square a, Square b) { return KING_DIST[a][b]; }

// The ranks strictly ahead of s for c, and the parts of them the pawn
// families read.
inline Bitboard forward_ranks(Color c, Square s) { return BB_FORWARD_RANKS[c][rank_of(s)]; }
inline Bitboard forward_file(Color c, Square s) { return forward_ranks(c, s) & file_bb_of(s); }
inline Bitboard adjacent_files(Square s) { return BB_ADJACENT_FILES[file_of(s)]; }
inline Bitboard pawn_attack_span(Color c, Square s) { return forward_ranks(c, s) & adjacent_files(s); }
inline Bitboard passed_pawn_span(Color c, Square s) { return pawn_attack_span(c, s) | forward_file(c, s); }

template<Color C> inline Bitboard pawn_attacks(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}
template<Color C> inline Bitboard pawn_double_attacks(Bitboard b) {
    return C == WHITE ? shift<NORTH_WEST>(b) & shift<NORTH_EAST>(b)
                      : shift<SOUTH_WEST>(b) & shift<SOUTH_EAST>(b);
}
template<Color C> inline Bitboard shift_up(Bitboard b) {
    return C == WHITE ? shift<NORTH>(b) : shift<SOUTH>(b);
}
template<Color C> inline Bitboard shift_down(Bitboard b) {
    return C == WHITE ? shift<SOUTH>(b) : shift<NORTH>(b);
}

// The most advanced square of b from c's side.
inline Square frontmost(Color c, Bitboard b) { return Square(c == WHITE ? msb(b) : lsb(b)); }

inline Bitboard pieces(const Board& b, PieceType pt) {
    return b.piece_bb(WHITE, pt) | b.piece_bb(BLACK, pt);
}
inline int count(const Board& b, Color c, PieceType pt) { return popcount(b.piece_bb(c, pt)); }

// Non-pawn material in the fixed values above.
inline int non_pawn_material(const Board& b, Color c) {
    return count(b, c, KNIGHT) * NPM_KNIGHT + count(b, c, BISHOP) * NPM_BISHOP
         + count(b, c, ROOK) * NPM_ROOK + count(b, c, QUEEN) * NPM_QUEEN;
}
inline int non_pawn_material(const Board& b) {
    return non_pawn_material(b, WHITE) + non_pawn_material(b, BLACK);
}

} // namespace rb
