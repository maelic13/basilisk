#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include "engine_command.h"
#include "parameters.h"
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

    // Called when a `position` command is rejected, with the diagnostic line
    // already written. The default flushes and ends the process with exit
    // status 1 (Stockfish/Rarog practice). Tests install their own handler;
    // if it returns, the engine loop ends as if the process had.
    using FatalHandler = std::function<void(const std::string& message)>;
    void set_fatal_handler(FatalHandler handler) { fatal_handler_ = std::move(handler); }

private:
    void handle_command(const EngineCommand& command, bool& quit);
    void start_search(uint64_t command_epoch,
                      std::chrono::steady_clock::time_point recv_time = {});
    void run_bench_command(const EngineCommand& command);
    void run_wac_command(const EngineCommand& command);
    void run_perft_command(uint64_t command_epoch);
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

    TranspositionTable tt_;
    SearchThreadPool   search_pool_;
    int current_hash_mb_ = 64;

    void apply_table_state();
    std::string current_syzygy_path_;
    FatalHandler fatal_handler_;
};

