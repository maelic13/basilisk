// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
#pragma once
#ifndef IO_H
#define IO_H

#include "defs.h"
#include <string>

extern void print_bitboard(U64 bitboard);
extern void print_board();
extern void reset_board();
extern void parse_fen(const char* fen);

// Serialize the current global board to a FEN string (used for data logging).
extern std::string board_to_fen();
// Serialize a move to UCI long-algebraic notation, e.g. "e2e4" or "e7e8q".
extern std::string move_to_uci(int move);

#endif
