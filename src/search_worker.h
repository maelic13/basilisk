#pragma once

// One search thread: quiescence, negamax, iterative deepening and the root
// and tablebase helpers. A worker's state falls into three lifetimes:
//
//   SearchConfig  fixed for one `go`: the caller's limits and this thread's
//                 place in the pool;
//   SearchShared  owned elsewhere and borrowed: the engine's table, flags and
//                 output callback, and the pool's per-`go` counters and root
//                 table;
//   SearchState   mutable, owned by this worker for one search: the board,
//                 counters, time, root state, diagnostics, stacks and buffers.
//
// Evaluator, history tables and the LMR table persist across searches.
// Recursive search reads all of them through the worker; nothing large is
// copied into a call.

#include "board.h"
#include "eval.h"
#include "history.h"
#include "move_picker.h"
#include "search_diagnostics.h"
#include "search_root.h"
#include "search_types.h"
#include "syzygy.h"
#include "tt.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

struct SearchConfig {
    SearchLimits limits;
    int  thread_id     = 0;
    int  thread_count  = 1;
    bool update_tt_age = true;   // false when the pool ages the table for all threads
    int  root_filter_index = -1; // -1 = search all root moves
    int  root_filter_count = 1;
};

// A pool's per-`go` resources. All null for a single-thread search.
struct PoolResources {
    std::atomic<int64_t>* nodes  = nullptr;
    std::atomic<int64_t>* tbhits = nullptr;
    RootMoveTable* root_table    = nullptr;
};

struct SearchShared {
    TranspositionTable& tt;
    std::atomic_bool&   stop;
    std::atomic_bool*   ponderhit = nullptr;
    InfoCallback        info;    // set on the reporting thread only
    PoolResources       pool;
};

struct SearchState {
    Board*   board = nullptr;
    int64_t  nodes = 0;
    int64_t  tb_hits = 0;
    int64_t  nodes_limit = 0;  // 0 = unlimited
    // 9.3(b) shared-counter batching. The multi-thread node total used to take
    // an atomic fetch_add on ONE shared cache line at EVERY node from EVERY
    // thread. Now each thread publishes in blocks of sharedNodeBatch and keeps
    // the last published total here so the node-limit check and current_nodes()
    // still see a sane figure between flushes. Single-thread searches have no
    // pool counter at all, so that path is untouched (bench cannot move).
    int64_t  shared_nodes_flushed = 0;  // local nodes already published
    int64_t  shared_nodes_total = 0;    // pool total as of the last publish
    int      sel_depth = 0;
    bool     stopped = false;
    bool     pondering = false;
    Color    root_side = WHITE;

    std::chrono::steady_clock::time_point start_time{};
    double soft_limit = 0.0;  // target time — stop early if best move is stable
    double hard_limit = 0.0;  // absolute maximum

    std::vector<Syzygy::RootMoveInfo> root_tb_moves;
    // In-search tablebase probes: off when the root was ranked by DTZ, or by
    // WDL and not winning (Stockfish's rule); on otherwise.
    bool tb_probe_in_search = true;
    // The root's move-ordering inputs, set at the start of each search.
    RootOrdering root_ordering;
    // MultiPV: root moves already reported at this depth, which the search of
    // the next line skips. Empty at every other time, so a single-PV search
    // never consults it.
    std::vector<Move> root_excluded;
    int multipv_lines = 1;
    int64_t root_depth_nodes = 0;
    int64_t root_best_nodes = 0;
    int     root_best_effort = 0;

    DiagCounters diag;
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    DecisionTrace trace;
#endif

    // stack[0..3] = sentinels; root = stack[4] (ply 0)
    SearchStack stack[MAX_PLY + 8];
    Move pv_table[MAX_PLY][MAX_PLY];
    int  pv_len[MAX_PLY];
    ScoredMove move_buffers[MAX_PLY][2][MoveList::CAPACITY];
};

class Searcher {
public:
    Searcher(TranspositionTable& tt,
             std::atomic_bool& stop_flag,
             InfoCallback info_cb = nullptr,
             std::atomic_bool* ponderhit_flag = nullptr);

    // A single-thread search: this worker is the whole pool.
    SearchResult search(Board board, const SearchLimits& limits);
    // One thread of a pool search.
    SearchResult search(Board board, SearchConfig config, const PoolResources& pool);
    void clear(); // Reset all history (e.g., on ucinewgame)

    // ---- 9.3(c) multi-thread diagnostics ----
    // print_search_diag() is gated on the info callback, which only thread 0
    // owns, so at Threads>1 every counter it printed described the MAIN THREAD
    // ALONE — sound, but blind to the pool. The pool collects each helper's
    // counters after the join and hands the aggregate back to thread 0 to
    // print. Main's own lines are unchanged, so 1T output is byte-for-byte
    // what it was.
    // Takes the pool itself rather than a pre-summed struct: same-class access
    // reaches each helper's private counters directly, so no accessor has to be
    // opened up and DiagCounters stays private.
    void print_pool_diag(const std::vector<std::unique_ptr<Searcher>>& pool,
                         int thread_count,
                         const std::vector<int>& completed_depths) const;

private:
    SearchShared shared_;
    SearchConfig config_;
    SearchState  state_;

    Evaluator evaluator_;
    // ---- History tables (persist across searches; aged each search) ----
    // Storage + whole-table lifecycle live in HistoryTables (history.h); the
    // update POLICY (bonus formulas, what a cutoff trains) is this class's, in
    // search_history.cpp.
    HistoryTables hist_;
    int history_age_counter_ = 0;

    // ---- LMR table (per-instance; recomputed at the start of each search) ----
    // Zero-initialised: init_lmr() fills only [1..63][1..63] (a reduction is
    // meaningless at depth 0 or move 0), and every consumer clamps its indices
    // into that range under a searched>0 / depth>=1 guard. The {} makes row and
    // column 0 defined rather than merely unread — cheap insurance against a
    // future consumer that forgets the guard (8.6.2b).
    int  lmr_table_[64][64]{};
    void init_lmr(float base, float divisor);

    // Publish this thread's unpublished node count to the shared counter
    // (9.3b). Called on every batch boundary and once at search teardown so no
    // node is left unpublished when the pool reads the total.
    void flush_shared_nodes();
    // 9.3(c): every TT store in the search goes through this wrapper, so the
    // same-key telemetry cannot drift out of sync with the actual store sites.
    void tt_store(Key key, int depth, int score, TTFlag flag, Move m, int ply,
                  int static_eval);

    // ---- Search ----
    static constexpr int MAX_QSEARCH_PLY = 10; // max extra plies of captures in qsearch
    int negamax(int depth, int alpha, int beta, int ply,
                SearchStack* ss, bool is_pv, bool allow_null, bool cut_node);
    int quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss);

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
        ss->moved_piece = type_of(state_.board->piece_on(from_sq(m)));
        state_.board->make_move(m);
        // Phase 9: accumulator.push(dirty piece delta) attaches here.
    }
    void undo_move(SearchStack* ss, Move m) {
        state_.board->unmake_move(m);
        ss->move = MOVE_NONE;
        // Phase 9: accumulator.pop() attaches here.
    }
    void do_null_move(SearchStack* ss) {
        ss->move        = MOVE_NULL;
        ss->moved_piece = NO_PIECE_TYPE;
        state_.board->make_null_move();
        // Null move has no piece delta; the accumulator is reused as-is.
    }
    void undo_null_move(SearchStack* ss) {
        state_.board->unmake_null_move();
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
    void   compute_time_limit(const SearchLimits& limits, bool ponder, Color side, int game_ply);
    void   send_info(int depth, int multipv, int score, const std::vector<Move>& line,
                     int64_t nodes, double elapsed) const;
    int64_t record_node();
    void   record_tbhit(int64_t count = 1);
    int64_t current_nodes() const;
    int64_t current_tbhits() const;
    void   init_root_tablebase_scores(const Board& board);
    int    root_tablebase_score(Move move) const;
    int    root_tablebase_display(Move move) const;
    bool   root_tablebase_allows(Move move) const;
    Move   ponder_from_tt(const Board& root, Move bestmove) const;
};
