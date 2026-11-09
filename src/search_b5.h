#pragma once

// Cluster 4's arithmetic: the root's record and its window, as pure functions
// the driver, the kernel and the tests share.

#include "constants.h"
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

// The largest width the window arithmetic carries: past twice the score range
// every window is open already.
inline constexpr int kMaxWindowDelta = 2 * INF_SCORE;

// A root line's window width: the base, the thread's offset in this engine's
// units, and the mean square of the line's scores over its divisor, so a stable
// level score searches narrowly and a won one is open from its first search.
[[nodiscard]] inline int window_delta(int base, int thread_id, int64_t mean_sq, int msq_div) {
    const int64_t magnitude = (mean_sq < 0 ? -mean_sq : mean_sq) / msq_div;
    return static_cast<int>(std::min<int64_t>(base + (thread_id % 8) * 282 / 1000 + magnitude,
                                              kMaxWindowDelta));
}

[[nodiscard]] inline int grow_delta(int delta, int growth) {
    return std::min(delta + growth * delta / 128, kMaxWindowDelta);
}

struct RootWindow {
    int alpha;
    int beta;
};

[[nodiscard]] inline RootWindow first_window(int average, int delta) {
    return {std::max(average - delta, -INF_SCORE), std::min(average + delta, INF_SCORE)};
}

// A fail low: the bound just proved is the new top, and alpha moves below the score.
[[nodiscard]] inline RootWindow after_fail_low(RootWindow window, int score, int delta) {
    return {std::max(score - delta, -INF_SCORE), window.alpha};
}

// A fail high: alpha follows the old top, kept within the width, and beta moves
// above the score.
[[nodiscard]] inline RootWindow after_fail_high(RootWindow window, int score, int delta) {
    return {std::max(window.beta - delta, window.alpha), std::min(score + delta, INF_SCORE)};
}

// LMR's share of the node's window against the root's. A null window is one unit
// of this engine's scale, and the donor's 577 reads its own unit, 0.282 of ours:
// at a root window as narrow as the donor's the fitted reductions stay where they
// were only with the null window in the donor's units.
[[nodiscard]] inline int lmr_window_term(int delta, int root_delta) {
    return (delta > 1 ? delta * 577 : 163) / root_delta;
}

// The record's order: by score, then by the last iteration's score. Every move
// but the line's best holds the "none" score, so a stable sort keeps the rest in
// the order they had.
[[nodiscard]] inline bool root_move_before(const RootMove& a, const RootMove& b) {
    return a.score != b.score ? a.score > b.score : a.previous_score > b.previous_score;
}

}  // namespace b5
