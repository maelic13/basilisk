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

// The singular search's bound: the TT score less a margin of `per_ply`
// (more `tt_pv_per_ply` at a non-PV node near a PV) per ply of depth, in
// 1/63 of an evaluation unit, with the donor's integer division.
[[nodiscard]] inline int singular_beta(int tt_score, int depth, bool tt_pv_off_pv, int per_ply,
                                       int tt_pv_per_ply) {
    return tt_score - (per_ply + tt_pv_per_ply * int(tt_pv_off_pv)) * depth / 63;
}

// The graded extension: one ply, a second when the exclusion search fell
// below the singular bound by more than `double_margin`, a third by more than
// `triple_margin`.
[[nodiscard]] inline int graded_extension(int value, int s_beta, int double_margin,
                                          int triple_margin) {
    return 1 + int(value < s_beta - double_margin) + int(value < s_beta - triple_margin);
}

// Multi-cut: the exclusion search failed high over the node's beta, so the
// node is pruned. `bound_rule` 0 is the donor's (the searched value clears
// beta), 1 the alternative (the singular bound itself clears beta); neither
// admits a decisive value.
[[nodiscard]] inline bool multicut_admits(int bound_rule, int value, int s_beta, int beta,
                                          bool value_decisive) {
    if (value_decisive)
        return false;
    return bound_rule == 0 ? value >= beta : s_beta >= beta;
}

// The negative extension, when the move is neither singular nor a multi-cut:
// the TT score proves the node fails high, or the node is an expected cut.
[[nodiscard]] inline bool negative_extension(int tt_score, int beta, bool cut_node) {
    return tt_score >= beta || cut_node;
}

// A quiet (not capture-stage) move shuffling a piece back and forth late in a
// reversible stretch: it moves from where this side's last move landed, which
// came from where the move before it landed. The donor's non-regression
// guard, ported as written and never tuned.
[[nodiscard]] inline bool is_shuffling(bool capture_stage, int rule50, int plies_from_null,
                                       int ply, Move m, Move prev2, Move prev4) {
    if (capture_stage || rule50 < 10)
        return false;
    if (plies_from_null < 6 || ply < 20)
        return false;
    return from_sq(m) == to_sq(prev2) && from_sq(prev2) == to_sq(prev4);
}

// A deep iteration whose root score is decisive enough that the search
// drives for mate: singular extensions stand down.
[[nodiscard]] inline bool seek_mate(int root_depth, int root_score, int min_depth, int min_score) {
    return root_depth >= min_depth && (root_score >= min_score || root_score <= -min_score);
}

}  // namespace b3
