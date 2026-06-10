#pragma once

#include "Board.h"
#include "EvalParams.h"

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

// Global evaluation parameters. Change fields then call init_eval_tables() to apply.
extern EvalParams g_eval_params;

void init_eval_tables(const EvalParams& p = g_eval_params);

#ifdef BASILISK_TUNE
// Load BASILISK_EVAL_FILE env-var path (if set) into g_eval_params and rebuild tables.
void load_eval_file_if_set();
// Dump g_eval_params to stdout in "name index value" format (one line per element).
void run_dumpeval();
#endif
