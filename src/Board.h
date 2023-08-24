#ifndef BASILISK_BOARD_H
#define BASILISK_BOARD_H

#include <string>

class Board {
public:
    Board();

    explicit Board(std::string fen);

    bool make_move(const std::string& move);

private:
    std::string fen;
};


#endif //BASILISK_BOARD_H
