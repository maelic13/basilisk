#pragma once

#include "Board.h"
#include "tt.h"
#include "eval.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <array>

static constexpr int MAX_SEARCH_DEPTH = 100;
static constexpr int MAX_PLY          = 128;
static constexpr int MATE_SCORE       = 32000;
static constexpr int INF_SCORE        = 32001;

struct SearchLimits {
    int depth      = MAX_SEARCH_DEPTH;
    int movetime   = 0;
    int wtime      = 0, btime      = 0;
    int winc       = 0, binc       = 0;
    int movestogo  = 0;
    int64_t nodes  = 0;
    bool infinite  = false;
    bool ponder    = false;
};

struct SearchResult {
    Move bestmove   = MOVE_NONE;
    Move pondermove = MOVE_NONE;
    int  score      = 0;
    int  depth      = 0;
};

class Searcher {
public:
    Searcher(TranspositionTable& tt,
             std::atomic_bool& stop_flag,
             std::function<void(const std::string&)> info_cb = nullptr);

    SearchResult search(Board board, const SearchLimits& limits);

    Evaluator evaluator;

private:
    TranspositionTable& tt_;
    std::atomic_bool&   stop_;
    std::function<void(const std::string&)> info_cb_;

    // Per-search state
    Board*   board_ptr_;
    int64_t  nodes_;
    int      sel_depth_;
    bool     stopped_;

    // Move ordering tables (persist across searches)
    Move killers_[MAX_PLY][2];
    int  history_[NCOLORS][SQUARE_NB][SQUARE_NB];
    Move countermove_[SQUARE_NB][SQUARE_NB];

    // Per-ply eval stack (for improving heuristic)
    int eval_stack_[MAX_PLY];

    // Triangular PV table
    Move pv_table_[MAX_PLY][MAX_PLY];
    int  pv_len_[MAX_PLY];

    // Time management
    std::chrono::steady_clock::time_point start_time_;
    double time_limit_; // seconds, 0 = unlimited

    // LMR table
    static int   LMR_TABLE[64][64];
    static bool  lmr_init_;
    static void  init_lmr();

    // Search helpers
    int  negamax(int depth, int alpha, int beta, int ply, bool is_pv, bool allow_null, Move prev_move);
    int  quiescence(int alpha, int beta, int ply);

    // Move ordering
    struct ScoredMove { Move move; int score; };
    void score_moves(ScoredMove* moves, int n, Move tt_move, int ply, Move prev_move) const;
    static Move pick_next(ScoredMove* moves, int idx, int n);

    bool   check_stop();
    double elapsed_seconds() const;
    void   compute_time_limit(const SearchLimits& limits, Color side);
    void   send_info(int depth, int score, int64_t total_nodes, double elapsed) const;

    void update_history(Color c, Square from, Square to, int bonus);
    void history_bonus(Color c, Square from, Square to, int depth);
    void history_malus(Color c, Square from, Square to, int depth);
    void age_history();

    static constexpr int MAX_HISTORY = 8192;
};
