#pragma once

#include <string>

#include "board.h"
#include "eval_params.h"

struct PawnEntry {
    Key      key;
    int      mg, eg;
    Bitboard passed[NCOLORS];
    Bitboard attacks[NCOLORS];
};

static constexpr int PAWN_TABLE_SIZE = 16384;

// 8.6.2a: the pawn cache is indexed `pkey & (PAWN_TABLE_SIZE - 1)`, which is a
// modulo only for powers of two — a non-pow2 size would silently alias entries
// (returning another position's pawn eval) rather than fail.
static_assert((PAWN_TABLE_SIZE & (PAWN_TABLE_SIZE - 1)) == 0,
              "PAWN_TABLE_SIZE must be a power of two: indexed with & (SIZE - 1)");

class Evaluator {
public:
    Evaluator();
    int evaluate(const Board& b);
    void clear_pawn_table();

    // ---- Lazy dual-eval audit (8.6.6b, Rarog 9.6b pattern) -----------------
    // When diag_lazy is set, a lazy-margin skip ALSO runs the full positional
    // tail on the side and records the divergence; the SERVED score is the
    // lazy value either way, so behaviour (and bench) is identical with the
    // audit on. Sign flips are the failure the margin promises cannot happen;
    // crossings are full scores the margin wrongly called decided. Matters
    // beyond curiosity: Phase-9 datagen labels come from this eval.
    bool    diag_lazy = false;
    int64_t lazy_fires = 0, lazy_sign_flips = 0, lazy_margin_crossings = 0;
    int64_t lazy_absdelta_sum = 0, lazy_absdelta_max = 0;

    // ---- 8.7.1(c) speed telemetry (always counted, NIL-cost class) --------
    // eval_calls / nodes = the eval RATE: evaluate() is gated behind the TT
    // static_eval cache and skipped in check, so the share of nodes actually
    // paying for a full eval is a guess today. pawn_probes/hits size the
    // 16384-entry direct-mapped pawn cache — the input 8.7.8 needs before
    // resizing it, and the counter Rarog's decisive cache-miss numbers came
    // from. Reset per `go` alongside the lazy-audit block.
    int64_t eval_calls = 0;
    int64_t pawn_probes = 0, pawn_hits = 0;

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

// ---- Draw-scaling predicates shared with the Texel tuner --------------------
//
// 8.6.2b: the tuner's linear_delta_scale() has to reproduce the evaluator's
// draw scaling, or the linearisation it fits is not the function the engine
// actually computes. Previously only ocb_draw_scale() was shared and the
// *predicates* were re-typed by hand on both sides — including two
// complementary dark-square masks (0xAA55.. here, 0x55AA.. there) that agreed
// only because the test is an inequality, and a hand-copied rule-50 curve.
// A silent divergence there does NOT trip --verify (it corrupts the fitted
// gradients instead, which is worse because nothing fails loudly), so every
// formula both sides need now lives here and has exactly one definition.

inline constexpr Bitboard EVAL_DARK_SQUARES = 0xAA55AA55AA55AA55ULL;

// Rule-50 damping: the evaluation decays toward zero as the halfmove clock
// grows, retaining ~50% at clock 99 (SF-era curve; the pre-8.4 (100-clock)/100
// line kept only 1%, which told the search almost every long-clock position was
// already drawn).
inline constexpr int DAMP_RULE50_DEN = 199;
[[nodiscard]] inline int damp_rule50_scale_num(int halfmove_clock) {
    return DAMP_RULE50_DEN - halfmove_clock;
}

[[nodiscard]] inline int damp_rule50(int score, int halfmove_clock) {
    return score * damp_rule50_scale_num(halfmove_clock) / DAMP_RULE50_DEN;
}

// Exactly one bishop each, on opposite colours. Says nothing about the rest of
// the material — the scaler deliberately fires with other pieces present, which
// is the scope refinement deferred to PLAN 13.8.
[[nodiscard]] inline bool is_opposite_coloured_bishops(const Board& b) {
    const bool wb1 = !more_than_one(b.pieces[WHITE][BISHOP]) && b.pieces[WHITE][BISHOP];
    const bool bb1 = !more_than_one(b.pieces[BLACK][BISHOP]) && b.pieces[BLACK][BISHOP];
    if (!wb1 || !bb1)
        return false;
    const bool wb_dark = (b.pieces[WHITE][BISHOP] & EVAL_DARK_SQUARES) != 0;
    const bool bb_dark = (b.pieces[BLACK][BISHOP] & EVAL_DARK_SQUARES) != 0;
    return wb_dark != bb_dark;
}

[[nodiscard]] inline bool is_lone_king(const Board& b, Color c) {
    return b.occupancy[c] == sq_bb(b.king_sq[c]);
}

// King and exactly n knights, nothing else.
[[nodiscard]] inline bool is_king_and_n_knights(const Board& b, Color c, int n) {
    return !b.pieces[c][PAWN] && !b.pieces[c][BISHOP] && !b.pieces[c][ROOK]
        && !b.pieces[c][QUEEN] && popcount(b.pieces[c][KNIGHT]) == n;
}

// KNNK is a dead draw: two knights cannot force mate against a bare king.
[[nodiscard]] inline bool is_knnk_draw(const Board& b) {
    return (is_lone_king(b, WHITE) && is_king_and_n_knights(b, BLACK, 2))
        || (is_lone_king(b, BLACK) && is_king_and_n_knights(b, WHITE, 2));
}

// Global evaluation parameters. Change fields then call init_eval_tables() to apply.
extern EvalParams g_eval_params;

void init_eval_tables(const EvalParams& p = g_eval_params);

#ifdef BASILISK_TUNE
// Load BASILISK_EVAL_FILE env-var path (if set) into g_eval_params and rebuild tables.
void load_eval_file_if_set();
// Dump g_eval_params to stdout in "name index value" format (one line per element).
void run_dumpeval();

// Tune-build-only atomic control for base, diagonal, edge, strong-king and
// knight KBNK terms. Four-field values remain accepted as the legacy
// diagonal,edge,king,knight form for reproducibility of the 6.1.c first pass.
bool set_kbnk_drive_weights(const std::string& value, std::string& error);
#endif
