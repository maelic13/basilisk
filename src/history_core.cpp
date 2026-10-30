#include "history_core.h"

CoreHistoryTables::CoreHistoryTables()
    : continuation(std::make_unique<ContinuationHistory>())
    , pawn(std::make_unique<PawnHistory>())
    , cont_corr(std::make_unique<ContinuationCorrection>()) {
    clear();
}

void CoreHistoryTables::clear(bool donor_fills) {
    const auto fill = [donor_fills](int donor) { return donor_fills ? donor : 0; };
    fill_entries(main.entry, fill(-5));
    fill_entries(capture.entry, fill(-742));
    fill_entries(low_ply.entry, 0);
    tt_move.value = 0;
    for (auto& a : continuation->row)
        for (auto& b : a)
            for (auto& c : b)
                for (auto& row : c)
                    fill_entries(row.entry, fill(-586));
    for (auto& bucket : pawn->bucket)
        fill_entries(bucket.entry, fill(-1338));
    fill_entries(pawn_corr, fill(-5));
    fill_entries(minor_corr, fill(-5));
    fill_entries(nonpawn_corr, fill(-5));
    for (auto& a : cont_corr->row)
        for (auto& row : a)
            fill_entries(row.entry, fill(5));
}

void CoreHistoryTables::age(int main_num) {
    scale_entries(main.entry, main_num, 1024);
    fill_entries(low_ply.entry, LOW_PLY_REFILL);
}
