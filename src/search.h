#pragma once

#include "board.h"
#include "search_params.h"
#include "tt.h"
#include "eval.h"
#include "history.h"
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
    int       double_exts = 0;               // stacked 2-ply singular extensions on this path
};

struct SearchResult;

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

struct SearchLimits {
    int depth      = MAX_SEARCH_DEPTH;
    int movetime   = 0;
    int wtime      = 0, btime = 0;
    int winc       = 0, binc  = 0;
    int movestogo  = 0;
    int64_t nodes  = 0;
    int mate        = 0;
    std::atomic<int64_t>* shared_nodes = nullptr;
    std::atomic<int64_t>* shared_tbhits = nullptr;
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
    bool tm_debug = false;      // emit per-move time-accounting info string (Step 5.3)
    bool diag = false;          // emit end-of-search diagnostic counters (8.6.6)
    // Instant the `go` command was parsed off UCI input (default = unset). Used
    // only to report dispatch latency in tm_debug; does not affect timing yet.
    std::chrono::steady_clock::time_point go_recv_time{};
    std::vector<Move> root_moves;
    std::vector<Syzygy::RootMoveInfo> syzygy_root_moves;
    RootMoveTable* root_table = nullptr;
    SearchParams params;
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

    // ---- Persistent per-root-move records (8.6.10e; Rarog 10.1 pattern) ----
    // Pure bookkeeping this phase: written by the root loop, consumed by
    // nothing yet. This is the substrate the 8.5.12 remainder
    // (uncertainty-aware aspiration, root-effort TM), Phase-10 consumers and
    // Phase-11 voting plug into — they become readers of existing data
    // instead of each re-plumbing the root loop. Mean/variance are Welford
    // over completed per-iteration scores; `nodes` is the cumulative subtree
    // effort; `pv` is captured whenever this move was best at its ply.
    struct RootMoveStat {
        Move    move           = MOVE_NONE;
        int     score          = -INF_SCORE;  // last search result for this move
        int     previous_score = -INF_SCORE;  // result from the prior visit
        double  mean           = 0.0;
        double  m2             = 0.0;         // Welford accumulator
        int     samples        = 0;
        int64_t nodes          = 0;
        int     seldepth       = 0;
        bool    exact          = false;       // last result inside the window?
        std::vector<Move> pv;                 // last PV when this move led

        void add_sample(int s) noexcept {
            previous_score = score;
            score = s;
            ++samples;
            const double d = double(s) - mean;
            mean += d / samples;
            m2   += d * (double(s) - mean);
        }
        [[nodiscard]] double variance() const noexcept {
            return samples > 1 ? m2 / (samples - 1) : 0.0;
        }
    };
    std::vector<RootMoveStat> root_stats_;
    RootMoveStat& root_stat(Move m) {
        for (auto& r : root_stats_)
            if (r.move == m) return r;
        root_stats_.push_back(RootMoveStat{});
        root_stats_.back().move = m;
        return root_stats_.back();
    }

    // ---- 8.6.6 diagnostic counters (Rarog 7.6 pattern) ----
    // Always counted — plain per-Searcher int64 increments on lines that are
    // already hot, measured to cost nothing (interleaved best-of-5 NPS) — and
    // printed only when SearchLimits.diag is set (UCI `Diag`, TUNE builds).
    // These size candidates BEFORE they spend SPRT slots and are the substrate
    // Phase 10's acceptance criteria assume; check_exts must read 0 once
    // 8.6.7 lands.
    struct DiagCounters {
        int64_t interior_nodes = 0, in_check_nodes = 0, check_exts = 0, tt_pv_nodes = 0;
        int64_t tt_probes = 0, tt_hits = 0, tt_cutoffs = 0;
        int64_t rfp_cuts = 0, razor_cuts = 0, null_tries = 0, null_cuts = 0;
        int64_t probcut_tries = 0, probcut_cuts = 0;
        int64_t fut_prunes = 0, lmp_prunes = 0, hist_prunes = 0, see_prunes = 0;
        int64_t lmr_applied = 0, lmr_researched = 0;
        int64_t qs_nodes = 0, qs_evasion_nodes = 0;
        int64_t hist_cutoff_updates = 0, hist_reward_updates = 0;
        // 8.7.1(c): snapshotted from the Board at search teardown (see
        // board.h) — board_ptr_ is nulled before print_diag() runs.
        int64_t see_ge_calls = 0, gives_check_calls = 0;
        // 9.3(c): TT stores that landed on a slot already holding this
        // position's key, versus stores that claimed a different slot. At
        // Threads>1 the same-key share is how much the pool is re-writing
        // entries it (or another thread) already owns rather than competing
        // for capacity — the quantity 9.5's TT-coordination work moves.
        int64_t tt_stores = 0, tt_stores_same_key = 0;

        // ---- 5.2 differential harness (selected by BAS-O03) ----------------
        // BAS-O01/O03 measured our effective branching factor at ~2.20 against
        // the reference's ~1.61: at equal time we finish 15.6 plies where it
        // finishes 25.2, on MORE nodes per move. The tree is too wide, not too
        // small. These counters localize where that width is created; the
        // pre-existing ones above could not, because they report how often a
        // mechanism fired without reporting how often it could have.

        // Ordering quality. A fail-high on the first move costs one move's
        // search; on the n-th it costs n. Mean cutoff index is therefore a
        // direct multiplier on tree width, and `cutoff_src` says which stage
        // to fix. Counted at the cutoff, so the denominator is fail_highs.
        int64_t fail_highs = 0, fail_high_first = 0, fail_high_index_sum = 0;
        int64_t cutoff_src_tt = 0, cutoff_src_good_tactical = 0;
        int64_t cutoff_src_quiet = 0, cutoff_src_bad_tactical = 0;

        // LMR width. `lmr_applied` alone cannot distinguish "rarely eligible"
        // from "eligible but almost never reduced" — the two have opposite
        // repairs. The blocked reasons are mutually exclusive and evaluated in
        // the live gate's own short-circuit order, so
        //   eligible = applied + clamped_zero + sum(blocked_*).
        // `reduction_plies / applied` is the mean reduction actually taken,
        // which is the number a timid-LMR hypothesis lives or dies on.
        int64_t lmr_eligible = 0, lmr_reduction_plies = 0, lmr_clamped_zero = 0;
        int64_t lmr_blocked_depth = 0, lmr_blocked_searched = 0;
        int64_t lmr_blocked_in_check = 0, lmr_blocked_movetype = 0;
        int64_t lmr_blocked_gives_check = 0;

        void reset() noexcept { *this = DiagCounters{}; }
        // Pool aggregation (9.3c): sum a helper's counters into this one.
        // Written out rather than punned through an int64_t* — the
        // static_assert below is what catches a counter added without a
        // matching line here (it fires the moment the field count changes).
        void add(const DiagCounters& o) noexcept {
            interior_nodes += o.interior_nodes;
            in_check_nodes += o.in_check_nodes;
            check_exts += o.check_exts;
            tt_pv_nodes += o.tt_pv_nodes;
            tt_probes += o.tt_probes;
            tt_hits += o.tt_hits;
            tt_cutoffs += o.tt_cutoffs;
            rfp_cuts += o.rfp_cuts;
            razor_cuts += o.razor_cuts;
            null_tries += o.null_tries;
            null_cuts += o.null_cuts;
            probcut_tries += o.probcut_tries;
            probcut_cuts += o.probcut_cuts;
            fut_prunes += o.fut_prunes;
            lmp_prunes += o.lmp_prunes;
            hist_prunes += o.hist_prunes;
            see_prunes += o.see_prunes;
            lmr_applied += o.lmr_applied;
            lmr_researched += o.lmr_researched;
            qs_nodes += o.qs_nodes;
            qs_evasion_nodes += o.qs_evasion_nodes;
            hist_cutoff_updates += o.hist_cutoff_updates;
            hist_reward_updates += o.hist_reward_updates;
            see_ge_calls += o.see_ge_calls;
            gives_check_calls += o.gives_check_calls;
            tt_stores += o.tt_stores;
            tt_stores_same_key += o.tt_stores_same_key;
            fail_highs += o.fail_highs;
            fail_high_first += o.fail_high_first;
            fail_high_index_sum += o.fail_high_index_sum;
            cutoff_src_tt += o.cutoff_src_tt;
            cutoff_src_good_tactical += o.cutoff_src_good_tactical;
            cutoff_src_quiet += o.cutoff_src_quiet;
            cutoff_src_bad_tactical += o.cutoff_src_bad_tactical;
            lmr_eligible += o.lmr_eligible;
            lmr_reduction_plies += o.lmr_reduction_plies;
            lmr_clamped_zero += o.lmr_clamped_zero;
            lmr_blocked_depth += o.lmr_blocked_depth;
            lmr_blocked_searched += o.lmr_blocked_searched;
            lmr_blocked_in_check += o.lmr_blocked_in_check;
            lmr_blocked_movetype += o.lmr_blocked_movetype;
            lmr_blocked_gives_check += o.lmr_blocked_gives_check;
        }
    };
    // 42 counters, all int64_t. If this fails you added a counter: add it to
    // add() above and update the count, or the pool aggregate silently drops it.
    static_assert(sizeof(DiagCounters) == 42 * sizeof(int64_t),
                  "DiagCounters changed shape — update DiagCounters::add()");
    DiagCounters diag_;
    void print_diag() const;
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
    double time_limit_;   // hard limit (legacy, = hard_limit_)
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
        ss->moved_piece = type_of(board_ptr_->board_sq[from_sq(m)]);
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

    // Phase 6.3 bonus/malus shape (single source of truth for both
    // update_all_histories and the Step 6.4 post-LMR conthist nudge).
    int  history_bonus_value(int depth) const;
    int  history_malus_value(int depth) const;
    // Applies a single (piece, to) continuation-history update at the 1/2/4-ply
    // back-references from `ss` -- the per-move half of update_all_histories'
    // continuation-history block, reused by the post-LMR update (Step 6.4).
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
    void   send_info(int depth, int score, int64_t nodes, double elapsed) const;
    int64_t record_node();
    void   record_tbhit(int64_t count = 1);
    int64_t current_nodes() const;
    int64_t current_tbhits() const;
    void   init_root_tablebase_scores(const Board& board);
    int    root_tablebase_score(Move move) const;
    int    root_tablebase_ordering_score(Move move) const;
    std::vector<Move> root_tablebase_pv(Move move) const;
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
