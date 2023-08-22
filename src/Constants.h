#ifndef ENGINE_CONSTANTS_H
#define ENGINE_CONSTANTS_H

#include <string>

const std::string engineName = "Basilisk";
const std::string engineVersion = "0.1";
const std::string engineAuthor = "Miloslav Macurek";

const int defaultMoveOverhead = 10;
const int infiniteDepth = std::numeric_limits<int>::max();
std::string startPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

#endif //ENGINE_CONSTANTS_H
