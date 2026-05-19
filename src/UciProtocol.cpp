#include <iostream>
#include <string>

#include "Constants.h"
#include "UciProtocol.h"

UciProtocol::UciProtocol(
    std::atomic_bool &go, std::atomic_bool &quit,
    Parameters &parameters, std::condition_variable &cv)
    : go(go), quit(quit), parameters(parameters), conditionVariable(cv) {}

void UciProtocol::UciLoop() {
    std::string input;

    while (std::getline(std::cin, input)) {
        if (input.empty()) continue;

        const auto sep = input.find(' ');
        const std::string command = input.substr(0, sep);
        const std::string args    = (sep != std::string::npos) ? input.substr(sep + 1) : "";

        if      (command == "uci")        cmdUci();
        else if (command == "isready")    cmdIsReady();
        else if (command == "register")   cmdRegister();
        else if (command == "setoption")  cmdSetOption(args);
        else if (command == "ucinewgame") cmdNewGame();
        else if (command == "position")   cmdPosition(args);
        else if (command == "go")         cmdGo(args);
        else if (command == "stop")       cmdStop();
        else if (command == "ponderhit")  cmdPonderHit();
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

void UciProtocol::cmdQuit() {
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
    // Pondering is not yet implemented; treat ponderhit as a stop+go with
    // the same position so the engine continues searching normally.
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

