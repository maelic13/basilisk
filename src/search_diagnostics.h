#pragma once

// Search instruments: the per-search counters behind `Diag`, their pool
// aggregate, and the bounded decision trace. They observe decisions and are
// never inputs to one.

#include "eval.h"
#include "move.h"
#include "search_types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC) \
    || defined(BASILISK_RELEASE_DIAG_COUNTERS)
#define DIAG_COUNT(expression) (expression)
#else
#define DIAG_COUNT(expression) ((void)0)
#endif

using InfoCallback = std::function<void(const std::string&)>;

// ---- 8.6.6 diagnostic counters (Rarog 7.6 pattern) ----
// Counted in diagnostic/tune builds and printed only when SearchLimits.diag
// is set. Production compiles the hot-path increments away; the retained
// RELEASE_DIAG_COUNTERS arm exists only to reproduce the cost measurement.
// These size candidates BEFORE they spend SPRT slots and are the substrate
// Phase 10's acceptance criteria assume; check_exts must read 0 once
// 8.6.7 lands.
struct DiagCounters {
    int64_t interior_nodes = 0, in_check_nodes = 0, check_exts = 0, tt_pv_nodes = 0;
    int64_t tt_probes = 0, tt_hits = 0, tt_cutoffs = 0;
    int64_t rfp_cuts = 0, razor_cuts = 0, null_tries = 0, null_cuts = 0;
    int64_t probcut_tries = 0, probcut_cuts = 0;
    int64_t fut_prunes = 0, lmp_prunes = 0, hist_prunes = 0, see_prunes = 0;
    int64_t lmr_applied = 0, lmr_researched = 0;
    int64_t qs_nodes = 0, qs_evasion_nodes = 0;
    int64_t hist_cutoff_updates = 0, hist_reward_updates = 0;
    // 8.7.1(c): snapshotted from the Board at search teardown (see
    // board.h) — board_ptr_ is nulled before print_diag() runs.
    int64_t see_ge_calls = 0, gives_check_calls = 0;
    // 9.3(c): TT stores that landed on a slot already holding this
    // position's key, versus stores that claimed a different slot. At
    // Threads>1 the same-key share is how much the pool is re-writing
    // entries it (or another thread) already owns rather than competing
    // for capacity — the quantity 9.5's TT-coordination work moves.
    int64_t tt_stores = 0, tt_stores_same_key = 0;

    // ---- 5.2 differential harness (selected by BAS-O03) ----------------
    // BAS-O01/O03 measured our effective branching factor at ~2.20 against
    // the reference's ~1.61: at equal time we finish 15.6 plies where it
    // finishes 25.2, on MORE nodes per move. The tree is too wide, not too
    // small. These counters localize where that width is created; the
    // pre-existing ones above could not, because they report how often a
    // mechanism fired without reporting how often it could have.

    // Ordering quality. A fail-high on the first move costs one move's
    // search; on the n-th it costs n. Mean cutoff index is therefore a
    // direct multiplier on tree width, and `cutoff_src` says which stage
    // to fix. Counted at the cutoff, so the denominator is fail_highs.
    int64_t fail_highs = 0, fail_high_first = 0, fail_high_index_sum = 0;
    int64_t cutoff_src_tt = 0, cutoff_src_good_tactical = 0;
    int64_t cutoff_src_quiet = 0, cutoff_src_bad_tactical = 0;

    // LMR width. `lmr_applied` alone cannot distinguish "rarely eligible"
    // from "eligible but almost never reduced" — the two have opposite
    // repairs. The blocked reasons are mutually exclusive and evaluated in
    // the live gate's own short-circuit order, so
    //   eligible = applied + clamped_zero + sum(blocked_*).
    // `reduction_plies / applied` is the mean reduction actually taken,
    // which is the number a timid-LMR hypothesis lives or dies on.
    int64_t lmr_eligible = 0, lmr_reduction_plies = 0, lmr_clamped_zero = 0;
    // 5.4.3: the formula's raw output exceeded the new_depth-1 ceiling,
    // so the reduction actually taken was set by remaining depth rather
    // than by the policy. Distinguishes "our modulation is too small"
    // from "our modulation cannot matter here" — opposite repairs.
    int64_t lmr_clamped_high = 0;
    // 5.7.3 probe: how often does an extension actually stack?
    //   sing_fired      -- singular extension applied (+1 or +2)
    //   sing_double     -- of those, the +2 double-extension path
    //   sing_in_check   -- of those, at a node that ALSO took the check
    //                      extension, i.e. the 3-ply case the audit flagged
    int64_t sing_fired = 0;
    int64_t sing_double = 0;
    int64_t sing_in_check = 0;
    int64_t sing_triple = 0;   // double AND in check: the full 3-ply stack
    int64_t sing_ttbeta = 0;   // 5.7.4: the tt_score >= beta branch
    // 5.8.2: the aspiration path. Nothing counted these, so none of the
    // cluster's candidates could be sized before implementing them.
    int64_t asp_windows = 0;    // root iterations that used a window at all
    int64_t asp_fail_low = 0;
    int64_t asp_fail_high = 0;
    int64_t asp_researches = 0; // total re-searches across all iterations
    int64_t asp_giveup = 0;     // the delta >= 900 full-width bail
    // 5.6: history-pruning reachability. The live threshold is
    // hist_prune_coeff * depth against a SUM of six bounded history
    // channels whose maximum magnitude is 81,920 — so at depth 6 the
    // condition is provably unsatisfiable and at depth 5 it needs 85% of
    // theoretical maximum negative on every channel at once. These count
    // how many quiet moves would fall below a looser threshold, sizing a
    // candidate before one is built.
    int64_t hist_prune_tested = 0, hist_below_half = 0;
    int64_t hist_below_quarter = 0, hist_below_eighth = 0;
    int64_t lmr_blocked_depth = 0, lmr_blocked_searched = 0;
    int64_t lmr_blocked_in_check = 0, lmr_blocked_movetype = 0;
    int64_t lmr_blocked_gives_check = 0;

    void reset() noexcept { *this = DiagCounters{}; }
    // Pool aggregation (9.3c): sum a helper's counters into this one.
    // Written out rather than punned through an int64_t* — the
    // static_assert below is what catches a counter added without a
    // matching line here (it fires the moment the field count changes).
    void add(const DiagCounters& o) noexcept {
        interior_nodes += o.interior_nodes;
        in_check_nodes += o.in_check_nodes;
        check_exts += o.check_exts;
        tt_pv_nodes += o.tt_pv_nodes;
        tt_probes += o.tt_probes;
        tt_hits += o.tt_hits;
        tt_cutoffs += o.tt_cutoffs;
        rfp_cuts += o.rfp_cuts;
        razor_cuts += o.razor_cuts;
        null_tries += o.null_tries;
        null_cuts += o.null_cuts;
        probcut_tries += o.probcut_tries;
        probcut_cuts += o.probcut_cuts;
        fut_prunes += o.fut_prunes;
        lmp_prunes += o.lmp_prunes;
        hist_prunes += o.hist_prunes;
        see_prunes += o.see_prunes;
        lmr_applied += o.lmr_applied;
        lmr_researched += o.lmr_researched;
        qs_nodes += o.qs_nodes;
        qs_evasion_nodes += o.qs_evasion_nodes;
        hist_cutoff_updates += o.hist_cutoff_updates;
        hist_reward_updates += o.hist_reward_updates;
        see_ge_calls += o.see_ge_calls;
        gives_check_calls += o.gives_check_calls;
        tt_stores += o.tt_stores;
        tt_stores_same_key += o.tt_stores_same_key;
        fail_highs += o.fail_highs;
        fail_high_first += o.fail_high_first;
        fail_high_index_sum += o.fail_high_index_sum;
        cutoff_src_tt += o.cutoff_src_tt;
        cutoff_src_good_tactical += o.cutoff_src_good_tactical;
        cutoff_src_quiet += o.cutoff_src_quiet;
        cutoff_src_bad_tactical += o.cutoff_src_bad_tactical;
        lmr_eligible += o.lmr_eligible;
        lmr_reduction_plies += o.lmr_reduction_plies;
        lmr_clamped_zero += o.lmr_clamped_zero;
        lmr_clamped_high += o.lmr_clamped_high;
        sing_fired += o.sing_fired;
        sing_double += o.sing_double;
        sing_in_check += o.sing_in_check;
        sing_triple += o.sing_triple;
        sing_ttbeta += o.sing_ttbeta;
        asp_windows += o.asp_windows;
        asp_fail_low += o.asp_fail_low;
        asp_fail_high += o.asp_fail_high;
        asp_researches += o.asp_researches;
        asp_giveup += o.asp_giveup;
        hist_prune_tested += o.hist_prune_tested;
        hist_below_half += o.hist_below_half;
        hist_below_quarter += o.hist_below_quarter;
        hist_below_eighth += o.hist_below_eighth;
        lmr_blocked_depth += o.lmr_blocked_depth;
        lmr_blocked_searched += o.lmr_blocked_searched;
        lmr_blocked_in_check += o.lmr_blocked_in_check;
        lmr_blocked_movetype += o.lmr_blocked_movetype;
        lmr_blocked_gives_check += o.lmr_blocked_gives_check;
    }
};
// 57 counters, all int64_t. If this fails you added a counter: add it to
// add() above and update the count, or the pool aggregate silently drops it.
static_assert(sizeof(DiagCounters) == 57 * sizeof(int64_t),
              "DiagCounters changed shape — update DiagCounters::add()");

// One search's counters as `info string diag` lines, followed by the
// evaluator's speed telemetry.
void print_search_diag(const DiagCounters& d, const Evaluator& evaluator,
                       const InfoCallback& info);

// The pool section, printed by the main thread after the join: `main` is its
// own counters, `pool` the sum over every thread.
void print_pool_diag(const DiagCounters& main, const DiagCounters& pool, int thread_count,
                     const std::vector<int>& completed_depths, const InfoCallback& info);

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
enum class TraceEvent : uint8_t {
    CheckExtension, TtCutoff, RfpPrune, RazorPrune,
    NullCutoff, ProbcutCutoff, IirReduction,
    FutilityPrune, LmpPrune, HistoryPrune, QuietSeePrune,
    CaptureFutilityPrune, CaptureSeePrune,
    SingularExtension, SingularMulticut, SingularNegative,
    LmrReduction, LmrResearch, BetaCutoff,
    QsTtCutoff, QsStandPatCutoff, QsDeltaPrune,
    QsFutilityPrune, QsSeePrune, QsLatePrune, QsBetaCutoff
};
struct TraceRecord {
    TraceEvent event{};
    Move move = MOVE_NONE;
    int sequence = 0;
    int ply = 0;
    int depth = 0;
    int alpha = 0;
    int beta = 0;
    int estimated_score = VALUE_NONE;
    int improving = -1;
    int correction = VALUE_NONE;
    int history = VALUE_NONE;
    int move_count = 0;
    int reduction = 0;
    int cutoff_count = -1; // no producer exists in the current architecture
    int margin = VALUE_NONE;
    int result = VALUE_NONE;
};

// Decision records at plies 1-2 of a single-root search, bounded so a trace
// cannot grow without limit.
class DecisionTrace {
public:
    static constexpr size_t CAPACITY = 32768;

    // Starts a search: clears the records and allocates them when enabled.
    void start(bool enabled);
    [[nodiscard]] bool enabled() const { return enabled_; }
    void record(TraceEvent event, int ply, int depth, Move move,
                int alpha, int beta, int estimated_score,
                int improving, int correction, int history,
                int move_count, int reduction, int margin, int result);
    // Prints the records; `root_moves` holds the one searched root move.
    void print(const InfoCallback& info, const std::vector<Move>& root_moves) const;

private:
    std::unique_ptr<std::array<TraceRecord, CAPACITY>> records_;
    size_t count_ = 0;
    bool enabled_ = false;
    bool overflow_ = false;
};
#endif
