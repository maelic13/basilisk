#pragma once

// The persistent Lazy SMP pool: one Searcher per thread, helpers parked
// between searches, a shared root table and node counters per `go`, and the
// merge of the threads' results into the one the engine plays.

#include "board.h"
#include "search_root.h"
#include "search_types.h"
#include "search_worker.h"
#include "tt.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class SearchThreadPool {
public:
    SearchThreadPool(TranspositionTable& tt,
                     std::atomic_bool& stop_flag,
                     InfoCallback info_cb = nullptr,
                     std::atomic_bool* ponderhit_flag = nullptr);
    ~SearchThreadPool();

    SearchThreadPool(const SearchThreadPool&) = delete;
    SearchThreadPool& operator=(const SearchThreadPool&) = delete;

    int ensure_threads(int count);
    int resize_threads(int count);
    int active_thread_count() const;
    void clear();
    SearchResult search(Board board, const SearchLimits& limits, int thread_count);

private:
    void worker_loop(int helper_slot);
    SearchConfig config_for_thread(const SearchLimits& limits, int thread_id,
                                   int thread_count) const;
    SearchResult merge_results(const std::vector<SearchResult>& results, int count,
                               const RootMoveTable& root_table, int64_t elapsed_ms) const;
    static int normalize_thread_count(int count);

    TranspositionTable& tt_;
    std::atomic_bool& stop_;
    std::atomic_bool* ponderhit_;
    InfoCallback info_cb_;

    std::vector<std::unique_ptr<Searcher>> searchers_;
    std::vector<std::thread> workers_;

    mutable std::mutex mutex_;
    std::condition_variable work_cv_;
    std::condition_variable done_cv_;
    bool shutdown_ = false;
    uint64_t epoch_ = 0;

    Board job_board_;
    SearchLimits job_limits_;
    std::vector<SearchResult>* job_results_ = nullptr;
    PoolResources job_resources_;
    int requested_helpers_ = 0;
    int active_helpers_ = 0;
};
