#include "move_picker_core.h"

#include <climits>
#include <utility>

void partial_insertion_sort(ScoredMove* begin, ScoredMove* end, int limit) {
    if (end - begin < 2)
        return;
    for (ScoredMove *sorted_end = begin, *p = begin + 1; p < end; ++p)
        if (p->score >= limit) {
            const ScoredMove tmp = *p;
            *p = *++sorted_end;
            ScoredMove* q = sorted_end;
            for (; q != begin && (q - 1)->score < tmp.score; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

int CoreMovePicker::root_ordering(Move m) const {
    if (!root_)
        return 0;
    int s = 0;
    if (root_->tablebase && !root_->tablebase->empty())
        s += root_tablebase_ordering_score(*root_->tablebase, m);
    if (root_->table)
        s += root_->table->ordering_score(m);
    return s;
}

int CoreMovePicker::fill(const MoveList& list, int at) {
    for (Move m : list) {
        if (m == excluded_ || (tt_delivered_ && m == tt_move_))
            continue;
        moves_[at++] = {m, 0};
    }
    return at;
}

void CoreMovePicker::score_captures(int begin, int end) {
    for (int i = begin; i < end; ++i) {
        const Move m = moves_[i].move;
        const Square to = Square(to_sq(m));
        const PieceType attacker = type_of(board_.piece_on(from_sq(m)));
        const PieceType captured = move_type(m) == EN_PASSANT ? PAWN
                                 : type_of(board_.piece_on(to));
        moves_[i].score = history_.capture.entry[attacker][to][captured]
                        + 7 * PIECE_VALUE[captured] + root_ordering(m);
    }
}

void CoreMovePicker::score_quiets(int begin, int end) {
    const Color us = board_.turn();

    // A quiet move away from a square a cheaper enemy piece attacks escapes a
    // threat; one onto such a square walks into it. Pawns and kings have no
    // cheaper attacker that matters here.
    const Threats threats = threats_against(board_, us);
    const Bitboard threat_by_lesser[PIECE_TYPE_NB] = {
        0, 0, threats.by_pawn, threats.by_pawn, threats.by_minor, threats.by_rook, 0};

    Bitboard check_squares[PIECE_TYPE_NB] = {};
    bool check_squares_ready[PIECE_TYPE_NB] = {};
    auto checks_for = [&](PieceType pt) {
        if (pt == KING)
            return Bitboard(0);
        if (!check_squares_ready[pt]) {
            check_squares[pt] = board_.check_squares(pt, us);
            check_squares_ready[pt] = true;
        }
        return check_squares[pt];
    };

    const auto& main = history_.main.entry[us];
    const auto* low_ply = ply_ < LowPlyHistory::PLIES ? &history_.low_ply.entry[ply_] : nullptr;

    for (int i = begin; i < end; ++i) {
        const Move m = moves_[i].move;
        const Square from = Square(from_sq(m));
        const Square to   = Square(to_sq(m));
        const PieceType pt = type_of(board_.piece_on(from));

        int v = 2 * main[from][to] + 2 * pawn_.entry[pt][to]
              + cont_[0]->entry[pt][to] + cont_[1]->entry[pt][to] + cont_[2]->entry[pt][to]
              + cont_[3]->entry[pt][to] + cont_[5]->entry[pt][to];

        if ((checks_for(pt) & sq_bb(to)) && board_.see_ge(m, -75))
            v += 16384;

        v += PIECE_VALUE[pt] * 20
           * (int((threat_by_lesser[pt] & sq_bb(from)) != 0)
              - int((threat_by_lesser[pt] & sq_bb(to)) != 0));

        if (low_ply)
            v += 8 * (*low_ply)[from][to] / (1 + ply_);

        moves_[i].score = v + root_ordering(m);
    }
}

Move CoreMovePicker::next() {
    last_src_ = Src::None;
    while (true) {
        switch (stage_) {
            case Stage::TT:
                stage_ = Stage::CaptureInit;
                if (tt_move_ != MOVE_NONE && tt_move_ != excluded_) {
                    const Piece p = board_.piece_on(from_sq(tt_move_));
                    if (p != NO_PIECE && color_of(p) == board_.turn() && board_.is_legal(tt_move_)) {
                        tt_delivered_ = true;
                        last_src_ = Src::TT;
                        return tt_move_;
                    }
                }
                break;

            case Stage::CaptureInit: {
                MoveList list;
                board_.gen_legal_captures(list);
                end_captures_ = fill(list, 0);
                score_captures(0, end_captures_);
                partial_insertion_sort(moves_, moves_ + end_captures_, INT_MIN);
                cur_ = end_bad_captures_ = 0;
                stage_ = Stage::GoodCapture;
                break;
            }

            case Stage::GoodCapture:
                while (cur_ < end_captures_) {
                    const ScoredMove sm = moves_[cur_];
                    if (board_.see_ge(sm.move, -sm.score / 18)) {
                        ++cur_;
                        last_src_ = Src::GoodTactical;
                        return sm.move;
                    }
                    std::swap(moves_[end_bad_captures_++], moves_[cur_]);
                    ++cur_;
                }
                stage_ = Stage::QuietInit;
                break;

            case Stage::QuietInit:
                end_quiets_ = end_captures_;
                if (!skip_quiets_) {
                    MoveList list;
                    board_.gen_legal_quiets(list);
                    end_quiets_ = fill(list, end_captures_);
                    score_quiets(end_captures_, end_quiets_);
                    partial_insertion_sort(moves_ + end_captures_, moves_ + end_quiets_,
                                           -3560 * depth_);
                }
                cur_ = end_captures_;
                stage_ = Stage::GoodQuiet;
                break;

            case Stage::GoodQuiet:
                while (!skip_quiets_ && cur_ < end_quiets_) {
                    const ScoredMove sm = moves_[cur_++];
                    if (sm.score > GOOD_QUIET_THRESHOLD) {
                        last_src_ = Src::Quiet;
                        return sm.move;
                    }
                }
                cur_ = 0;
                stage_ = Stage::BadCapture;
                break;

            case Stage::BadCapture:
                if (cur_ < end_bad_captures_) {
                    last_src_ = Src::BadTactical;
                    return moves_[cur_++].move;
                }
                cur_ = end_captures_;
                stage_ = Stage::BadQuiet;
                break;

            case Stage::BadQuiet:
                while (!skip_quiets_ && cur_ < end_quiets_) {
                    const ScoredMove sm = moves_[cur_++];
                    if (sm.score <= GOOD_QUIET_THRESHOLD) {
                        last_src_ = Src::Quiet;
                        return sm.move;
                    }
                }
                stage_ = Stage::Done;
                break;

            case Stage::Done:
                return MOVE_NONE;
        }
    }
}
