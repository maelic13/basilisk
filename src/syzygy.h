#pragma once

#include "board.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Syzygy {

enum class Wdl : int {
    Loss = -2,
    BlessedLoss = -1,
    Draw = 0,
    CursedWin = 1,
    Win = 2,
};

struct RootProbeResult {
    Move bestmove = MOVE_NONE;
    int score = 0;
    int rank = 0;
    bool used_dtz = false;
};

struct RootMoveInfo {
    Move bestmove = MOVE_NONE;
    int score = 0;
    int rank = 0;
    bool used_dtz = false;
};

bool init(const std::string& path);
void clear();
bool enabled();
int largest();
int wdl_file_count();
int dtz_file_count();
std::string path();

bool can_probe_wdl(const Board& board, int probe_limit = 7, bool use_rule50 = true);
bool can_probe_root(const Board& board, int probe_limit = 7);

std::optional<Wdl> probe_wdl(const Board& board, int probe_limit = 7, bool use_rule50 = true);
std::optional<RootProbeResult> probe_root(const Board& board, bool use_rule50,
                                          int probe_limit = 7);
// find_preferred = false skips the extra tb_probe_root() call that only breaks
// ties between equally ranked moves -- half the cost, for callers that do not
// need that tie-break.
std::vector<RootMoveInfo> probe_root_moves(const Board& board, bool use_rule50,
                                           int probe_limit = 7, bool rank_dtz = false,
                                           bool find_preferred = true);

struct PvExtension {
    std::vector<Move> pv;
    bool ends_in_draw = false;  // the line ends in a fifty-move draw: show score 0
    bool timed_out    = false;  // time_abort() cut the extension short
};

// Stockfish's syzygy_extend_pv (search.cpp, official master 2026-09-22), for
// a PV whose score is a decisive tablebase score. Step 1 keeps the searched PV
// for as long as every move keeps the best tablebase rank, and cuts it at the
// first move that does not, or that reaches a draw. Step 2 extends it towards
// mate with minimal-DTZ moves, breaking ties by leaving the opponent the least
// mobility (a capture reply counts 100). The result is a plausible
// continuation, not a proven mating line. time_abort() is polled throughout;
// the caller decides the budget.
PvExtension extend_pv(const Board& root, const std::vector<Move>& pv, bool use_rule50,
                      int probe_limit, const std::function<bool()>& time_abort);

} // namespace Syzygy
