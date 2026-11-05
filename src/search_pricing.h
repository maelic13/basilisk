#pragma once

// What the search's pruning rules and reductions hide (tune and diagnostic
// builds). Each rule's decisions in the engine's own tree are counted per
// remaining depth, node type and move number, and every N-th one (the
// `Pricing` option, 0 = off) is searched again as the rule denied it: the
// node without its node-level shortcuts at its own depth and window, a pruned
// move at the node's full depth with the node's window, a reduced move that
// failed low at its full depth with the node's window. A probe that lands on
// the other side of the bound the rule claimed "beats" it; a pruned or
// reduced move whose probe also exceeds the node's final best score would
// have "changed" the node's best move.
//
// A probe is isolated, so the engine's own search cannot tell it ran: it
// reads the transposition table and the histories but writes neither (its
// stores go to a private table that only probes read), its nodes, counters
// and stack records are restored afterwards, and no decision inside it is
// counted or sampled. With pricing on, a search limited by depth or nodes
// therefore visits exactly the nodes it visits with pricing off; a clock
// would count the probes' time. The board's and evaluator's work counters
// and the cross-colour history reads do include the probes. One thread only.

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
#define BASILISK_PRICING 1

#include "search_diagnostics.h"
#include "tt.h"
#include <cstdint>
#include <memory>
#include <vector>

enum class PriceRule : uint8_t {
    // Node rules: the node returns without searching its moves.
    Razor, Rfp, NullMove, ProbCut, ProbCutTt, MultiCut, QsDelta,
    // Move rules: one move is not searched.
    CaptureFutility, CaptureSee, ContHist, QuietFutility, QuietSee, SkipQuiets,
    QsFutility, QsSee, QsLate,
    // A reduced search that failed low.
    LmrFailLow,
    Count
};

// The node a decision was taken at: a PV node, an expected cut or all node,
// or a null-window quiescence node.
enum class PriceNode : uint8_t { Pv, Cut, All, Qs, Count };

struct PriceCell {
    int64_t decisions = 0;    // the rule's decisions in the engine's own tree
    int64_t probed = 0;       // ... searched again
    int64_t beats = 0;        // ... whose probe crossed the bound the rule claimed
    int64_t changes = 0;      // ... and exceeded the node's final best score
    int64_t probe_nodes = 0;  // nodes the probes searched
};

class PruningPrice {
public:
    static constexpr int DEPTHS = 16;       // remaining depth 0 (quiescence) to 15+
    static constexpr int MOVE_BUCKETS = 6;  // none (a node rule), 1-3, 4-6, 7-10, 11-16, 17+
    static constexpr size_t PROBE_TT_MB = 16;

    // Starts a search: `every` 0 switches pricing off.
    void start(int every);
    [[nodiscard]] bool enabled() const { return every_ > 0; }
    [[nodiscard]] bool probing() const { return probing_ > 0; }

    // Counts one decision of `rule` in the engine's own tree and returns its
    // cell when this decision is to be probed, -1 otherwise. Inside a probe
    // nothing is counted.
    int decide(PriceRule rule, int depth, PriceNode node, int move_number) {
        if (every_ <= 0 || probing_ > 0)
            return -1;
        const int index = cell_index(rule, depth, node, move_number);
        ++cells_[static_cast<size_t>(index)].decisions;
        return ++seen_[static_cast<size_t>(rule)] % every_ == 0 ? index : -1;
    }
    void probed(int cell, bool beats, int64_t nodes) {
        PriceCell& c = cells_[static_cast<size_t>(cell)];
        ++c.probed;
        c.beats += beats;
        c.probe_nodes += nodes;
    }
    void changed(int cell) { ++cells_[static_cast<size_t>(cell)].changes; }

    // A probe's span. The next node entered takes `bare` and runs without
    // its node-level shortcuts.
    void enter() { ++probing_; }
    void leave() { --probing_; }
    void set_bare() { bare_ = true; }
    [[nodiscard]] bool take_bare() {
        const bool bare = bare_;
        bare_ = false;
        return bare;
    }

    // The table a probe reads (the deeper of its own entry and the engine's)
    // and the only one it writes.
    bool probe_tt(const TranspositionTable& engine, Key key, TTEntry& out) const {
        TTEntry own{};
        const bool own_found = tt_->probe_copy(key, own);
        const bool engine_found = engine.probe_copy(key, out);
        if (own_found && (!engine_found || own.depth >= out.depth)) {
            out = own;
            return true;
        }
        return engine_found;
    }
    void store_tt(Key key, int depth, int score, TTFlag flag, Move m, int ply,
                  int static_eval, bool pv) {
        tt_->store(key, depth, score, flag, m, ply, static_eval, pv);
    }

    // The cells as `info string price` lines.
    void print(const InfoCallback& info) const;

private:
    static int cell_index(PriceRule rule, int depth, PriceNode node, int move_number);

    int every_ = 0;
    int probing_ = 0;
    bool bare_ = false;
    std::vector<PriceCell> cells_;
    int64_t seen_[static_cast<int>(PriceRule::Count)] = {};
    std::unique_ptr<TranspositionTable> tt_;
};

// A pruned or reduced move whose probe beat alpha; whether it beats the node's
// final best score is known only when the node's move loop ends.
struct PricePending {
    int cell;
    int value;
};
#endif
