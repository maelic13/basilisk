#pragma once

// Cluster 3's arithmetic, kept free of the kernel so tests read it directly.
// The kernel calls these on the B4_QSEARCH arm, and pricing on either arm.

#include "move.h"
#include "search_types.h"

namespace b4 {

// A capture that finishes an exchange: it lands where the previous move did.
// A null move or the start of the line has no such square.
[[nodiscard]] inline bool is_recapture(Move m, Move previous) {
    return previous != MOVE_NONE && previous != MOVE_NULL && to_sq(m) == to_sq(previous);
}

// A fail high pulled toward the bound it proved, `weight` 1024ths of the value
// and the rest beta. The weights sum to 1024 and the division truncates toward
// zero, so the result stays at or above beta for a value above it. A decisive
// value is a proof, not an estimate, and stays as it is.
[[nodiscard]] inline int fail_high_value(int value, int beta, int weight) {
    if (value <= beta || is_decisive(value))
        return value;
    return (weight * value + (1024 - weight) * beta) / 1024;
}

// Whether a depth is past reverse futility's cap in a search that drives for
// mate (Cluster 2's seek_mate): a cut there would spend the depth the drive
// needs on the estimate's word.
[[nodiscard]] inline bool rfp_past_seek_mate_cap(bool seek_mate, int depth, int cap) {
    return seek_mate && depth > cap;
}

}  // namespace b4
