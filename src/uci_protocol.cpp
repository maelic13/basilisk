#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <string>

#include "constants.h"
#include "uci_output.h"
#include "uci_protocol.h"
#ifdef BASILISK_TUNE
#include "eval.h"
#endif

UciProtocol::UciProtocol(EngineCommandQueue& commands,
                         std::atomic_bool& stop_requested,
                         std::atomic_bool& ponderhit_requested,
                         std::atomic_bool& searching,
                         std::atomic_uint64_t& control_epoch)
    : commands_(commands)
    , stop_requested_(stop_requested)
    , ponderhit_requested_(ponderhit_requested)
    , searching_(searching)
    , control_epoch_(control_epoch) {}

UciProtocol::Dispatch UciProtocol::dispatch(const std::string& input) {
    const auto sep = input.find(' ');
    const std::string command = input.substr(0, sep);
    const std::string args    = (sep != std::string::npos) ? input.substr(sep + 1) : "";

    if (debug_mode)
        uci_write_line("info string received: " + input);

    if      (command == "uci")        cmdUci();
    else if (command == "debug")      cmdDebug(args);
    else if (command == "isready")    cmdIsReady();
    else if (command == "register")   cmdRegister();
    else if (command == "setoption")  cmdSetOption(args);
    else if (command == "ucinewgame") cmdNewGame();
    else if (command == "position")   cmdPosition(args);
    else if (command == "go")         cmdGo(args);
    else if (command == "stop")       cmdStop();
    else if (command == "ponderhit")  cmdPonderHit();
    else if (command == "bench")      cmdBench(args);
    else if (command == "wac")        cmdWac(args);
    else if (command == "help")       cmdHelp();
#ifdef BASILISK_TUNE
    else if (command == "dumpeval")   run_dumpeval();
#endif
    else if (command == "quit") {
        cmdQuit();
        return Dispatch::Quit;
    }
    else return Dispatch::Unknown;
    return Dispatch::Handled;
}

void UciProtocol::UciLoop() {
    std::string input;
    bool sent_quit = false;

    while (std::getline(std::cin, input)) {
        if (!input.empty() && input.back() == '\r') input.pop_back();
        if (input.empty()) continue;
        // An unknown command is ignored, as the UCI protocol asks of an engine.
        if (dispatch(input) == Dispatch::Quit) {
            sent_quit = true;
            break;
        }
    }

    if (!sent_quit)
        cmdQuit();
}

int UciProtocol::RunArguments(const std::string& command_line) {
    const Dispatch result = dispatch(command_line);
    if (result == Dispatch::Quit)
        return 0;
    if (result == Dispatch::Unknown)
        uci_write_line("Unknown command: '" + command_line
                       + "'. Run `basilisk help` for the commands.");
    // Quit after the command, without raising stop: a `go depth N` or a bench
    // given on the command line runs to its end, as Stockfish runs it.
    commands_.push(EngineCommand{EngineCommandType::Quit, {}, nullptr,
                                 control_epoch_.load(std::memory_order_acquire)});
    return result == Dispatch::Unknown ? 2 : 0;
}

void UciProtocol::cmdHelp() {
    uci_write(std::string(engineName) + " " + std::string(engineVersion)
        + " is a UCI chess engine by " + std::string(engineAuthor) + ".\n"
        "Use it from a chess GUI or harness that speaks UCI, or type its commands:\n"
        "  uci, isready, setoption name <name> value <value>, ucinewgame,\n"
        "  position [startpos | fen <fen>] [moves <move>...],\n"
        "  go [depth | nodes | movetime | wtime btime winc binc movestogo | mate\n"
        "      | infinite | ponder | searchmoves <move>...], stop, ponderhit, quit.\n"
        "Also: bench [depth] [repeats] [threads], wac [depth], help.\n"
        "Arguments run one command and exit: `basilisk bench 13`.\n"
        "Source and releases: https://github.com/maelic13/basilisk\n");
}

uint64_t UciProtocol::next_control_epoch() {
    return control_epoch_.fetch_add(1, std::memory_order_seq_cst) + 1;
}

void UciProtocol::enqueue(EngineCommandType type, const std::string& args, uint64_t epoch,
                          std::chrono::steady_clock::time_point recv_time) {
    commands_.push(EngineCommand{type, args, nullptr, epoch, recv_time});
}

// ---------------------------------------------------------------------------
// Static handlers
// ---------------------------------------------------------------------------

void UciProtocol::cmdUci() {
    // A binary built with a cluster umbrella on says so in its name.
#if defined(BASILISK_B2_CORE)
    constexpr std::string_view arm = "+b2core";
#else
    constexpr std::string_view arm = "";
#endif
    std::string out = "id name " + std::string(engineName) + " " + std::string(engineVersion)
                    + std::string(arm) + "\n"
                    + "id author " + std::string(engineAuthor) + "\n"
                    + Parameters::uci_options()
                    + "uciok\n";
    uci_write(out);
}

void UciProtocol::cmdIsReady() {
    if (!searching_.load(std::memory_order_acquire)) {
        auto ack = std::make_shared<std::promise<void>>();
        auto ready = ack->get_future();
        commands_.push(EngineCommand{EngineCommandType::Ready, {}, ack});
        ready.wait();
    }

    uci_write_line("readyok");
}

void UciProtocol::cmdRegister() {
    uci_write_line("registration ok");
}

// ---------------------------------------------------------------------------
// Instance handlers
// ---------------------------------------------------------------------------

void UciProtocol::cmdDebug(const std::string &args) {
    debug_mode = (args == "on");
}

void UciProtocol::cmdQuit() {
    uint64_t epoch = control_epoch_.load(std::memory_order_acquire);
    stop_requested_.store(true, std::memory_order_release);
    commands_.push(EngineCommand{EngineCommandType::Quit, {}, nullptr, epoch});
}

void UciProtocol::cmdGo(const std::string &args) {
    const auto recv_time = std::chrono::steady_clock::now();
    uint64_t epoch = next_control_epoch();
    if (searching_.exchange(true, std::memory_order_acq_rel))
        stop_requested_.store(true, std::memory_order_seq_cst);
    // A `ponderhit` always follows the `go ponder` it answers, and both arrive
    // on this thread, so this is the one place the flag can be reset without
    // racing a legitimate ponderhit. The engine thread must never clear it: a
    // GUI whose opponent replies instantly sends `ponderhit` while the engine
    // thread is still in start_search's setup, and clearing there lost it —
    // the search then pondered on with no clock and forfeited on time.
    ponderhit_requested_.store(false, std::memory_order_release);
    enqueue(EngineCommandType::Go, args, epoch, recv_time);
}

void UciProtocol::cmdStop() {
    uint64_t epoch = next_control_epoch();
    // seq_cst pairs with start_search's clear-then-recheck of the epoch.
    stop_requested_.store(true, std::memory_order_seq_cst);
    enqueue(EngineCommandType::Stop, {}, epoch);
}

void UciProtocol::cmdPonderHit() {
    ponderhit_requested_.store(true, std::memory_order_release);
    enqueue(EngineCommandType::PonderHit);
}

void UciProtocol::cmdSetOption(const std::string &args) {
    enqueue(EngineCommandType::SetOption, args);
}

void UciProtocol::cmdPosition(const std::string &args) {
    enqueue(EngineCommandType::Position, args);
}

void UciProtocol::cmdNewGame() {
    enqueue(EngineCommandType::NewGame);
}

void UciProtocol::cmdBench(const std::string& args) {
    uint64_t epoch = next_control_epoch();
    if (searching_.exchange(true, std::memory_order_acq_rel))
        stop_requested_.store(true, std::memory_order_release);
    enqueue(EngineCommandType::Bench, args, epoch);
}

void UciProtocol::cmdWac(const std::string& args) {
    uint64_t epoch = next_control_epoch();
    if (searching_.exchange(true, std::memory_order_acq_rel))
        stop_requested_.store(true, std::memory_order_release);
    enqueue(EngineCommandType::Wac, args, epoch);
}
