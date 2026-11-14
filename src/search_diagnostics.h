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


using InfoCallback = std::function<void(const std::string&)>;

// The diagnostic counters, one X line each. The table generates the fields,
// reset() and the pool aggregate add(), so a counter cannot be added without
// being aggregated. Counted in diagnostic and tune builds and printed only
// when SearchLimits.diag is set; production compiles the increments away. Names
// and units are the cross-engine contract of tools/diag/run_suite.py and the
// oracle's matching build: a renamed counter breaks the differential harness.
// New counters go at the end of the table, each with the denominator it is
// read against, and are printed on new `kv` lines, never spliced into old ones.
// The oracle still prints the fourteen contract counters only the deleted
// original kernel and the check extension wrote; run_suite.py reads them when
// present.
#define BASILISK_DIAG_COUNTERS(X)                                            \
    /* Node population. Shares are of interior (negamax) nodes. */           \
    X(interior_nodes) X(in_check_nodes) X(tt_pv_nodes)                      \
    X(tt_probes) X(tt_hits) X(tt_cutoffs)                                    \
    /* Node and move-loop pruning, each with its tries where it has them. */ \
    X(rfp_cuts) X(razor_cuts) X(null_tries) X(null_cuts)                     \
    X(probcut_tries) X(probcut_cuts)                                         \
    X(fut_prunes)                                                            \
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
       one blocked reason is the depth, so                                  \
       eligible = applied + clamped_zero + blocked_depth.                    \
       reduction_plies / applied is the mean reduction taken; clamped_high  \
       counts a formula output above the new_depth - 1 ceiling, where the   \
       policy could not have mattered. */                                    \
    X(lmr_eligible) X(lmr_reduction_plies) X(lmr_clamped_zero)               \
    X(lmr_clamped_high)                                                      \
    /* Singular extensions: fired, the double (+2) path, and the            \
       negative-extension branch (TT score at or above beta). */             \
    X(sing_fired) X(sing_double) X(sing_ttbeta)                              \
    /* Aspiration at the root: iterations with a window, the two failure    \
       directions, re-searches in total, and the full-width bail. */         \
    X(asp_windows) X(asp_fail_low) X(asp_fail_high) X(asp_researches) X(asp_giveup) \
    X(lmr_blocked_depth)                                                     \
    /* The selectivity core's mechanisms.                                   \
       Node pruning by remaining depth: each bucket set sums to rfp_cuts or  \
       razor_cuts. */                                                        \
    X(rfp_cuts_d1_3) X(rfp_cuts_d4_7) X(rfp_cuts_d8p)                        \
    X(razor_cuts_d1_3) X(razor_cuts_d4_7) X(razor_cuts_d8p)                  \
    /* LMR: the reduced depth clamped up to one ply, a negative reduction    \
       taken as an extension, and the full-depth re-search after a reduced   \
       fail high moved one ply deeper. Denominator: lmr_eligible for the     \
       first two, lmr_researched for the last. */                            \
    X(lmr_floor_hits) X(lmr_extended)                                        \
    X(lmr_research_deeper)                                                   \
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
    X(quiet_see_pruned) X(capture_see_pruned) X(moveloop_tested)             \
    /* Exclusion searches whose only legal move was the excluded one. */     \
    X(singular_lone_move)                                                    \
    /* The equal-score tie rule's promotions of a move to best, by the tied \
       score: equal to alpha (a fail-low echoing the window's edge) or      \
       below it (a fail-low's own value). Either is a bound. */             \
    X(tie_promotions_at_alpha) X(tie_promotions_below_alpha)                 \
    /* In-search tablebase stores whose PV flag differs from the node's     \
       own (a PV node, or a TT entry marked PV). Zero once the probe runs   \
       after the node sets its flag. Denominator: tbhits. */                 \
    X(tb_store_stale_pv)                                                     \
    /* IIR: reductions, and those at a node hindsight had already          \
       reduced. */                                                          \
    X(iir_reductions) X(iir_after_hindsight)                              \
    /* Singular exclusion searches attempted, and those that ended in a    \
       multi-cut. Denominator: interior_nodes for the tries; sing_tries   \
       for the outcomes sing_fired, sing_multicut and sing_ttbeta. */     \
    X(sing_tries) X(sing_multicut)                                         \
    /* Null-move tries and cuts by remaining depth, in rfp_cuts' buckets  \
       (the RFP/NMP crossover band): each set sums to null_tries or       \
       null_cuts. */                                                      \
    X(null_tries_d1_3) X(null_tries_d4_7) X(null_tries_d8p)                \
    X(null_cuts_d1_3) X(null_cuts_d4_7) X(null_cuts_d8p)                   \
    /* Cluster 2.                                                          \
       Null-move verification: searches run, those that failed to reach   \
       beta (denominator null_verifications), and nodes the region kept   \
       from trying a null move (denominator interior_nodes). */           \
    X(null_verifications) X(null_verify_failed) X(null_region_refusals)    \
    /* ProbCut: tries at PV nodes (denominator probcut_tries). */          \
    X(probcut_pv_tries)                                                    \
    /* Singular: three-ply extensions (denominator sing_fired), negative  \
       extensions taken at a cut node whose TT score is below beta        \
       (denominator sing_tries), and entries the shuffling and seek-mate  \
       guards refused (denominator interior_nodes). */                    \
    X(sing_ext3) X(sing_negative_cut)                                      \
    X(sing_shuffle_refused) X(sing_seekmate_refused)                       \
    /* The per-line extension budget: extensions it truncated             \
       (denominator sing_fired). */                                       \
    X(ext_budget_bound)                                                    \
    /* Nodes on the previous iteration's PV (denominator interior_nodes)  \
       and IIR firings at PV nodes (denominator iir_reductions). */       \
    X(follow_pv_nodes) X(iir_pv_nodes)                                     \
    /* Upcoming repetitions that raised alpha to the draw (denominator    \
       interior_nodes + qs_nodes). */                                      \
    X(cuckoo_draws)                                                        \
    /* Interior nodes without a TT move (denominator interior_nodes), and \
       TT entries whose depth the bound-mismatch penalty lowered          \
       (denominator tt_hits). */                                          \
    X(tt_move_absent) X(tt_penalized)                                      \
    /* In-check shares of the reductions and move-loop rules: each is the \
       in-check subset of lmr_applied, skip_quiets_nodes,                 \
       capture_futility_pruned, capture_see_pruned, cont_hist_pruned and  \
       quiet_see_pruned. */                                               \
    X(in_check_lmr_applied) X(in_check_skip_quiets) X(in_check_capt_fut)   \
    X(in_check_capt_see) X(in_check_cont_hist) X(in_check_quiet_see)      \
    /* Moves the quiet branch of move-loop pruning removed at PV nodes off \
       the previous PV (denominator moveloop_tested). */                  \
    X(pv_quiet_pruned)                                                     \
    /* Diagnostic builds: reads of the colourless capture (cap),          \
       continuation (cont) and pawn histories and the continuation        \
       correction (corr), by the colour that trained the entry read. Per  \
       table: reads of a trained entry, those trained by the other colour \
       alone, those trained by both, and the sum of the other colour's    \
       share of the entry's updates in parts per million (denominator     \
       its reads). */                                                     \
    X(xc_cap_reads) X(xc_cap_other_only) X(xc_cap_mixed) X(xc_cap_other_ppm) \
    X(xc_cont_reads) X(xc_cont_other_only) X(xc_cont_mixed)               \
    X(xc_cont_other_ppm)                                                   \
    X(xc_pawn_reads) X(xc_pawn_other_only) X(xc_pawn_mixed)               \
    X(xc_pawn_other_ppm)                                                   \
    X(xc_corr_reads) X(xc_corr_other_only) X(xc_corr_mixed)               \
    X(xc_corr_other_ppm)                                                   \
    /* Quiescence's population (denominator qs_nodes): PV nodes, table     \
       cutoffs, stand-pat fail highs and the sum of their margins over    \
       beta, and stand-pats a table bound replaced and the sum of the     \
       distances it moved them. */                                        \
    X(qs_pv_nodes) X(qs_tt_cutoffs) X(qs_standpat_cuts) X(qs_standpat_excess) \
    X(qs_standpat_refined) X(qs_standpat_refine_mass)                      \
    /* Its move loop outside check: captures generated at nodes that      \
       reached it, those each rule pruned and those searched (the three   \
       sum to qs_moves at a node that fails low); fail highs, those on    \
       the first searched move, the sum of the searched index at the      \
       cutoff and of the margins over beta (decisive values excluded);    \
       nodes that ended without a cutoff, and those of them that searched \
       a move. */                                                         \
    X(qs_moves) X(qs_futility_pruned) X(qs_see_pruned)                     \
    X(qs_searched)                                                         \
    X(qs_fail_highs) X(qs_fail_high_first) X(qs_fail_high_index_sum)       \
    X(qs_fail_high_excess)                                                 \
    X(qs_fail_lows) X(qs_fail_low_searched)                                \
    /* Its evasion nodes (denominator qs_evasion_nodes): evasions         \
       searched; fail highs, those on the first evasion and the sum of    \
       the searched index at the cutoff; nodes whose table move is a      \
       legal evasion, and cutoffs that move made. */                      \
    X(qs_evasion_moves)                                                    \
    X(qs_evasion_fail_highs) X(qs_evasion_fail_high_first)                 \
    X(qs_evasion_fail_high_index_sum)                                      \
    X(qs_evasion_tt_move) X(qs_evasion_tt_cuts)                            \
    /* Interior nodes whose pruning estimate a table bound replaced, the  \
       sum of the distances, and the share of both from entries at depth  \
       0 or below, which quiescence wrote (denominator interior_nodes);   \
       interior nodes whose table move came from such an entry. */        \
    X(eval_refined) X(eval_refined_d0)                                     \
    X(eval_refine_mass) X(eval_refine_mass_d0)                             \
    X(tt_move_d0)                                                          \
    /* Reverse futility by the estimate's source: cuts taken on a table   \
       bound (denominator rfp_cuts), and nodes the margin's correction    \
       term alone kept from cutting, with those of them on a table bound. */ \
    X(rfp_cuts_tt_eval) X(rfp_corr_blocked) X(rfp_corr_blocked_tt_eval)   \
    /* Cluster 3.                                                           \
       Captures the count rule pruned (denominator qs_moves); recaptures    \
       the exemption alone kept from the gain rules (denominator            \
       qs_gain_tested plus itself);                                         \
       quiet evasions the rule left unsearched at nodes that ended without \
       a cutoff, and evasion captures it skipped on SEE (denominator        \
       qs_evasion_moves plus themselves); stalemates quiescence found       \
       (denominator qs_nodes); and captures that entered the gain rules'    \
       block: no check, promotion or exempt recapture, at a node where the  \
       rules run (denominator the captures the loop delivered: qs_searched  \
       plus the rules' prunes). */                                          \
    X(qs_count_pruned) X(qs_recapture_exempt)                              \
    X(qs_evasion_quiets_pruned) X(qs_evasion_see_pruned)                   \
    X(qs_stalemates) X(qs_gain_tested)                                     \
    /* Cluster 4.                                                           \
       Returns at the ply cap and those in check, in the search (denominator \
       interior_nodes) and in quiescence (denominator qs_nodes);            \
       searches a negative reduction took past the move's full depth, the   \
       PV ply aside (denominator the reduced searches, lmr_applied plus     \
       lmr_clamped_zero), and those on a line already holding one and two   \
       or more (denominator negext_total);                                  \
       root moves reduced and those searched again (denominator             \
       root_lmr_reduced for the second); the table's root move probed before \
       a window and how often it is not the last completed iteration's best, \
       at an iteration's first window, after a fail low and after a fail    \
       high (denominator the matching probe);                               \
       nodes in the windows of an iteration at depth > 3 (denominator all   \
       nodes), in those that failed, in give-up windows and in the windows  \
       that follow a fail high (denominator asp_nodes_windowed, which holds \
       every one of them but the give-up windows);                          \
       fail-high stops that ended a search (denominator asp_fail_high);     \
       completed iterations with a window whose best move's share of the   \
       nodes reached TmEffortHi and fell to TmEffortLo (denominator         \
       asp_windows). */                                                     \
    X(ply_cap_hits) X(ply_cap_in_check)                                    \
    X(qs_ply_cap_hits) X(qs_ply_cap_in_check)                              \
    X(negext_total) X(negext_chain1) X(negext_chain2p)                     \
    X(root_lmr_reduced) X(root_lmr_researched)                             \
    X(root_tt_probe_first) X(root_tt_mismatch_first)                       \
    X(root_tt_probe_fl) X(root_tt_mismatch_fl)                             \
    X(root_tt_probe_fh) X(root_tt_mismatch_fh)                             \
    X(asp_nodes_windowed) X(asp_nodes_failed) X(asp_nodes_giveup)          \
    X(asp_fail_high_research_nodes) X(fhstop_fired)                        \
    X(root_effort_hi) X(root_effort_lo)

// The stack's negative-reduction fields are written where a counter or the
// mechanism reads them.
inline constexpr bool kDiagCountersCompiled =
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    true;
#else
    false;
#endif

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
static_assert(DiagCounters::COUNT == 173, "the counter contract has 173 entries; extend, never rename");

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
    TtCutoff, RfpPrune, RazorPrune,
    NullCutoff, ProbcutCutoff, IirReduction,
    FutilityPrune, HistoryPrune, QuietSeePrune,
    CaptureFutilityPrune, CaptureSeePrune,
    SingularExtension, SingularMulticut, SingularNegative,
    LmrReduction, LmrResearch, BetaCutoff,
    QsTtCutoff, QsStandPatCutoff,
    QsFutilityPrune, QsSeePrune, QsBetaCutoff,
    SkipQuiets,
    // Cluster 2's events.
    NullVerify, NullRegionRefused, ExtBudgetTruncate,
    CuckooDraw, TtPenalty,
    // Cluster 3's events.
    QsCountPrune, QsEvasionPrune, QsStalemate,
    // Cluster 4's event: a root move searched again with the full window.
    RootPvResearch
};
// The pruning family a record belongs to, so the move-loop family's members
// can be told apart where one event name covers several rules.
enum class TraceFamily : uint8_t {
    None, Razor, Rfp, SkipQuiets, ContHist, QuietFutility, QuietSee,
    CaptureFutility, CaptureSee
};
// Whether this kernel produces cutoff_count; the trace header states it.
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

// One decision as the kernel reports it, named field by field (designated
// initialisers), so two of its many integers cannot be swapped unnoticed.
struct TraceDecision {
    TraceEvent event{};
    int ply = 0;
    int depth = 0;
    Move move = MOVE_NONE;
    int alpha = 0;
    int beta = 0;
    int estimated_score = 0;
    int improving = 0;
    int correction = 0;
    int history = 0;
    int move_count = 0;
    int reduction = 0;
    int margin = 0;
    int result = 0;
    int cutoff_count = -1;
    int lmr_depth = VALUE_NONE;
    int skip_quiets = -1;
    TraceFamily family = TraceFamily::None;
};

// Decision records at plies 1-2 of a single-root search, bounded so a trace
// cannot grow without limit.
class DecisionTrace {
public:
    static constexpr size_t CAPACITY = 32768;

    // Starts a search: clears the records and allocates them when enabled.
    // `root_only` records the root's decisions instead of plies 1 and 2.
    void start(bool enabled, bool root_only = false);
    [[nodiscard]] bool enabled() const { return enabled_; }
    void record(const TraceDecision& d);
    // Prints the records; `root_moves` holds the one searched root move.
    void print(const InfoCallback& info, const std::vector<Move>& root_moves) const;

private:
    std::unique_ptr<std::array<TraceRecord, CAPACITY>> records_;
    size_t count_ = 0;
    bool enabled_ = false;
    bool overflow_ = false;
    bool root_only_ = false;
};
#endif
