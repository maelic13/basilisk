#include "search_worker.h"
#include "constants.h"
#include "syzygy.h"
#include <algorithm>
#include <bit>
#include <format>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <ratio>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

static_assert(TranspositionTable::MATE_SCORE == MATE_SCORE
              && TranspositionTable::MAX_PLY == MAX_PLY,
              "the table's mate-score encoding uses the search's mate band");


// ---- Constructor -----------------------------------------------------------

Searcher::Searcher(TranspositionTable& tt,
                   std::atomic_bool& stop_flag,
                   InfoCallback info_cb,
                   std::atomic_bool* ponderhit_flag)
    : shared_{tt, stop_flag, ponderhit_flag, std::move(info_cb), {}}
    , evaluator_()
{
    // hist_ constructs cleared (history.h); clear() also resets the age counter.
    clear();
}

void Searcher::clear() {
    kernel_new_game();
}

// ---- Time management -------------------------------------------------------

void Searcher::compute_time_limit(const SearchLimits& limits, bool ponder, Color side,
                                  int game_ply) {
    state_.soft_limit = 0.0;
    state_.hard_limit = 0.0;

    if (limits.infinite || ponder) return;
    if (limits.movetime > 0) {
        // Fixed movetime: use the full movetime as the hard limit. GUIs and
        // adjudicators tolerate ~10% over nominal, and movetime games never
        // forfeit on time the way clock play does, so subtracting overhead
        // here only costs depth for no safety gain.
        state_.hard_limit = std::max(1, limits.movetime) / 1000.0;
        return;
    }

    // Logarithmic-time-left, increment- and ply-aware clock budget in
    // Stockfish's shape, shared with Rarog so both engines carry the same
    // proven formula and time-safety reserve.
    const double time     = std::max(0, (side == WHITE) ? limits.wtime : limits.btime);
    const double inc      = (side == WHITE) ? limits.winc : limits.binc;
    const double overhead = limits.overhead;

    if (time <= 0.0 && inc <= 0.0) return;

    const bool   explicit_mtg = limits.movestogo > 0;
    const double mtg = explicit_mtg ? std::min(limits.movestogo, 50) : 50.0;

    // SF: timeLeft = max(1, time + inc*(mtg-1) - overhead*(2+mtg))
    const double time_left = std::max(1.0,
        time + inc * (mtg - 1.0) - overhead * (2.0 + mtg));

    const double ply = static_cast<double>(std::max(0, game_ply));

    double opt_scale, max_scale;
    if (explicit_mtg) {
        // Explicit movestogo branch (SF timeman.cpp)
        opt_scale = std::min((0.88 + ply / 116.4) / mtg, 0.88 * time / time_left);
        max_scale = 1.3 + 0.11 * mtg;
    } else {
        // Sudden death / increment branch (SF timeman.cpp)
        const double log_t     = std::log10(std::max(time_left / 1000.0, 1e-9));
        const double opt_const = std::min(0.0029869 + 0.00033554 * log_t, 0.004905);
        const double max_const = std::max(3.3744 + 3.0608 * log_t, 3.1441);
        opt_scale = std::min(0.012112 + std::pow(std::max(ply + 3.22713, 0.0), 0.46866) * opt_const,
                             0.19404 * time / time_left);
        max_scale = std::min(6.873, max_const + ply / 12.352);
    }

    double optimum_ms = std::max(opt_scale * time_left, 1.0);
    // SF: maximum = max(optimum, min(0.8097*time - overhead, maxScale*optimum))
    double maximum_ms = std::max(
        std::min(0.8097 * time - overhead, max_scale * optimum_ms),
        optimum_ms);

    // Overall budget multipliers, tunable; the defaults are a no-op.
    optimum_ms *= limits.params.tm_opt_mult / 100.0;
    maximum_ms *= limits.params.tm_max_mult / 100.0;

    // Time-safety reserve. The maximum above leaves only about 19% of the
    // clock plus one move overhead unused, a few milliseconds when time is
    // short, while the wall time the GUI charges also includes the dispatch
    // latency before our clock starts and the latency of bestmove reaching
    // it. Two move overheads cover that at one thread, where the stop poll
    // (every 2048 nodes) lands within a millisecond. Under multi-thread
    // scheduler contention the poll stretches to tens of milliseconds and the
    // hard cap can be overrun by the difference: Rarog measured 10 forfeits in
    // 240 games at four threads, and a flat extra 30 ms took it to 0 in 103
    // (a finer timer was measured not to be the fix). The reserve binds only
    // in a low-time scramble; at one thread the budget is unchanged.
    const double smp_reserve  = (config_.thread_count > 1) ? 30.0 : 0.0;
    const double reserve      = 2.0 * overhead + smp_reserve;
    const double hard_ceiling = std::max(time - reserve, 1.0);
    maximum_ms = std::min(maximum_ms, hard_ceiling);
    optimum_ms = std::min(optimum_ms, maximum_ms);

    state_.soft_limit = optimum_ms / 1000.0;
    state_.hard_limit = maximum_ms / 1000.0;
}

double Searcher::elapsed_seconds() const {
    using namespace std::chrono;
    return duration<double>(steady_clock::now() - state_.start_time).count();
}

// The shared node count is published in batches instead of one atomic add
// per node per thread on a single cache line. The batch is a power of two so
// the test is a mask; the node limit's granularity across threads is the
// batch, which every engine that batches accepts. A single-thread search has
// no pool counter and never enters this path.
static constexpr int64_t sharedNodeBatch = 1024;
static_assert((sharedNodeBatch & (sharedNodeBatch - 1)) == 0,
              "sharedNodeBatch must be a power of two (the flush test is a mask)");


void Searcher::flush_shared_nodes() {
    if (!shared_.pool.nodes)
        return;
    const int64_t pending = state_.nodes - state_.shared_nodes_flushed;
    if (pending <= 0)
        return;
    state_.shared_nodes_flushed = state_.nodes;
    state_.shared_nodes_total =
        shared_.pool.nodes->fetch_add(pending, std::memory_order_relaxed) + pending;
}

int64_t Searcher::record_node() {
    ++state_.nodes;
    if (shared_.pool.nodes) {
        if ((state_.nodes & (sharedNodeBatch - 1)) == 0)
            flush_shared_nodes();
        // The node LIMIT is still checked every node, against the last
        // published pool total plus this thread's unpublished nodes. That
        // estimate is a lower bound on the true total (other threads hold
        // unpublished nodes of their own) and is exact immediately after a
        // flush — so `go nodes N` keeps roughly its old accuracy, which
        // checking only at batch boundaries would NOT: a limit smaller than
        // one batch would have been missed entirely and overshot ~10x.
        // The comparison is local arithmetic; the atomic is what got batched.
        const int64_t estimate = state_.shared_nodes_total + (state_.nodes - state_.shared_nodes_flushed);
        if (state_.nodes_limit > 0 && estimate >= state_.nodes_limit)
            state_.stopped = true;
        return estimate;
    }
    if (state_.nodes_limit > 0 && state_.nodes >= state_.nodes_limit)
        state_.stopped = true;
    return state_.nodes;
}

void Searcher::record_tbhit(int64_t count) {
    if (count <= 0)
        return;
    state_.tb_hits += count;
    if (shared_.pool.tbhits)
        shared_.pool.tbhits->fetch_add(count, std::memory_order_relaxed);
}

int64_t Searcher::current_nodes() const {
    return shared_.pool.nodes
        ? shared_.pool.nodes->load(std::memory_order_relaxed)
        : state_.nodes;
}

int64_t Searcher::current_tbhits() const {
    return shared_.pool.tbhits
        ? shared_.pool.tbhits->load(std::memory_order_relaxed)
        : state_.tb_hits;
}

bool Searcher::check_stop() {
    if (state_.stopped) return true;

    if (shared_.stop.load(std::memory_order_acquire)) {
        state_.stopped = true;
        return true;
    }

    if (state_.pondering && shared_.ponderhit && shared_.ponderhit->load(std::memory_order_acquire)) {
        state_.pondering = false;
        const int game_ply = state_.board
            ? 2 * (state_.board->fullmove() - 1) + (state_.board->turn() == BLACK ? 1 : 0)
            : 0;
        compute_time_limit(config_.limits, false, state_.root_side, game_ply);
        if (state_.soft_limit > 0.0 && elapsed_seconds() >= state_.soft_limit) {
            state_.stopped = true;
            return true;
        }
    }

    if (state_.hard_limit > 0.0 && elapsed_seconds() >= state_.hard_limit) {
        state_.stopped = true;
        return true;
    }
    if (state_.nodes_limit > 0 && current_nodes() >= state_.nodes_limit) {
        state_.stopped = true;
        return true;
    }
    return false;
}


// ---- UCI info ---------------------------------------------------------------

// The pool section, printed by thread 0 after the join (its own search and
// per-thread lines finish before the helpers do), so it appends rather than
// replacing anything. Emitted only at Threads > 1.
void Searcher::print_pool_diag(const std::vector<std::unique_ptr<Searcher>>& pool,
                               int thread_count,
                               const std::vector<int>& completed_depths) const {
    if (!shared_.info || !config_.limits.diag || thread_count <= 1) return;

    DiagCounters total;
    const int counted = std::min<int>(thread_count, static_cast<int>(pool.size()));
    for (int i = 0; i < counted; ++i)
        total.add(pool[static_cast<size_t>(i)]->state_.diag);
    ::print_pool_diag(state_.diag, total, thread_count, completed_depths, shared_.info);
}

void Searcher::send_info(int depth, int multipv, int score, const std::vector<Move>& line,
                         int64_t total_nodes, double elapsed) const {
    std::vector<Move> pv_moves = legal_line(*state_.board, line);

    // Stockfish-style tablebase PV extension. With no clock or movetime it
    // runs on every line, unbounded, as in Stockfish's analysis mode. Under
    // time control only the final line is extended (Engine), once per move and
    // within half the Move Overhead, because Basilisk prints a line at every
    // depth. A line that ends in a fifty-move draw shows the draw.
    if (!pv_moves.empty() && is_tablebase_decisive(score) && Syzygy::enabled()
        && config_.limits.movetime <= 0 && config_.limits.wtime <= 0
        && config_.limits.btime <= 0) {
        Syzygy::PvExtension ext = Syzygy::extend_pv(
            *state_.board, pv_moves, config_.limits.syzygy_50_move_rule,
            config_.limits.syzygy_probe_limit, [] { return false; });
        pv_moves = std::move(ext.pv);
        if (ext.ends_in_draw)
            score = 0;
    }

    if (shared_.info)
        shared_.info(format_info_line(depth, state_.sel_depth, multipv, score, {}, total_nodes, elapsed,
                                  current_tbhits(), shared_.tt.hashfull(), pv_moves));
}

void Searcher::init_root_tablebase_scores(const Board& board) {
    (void) board;
    state_.root_tb_moves = config_.limits.syzygy_root_moves;
    state_.tb_probe_in_search = state_.root_tb_moves.empty()
        || (!state_.root_tb_moves.front().used_dtz && state_.root_tb_moves.front().score > 0);
    if (!state_.root_tb_moves.empty() && config_.thread_id == 0)
        record_tbhit(static_cast<int64_t>(state_.root_tb_moves.size()));
}

int Searcher::root_tablebase_score(Move move) const {
    for (const auto& entry : state_.root_tb_moves) {
        if (entry.bestmove == move)
            return entry.score;
    }
    return VALUE_NONE;
}

int Searcher::root_tablebase_display(Move move) const {
    for (const auto& entry : state_.root_tb_moves) {
        if (entry.bestmove == move)
            return entry.display;
    }
    return VALUE_NONE;
}

bool Searcher::root_tablebase_allows(Move move) const {
    if (state_.root_tb_moves.empty())
        return true;
    return root_tablebase_score(move) != VALUE_NONE;
}

Move Searcher::ponder_from_tt(const Board& root, Move bestmove) const {
    if (!is_legal_move_on_board(root, bestmove))
        return MOVE_NONE;

    Board child = root;
    child.make_move(bestmove);

    TTEntry entry{};
    if (!shared_.tt.probe_copy(child.position_key(), entry))
        return MOVE_NONE;

    const Move ponder = move_from_tt(entry.move16);
    return is_legal_move_on_board(child, ponder) ? ponder : MOVE_NONE;
}


// ---- Iterative deepening ---------------------------------------------------

SearchResult Searcher::search(Board board, const SearchLimits& limits) {
    SearchConfig config;
    config.limits = limits;
    return search(std::move(board), std::move(config), PoolResources{});
}

SearchResult Searcher::search(Board board, SearchConfig config, const PoolResources& pool) {
    config.root_filter_count = std::max(1, config.root_filter_count);
    if (config.root_filter_index < 0 || config.root_filter_index >= config.root_filter_count)
        config.root_filter_index = -1;
    config.thread_id = std::max(0, config.thread_id);
    config_ = std::move(config);
    shared_.pool = pool;
    const SearchLimits& limits = config_.limits;

    state_.board        = &board;
    state_.nodes        = 0;
    state_.tb_hits      = 0;
    state_.nodes_limit  = limits.nodes;
    state_.shared_nodes_flushed = 0;
    state_.shared_nodes_total   = 0;
    state_.sel_depth    = 0;
    state_.stopped      = false;
    state_.pondering    = limits.ponder;
    state_.root_side    = board.turn();
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    state_.trace.start(limits.decision_trace && limits.diag && shared_.info
                       && config_.thread_count == 1 && config_.thread_id == 0
                       && limits.root_moves.size() == 1);
    if (limits.decision_trace && shared_.info && !state_.trace.enabled())
        shared_.info("info string trace error requires Diag=true Threads=1 and exactly one searchmoves root");
#endif

    // The clock starts at the `go`-receipt instant the protocol captured, not
    // at the worker's entry, so elapsed_seconds() counts the dispatch and
    // thread-handoff latency the GUI already charges (measured up to about
    // 20 ms at bullet under load). Internal callers leave go_recv_time unset
    // and the clock starts now, so bench is unaffected.
    state_.start_time = (limits.go_recv_time.time_since_epoch().count() != 0)
                ? limits.go_recv_time
                : std::chrono::steady_clock::now();
    const int game_ply = 2 * (board.fullmove() - 1) + (board.turn() == BLACK ? 1 : 0);
    compute_time_limit(limits, limits.ponder, board.turn(), game_ply);

    if (config_.update_tt_age)
        shared_.tt.new_search();
    kernel_begin_search();

    std::memset(state_.pv_len, 0, sizeof(state_.pv_len));
    init_root_tablebase_scores(board);
    state_.root_ordering = RootOrdering{&state_.root_tb_moves, shared_.pool.root_table};

    SearchResult result;
    int prev_score      = 0;
    Move prev_best      = MOVE_NONE;
    int  best_stability = 0;   // how many consecutive depths best move hasn't changed
    double best_move_changes = 0.0;  // decaying count of root best-move flips

    int max_depth = limits.infinite ? MAX_SEARCH_DEPTH
                  : std::min(limits.depth, MAX_SEARCH_DEPTH);

    // A root with no legal move is decided: report it once, as Stockfish does,
    // and search nothing. `bestmove` (0000) still waits for `stop` under
    // `infinite` or `ponder`; that is the caller's.
    {
        MoveList root_legal;
        board.gen_legal(root_legal);
        if (root_legal.size() == 0) {
            const bool mated = board.is_in_check();
            result.score = mated ? -MATE_SCORE : 0;
            if (shared_.info)
                shared_.info(mated ? "info depth 0 score mate 0" : "info depth 0 score cp 0");
            max_depth = 0;
        }
    }

    // MultiPV: the main thread reports up to `multipv` lines, never more than
    // the root moves the search may play; helpers search one.
    state_.multipv_lines = 1;
    state_.root_excluded.clear();
    if (limits.multipv > 1 && config_.thread_id == 0) {
        MoveList root_legal;
        board.gen_legal(root_legal);
        int allowed = 0;
        for (Move m : root_legal)
            if (move_in_root_moves(m, limits.root_moves) && root_tablebase_allows(m))
                ++allowed;
        state_.multipv_lines = std::clamp(allowed, 1, limits.multipv);
    }

    int start_depth = 1;
    state_.diag.reset();         // fresh diagnostic counters per `go`
    evaluator_.diag_lazy = config_.limits.diag;
    evaluator_.lazy_fires = evaluator_.lazy_sign_flips = 0;
    evaluator_.lazy_margin_crossings = 0;
    evaluator_.lazy_absdelta_sum = evaluator_.lazy_absdelta_max = 0;
    // Speed telemetry, fresh per `go`. The Board counters are reset through
    // state_.board because the Board arrived by value from a caller whose own
    // counters may be stale.
    evaluator_.eval_calls = 0;
    evaluator_.pawn_probes = evaluator_.pawn_hits = 0;
#ifdef BASILISK_TUNE
    evaluator_.diag_endgames = config_.limits.diag;
    evaluator_.endgame_occurrence.reset();
#endif
    if (state_.board)
        state_.board->reset_diag_counters();

    if (config_.thread_id > 0 && max_depth > 2)
        start_depth = 1 + (config_.thread_id % 2);

    for (int depth = start_depth; depth <= max_depth && !state_.stopped; depth++) {
        state_.pv_len[0] = 0;
        state_.sel_depth = 0;
        state_.root_depth_nodes = 0;
        state_.root_best_nodes = 0;
        state_.root_best_effort = 0;
        int score;

        if (depth <= 3 || std::abs(prev_score) >= MATE_SCORE - MAX_PLY) {
            score = root_search(depth, -INF_SCORE, INF_SCORE);
        } else {
            int delta = config_.limits.params.aspiration_delta;
            int asp_a = prev_score - delta;
            int asp_b = prev_score + delta;
            // A long iteration whose window fails reports the bound it proved,
            // with the line that failed high, or the previous line on a fail low.
            auto bound_line = [&](int bound_score, bool lower) {
                if (!shared_.info || elapsed_seconds() <= kBoundLineAfterSeconds)
                    return;
                std::vector<Move> line = result.pv;
                if (lower && state_.pv_len[0] > 0)
                    line.assign(state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY));
                shared_.info(format_info_line(depth, state_.sel_depth, 1, bound_score,
                                          lower ? "lowerbound" : "upperbound", current_nodes(),
                                          elapsed_seconds(), current_tbhits(), shared_.tt.hashfull(),
                                          legal_line(*state_.board, line)));
            };
            DIAG_COUNT(++state_.diag.asp_windows);
            while (true) {
                // Full depth on every re-search. Stockfish re-searches
                // shallower after each fail high; here that read WAC 137 -> 119
                // against a floor of 130 and raised re-searches 1305 -> 1450,
                // because a root failing high is often a tactical shot that a
                // shallower search misses (BAS-D17).
                score = root_search(depth, asp_a, asp_b);
                if (state_.stopped) break;
                if (score <= asp_a) {
                    bound_line(score, false);
                    DIAG_COUNT(++state_.diag.asp_fail_low);
                    DIAG_COUNT(++state_.diag.asp_researches);
                    // Only alpha moves, and delta grows by half. Pulling beta
                    // to the window's midpoint on a fail low, as Stockfish does,
                    // raised re-searches 1305 -> 1342 here because the tighter
                    // window fails again (18 positions deeper, 19 shallower);
                    // its slower growth, delta/4 + 5, measured -0.243 ply
                    // (BAS-D16).
                    asp_a  = std::max(score - delta, -INF_SCORE);
                    delta += delta / 2;
                } else if (score >= asp_b) {
                    bound_line(score, true);
                    DIAG_COUNT(++state_.diag.asp_fail_high);
                    // Past the optimum, a fail-high on the move the last
                    // iteration chose only proves that move better than the
                    // window expected: play it rather than spend the rest of
                    // the budget measuring by how much. A rising score in a won
                    // ending otherwise re-searches at full depth until the hard
                    // maximum. The iteration is discarded as on any stop, so
                    // the move played is that iteration's, and it is this one.
                    if (state_.soft_limit > 0.0 && !state_.pondering && config_.thread_id == 0
                        && state_.pv_len[0] > 0 && state_.pv_table[0][0] == result.bestmove
                        && elapsed_seconds() >= state_.soft_limit) {
                        state_.stopped = true;
                        break;
                    }
                    DIAG_COUNT(++state_.diag.asp_researches);
                    asp_b  = std::min(score + delta, INF_SCORE);
                    delta += delta / 2;
                } else {
                    break;
                }
                if (delta >= 900) {
                    DIAG_COUNT(++state_.diag.asp_giveup);
                    DIAG_COUNT(++state_.diag.asp_researches);
                    asp_a = -INF_SCORE;
                    asp_b =  INF_SCORE;
                    score = root_search(depth, asp_a, asp_b);
                    break;
                }
            }
        }

        if (state_.stopped && depth > 1) break;

        if (state_.root_depth_nodes > 0)
            state_.root_best_effort = static_cast<int>(
                std::min<int64_t>(100, state_.root_best_nodes * 100 / state_.root_depth_nodes));

        // Track best-move stability for adaptive soft time limit
        Move cur_best = (state_.pv_len[0] > 0) ? state_.pv_table[0][0] : MOVE_NONE;

        int reported_score = score;
        if (cur_best != MOVE_NONE) {
            const int tb_score = root_tablebase_display(cur_best);
            if (tb_score != VALUE_NONE)
                reported_score = tb_score;
        }

        int prev_score_saved = prev_score;
        prev_score = score;
        // Decaying best-move-change signal (Stockfish's totBestMoveChanges):
        // halved each iteration, a flip adds one. It extends time when the root
        // best move is thrashing, complementing stability_scale, which only
        // shrinks time when the move is stable.
        best_move_changes *= 0.5;
        if (cur_best == prev_best)
            best_stability++;
        else {
            best_stability = 0;
            prev_best      = cur_best;
            if (depth > 1)
                best_move_changes += 1.0;
        }

        if (state_.pv_len[0] > 0) {
            result.bestmove   = state_.pv_table[0][0];
            result.pondermove = (state_.pv_len[0] > 1) ? state_.pv_table[0][1] : MOVE_NONE;
            result.pv.assign(state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY));
            if (result.pondermove == MOVE_NONE)
                result.pondermove = ponder_from_tt(board, result.bestmove);
        }
        result.score = reported_score;
        result.depth = depth;
        result.seldepth = state_.sel_depth;

        // The table is given the raw `score`, not the tablebase-corrected
        // `reported_score` that goes out over UCI: its scores order root moves
        // for the next iteration and the helper threads, and a tablebase
        // verdict carries no ordering information. The asymmetry is deliberate.
        if (shared_.pool.root_table && result.bestmove != MOVE_NONE)
            shared_.pool.root_table->update(result.bestmove, result.pondermove, depth, score);

        // The first line's own PV, before any later line overwrites the table.
        const std::vector<Move> first_line(state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY));

        // MultiPV lines 2..N: each is the best of the root moves the earlier
        // lines did not play, searched at full width. A line a stop cuts short
        // is not reported; its previous depth stands.
        struct ExtraLine { int score; std::vector<Move> pv; };
        std::vector<ExtraLine> extra_lines;
        if (state_.multipv_lines > 1 && cur_best != MOVE_NONE && !state_.stopped) {
            state_.root_excluded.assign(1, cur_best);
            for (int k = 2; k <= state_.multipv_lines; ++k) {
                state_.pv_len[0] = 0;
                const int line_score = root_search(depth, -INF_SCORE, INF_SCORE);
                if (state_.stopped || state_.pv_len[0] == 0)
                    break;
                extra_lines.push_back({line_score, std::vector<Move>(
                    state_.pv_table[0], state_.pv_table[0] + std::clamp(state_.pv_len[0], 0, MAX_PLY))});
                state_.root_excluded.push_back(state_.pv_table[0][0]);
            }
            state_.root_excluded.clear();
        }

        double elapsed = elapsed_seconds();
        if (extra_lines.empty()) {
            send_info(depth, 1, reported_score, first_line, current_nodes(), elapsed);
        } else {
            // Report the lines best first, as Stockfish sorts its root moves, so
            // `bestmove` is always line 1: a later line searched without the
            // first can score above it.
            std::vector<ExtraLine> lines;
            lines.push_back({reported_score, first_line});
            for (ExtraLine& line : extra_lines) {
                const int tb_display = root_tablebase_display(line.pv.front());
                lines.push_back({tb_display != VALUE_NONE ? tb_display : line.score,
                                 std::move(line.pv)});
            }
            std::stable_sort(lines.begin(), lines.end(),
                             [](const ExtraLine& a, const ExtraLine& b) { return a.score > b.score; });
            if (lines.front().pv.front() != result.bestmove) {
                result.bestmove   = lines.front().pv.front();
                result.pv         = lines.front().pv;
                result.pondermove = result.pv.size() > 1 ? result.pv[1]
                                  : ponder_from_tt(board, result.bestmove);
                result.score      = lines.front().score;
            }
            for (size_t k = 0; k < lines.size(); ++k)
                send_info(depth, static_cast<int>(k) + 1, lines[k].score, lines[k].pv,
                          current_nodes(), elapsed);
        }

        // Adaptive soft time limit:
        // The more stable the best move, the less time we need to confirm it.
        // stability=0 → 100% of soft, stability=6+ → ~64% of soft
        // A significant score drop signals instability — extend time budget.
        // Only the main thread owns a clock. Helpers run until `shared_.stop`,
        // which the pool sets when main returns.
        if (state_.soft_limit > 0.0 && !state_.pondering && config_.thread_id == 0) {
            const SearchParams& tp = config_.limits.params;
            double stability_scale = 1.0 - (tp.tm_stability / 1000.0) * std::min(best_stability, 6);
            // Score-based time extension: if score dropped enough, take more time
            int score_drop = prev_score_saved - score;
            double score_scale = (depth > 4 && score_drop > tp.tm_scoredrop_thr)
                               ? 1.0 + std::min(score_drop - tp.tm_scoredrop_thr, 120)
                                       / static_cast<double>(tp.tm_scoredrop_div)
                               : 1.0;
            double effort_scale = (depth > 5 && state_.root_best_effort >= tp.tm_effort_hi) ? tp.tm_effort_hi_mult / 100.0
                                : (depth > 5 && state_.root_best_effort <= tp.tm_effort_lo) ? tp.tm_effort_lo_mult / 100.0
                                : 1.0;
            // Instability extension: a thrashing root best move raises the
            // threshold and buys more time, complementing stability_scale.
            double instability_scale = 1.0 + std::min(best_move_changes, 2.0) * (tp.tm_instability / 100.0);
            if (elapsed >= state_.soft_limit * stability_scale * score_scale
                         * effort_scale * instability_scale)
                break;
        }

        // Do not stop at the first forced mate. A shallow iteration can find a
        // longer checking mate before a deeper iteration sees a shorter quiet
        // mating net. Only mate-in-1 is impossible to improve.
        if (limits.mate > 0 && std::abs(score) >= MATE_SCORE - MAX_PLY) {
            const int mate_in = (MATE_SCORE - std::abs(score) + 1) / 2;
            if (mate_in > 0 && mate_in <= limits.mate)
                break;
        }
        if (score >= MATE_SCORE - 1)
            break;
    }

    result = sanitize_search_result(board, result);
    // Publish whatever this thread accumulated since the last batch boundary,
    // so the shared total is exact once the pool joins.
    flush_shared_nodes();
    // Harvest the Board-side speed counters before state_.board is dropped;
    // print_search_diag() runs after this point.
    state_.diag.see_ge_calls      = board.see_ge_call_count();
    state_.diag.gives_check_calls = board.gives_check_call_count();
    state_.board = nullptr;
    shared_.pool = PoolResources{};
    state_.root_ordering = RootOrdering{};
    state_.pondering = false;
    state_.root_tb_moves.clear();
    result.nodes      = state_.nodes;
    result.tbhits     = state_.tb_hits;
    result.elapsed_ms = int64_t(elapsed_seconds() * 1000.0);

    if (shared_.info && config_.limits.diag)
        print_search_diag(state_.diag, evaluator_, shared_.info);
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    if (config_.limits.decision_trace)
        state_.trace.print(shared_.info, config_.limits.root_moves);
#endif

    // TM_Debug: one line per move with the time budget, the elapsed time and
    // the go-receipt to search-start dispatch latency, on the reporting
    // thread only.
    if (shared_.info && config_.limits.tm_debug) {
        long long dispatch_ms = -1;
        if (config_.limits.go_recv_time.time_since_epoch().count() != 0)
            dispatch_ms = int64_t(std::chrono::duration<double, std::milli>(
                              state_.start_time - config_.limits.go_recv_time).count());
        shared_.info("info string tm soft_ms=" + std::to_string(int64_t(state_.soft_limit * 1000.0))
               + " hard_ms="     + std::to_string(int64_t(state_.hard_limit * 1000.0))
               + " elapsed_ms="  + std::to_string(int64_t(elapsed_seconds() * 1000.0))
               + " dispatch_ms=" + std::to_string(dispatch_ms));
    }
    return result;
}

