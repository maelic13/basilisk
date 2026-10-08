#pragma once

// The root move table shared by a thread pool, root-move filters, and the
// sanitation every published result passes through: a best move and ponder
// move are always legal from the root.

#include "board.h"
#include "search_types.h"
#include "syzygy.h"
#include <mutex>
#include <vector>

// Whether `move` may be searched under a `searchmoves` restriction (empty
// means every move).
[[nodiscard]] bool move_in_root_moves(Move move, const std::vector<Move>& root_moves);
[[nodiscard]] bool is_legal_move_on_board(const Board& board, Move move);
[[nodiscard]] Move first_legal_move(const Board& board);

class RootMoveTable {
public:
    void reset(const Board& board,
               const std::vector<Move>& root_moves = {},
               const std::vector<Syzygy::RootMoveInfo>& syzygy_root_moves = {});
    void update(Move bestmove, Move pondermove, int depth, int score);
    bool contains(Move move) const;
    Move fallback_move() const;
    int  ordering_score(Move move) const;
    SearchResult best_result() const;

private:
    struct Entry {
        Move bestmove   = MOVE_NONE;
        Move pondermove = MOVE_NONE;
        int  depth      = 0;
        int  score      = -INF_SCORE;
        int  sequence   = 0;
    };

    mutable std::mutex mutex_;
    std::vector<Entry> entries_;
    int sequence_ = 0;
};

[[nodiscard]] SearchResult sanitize_search_result(const Board& root_board, SearchResult result);
