#include "move_picker.h"
#include <array>
#include <cstddef>

void score_moves(ScoredMove* moves, int n, const Board& b, const HistoryTables& history,
                 const SearchStack* ss, int ply, const RootOrdering* root) {

    // 9.6: these history-table dimensions are constant for every quiet move at
    // this node. Hoist them beside the continuation rows so the move loop only
    // indexes its varying piece/from/to dimensions. Keeping the additions in
    // the same order preserves the fixed-depth bench exactly.
    const auto& main_hist = history.main[b.turn()];
    const auto& pawn_hist = history.pawn->data[
        b.pawn_key_value() & (HistoryTables::PAWN_HIST_SIZE - 1)];
    const auto* low_ply_hist = ply < HistoryTables::LOW_PLY_HISTORY_SIZE
                             ? &history.low_ply[ply] : nullptr;

    Move cm = MOVE_NONE;
    Move prev = (ss-1)->move;
    if (prev != MOVE_NONE && prev != MOVE_NULL)
        cm = history.countermove[from_sq(prev)][to_sq(prev)];

    std::array<Bitboard, PIECE_TYPE_NB> check_squares{};
    std::array<bool, PIECE_TYPE_NB> check_squares_ready{};
    auto checks_for = [&](PieceType pt) {
        const auto idx = static_cast<size_t>(pt);
        if (!check_squares_ready[idx]) {
            check_squares[idx] = b.check_squares(pt, b.turn());
            check_squares_ready[idx] = true;
        }
        return check_squares[idx];
    };

    // 8.7.6(b): hoist the continuation-history row bases ONCE per node. The
    // (ss-1/2/4) piece/square indices are constant across every scored move,
    // so recomputing the guards and the first two array dimensions (two index
    // multiplies each) per quiet — cont_hist_score(ss, ...) — is pure waste.
    // With the rows hoisted, the per-move cost is three [pt][to] loads. The
    // computed sum is identical (same terms, same order, same cont4/2 integer
    // divide), so bench stays 11,941,440. Standard SF conthist pattern.
    const int16_t (*ch1)[SQUARE_NB] = nullptr;
    const int16_t (*ch2)[SQUARE_NB] = nullptr;
    const int16_t (*ch4)[SQUARE_NB] = nullptr;
    if ((ss-1)->move != MOVE_NONE && (ss-1)->move != MOVE_NULL
        && (ss-1)->moved_piece != NO_PIECE_TYPE)
        ch1 = history.cont1->data[(ss-1)->moved_piece][to_sq((ss-1)->move)];
    if ((ss-2)->move != MOVE_NONE && (ss-2)->move != MOVE_NULL
        && (ss-2)->moved_piece != NO_PIECE_TYPE)
        ch2 = history.cont2->data[(ss-2)->moved_piece][to_sq((ss-2)->move)];
    if ((ss-4)->move != MOVE_NONE && (ss-4)->move != MOVE_NULL
        && (ss-4)->moved_piece != NO_PIECE_TYPE)
        ch4 = history.cont4->data[(ss-4)->moved_piece][to_sq((ss-4)->move)];

    for (int i = 0; i < n; i++) {
        Move m = moves[i].move;

        bool is_cap   = (b.piece_on(to_sq(m)) != NO_PIECE) || (move_type(m) == EN_PASSANT);
        bool is_promo = (move_type(m) == PROMOTION);

        if (is_cap) {
            PieceType atk = type_of(b.piece_on(from_sq(m)));
            PieceType cap = (move_type(m) == EN_PASSANT) ? PAWN : type_of(b.piece_on(to_sq(m)));
            moves[i].score = 6'000'000 + PIECE_VALUE[cap] * 16 - PIECE_VALUE[atk]
                                       + history.capture[atk][to_sq(m)][cap];
        } else if (is_promo) {
            moves[i].score = (promo_type(m) == QUEEN) ? 5'500'000 : -100;
        } else {
            // Quiet
            const Square from = Square(from_sq(m));
            const Square to = Square(to_sq(m));
            const PieceType pt = type_of(b.piece_on(from));
            int hist = main_hist[from][to];
            if (ch1) hist += ch1[pt][to];
            if (ch2) hist += ch2[pt][to];
            if (ch4) hist += ch4[pt][to] / 2;
            hist += pawn_hist[pt][to];
            if (low_ply_hist) hist += (*low_ply_hist)[from][to];

            if (checks_for(pt) & sq_bb(to))
                hist += 32'000;

            if      (m == ss->killers[0]) moves[i].score = 4'000'000;
            else if (m == ss->killers[1]) moves[i].score = 3'900'000;
            else if (m == cm)             moves[i].score = 3'800'000;
            else                          moves[i].score = hist;
        }

        if (root && !root->tablebase->empty())
            moves[i].score += root_tablebase_ordering_score(*root->tablebase, m);

        if (root && root->table)
            moves[i].score += root->table->ordering_score(m);
    }
}
