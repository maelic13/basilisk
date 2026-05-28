#pragma once

#include "Board.h"
#include "tt.h"
#include "eval.h"
#include "syzygy.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

static constexpr int MAX_SEARCH_DEPTH = 100;
static constexpr int MAX_PLY          = 128;
static constexpr int MATE_SCORE       = 32000;
static constexpr int INF_SCORE        = 32001;
static constexpr int VALUE_NONE       = 32002;

// Per-ply search stack used by alpha-beta/PVS search.
// Root is at ss[0]; ss[-1]..ss[-4] are sentinel slots pre-filled with MOVE_NONE.
struct SearchStack {
    Move      move        = MOVE_NONE;       // move being searched at this ply
    Move      excluded    = MOVE_NONE;       // excluded move (singular extensions)
    Move      killers[2]  = {};              // killer moves
    int       eval        = VALUE_NONE;      // static eval at this ply
    int       stat_score  = 0;               // combined history score for this move
    int       reduction   = 0;               // LMR reduction applied by parent
    PieceType moved_piece = NO_PIECE_TYPE;   // piece type that made 'move'
    bool      tt_pv       = false;           // node lies near a TT/PV line
};

struct SearchResult;

class RootMoveTable {
public:
    void reset(const Board& board);
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

struct SearchLimits {
    int depth      = MAX_SEARCH_DEPTH;
    int movetime   = 0;
    int wtime      = 0, btime = 0;
    int winc       = 0, binc  = 0;
    int movestogo  = 0;
    int64_t nodes  = 0;
    int overhead   = 0;   // move overhead to subtract [ms]
    bool infinite  = false;
    bool ponder    = false;
    bool update_tt_age = true;
    int root_filter_index = -1; // -1 = search all root moves
    int root_filter_count = 1;
    int thread_id = 0;
    int thread_count = 1;
    int syzygy_probe_depth = 0; // 0 = disabled
    int syzygy_probe_limit = 0;
    bool syzygy_50_move_rule = true;
    std::vector<Syzygy::RootMoveInfo> syzygy_root_moves;
    RootMoveTable* root_table = nullptr;
};

struct SearchResult {
    Move    bestmove   = MOVE_NONE;
    Move    pondermove = MOVE_NONE;
    int     score      = 0;
    int     depth      = 0;
    int64_t nodes      = 0;
    int64_t tbhits     = 0;
    int64_t elapsed_ms = 0;
};

SearchResult sanitize_search_result(const Board& root_board, SearchResult result);

class Searcher {
public:
    Searcher(TranspositionTable& tt,
             std::atomic_bool& stop_flag,
             std::function<void(const std::string&)> info_cb = nullptr,
             std::atomic_bool* ponderhit_flag = nullptr);

    SearchResult search(Board board, const SearchLimits& limits);
    void clear(); // Reset all history (e.g., on ucinewgame)
    void blend_history_from(const Searcher& other);

private:
    Evaluator evaluator;

private:
    TranspositionTable& tt_;
    std::atomic_bool&   stop_;
    std::atomic_bool*   ponderhit_;
    std::function<void(const std::string&)> info_cb_;

    Board*   board_ptr_;
    int64_t  nodes_;
    int64_t  tb_hits_;
    int64_t  nodes_limit_;  // 0 = unlimited
    int      sel_depth_;
    bool     stopped_;
    int      root_filter_index_;
    int      root_filter_count_;
    int      thread_id_;
    RootMoveTable* root_table_;
    bool     pondering_;
    SearchLimits active_limits_;
    Color    root_side_;
    std::vector<Syzygy::RootMoveInfo> root_tb_moves_;
    int64_t  root_depth_nodes_;
    int64_t  root_best_nodes_;
    int      root_best_effort_;

    // ---- History tables (persist across searches; aged each search) ----
    static constexpr int MAX_MAIN_HIST = 16384;
    static constexpr int MAX_CAP_HIST  = 16384;
    static constexpr int MAX_CONT_HIST = 16384;
    static constexpr int MAX_PAWN_HIST = 16384;
    static constexpr int MAX_LOW_HIST  = 8192;
    static constexpr int CORR_SIZE     = 16384;
    static constexpr int PAWN_HIST_SIZE = 2048;
    static constexpr int LOW_PLY_HISTORY_SIZE = 8;

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
    std::unique_ptr<ContHistTable> cont_hist4_; // 4-ply continuation

    // Pawn-structure keyed quiet history [pawn_key][piece][to]
    struct PawnHistTable {
        int16_t data[PAWN_HIST_SIZE][PIECE_TYPE_NB][SQUARE_NB];
    };
    std::unique_ptr<PawnHistTable> pawn_hist_;

    // Low-ply quiet history improves opening/root move ordering.
    int16_t low_ply_hist_[LOW_PLY_HISTORY_SIZE][SQUARE_NB][SQUARE_NB];

    // Countermove [from][to] -> best response
    Move     countermove_[SQUARE_NB][SQUARE_NB];

    // Correction histories keyed by pawn, minor-piece, non-pawn, and continuation context.
    int16_t  pawn_corr_hist_[NCOLORS][CORR_SIZE];
    int16_t  minor_corr_hist_[NCOLORS][CORR_SIZE];
    int16_t  nonpawn_corr_hist_[NCOLORS][NCOLORS][CORR_SIZE];
    int16_t  cont_corr_hist_[NCOLORS][PIECE_TYPE_NB][SQUARE_NB];

    // ---- Per-search state ----
    // ss_arr_[0..3] = sentinels; root = ss_arr_[4] (ply 0)
    SearchStack ss_arr_[MAX_PLY + 8];

    Move pv_table_[MAX_PLY][MAX_PLY];
    int  pv_len_[MAX_PLY];

    struct ScoredMove { Move move; int score; };
    ScoredMove move_buffers_[MAX_PLY][2][MoveList::CAPACITY];

    std::chrono::steady_clock::time_point start_time_;
    double time_limit_;   // hard limit (legacy, = hard_limit_)
    double soft_limit_;   // target time — stop early if best move is stable
    double hard_limit_;   // absolute maximum

    // ---- LMR table ----
    static int  LMR_TABLE[64][64];
    static bool lmr_init_;
    static void init_lmr();

    // ---- Search ----
    static constexpr int MAX_QSEARCH_PLY = 10; // max extra plies of captures in qsearch
    int negamax(int depth, int alpha, int beta, int ply,
                SearchStack* ss, bool is_pv, bool allow_null, bool cut_node);
    int quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss);

    // ---- Move ordering ----
    class MovePicker;
    void  score_moves(ScoredMove* moves, int n, Move tt_move, SearchStack* ss,
                       bool is_root, int ply) const;
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
    void update_pawn_hist(Key pawn_key, PieceType pt, Square to, int bonus);
    void update_low_ply(int ply, Square from, Square to, int bonus);

    // Combined continuation history score for a (piece, to) pair
    int  cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const;
    int  pawn_hist_score(Key pawn_key, PieceType pt, Square to) const;
    int  low_ply_score(int ply, Square from, Square to) const;

    // Bulk history update after a beta cutoff
    void update_all_histories(Move best,
                              const Move* quiets, int quiet_count,
                              const Move* bad_caps, int bad_cap_count,
                              Color stm, int depth, SearchStack* ss);

    // Correction history
    void update_correction(Color stm, const Board& board, SearchStack* ss, int diff, int depth);
    int  correction_value(Color stm, const Board& board, const SearchStack* ss) const;

    void age_history();

    // ---- Misc ----
    bool   check_stop();
    double elapsed_seconds() const;
    void   compute_time_limit(const SearchLimits& limits, Color side);
    void   send_info(int depth, int score, int64_t nodes, double elapsed) const;
    void   init_root_tablebase_scores(const Board& board);
    int    root_tablebase_score(Move move) const;
    int    root_tablebase_ordering_score(Move move) const;
    std::vector<Move> root_tablebase_pv(Move move) const;
    bool   root_tablebase_allows(Move move) const;
};

class SearchThreadPool {
public:
    SearchThreadPool(TranspositionTable& tt,
                     std::atomic_bool& stop_flag,
                     std::function<void(const std::string&)> info_cb = nullptr,
                     std::atomic_bool* ponderhit_flag = nullptr);
    ~SearchThreadPool();

    SearchThreadPool(const SearchThreadPool&) = delete;
    SearchThreadPool& operator=(const SearchThreadPool&) = delete;

    int ensure_threads(int count);
    void clear();
    SearchResult search(Board board, const SearchLimits& limits, int thread_count);

private:
    void worker_loop(int helper_slot);
    SearchLimits limits_for_thread(const SearchLimits& limits, int thread_id, int thread_count,
                                   RootMoveTable& root_table) const;
    SearchResult merge_results(const std::vector<SearchResult>& results, int count,
                               const RootMoveTable& root_table, int64_t elapsed_ms) const;

    TranspositionTable& tt_;
    std::atomic_bool& stop_;
    std::atomic_bool* ponderhit_;
    std::function<void(const std::string&)> info_cb_;

    std::vector<std::unique_ptr<Searcher>> searchers_;
    std::vector<std::thread> workers_;

    mutable std::mutex mutex_;
    std::condition_variable work_cv_;
    std::condition_variable done_cv_;
    bool shutdown_ = false;
    uint64_t epoch_ = 0;

    Board job_board_;
    SearchLimits job_limits_;
    std::vector<SearchResult>* job_results_ = nullptr;
    RootMoveTable* job_root_table_ = nullptr;
    int requested_helpers_ = 0;
    int active_helpers_ = 0;
};
