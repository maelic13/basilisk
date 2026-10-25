#pragma once

#include <string>
#include <string_view>

// The name the engine reports, in Stockfish's form. A release reads
// "Basilisk 1.11.0". A development build (an engineVersion ending in "-dev")
// reads "Basilisk dev-<commit date>-<8 hash characters>", with "-m" after the
// date when the working tree differed from the commit, and "nogit" in place
// of the hash, after the compile date, when git was not available.
std::string engine_version_info();

// The formatting itself, separate so it can be tested without a build's git
// state. An empty date means the compile date is used.
std::string format_engine_version(std::string_view name, std::string_view version,
                                  std::string_view git_date, std::string_view git_sha,
                                  bool modified, std::string_view dev_id = {});
