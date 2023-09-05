#include <iostream>

#include "Board.h"
#include "Constants.h"

Board::Board() : Board(startPosition) {}

Board::Board(std::string fen) : fen(std::move(fen)) {}

bool Board::makeMove(const std::string &move) {
    std::cout << "Played move: " << move << "\n";
    return false;
}

bool Board::sideToMove() {
    return true;
}
