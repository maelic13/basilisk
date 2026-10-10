#pragma once

// One search thread. A worker is two halves with one seam between them:
//
//   the driver (search_worker.cpp)  iterative deepening, aspiration, time,
//                                   the root and tablebase helpers, info lines
//                                   and the pool interplay;
//   the node kernel (search_kernel.cpp, search_history.cpp, move_picker.cpp,
//                                   history.cpp)  quiescence, negamax, the
//                                   per-ply stack, the move picker, the history
//                                   and correction tables and their policy.
//
// The driver reaches the kernel only through root_search(),
// kernel_begin_search() and kernel_new_game(); the kernel hands results back
// through SearchState (the PV table, selective depth, root effort, node count,
// diagnostics). A worker's state falls into three lifetimes:
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

// Kernel headers: the tables the node kernel owns and the picker it drives.
#include "history.h"
#include "move_picker.h"

// The kind of negamax node, fixed at compile time: the root (ply 0, always
// PV), a PV node searched with an open window, or a null-window node.
enum class NodeType { Root, PV, NonPV };

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

// Stack records below the root. The kernel reads (ss - n) for n up to this
// many plies; those records are sentinels, and the root is stack[STACK_SENTINELS].
// Seven covers the deepest continuation read (six plies back from a child's
// update) on either kernel.
inline constexpr int STACK_SENTINELS = 7;

struct SearchState {
    Board*   board = nullptr;
    int64_t  nodes = 0;
    int64_t  tb_hits = 0;
    int64_t  nodes_limit = 0;  // 0 = unlimited
    // The pool's node total is published in blocks of sharedNodeBatch rather
    // than with one atomic add per node on a line every thread shares. The
    // last published total is kept so the node-limit check and current_nodes()
    // see a sane figure between flushes; a single-thread search has no pool
    // counter and never enters that path.
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

    // The kernel's per-ply records and buffers, sized for the deepest line.
    SearchStack stack[MAX_PLY + 2 * STACK_SENTINELS];
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

    // The pool's diagnostic aggregate, printed by thread 0 after the join
    // because the info callback is thread 0's and the helpers' counters are
    // complete only then. Takes the pool itself so each helper's private
    // counters are read directly and DiagCounters stays private.
    void print_pool_diag(const std::vector<std::unique_ptr<Searcher>>& pool,
                         int thread_count,
                         const std::vector<int>& completed_depths) const;

private:
    SearchShared shared_;
    SearchConfig config_;
    SearchState  state_;

    Evaluator evaluator_;

    // ---- The driver's view of the kernel ---------------------------------
    // Searches the root at `depth` inside [alpha, beta] and returns its score;
    // the line, selective depth and root effort land in state_.
    int  root_search(int depth, int alpha, int beta);
    // Prepares the kernel's tables and stack for one `go`: the LMR table from
    // the parameters, history ageing, a clean stack.
    void kernel_begin_search();
    // Forgets everything learned across searches (ucinewgame).
    void kernel_new_game();

    // ---- Driver plumbing ---------------------------------------------------
    // Publish this thread's unpublished node count to the shared counter.
    // Called on every batch boundary and once at search teardown so no node is
    // left unpublished when the pool reads the total.
    void flush_shared_nodes();
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

    // ---- Node kernel -------------------------------------------------------
    // Everything from here to the end of the class belongs to the node kernel
    // and is defined in search_kernel.cpp and search_history.cpp. The driver
    // never names any of it: a kernel may change its tables, its stack record
    // and its recursive signatures without touching the driver, and a cluster
    // built behind an umbrella option replaces this section and those files
    // while the driver stays shared.

    // History tables persist across searches and age between them; storage
    // and whole-table lifecycle live in HistoryTables, the update policy
    // (bonus shapes, what a cutoff trains) in search_history.cpp.
    HistoryTables hist_;
    int history_age_counter_ = 0;

    // LMR table, recomputed from the parameters at the start of each search.
    // init_lmr() fills [1..63][1..63]; row and column 0 are defined (zero)
    // rather than merely unread, so a consumer that forgets the depth >= 1,
    // searched > 0 guard reads a zero reduction instead of garbage.
    int  lmr_table_[64][64]{};
    void init_lmr(float base, float divisor);

    // Every TT store in the search goes through this wrapper, so the same-key
    // telemetry cannot drift out of sync with the store sites.
    void tt_store(Key key, int depth, int score, TTFlag flag, Move m, int ply,
                  int static_eval);

    static constexpr int MAX_QSEARCH_PLY = 10; // max extra plies of captures in qsearch
    template<NodeType NT>
    int negamax(int depth, int alpha, int beta, int ply,
                SearchStack* ss, bool allow_null, bool cut_node);
    int quiescence(int alpha, int beta, int ply, int qply, SearchStack* ss);

    // History gravity: a bonus pulls the entry toward its bound, so repeated
    // bonuses saturate at MAX_VAL instead of overflowing.
    template<int MAX_VAL>
    static void hist_update(int16_t& e, int bonus) {
        e += static_cast<int16_t>(bonus - static_cast<int>(e) * std::abs(bonus) / MAX_VAL);
    }

    // The single make/unmake seam. Every search-side move execution goes
    // through this pair (negamax, quiescence, evasions, ProbCut, null move),
    // so per-ply bookkeeping, and later an NNUE accumulator push/pop, attaches
    // in exactly one place. Do not call board.make_move directly from search.
    void do_move(SearchStack* ss, Move m) {
        ss->move        = m;
        ss->moved_piece = type_of(state_.board->piece_on(from_sq(m)));
        state_.board->make_move(m);
    }
    void undo_move(SearchStack* ss, Move m) {
        state_.board->unmake_move(m);
        ss->move = MOVE_NONE;
    }
    void do_null_move(SearchStack* ss) {
        ss->move        = MOVE_NULL;
        ss->moved_piece = NO_PIECE_TYPE;
        state_.board->make_null_move();
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

    int  history_bonus_value(int depth) const;
    int  history_malus_value(int depth) const;
    // One (piece, to) continuation-history update at the 1/2/4-ply
    // back-references from `ss`.
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
};
