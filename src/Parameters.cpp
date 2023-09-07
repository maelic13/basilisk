#include <iostream>
#include <regex>

#include "Board.h"
#include "Constants.h"
#include "Parameters.h"

Parameters::Parameters() {
    board = Board();

    moveTime = 0;                           // [ms]
    whiteTime = 0;                          // [ms]
    whiteIncrement = 0;                     // [ms]
    blackTime = 0;                          // [ms]
    blackIncrement = 0;                     // [ms]
    depth = infiniteDepth;

    moveOverhead = defaultMoveOverhead;     // [ms]
}

void Parameters::reset() {
    board = Board();
    resetTemporaryParameters();
}

void Parameters::resetTemporaryParameters() {
    moveTime = 0;
    whiteTime = 0;
    whiteIncrement = 0;
    blackTime = 0;
    blackIncrement = 0;
    depth = infiniteDepth;
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
        depth = defaultDepth;
    }

    if (args.contains("infinite")) {
        depth = infiniteDepth;
    }

    std::smatch matches;
    std::vector<std::string> searchParameters = Parameters::searchParameters();
    for (std::string &parameter: searchParameters) {
        for (const std::regex &regex: {std::regex(parameter + " (.*) "), std::regex(parameter + " (.*)")}) {
            if (std::regex_search(args, matches, regex)) {
                setSearchParameter(parameter, matches[1].str());
                break;
            }
        }
    }
}

void Parameters::setSearchParameter(const std::string &parameter, const std::string &value) {
    if (parameter == "depth") {
        depth = std::stoi(value);
    }

    if (parameter == "movetime") {
        moveTime = std::stoi(value);
    }

    if (parameter == "wtime") {
        whiteTime = std::stoi(value);
    }

    if (parameter == "winc") {
        whiteIncrement = std::stoi(value);
    }

    if (parameter == "btime") {
        blackTime = std::stoi(value);
    }

    if (parameter == "binc") {
        blackIncrement = std::stoi(value);
    }
}

void Parameters::setOption(const std::string &args) {
    std::smatch matches;
    if (!std::regex_search(args, matches, std::regex("name (.*) value"))) {
        std::cout << "Incorrect option.\n";
        return;
    }
    std::string name = matches[1].str();
    std::transform(name.begin(), name.end(), name.begin(), tolower);

    if (!std::regex_search(args, matches, std::regex("value (.*)"))) {
        std::cout << "Incorrect option.\n";
        return;
    }
    std::string value = matches[1].str();
    std::transform(value.begin(), value.end(), value.begin(), tolower);

    if (name == "move overhead") {
        moveOverhead = std::stoi(value);
    }
}

void Parameters::setPosition(const std::string &args) {
    Board new_board = Board();

    std::smatch matches;
    for (const std::regex &regex: {std::regex("fen (.*) moves"), std::regex("fen (.*)")}) {
        if (std::regex_search(args, matches, regex)) {
            new_board = Board(matches[1].str());
            break;
        }
    }

    if (std::regex_search(args, matches, std::regex("moves (.*)"))) {
        std::string moves = matches[1].str();

        if (moves.back() != ' ') {
            moves.push_back(' ');
        }

        std::string move;
        for (char character: moves) {
            if (character != ' ') {
                move.push_back(character);
                continue;
            }
            board.makeMove(move);
            move = "";
        }
    }

    board = new_board;
}
