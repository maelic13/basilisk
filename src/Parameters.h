#ifndef BASILISK_PARAMETERS_H
#define BASILISK_PARAMETERS_H

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

    Parameters();

    static std::string uciOptions();

    void reset();

    void resetTemporaryParameters();

    void setOption(const std::string &args);

    void setPosition(const std::string &args);

    void setSearchParameters(const std::string &args);
};

#endif //BASILISK_PARAMETERS_H
