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

    int moveOverhead;       // [ms]
    int hash_mb;            // TT size in MB

    bool new_game = false;  // set by "ucinewgame", cleared after engine processes it

    Parameters();

    void reset();

    void resetTemporaryParameters();

    void setOption(const std::string &args);

    void setPosition(const std::string &args);

    void setSearchParameters(const std::string &args);

    static std::string uciOptions();

private:
    void setSearchParameter(const std::string &parameter, const std::string &value);

    static std::vector<std::string> searchParameters();
};

