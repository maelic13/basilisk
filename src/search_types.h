#pragma once

// Value types shared by the search and its callers: score and depth limits,
// the per-ply stack, the limits of one `go`, its result, and the score-band
// predicates.

#include "constants.h"
#include "move.h"
#include "search_params.h"
#include "syzygy.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
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

class RootMoveTable;

struct SearchLimits {
    int depth      = MAX_SEARCH_DEPTH;
    int movetime   = 0;
    int wtime      = 0, btime = 0;
    int winc       = 0, binc  = 0;
    int movestogo  = 0;
    int64_t nodes  = 0;
    int mate        = 0;
    int multipv     = 1;   // lines reported per depth; only the main thread searches more than one
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
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    bool decision_trace = false; // bounded plies 1-2 decision trace (A.5.3)
#endif
#ifdef BASILISK_ABLATION
    int ablation_mask = 0;
#endif
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
    int     seldepth   = 0;
    int64_t nodes      = 0;
    int64_t tbhits     = 0;
    int64_t elapsed_ms = 0;
    // The searched PV, pv[0] == bestmove when non-empty. Carried so the final
    // tablebase PV extension (Engine) can validate the searched line itself.
    std::vector<Move> pv;
};

static_assert(tablebaseValue == MATE_SCORE - MAX_PLY - 1,
              "the tablebase band sits directly below the mate band");

// A decisive tablebase score (a TB win or loss, ply-adjusted), as opposed to a
// mate score or an ordinary evaluation.
[[nodiscard]] inline bool is_tablebase_decisive(int score) {
    const int a = score < 0 ? -score : score;
    return a >= tablebaseWinInMaxPly && a < MATE_SCORE - MAX_PLY;
}

// A mate or a tablebase result: never an evaluation.
[[nodiscard]] inline bool is_decisive(int score) {
    return (score < 0 ? -score : score) >= tablebaseWinInMaxPly;
}

// One UCI `info` line in Stockfish's field order: depth seldepth multipv
// score [bound] nodes nps hashfull tbhits time pv. `bound` is empty,
// "lowerbound" or "upperbound". `pv` must already be legal from the root; the
// caller owns any tablebase extension. `nps` floors the elapsed time at 1 ms.
[[nodiscard]] std::string format_info_line(int depth, int seldepth, int multipv, int score,
                             std::string_view bound, int64_t nodes, double elapsed,
                             int64_t tbhits, int hashfull, const std::vector<Move>& pv);

// A single-PV aspiration failure prints its bound only once a search has run
// this long, as Stockfish does, so fast games print the same lines as before.
inline constexpr double kBoundLineAfterSeconds = 3.0;

// The longest legal prefix of `line` played from `root`.
[[nodiscard]] std::vector<Move> legal_line(const Board& root, const std::vector<Move>& line);

// Whether a multi-thread search must print the merged result's line before
// `bestmove`: the last line a GUI saw is the main thread's, and it describes
// the merged result only when move and depth agree.
[[nodiscard]] bool needs_pool_line(const SearchResult& merged, const SearchResult& main_thread);
