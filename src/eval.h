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

// Draw-scale factor for opposite-coloured-bishop positions (8.3). Eases
// toward normal as pawns are added but is capped at the neutral 48/48: a
// draw scaler must never amplify the evaluation. (Pre-8.3 the uncapped
// 32 + 4*pawns reached 96/48 = 2x at 16 pawns; the Texel fit ratified the
// bug, so the dependent weights are re-arbitrated by SPRT run #3.)
// Shared by the evaluator and the tuner's linear_delta_scale so the
// --verify trace reconstruction stays exact.
inline int ocb_draw_scale(int total_pawns) {
    const int scale = 32 + total_pawns * 4;
    return scale < 48 ? scale : 48;
}

// Global evaluation parameters. Change fields then call init_eval_tables() to apply.
extern EvalParams g_eval_params;

void init_eval_tables(const EvalParams& p = g_eval_params);

#ifdef BASILISK_TUNE
// Load BASILISK_EVAL_FILE env-var path (if set) into g_eval_params and rebuild tables.
void load_eval_file_if_set();
// Dump g_eval_params to stdout in "name index value" format (one line per element).
void run_dumpeval();
#endif
