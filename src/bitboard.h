#pragma once

#include "types.h"

#include <bit>
#include <cassert>

// ---- Bit intrinsics --------------------------------------------------------
// C++23 <bit> replaces the hand-rolled compiler builtins (8.6.2b). std::countr_zero
// / std::countl_zero / std::popcount are portable and constexpr, and lower to the
// same TZCNT/LZCNT/POPCNT on x86-64 and RBIT+CLZ/CNT on aarch64 — so this is a
// portability and clarity win at identical codegen, not a trade. It also fixes a
// real break: popcount() called __builtin_popcountll unguarded, so the MSVC
// branch the rest of this header carefully maintained could never compile anyway.
//
// lsb/msb have no meaningful answer for an empty bitboard (std::countr_zero(0)
// is a well-defined 64, but 64 is not a square), and sq_bb shifts by its
// argument, which is UB at SQ_NONE. Every caller guards today; the debug
// asserts make that a checked contract rather than a convention — worth adding
// now that the sanitizer/debug suite actually runs (8.6.2b P0). All of them
// compile out entirely under NDEBUG, so release codegen is unchanged.
//
// 8.6.2c: all constexpr, which the <bit> switch is what made possible — the
// compiler builtins were not portably usable in constant expressions. Nothing
// evaluates them at compile time *yet*; this is the prerequisite for the
// consteval table generation deferred to the post-NNUE cleanup, and it costs
// nothing today. (asserts are permitted in constexpr functions: they only
// matter if the function is actually constant-evaluated with a bad argument,
// which is then a compile error rather than a silent bad table.)
[[nodiscard]] inline constexpr int lsb(Bitboard b) {
    assert(b && "lsb() on an empty bitboard");
    return std::countr_zero(b);
}
[[nodiscard]] inline constexpr int msb(Bitboard b) {
    assert(b && "msb() on an empty bitboard");
    return 63 - std::countl_zero(b);
}

inline constexpr int pop_lsb(Bitboard& b) { int s = lsb(b); b &= b - 1; return s; }
[[nodiscard]] inline constexpr int popcount(Bitboard b) { return std::popcount(b); }

// NOT std::has_single_bit: that is false for an empty board, where this must
// also be false. `b & (b - 1)` is correct for 0 and one-bit boards alike.
[[nodiscard]] inline constexpr bool more_than_one(Bitboard b) { return b & (b - 1); }

[[nodiscard]] inline constexpr Bitboard sq_bb(Square s) {
    assert(s >= A1 && s <= H8 && "sq_bb() out of range (SQ_NONE?)");
    return 1ULL << s;
}
[[nodiscard]] inline constexpr Bitboard sq_bb(int s) {
    assert(s >= 0 && s < SQUARE_NB && "sq_bb() out of range");
    return 1ULL << s;
}

// ---- Precomputed tables ----------------------------------------------------
extern Bitboard BB_SQUARES[SQUARE_NB];
extern Bitboard BB_FILES[FILE_NB];
extern Bitboard BB_RANKS[RANK_NB];
extern Bitboard BB_ADJACENT_FILES[FILE_NB];
extern Bitboard BB_FORWARD_RANKS[NCOLORS][RANK_NB];
extern Bitboard BB_PASSED_PAWN_MASK[NCOLORS][SQUARE_NB];
extern Bitboard BB_BETWEEN[SQUARE_NB][SQUARE_NB];
extern Bitboard BB_LINE[SQUARE_NB][SQUARE_NB];
extern int      KING_DIST[SQUARE_NB][SQUARE_NB];

// ---- Directional shift (compile-time direction, no wrap) -------------------
template<Direction D>
constexpr Bitboard shift(Bitboard b) {
    constexpr Bitboard notA = ~0x0101010101010101ULL;
    constexpr Bitboard notH = ~0x8080808080808080ULL;
    if constexpr (D == NORTH)      return b << 8;
    if constexpr (D == SOUTH)      return b >> 8;
    if constexpr (D == EAST)       return (b & notH) << 1;
    if constexpr (D == WEST)       return (b & notA) >> 1;
    if constexpr (D == NORTH_EAST) return (b & notH) << 9;
    if constexpr (D == NORTH_WEST) return (b & notA) << 7;
    if constexpr (D == SOUTH_EAST) return (b & notH) >> 7;
    if constexpr (D == SOUTH_WEST) return (b & notA) >> 9;
    return b;
}

// Call once at startup
void init_bitboards();
