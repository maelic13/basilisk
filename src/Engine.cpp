#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <thread>

#include "Constants.h"
#include "Engine.h"
#include "UciOutput.h"
#include "bench.h"
#include "syzygy.h"

Engine::Engine(EngineCommandQueue& commands,
               std::atomic_bool& stop_requested,
               std::atomic_bool& ponderhit_requested,
               std::atomic_bool& searching,
               std::atomic_uint64_t& control_epoch)
    : commands_(commands)
    , stop_requested_(stop_requested)
    , ponderhit_requested_(ponderhit_requested)
    , searching_(searching)
    , control_epoch_(control_epoch)
    , tt(64)
    , search_pool_(tt, stop_requested_,
          [](const std::string& info) {
              uci_write_line(info);
          },
          &ponderhit_requested_)
{
}

SearchLimits Engine::build_limits() const {
    SearchLimits limits;
    limits.depth     = parameters_.depth;
    limits.movetime  = parameters_.moveTime;
    limits.wtime     = parameters_.whiteTime;
    limits.btime     = parameters_.blackTime;
    limits.winc      = parameters_.whiteIncrement;
    limits.binc      = parameters_.blackIncrement;
    limits.movestogo = parameters_.movestogo;
    limits.nodes     = parameters_.nodes;
    limits.overhead  = parameters_.moveOverhead;
    limits.ponder    = parameters_.ponder;
    limits.syzygy_probe_depth = Syzygy::enabled() ? parameters_.syzygyProbeDepth : 0;
    limits.syzygy_probe_limit = Syzygy::enabled() ? parameters_.syzygyProbeLimit : 0;
    limits.syzygy_50_move_rule = parameters_.syzygy50MoveRule;
    limits.infinite  = (parameters_.depth == infiniteDepth && parameters_.moveTime == 0
                        && parameters_.whiteTime == 0 && parameters_.blackTime == 0
                        && parameters_.whiteIncrement == 0 && parameters_.blackIncrement == 0
                        && parameters_.movestogo == 0
                        && parameters_.nodes == 0
                        && !parameters_.ponder);
    return limits;
}

void Engine::configure_syzygy() {
    if (parameters_.syzygyPath == current_syzygy_path_)
        return;

    current_syzygy_path_ = parameters_.syzygyPath;
    const bool ok = Syzygy::init(current_syzygy_path_);

    if (current_syzygy_path_.empty()) {
        uci_write_line("info string Syzygy disabled");
    } else if (!ok) {
        uci_write_line("info string Syzygy initialization failed for path: "
                       + current_syzygy_path_);
    } else if (!Syzygy::enabled()) {
        uci_write_line("info string Syzygy path set but no tablebase files were found");
    } else {
        uci_write_line("info string Found "
                       + std::to_string(Syzygy::wdl_file_count()) + " WDL and "
                       + std::to_string(Syzygy::dtz_file_count())
                       + " DTZ tablebase files (up to "
                       + std::to_string(Syzygy::largest()) + "-man).");
    }
}

void Engine::send_bestmove(const SearchResult& result, const Board& root_board) const {
    SearchResult safe_result = sanitize_search_result(root_board, result);
    if (result.bestmove != MOVE_NONE && result.bestmove != safe_result.bestmove) {
        std::string replacement = safe_result.bestmove == MOVE_NONE
            ? "0000"
            : move_to_uci(safe_result.bestmove);
        uci_write_line("info string Replaced illegal bestmove "
                       + move_to_uci(result.bestmove)
                       + " with " + replacement);
    }

    if (safe_result.bestmove != MOVE_NONE) {
        std::string out = "bestmove " + move_to_uci(safe_result.bestmove);
        if (safe_result.pondermove != MOVE_NONE)
            out += " ponder " + move_to_uci(safe_result.pondermove);
        uci_write_line(out);
    } else {
        uci_write_line("bestmove 0000");
    }
}

void Engine::wait_until_bestmove_allowed(const SearchLimits& limits,
                                         uint64_t command_epoch) const {
    if (!limits.ponder && !limits.infinite)
        return;

    // A ponder/infinite search may exhaust its depth cap before the GUI sends
    // stop or ponderhit. Keep the completed result but do not emit bestmove yet.
    while (!stop_requested_.load(std::memory_order_acquire)) {
        if (limits.ponder && ponderhit_requested_.load(std::memory_order_acquire))
            return;
        if (command_epoch != 0
            && control_epoch_.load(std::memory_order_acquire) != command_epoch)
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Engine::start_search(uint64_t command_epoch) {
    configure_syzygy();

    SearchLimits limits = build_limits();
    const int desired_hash_mb = parameters_.hash_mb;
    const int desired_threads = parameters_.threads;
    const bool do_clear_hash = parameters_.clear_hash;
    const bool is_new_game = parameters_.new_game;
    Board board_copy = parameters_.board;
    const bool stop_was_requested = stop_requested_.load(std::memory_order_acquire);

    if (Syzygy::enabled()) {
        limits.syzygy_root_moves = Syzygy::probe_root_moves(board_copy,
                                                            parameters_.syzygy50MoveRule,
                                                            parameters_.syzygyProbeLimit,
                                                            true);
        if (!limits.syzygy_root_moves.empty()) {
            const int best_rank = limits.syzygy_root_moves.front().rank;
            limits.syzygy_root_moves.erase(
                std::remove_if(limits.syzygy_root_moves.begin(),
                               limits.syzygy_root_moves.end(),
                               [best_rank](const Syzygy::RootMoveInfo& move) {
                                   return move.rank != best_rank;
                               }),
                limits.syzygy_root_moves.end());
        }
        for (auto& move : limits.syzygy_root_moves) {
            move.pv = Syzygy::extend_pv(board_copy, {move.bestmove},
                                        parameters_.syzygy50MoveRule,
                                        parameters_.syzygyProbeLimit,
                                        MAX_PLY / 2);
        }
    }

    parameters_.new_game = false;
    parameters_.clear_hash = false;

    if (command_epoch != 0
        && control_epoch_.load(std::memory_order_acquire) != command_epoch) {
        send_bestmove(SearchResult{}, board_copy);
        return;
    }

    if (desired_hash_mb != current_hash_mb_) {
        tt.resize(static_cast<size_t>(desired_hash_mb));
        current_hash_mb_ = desired_hash_mb;
    }

    const int active_threads = search_pool_.resize_threads(desired_threads);
    if (active_threads != desired_threads)
        parameters_.threads = active_threads;

    if (is_new_game || do_clear_hash) {
        tt.clear();
        search_pool_.clear();
    }

    if (stop_was_requested && (limits.infinite || limits.ponder)) {
        ponderhit_requested_.store(false, std::memory_order_release);
        searching_.store(false, std::memory_order_release);
        send_bestmove(SearchResult{}, board_copy);
        return;
    }

    ponderhit_requested_.store(false, std::memory_order_release);
    stop_requested_.store(false, std::memory_order_release);

    searching_.store(true, std::memory_order_release);

    SearchResult result = search_pool_.search(board_copy, limits, active_threads);
    wait_until_bestmove_allowed(limits, command_epoch);

    if (command_epoch == 0
        || control_epoch_.load(std::memory_order_acquire) == command_epoch)
        searching_.store(false, std::memory_order_release);
    send_bestmove(result, board_copy);
    ponderhit_requested_.store(false, std::memory_order_release);
    stop_requested_.store(false, std::memory_order_release);
}

void Engine::run_bench_command(const EngineCommand& command) {
    int depth = 13;
    if (!command.args.empty()) {
        try { depth = std::stoi(command.args); } catch (...) {}
    }

    if (command.epoch != 0
        && control_epoch_.load(std::memory_order_acquire) != command.epoch)
        return;

    stop_requested_.store(false, std::memory_order_release);
    searching_.store(true, std::memory_order_release);
    run_bench(depth, parameters_.threads);
    if (command.epoch == 0
        || control_epoch_.load(std::memory_order_acquire) == command.epoch)
        searching_.store(false, std::memory_order_release);
}

void Engine::handle_command(const EngineCommand& command, bool& quit) {
    switch (command.type) {
        case EngineCommandType::SetOption:
        {
            const int old_threads = parameters_.threads;
            parameters_.setOption(command.args);
            configure_syzygy();
            if (parameters_.threads != old_threads) {
                const int active_threads = search_pool_.resize_threads(parameters_.threads);
                parameters_.threads = active_threads;
                uci_write_line("info string Using "
                               + std::to_string(active_threads)
                               + (active_threads == 1 ? " thread" : " threads"));
            }
            break;
        }
        case EngineCommandType::Position:
            parameters_.setPosition(command.args);
            break;
        case EngineCommandType::NewGame:
            parameters_.reset();
            break;
        case EngineCommandType::Go:
            parameters_.setSearchParameters(command.args);
            start_search(command.epoch);
            break;
        case EngineCommandType::Stop:
            if (command.epoch == 0
                || control_epoch_.load(std::memory_order_acquire) == command.epoch)
                searching_.store(false, std::memory_order_release);
            stop_requested_.store(false, std::memory_order_release);
            ponderhit_requested_.store(false, std::memory_order_release);
            break;
        case EngineCommandType::PonderHit:
            ponderhit_requested_.store(true, std::memory_order_release);
            parameters_.ponder = false;
            break;
        case EngineCommandType::Bench:
            run_bench_command(command);
            break;
        case EngineCommandType::Ready:
            if (command.ack)
                command.ack->set_value();
            break;
        case EngineCommandType::Quit:
            stop_requested_.store(true, std::memory_order_release);
            quit = true;
            break;
    }
}

void Engine::start() {
    bool quit = false;
    while (!quit) {
        EngineCommand command = commands_.wait_pop();
        handle_command(command, quit);
    }
}
