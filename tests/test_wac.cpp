/// WAC tactical-suite tests (Rarog-mirrored feature; SaberTooth CI pattern).
///
/// 1. Parse/matcher correctness: all 300 suite positions parse and every
///    accepted SAN matches exactly one legal move — guards both the matcher
///    and the suite against typos/ambiguity.
/// 2. Solved-count floor: runs the full suite at a shallow fixed depth and
///    asserts a conservative pass-count floor (a floor, not all-pass, so
///    normal search evolution doesn't flap the suite — only a collapse, i.e.
///    a real tactical regression, fails it). `wac [depth]` (the engine
///    command) is the fine-grained per-step diagnostic; this is the tripwire.
///
/// Build:
///   cmake --build --preset release --target test_wac
///   ./build/release/test_wac

#include "board.h"
#include "wac.h"
#include "eval.h"
#include "move.h"
#include "search.h"
#include "tt.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"
#include "test_harness.h"

#include <atomic>
#include <cstdio>
#include <memory>
#include <string>

static const int TEST_DEPTH = 6;
// Calibrated 2026-07-15 (8.3 head after the 8.4 revert, bench 11,555,879):
// solved 145/300 at depth 6 in ~4s. The floor leaves ~10% headroom for
// benign search-shape drift; only a collapse (a real tactical regression)
// fails it.
static const int FLOOR = 130;

static void test_suite_parses_and_matcher_is_exact() {
    const std::vector<WacPosition> positions = wac_positions();

    begin_section("full 300-position suite parses");
    EXPECT_EQ(static_cast<int>(positions.size()), 300);
    end_section();

    begin_section("every accepted SAN matches exactly one legal move");
    bool all_ok = true;
    for (const WacPosition& pos : positions) {
        Board b;
        auto r = b.try_set_fen(pos.fen, /*validate_legal_position=*/true);
        if (!r) {
            std::fprintf(stderr, "  %s illegal FEN: %s\n", pos.id.c_str(), r.error().c_str());
            all_ok = false;
            continue;
        }
        if (pos.best_moves.empty()) {
            std::fprintf(stderr, "  %s has no bm\n", pos.id.c_str());
            all_ok = false;
            continue;
        }
        MoveList legal;
        b.gen_legal(legal);
        for (const std::string& san : pos.best_moves) {
            int matches = 0;
            for (int i = 0; i < legal.size(); ++i)
                if (wac_san_matches(b, legal[i], san)) ++matches;
            if (matches != 1) {
                std::fprintf(stderr, "  %s: '%s' matched %d moves\n",
                             pos.id.c_str(), san.c_str(), matches);
                all_ok = false;
            }
        }
    }
    EXPECT(all_ok);
    end_section();
}

static void test_san_matcher_rejects_wrong_piece_and_destination() {
    // WAC.001's answer is Qg6: exactly one legal move matches, and it is a
    // queen move to g6.
    begin_section("Qg6 in WAC.001 matches exactly one legal queen move");
    Board b;
    b.set_fen("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    MoveList legal;
    b.gen_legal(legal);
    int matches = 0;
    Move matched = MOVE_NONE;
    for (int i = 0; i < legal.size(); ++i) {
        if (wac_san_matches(b, legal[i], "Qg6")) {
            ++matches;
            matched = legal[i];
        }
    }
    EXPECT_EQ(matches, 1);
    EXPECT(matched != MOVE_NONE
           && type_of(b.board_sq[from_sq(matched)]) == QUEEN
           && to_sq(matched) == G6);
    end_section();
}

static void test_wac_floor() {
    begin_section("solved count stays above the floor");
    const std::vector<WacPosition> positions = wac_positions();

    int solved = 0;
    for (const WacPosition& pos : positions) {
        TranspositionTable tt(16);
        std::atomic_bool stop{false};
        Board b;
        b.set_fen(pos.fen);

        SearchLimits lim;
        lim.depth = TEST_DEPTH;

        auto searcher = std::make_unique<Searcher>(tt, stop);
        SearchResult sr = searcher->search(b, lim);
        if (wac_move_matches_any(b, sr.bestmove, pos.best_moves)) ++solved;
    }

    std::printf("  WAC solved %d/%d at depth %d (floor %d)\n",
                solved, static_cast<int>(positions.size()), TEST_DEPTH, FLOOR);
    EXPECT(solved >= FLOOR);
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("WAC tactical-suite tests\n");
    std::printf("========================\n");

    std::printf("\nSuite parsing and SAN matcher\n");
    test_suite_parses_and_matcher_is_exact();
    test_san_matcher_rejects_wrong_piece_and_destination();

    std::printf("\nSolved-count floor (depth %d)\n", TEST_DEPTH);
    test_wac_floor();

    return harness_summary();
}
