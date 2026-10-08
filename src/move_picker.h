#pragma once

// Move ordering: the staged picker (TT move, good tacticals, quiets, bad
// tacticals) and the scores it orders by. Its inputs are explicit and
// read-only; the move buffers belong to the caller, so picking allocates
// nothing.

#include "board.h"
#include "history.h"
#include "search_root.h"
#include "search_types.h"
#include "syzygy.h"
#include <utility>
#include <vector>

inline constexpr int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};

struct ScoredMove { Move move; int score; };

// The root's ordering inputs: the tablebase ranking (empty without one) and
// the pool's shared root table (null at one thread).
struct RootOrdering {
    const std::vector<Syzygy::RootMoveInfo>* tablebase = nullptr;
    const RootMoveTable* table = nullptr;
};

// Scores `n` moves of the node at `ss`. `root` is null below the root.
void score_moves(ScoredMove* moves, int n, const Board& b, const HistoryTables& history,
                 const SearchStack* ss, int ply, const RootOrdering* root);

// Selects the best-scored move of [idx, n) into slot idx and returns it.
inline Move pick_next(ScoredMove* moves, int idx, int n) {
    ScoredMove* const first = moves + idx;
    ScoredMove* best = first;
    // 8.7.6(d): keep the running best SCORE in a register instead of reloading
    // best->score on every comparison. Selection order is unchanged (strict >,
    // first-wins on ties), so bench stays identical.
    int best_score = first->score;
    for (ScoredMove* it = first + 1, *end = moves + n; it != end; ++it)
        if (it->score > best_score) {
            best = it;
            best_score = it->score;
        }

    if (best != first)
        std::swap(*first, *best);
    return first->move;
}

class MovePicker {
public:
    // `root` is the root's ordering inputs, null below the root. The buffers
    // belong to the caller and hold every move of one node.
    MovePicker(const Board& board, const HistoryTables& history, Move tt_move, Move excluded,
               const SearchStack* ss, int ply, const RootOrdering* root,
               ScoredMove* tactical_buffer, ScoredMove* bad_buffer)
        : board_(board)
        , history_(history)
        , tt_move_(tt_move)
        , excluded_(excluded)
        , ss_(ss)
        , root_(root)
        , ply_(ply)
        , scored_(tactical_buffer)
        , bad_(bad_buffer) {}

    Move next() {
        last_see_ = VALUE_NONE;   // 8.7.5(a): reset the per-move SEE verdict
        last_src_ = Src::None;    // 5.2: reset the per-move picker source
        while (true) {
            switch (stage_) {
                case Stage::TT:
                    stage_ = Stage::TacticalsInit;
                    if (tt_move_ != MOVE_NONE && tt_move_ != excluded_) {
                        Piece p = board_.piece_on(from_sq(tt_move_));
                        if (p != NO_PIECE
                            && color_of(p) == board_.turn()
                            && board_.is_legal(tt_move_)) {
                            tt_searched_ = true;
                            last_src_ = Src::TT;
                            return tt_move_;
                        }
                    }
                    break;

                case Stage::TacticalsInit:
                    fill_tacticals();
                    stage_ = Stage::GoodTacticals;
                    break;

                case Stage::GoodTacticals:
                    while (idx_ < n_) {
                        Move move = pick_next(scored_, idx_++, n_);
                        if (is_bad_tactical(move)) {
                            bad_[bad_count_++] = {move, scored_[idx_ - 1].score};
                            continue;
                        }
                        // 8.7.5(a): a good tactical that is a non-promo capture
                        // passed is_bad_tactical == false, i.e. see_ge(m,0) was
                        // TRUE — memoize see_score = 0 so search_one need not
                        // recompute the identical see_ge. Promotions carry no
                        // SEE verdict (search skips SEE for them).
                        last_see_ = is_nonpromo_capture(move) ? 0 : VALUE_NONE;
                        last_src_ = Src::GoodTactical;
                        return move;
                    }
                    stage_ = Stage::QuietsInit;
                    break;

                case Stage::QuietsInit:
                    fill_quiets();
                    stage_ = Stage::Quiets;
                    break;

                case Stage::Quiets:
                    if (idx_ < n_) {
                        last_src_ = Src::Quiet;
                        return pick_next(scored_, idx_++, n_);
                    }
                    stage_ = Stage::BadTacticals;
                    bad_idx_ = 0;
                    break;

                case Stage::BadTacticals:
                    if (bad_idx_ < bad_count_) {
                        // 8.7.5(a): the bad-tactical buffer holds only non-promo
                        // captures with see_ge(m,0) == FALSE → see_score = -1.
                        last_see_ = -1;
                        last_src_ = Src::BadTactical;
                        return pick_next(bad_, bad_idx_++, bad_count_);
                    }
                    stage_ = Stage::Done;
                    break;

                case Stage::Done:
                    return MOVE_NONE;
            }
        }
    }

private:
    enum class Stage {
        TT,
        TacticalsInit,
        GoodTacticals,
        QuietsInit,
        Quiets,
        BadTacticals,
        Done
    };

    void fill_tacticals() {
        MoveList moves;
        board_.gen_legal_captures(moves);
        fill_from(moves);
    }

    void fill_quiets() {
        MoveList moves;
        board_.gen_legal_quiets(moves);
        fill_from(moves);
    }

    void fill_from(const MoveList& moves) {
        n_ = 0;
        idx_ = 0;
        for (Move move : moves) {
            if (move == excluded_ || (tt_searched_ && move == tt_move_))
                continue;
            scored_[n_++] = {move, 0};
        }
        score_moves(scored_, n_, board_, history_, ss_, ply_, root_);
    }

    bool is_bad_tactical(Move move) const {
        if (move_type(move) == PROMOTION)
            return false;
        const Board& board = board_;
        const bool is_cap = board.piece_on(to_sq(move)) != NO_PIECE || move_type(move) == EN_PASSANT;
        return is_cap && !board.see_ge(move, 0);
    }

    // 8.7.5(a): matches search_one's `is_cap && !is_promo` — the exact class
    // for which see_score = see_ge(m,0)?0:-1 is computed downstream.
    bool is_nonpromo_capture(Move move) const {
        if (move_type(move) == PROMOTION)
            return false;
        const Board& board = board_;
        return board.piece_on(to_sq(move)) != NO_PIECE || move_type(move) == EN_PASSANT;
    }

public:
    // The SEE verdict for the move next() just returned: 0 (good capture),
    // -1 (bad capture), or VALUE_NONE (TT move / promo / quiet / not a capture)
    // — lets search_one skip recomputing the identical see_ge(m,0).
    int last_see_score() const { return last_see_; }
    // 5.2: which stage produced the move just returned. Recorded at each
    // return rather than read from stage_, because the TT and tactical
    // stages advance stage_ before returning.
    enum class Src { None, TT, GoodTactical, Quiet, BadTactical };
    Src last_source() const { return last_src_; }
private:

    const Board& board_;
    const HistoryTables& history_;
    Move tt_move_;
    Move excluded_;
    const SearchStack* ss_;
    const RootOrdering* root_;
    int ply_;
    bool tt_searched_ = false;
    Stage stage_ = Stage::TT;
    ScoredMove* scored_;
    ScoredMove* bad_;
    int n_ = 0;
    int idx_ = 0;
    int bad_count_ = 0;
    int bad_idx_ = 0;
    int last_see_ = VALUE_NONE;   // 8.7.5(a): SEE verdict of the last move returned
    Src last_src_ = Src::None;    // 5.2: picker stage of the last move returned
};
