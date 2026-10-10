// The selectivity core's history and correction policy (the B2_CORE arm):
// where a training signal goes and how large it is. CoreHistoryTables owns the
// storage and its lifecycle.
//
// The routing is the cluster's: a quiet move trains the main, low-ply,
// continuation and pawn histories in fixed proportion, and a continuation
// update reaches the rows of the six previous moves with per-ply weights and
// a multiplier that grows with the number of rows already agreeing. The
// correction is the cluster's weighted form. The bonus and malus shapes and
// the moves a cutoff trains are still the legacy kernel's.

#include "search_worker.h"
#include <algorithm>
#include <cstdlib>

static bool move_made(Move m) { return m != MOVE_NONE && m != MOVE_NULL; }

// Bonus = min(quad*d^2/64 + lin*d, max), malus mirrored with its own knobs.
int Searcher::history_bonus_value(int depth) const {
    const auto& p = config_.limits.params;
    return std::min(p.hist_bonus_quad * depth * depth / 64 + p.hist_bonus_lin * depth,
                    p.hist_bonus_max);
}

int Searcher::history_malus_value(int depth) const {
    const auto& p = config_.limits.params;
    return -std::min(p.hist_malus_quad * depth * depth / 64 + p.hist_malus_lin * depth,
                     p.hist_malus_max);
}

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

void Searcher::update_all_histories(Move best, bool best_is_tt,
                                    const Move* quiets, int quiet_count,
                                    const Move* bad_caps, int bad_cap_count,
                                    Color stm, int depth, SearchStack* ss,
                                    bool reward_only, int bonus_scale) {
    (void)stm;
    DIAG_COUNT((reward_only ? state_.diag.hist_reward_updates : state_.diag.hist_cutoff_updates)++);
    // bonus_scale (percent) boosts the reward when the cutoff was surprising:
    // the static eval was below beta.
    const int bonus = history_bonus_value(depth) * bonus_scale / 100
                    + (best_is_tt ? config_.limits.params.hist_ttmove_bonus : 0);
    const int malus = history_malus_value(depth);

    const bool best_is_cap   = state_.board->piece_on(to_sq(best)) != NO_PIECE
                            || move_type(best) == EN_PASSANT;
    const bool best_is_promo = move_type(best) == PROMOTION;

    if (!best_is_cap && !best_is_promo) {
        update_quiet_histories(ss, best, bonus);
        if (!reward_only)
            for (int i = 0; i < quiet_count; ++i)
                if (quiets[i] != best)
                    update_quiet_histories(ss, quiets[i], malus);
    } else if (best_is_cap) {
        update_capture_history(best, bonus);
    }
    // Quiet promotions train nothing.

    if (!reward_only)
        for (int i = 0; i < bad_cap_count; ++i)
            if (bad_caps[i] != best)
                update_capture_history(bad_caps[i], malus);
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
