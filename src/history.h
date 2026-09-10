#pragma once

// Search history tables — the engine's move-ordering memory (8.6.10b).
//
// Extracted from Searcher, where twelve raw table members and their
// clear/age/blend rules were interleaved with search state (the Rarog 11.0(i)
// "history module" shape). Everything that persists ACROSS searches and feeds
// move ordering or eval correction lives here; per-search state stays in
// Searcher. The POLICY layer — bonus formulas, which tables a cutoff trains,
// SearchParams coupling — deliberately stays in search.cpp: this struct owns
// storage and whole-table lifecycle only, so the Phase-10 history work edits
// policy without touching layout.
//
// Layout note: the two large tables (continuation ~392 KB each, pawn ~1.7 MB)
// are heap-allocated so a Searcher stays cheaply constructible; the rest are
// inline members. One HistoryTables per search thread; never shared.

#include "types.h"
#include "move.h"

#include <cstdint>
#include <memory>

struct HistoryTables {
    // ---- Saturation bounds (hist_update gravity) ----
    static constexpr int MAX_MAIN_HIST = 16384;
    static constexpr int MAX_CAP_HIST  = 16384;
    static constexpr int MAX_CONT_HIST = 16384;
    static constexpr int MAX_PAWN_HIST = 16384;
    static constexpr int MAX_LOW_HIST  = 8192;

    // ---- Table sizes ----
    static constexpr int CORR_SIZE            = 16384;
    static constexpr int PAWN_HIST_SIZE       = 2048;
    static constexpr int LOW_PLY_HISTORY_SIZE = 8;

    // 8.6.2a: correction and pawn history are indexed `key & (SIZE - 1)`.
    // A non-power-of-two size would alias silently — a wrong-position
    // correction value, not a crash — so pin it at build time.
    static_assert((CORR_SIZE & (CORR_SIZE - 1)) == 0,
                  "CORR_SIZE must be a power of two: indexed with & (SIZE - 1)");
    static_assert((PAWN_HIST_SIZE & (PAWN_HIST_SIZE - 1)) == 0,
                  "PAWN_HIST_SIZE must be a power of two: indexed with & (SIZE - 1)");

    // Quiet history [color][from][to]
    int16_t main[NCOLORS][SQUARE_NB][SQUARE_NB];

    // Capture history [attacker_pt][to][captured_pt]
    int16_t capture[PIECE_TYPE_NB][SQUARE_NB][PIECE_TYPE_NB];

    // Continuation history: heap-allocated (~400 KB each)
    struct ContHistTable {
        int16_t data[PIECE_TYPE_NB][SQUARE_NB][PIECE_TYPE_NB][SQUARE_NB];
    };
    std::unique_ptr<ContHistTable> cont1; // 1-ply continuation
    std::unique_ptr<ContHistTable> cont2; // 2-ply continuation
    std::unique_ptr<ContHistTable> cont4; // 4-ply continuation

    // Pawn-structure keyed quiet history [pawn_key][piece][to]
    struct PawnHistTable {
        int16_t data[PAWN_HIST_SIZE][PIECE_TYPE_NB][SQUARE_NB];
    };
    std::unique_ptr<PawnHistTable> pawn;

    // Low-ply quiet history improves opening/root move ordering.
    int16_t low_ply[LOW_PLY_HISTORY_SIZE][SQUARE_NB][SQUARE_NB];

    // Countermove [from][to] -> best response
    Move countermove[SQUARE_NB][SQUARE_NB];

    // Correction histories keyed by pawn, minor-piece, non-pawn, and
    // continuation context.
    int16_t pawn_corr[NCOLORS][CORR_SIZE];
    int16_t minor_corr[NCOLORS][CORR_SIZE];
    int16_t nonpawn_corr[NCOLORS][NCOLORS][CORR_SIZE];
    int16_t cont_corr[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];

    HistoryTables();

    void clear();                                // zero everything (ucinewgame)
    void age();                                  // halve everything between searches
};
