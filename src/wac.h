#pragma once

// Win At Chess (WAC) tactical suite — a bench-style diagnostic, not a gate.
//
// 300 classic tactical positions (src/wac.epd, standard public EPD with `bm`
// best moves in SAN). The `wac [depth]` engine command searches each position
// to a fixed depth and reports how many found an accepted best move, plus the
// ids of the failures. The solved count is a *tactical-regression telltale*
// for search-selectivity work (a sudden drop localizes a tactical regression
// long before an SPRT can) — it is NOT a strength metric and never gates a
// change by itself (SPRT remains the only verdict).
//
// Like bench, runs are deterministic: TT and histories are reset per
// position, so the solved set is reproducible at a given depth and safe to
// compare across candidates. tests/test_wac.cpp holds a floor test guarding
// against gross tactical breakage (SaberTooth's CI pattern: a pass-count
// floor, not all-pass). Feature mirrored from sibling engine Rarog
// (src/wac.rs).

#include <string>
#include <vector>

#include "board.h"

inline constexpr int DEFAULT_WAC_DEPTH = 10;

// One suite entry: a position, its accepted best moves (SAN), and its id.
struct WacPosition {
    std::string fen;
    std::vector<std::string> best_moves;
    std::string id;
};

// Parses the embedded EPD into positions. EPD carries no move counters, so
// "0 1" is appended to form a FEN. Malformed lines are skipped (the suite is
// static; the unit test asserts the full 300 parse).
std::vector<WacPosition> wac_positions();

// Whether `mv` matches one SAN token in `board`. Handles castling
// (O-O/O-O-O), promotions (=Q), piece letters, 'x', '+'/'#' suffixes, and
// file/rank disambiguation hints. Legality/uniqueness is the suite's
// responsibility — `mv` comes from the search, so only faithful matching
// matters here.
bool wac_san_matches(const Board& board, Move mv, const std::string& san);

// Whether `mv` (legal in `board`) is one of the accepted SAN best moves.
bool wac_move_matches_any(const Board& board, Move mv,
                          const std::vector<std::string>& best_moves);

// wac [depth] — run the full suite at a fixed depth, print per-position
// progress and the solved-count summary (single-threaded, deterministic).
void run_wac(int depth = DEFAULT_WAC_DEPTH);
