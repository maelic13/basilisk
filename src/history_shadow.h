#pragma once

// Diagnostic builds only: how much of what the search reads from the
// colourless history tables was trained by the other colour. The capture,
// continuation and pawn histories and the continuation correction index a
// move by piece type, so White's NxB on f6 and Black's share one entry. A
// shadow cell per entry counts the updates each colour made; a read by the
// side to move then records whether the entry it got was trained by its own
// colour, the other one, or both. Never an input to a decision.

#include "types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class ShadowTable : int { Capture, Continuation, Pawn, ContCorr, Count };

struct CrossColourReads {
    int64_t reads = 0;            // reads of an entry some colour has trained
    int64_t other_only = 0;       // ... trained by the other colour alone
    int64_t mixed = 0;            // ... trained by both colours
    int64_t other_share_ppm = 0;  // sum of the other colour's share of updates
};

class CrossColourShadow {
public:
    CrossColourShadow(size_t capture, size_t continuation, size_t pawn, size_t cont_corr) {
        cells_[int(ShadowTable::Capture)].resize(capture);
        cells_[int(ShadowTable::Continuation)].resize(continuation);
        cells_[int(ShadowTable::Pawn)].resize(pawn);
        cells_[int(ShadowTable::ContCorr)].resize(cont_corr);
    }

    void write(ShadowTable t, size_t index, Color mover) {
        uint16_t& n = cells_[int(t)][index][static_cast<size_t>(mover)];
        if (n < UINT16_MAX)
            ++n;
    }

    void read(ShadowTable t, size_t index, Color reader) {
        const auto& cell = cells_[int(t)][index];
        const int64_t own = cell[static_cast<size_t>(reader)];
        const int64_t other = cell[static_cast<size_t>(~reader)];
        if (own + other == 0)
            return;
        CrossColourReads& r = reads_[int(t)];
        ++r.reads;
        if (own == 0)
            ++r.other_only;
        else if (other > 0)
            ++r.mixed;
        r.other_share_ppm += other * 1'000'000 / (own + other);
    }

    // The cells follow the tables they shadow; the read totals are per search.
    void clear_cells() {
        for (auto& table : cells_)
            for (auto& cell : table)
                cell = {0, 0};
    }
    void reset_reads() {
        for (auto& r : reads_)
            r = CrossColourReads{};
    }
    [[nodiscard]] const CrossColourReads& reads(ShadowTable t) const { return reads_[int(t)]; }

private:
    std::vector<std::array<uint16_t, NCOLORS>> cells_[int(ShadowTable::Count)];
    CrossColourReads reads_[int(ShadowTable::Count)];
};
