#pragma once

#include "Board.h"

#include <optional>
#include <string>

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

bool init(const std::string& path);
void clear();
bool enabled();
int largest();
std::string path();

bool can_probe_wdl(const Board& board);
bool can_probe_root(const Board& board);

std::optional<Wdl> probe_wdl(const Board& board);
std::optional<RootProbeResult> probe_root(const Board& board, bool use_rule50);

} // namespace Syzygy
