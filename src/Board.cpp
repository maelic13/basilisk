#include "Board.h"

#include "Constants.h"

Board::Board() : Board(startPosition) {}

Board::Board(std::string fen) : fen(std::move(fen)) {}
