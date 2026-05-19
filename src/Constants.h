#pragma once

#include <limits>
#include <string_view>

inline constexpr std::string_view engineName    = "Basilisk";
inline constexpr std::string_view engineVersion = "0.1.0";
inline constexpr std::string_view engineAuthor  = "Miloslav Macurek";

// Default time per move when no clock information is provided [ms]
inline constexpr int defaultMoveTime     = 500;
inline constexpr int defaultMoveOverhead = 10;

inline constexpr int infiniteDepth = std::numeric_limits<int>::max();

inline constexpr std::string_view startPosition =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
