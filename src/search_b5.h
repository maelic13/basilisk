#pragma once

// Cluster 4's arithmetic: the root's record and its window, as pure functions
// the driver, the kernel and the tests share.

#include "search_root.h"
#include <algorithm>
#include <cstdint>

namespace b5 {

// One root move's weights in the running average and the running mean square,
// in 32nds: the more of the move's effort this search was, the more it counts.
// `nodes` is what this search of the move cost, `earlier` what its searches
// cost before it.
struct EmaWeights {
    int average;
    int mean_sq;
};

[[nodiscard]] inline EmaWeights ema_weights(int64_t nodes, int64_t earlier) {
    const int64_t e = std::max<int64_t>(earlier, 1);
    const int w = static_cast<int>(std::clamp<int64_t>(32 * nodes * 2 / (nodes * 2 + 3 * e), 12, 24));
    return {w, std::min(w, 16)};
}

[[nodiscard]] inline int blend_average(int average, int value, int weight) {
    if (average == kRootScoreNone)
        return value;
    return (value * weight + average * (32 - weight)) / 32;
}

[[nodiscard]] inline int64_t blend_mean_sq(int64_t mean_sq, int value, int weight) {
    const int64_t square = int64_t(value) * (value < 0 ? -value : value);
    if (mean_sq == kRootMeanSqNone)
        return square;
    return (square * weight + mean_sq * (32 - weight)) / 32;
}

// The record's order: by score, then by the last iteration's score. Every move
// but the line's best holds the "none" score, so a stable sort keeps the rest in
// the order they had.
[[nodiscard]] inline bool root_move_before(const RootMove& a, const RootMove& b) {
    return a.score != b.score ? a.score > b.score : a.previous_score > b.previous_score;
}

}  // namespace b5
