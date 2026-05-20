#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "Board.h"
#include "Constants.h"
#include "Parameters.h"

Parameters::Parameters() {
    board.set_fen(std::string(startPosition));

    moveTime       = 0;
    whiteTime      = 0;
    whiteIncrement = 0;
    blackTime      = 0;
    blackIncrement = 0;
    depth          = infiniteDepth;
    movestogo      = 0;
    nodes          = 0;
    ponder         = false;

    moveOverhead = defaultMoveOverhead;
    hash_mb      = 64;
}

void Parameters::reset() {
    board.set_fen(std::string(startPosition));
    new_game = true;
    resetTemporaryParameters();
}

void Parameters::resetTemporaryParameters() {
    moveTime       = 0;
    whiteTime      = 0;
    whiteIncrement = 0;
    blackTime      = 0;
    blackIncrement = 0;
    depth          = infiniteDepth;
    movestogo      = 0;
    nodes          = 0;
    ponder         = false;
}

std::vector<std::string> Parameters::searchParameters() {
    return {"depth", "movetime", "wtime", "btime", "winc", "binc", "movestogo", "nodes"};
}

std::string Parameters::uciOptions() {
    return "option name Hash type spin default 64 min 1 max 4096\n"
           "option name Clear Hash type button\n"
           "option name Move Overhead type spin default 10 min 0 max 5000\n";
}

void Parameters::setSearchParameters(const std::string& args) {
    resetTemporaryParameters();

    if (args.empty()) {
        moveTime = defaultMoveTime;
        return;
    }

    if (args.find("infinite") != std::string::npos) {
        depth = infiniteDepth;
    }

    ponder = (args.find("ponder") != std::string::npos);

    std::smatch matches;
    const auto params = Parameters::searchParameters();
    for (const std::string& parameter : params) {
        for (const std::regex& re : {std::regex(parameter + " (\\S+) "),
                                     std::regex(parameter + " (\\S+)$")}) {
            if (std::regex_search(args, matches, re)) {
                setSearchParameter(parameter, matches[1].str());
                break;
            }
        }
    }
}

void Parameters::setSearchParameter(const std::string& parameter, const std::string& value) {
    if (parameter == "depth")     { depth          = std::stoi(value); return; }
    if (parameter == "movetime")  { moveTime        = std::stoi(value); return; }
    if (parameter == "wtime")     { whiteTime       = std::stoi(value); return; }
    if (parameter == "winc")      { whiteIncrement  = std::stoi(value); return; }
    if (parameter == "btime")     { blackTime       = std::stoi(value); return; }
    if (parameter == "binc")      { blackIncrement  = std::stoi(value); return; }
    if (parameter == "movestogo") { movestogo       = std::stoi(value); return; }
    if (parameter == "nodes")     { nodes           = std::stoll(value); return; }
}

void Parameters::setOption(const std::string& args) {
    std::smatch matches;

    // Extract option name (everything between "name " and either " value" or end-of-string)
    std::string name;
    if (std::regex_search(args, matches, std::regex(R"(name (.*?) value)")))
        name = matches[1].str();
    else if (std::regex_search(args, matches, std::regex(R"(name (.*))")))
        name = matches[1].str();
    else {
        std::cout << "info string Incorrect setoption format.\n";
        return;
    }

    // Trim trailing whitespace from name
    while (!name.empty() && name.back() == ' ') name.pop_back();

    std::string name_lower = name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Button-type options (no value)
    if (name_lower == "clear hash") {
        clear_hash = true;
        return;
    }

    // Extract value
    if (!std::regex_search(args, matches, std::regex(R"(value (.*))"))) {
        std::cout << "info string Option '" << name << "' requires a value.\n";
        return;
    }
    const std::string value = matches[1].str();

    if (name_lower == "move overhead") {
        moveOverhead = std::stoi(value);
    } else if (name_lower == "hash") {
        hash_mb = std::clamp(std::stoi(value), 1, 4096);
    }
}

void Parameters::setPosition(const std::string& args) {
    Board new_board;

    // Check for "fen ..." or default to startpos
    std::smatch matches;
    for (const std::regex& re : {std::regex(R"(fen (.*?) moves)"),
                                  std::regex(R"(fen (.*))")}) {
        if (std::regex_search(args, matches, re)) {
            new_board.set_fen(matches[1].str());
            goto apply_moves;
        }
    }
    // startpos — already set to starting position in constructor

apply_moves:
    if (std::regex_search(args, matches, std::regex(R"(moves (.*))"))) {
        const std::string movesStr = matches[1].str();
        std::istringstream iss(movesStr);
        std::string token;
        while (iss >> token) {
            // Find matching legal move
            std::vector<Move> pseudo;
            new_board.gen_pseudo_legal(pseudo);
            bool found = false;
            for (Move m : pseudo) {
                if (!new_board.is_legal(m)) continue;
                if (move_to_uci(m) == token) {
                    new_board.make_move(m);
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cout << "info string Illegal or unknown move: " << token << "\n";
                break;
            }
        }
    }

    board = std::move(new_board);
}

