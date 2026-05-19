#ifndef BASILISK_BOARD_H
#define BASILISK_BOARD_H

#include "chess-library/chess.hpp"

// Re-export commonly used chess-library types so the rest of the codebase
// can include just this header.
using Board = chess::Board;
using Move  = chess::Move;
using Color = chess::Color;

#endif // BASILISK_BOARD_H
