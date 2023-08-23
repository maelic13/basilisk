#include <sstream>
#include <regex>
#include <iostream>
#include <list>

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

void Parameters::setSearchParameters(const std::string &args) {

}

void Parameters::setOption(const std::string &args) {

}

void Parameters::setPosition(const std::string &args) {
    Board new_board = Board();

    std::string fen_keyword = "fen";
    std::string move_keyword = "move";
    size_t fen_index = args.find(fen_keyword);
    size_t move_index = args.find(move_keyword, fen_index + fen_keyword.length());

    std::regex fen_regexes[] = {
            std::regex(fen_keyword + " (.*) " + move_keyword),
            std::regex(fen_keyword + " (.*)")
    };
    for (const std::regex& regex : fen_regexes) {
        std::smatch matches;
        if (std::regex_search(args, matches, regex)) {
            new_board = Board(matches[1].str());
            break;
        }
    }
    std::regex move_regex = std::regex(move_keyword + " (.*)");
    std::smatch matches;

    if (std::regex_search(args, matches, move_regex)) {
        std::string moves = matches[1].str();
        // TODO: play moves on new_board
    }

    board = new_board;
}

std::string Parameters::uciOptions() {
    return "option name Move Overhead type spin default 10 min 0 max 5000\n";
}
