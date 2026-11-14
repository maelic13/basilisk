#pragma once

// The root move table shared by a thread pool, root-move filters, the record
// one thread keeps of its root moves, and the sanitation every published result
// passes through: a best move and ponder move are always legal from the root.

#include "board.h"
#include "search_types.h"
#include "syzygy.h"
#include <cstdint>
#include <mutex>
#include <vector>

// Whether `move` may be searched under a `searchmoves` restriction (empty
// means every move).
[[nodiscard]] bool move_in_root_moves(Move move, const std::vector<Move>& root_moves);
[[nodiscard]] bool is_legal_move_on_board(const Board& board, Move move);
[[nodiscard]] Move first_legal_move(const Board& board);
// Root ordering bonus from the tablebase ranking: every ranked move sorts
// above any unranked one, best rank first.
[[nodiscard]] int root_tablebase_ordering_score(const std::vector<Syzygy::RootMoveInfo>& root_moves,
                                                Move move);

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

// "No score yet" in a root move's record. It sorts below every searched move.
inline constexpr int kRootScoreNone = -INF_SCORE;
inline constexpr int64_t kRootMeanSqNone = -int64_t(INF_SCORE) * INF_SCORE;

// One thread's record of a root move the limits allow, sized at the root once
// per `go`. The kernel's root loop writes it after each search of the move; the
// driver reads it between searches. `score` is the last search's when the move
// was searched first or raised alpha, otherwise "none"; `average` and `mean_sq`
// run over every search of the move, whatever it proved; `effort` is the nodes
// those searches took. `tb_rank` is the tablebase ordering score (0 without one).
struct RootMove {
    Move move = MOVE_NONE;
    int score = kRootScoreNone;
    int previous_score = kRootScoreNone;
    int average = kRootScoreNone;
    int64_t mean_sq = kRootMeanSqNone;
    int64_t effort = 0;
    int sel_depth = 0;
    int tb_rank = 0;
    bool lower_bound = false;
    bool upper_bound = false;
    std::vector<Move> pv;
    std::vector<Move> previous_pv;
};

// The root's ordering inputs: the tablebase ranking (empty without one) and
// the pool's shared root table (null at one thread).
struct RootOrdering {
    const std::vector<Syzygy::RootMoveInfo>* tablebase = nullptr;
    const RootMoveTable* table = nullptr;
};

[[nodiscard]] SearchResult sanitize_search_result(const Board& root_board, SearchResult result);
