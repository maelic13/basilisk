// History and correction policy: the bonus and malus shapes, what a cutoff
// trains, and the correction applied to the static evaluation. HistoryTables
// owns the storage and its lifecycle.

#include "search.h"
#include <algorithm>
#include <cstdlib>

void Searcher::update_quiet(Color stm, Square from, Square to, int bonus) {
    hist_update<HistoryTables::MAX_MAIN_HIST>(hist_.main[stm][from][to], bonus);
}

void Searcher::update_cap(PieceType pt, Square to, PieceType cap, int bonus) {
    hist_update<HistoryTables::MAX_CAP_HIST>(hist_.capture[pt][to][cap], bonus);
}

void Searcher::update_cont(HistoryTables::ContHistTable& tbl,
                           PieceType ppt, Square pto,
                           PieceType cpt, Square cto, int bonus) {
    hist_update<HistoryTables::MAX_CONT_HIST>(tbl.data[ppt][pto][cpt][cto], bonus);
}

void Searcher::update_pawn_hist(Key pawn_key, PieceType pt, Square to, int bonus) {
    hist_update<HistoryTables::MAX_PAWN_HIST>(hist_.pawn->data[pawn_key & (HistoryTables::PAWN_HIST_SIZE - 1)][pt][to], bonus);
}

void Searcher::update_low_ply(int ply, Square from, Square to, int bonus) {
    if (ply < HistoryTables::LOW_PLY_HISTORY_SIZE)
        hist_update<HistoryTables::MAX_LOW_HIST>(hist_.low_ply[ply][from][to], bonus);
}

// Phase 6.3 bonus/malus shape: bonus = min(quad*d^2/64 + lin*d, max), malus
// mirrored with its own knobs. Defaults reproduce the legacy min(d*d, 2048).
int Searcher::history_bonus_value(int depth) const {
    const auto& p = active_limits_.params;
    return std::min(p.hist_bonus_quad * depth * depth / 64 + p.hist_bonus_lin * depth,
                    p.hist_bonus_max);
}

int Searcher::history_malus_value(int depth) const {
    const auto& p = active_limits_.params;
    return -std::min(p.hist_malus_quad * depth * depth / 64 + p.hist_malus_lin * depth,
                     p.hist_malus_max);
}

void Searcher::update_cont_for_move(SearchStack* ss, PieceType pt, Square to, int bonus) {
    if ((ss-1)->moved_piece != NO_PIECE_TYPE && (ss-1)->move != MOVE_NONE
        && (ss-1)->move != MOVE_NULL) {
        update_cont(*hist_.cont1, (ss-1)->moved_piece,
                    Square(to_sq((ss-1)->move)), pt, to, bonus);
    }
    if ((ss-2)->moved_piece != NO_PIECE_TYPE && (ss-2)->move != MOVE_NONE
        && (ss-2)->move != MOVE_NULL) {
        update_cont(*hist_.cont2, (ss-2)->moved_piece,
                    Square(to_sq((ss-2)->move)), pt, to, bonus);
    }
    if ((ss-4)->moved_piece != NO_PIECE_TYPE && (ss-4)->move != MOVE_NONE
        && (ss-4)->move != MOVE_NULL) {
        update_cont(*hist_.cont4, (ss-4)->moved_piece,
                    Square(to_sq((ss-4)->move)), pt, to, bonus / 2);
    }
}

int Searcher::cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const {
    int score = 0;
    // 1-ply back
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        score += hist_.cont1->data[(ss-1)->moved_piece][to_sq((ss-1)->move)][pt][to];
    }
    // 2-ply back
    if ((ss-2)->move != MOVE_NONE && (ss-2)->move != MOVE_NULL
        && (ss-2)->moved_piece != NO_PIECE_TYPE) {
        score += hist_.cont2->data[(ss-2)->moved_piece][to_sq((ss-2)->move)][pt][to];
    }
    // 4-ply back keeps useful quiet continuations across one full move pair.
    if ((ss-4)->move != MOVE_NONE && (ss-4)->move != MOVE_NULL
        && (ss-4)->moved_piece != NO_PIECE_TYPE) {
        score += hist_.cont4->data[(ss-4)->moved_piece][to_sq((ss-4)->move)][pt][to] / 2;
    }
    return score;
}

// (8.6.6: first statement counts the update event by type)
void Searcher::update_all_histories(Move best, bool best_is_tt,
                                    const Move* quiets, int quiet_count,
                                    const Move* bad_caps, int bad_cap_count,
                                    Color stm, int depth, SearchStack* ss,
                                    bool reward_only, int bonus_scale) {
    DIAG_COUNT((reward_only ? diag_.hist_reward_updates : diag_.hist_cutoff_updates)++);
    // 8.5.10(e): bonus_scale (percent) lets the caller boost the reward when the
    // cutoff was "surprising" (static eval below beta -- the search saw a good
    // move the eval did not). Default 100 = unchanged.
    int bonus = history_bonus_value(depth) * bonus_scale / 100
              + (best_is_tt ? active_limits_.params.hist_ttmove_bonus : 0);
    int malus = history_malus_value(depth);

    bool best_is_cap   = (board_ptr_->piece_on(to_sq(best)) != NO_PIECE)
                      || (move_type(best) == EN_PASSANT);
    bool best_is_promo = (move_type(best) == PROMOTION);

    if (!best_is_cap && !best_is_promo) {
        Square from  = Square(from_sq(best));
        Square to    = Square(to_sq(best));
        PieceType pt = type_of(board_ptr_->piece_on(from));

        // Quiet history
        update_quiet(stm, from, to, bonus);
        update_pawn_hist(board_ptr_->pawn_key_value(), pt, to, bonus);
        update_low_ply(static_cast<int>(ss - (ss_arr_ + 4)), from, to, bonus);

        // Killers / countermove are cutoff semantics ("this move refuted the
        // node"). In reward_only mode (exact/PV nodes) the best move improved
        // alpha but did not refute anything, so we boost only the graded history
        // tables and leave the categorical killer/countermove slots alone.
        if (!reward_only) {
            // Killers
            if (ss->killers[0] != best) {
                ss->killers[1] = ss->killers[0];
                ss->killers[0] = best;
            }

            // Countermove
            Move prev = (ss-1)->move;
            if (prev != MOVE_NONE && prev != MOVE_NULL)
                hist_.countermove[from_sq(prev)][to_sq(prev)] = best;
        }

        // Continuation history
        update_cont_for_move(ss, pt, to, bonus);

        // Malus for other searched quiets
        if (!reward_only)
        for (int i = 0; i < quiet_count; ++i) {
            Move m = quiets[i];
            if (m == best) continue;
            Square mf = Square(from_sq(m)), mt = Square(to_sq(m));
            PieceType mpt = type_of(board_ptr_->piece_on(mf));
            update_quiet(stm, mf, mt, malus);
            update_pawn_hist(board_ptr_->pawn_key_value(), mpt, mt, malus);
            update_low_ply(static_cast<int>(ss - (ss_arr_ + 4)), mf, mt, malus);
            update_cont_for_move(ss, mpt, mt, malus);
        }
    } else if (best_is_cap) {
        // Best was a capture (not a quiet promotion)
        PieceType atk = type_of(board_ptr_->piece_on(from_sq(best)));
        PieceType cap = (move_type(best) == EN_PASSANT)
                      ? PAWN : type_of(board_ptr_->piece_on(to_sq(best)));
        update_cap(atk, Square(to_sq(best)), cap, bonus);
    }
    // Quiet promotions: no history update (too rare to matter)

    // Malus for bad captures searched before best
    if (!reward_only)
    for (int i = 0; i < bad_cap_count; ++i) {
        Move m = bad_caps[i];
        if (m == best) continue;
        PieceType atk = type_of(board_ptr_->piece_on(from_sq(m)));
        PieceType cap = (move_type(m) == EN_PASSANT)
                      ? PAWN : type_of(board_ptr_->piece_on(to_sq(m)));
        update_cap(atk, Square(to_sq(m)), cap, malus);
    }
}

static void update_correction_slot(int16_t& slot, int diff, int depth) {
    static constexpr int MAX_CORR = 1024;
    int w = std::min(depth + 1, 16);
    int updated = std::clamp((int(slot) * (256 - w) + diff * w) / 256, -MAX_CORR, MAX_CORR);
    slot = static_cast<int16_t>(updated);
}

void Searcher::update_correction(Color stm, const Board& board, SearchStack* ss, int diff, int depth) {
    update_correction_slot(hist_.pawn_corr[stm][board.pawn_key_value() & (HistoryTables::CORR_SIZE - 1)], diff, depth);
    update_correction_slot(hist_.minor_corr[stm][board.minor_key_value() & (HistoryTables::CORR_SIZE - 1)], diff, depth);
    update_correction_slot(hist_.nonpawn_corr[stm][WHITE][board.nonpawn_key_value(WHITE) & (HistoryTables::CORR_SIZE - 1)],
                           diff, depth);
    update_correction_slot(hist_.nonpawn_corr[stm][BLACK][board.nonpawn_key_value(BLACK) & (HistoryTables::CORR_SIZE - 1)],
                           diff, depth);

    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        update_correction_slot(hist_.cont_corr[stm][(ss-1)->moved_piece][to_sq((ss-1)->move)],
                               diff, depth);
    }
}

int Searcher::correction_value(Color stm, const Board& board, const SearchStack* ss) const {
    const int pawn = hist_.pawn_corr[stm][board.pawn_key_value() & (HistoryTables::CORR_SIZE - 1)];
    const int minor = hist_.minor_corr[stm][board.minor_key_value() & (HistoryTables::CORR_SIZE - 1)];
    const int own = hist_.nonpawn_corr[stm][stm][board.nonpawn_key_value(stm) & (HistoryTables::CORR_SIZE - 1)];
    const int opp = hist_.nonpawn_corr[stm][~stm][board.nonpawn_key_value(~stm) & (HistoryTables::CORR_SIZE - 1)];

    int cont = 0;
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE) {
        cont = hist_.cont_corr[stm][(ss-1)->moved_piece][to_sq((ss-1)->move)];
    }

    return (pawn + minor + own + opp + cont) / 5;
}

void Searcher::age_history() {
    hist_.age();
}
