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

// The diagnostic counters, one X line each. The table generates the fields,
// reset() and the pool aggregate add(), so a counter cannot be added without
// being aggregated. Counted in diagnostic and tune builds and printed only
// when SearchLimits.diag is set; production compiles the increments away (the
// RELEASE_DIAG_COUNTERS arm exists to reproduce the cost measurement). Names
// and units are the cross-engine contract of tools/diag/run_suite.py and the
// oracle's matching build: a renamed counter breaks the differential harness.
// New counters go at the end of the table, each with the denominator it is
// read against, and are printed on new `kv` lines, never spliced into old ones.
#define BASILISK_DIAG_COUNTERS(X)                                            \
    /* Node population. Shares are of interior (negamax) nodes. */           \
    X(interior_nodes) X(in_check_nodes) X(check_exts) X(tt_pv_nodes)         \
    X(tt_probes) X(tt_hits) X(tt_cutoffs)                                    \
    /* Node and move-loop pruning, each with its tries where it has them. */ \
    X(rfp_cuts) X(razor_cuts) X(null_tries) X(null_cuts)                     \
    X(probcut_tries) X(probcut_cuts)                                         \
    X(fut_prunes) X(lmp_prunes) X(hist_prunes) X(see_prunes)                 \
    X(lmr_applied) X(lmr_researched)                                         \
    X(qs_nodes) X(qs_evasion_nodes)                                          \
    X(hist_cutoff_updates) X(hist_reward_updates)                            \
    /* Board work, snapshotted from the Board at search teardown. */         \
    X(see_ge_calls) X(gives_check_calls)                                     \
    /* TT stores that landed on a slot already holding this position's key  \
       against stores that claimed a different slot: how much of a pool's   \
       traffic rewrites entries it already owns rather than competing. */    \
    X(tt_stores) X(tt_stores_same_key)                                       \
    /* Ordering quality at the cutoff. A fail-high on the first move costs  \
       one move's search, on the n-th it costs n, so the mean cutoff index  \
       multiplies the tree's width; the source says which picker stage to   \
       look at. Denominator: fail_highs. */                                  \
    X(fail_highs) X(fail_high_first) X(fail_high_index_sum)                  \
    X(cutoff_src_tt) X(cutoff_src_good_tactical)                             \
    X(cutoff_src_quiet) X(cutoff_src_bad_tactical)                           \
    /* LMR width. lmr_applied alone cannot separate "rarely eligible" from  \
       "eligible but almost never reduced", which have opposite repairs. The \
       blocked reasons are exclusive and follow the gate's own short-circuit \
       order, so eligible = applied + clamped_zero + sum(blocked_*).         \
       reduction_plies / applied is the mean reduction taken; clamped_high  \
       counts a formula output above the new_depth - 1 ceiling, where the   \
       policy could not have mattered. */                                    \
    X(lmr_eligible) X(lmr_reduction_plies) X(lmr_clamped_zero)               \
    X(lmr_clamped_high)                                                      \
    /* Singular extensions: fired, the double (+2) path, fired at a node    \
       that also took the check extension, both (the 3-ply stack), and the  \
       negative-extension branch (TT score at or above beta). */             \
    X(sing_fired) X(sing_double) X(sing_in_check) X(sing_triple) X(sing_ttbeta) \
    /* Aspiration at the root: iterations with a window, the two failure    \
       directions, re-searches in total, and the full-width bail. */         \
    X(asp_windows) X(asp_fail_low) X(asp_fail_high) X(asp_researches) X(asp_giveup) \
    /* History-pruning reachability: quiets tested, and how many fell below \
       a half, quarter or eighth of the live threshold, which sizes a looser \
       candidate before one is built. */                                     \
    X(hist_prune_tested) X(hist_below_half)                                  \
    X(hist_below_quarter) X(hist_below_eighth)                               \
    X(lmr_blocked_depth) X(lmr_blocked_searched)                             \
    X(lmr_blocked_in_check) X(lmr_blocked_movetype)                          \
    X(lmr_blocked_gives_check)                                               \
    /* The selectivity core's mechanisms; the legacy kernel leaves them 0.  \
       Node pruning by remaining depth: each bucket set sums to rfp_cuts or  \
       razor_cuts. */                                                        \
    X(rfp_cuts_d1_3) X(rfp_cuts_d4_7) X(rfp_cuts_d8p)                        \
    X(razor_cuts_d1_3) X(razor_cuts_d4_7) X(razor_cuts_d8p)                  \
    /* LMR: the reduced depth clamped up to one ply, a negative reduction    \
       taken as an extension, and the full-depth re-search after a reduced   \
       fail high moved one ply deeper or shallower. Denominator: lmr_eligible \
       for the first two, lmr_researched for the last two. */                \
    X(lmr_floor_hits) X(lmr_extended)                                        \
    X(lmr_research_deeper) X(lmr_research_shallower)                         \
    /* Hindsight depth adjustment at node entry. Denominator: interior_nodes. */ \
    X(hindsight_up) X(hindsight_down)                                        \
    /* Nodes whose quiet delivery the move-count rule ended. */              \
    X(skip_quiets_nodes)                                                     \
    /* Continuation-correction updates at plies 2 and 4. */                  \
    X(corr_cont2_updates) X(corr_cont4_updates)                              \
    /* TT cutoffs: the quiet TT-move bonus, and cutoffs the graph-history    \
       check refused. Denominator: tt_hits. */                               \
    X(tt_cutoff_quiet_bonus) X(tt_cutoff_graph_refused)                      \
    /* The move-loop family, split: moves tested, and those pruned by        \
       continuation history, capture futility and SEE on either branch.      \
       Quiet futility counts in fut_prunes. */                               \
    X(cont_hist_pruned) X(capture_futility_pruned)                           \
    X(quiet_see_pruned) X(capture_see_pruned) X(moveloop_tested)

struct DiagCounters {
#define BASILISK_DIAG_COUNTER_FIELD(name) int64_t name = 0;
    BASILISK_DIAG_COUNTERS(BASILISK_DIAG_COUNTER_FIELD)
#undef BASILISK_DIAG_COUNTER_FIELD

    void reset() noexcept { *this = DiagCounters{}; }
    // Pool aggregation: sum a helper's counters into this one.
    void add(const DiagCounters& o) noexcept {
#define BASILISK_DIAG_COUNTER_ADD(name) name += o.name;
        BASILISK_DIAG_COUNTERS(BASILISK_DIAG_COUNTER_ADD)
#undef BASILISK_DIAG_COUNTER_ADD
    }

#define BASILISK_DIAG_COUNTER_ONE(name) + 1
    static constexpr int COUNT = 0 BASILISK_DIAG_COUNTERS(BASILISK_DIAG_COUNTER_ONE);
#undef BASILISK_DIAG_COUNTER_ONE
};
// Every counter is one int64_t and nothing else lives in the struct, so the
// aggregate and the field count stay in step with the table.
static_assert(sizeof(DiagCounters) == DiagCounters::COUNT * sizeof(int64_t),
              "DiagCounters holds something that is not a counter from the table");
static_assert(DiagCounters::COUNT == 79, "the counter contract has 79 entries; extend, never rename");

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
    QsFutilityPrune, QsSeePrune, QsLatePrune, QsBetaCutoff,
    SkipQuiets
};
// The pruning family a record belongs to, so the move-loop family's members
// can be told apart where one event name covers several rules.
enum class TraceFamily : uint8_t {
    None, Razor, Rfp, SkipQuiets, ContHist, QuietFutility, QuietSee,
    CaptureFutility, CaptureSee
};
// Whether this kernel produces cutoff_count; the trace header states it.
inline constexpr bool kTraceCutoffCountAvailable = false;
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
    int cutoff_count = -1; // -1 where the kernel has no producer
    int margin = VALUE_NONE;
    int result = VALUE_NONE;
    int lmr_depth = VALUE_NONE;  // the reduced depth move-loop pruning read
    int skip_quiets = -1;        // 0/1 once the move-count rule was evaluated
    TraceFamily family = TraceFamily::None;
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
                int move_count, int reduction, int margin, int result,
                int cutoff_count = -1, int lmr_depth = VALUE_NONE,
                int skip_quiets = -1, TraceFamily family = TraceFamily::None);
    // Prints the records; `root_moves` holds the one searched root move.
    void print(const InfoCallback& info, const std::vector<Move>& root_moves) const;

private:
    std::unique_ptr<std::array<TraceRecord, CAPACITY>> records_;
    size_t count_ = 0;
    bool enabled_ = false;
    bool overflow_ = false;
};
#endif
