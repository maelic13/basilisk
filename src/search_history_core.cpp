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
#include <algorithm>
#include <climits>
#include <cstdlib>

static bool move_made(Move m) { return m != MOVE_NONE && m != MOVE_NULL; }

namespace {

// The continuation update's per-ply weights, and its multiplier indexed by
// the number of rows that already rate the move positively.
struct Back { int plies; int weight; };
constexpr Back CONT_BACKS[] = {{1, 520}, {2, 390}, {3, 145}, {4, 251}, {5, 66}, {6, 209}};
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
constexpr int FL_BASE = -241;
constexpr int FL_STAT_DIVISOR = 98;
constexpr int FL_DEPTH_SLOPE = 59;
constexpr int FL_DEPTH_CAP = 420;
constexpr int FL_LATE_MOVE = 186;
constexpr int FL_BELOW_OWN_EVAL = 142;
constexpr int FL_BELOW_PARENT_EVAL = 159;
constexpr int FL_SIZE_SLOPE = 150;
constexpr int FL_SIZE_OFFSET = 85;
constexpr int FL_SIZE_CAP = 1337;
constexpr int FL_CONT_NUM = 263;
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
    for (const Back& b : CONT_BACKS) {
        // A move made in check trains only the two nearest continuations.
        if (ss->in_check && b.plies > 2)
            break;
        SearchStack* prev = ss - b.plies;
        if (!move_made(prev->move))
            continue;
        int16_t& e = prev->cont_hist->entry[hist_piece(piece)][to];
        if (e > 0)
            ++positive;
        PieceToHistory::update(e, bonus * b.weight * CONT_CONSISTENCY[positive] / 65536
                                  + 73 * (b.plies < 2));
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
        LowPlyHistory::update(hist_.low_ply.entry[ply][from][to], bonus * 712 / 1024);
    update_continuation_histories(ss, piece, to, bonus * 750 / 1024);
    PawnHistoryBucket::update(pawn_bucket().entry[hist_piece(piece)][to],
                              bonus * (bonus > -4 ? 1104 : 459) / 1024);
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
    int bonus = std::min(p.quiet_bonus_slope * depth - 81, p.quiet_bonus_cap)
              + 364 * (best == tt_move) + (ss - 1)->stat_score / 28;
    const int malus = std::min(p.malus_slope * depth - 235, p.malus_cap);
    if (!pv_node)
        bonus += int(int64_t(bonus) * (quiet_count + capture_count) / 256);

    const bool best_capture_stage = is_capture_stage(b, best);
    if (!best_capture_stage) {
        update_quiet_histories(ss, best, bonus * 899 / 1024);
        int actual_malus = malus * 1159 / 1024;
        for (int i = 0; i < quiet_count; ++i) {
            actual_malus = actual_malus * p.malus_decay / 1024;
            update_quiet_histories(ss, quiets[i], -actual_malus);
        }
    } else {
        update_capture_history(best, bonus * 1427 / 1024);
    }

    const Move prev = (ss - 1)->move;
    if (move_made(prev) && (ss - 1)->move_count == 1 + int((ss - 1)->tt_hit)
        && b.captured_piece() == NO_PIECE) {
        const Square prev_to = Square(to_sq(prev));
        update_continuation_histories(ss - 1, b.piece_on(prev_to), prev_to,
                                      -malus * 713 / 1024);
    }

    for (int i = 0; i < capture_count; ++i)
        update_capture_history(captures[i], -malus * 1489 / 1024);
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
            hist_.capture.entry[hist_piece(b.piece_on(prev_to))][prev_to][type_of(captured)], 892);
        HIST_SHADOW(hist_.shadow->write(
            ShadowTable::Capture,
            CoreHistoryTables::capture_cell(hist_piece(b.piece_on(prev_to)), prev_to, type_of(captured)),
            ~b.turn()));
        return;
    }
    int scale = FL_BASE;
    scale -= (ss - 1)->stat_score / FL_STAT_DIVISOR;
    scale += std::min(FL_DEPTH_SLOPE * depth, FL_DEPTH_CAP);
    scale += FL_LATE_MOVE * ((ss - 1)->move_count > 9);
    scale += FL_BELOW_OWN_EVAL * (!ss->in_check && best_score <= ss->eval - 30);
    scale += FL_BELOW_PARENT_EVAL * (!(ss - 1)->in_check && best_score <= -(ss - 1)->eval - 19);
    scale = std::max(scale, 0);
    const int scaled = std::min(FL_SIZE_SLOPE * depth - FL_SIZE_OFFSET, FL_SIZE_CAP) * scale;

    const PieceType pt = type_of(b.piece_on(prev_to));
    update_continuation_histories(ss - 1, b.piece_on(prev_to), prev_to,
                                  scaled * FL_CONT_NUM / FL_CONT_DEN);
    MainHistory::update(hist_.main.entry[~b.turn()][from_sq(prev)][prev_to], scaled * 215 / 32768);
    if (pt != PAWN && move_type(prev) != PROMOTION) {
        const int piece = hist_piece(b.piece_on(prev_to));
        PawnHistoryBucket::update(pawn_bucket().entry[piece][prev_to], scaled * 324 / 8192);
        HIST_SHADOW(hist_.shadow->write(ShadowTable::Pawn,
                                        hist_.pawn_cell(&pawn_bucket(), piece, prev_to), ~b.turn()));
    }
}

void Searcher::update_parent_by_eval_difference(SearchStack* ss, int eval_diff, bool tt_hit) {
    if (PRICE_PROBING()) return;
    const Move prev = (ss - 1)->move;
    const Square from = Square(from_sq(prev));
    const Square to   = Square(to_sq(prev));
    MainHistory::update(hist_.main.entry[~state_.board->turn()][from][to], eval_diff * 11);
    const PieceType pt = type_of(state_.board->piece_on(to));
    if (!tt_hit && pt != PAWN && move_type(prev) != PROMOTION) {
        const int piece = hist_piece(state_.board->piece_on(to));
        PawnHistoryBucket::update(pawn_bucket().entry[piece][to], eval_diff * 13);
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
    const Color us = b.turn();
    const int pawn  = hist_.pawn_corr[us][b.pawn_key_value() & mask];
    const int minor = hist_.minor_corr[us][b.minor_key_value() & mask];
    const int white = hist_.nonpawn_corr[us][WHITE][b.nonpawn_key_value(WHITE) & mask];
    const int black = hist_.nonpawn_corr[us][BLACK][b.nonpawn_key_value(BLACK) & mask];
    const Move m = (ss - 1)->move;
    int cont = 64049;
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
    return 15341 * pawn + 10569 * minor + 12906 * (white + black) + cont;
}

void Searcher::update_correction_history(SearchStack* ss, int bonus) {
    if (PRICE_PROBING()) return;
    constexpr Key mask = CoreHistoryTables::CORR_SIZE - 1;
    const Board& b = *state_.board;
    const Color us = b.turn();
    CorrectionBound::update(hist_.pawn_corr[us][b.pawn_key_value() & mask], bonus);
    CorrectionBound::update(hist_.minor_corr[us][b.minor_key_value() & mask], bonus * 150 / 128);
    CorrectionBound::update(hist_.nonpawn_corr[us][WHITE][b.nonpawn_key_value(WHITE) & mask],
                            bonus * 186 / 128);
    CorrectionBound::update(hist_.nonpawn_corr[us][BLACK][b.nonpawn_key_value(BLACK) & mask],
                            bonus * 186 / 128);
    const Move m = (ss - 1)->move;
    if (move_made(m)) {
        const Square to = Square(to_sq(m));
        const PieceType pt = type_of(b.piece_on(to));
        PieceToCorrection::update((ss - 2)->cont_corr->entry[pt][to], bonus * 130 / 128);
        PieceToCorrection::update((ss - 4)->cont_corr->entry[pt][to], bonus * 70 / 128);
#ifdef BASILISK_DIAGNOSTIC
        for (int back : {2, 4})
            hist_.shadow->write(ShadowTable::ContCorr,
                                hist_.cont_corr_cell((ss - back)->cont_corr, pt, to), us);
#endif
        DIAG_COUNT(++state_.diag.corr_cont2_updates);
        DIAG_COUNT(++state_.diag.corr_cont4_updates);
    }
}
