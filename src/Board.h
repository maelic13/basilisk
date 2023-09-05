#ifndef BASILISK_BOARD_H
#define BASILISK_BOARD_H

#include <string>

#include "Piece.h"

class Board {
public:
    Board();

    explicit Board(std::string fen);

    bool makeMove(const std::string &move);

    bool sideToMove();

private:
    std::string fen;
};

#endif //BASILISK_BOARD_H
