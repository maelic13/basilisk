#ifndef BASILISK_CONSTANTS_H
#define BASILISK_CONSTANTS_H

#include <limits>
#include <string>

const std::string engineName = "Basilisk";
const std::string engineVersion = "0.1";
const std::string engineAuthor = "Miloslav Macurek";

const int defaultDepth = 2;
const int defaultMoveOverhead = 10;
const int infiniteDepth = std::numeric_limits<int>::max();
const std::string startPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

#endif //BASILISK_CONSTANTS_H
