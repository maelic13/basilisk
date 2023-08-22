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

std::string Parameters::uciOptions() {
    return "option name Move Overhead type spin default 10 min 0 max 5000\n";
}

void Parameters::setSearchParameters(const std::string &args) {

}

void Parameters::setOption(const std::string &args) {

}

void Parameters::setPosition(const std::string &args) {

}
