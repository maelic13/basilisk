/// 8.7.10: baked magic bitboard coverage.
///
/// The non-PEXT tiers ship baked magic constants (src/magics.h) — the
/// deterministic output of find_magic's seeded splitmix64 search — and use them
/// at startup instead of re-running that ~1e8-attempt-per-square search on every
/// launch (which cost ~600 ms on this machine). This test asserts EVERY
/// square's baked magic is valid, i.e. init_attacks() fell back to the live
/// search for ZERO squares. A nonzero count means a stale constant is silently
/// costing startup time (correctness is unaffected — the fallback still finds a
/// working magic), and is the signal to regenerate src/magics.h.
///
/// On PEXT builds there are no magics (PEXT indexes the table directly), so the
/// check is vacuous and the test trivially passes.
///
/// The second test compares every slider lookup, for every square and every
/// subset of its relevant occupancy, with a ray walk, so each tier's table
/// form (magic, pext, or pext with compressed sets) is checked exhaustively.

#include "attacks.h"
#include "test_harness.h"

#include <cstdint>
#include <cstdio>
#include <string>

static void baked_magics_cover_every_square() {
    begin_section("baked magics cover every square (0 search fallbacks)");
    init_attacks();
#if !defined(USE_PEXT)
    EXPECT_EQ(g_baked_magic_fallbacks, 0);
#else
    EXPECT(true);  // PEXT tier: no magic bitboards to bake
#endif
    end_section();
}

// Squares a slider on sq reaches through occ along the given steps.
static Bitboard ray_walk(int sq, Bitboard occ, const int (&steps)[4][2]) {
    Bitboard attacks = 0;
    for (const auto& step : steps) {
        int file = sq % 8 + step[0], rank = sq / 8 + step[1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Bitboard bit = Bitboard(1) << (rank * 8 + file);
            attacks |= bit;
            if (occ & bit) break;
            file += step[0];
            rank += step[1];
        }
    }
    return attacks;
}

// The relevant occupancy: each ray without its last square.
static Bitboard relevant(int sq, const int (&steps)[4][2]) {
    Bitboard mask = 0;
    for (const auto& step : steps) {
        int file = sq % 8 + step[0], rank = sq / 8 + step[1];
        while (file + step[0] >= 0 && file + step[0] < 8 && rank + step[1] >= 0 && rank + step[1] < 8) {
            mask |= Bitboard(1) << (rank * 8 + file);
            file += step[0];
            rank += step[1];
        }
    }
    return mask;
}

static void slider_lookups_match_a_ray_walk() {
    begin_section("every slider lookup matches a ray walk");
    init_attacks();
    static constexpr int rook[4][2]   = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    static constexpr int bishop[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    int64_t checked = 0, wrong = 0;
    for (int sq = 0; sq < 64; sq++) {
        for (int piece = 0; piece < 2; piece++) {
            const auto& steps = piece == 0 ? rook : bishop;
            const Bitboard mask = relevant(sq, steps);
            Bitboard occ = 0;
            do {
                // Blockers outside the mask, the edge squares among them, change nothing.
                const Bitboard noisy = occ | ~mask;
                const Bitboard got = piece == 0 ? rook_attacks(Square(sq), noisy)
                                                : bishop_attacks(Square(sq), noisy);
                if (got != ray_walk(sq, noisy, steps)) wrong++;
                checked++;
                occ = (occ - mask) & mask;
            } while (occ);
        }
    }
    std::printf("  %lld occupancies checked\n", static_cast<long long>(checked));
    EXPECT_EQ(checked, int64_t(107648));
    EXPECT_EQ(wrong, int64_t(0));
    end_section();
}

int main() {
    std::printf("Baked-magic coverage and slider lookup test\n");
    std::printf("%s\n", std::string(62, '=').c_str());
    baked_magics_cover_every_square();
    slider_lookups_match_a_ray_walk();
    return harness_summary();
}
