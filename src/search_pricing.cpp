#include "search_pricing.h"

#ifdef BASILISK_PRICING
#include <algorithm>
#include <array>
#include <format>
#include <string>

namespace {

constexpr int RULES = static_cast<int>(PriceRule::Count);
constexpr int NODES = static_cast<int>(PriceNode::Count);
constexpr int CELLS = RULES * PruningPrice::DEPTHS * NODES * PruningPrice::MOVE_BUCKETS;

constexpr std::array<const char*, RULES> RULE_NAMES = {
    "razor", "rfp", "null_move", "probcut", "probcut_tt", "multicut", "qs_delta",
    "capture_futility", "capture_see", "cont_hist", "quiet_futility", "quiet_see",
    "skip_quiets", "qs_futility", "qs_see", "qs_late", "lmr_fail_low"};
constexpr std::array<const char*, NODES> NODE_NAMES = {"pv", "cut", "all", "qs"};
constexpr std::array<const char*, PruningPrice::MOVE_BUCKETS> MOVE_NAMES = {
    "-", "1-3", "4-6", "7-10", "11-16", "17+"};

int move_bucket(int move_number) {
    if (move_number <= 0) return 0;
    if (move_number <= 3) return 1;
    if (move_number <= 6) return 2;
    if (move_number <= 10) return 3;
    if (move_number <= 16) return 4;
    return 5;
}

}  // namespace

void PruningPrice::start(int every) {
    every_ = std::max(every, 0);
    probing_ = 0;
    bare_ = false;
    std::fill(std::begin(seen_), std::end(seen_), int64_t{0});
    if (every_ == 0)
        return;
    cells_.assign(CELLS, PriceCell{});
    if (!tt_)
        tt_ = std::make_unique<TranspositionTable>(PROBE_TT_MB);
    else
        tt_->clear();
}

int PruningPrice::cell_index(PriceRule rule, int depth, PriceNode node, int move_number) {
    const int d = std::clamp(depth, 0, DEPTHS - 1);
    return ((static_cast<int>(rule) * DEPTHS + d) * NODES + static_cast<int>(node))
               * MOVE_BUCKETS
         + move_bucket(move_number);
}

void PruningPrice::print(const InfoCallback& info) const {
    if (!info || every_ == 0)
        return;
    PriceCell total;
    for (int rule = 0; rule < RULES; ++rule)
        for (int d = 0; d < DEPTHS; ++d)
            for (int node = 0; node < NODES; ++node)
                for (int mb = 0; mb < MOVE_BUCKETS; ++mb) {
                    const PriceCell& c = cells_[static_cast<size_t>(
                        ((rule * DEPTHS + d) * NODES + node) * MOVE_BUCKETS + mb)];
                    if (c.decisions == 0)
                        continue;
                    info(std::format("info string price kv rule={} depth={} node={} moves={} "
                                     "decisions={} probed={} beats={} changes={} probe_nodes={}",
                                     RULE_NAMES[static_cast<size_t>(rule)], d,
                                     NODE_NAMES[static_cast<size_t>(node)],
                                     MOVE_NAMES[static_cast<size_t>(mb)], c.decisions, c.probed,
                                     c.beats, c.changes, c.probe_nodes));
                    total.decisions += c.decisions;
                    total.probed += c.probed;
                    total.probe_nodes += c.probe_nodes;
                }
    info(std::format("info string price total every={} decisions={} probed={} probe_nodes={}",
                     every_, total.decisions, total.probed, total.probe_nodes));
}
#endif
