#include <iostream>
#include <string>

#include "Constants.h"
#include "UciProtocol.h"
#include "bench.h"

UciProtocol::UciProtocol(
    std::atomic_bool &go_, std::atomic_bool &quit_,
    Parameters &params_, std::condition_variable &cv)
    : go(go_), quit(quit_), parameters(params_), conditionVariable(cv) {}

void UciProtocol::UciLoop() {
    std::string input;

    while (std::getline(std::cin, input)) {
        if (input.empty()) continue;
        // Strip trailing \r (Windows line endings)
        if (!input.empty() && input.back() == '\r') input.pop_back();
        if (input.empty()) continue;

        const auto sep = input.find(' ');
        const std::string command = input.substr(0, sep);
        const std::string args    = (sep != std::string::npos) ? input.substr(sep + 1) : "";

        if (debug_mode)
            std::cout << "info string received: " << input << "\n";

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
        else if (command == "bench")     cmdBench(args);
        else if (command == "quit") {
            cmdQuit();
            break;
        }
        // Unknown commands are silently ignored per UCI spec
    }
}

// ---------------------------------------------------------------------------
// Static handlers
// ---------------------------------------------------------------------------

void UciProtocol::cmdUci() {
    std::cout << "id name "   << engineName    << " " << engineVersion << "\n"
              << "id author " << engineAuthor  << "\n"
              << Parameters::uciOptions()
              << "uciok\n";
    std::cout.flush();
}

void UciProtocol::cmdIsReady() {
    std::cout << "readyok\n";
    std::cout.flush();
}

void UciProtocol::cmdRegister() {
    // Registration not required
    std::cout << "registration ok\n";
    std::cout.flush();
}

// ---------------------------------------------------------------------------
// Instance handlers
// ---------------------------------------------------------------------------

void UciProtocol::cmdDebug(const std::string &args) {
    debug_mode = (args == "on");
}

void UciProtocol::cmdQuit() {
    go   = false;  // stop any ongoing search
    quit = true;
    conditionVariable.notify_one();
}

void UciProtocol::cmdGo(const std::string &args) {
    parameters.setSearchParameters(args);
    go = true;
    conditionVariable.notify_one();
}

void UciProtocol::cmdStop() {
    go = false;
    conditionVariable.notify_one();
}

void UciProtocol::cmdPonderHit() {
    // The expected move was played; switch from pondering to normal search.
    // Stop the current ponder search and restart immediately without ponder mode
    // so that time management activates using the clock values from the go command.
    parameters.ponder = false;
    go = false;
    conditionVariable.notify_one();
    // Small pause so engine thread wakes and exits, then restart
    go = true;
    conditionVariable.notify_one();
}

void UciProtocol::cmdSetOption(const std::string &args) {
    parameters.setOption(args);
}

void UciProtocol::cmdPosition(const std::string &args) {
    parameters.setPosition(args);
}

void UciProtocol::cmdNewGame() {
    parameters.reset();
}

void UciProtocol::cmdBench(const std::string& args) {
    int depth = 13;
    if (!args.empty()) {
        try { depth = std::stoi(args); } catch (...) {}
    }
    // Stop any running search first
    go   = false;
    conditionVariable.notify_one();
    run_bench(depth);
}

