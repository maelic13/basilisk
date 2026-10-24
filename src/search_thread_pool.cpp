#include "search_thread_pool.h"
#include "constants.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

// ---- Persistent Lazy SMP thread pool ---------------------------------------

SearchThreadPool::SearchThreadPool(TranspositionTable& tt,
                                   std::atomic_bool& stop_flag,
                                   InfoCallback info_cb,
                                   std::atomic_bool* ponderhit_flag)
    : tt_(tt)
    , stop_(stop_flag)
    , ponderhit_(ponderhit_flag)
    , info_cb_(std::move(info_cb)) {
    resize_threads(1);
}

SearchThreadPool::~SearchThreadPool() {
    {
        std::scoped_lock lock(mutex_);
        shutdown_ = true;
        ++epoch_;
    }
    work_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable())
            worker.join();
    }
}

int SearchThreadPool::ensure_threads(int count) {
    return resize_threads(count);
}

// The one definition of the Threads cap (declared in constants.h): flat 1024,
// as Stockfish does it; constants.h says why the machine is not consulted. It
// is defined once because the option advertisement and the pool's real limit
// once computed it independently and could drift apart.
int max_search_threads() {
    return maxSearchThreads;
}

int SearchThreadPool::normalize_thread_count(int count) {
    count = std::max(1, count);
    return std::min(count, max_search_threads());
}

int SearchThreadPool::active_thread_count() const {
    std::scoped_lock lock(mutex_);
    return static_cast<int>(searchers_.size());
}

int SearchThreadPool::resize_threads(int count) {
    count = normalize_thread_count(count);

    bool already_exact = false;
    {
        std::scoped_lock lock(mutex_);
        already_exact = !shutdown_
            && std::cmp_equal(searchers_.size(), count)
            && static_cast<int>(workers_.size()) + 1 == count;
    }
    if (already_exact)
        return count;

    {
        std::scoped_lock lock(mutex_);
        shutdown_ = true;
        ++epoch_;
    }
    work_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable())
            worker.join();
    }

    {
        std::scoped_lock lock(mutex_);
        workers_.clear();
        searchers_.clear();
        job_results_ = nullptr;
        job_resources_ = PoolResources{};
        requested_helpers_ = 0;
        active_helpers_ = 0;
        shutdown_ = false;
        epoch_ = 0;
    }

    while (std::cmp_less(searchers_.size(), count)) {
        const bool emit_info = searchers_.empty();
        auto cb = emit_info ? info_cb_ : InfoCallback();
        searchers_.push_back(std::make_unique<Searcher>(tt_, stop_, std::move(cb), ponderhit_));
    }

    while (static_cast<int>(workers_.size()) + 1 < count) {
        const int helper_slot = static_cast<int>(workers_.size());
        try {
            workers_.emplace_back(&SearchThreadPool::worker_loop, this, helper_slot);
        } catch (const std::system_error& e) {
            if (info_cb_) {
                info_cb_("info string Threads reduced to "
                         + std::to_string(static_cast<int>(workers_.size()) + 1)
                         + " after worker creation failed: " + e.what());
            }
            break;
        }
    }

    const int active_count = std::min<int>(count, static_cast<int>(workers_.size()) + 1);
    if (std::cmp_greater(searchers_.size(), active_count))
        searchers_.resize(static_cast<size_t>(active_count));
    return active_count;
}

void SearchThreadPool::clear() {
    for (auto& searcher : searchers_)
        searcher->clear();
}

SearchConfig SearchThreadPool::config_for_thread(const SearchLimits& limits,
                                                 int thread_id,
                                                 int thread_count) const {
    SearchConfig config;
    config.limits = limits;
    config.update_tt_age = false;
    config.thread_id = thread_id;
    config.thread_count = thread_count;
    config.root_filter_count = 1;
    config.root_filter_index = -1;

    // Helpers do not inherit the depth limit: under `go depth N` they would
    // stop at N and idle instead of widening the shared table for the main
    // thread, which keeps the depth contract because its result is the one
    // reported. Clock-limited games carry no depth limit, and one thread never
    // reaches this function.
    if (thread_id > 0)
        config.limits.depth = infiniteDepth;

    return config;
}

SearchResult SearchThreadPool::merge_results(const std::vector<SearchResult>& results,
                                             int count,
                                             const RootMoveTable& root_table,
                                             int64_t elapsed_ms) const {
    SearchResult best = root_table.best_result();
    int64_t total_nodes = 0;
    int64_t total_tbhits = 0;

    for (int i = 0; i < count; ++i) {
        const SearchResult& result = results[static_cast<size_t>(i)];
        total_nodes += result.nodes;
        total_tbhits += result.tbhits;

        if (result.bestmove == MOVE_NONE || !root_table.contains(result.bestmove))
            continue;

        const bool result_mates = result.score >= MATE_SCORE - MAX_PLY;
        const bool best_mates = best.score >= MATE_SCORE - MAX_PLY;
        if (best.bestmove == MOVE_NONE
            || (result_mates && (!best_mates || result.score > best.score))
            || (!best_mates && result.depth > best.depth)
            || (!best_mates && result.depth == best.depth && result.score > best.score)) {
            best = result;
        }
    }

    if (best.bestmove == MOVE_NONE)
        best.bestmove = root_table.fallback_move();

    best.nodes = total_nodes;
    best.tbhits = total_tbhits;
    best.elapsed_ms = elapsed_ms;
    return best;
}

SearchResult SearchThreadPool::search(Board board, const SearchLimits& limits, int thread_count) {
    thread_count = resize_threads(thread_count);
    const Board root_board = board;

    if (thread_count <= 1)
        return sanitize_search_result(root_board, searchers_[0]->search(std::move(board), limits));

    tt_.new_search();

    RootMoveTable root_table;
    root_table.reset(board, limits.root_moves, limits.syzygy_root_moves);

    std::vector<SearchResult> results(static_cast<size_t>(thread_count));
    // One cache line each: as adjacent atomics every tablebase-hit publish
    // invalidated the node counter's line for every thread and vice versa.
    // Batching in record_node() makes the traffic rare; this makes what
    // remains non-interfering.
    alignas(64) std::atomic<int64_t> shared_nodes{0};
    alignas(64) std::atomic<int64_t> shared_tbhits{0};
    const PoolResources resources{&shared_nodes, &shared_tbhits, &root_table};
    const auto wall_start = std::chrono::steady_clock::now();

    {
        std::scoped_lock lock(mutex_);
        job_board_ = board;
        job_limits_ = limits;
        job_results_ = &results;
        job_resources_ = resources;
        requested_helpers_ = thread_count - 1;
        active_helpers_ = requested_helpers_;
        ++epoch_;
    }
    work_cv_.notify_all();

    results[0] = searchers_[0]->search(std::move(board),
                                       config_for_thread(limits, 0, thread_count), resources);

    while (!stop_.load(std::memory_order_acquire) && (limits.ponder || limits.infinite)) {
        if (limits.ponder && ponderhit_ && ponderhit_->load(std::memory_order_acquire))
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    stop_.store(true, std::memory_order_release);

    {
        std::unique_lock lock(mutex_);
        done_cv_.wait(lock, [&] { return active_helpers_ == 0; });
        job_results_ = nullptr;
        job_resources_ = PoolResources{};
        requested_helpers_ = 0;
    }

    // The pool aggregate, printed after the join because that is the first
    // moment the helpers' counters are complete. Thread 0 owns info_cb_, so it
    // does the printing. Gated here as well as inside, so normal play does not
    // build the depth vector on every multi-thread search.
    if (limits.diag) {
        std::vector<int> completed_depths;
        completed_depths.reserve(static_cast<size_t>(thread_count));
        for (int i = 0; i < thread_count; ++i)
            completed_depths.push_back(results[static_cast<size_t>(i)].depth);
        searchers_[0]->print_pool_diag(searchers_, thread_count, completed_depths);
    }

    const int64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - wall_start).count();

    SearchResult merged = merge_results(results, thread_count, root_table, elapsed_ms);
    // With several lines the main thread's first line is the answer, as in
    // Stockfish: helpers search one line and do not vote.
    if (limits.multipv > 1 && results[0].bestmove != MOVE_NONE) {
        const SearchResult& main_result = results[0];
        merged.bestmove   = main_result.bestmove;
        merged.pondermove = main_result.pondermove;
        merged.score      = main_result.score;
        merged.depth      = main_result.depth;
        merged.seldepth   = main_result.seldepth;
        merged.pv         = main_result.pv;
    }

    // The merged result may come from a helper, or from the shared root table,
    // which keeps no line. Take the line of the thread that reached it, so the
    // last `info` before `bestmove` names the move played (Stockfish prints its
    // best thread's PV the same way). Its score is the reported one, which at
    // a tablebase root differs from the table's raw value.
    int line_score = merged.score;
    for (int i = 0; i < thread_count; ++i) {
        const SearchResult& result = results[static_cast<size_t>(i)];
        if (result.bestmove == merged.bestmove && result.depth == merged.depth) {
            if (merged.pv.empty() || merged.pv.front() != merged.bestmove) {
                merged.pv = result.pv;
                merged.seldepth = result.seldepth;
            }
            line_score = result.score;
            break;
        }
    }
    if (merged.pv.empty() || merged.pv.front() != merged.bestmove) {
        merged.pv.assign(1, merged.bestmove);
        if (merged.pondermove != MOVE_NONE)
            merged.pv.push_back(merged.pondermove);
    }
    if (info_cb_ && needs_pool_line(merged, results[0])) {
        info_cb_(format_info_line(merged.depth, merged.seldepth, 1, line_score, {}, merged.nodes,
                                  static_cast<double>(elapsed_ms) / 1000.0, merged.tbhits,
                                  tt_.hashfull(), legal_line(root_board, merged.pv)));
    }

    return sanitize_search_result(root_board, merged);
}

void SearchThreadPool::worker_loop(int helper_slot) {
    uint64_t seen_epoch = 0;

    while (true) {
        Board board;
        SearchConfig config;
        PoolResources resources;
        std::vector<SearchResult>* results = nullptr;
        int thread_id = helper_slot + 1;
        int thread_count = 1;

        {
            std::unique_lock lock(mutex_);
            work_cv_.wait(lock, [&] { return shutdown_ || epoch_ != seen_epoch; });
            if (shutdown_)
                return;

            seen_epoch = epoch_;
            if (helper_slot >= requested_helpers_ || !job_results_ || !job_resources_.root_table)
                continue;

            board = job_board_;
            resources = job_resources_;
            results = job_results_;
            thread_count = requested_helpers_ + 1;
            config = config_for_thread(job_limits_, thread_id, thread_count);
        }

        SearchResult result = searchers_[static_cast<size_t>(thread_id)]->search(
            std::move(board), std::move(config), resources);

        {
            std::scoped_lock lock(mutex_);
            if (results && std::cmp_less(thread_id, results->size()))
                (*results)[static_cast<size_t>(thread_id)] = result;

            if (active_helpers_ > 0)
                --active_helpers_;
            if (active_helpers_ == 0)
                done_cv_.notify_one();
        }
    }
}
