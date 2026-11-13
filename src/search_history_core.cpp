// The selectivity core's history and correction policy:
// where a training signal goes and how large it is. CoreHistoryTables owns the
// storage and its lifecycle.
//
// A quiet move trains the main, low-ply, continuation and pawn histories in
// fixed proportion, and a continuation update reaches the rows of the six
// previous moves with per-ply weights and a multiplier that grows with the
// number of rows already agreeing. A node whose best move raised alpha
// rewards it and punishes the moves searched before it; a node that failed
// low rewards the parent's move that caused it. The correction is the
// cluster's weighted form.

#include "search_worker.h"
#include "zobrist.h"
#include <algorithm>
#include <climits>
#include <cstdlib>

static bool move_made(Move m) { return m != MOVE_NONE && m != MOVE_NULL; }

// A side's non-pawn correction key; with `kings` it includes the side's king,
// the donor's form, so a bare king no longer shares one entry.
static Key nonpawn_corr_key(const Board& b, Color c, bool kings) {
    const Key k = b.nonpawn_key_value(c);
    return kings ? k ^ Zobrist::PieceKeys[c][KING][b.king_square(c)] : k;
}

namespace {

// The continuation update's per-ply weights, and its multiplier indexed by
// the number of rows that already rate the move positively.
struct Back { int plies; int weight; };
constexpr const KernelLiterals& KL = kKernelLiteralDefaults;
constexpr Back CONT_BACKS[] = {{1, KL.cont_w1}, {2, KL.cont_w2}, {3, KL.cont_w3},
                               {4, KL.cont_w4}, {5, KL.cont_w5}, {6, KL.cont_w6}};
constexpr int CONT_CONSISTENCY[] = {94, 103, 110, 106, 119, 126, 121};

constexpr int max_cont_weight() {
    int m = 0;
    for (const Back& b : CONT_BACKS) m = std::max(m, b.weight);
    return m;
}
constexpr int max_cont_consistency() {
    int m = 0;
    for (int c : CONT_CONSISTENCY) m = std::max(m, c);
    return m;
}

// The largest incoming bonus whose product with any weight and multiplier
// fits an int: the update's arithmetic is 32-bit and must stay so.
constexpr int CONT_BONUS_BOUND = INT_MAX / (max_cont_weight() * max_cont_consistency());

// The fail-low parent's scale terms and its continuation share.
// The compiled values (src/kernel_literals.h): the proof below reads them, the
// update reads KLIT, which is these outside the literal-screen build.
constexpr int FL_BASE = KL.fl_base;
constexpr int FL_STAT_DIVISOR = KL.fl_stat_div;
constexpr int FL_DEPTH_CAP = KL.fl_depth_cap;
constexpr int FL_LATE_MOVE = KL.fl_late_move;
constexpr int FL_BELOW_OWN_EVAL = KL.fl_below_own;
constexpr int FL_BELOW_PARENT_EVAL = KL.fl_below_parent;
constexpr int FL_SIZE_SLOPE = KL.fl_size_slope;
constexpr int FL_SIZE_OFFSET = KL.fl_size_offset;
constexpr int FL_SIZE_CAP = KL.fl_size_cap;
constexpr int FL_CONT_NUM = KL.fl_cont_num;
constexpr int FL_CONT_DEN = 16384;

// The width proof for the widest caller: at the history limits the fail-low
// parent's continuation bonus stays within the bound. The parent's stat
// score is at least the quiet score of three entries at their negative
// limits, or a capture-stage move's at least -CaptureHistory::LIMIT (its
// captured value is never negative); every other scale term is at its cap;
// a main-search node's depth is at least 1, so the size term is positive
// and at most its cap. The scale's floor is 0, so the bonus is never
// negative.
constexpr int FL_STAT_SCORE_FLOOR =
    std::min(QuietStatScore::of(-MainHistory::LIMIT, -PieceToHistory::LIMIT,
                                -PieceToHistory::LIMIT),
             -CaptureHistory::LIMIT);
constexpr int FL_SCALE_MAX = FL_BASE - FL_STAT_SCORE_FLOOR / FL_STAT_DIVISOR + FL_DEPTH_CAP
                           + FL_LATE_MOVE + FL_BELOW_OWN_EVAL + FL_BELOW_PARENT_EVAL;
static_assert(FL_SIZE_SLOPE * 1 - FL_SIZE_OFFSET > 0, "the size term is positive from depth 1");
constexpr int FL_CONT_BONUS_MAX = FL_SIZE_CAP * FL_SCALE_MAX * FL_CONT_NUM / FL_CONT_DEN;
static_assert(FL_CONT_BONUS_MAX <= CONT_BONUS_BOUND,
              "the fail-low parent's continuation bonus can wrap the continuation update");

}  // namespace

void Searcher::update_continuation_histories(SearchStack* ss, Piece piece, Square to,
                                             int bonus) {
    CORE_NODE_CHECK(bonus >= -CONT_BONUS_BOUND && bonus <= CONT_BONUS_BOUND);
    if (PRICE_PROBING()) return;

    int positive = 0;
#ifdef BASILISK_LITERAL_SCREEN
    const Back backs[] = {{1, KLIT(cont_w1)}, {2, KLIT(cont_w2)}, {3, KLIT(cont_w3)},
                          {4, KLIT(cont_w4)}, {5, KLIT(cont_w5)}, {6, KLIT(cont_w6)}};
#else
    constexpr const auto& backs = CONT_BACKS;
#endif
    for (const Back& b : backs) {
        // A move made in check trains only the two nearest continuations.
        if (ss->in_check && b.plies > 2)
            break;
        SearchStack* prev = ss - b.plies;
        if (!move_made(prev->move))
            continue;
        int16_t& e = prev->cont_hist->entry[hist_piece(piece)][to];
        if (e > 0)
            ++positive;
#ifdef BASILISK_LITERAL_SCREEN
        // The screen's weights are run-time values the width proof above does
        // not cover, so the product is taken in 64 bits; the update clamps it.
        PieceToHistory::update(e, static_cast<int>(std::clamp<long long>(
            KLIT_WIDE(bonus) * b.weight
                * KLIT_SCALED(CONT_CONSISTENCY[positive], cont_consistency_scale) / 65536
            + KLIT(cont_near_bonus) * (b.plies < 2), -1'000'000, 1'000'000)));
#else
        PieceToHistory::update(e, bonus * b.weight
                                      * KLIT_SCALED(CONT_CONSISTENCY[positive], cont_consistency_scale) / 65536
                                  + KLIT(cont_near_bonus) * (b.plies < 2));
#endif
        HIST_SHADOW(hist_.shadow->write(ShadowTable::Continuation,
                                        hist_.continuation_cell(prev->cont_hist, hist_piece(piece), to),
                                        shadow_mover(ss)));
    }
}

void Searcher::update_quiet_histories(SearchStack* ss, Move m, int bonus) {
    if (PRICE_PROBING()) return;
    const Square from = Square(from_sq(m));
    const Square to   = Square(to_sq(m));
    const Piece piece = state_.board->piece_on(from);
    const int ply = static_cast<int>(ss - (state_.stack + STACK_SENTINELS));

    MainHistory::update(hist_.main.entry[state_.board->turn()][from][to], bonus);
    if (ply < LowPlyHistory::PLIES)
        LowPlyHistory::update(hist_.low_ply.entry[ply][from][to], bonus * KLIT(low_ply_share) / 1024);
    update_continuation_histories(ss, piece, to, bonus * KLIT(cont_share) / 1024);
    PawnHistoryBucket::update(pawn_bucket().entry[hist_piece(piece)][to],
                              bonus * (bonus > KLIT(pawn_share_threshold) ? KLIT(pawn_share_up)
                                                                       : KLIT(pawn_share_down)) / 1024);
    HIST_SHADOW(hist_.shadow->write(ShadowTable::Pawn,
                                    hist_.pawn_cell(&pawn_bucket(), hist_piece(piece), to),
                                    state_.board->turn()));
}

void Searcher::update_capture_history(Move m, int bonus) {
    if (PRICE_PROBING()) return;
    const Square to = Square(to_sq(m));
    const int attacker = hist_piece(state_.board->piece_on(from_sq(m)));
    const PieceType captured = move_type(m) == EN_PASSANT ? PAWN
                             : type_of(state_.board->piece_on(to));
    CaptureHistory::update(hist_.capture.entry[attacker][to][captured], bonus);
    HIST_SHADOW(hist_.shadow->write(ShadowTable::Capture,
                                    CoreHistoryTables::capture_cell(attacker, to, captured),
                                    state_.board->turn()));
}

int Searcher::cont_hist_score(const SearchStack* ss, int piece, Square to) const {
#ifdef BASILISK_DIAGNOSTIC
    for (int back : {1, 2, 4})
        hist_.shadow->read(ShadowTable::Continuation,
                           hist_.continuation_cell((ss - back)->cont_hist, piece, to),
                           state_.board->turn());
#endif
    return (ss - 1)->cont_hist->entry[piece][to]
         + (ss - 2)->cont_hist->entry[piece][to]
         + (ss - 4)->cont_hist->entry[piece][to] / 2;
}

// After a move raised alpha: the bonus grows with depth, with the TT move as
// best and with the parent's move's history score, and at non-PV nodes with
// the number of moves searched; the malus grows with depth. A quiet best
// move trains the quiet histories and every earlier quiet loses a malus
// that decays with each one; a capture-stage best move trains the capture
// history, and every earlier capture loses. The parent's early quiet move
// that this node refutes loses too.
void Searcher::update_all_stats(SearchStack* ss, Move best, Move tt_move,
                                const Move* quiets, int quiet_count,
                                const Move* captures, int capture_count,
                                int depth, bool pv_node) {
    if (PRICE_PROBING()) return;
    const auto& p = config_.limits.params;
    const Board& b = *state_.board;
    int bonus = std::min(p.quiet_bonus_slope * depth + KLIT(quiet_bonus_offset), p.quiet_bonus_cap)
              + KLIT(quiet_bonus_tt) * (best == tt_move) + (ss - 1)->stat_score / KLIT(quiet_bonus_stat_den);
    const int malus = std::min(p.malus_slope * depth + KLIT(malus_offset), p.malus_cap);
    if (!pv_node)
        bonus += int(int64_t(bonus) * (quiet_count + capture_count) / KLIT(bonus_count_den));

    const bool best_capture_stage = is_capture_stage(b, best);
    if (!best_capture_stage) {
        update_quiet_histories(ss, best, bonus * KLIT(quiet_bonus_share) / 1024);
        int actual_malus = malus * KLIT(malus_share) / 1024;
        for (int i = 0; i < quiet_count; ++i) {
            actual_malus = actual_malus * p.malus_decay / 1024;
            update_quiet_histories(ss, quiets[i], -actual_malus);
        }
    } else {
        update_capture_history(best, bonus * KLIT(capt_bonus_share) / 1024);
    }

    const Move prev = (ss - 1)->move;
    if (move_made(prev) && (ss - 1)->move_count == 1 + int((ss - 1)->tt_hit)
        && b.captured_piece() == NO_PIECE) {
        const Square prev_to = Square(to_sq(prev));
        update_continuation_histories(ss - 1, b.piece_on(prev_to), prev_to,
                                      -malus * KLIT(cont_malus_share) / 1024);
    }

    for (int i = 0; i < capture_count; ++i)
        update_capture_history(captures[i], -malus * KLIT(capt_malus_share) / 1024);
}

// After a fail low: the parent's move caused it. A quiet one is rewarded by
// a scale that grows with depth and with how late and how badly rated it
// was, and with how far this node fell below its own and the parent's
// static evals; a capture is rewarded by a fixed bonus.
void Searcher::update_fail_low_parent(SearchStack* ss, int depth, int best_score) {
    if (PRICE_PROBING()) return;
    const Board& b = *state_.board;
    const Move prev = (ss - 1)->move;
    const Square prev_to = Square(to_sq(prev));
    const Piece captured = b.captured_piece();
    if (captured != NO_PIECE) {
        CaptureHistory::update(
            hist_.capture.entry[hist_piece(b.piece_on(prev_to))][prev_to][type_of(captured)], KLIT(fl_capture));
        HIST_SHADOW(hist_.shadow->write(
            ShadowTable::Capture,
            CoreHistoryTables::capture_cell(hist_piece(b.piece_on(prev_to)), prev_to, type_of(captured)),
            ~b.turn()));
        return;
    }
    int scale = KLIT(fl_base);
    scale -= (ss - 1)->stat_score / KLIT(fl_stat_div);
    scale += std::min(KLIT(fl_depth_slope) * depth, KLIT(fl_depth_cap));
    scale += KLIT(fl_late_move) * ((ss - 1)->move_count > KLIT(fl_late_count));
    scale += KLIT(fl_below_own) * (!ss->in_check && best_score <= ss->eval - KLIT(fl_own_margin));
    scale += KLIT(fl_below_parent)
           * (!(ss - 1)->in_check && best_score <= -(ss - 1)->eval - KLIT(fl_parent_margin));
    scale = std::max(scale, 0);
    const int scaled = std::min(KLIT(fl_size_slope) * depth - KLIT(fl_size_offset), KLIT(fl_size_cap)) * scale;

    const PieceType pt = type_of(b.piece_on(prev_to));
    update_continuation_histories(ss - 1, b.piece_on(prev_to), prev_to,
                                  static_cast<int>(KLIT_WIDE(scaled) * KLIT(fl_cont_num) / FL_CONT_DEN));
    MainHistory::update(hist_.main.entry[~b.turn()][from_sq(prev)][prev_to],
                        static_cast<int>(KLIT_WIDE(scaled) * KLIT(fl_main_share) / 32768));
    if (pt != PAWN && move_type(prev) != PROMOTION) {
        const int piece = hist_piece(b.piece_on(prev_to));
        PawnHistoryBucket::update(pawn_bucket().entry[piece][prev_to],
                                  static_cast<int>(KLIT_WIDE(scaled) * KLIT(fl_pawn_share) / 8192));
        HIST_SHADOW(hist_.shadow->write(ShadowTable::Pawn,
                                        hist_.pawn_cell(&pawn_bucket(), piece, prev_to), ~b.turn()));
    }
}

void Searcher::update_parent_by_eval_difference(SearchStack* ss, int eval_diff, bool tt_hit) {
    if (PRICE_PROBING()) return;
    const Move prev = (ss - 1)->move;
    const Square from = Square(from_sq(prev));
    const Square to   = Square(to_sq(prev));
    MainHistory::update(hist_.main.entry[~state_.board->turn()][from][to], eval_diff * KLIT(eval_diff_main));
    const PieceType pt = type_of(state_.board->piece_on(to));
    if (!tt_hit && pt != PAWN && move_type(prev) != PROMOTION) {
        const int piece = hist_piece(state_.board->piece_on(to));
        PawnHistoryBucket::update(pawn_bucket().entry[piece][to], eval_diff * KLIT(eval_diff_pawn));
        HIST_SHADOW(hist_.shadow->write(ShadowTable::Pawn, hist_.pawn_cell(&pawn_bucket(), piece, to),
                                        ~state_.board->turn()));
    }
}

// The correction of the node at `ss`, in 131072ths of an evaluation unit: the
// pawn, minor-piece and both non-pawn structures' entries for the side to
// move, and the continuation corrections the moves two and four plies up
// select for the previous move (a fixed offset when there is none).
int Searcher::correction_value(const SearchStack* ss) const {
    constexpr Key mask = CoreHistoryTables::CORR_SIZE - 1;
    const Board& b = *state_.board;
    const bool kings = config_.limits.params.corr_nonpawn_kings;
    const Color us = b.turn();
    const int pawn  = hist_.pawn_corr[us][b.pawn_key_value() & mask];
    const int minor = hist_.minor_corr[us][b.minor_key_value() & mask];
    const int white = hist_.nonpawn_corr[us][WHITE][nonpawn_corr_key(b, WHITE, kings) & mask];
    const int black = hist_.nonpawn_corr[us][BLACK][nonpawn_corr_key(b, BLACK, kings) & mask];
    const Move m = (ss - 1)->move;
    int cont = KLIT(corr_cont_none);
    if (move_made(m)) {
        const Square to = Square(to_sq(m));
        const PieceType pt = type_of(b.piece_on(to));
        cont = config_.limits.params.corr_cont_weight
             * ((ss - 2)->cont_corr->entry[pt][to] + (ss - 4)->cont_corr->entry[pt][to]);
#ifdef BASILISK_DIAGNOSTIC
        for (int back : {2, 4})
            hist_.shadow->read(ShadowTable::ContCorr,
                               hist_.cont_corr_cell((ss - back)->cont_corr, pt, to), us);
#endif
    }
    return KLIT(corr_pawn_w) * pawn + KLIT(corr_minor_w) * minor + KLIT(corr_nonpawn_w) * (white + black) + cont;
}

void Searcher::update_correction_history(SearchStack* ss, int bonus) {
    if (PRICE_PROBING()) return;
    constexpr Key mask = CoreHistoryTables::CORR_SIZE - 1;
    const Board& b = *state_.board;
    const bool kings = config_.limits.params.corr_nonpawn_kings;
    const Color us = b.turn();
    CorrectionBound::update(hist_.pawn_corr[us][b.pawn_key_value() & mask], bonus);
    CorrectionBound::update(hist_.minor_corr[us][b.minor_key_value() & mask], bonus * KLIT(corr_minor_share) / 128);
    CorrectionBound::update(hist_.nonpawn_corr[us][WHITE][nonpawn_corr_key(b, WHITE, kings) & mask],
                            bonus * KLIT(corr_nonpawn_share) / 128);
    CorrectionBound::update(hist_.nonpawn_corr[us][BLACK][nonpawn_corr_key(b, BLACK, kings) & mask],
                            bonus * KLIT(corr_nonpawn_share) / 128);
    const Move m = (ss - 1)->move;
    if (move_made(m)) {
        const Square to = Square(to_sq(m));
        const PieceType pt = type_of(b.piece_on(to));
        PieceToCorrection::update((ss - 2)->cont_corr->entry[pt][to], bonus * KLIT(corr_cont2_share) / 128);
        PieceToCorrection::update((ss - 4)->cont_corr->entry[pt][to], bonus * KLIT(corr_cont4_share) / 128);
#ifdef BASILISK_DIAGNOSTIC
        for (int back : {2, 4})
            hist_.shadow->write(ShadowTable::ContCorr,
                                hist_.cont_corr_cell((ss - back)->cont_corr, pt, to), us);
#endif
        DIAG_COUNT(++state_.diag.corr_cont2_updates);
        DIAG_COUNT(++state_.diag.corr_cont4_updates);
    }
}
