#pragma once

#include "Board.h"

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
    std::vector<Move> pv;
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
std::vector<RootMoveInfo> probe_root_moves(const Board& board, bool use_rule50,
                                           int probe_limit = 7, bool rank_dtz = false);
std::vector<Move> extend_pv(const Board& root, const std::vector<Move>& initial_pv,
                            bool use_rule50, int probe_limit = 7, int max_plies = 64);

} // namespace Syzygy
