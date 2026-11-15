#pragma once

// The verify switch for the speed pass's picker changes (B.8.9): the picker
// as it stood before them, run beside the shipped one at every node of a
// BASILISK_VERIFY_PICKER build, which aborts when a delivered move or its
// stage differs. Built only in that build; it leaves with the pass.

#ifdef BASILISK_VERIFY_PICKER

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
#error "the picker check is a Release-flavour build: the pricing probes share its second buffer"
#endif

#include "move_picker_core.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace reference_picker {

inline void sort_reference(ScoredMove* begin, ScoredMove* end, int limit) {
    if (end - begin < 2)
        return;
    ScoredMove* sorted_end = begin;
    for (ScoredMove* p = begin + 1; p < end; ++p) {
        const ScoredMove tmp = *p;
        ScoredMove* const slot = sorted_end + 1;
        const bool above = tmp.score >= limit;
        ScoredMove* const swap_with = above ? slot : p;
        *p = *swap_with;
        *swap_with = tmp;
        sorted_end += above;
    }
    for (ScoredMove* p = begin + 1; p <= sorted_end; ++p) {
        const ScoredMove tmp = *p;
        ScoredMove* q = p;
        for (; q != begin && (q - 1)->score < tmp.score; --q)
            *q = *(q - 1);
        *q = tmp;
    }
}

class Picker {
public:
    using Src = CoreMovePicker::Src;

    Picker(const Board& board, const CoreHistoryTables& history, const PawnHistoryBucket& pawn,
           const PieceToHistory* const cont[6], Move tt_move, Move excluded, int depth, int ply,
           const RootOrdering* root, ScoredMove* buffer)
        : board_(board), history_(history), pawn_(pawn), cont_(cont), tt_move_(tt_move),
          excluded_(excluded), depth_(depth), ply_(ply), root_(root), moves_(buffer) {}

    void skip_quiets() { skip_quiets_ = true; }
    [[nodiscard]] Src last_source() const { return last_src_; }

    Move next() {
        last_src_ = Src::None;
        while (true) {
            switch (stage_) {
                case Stage::TT:
                    stage_ = Stage::CaptureInit;
                    if (tt_move_ != excluded_ && table_move_usable(board_, tt_move_)) {
                        tt_delivered_ = true;
                        last_src_ = Src::TT;
                        return tt_move_;
                    }
                    break;
                case Stage::CaptureInit: {
                    MoveList list;
                    board_.gen_legal_captures(list);
                    end_captures_ = fill(list, 0);
                    for (int i = 0; i < end_captures_; ++i) {
                        const Move m = moves_[i].move;
                        moves_[i].score = capture_order_score(board_, history_, m) + root_ordering(m);
                    }
                    sort_reference(moves_, moves_ + end_captures_, INT_MIN);
                    cur_ = end_bad_captures_ = 0;
                    stage_ = Stage::GoodCapture;
                    break;
                }
                case Stage::GoodCapture:
                    while (cur_ < end_captures_) {
                        const ScoredMove sm = moves_[cur_];
                        if (board_.see_ge(sm.move, -sm.score / KLIT(pick_good_capture_div))) {
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
                        sort_reference(moves_ + end_captures_, moves_ + end_quiets_,
                                       KLIT(pick_sort_limit) * depth_);
                    }
                    cur_ = end_captures_;
                    stage_ = Stage::GoodQuiet;
                    break;
                case Stage::GoodQuiet:
                    while (!skip_quiets_ && cur_ < end_quiets_) {
                        const ScoredMove sm = moves_[cur_++];
                        if (sm.score > KLIT(pick_good_quiet)) {
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
                        if (sm.score <= KLIT(pick_good_quiet)) {
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

private:
    enum class Stage { TT, CaptureInit, GoodCapture, QuietInit, GoodQuiet, BadCapture, BadQuiet, Done };

    int fill(const MoveList& list, int at) {
        for (Move m : list) {
            if (m == excluded_ || (tt_delivered_ && m == tt_move_))
                continue;
            moves_[at++] = {m, 0};
        }
        return at;
    }

    int root_ordering(Move m) const {
        if (!root_)
            return 0;
        int s = 0;
        if (root_->tablebase && !root_->tablebase->empty())
            s += root_tablebase_ordering_score(*root_->tablebase, m);
        if (root_->table)
            s += root_->table->ordering_score(m);
        return s;
    }

    Bitboard checks_for(PieceType pt) {
        if (pt == KING || pt == NO_PIECE_TYPE)
            return 0;
        if (!check_squares_ready_[pt]) {
            check_squares_[pt] = board_.check_squares(pt, board_.turn());
            check_squares_ready_[pt] = true;
        }
        return check_squares_[pt];
    }

    void score_quiets(int begin, int end) {
        const Color us = board_.turn();
        Bitboard threat_by_lesser[PIECE_TYPE_NB] = {};
        bool threats_known = false;
        const auto& main = history_.main.entry[us];
        const auto* low_ply = ply_ < LowPlyHistory::PLIES ? &history_.low_ply.entry[ply_] : nullptr;
        for (int i = begin; i < end; ++i) {
            const Move m = moves_[i].move;
            const Square from = Square(from_sq(m));
            const Square to = Square(to_sq(m));
            const PieceType pt = type_of(board_.piece_on(from));
            const int hp = hist_piece(us, pt);
            int v = KLIT_SCALED(2 * main[from][to], pick_main_scale)
                  + KLIT_SCALED(2 * pawn_.entry[hp][to], pick_pawn_scale)
                  + cont_[0]->entry[hp][to] + cont_[1]->entry[hp][to]
                  + KLIT_SCALED(cont_[2]->entry[hp][to] + cont_[3]->entry[hp][to]
                                    + cont_[5]->entry[hp][to], pick_cont_far_scale);
            if ((checks_for(pt) & sq_bb(to)) && board_.see_ge(m, -75))
                v += KLIT(pick_check_bonus);
            if (pt != KING && pt != PAWN) {
                if (!threats_known) {
                    const Threats threats = threats_against(board_, us);
                    threat_by_lesser[KNIGHT] = threat_by_lesser[BISHOP] = threats.by_pawn;
                    threat_by_lesser[ROOK] = threats.by_minor;
                    threat_by_lesser[QUEEN] = threats.by_rook;
                    threats_known = true;
                }
                v += KLIT_SCALED(HIST_PIECE_VALUE[pt] * 20, pick_threat_scale)
                   * (int((threat_by_lesser[pt] & sq_bb(from)) != 0)
                      - int((threat_by_lesser[pt] & sq_bb(to)) != 0));
            }
            if (low_ply)
                v += KLIT_SCALED(8 * (*low_ply)[from][to], pick_low_ply_scale) / (1 + ply_);
            moves_[i].score = v + root_ordering(m);
        }
    }

    const Board& board_;
    const CoreHistoryTables& history_;
    const PawnHistoryBucket& pawn_;
    const PieceToHistory* const* cont_;
    Move tt_move_;
    Move excluded_;
    int depth_;
    int ply_;
    const RootOrdering* root_;
    ScoredMove* moves_;
    Stage stage_ = Stage::TT;
    bool tt_delivered_ = false;
    bool skip_quiets_ = false;
    Bitboard check_squares_[PIECE_TYPE_NB] = {};
    bool check_squares_ready_[PIECE_TYPE_NB] = {};
    int cur_ = 0;
    int end_bad_captures_ = 0;
    int end_captures_ = 0;
    int end_quiets_ = 0;
    Src last_src_ = Src::None;
};

}  // namespace reference_picker

// The shipped picker with the reference beside it, on a second buffer: every
// move either delivers, and the stage it came from, must agree.
class VerifiedMovePicker {
public:
    using Src = CoreMovePicker::Src;

    VerifiedMovePicker(const Board& board, const CoreHistoryTables& history,
                       const PawnHistoryBucket& pawn, const PieceToHistory* const cont[6],
                       Move tt_move, Move excluded, int depth, int ply, const RootOrdering* root,
                       ScoredMove* buffer, ScoredMove* reference_buffer)
        : picker_(board, history, pawn, cont, tt_move, excluded, depth, ply, root, buffer),
          reference_(board, history, pawn, cont, tt_move, excluded, depth, ply, root, reference_buffer) {}

    Move next() {
        const Move m = picker_.next();
        const Move want = reference_.next();
        if (m != want || picker_.last_source() != reference_.last_source()) {
            std::fprintf(stderr, "picker verify failed: delivered %d (source %d), reference %d (source %d)\n",
                         int(m), int(picker_.last_source()), int(want), int(reference_.last_source()));
            std::abort();
        }
        return m;
    }
    void skip_quiets() {
        picker_.skip_quiets();
        reference_.skip_quiets();
    }
    [[nodiscard]] bool quiets_skipped() const { return picker_.quiets_skipped(); }
    [[nodiscard]] Src last_source() const { return picker_.last_source(); }

private:
    CoreMovePicker picker_;
    reference_picker::Picker reference_;
};

#endif  // BASILISK_VERIFY_PICKER
