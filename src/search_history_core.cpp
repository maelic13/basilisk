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
#include <cstdlib>

static bool move_made(Move m) { return m != MOVE_NONE && m != MOVE_NULL; }

void Searcher::update_continuation_histories(SearchStack* ss, PieceType pt, Square to,
                                             int bonus) {
    struct Back { int plies; int weight; };
    static constexpr Back backs[] = {{1, 520}, {2, 390}, {3, 145}, {4, 251}, {5, 66}, {6, 209}};
    // Indexed by the number of rows that already rate the move positively.
    static constexpr int consistency[] = {94, 103, 110, 106, 119, 126, 121};

    int positive = 0;
    for (const Back& b : backs) {
        // A move made in check trains only the two nearest continuations.
        if (ss->in_check && b.plies > 2)
            break;
        SearchStack* prev = ss - b.plies;
        if (!move_made(prev->move))
            continue;
        int16_t& e = prev->cont_hist->entry[pt][to];
        if (e > 0)
            ++positive;
        PieceToHistory::update(e, bonus * b.weight * consistency[positive] / 65536
                                  + 73 * (b.plies < 2));
    }
}

void Searcher::update_quiet_histories(SearchStack* ss, Move m, int bonus) {
    const Square from = Square(from_sq(m));
    const Square to   = Square(to_sq(m));
    const PieceType pt = type_of(state_.board->piece_on(from));
    const int ply = static_cast<int>(ss - (state_.stack + STACK_SENTINELS));

    MainHistory::update(hist_.main.entry[state_.board->turn()][from][to], bonus);
    if (ply < LowPlyHistory::PLIES)
        LowPlyHistory::update(hist_.low_ply.entry[ply][from][to], bonus * 712 / 1024);
    update_continuation_histories(ss, pt, to, bonus * 750 / 1024);
    PawnHistoryBucket::update(pawn_bucket().entry[pt][to],
                              bonus * (bonus > -4 ? 1104 : 459) / 1024);
}

void Searcher::update_capture_history(Move m, int bonus) {
    const Square to = Square(to_sq(m));
    const PieceType attacker = type_of(state_.board->piece_on(from_sq(m)));
    const PieceType captured = move_type(m) == EN_PASSANT ? PAWN
                             : type_of(state_.board->piece_on(to));
    CaptureHistory::update(hist_.capture.entry[attacker][to][captured], bonus);
}

int Searcher::cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const {
    return (ss - 1)->cont_hist->entry[pt][to]
         + (ss - 2)->cont_hist->entry[pt][to]
         + (ss - 4)->cont_hist->entry[pt][to] / 2;
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
    const auto& p = config_.limits.params;
    const Board& b = *state_.board;
    int bonus = std::min(p.quiet_bonus_slope * depth - 81, p.quiet_bonus_cap)
              + 364 * (best == tt_move) + (ss - 1)->stat_score / 28;
    const int malus = std::min(p.malus_slope * depth - 235, p.malus_cap);
    if (!pv_node)
        bonus += int(int64_t(bonus) * (quiet_count + capture_count) / 256);

    const bool best_capture_stage = b.piece_on(to_sq(best)) != NO_PIECE
                                 || move_type(best) == EN_PASSANT || move_type(best) == PROMOTION;
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
        update_continuation_histories(ss - 1, type_of(b.piece_on(prev_to)), prev_to,
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
    const Board& b = *state_.board;
    const Move prev = (ss - 1)->move;
    const Square prev_to = Square(to_sq(prev));
    const Piece captured = b.captured_piece();
    if (captured != NO_PIECE) {
        CaptureHistory::update(
            hist_.capture.entry[type_of(b.piece_on(prev_to))][prev_to][type_of(captured)], 892);
        return;
    }
    int scale = -241;
    scale -= (ss - 1)->stat_score / 98;
    scale += std::min(59 * depth, 420);
    scale += 186 * ((ss - 1)->move_count > 9);
    scale += 142 * (!ss->in_check && best_score <= ss->eval - 30);
    scale += 159 * (!(ss - 1)->in_check && best_score <= -(ss - 1)->eval - 19);
    scale = std::max(scale, 0);
    const int scaled = std::min(150 * depth - 85, 1337) * scale;

    const PieceType pt = type_of(b.piece_on(prev_to));
    update_continuation_histories(ss - 1, pt, prev_to, scaled * 263 / 16384);
    MainHistory::update(hist_.main.entry[~b.turn()][from_sq(prev)][prev_to], scaled * 215 / 32768);
    if (pt != PAWN && move_type(prev) != PROMOTION)
        PawnHistoryBucket::update(pawn_bucket().entry[pt][prev_to], scaled * 324 / 8192);
}

void Searcher::update_parent_by_eval_difference(SearchStack* ss, int eval_diff, bool tt_hit) {
    const Move prev = (ss - 1)->move;
    const Square from = Square(from_sq(prev));
    const Square to   = Square(to_sq(prev));
    MainHistory::update(hist_.main.entry[~state_.board->turn()][from][to], eval_diff * 11);
    const PieceType pt = type_of(state_.board->piece_on(to));
    if (!tt_hit && pt != PAWN && move_type(prev) != PROMOTION)
        PawnHistoryBucket::update(pawn_bucket().entry[pt][to], eval_diff * 13);
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
    }
    return 15341 * pawn + 10569 * minor + 12906 * (white + black) + cont;
}

void Searcher::update_correction_history(SearchStack* ss, int bonus) {
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
        DIAG_COUNT(++state_.diag.corr_cont2_updates);
        DIAG_COUNT(++state_.diag.corr_cont4_updates);
    }
}
