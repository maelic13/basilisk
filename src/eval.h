#pragma once

#include "Board.h"

struct PawnEntry {
    Key      key;
    int      mg, eg;
    Bitboard passed[NCOLORS];
    Bitboard attacks[NCOLORS];
};

static constexpr int PAWN_TABLE_SIZE = 16384;

class Evaluator {
public:
    Evaluator();
    int evaluate(const Board& b);
    void clear_pawn_table();

private:
    PawnEntry pawn_table_[PAWN_TABLE_SIZE];

    void eval_pawns(const Board& b,
                    int& mg, int& eg,
                    Bitboard passed[NCOLORS],
                    Bitboard attacks[NCOLORS]);
};

void init_eval_tables();
