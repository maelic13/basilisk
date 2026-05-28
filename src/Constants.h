#pragma once

#include <limits>
#include <string_view>

inline constexpr std::string_view engineName    = "Basilisk";
inline constexpr std::string_view engineVersion = "1.4.4";
inline constexpr std::string_view engineAuthor  = "Miloslav Macurek";

// Default search limit when "go" is sent without explicit limits.
inline constexpr int defaultMoveOverhead = 10;
inline constexpr int defaultSearchDepth  = 7;
inline constexpr int tablebaseWinScore   = 20000;

inline constexpr int infiniteDepth = std::numeric_limits<int>::max();

inline constexpr std::string_view startPosition =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
