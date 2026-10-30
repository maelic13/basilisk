#pragma once

// Cluster 2's arithmetic, kept free of the kernel so tests read it directly:
// the per-line extension budget and the previous-PV predicate. The kernel
// calls these on the B3_PROOF arm only.

#include "move.h"
#include <algorithm>

namespace b3 {

// A positive extension within the line's budget: the plies already taken
// along the line plus this one never exceed `budget` (the root depth). An
// extension past it is truncated, not refused; `truncated` says it was.
[[nodiscard]] inline int budget_extension(int extension, int ext_plies, int budget,
                                          bool& truncated) {
    truncated = false;
    if (extension <= 0)
        return extension;
    const int allowed = std::max(0, budget - ext_plies);
    if (extension > allowed) {
        truncated = true;
        return allowed;
    }
    return extension;
}

// Whether a node follows the previous iteration's PV: the root always; any
// other node when its parent did and the move made from the parent is the
// previous PV's move at the parent's ply.
[[nodiscard]] inline bool follow_pv(bool is_root, bool parent_follows, int ply,
                                    Move parent_move, const Move* prev_pv, int prev_pv_len) {
    if (is_root)
        return true;
    return parent_follows && ply - 1 < prev_pv_len && parent_move == prev_pv[ply - 1];
}

}  // namespace b3
