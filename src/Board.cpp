#include <iostream>

#include "Board.h"
#include "Constants.h"

Board::Board() : Board(startPosition) {}

Board::Board(std::string fen) : fen(std::move(fen)) {}

bool Board::make_move(const std::string& move) {
    std::cout << "Played move: " << move << "\n";
    return false;
}
