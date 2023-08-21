#include <iostream>
#include <thread>

#include "Constants.h"
#include "UciProtocol.h"

UciProtocol::UciProtocol(
        std::atomic_bool &go, std::atomic_bool &quit,
        Parameters &parameters, std::mutex &m, std::condition_variable &cv)
        : go(go), quit(quit), parameters(parameters), mutex(m), conditionVariable(cv) {}

void UciProtocol::UciLoop() {
    std::string args, command, input;
    while (true) {
        getline(std::cin, input);
        auto pos = input.find(' ');
        command = input.substr(0, pos);
        if (pos != std::string::npos) args = input.substr(pos + 1);
        else args = "";

        if (command == "uci") uci();
        if (command == "isready") isReady();
        if (command == "go") uciGo(args);
        if (command == "stop") uciStop();
        if (command == "setoption") uciSetOption(args);
        if (command == "ucinewgame") uciNewGame();
        if (command == "position") uciPosition(args);
        if (command == "quit") {
            uciQuit();
            break;
        }
    }
}

void UciProtocol::uci() {
    std::cout << "id name " << engineName << " " << engineVersion << std::endl;
    std::cout << "id author " << engineAuthor << std::endl;
    std::cout << Parameters::uciOptions();
    std::cout << "uciok" << std::endl;
}

void UciProtocol::isReady() {
    std::cout << "readyok" << std::endl;
}

void UciProtocol::uciQuit() {
    quit = true;
    conditionVariable.notify_one();
}

void UciProtocol::uciGo(const std::string &args) {
    parameters.setSearchParameters(args);
    go = true;
    conditionVariable.notify_one();
}

void UciProtocol::uciStop() {
    go = false;
    conditionVariable.notify_one();
}

void UciProtocol::uciSetOption(const std::string &args) {
    parameters.setOption(args);
}

void UciProtocol::uciPosition(const std::string &args) {
    parameters.setPosition(args);
}

void UciProtocol::uciNewGame() {
    parameters.reset();
}
