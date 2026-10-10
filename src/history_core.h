#pragma once

// The selectivity core's history tables (the B2_CORE arm), one set per search
// thread. Each table is a type that owns its storage, its saturation bound and
// its whole-table lifecycle (clear and per-search ageing), so the tables can be
// tested without a Searcher; what trains them, and by how much, is the update
// policy in search_history_core.cpp.
//
// Pieces are indexed by type: every consumer knows the side to move, so the
// donor's colour-qualified piece index would only duplicate it. The main
// history is [colour][from][to]: promotions share their from-to, which differs
// from keying the whole move only for under-promotions.

#include "types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>

// History gravity: a bonus, first clamped to [-Limit, Limit], pulls the entry
// toward the bound of its sign by the bonus's share of the remaining distance,
// so repeated bonuses saturate at the bound instead of overflowing.
template<int Limit>
struct HistoryBound {
    static_assert(Limit > 0 && Limit <= 32767, "an entry is a signed 16-bit value");
    static constexpr int LIMIT = Limit;
    static void update(int16_t& e, int bonus) {
        const int b = std::clamp(bonus, -Limit, Limit);
        e = static_cast<int16_t>(e + b - e * std::abs(b) / Limit);
    }
};

// Whole-table helpers over nested int16 arrays.
inline void fill_entries(int16_t& e, int value) { e = static_cast<int16_t>(value); }
template<class T, size_t N>
void fill_entries(T (&a)[N], int value) { for (auto& x : a) fill_entries(x, value); }

inline void scale_entries(int16_t& e, int num, int den) { e = static_cast<int16_t>(e * num / den); }
template<class T, size_t N>
void scale_entries(T (&a)[N], int num, int den) { for (auto& x : a) scale_entries(x, num, den); }

// Quiet moves by colour and from-to (the butterfly table).
struct MainHistory : HistoryBound<7183> {
    int16_t entry[NCOLORS][SQUARE_NB][SQUARE_NB];
};

// Captures by attacking piece type, destination and captured piece type.
struct CaptureHistory : HistoryBound<10692> {
    int16_t entry[PIECE_TYPE_NB][SQUARE_NB][PIECE_TYPE_NB];
};

// A [piece type][to] row: one continuation-history leaf, selected by an
// earlier move and read for the move being scored.
struct PieceToHistory : HistoryBound<30000> {
    int16_t entry[PIECE_TYPE_NB][SQUARE_NB];
};

// Continuation history: the row an earlier move selects, by whether that move
// was made in check and whether it was from the capture stage, its piece type
// and its destination. Row [0][0][NO_PIECE_TYPE][A1] is the sentinel that
// records without a move point to; no update ever reaches it.
struct ContinuationHistory {
    PieceToHistory row[2][2][PIECE_TYPE_NB][SQUARE_NB];
};

// Quiet moves by pawn structure: a bucket per pawn-key hash, each a
// [piece type][to] row.
struct PawnHistoryBucket : HistoryBound<8192> {
    int16_t entry[PIECE_TYPE_NB][SQUARE_NB];
};
struct PawnHistory {
    static constexpr int MAX_BUCKETS_LOG2 = 13;
    static constexpr int MAX_BUCKETS = 1 << MAX_BUCKETS_LOG2;
    PawnHistoryBucket bucket[MAX_BUCKETS];
};

// Quiet moves at the first plies of the search, by ply and from-to.
struct LowPlyHistory : HistoryBound<7183> {
    static constexpr int PLIES = 5;
    int16_t entry[PLIES][SQUARE_NB][SQUARE_NB];
};

// How often the TT move turned out best.
struct TTMoveHistory : HistoryBound<8192> {
    int16_t value;
};

// Correction histories: the error of the static evaluation against the
// search, by structure and by move pair, in evaluation units.
using CorrectionBound = HistoryBound<1024>;

// A [piece type][to] correction row, selected by an earlier move.
struct PieceToCorrection : HistoryBound<1024> {
    int16_t entry[PIECE_TYPE_NB][SQUARE_NB];
};

// Continuation correction: the row a move selects, by its piece type and
// destination. Row [NO_PIECE_TYPE][A1] is the one records without a move
// point to.
struct ContinuationCorrection {
    PieceToCorrection row[PIECE_TYPE_NB][SQUARE_NB];
};

// The storage owner: every table one search thread trains, and their
// lifecycle. The two large tables are on the heap so a Searcher stays cheap
// to construct.
struct CoreHistoryTables {
    // Correction histories keyed by pawn, minor-piece and per-colour
    // non-pawn structure, by side to move; the continuation correction is
    // keyed by move pairs.
    static constexpr int CORR_SIZE = 16384;
    static_assert((CORR_SIZE & (CORR_SIZE - 1)) == 0, "indexed with & (CORR_SIZE - 1)");

    MainHistory    main;
    CaptureHistory capture;
    LowPlyHistory  low_ply;
    TTMoveHistory  tt_move;
    std::unique_ptr<ContinuationHistory>    continuation;
    std::unique_ptr<PawnHistory>            pawn;
    std::unique_ptr<ContinuationCorrection> cont_corr;

    int16_t pawn_corr[NCOLORS][CORR_SIZE];
    int16_t minor_corr[NCOLORS][CORR_SIZE];
    int16_t nonpawn_corr[NCOLORS][NCOLORS][CORR_SIZE];

    CoreHistoryTables();

    // Every entry to zero (ucinewgame).
    void clear();
    // Halves every table except the TT-move history, between searches.
    void halve();

    [[nodiscard]] PieceToHistory* sentinel_row() {
        return &continuation->row[0][0][NO_PIECE_TYPE][0];
    }
    [[nodiscard]] PieceToCorrection* sentinel_corr_row() {
        return &cont_corr->row[NO_PIECE_TYPE][0];
    }
};
