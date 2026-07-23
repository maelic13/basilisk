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

#include "attacks.h"
#include "test_harness.h"

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

int main() {
    std::printf("Baked-magic coverage test\n");
    std::printf("%s\n", std::string(62, '=').c_str());
    baked_magics_cover_every_square();
    return harness_summary();
}
