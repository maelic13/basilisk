#pragma once

#include "Board.h"
#include "tt.h"
#include "eval.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

static constexpr int MAX_SEARCH_DEPTH = 100;
static constexpr int MAX_PLY          = 128;
static constexpr int MATE_SCORE       = 32000;
static constexpr int INF_SCORE        = 32001;
static constexpr int VALUE_NONE       = 32002;

// Per-ply search stack (Stockfish-style).
// Root is at ss[0]; ss[-1]..ss[-4] are sentinel slots pre-filled with MOVE_NONE.
struct SearchStack {
    Move      move        = MOVE_NONE;       // move being searched at this ply
    Move      excluded    = MOVE_NONE;       // excluded move (singular extensions)
    Move      killers[2]  = {};              // killer moves
    int       eval        = VALUE_NONE;      // static eval at this ply
    PieceType moved_piece = NO_PIECE_TYPE;   // piece type that made 'move'
};

struct SearchLimits {
    int depth      = MAX_SEARCH_DEPTH;
    int movetime   = 0;
    int wtime      = 0, btime = 0;
    int winc       = 0, binc  = 0;
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
    void clear(); // Reset all history (e.g., on ucinewgame)

    Evaluator evaluator;

private:
    TranspositionTable& tt_;
    std::atomic_bool&   stop_;
    std::function<void(const std::string&)> info_cb_;

    Board*   board_ptr_;
    int64_t  nodes_;
    int      sel_depth_;
    bool     stopped_;

    // ---- History tables (persist across searches; aged each search) ----
    static constexpr int MAX_MAIN_HIST = 16384;
    static constexpr int MAX_CAP_HIST  = 16384;
    static constexpr int MAX_CONT_HIST = 16384;
    static constexpr int CORR_SIZE     = 16384;

    // Quiet history [color][from][to]
    int16_t main_hist_[NCOLORS][SQUARE_NB][SQUARE_NB];

    // Capture history [attacker_pt][to][captured_pt]
    int16_t cap_hist_[PIECE_TYPE_NB][SQUARE_NB][PIECE_TYPE_NB];

    // Continuation history: heap-allocated (~400 KB each)
    struct ContHistTable {
        int16_t data[PIECE_TYPE_NB][SQUARE_NB][PIECE_TYPE_NB][SQUARE_NB];
    };
    std::unique_ptr<ContHistTable> cont_hist1_; // 1-ply continuation
    std::unique_ptr<ContHistTable> cont_hist2_; // 2-ply continuation

    // Countermove [from][to] -> best response
    Move     countermove_[SQUARE_NB][SQUARE_NB];

    // Pawn-structure correction history [color][pawn_key & mask]
    int16_t  corr_hist_[NCOLORS][CORR_SIZE];

    // ---- Per-search state ----
    // ss_arr_[0..3] = sentinels; root = ss_arr_[4] (ply 0)
    SearchStack ss_arr_[MAX_PLY + 8];

    Move pv_table_[MAX_PLY][MAX_PLY];
    int  pv_len_[MAX_PLY];

    std::chrono::steady_clock::time_point start_time_;
    double time_limit_;

    // ---- LMR table ----
    static int  LMR_TABLE[64][64];
    static bool lmr_init_;
    static void init_lmr();

    // ---- Search ----
    static constexpr int MAX_QSEARCH_PLY = 10; // max extra plies of captures in qsearch
    int negamax(int depth, int alpha, int beta, int ply,
                SearchStack* ss, bool is_pv, bool allow_null);
    int quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss);

    // ---- Move ordering ----
    struct ScoredMove { Move move; int score; };
    void  score_moves(ScoredMove* moves, int n, Move tt_move, SearchStack* ss) const;
    static Move pick_next(ScoredMove* moves, int idx, int n);

    // ---- History helpers ----
    template<int MAX_VAL>
    static void hist_update(int16_t& e, int bonus) {
        e += static_cast<int16_t>(bonus - static_cast<int>(e) * std::abs(bonus) / MAX_VAL);
    }

    void update_quiet(Color stm, Square from, Square to, int bonus);
    void update_cap(PieceType pt, Square to, PieceType cap, int bonus);
    void update_cont(ContHistTable& tbl,
                     PieceType ppt, Square pto,
                     PieceType cpt, Square cto, int bonus);

    // Combined 1+2-ply continuation history score for a (piece, to) pair
    int  cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const;

    // Bulk history update after a beta cutoff
    void update_all_histories(Move best,
                              const std::vector<Move>& quiets,
                              const std::vector<Move>& bad_caps,
                              Color stm, int depth, SearchStack* ss);

    // Correction history
    void update_correction(Color stm, Key pawn_key, int diff, int depth);
    int  correction_value(Color stm, Key pawn_key) const;

    void age_history();

    // ---- Misc ----
    bool   check_stop();
    double elapsed_seconds() const;
    void   compute_time_limit(const SearchLimits& limits, Color side);
    void   send_info(int depth, int score, int64_t nodes, double elapsed) const;
};
