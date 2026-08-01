#pragma once

#include <limits>
#include <string_view>

inline constexpr std::string_view engineName    = "Basilisk";
inline constexpr std::string_view engineVersion = "1.9.3";
inline constexpr std::string_view engineAuthor  = "Miloslav Macurek";

// Default search limit when "go" is sent without explicit limits.
inline constexpr int defaultMoveOverhead = 10;
inline constexpr int defaultSearchDepth  = 7;
inline constexpr int tablebaseWinScore   = 20000;

inline constexpr int infiniteDepth = std::numeric_limits<int>::max();

// Upper bound on the `Threads` UCI option: a flat constant, matching
// Stockfish, which declares its own as Option(1, 1, 1024) with no reference to
// the host's core count. Choosing a sensible thread count is the operator's
// job — the engine's job is not to second-guess it. A machine-derived cap also
// misfires exactly where it would matter, since hardware_concurrency() is the
// value that under-reports inside containers and cgroups.
//
// 9.3(a) originally made this min(1024, 4*hw). That tracked the machine but
// bought nothing over the flat bound on real hardware (user decision
// 2026-07-30 — see PLAN §5 9.3).
//
// Still defined ONCE, in search.cpp, because two places read it and must not
// disagree: the option advertisement (Parameters::max_threads) and the pool
// that actually allocates (SearchThreadPool::normalize_thread_count).
// Advertising a maximum the pool then silently clamps is a protocol lie.
// The real backstop against an absurd value is the graceful
// "Threads reduced to N" path on allocation/creation failure, not the cap.
inline constexpr int maxSearchThreads = 1024;
int max_search_threads();

inline constexpr std::string_view startPosition =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
