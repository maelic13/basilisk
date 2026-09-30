#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

#include "constants.h"
#include "engine.h"
#include "uci_output.h"
#include "bench.h"
#include "wac.h"
#include "syzygy.h"

namespace {

uint64_t perft(Board& board, int depth) {
    if (depth <= 0)
        return 1;

    MoveList legal;
    board.gen_legal(legal);
    if (depth == 1)
        return static_cast<uint64_t>(legal.size());

    uint64_t nodes = 0;
    for (Move move : legal) {
        board.make_move(move);
        nodes += perft(board, depth - 1);
        board.unmake_move(move);
    }
    return nodes;
}

bool move_allowed_by_root_list(Move move, const std::vector<Move>& root_moves) {
    return root_moves.empty()
        || std::find(root_moves.begin(), root_moves.end(), move) != root_moves.end();
}

} // namespace

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
    , tt_(64)
    , search_pool_(tt_, stop_requested_,
          [](const std::string& info) {
              uci_write_line(info);
          },
          &ponderhit_requested_)
{
}

SearchLimits Engine::build_limits() const {
    SearchLimits limits;
    limits.depth     = parameters_.depth;
    limits.movetime  = parameters_.move_time;
    limits.wtime     = parameters_.white_time;
    limits.btime     = parameters_.black_time;
    limits.winc      = parameters_.white_increment;
    limits.binc      = parameters_.black_increment;
    limits.movestogo = parameters_.movestogo;
    limits.nodes     = parameters_.nodes;
    limits.mate      = parameters_.mate;
    limits.overhead  = parameters_.move_overhead;
    limits.ponder    = parameters_.ponder;
    limits.root_moves = parameters_.search_moves;
    limits.syzygy_probe_depth = Syzygy::enabled() ? parameters_.syzygy_probe_depth : 0;
    limits.syzygy_probe_limit = Syzygy::enabled() ? parameters_.syzygy_probe_limit : 0;
    limits.syzygy_50_move_rule = parameters_.syzygy_50_move_rule;
    limits.tm_debug  = parameters_.tm_debug;
    limits.diag      = parameters_.diag;
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    limits.decision_trace = parameters_.decision_trace;
#endif
#ifdef BASILISK_ABLATION
    limits.ablation_mask = parameters_.ablation_mask;
#endif
    limits.params    = parameters_.search_params;
    limits.infinite  = (parameters_.depth == infiniteDepth && parameters_.move_time == 0
                        && parameters_.white_time == 0 && parameters_.black_time == 0
                        && parameters_.white_increment == 0 && parameters_.black_increment == 0
                        && parameters_.movestogo == 0
                        && parameters_.nodes == 0
                        && parameters_.mate == 0
                        && !parameters_.ponder);
    return limits;
}

void Engine::configure_syzygy() {
    if (parameters_.syzygy_path == current_syzygy_path_)
        return;

    current_syzygy_path_ = parameters_.syzygy_path;
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

// Stockfish-style final tablebase PV (Syzygy::extend_pv): extend the chosen
// line once, re-send it, and take the ponder move from it. Under a clock or
// movetime this runs on the engine's clock, so like Stockfish it stops once
// 2 x elapsed >= Move Overhead: never more than half the overhead the GUI
// latency reserve already allows. Without a time limit it is unbounded, as in
// Stockfish's analysis mode.
void Engine::publish_tablebase_pv(SearchResult& result, const Board& root_board,
                                  const SearchLimits& limits) const {
    if (!Syzygy::enabled() || result.bestmove == MOVE_NONE
        || !is_tablebase_decisive(result.score))
        return;

    std::vector<Move> line = result.pv;
    if (line.empty() || line.front() != result.bestmove) {
        line.assign(1, result.bestmove);
        if (result.pondermove != MOVE_NONE)
            line.push_back(result.pondermove);
    }

    const auto start = std::chrono::steady_clock::now();
    const bool timed = limits.movetime > 0 || limits.wtime > 0 || limits.btime > 0;
    const int overhead_ms = limits.overhead;
    auto time_abort = [&] {
        if (!timed)
            return false;
        const double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        return 2.0 * elapsed_ms >= overhead_ms;
    };

    Syzygy::PvExtension ext = Syzygy::extend_pv(root_board, line,
                                                 limits.syzygy_50_move_rule,
                                                 limits.syzygy_probe_limit, time_abort);
    if (ext.timed_out)
        uci_write_line("info string Syzygy based PV extension requires more time, "
                       "increase Move Overhead as needed.");
    if (ext.pv == line && !ext.ends_in_draw)
        return;

    const int score = ext.ends_in_draw ? 0 : result.score;
    std::string info = "info depth " + std::to_string(result.depth)
        + " score cp " + std::to_string(score)
        + " nodes " + std::to_string(result.nodes)
        + " time " + std::to_string(result.elapsed_ms)
        + " tbhits " + std::to_string(result.tbhits)
        + " pv";
    for (Move m : ext.pv)
        info += ' ' + move_to_uci(m);
    uci_write_line(info);

    if (ext.pv.size() > 1)
        result.pondermove = ext.pv[1];
    result.pv = std::move(ext.pv);
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

void Engine::start_search(uint64_t command_epoch,
                          std::chrono::steady_clock::time_point recv_time) {
    configure_syzygy();

    SearchLimits limits = build_limits();
    limits.go_recv_time = recv_time;
    const int desired_threads = parameters_.threads;
    Board board_copy = parameters_.board;
    const bool stop_was_requested = stop_requested_.load(std::memory_order_acquire);

    if (Syzygy::enabled()) {
        limits.syzygy_root_moves = Syzygy::probe_root_moves(board_copy,
                                                            parameters_.syzygy_50_move_rule,
                                                            parameters_.syzygy_probe_limit,
                                                            true);
        if (!limits.root_moves.empty()) {
            limits.syzygy_root_moves.erase(
                std::remove_if(limits.syzygy_root_moves.begin(),
                               limits.syzygy_root_moves.end(),
                               [&](const Syzygy::RootMoveInfo& move) {
                                   return !move_allowed_by_root_list(move.bestmove,
                                                                     limits.root_moves);
                               }),
                limits.syzygy_root_moves.end());
        }
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
        // No tablebase line is built here. Everything in start_search runs
        // after `go` (or an early `ponderhit`) and is charged to the clock,
        // and extending a line for every best-rank move cost 95-285 ms per
        // move in 5-6-man positions (two full root probes per ply): 1.10.0
        // and 1.10.1-rc lost on time with 27-630 ms left. The info PV and the
        // ponder move come from the search.
    }

    if (command_epoch != 0
        && control_epoch_.load(std::memory_order_acquire) != command_epoch) {
        send_bestmove(SearchResult{}, board_copy);
        return;
    }

    // The hash is sized and cleared when `setoption` / `ucinewgame` is
    // processed (apply_table_state), before the GUI's `readyok`, never here.
    const int active_threads = search_pool_.resize_threads(desired_threads);
    if (active_threads != desired_threads)
        parameters_.threads = active_threads;

    if (stop_was_requested && (limits.infinite || limits.ponder)) {
        searching_.store(false, std::memory_order_release);
        send_bestmove(SearchResult{}, board_copy);
        return;
    }

    // ponderhit_requested_ is deliberately NOT cleared here: UciProtocol::cmdGo
    // resets it in protocol order, and a ponderhit may already have arrived
    // during the setup above (see cmdGo).
    //
    // A stale stop from the previous search is cleared, but a stop (or any
    // other control command) received since this `go` must survive: it bumps
    // the epoch before raising the flag, so re-checking the epoch after the
    // clear restores it. seq_cst pairs with the UCI-thread side.
    stop_requested_.store(false, std::memory_order_seq_cst);
    if (command_epoch != 0
        && control_epoch_.load(std::memory_order_seq_cst) != command_epoch)
        stop_requested_.store(true, std::memory_order_seq_cst);

    searching_.store(true, std::memory_order_release);

    SearchResult result = search_pool_.search(board_copy, limits, active_threads);
    wait_until_bestmove_allowed(limits, command_epoch);
    publish_tablebase_pv(result, board_copy, limits);

    if (command_epoch == 0
        || control_epoch_.load(std::memory_order_acquire) == command_epoch)
        searching_.store(false, std::memory_order_release);
    send_bestmove(result, board_copy);
    stop_requested_.store(false, std::memory_order_release);
}

void Engine::run_wac_command(const EngineCommand& command) {
    WacRequest request;
    std::string error;
    if (!parse_wac_request(command.args, request, error)) {
        uci_write_line("info string " + error);
        return;
    }

    if (command.epoch != 0
        && control_epoch_.load(std::memory_order_acquire) != command.epoch)
        return;

    stop_requested_.store(false, std::memory_order_release);
    searching_.store(true, std::memory_order_release);
    run_wac(request, parameters_.search_params);
    if (command.epoch == 0
        || control_epoch_.load(std::memory_order_acquire) == command.epoch)
        searching_.store(false, std::memory_order_release);
}

void Engine::run_bench_command(const EngineCommand& command) {
    // bench [depth] [repeats] [threads] — single-threaded BY DEFAULT so the
    // node total is a deterministic fingerprint (matches sibling engine
    // Rarog's bench command).
    // 9.3: the third argument is now actually parsed. run_bench() has always
    // taken and documented a thread count, but this parser dropped it and
    // hardcoded 1, so the documented `bench [depth] [repeats] [threads]` form
    // was unreachable and there was NO way to measure multi-thread NPS from
    // the engine at all — which is exactly what this phase's 4T gate needs.
    // The default is unchanged (1), so the bench fingerprint is untouched;
    // above 1 the node total is deliberately non-deterministic (Lazy SMP
    // helpers add work) and is a speed reading, never a fingerprint.
    // NOTE: this is deliberately NOT the `Threads` UCI option. Bench must stay
    // reproducible regardless of how the GUI or a harness left that option.
    int depth = 13;
    int repeats = 1;
    int threads = 1;
    {
        std::istringstream iss(command.args);
        int value;
        if (iss >> value) depth   = value;
        if (iss >> value) repeats = value;
        if (iss >> value) threads = value;
    }

    if (command.epoch != 0
        && control_epoch_.load(std::memory_order_acquire) != command.epoch)
        return;

    stop_requested_.store(false, std::memory_order_release);
    searching_.store(true, std::memory_order_release);
    run_bench(depth, repeats, threads);
    if (command.epoch == 0
        || control_epoch_.load(std::memory_order_acquire) == command.epoch)
        searching_.store(false, std::memory_order_release);
}

void Engine::run_perft_command(uint64_t command_epoch) {
    const int depth = parameters_.perft;
    Board board_copy = parameters_.board;

    if (command_epoch != 0
        && control_epoch_.load(std::memory_order_acquire) != command_epoch)
        return;

    stop_requested_.store(false, std::memory_order_release);
    searching_.store(true, std::memory_order_release);
    const uint64_t nodes = perft(board_copy, depth);
    uci_write_line("Nodes searched: " + std::to_string(nodes));
    if (command_epoch == 0
        || control_epoch_.load(std::memory_order_acquire) == command_epoch)
        searching_.store(false, std::memory_order_release);
    stop_requested_.store(false, std::memory_order_release);
}

// Put the hash table and search state in the form the next search needs NOW,
// while no clock is running: a GUI sends `isready` after `setoption` and
// `ucinewgame`, and readyok waits for this. Done inside start_search, the
// resize and clear ran after `go` on the engine's clock -- 138 ms at 256 MB,
// ~550 ms at 1 GB on the first move of every game.
void Engine::apply_table_state() {
    if (parameters_.hash_mb != current_hash_mb_) {
        tt_.resize(static_cast<size_t>(parameters_.hash_mb));
        current_hash_mb_ = parameters_.hash_mb;
    }
    if (parameters_.new_game || parameters_.clear_hash) {
        tt_.clear();
        search_pool_.clear();
        parameters_.new_game = false;
        parameters_.clear_hash = false;
    }
}

void Engine::handle_command(const EngineCommand& command, bool& quit) {
    switch (command.type) {
        case EngineCommandType::SetOption:
        {
            const int old_threads = parameters_.threads;
            parameters_.set_option(command.args);
            configure_syzygy();
            if (parameters_.threads != old_threads) {
                const int active_threads = search_pool_.resize_threads(parameters_.threads);
                parameters_.threads = active_threads;
                uci_write_line("info string Using "
                               + std::to_string(active_threads)
                               + (active_threads == 1 ? " thread" : " threads"));
            }
            apply_table_state();
            break;
        }
        case EngineCommandType::Position:
            // A rejected position is fatal (decided 2026-09-27, replacing the
            // 8.6.3a reject-and-retain contract). Retaining the old board let
            // the next `go` answer with a move for the PREVIOUS position --
            // same side to move, so often legal and silently played wrong.
            // Stockfish and Rarog exit here; so does Basilisk.
            if (!parameters_.set_position(command.args)) {
                const std::string message = "info string CRITICAL ERROR: command `position "
                                          + command.args + "` was rejected; exiting.";
                uci_write_line(message);
                if (fatal_handler_) {
                    fatal_handler_(message);
                } else {
                    std::cout.flush();
                    // _Exit, not exit: the UCI thread is still blocked reading
                    // stdin, and exit() would run static destructors under it.
                    std::_Exit(1);
                }
                quit = true;
            }
            break;
        case EngineCommandType::NewGame:
            parameters_.reset();
            apply_table_state();
            break;
        case EngineCommandType::Go:
            parameters_.set_search_parameters(command.args);
            if (parameters_.perft > 0)
                run_perft_command(command.epoch);
            else
                start_search(command.epoch, command.recv_time);
            break;
        case EngineCommandType::Stop:
            if (command.epoch == 0
                || control_epoch_.load(std::memory_order_acquire) == command.epoch)
                searching_.store(false, std::memory_order_release);
            stop_requested_.store(false, std::memory_order_release);
            break;
        case EngineCommandType::PonderHit:
            // The flag itself is owned by the UCI thread (set on receipt,
            // reset by the next `go`). This queued copy runs only after the
            // search it answered has ended, so touching the flag here could
            // only erase or resurrect it for a later search.
            parameters_.ponder = false;
            break;
        case EngineCommandType::Bench:
            run_bench_command(command);
            break;
        case EngineCommandType::Wac:
            run_wac_command(command);
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
