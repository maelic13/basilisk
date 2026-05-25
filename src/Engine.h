#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include "EngineCommand.h"
#include "Parameters.h"
#include "tt.h"
#include "search.h"

class Engine {
public:
    explicit Engine(EngineCommandQueue& commands,
                    std::atomic_bool& stop_requested,
                    std::atomic_bool& ponderhit_requested,
                    std::atomic_bool& searching,
                    std::atomic_uint64_t& control_epoch);

    void start();

private:
    void handle_command(const EngineCommand& command, bool& quit);
    void start_search(uint64_t command_epoch);
    void run_bench_command(const EngineCommand& command);
    void configure_syzygy();
    void send_bestmove(const SearchResult& result, const Board& root_board) const;
    void wait_until_bestmove_allowed(const SearchLimits& limits, uint64_t command_epoch) const;
    SearchLimits build_limits() const;

    EngineCommandQueue& commands_;
    std::atomic_bool& stop_requested_;
    std::atomic_bool& ponderhit_requested_;
    std::atomic_bool& searching_;
    std::atomic_uint64_t& control_epoch_;

    Parameters parameters_;

    TranspositionTable tt;
    SearchThreadPool   search_pool_;
    int current_hash_mb_ = 64;
    std::string current_syzygy_path_;
};

