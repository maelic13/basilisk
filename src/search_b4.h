#pragma once

// Cluster 3's arithmetic, kept free of the kernel so tests read it directly.
// The kernel calls these on the B4_QSEARCH arm, and pricing on either arm.

#include "move.h"

namespace b4 {

// A capture that finishes an exchange: it lands where the previous move did.
// A null move or the start of the line has no such square.
[[nodiscard]] inline bool is_recapture(Move m, Move previous) {
    return previous != MOVE_NONE && previous != MOVE_NULL && to_sq(m) == to_sq(previous);
}

}  // namespace b4
