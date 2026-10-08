#pragma once

#include "board.h"
#include "constants.h"
#include "search_params.h"
#include "search_types.h"
#include "search_root.h"
#include "search_diagnostics.h"
#include "tt.h"
#include "eval.h"
#include "history.h"
#include "syzygy.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class Searcher {
public:
    Searcher(TranspositionTable& tt,
             std::atomic_bool& stop_flag,
             std::function<void(const std::string&)> info_cb = nullptr,
             std::atomic_bool* ponderhit_flag = nullptr);

    SearchResult search(Board board, const SearchLimits& limits);
    void clear(); // Reset all history (e.g., on ucinewgame)

    // ---- 9.3(c) multi-thread diagnostics ----
    // print_diag() is gated on info_cb_, which only thread 0 owns, so at
    // Threads>1 every counter it printed described the MAIN THREAD ALONE —
    // sound, but blind to the pool, and 9.5 cannot be read that way. The pool
    // collects each helper's counters after the join and hands the aggregate
    // back to thread 0 to print. Main's own lines are unchanged, so 1T output
    // is byte-for-byte what it was.
    // Takes the pool itself rather than a pre-summed struct: same-class access
    // reaches each helper's private counters directly, so no accessor has to be
    // opened up and DiagCounters stays private.
    void print_pool_diag(const std::vector<std::unique_ptr<Searcher>>& pool,
                         int thread_count,
                         const std::vector<int>& completed_depths) const;

private:
    Evaluator evaluator_;
    TranspositionTable& tt_;
    std::atomic_bool&   stop_;
    std::atomic_bool*   ponderhit_;
    std::function<void(const std::string&)> info_cb_;

    DiagCounters diag_;
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    DecisionTrace trace_;
#endif
    // Publish this thread's unpublished node count to the shared counter
    // (9.3b). Called on every batch boundary and once at search teardown so no
    // node is left unpublished when the pool reads the total.
    void flush_shared_nodes();
    // 9.3(c): every TT store in the search goes through this wrapper, so the
    // same-key telemetry cannot drift out of sync with the actual store sites.
    void tt_store(Key key, int depth, int score, TTFlag flag, Move m, int ply,
                  int static_eval);

    Board*   board_ptr_;
    int64_t  nodes_;
    int64_t  tb_hits_;
    int64_t  nodes_limit_;  // 0 = unlimited
    // 9.3(b) shared-counter batching. The multi-thread node total used to take
    // an atomic fetch_add on ONE shared cache line at EVERY node from EVERY
    // thread. Now each thread publishes in blocks of sharedNodeBatch and keeps
    // the last published total here so the node-limit check and current_nodes()
    // still see a sane figure between flushes. Single-thread searches never set
    // `shared_nodes` at all, so that path is untouched (bench cannot move).
    int64_t  shared_nodes_flushed_;  // local nodes already published
    int64_t  shared_nodes_total_;    // pool total as of the last publish
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
    // MultiPV: root moves already reported at this depth, which the search of
    // the next line skips. Empty at every other time, so a single-PV search
    // never consults it.
    std::vector<Move> root_excluded_;
    // In-search tablebase probes: off when the root was ranked by DTZ, or by
    // WDL and not winning (Stockfish's rule); on otherwise.
    bool tb_probe_in_search_ = true;
    int multipv_lines_ = 1;
    int64_t  root_depth_nodes_;
    int64_t  root_best_nodes_;
    int      root_best_effort_;
    int      history_age_counter_;

    // ---- History tables (persist across searches; aged each search) ----
    // Storage + whole-table lifecycle live in HistoryTables (history.h,
    // 8.6.10b); the update POLICY (bonus formulas, what a cutoff trains)
    // stays in this class.
    HistoryTables hist_;

    // ---- Per-search state ----
    // ss_arr_[0..3] = sentinels; root = ss_arr_[4] (ply 0)
    SearchStack ss_arr_[MAX_PLY + 8];

    Move pv_table_[MAX_PLY][MAX_PLY];
    int  pv_len_[MAX_PLY];

    struct ScoredMove { Move move; int score; };
    ScoredMove move_buffers_[MAX_PLY][2][MoveList::CAPACITY];

    std::chrono::steady_clock::time_point start_time_;
    double soft_limit_;   // target time — stop early if best move is stable
    double hard_limit_;   // absolute maximum

    // ---- LMR table (per-instance; recomputed at the start of each search) ----
    // Zero-initialised: init_lmr() fills only [1..63][1..63] (a reduction is
    // meaningless at depth 0 or move 0), and every consumer clamps its indices
    // into that range under a searched>0 / depth>=1 guard. The {} makes row and
    // column 0 defined rather than merely unread — cheap insurance against a
    // future consumer that forgets the guard (8.6.2b).
    int  lmr_table_[64][64]{};
    void init_lmr(float base, float divisor);

    // ---- Search ----
    static constexpr int MAX_QSEARCH_PLY = 10; // max extra plies of captures in qsearch
    int negamax(int depth, int alpha, int beta, int ply,
                SearchStack* ss, bool is_pv, bool allow_null, bool cut_node);
    int quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss);

    // ---- Move ordering ----
    class MovePicker;
    void  score_moves(ScoredMove* moves, int n, SearchStack* ss,
                       bool is_root, int ply) const;
    static Move pick_next(ScoredMove* moves, int idx, int n);

    // ---- History helpers ----
    template<int MAX_VAL>
    static void hist_update(int16_t& e, int bonus) {
        e += static_cast<int16_t>(bonus - static_cast<int>(e) * std::abs(bonus) / MAX_VAL);
    }

    // ---- The single make/unmake seam (8.6.10d) ----
    // EVERY search-side move execution goes through this pair (negamax,
    // quiescence, in-check evasions, ProbCut, null move). It exists so that
    // per-ply bookkeeping happens in exactly one place — which is where the
    // Phase-9 NNUE accumulator push/pop and the 8.5.3 dirty-piece recording
    // attach, once each, instead of at every call site. Do not call
    // board.make_move directly from search code.
    void do_move(SearchStack* ss, Move m) {
        ss->move        = m;
        ss->moved_piece = type_of(board_ptr_->piece_on(from_sq(m)));
        board_ptr_->make_move(m);
        // Phase 9: accumulator.push(dirty piece delta) attaches here.
    }
    void undo_move(SearchStack* ss, Move m) {
        board_ptr_->unmake_move(m);
        ss->move = MOVE_NONE;
        // Phase 9: accumulator.pop() attaches here.
    }
    void do_null_move(SearchStack* ss) {
        ss->move        = MOVE_NULL;
        ss->moved_piece = NO_PIECE_TYPE;
        board_ptr_->make_null_move();
        // Null move has no piece delta; the accumulator is reused as-is.
    }
    void undo_null_move(SearchStack* ss) {
        board_ptr_->unmake_null_move();
        ss->move = MOVE_NONE;
    }

    void update_quiet(Color stm, Square from, Square to, int bonus);
    void update_cap(PieceType pt, Square to, PieceType cap, int bonus);
    void update_cont(HistoryTables::ContHistTable& tbl,
                     PieceType ppt, Square pto,
                     PieceType cpt, Square cto, int bonus);
    void update_pawn_hist(Key pawn_key, PieceType pt, Square to, int bonus);
    void update_low_ply(int ply, Square from, Square to, int bonus);

    // Phase 6.3 bonus/malus shape.
    int  history_bonus_value(int depth) const;
    int  history_malus_value(int depth) const;
    // Applies a single (piece, to) continuation-history update at the 1/2/4-ply
    // back-references from `ss` -- the per-move half of update_all_histories'
    // continuation-history block.
    void update_cont_for_move(SearchStack* ss, PieceType pt, Square to, int bonus);

    // Combined continuation history score for a (piece, to) pair
    int  cont_hist_score(const SearchStack* ss, PieceType pt, Square to) const;

    // Bulk history update after a beta cutoff
    void update_all_histories(Move best, bool best_is_tt,
                              const Move* quiets, int quiet_count,
                              const Move* bad_caps, int bad_cap_count,
                              Color stm, int depth, SearchStack* ss,
                              bool reward_only = false, int bonus_scale = 100);

    // Correction history
    void update_correction(Color stm, const Board& board, SearchStack* ss, int diff, int depth);
    int  correction_value(Color stm, const Board& board, const SearchStack* ss) const;

    void age_history();

    // ---- Misc ----
    bool   check_stop();
    double elapsed_seconds() const;
    void   compute_time_limit(const SearchLimits& limits, Color side, int game_ply);
    void   send_info(int depth, int multipv, int score, const std::vector<Move>& line,
                     int64_t nodes, double elapsed) const;
    int64_t record_node();
    void   record_tbhit(int64_t count = 1);
    int64_t current_nodes() const;
    int64_t current_tbhits() const;
    void   init_root_tablebase_scores(const Board& board);
    int    root_tablebase_score(Move move) const;
    int    root_tablebase_display(Move move) const;
    int    root_tablebase_ordering_score(Move move) const;
    bool   root_tablebase_allows(Move move) const;
    Move   ponder_from_tt(const Board& root, Move bestmove) const;
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
    int resize_threads(int count);
    int active_thread_count() const;
    void clear();
    SearchResult search(Board board, const SearchLimits& limits, int thread_count);

private:
    void worker_loop(int helper_slot);
    SearchLimits limits_for_thread(const SearchLimits& limits, int thread_id, int thread_count,
                                   RootMoveTable& root_table) const;
    SearchResult merge_results(const std::vector<SearchResult>& results, int count,
                               const RootMoveTable& root_table, int64_t elapsed_ms) const;
    static int normalize_thread_count(int count);

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
