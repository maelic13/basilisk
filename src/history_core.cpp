#include "history_core.h"

CoreHistoryTables::CoreHistoryTables()
    : continuation(std::make_unique<ContinuationHistory>())
    , pawn(std::make_unique<PawnHistory>())
    , cont_corr(std::make_unique<ContinuationCorrection>()) {
    clear();
}

void CoreHistoryTables::clear() {
    fill_entries(main.entry, 0);
    fill_entries(capture.entry, 0);
    fill_entries(low_ply.entry, 0);
    tt_move.value = 0;
    for (auto& a : continuation->row)
        for (auto& b : a)
            for (auto& c : b)
                for (auto& row : c)
                    fill_entries(row.entry, 0);
    for (auto& bucket : pawn->bucket)
        fill_entries(bucket.entry, 0);
    fill_entries(pawn_corr, 0);
    fill_entries(minor_corr, 0);
    fill_entries(nonpawn_corr, 0);
    for (auto& a : cont_corr->row)
        for (auto& row : a)
            fill_entries(row.entry, 0);
}

void CoreHistoryTables::age(int main_num) {
    scale_entries(main.entry, main_num, 1024);
    fill_entries(low_ply.entry, LOW_PLY_REFILL);
}
