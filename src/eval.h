#pragma once

#include <cstdint>
#include <string>

#include "board.h"
#include "constants.h"
#include "eval_params.h"

struct PawnEntry {
    Key      key;
    int      mg, eg;
    Bitboard passed[NCOLORS];
    Bitboard attacks[NCOLORS];
};

static constexpr int PAWN_TABLE_SIZE = 16384;

// The search treats any score at or above MATE_SCORE - MAX_PLY as a mate and
// ply-adjusts it on its way into the TT, so a STATIC evaluation must stay
// strictly below it.
inline constexpr int KBNK_STATIC_MATE_FLOOR = MATE_SCORE - MAX_PLY;

// The pawn cache is indexed `pkey & (PAWN_TABLE_SIZE - 1)`, which is a
// modulo only for powers of two — a non-pow2 size would silently alias entries
// (returning another position's pawn eval) rather than fail.
static_assert((PAWN_TABLE_SIZE & (PAWN_TABLE_SIZE - 1)) == 0,
              "PAWN_TABLE_SIZE must be a power of two: indexed with & (SIZE - 1)");

#ifdef BASILISK_TUNE
// Exact search-tree occurrence of the 20 pre-NNUE reference endgame families.
// This is tune-build-only diagnostic state: release binaries contain neither
// the counters nor the classifier. `classified` is the denominator (every
// evaluated position with at most seven men), not a sum of the family fields;
// KPK also belongs to KPsK, and KBNK also belongs to KXK.
struct EndgameOccurrenceCounters {
    int64_t krpkr = 0, krpkb = 0, kpsk = 0, kpk = 0, krkp = 0;
    int64_t kbpsk = 0, kpkp = 0, kqkp = 0, kbpkb = 0, kbppkb = 0;
    int64_t krkn = 0, krkb = 0, kbpkn = 0, knnkp = 0, knnk = 0;
    int64_t kqkr = 0, kqkrps = 0, krppkrp = 0, kxk = 0, kbnk = 0;
    int64_t classified = 0;

    void reset() noexcept { *this = EndgameOccurrenceCounters{}; }
};
#endif

class Evaluator {
public:
    Evaluator();
    // The evaluation the search serves: evaluate_undamped() with the rule-50
    // damping at the board's halfmove clock.
    int evaluate(const Board& b);
    // Side-to-move evaluation without the halfmove clock, so it is a function
    // of the position key alone. This is the value the TT may store and reuse;
    // the damping is applied at the reading node's own clock.
    int evaluate_undamped(const Board& b);
    void clear_pawn_table();

    // ---- Lazy dual-eval audit ----------------------------------------------
    // When diag_lazy is set, a lazy-margin skip ALSO runs the full positional
    // tail on the side and records the divergence; the SERVED score is the
    // lazy value either way, so behaviour (and bench) is identical with the
    // audit on. Sign flips are the failure the margin promises cannot happen;
    // crossings are full scores the margin wrongly called decided. Matters
    // beyond curiosity: datagen labels come from this eval.
    bool    diag_lazy = false;
    int64_t lazy_fires = 0, lazy_sign_flips = 0, lazy_margin_crossings = 0;
    int64_t lazy_absdelta_sum = 0, lazy_absdelta_max = 0;

    // ---- Speed telemetry (always counted, NIL-cost class) ------------------
    // eval_calls / nodes = the eval RATE: evaluate() is gated behind the TT
    // static_eval cache and skipped in check, so the share of nodes actually
    // paying for a full eval is a guess today. pawn_probes/hits size the
    // 16384-entry direct-mapped pawn cache — the input needed before
    // resizing it, and the counter Rarog's decisive cache-miss numbers came
    // from. Reset per `go` alongside the lazy-audit block.
    int64_t eval_calls = 0;
    int64_t pawn_probes = 0, pawn_hits = 0;

#ifdef BASILISK_TUNE
    // Enabled only by the hidden tune-build UCI `Diag` option. Counted at the
    // top of evaluate(), before any endgame or lazy-eval early return, because
    // every invocation represents a family reached by the search tree.
    bool diag_endgames = false;
    EndgameOccurrenceCounters endgame_occurrence;
#endif

private:
    PawnEntry pawn_table_[PAWN_TABLE_SIZE];

    void eval_pawns(const Board& b,
                    int& mg, int& eg,
                    Bitboard passed[NCOLORS],
                    Bitboard attacks[NCOLORS]);
};

// Draw-scale factor for opposite-coloured-bishop positions. Eases
// toward normal as pawns are added but is capped at the neutral 48/48: a
// draw scaler must never amplify the evaluation. (Uncapped, 32 + 4*pawns
// would reach 96/48 = 2x at 16 pawns.)
// Shared by the evaluator and the tuner's linear_delta_scale so the
// --verify trace reconstruction stays exact.
inline int ocb_draw_scale(int total_pawns) {
    const int scale = 32 + total_pawns * 4;
    return scale < 48 ? scale : 48;
}

// ---- Draw-scaling predicates shared with the Texel tuner --------------------
//
// The tuner's linear_delta_scale() has to reproduce the evaluator's draw
// scaling, or the linearisation it fits is not the function the engine
// actually computes. A silent divergence there does NOT trip --verify (it
// corrupts the fitted gradients instead, which is worse because nothing fails
// loudly), so every formula both sides need (the predicates, the dark-square
// mask, the rule-50 curve) lives here and has exactly one definition.

inline constexpr Bitboard EVAL_DARK_SQUARES = 0xAA55AA55AA55AA55ULL;

// The static "won, mate is technique" magnitude: every known-win class
// scores at or above it. The search reads it too, as the edge below which
// a stand-pat says nothing about what a capture leaves.
inline constexpr int KNOWN_WIN = 10000;

// Bare-king wins against a queen or a rook sit in tiers by the winner's
// strongest piece (apply_endgame), above every minor-piece mate.
inline constexpr int BARE_KING_QUEEN_TIER = 28700;
inline constexpr int BARE_KING_ROOK_TIER  = 26100;

// Rule-50 damping: the evaluation decays toward zero as the halfmove clock
// grows, retaining ~50% at clock 99 (SF-era curve; a (100-clock)/100 line would
// keep only 1%, telling the search almost every long-clock position is
// already drawn).
inline constexpr int DAMP_RULE50_DEN = 199;
[[nodiscard]] inline int damp_rule50_scale_num(int halfmove_clock) {
    return DAMP_RULE50_DEN - halfmove_clock;
}

// A tier's base is not damped, only the score above it: damped whole, the
// rook tier lost about 130 points a ply, more than the king-drive gradient
// the search mates with, and a lower tier could outrank it at a longer clock.
[[nodiscard]] inline int damp_rule50(int score, int halfmove_clock) {
    const int magnitude = score < 0 ? -score : score;
    const int base = magnitude >= BARE_KING_QUEEN_TIER ? BARE_KING_QUEEN_TIER
                   : magnitude >= BARE_KING_ROOK_TIER  ? BARE_KING_ROOK_TIER
                                                       : 0;
    const int damped = base + (magnitude - base) * damp_rule50_scale_num(halfmove_clock)
                                                 / DAMP_RULE50_DEN;
    return score < 0 ? -damped : damped;
}

// Division truncates toward zero, so the damping is odd and may be applied to
// a side-to-move score: the result equals damping the white-POV score.
inline int Evaluator::evaluate(const Board& b) {
    const int score = evaluate_undamped(b);
    return b.rule50_count() > 0 ? damp_rule50(score, b.rule50_count()) : score;
}

// The pawnless scaler, in Stockfish's form (material.cpp): `strong`, without
// pawns and at most a minor piece ahead in the donor's middlegame values,
// cannot force mate. The scale, of 64: 0 below a rook's worth, 4 against at
// most a bishop's worth, otherwise 14; 64 where the rule does not apply.
// Counted in middlegame values, a queen against two minors is not "a minor
// ahead"; the endgame table would make it one.
[[nodiscard]] inline int pawnless_scale(const Board& b, Color strong) {
    if (b.piece_bb(strong, PAWN))
        return 64;
    auto material = [&](Color c) {
        return popcount(b.piece_bb(c, KNIGHT)) * 781 + popcount(b.piece_bb(c, BISHOP)) * 825
             + popcount(b.piece_bb(c, ROOK)) * 1276 + popcount(b.piece_bb(c, QUEEN)) * 2538;
    };
    const int ours = material(strong), theirs = material(~strong);
    if (ours - theirs > 825)
        return 64;
    return ours < 1276 ? 0 : theirs <= 825 ? 4 : 14;
}

// Exactly one bishop each, on opposite colours. Says nothing about the rest of
// the material — the scaler deliberately fires with other pieces present;
// narrowing that scope is a deferred refinement.
[[nodiscard]] inline bool is_opposite_coloured_bishops(const Board& b) {
    const bool wb1 = !more_than_one(b.piece_bb(WHITE, BISHOP)) && b.piece_bb(WHITE, BISHOP);
    const bool bb1 = !more_than_one(b.piece_bb(BLACK, BISHOP)) && b.piece_bb(BLACK, BISHOP);
    if (!wb1 || !bb1)
        return false;
    const bool wb_dark = (b.piece_bb(WHITE, BISHOP) & EVAL_DARK_SQUARES) != 0;
    const bool bb_dark = (b.piece_bb(BLACK, BISHOP) & EVAL_DARK_SQUARES) != 0;
    return wb_dark != bb_dark;
}

[[nodiscard]] inline bool is_lone_king(const Board& b, Color c) {
    return b.occupancy_bb(c) == sq_bb(b.king_square(c));
}

// King and exactly n knights, nothing else.
[[nodiscard]] inline bool is_king_and_n_knights(const Board& b, Color c, int n) {
    return !b.piece_bb(c, PAWN) && !b.piece_bb(c, BISHOP) && !b.piece_bb(c, ROOK)
        && !b.piece_bb(c, QUEEN) && popcount(b.piece_bb(c, KNIGHT)) == n;
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
#endif
