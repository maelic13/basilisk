#pragma once

#include <string>
#include <vector>

#include "Board.h"

class Parameters {
public:
    Board board;

    int moveTime;           // [ms]
    int whiteTime;          // [ms]
    int whiteIncrement;     // [ms]
    int blackTime;          // [ms]
    int blackIncrement;     // [ms]
    int depth;

    int     moveOverhead;   // [ms]
    int     hash_mb;        // TT size in MB
    int     threads;        // search worker count
    int64_t nodes;          // node limit (0 = unlimited)
    int     movestogo;      // moves until next time control (0 = sudden death)
    bool    ponder;         // go ponder mode
    std::string syzygyPath; // semicolon-separated Syzygy tablebase paths
    int     syzygyProbeDepth;
    int     syzygyProbeLimit;
    bool    syzygy50MoveRule;

    bool new_game    = false;  // set by "ucinewgame", cleared after engine processes it
    bool clear_hash  = false;  // set by "setoption name Clear Hash", cleared after engine clears TT

    Parameters();

    void reset();

    void resetTemporaryParameters();

    void setOption(const std::string &args);

    void setPosition(const std::string &args);

    void setSearchParameters(const std::string &args);

    static std::string uciOptions();
    static int maxThreads();

private:
    void setSearchParameter(const std::string &parameter, const std::string &value);

    static std::vector<std::string> searchParameters();
};

