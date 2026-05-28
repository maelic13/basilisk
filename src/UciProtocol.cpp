#include <future>
#include <iostream>
#include <string>

#include "Constants.h"
#include "UciOutput.h"
#include "UciProtocol.h"

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

void UciProtocol::UciLoop() {
    std::string input;
    bool sent_quit = false;

    while (std::getline(std::cin, input)) {
        if (input.empty()) continue;
        if (!input.empty() && input.back() == '\r') input.pop_back();
        if (input.empty()) continue;

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
        else if (command == "quit") {
            cmdQuit();
            sent_quit = true;
            break;
        }
    }

    if (!sent_quit)
        cmdQuit();
}

uint64_t UciProtocol::next_control_epoch() {
    return control_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
}

void UciProtocol::enqueue(EngineCommandType type, const std::string& args, uint64_t epoch) {
    commands_.push(EngineCommand{type, args, nullptr, epoch});
}

// ---------------------------------------------------------------------------
// Static handlers
// ---------------------------------------------------------------------------

void UciProtocol::cmdUci() {
    std::string out = "id name " + std::string(engineName) + " " + std::string(engineVersion) + "\n"
                    + "id author " + std::string(engineAuthor) + "\n"
                    + Parameters::uciOptions()
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
    uint64_t epoch = next_control_epoch();
    if (searching_.exchange(true, std::memory_order_acq_rel))
        stop_requested_.store(true, std::memory_order_release);
    enqueue(EngineCommandType::Go, args, epoch);
}

void UciProtocol::cmdStop() {
    uint64_t epoch = next_control_epoch();
    stop_requested_.store(true, std::memory_order_release);
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
