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
    std::smatch matches;
    if (std::regex_search(args, matches, std::regex("name (.*) value"))) {
        std::string moves = matches[1].str();
    }
}

void Parameters::setPosition(const std::string &args) {
    Board new_board = Board();

    std::regex fen_regexes[] = {
            std::regex("fen (.*) moves"),
            std::regex("fen (.*)")
    };
    for (const std::regex& regex : fen_regexes) {
        std::smatch matches;
        if (std::regex_search(args, matches, regex)) {
            new_board = Board(matches[1].str());
            break;
        }
    }

    std::smatch matches;
    if (std::regex_search(args, matches, std::regex("moves (.*)"))) {
        std::string moves = matches[1].str();

        if (moves.back() != ' ') {
            moves.push_back(' ');
        }

        std::string move;
        for (char character : moves) {
            if (character != ' ') {
                move.push_back(character);
                continue;
            }
            board.make_move(move);
            move = "";
        }
    }

    board = new_board;
}

std::string Parameters::uciOptions() {
    return "option name Move Overhead type spin default 10 min 0 max 5000\n";
}
