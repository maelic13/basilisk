#include "history.h"

#include <algorithm>
#include <cstring>

HistoryTables::HistoryTables()
    : cont1(std::make_unique<ContHistTable>())
    , cont2(std::make_unique<ContHistTable>())
    , cont4(std::make_unique<ContHistTable>())
    , pawn(std::make_unique<PawnHistTable>()) {
    clear();
}

// 8.6.2b: the `= {}` / std::ranges::fill modernization was tried here and
// REVERTED — raw multi-dimensional C arrays are not assignable, and the
// alternatives (nested std::array, or a flattened span via reinterpret_cast)
// are respectively a large invasive change and strictly worse. std::memset on
// trivially-copyable arrays is the correct tool, not a legacy habit.
void HistoryTables::clear() {
    std::memset(main,         0, sizeof(main));
    std::memset(capture,      0, sizeof(capture));
    std::memset(cont1->data,  0, sizeof(cont1->data));
    std::memset(cont2->data,  0, sizeof(cont2->data));
    std::memset(cont4->data,  0, sizeof(cont4->data));
    std::memset(pawn->data,   0, sizeof(pawn->data));
    std::memset(low_ply,      0, sizeof(low_ply));
    std::memset(countermove,  0, sizeof(countermove));
    std::memset(pawn_corr,    0, sizeof(pawn_corr));
    std::memset(minor_corr,   0, sizeof(minor_corr));
    std::memset(nonpawn_corr, 0, sizeof(nonpawn_corr));
    std::memset(cont_corr,    0, sizeof(cont_corr));
}

// Halve all history values: preserves inter-search learning while decaying
// stale information. Required in its own right — the 8.1b no-aging retry at
// the sibling engine confirmed that without periodic damping history
// saturates and move ordering degrades (its finding, our formula family).
void HistoryTables::age() {
    // Nested ranged-for on purpose: a flattened single-pointer sweep over a
    // multi-dimensional array is formally UB (pointer arithmetic across
    // subobject boundaries), and this runs once per search — not worth even a
    // theoretical soundness hole. countermove is Move-valued, not
    // magnitude-valued, so it is not aged: an entry is either still plausible
    // or gets replaced.
    auto halve2 = [](auto& t) { for (auto& a : t) for (auto& b : a) b /= 2; };
    auto halve3 = [](auto& t) { for (auto& a : t) for (auto& b : a) for (auto& c : b) c /= 2; };
    auto halve4 = [](auto& t) { for (auto& a : t) for (auto& b : a) for (auto& c : b) for (auto& d : c) d /= 2; };
    halve3(main);
    halve3(capture);
    halve4(cont1->data);
    halve4(cont2->data);
    halve4(cont4->data);
    halve3(pawn->data);
    halve3(low_ply);
    halve2(pawn_corr);
    halve2(minor_corr);
    halve3(nonpawn_corr);
    halve3(cont_corr);
}

static void blend_history_value(int16_t& dst, int16_t src) {
    const int value = std::clamp(int(dst) + int(src) / 4, -16384, 16384);
    dst = static_cast<int16_t>(value);
}

// Merge a helper thread's ordering knowledge into this table set (dst +=
// src/4, clamped). Called only after all helpers have stopped — never
// concurrently with search.
void HistoryTables::blend_from(const HistoryTables& other) {
    for (Color c : {WHITE, BLACK})
        for (int from = 0; from < SQUARE_NB; ++from)
            for (int to = 0; to < SQUARE_NB; ++to)
                blend_history_value(main[c][from][to],
                                    other.main[c][from][to]);

    for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
        for (int to = 0; to < SQUARE_NB; ++to)
            for (int cap = 0; cap < PIECE_TYPE_NB; ++cap)
                blend_history_value(capture[pt][to][cap],
                                    other.capture[pt][to][cap]);

    for (int p_pt = 0; p_pt < PIECE_TYPE_NB; ++p_pt) {
        for (int p_to = 0; p_to < SQUARE_NB; ++p_to) {
            for (int c_pt = 0; c_pt < PIECE_TYPE_NB; ++c_pt) {
                for (int c_to = 0; c_to < SQUARE_NB; ++c_to) {
                    blend_history_value(cont1->data[p_pt][p_to][c_pt][c_to],
                                        other.cont1->data[p_pt][p_to][c_pt][c_to]);
                    blend_history_value(cont2->data[p_pt][p_to][c_pt][c_to],
                                        other.cont2->data[p_pt][p_to][c_pt][c_to]);
                    blend_history_value(cont4->data[p_pt][p_to][c_pt][c_to],
                                        other.cont4->data[p_pt][p_to][c_pt][c_to]);
                }
            }
        }
    }

    for (int key = 0; key < PAWN_HIST_SIZE; ++key)
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            for (int to = 0; to < SQUARE_NB; ++to)
                blend_history_value(pawn->data[key][pt][to],
                                    other.pawn->data[key][pt][to]);

    for (int ply = 0; ply < LOW_PLY_HISTORY_SIZE; ++ply)
        for (int from = 0; from < SQUARE_NB; ++from)
            for (int to = 0; to < SQUARE_NB; ++to)
                blend_history_value(low_ply[ply][from][to],
                                    other.low_ply[ply][from][to]);
}
