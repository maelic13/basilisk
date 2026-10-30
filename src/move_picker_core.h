#pragma once

// The selectivity core's staged move picker:
//
//   1. the TT move, when it is a legal move of the side to move;
//   2. good captures: the capture stage (captures and every promotion) scored
//      by capture history plus seven times the captured value, best first,
//      each delivered if its SEE clears -score/18 and set aside otherwise;
//   3. good quiets: quiets scored from the histories, checks and threats,
//      sorted above -3560 * depth, delivered while above -14000;
//   4. the captures set aside;
//   5. the remaining quiets.
//
// skip_quiets() ends quiet delivery: no quiet is delivered after it, while the
// captures set aside still are; with `keep_direct_checks`, a quiet whose
// moved piece gives check from its destination still is. In check the same
// stages deliver the legal
// evasions. All moves of one node live in the caller's 256-slot buffer,
// partitioned by index: [0, end_bad_captures) the captures set aside,
// [end_bad_captures, end_captures) the captures being scanned, and
// [end_captures, end_quiets) the quiets. Picking allocates nothing.

#include "board.h"
#include "history_core.h"
#include "search_root.h"
#include "search_types.h"
#include "threats.h"

#include <utility>

// Material in history units: the value a sum of histories adds for a piece
// (a capture's victim, a threatened piece). The histories keep the donor's
// limits, so the material added into them is the donor's piece value too;
// SEE, futility and the evaluation keep the search's own PIECE_VALUE.
inline constexpr int HIST_PIECE_VALUE[PIECE_TYPE_NB] = {0, 208, 781, 825, 1276, 2538, 0};

class CoreMovePicker {
public:
    // The stage that produced the move next() just returned.
    enum class Src { None, TT, GoodTactical, Quiet, BadTactical };

    // `cont[k]` is the continuation row selected by the move k + 1 plies
    // back. `root` is the root's ordering inputs, null below the root.
    CoreMovePicker(const Board& board, const CoreHistoryTables& history,
                   const PawnHistoryBucket& pawn, const PieceToHistory* const cont[6],
                   Move tt_move, Move excluded, int depth, int ply,
                   const RootOrdering* root, ScoredMove* buffer)
        : board_(board), history_(history), pawn_(pawn), cont_(cont),
          tt_move_(tt_move), excluded_(excluded), depth_(depth), ply_(ply),
          root_(root), moves_(buffer) {}

    Move next();
    void skip_quiets(bool keep_direct_checks = false) {
        skip_quiets_ = true;
        keep_direct_checks_ = keep_direct_checks;
    }
    [[nodiscard]] bool quiets_skipped() const { return skip_quiets_; }
    [[nodiscard]] Src last_source() const { return last_src_; }

    static constexpr int GOOD_QUIET_THRESHOLD = -14000;

private:
    enum class Stage { TT, CaptureInit, GoodCapture, QuietInit, GoodQuiet, BadCapture, BadQuiet, Done };

    // Copies `list` into the buffer from `at`, leaving out the excluded move
    // and a TT move already delivered; returns the new end.
    int  fill(const MoveList& list, int at);
    void score_captures(int begin, int end);
    void score_quiets(int begin, int end);
    int  root_ordering(Move m) const;
    // The squares from which a piece of this type gives check (none for the
    // king), computed once per type per node.
    Bitboard checks_for(PieceType pt);
    bool direct_check(Move m);
    // Whether a quiet is delivered at all in the current skip state.
    bool deliverable_quiet(Move m) { return !skip_quiets_ || (keep_direct_checks_ && direct_check(m)); }

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
    bool keep_direct_checks_ = false;
    Bitboard check_squares_[PIECE_TYPE_NB] = {};
    bool check_squares_ready_[PIECE_TYPE_NB] = {};
    int cur_ = 0;
    int end_bad_captures_ = 0;
    int end_captures_ = 0;
    int end_quiets_ = 0;
    Src last_src_ = Src::None;
};

// A capture-stage move's ordering score: its capture history plus seven
// times the captured value (in history units).
int capture_order_score(const Board& board, const CoreHistoryTables& history, Move m);

// ProbCut's candidates: the TT move when it is a capture or a queen
// promotion, then the captures and queen promotions in the capture stage's
// order, each delivered only if its SEE clears `threshold`; never the
// excluded move, never an underpromotion. The TT move is validated as the
// main picker validates it. Picking allocates nothing.
class ProbCutPicker {
public:
    ProbCutPicker(const Board& board, const CoreHistoryTables& history, Move tt_move,
                  Move excluded, int threshold, ScoredMove* buffer)
        : board_(board), history_(history), tt_move_(tt_move), excluded_(excluded),
          threshold_(threshold), moves_(buffer) {}

    Move next();

private:
    enum class Stage { TT, Init, Captures, Done };
    // A capture or a queen promotion: what this picker delivers.
    bool candidate(Move m) const;

    const Board& board_;
    const CoreHistoryTables& history_;
    Move tt_move_;
    Move excluded_;
    int threshold_;
    ScoredMove* moves_;
    Stage stage_ = Stage::TT;
    bool tt_delivered_ = false;
    int cur_ = 0;
    int end_ = 0;
};

// Sorts [begin, end) in descending score order up to and including `limit`;
// the order of the moves below it is left unspecified.
void partial_insertion_sort(ScoredMove* begin, ScoredMove* end, int limit);

// Selects the best-scored move of [idx, n) into slot idx and returns it, ties
// to the first (quiescence's capture loop).
inline Move pick_next(ScoredMove* moves, int idx, int n) {
    ScoredMove* const first = moves + idx;
    ScoredMove* best = first;
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
