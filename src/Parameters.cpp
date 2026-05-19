#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "Board.h"
#include "Constants.h"
#include "Parameters.h"

Parameters::Parameters() {
    board = Board(std::string(startPosition));

    moveTime      = 0;
    whiteTime     = 0;
    whiteIncrement = 0;
    blackTime     = 0;
    blackIncrement = 0;
    depth         = infiniteDepth;

    moveOverhead = defaultMoveOverhead;
}

void Parameters::reset() {
    board = Board(std::string(startPosition));
    resetTemporaryParameters();
}

void Parameters::resetTemporaryParameters() {
    moveTime      = 0;
    whiteTime     = 0;
    whiteIncrement = 0;
    blackTime     = 0;
    blackIncrement = 0;
    depth         = infiniteDepth;
}

std::vector<std::string> Parameters::searchParameters() {
    return {"depth", "movetime", "wtime", "btime", "winc", "binc"};
}

std::string Parameters::uciOptions() {
    return "option name Move Overhead type spin default 10 min 0 max 5000\n";
}

void Parameters::setSearchParameters(const std::string &args) {
    resetTemporaryParameters();

    if (args.empty()) {
        moveTime = defaultMoveTime;
        return;
    }

    if (args.find("infinite") != std::string::npos) {
        depth = infiniteDepth;
    }

    std::smatch matches;
    const auto params = Parameters::searchParameters();
    for (const std::string &parameter : params) {
        for (const std::regex &re : {std::regex(parameter + " (\\S+) "),
                                     std::regex(parameter + " (\\S+)$")}) {
            if (std::regex_search(args, matches, re)) {
                setSearchParameter(parameter, matches[1].str());
                break;
            }
        }
    }
}

void Parameters::setSearchParameter(const std::string &parameter, const std::string &value) {
    if (parameter == "depth")    { depth         = std::stoi(value); return; }
    if (parameter == "movetime") { moveTime       = std::stoi(value); return; }
    if (parameter == "wtime")    { whiteTime      = std::stoi(value); return; }
    if (parameter == "winc")     { whiteIncrement = std::stoi(value); return; }
    if (parameter == "btime")    { blackTime      = std::stoi(value); return; }
    if (parameter == "binc")     { blackIncrement = std::stoi(value); return; }
}

void Parameters::setOption(const std::string &args) {
    std::smatch matches;
    if (!std::regex_search(args, matches, std::regex(R"(name (.*?) value)"))) {
        std::cout << "info string Incorrect setoption format.\n";
        return;
    }
    std::string name = matches[1].str();
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (!std::regex_search(args, matches, std::regex(R"(value (.*))"))) {
        std::cout << "info string Incorrect setoption format.\n";
        return;
    }
    std::string value = matches[1].str();

    if (name == "move overhead") {
        moveOverhead = std::stoi(value);
    }
}

void Parameters::setPosition(const std::string &args) {
    Board new_board(std::string(startPosition));

    std::smatch matches;
    // Check for "fen ..." first
    for (const std::regex &re : {std::regex(R"(fen (.*?) moves)"),
                                  std::regex(R"(fen (.*))")}) {
        if (std::regex_search(args, matches, re)) {
            new_board = Board(matches[1].str());
            break;
        }
    }
    // If "startpos" — board is already the start position

    // Apply the move list to new_board (fixing the original bug where
    // moves were applied to the old board member before overwriting it)
    if (std::regex_search(args, matches, std::regex(R"(moves (.*))"))) {
        const std::string movesStr = matches[1].str();
        std::istringstream iss(movesStr);
        std::string token;
        while (iss >> token) {
            try {
                Move m = chess::uci::uciToMove(new_board, token);
                new_board.makeMove(m);
            } catch (const std::exception &) {
                std::cout << "info string Illegal move: " << token << "\n";
                break;
            }
        }
    }

    board = std::move(new_board);
}

