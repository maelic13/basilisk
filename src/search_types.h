#pragma once

// Value types shared by the search and its callers: score and depth limits,
// the per-ply stack, the limits of one `go`, its result, and the score-band
// predicates.

#include "constants.h"
#include "move.h"
#include "search_params.h"
#include "syzygy.h"
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

static constexpr int MAX_SEARCH_DEPTH = 100;

// Material the search reasons with in ordering and pruning margins; the
// evaluation has its own values.
inline constexpr int PIECE_VALUE[PIECE_TYPE_NB] = {0, 100, 300, 300, 500, 900, 20000};

// A move with its ordering score; the kernel's move buffers hold these.
struct ScoredMove { Move move; int score; };

struct PieceToHistory;
struct PieceToCorrection;

// The selectivity core's per-ply record. The root is the record at
// STACK_SENTINELS; the records below it are default records, which read as
// "no move, no piece, no evaluation" to every (ss - n) consumer. The records
// are shared by all children of one node, so a field written by a child (the
// cutoff count) accumulates over that node's searched moves.
struct SearchStack {
    Move      move        = MOVE_NONE;       // move being searched at this ply
    Move      excluded    = MOVE_NONE;       // excluded move (singular extensions)
    // The continuation row the move made from here selects; the kernel points
    // every record at the sentinel row before a search, so a record without a
    // move reads zeros.
    PieceToHistory* cont_hist = nullptr;
    // The continuation-correction row the move made from here selects (the
    // sentinel row likewise).
    PieceToCorrection* cont_corr = nullptr;
    int       eval        = VALUE_NONE;      // corrected static eval at this ply
    int       stat_score  = 0;               // history score of the move searched from here
    int       reduction   = 0;               // reduction applied to the move searched from here
    int       move_count  = 0;               // moves picked at this node, pruned ones included
    int       cutoff_cnt  = 0;               // fail highs at this ply among one parent's children
    PieceType moved_piece = NO_PIECE_TYPE;   // piece type that made 'move'
    bool      tt_pv       = false;           // node lies near a TT/PV line
    bool      tt_hit      = false;           // the TT probe found this position
    bool      in_check    = false;           // side to move is in check
    int       double_exts = 0;               // stacked 2-ply singular extensions on this path
};

// A single-PV aspiration failure prints its bound only once a search has run
// this long, as Stockfish does, so fast games print the same lines as before.
inline constexpr double kBoundLineAfterSeconds = 3.0;

struct SearchLimits {
    int depth      = MAX_SEARCH_DEPTH;
    int movetime   = 0;
    int wtime      = 0, btime = 0;
    int winc       = 0, binc  = 0;
    int movestogo  = 0;
    int64_t nodes  = 0;
    int mate        = 0;
    int multipv     = 1;   // lines reported per depth; only the main thread searches more than one
    int overhead   = 0;   // move overhead to subtract [ms]
    bool infinite  = false;
    bool ponder    = false;
    int syzygy_probe_depth = 0; // 0 = disabled
    int syzygy_probe_limit = 0;
    bool syzygy_50_move_rule = true;
    bool tm_debug = false;      // emit the per-move time-accounting info string
    bool diag = false;          // emit end-of-search diagnostic counters
#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
    bool decision_trace = false; // the bounded plies 1-2 decision trace
#endif
#ifdef BASILISK_ABLATION
    int ablation_mask = 0;
#endif
    // Instant the `go` command was parsed off UCI input (default = unset). Used
    // only to report dispatch latency in tm_debug; does not affect timing yet.
    std::chrono::steady_clock::time_point go_recv_time{};
    std::vector<Move> root_moves;
    std::vector<Syzygy::RootMoveInfo> syzygy_root_moves;
    SearchParams params;
    // Bound lines print past this elapsed time; a test sets 0 to see them.
    double bound_line_after = kBoundLineAfterSeconds;
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

// The value of a node that searched no move: checkmate or stalemate, unless
// a move was excluded. Then the excluded move was the only legal one, which
// proves nothing about the others, so the exclusion search fails low by
// exactly one and a forced move is singular by one ply. Scoring it as mate or
// stalemate put it far below the singular beta, and forced lines took double
// extensions that chained.
[[nodiscard]] inline int no_move_score(bool excluded, bool in_check, int alpha, int ply) {
    if (excluded)
        return alpha;
    return in_check ? -(MATE_SCORE - ply) : 0;
}

// One UCI `info` line in Stockfish's field order: depth seldepth multipv
// score [bound] nodes nps hashfull tbhits time pv. `bound` is empty,
// "lowerbound" or "upperbound". `pv` must already be legal from the root; the
// caller owns any tablebase extension. `nps` floors the elapsed time at 1 ms.
[[nodiscard]] std::string format_info_line(int depth, int seldepth, int multipv, int score,
                             std::string_view bound, int64_t nodes, double elapsed,
                             int64_t tbhits, int hashfull, const std::vector<Move>& pv);


// The longest legal prefix of `line` played from `root`.
[[nodiscard]] std::vector<Move> legal_line(const Board& root, const std::vector<Move>& line);

// Whether a multi-thread search must print the merged result's line before
// `bestmove`: the last line a GUI saw is the main thread's, and it describes
// the merged result only when move and depth agree.
[[nodiscard]] bool needs_pool_line(const SearchResult& merged, const SearchResult& main_thread);
