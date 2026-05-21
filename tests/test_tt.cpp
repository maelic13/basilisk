/// Transposition table tests.
///
/// Covers: store/probe cycle, key miss, mate-score adjustments (to/from TT),
/// replacement policy (deeper entry wins), age cycling, hashfull, clear().
///
/// Build:
///   cmake --build --preset release --target test_tt
///   ./build/release/test_tt

#include "tt.h"
#include "move.h"
#include "types.h"
#include "test_harness.h"

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_store_probe() {
    TranspositionTable tt(1);

    const Key  key    = 0x1234567890ABCDEFULL;
    const int  depth  = 5;
    const int  score  = 150;
    const int  seval  = 120;
    const Move m      = make_move(E2, E4);

    tt.store(key, depth, score, TT_EXACT, m, /*ply=*/0, seval);

    TTEntry e{};
    bool found = tt.probe_copy(key, e);

    begin_section("probe: found == true after store");
    EXPECT(found);
    end_section();

    begin_section("probe: flag == TT_EXACT");
    EXPECT(found && (e.flag_age & 3) == TT_EXACT);
    end_section();

    begin_section("probe: depth matches");
    EXPECT(found && e.depth == depth);
    end_section();

    begin_section("probe: score round-trips at ply 0");
    if (found) {
        int out = TranspositionTable::score_from_tt(e.score, 0);
        EXPECT_EQ(out, score);
    } else { ++g_total; }
    end_section();

    begin_section("probe: static_eval matches");
    EXPECT(found && e.static_eval == seval);
    end_section();

    begin_section("probe: move matches");
    EXPECT(found && move_from_tt(e.move16) == m);
    end_section();
}

static void test_key_miss() {
    TranspositionTable tt(1);
    const Key stored = 0xDEADBEEF00000001ULL;
    const Key other  = 0xCAFEBABE00000002ULL;

    tt.store(stored, 4, 100, TT_EXACT, MOVE_NONE, 0, 100);

    TTEntry e{};
    bool found = tt.probe_copy(other, e);

    begin_section("key miss: found == false for different key");
    EXPECT(!found);
    end_section();
}

static void test_clear() {
    TranspositionTable tt(1);
    const Key key = 0xABCDEF1234567890ULL;

    tt.store(key, 3, 50, TT_BETA, MOVE_NONE, 0, 50);

    TTEntry e{};
    bool found = tt.probe_copy(key, e);
    EXPECT(found); // sanity — should be found

    tt.clear();
    found = tt.probe_copy(key, e);

    begin_section("clear: entry gone after clear()");
    EXPECT(!found);
    end_section();
}

static void test_mate_score_adjustment() {
    // Normal scores pass through unchanged
    begin_section("score_to_tt: normal score unchanged");
    EXPECT_EQ(TranspositionTable::score_to_tt(200, 5), 200);
    end_section();

    begin_section("score_from_tt: normal score unchanged");
    EXPECT_EQ(TranspositionTable::score_from_tt(200, 5), 200);
    end_section();

    // Boundary: just below mate threshold is unchanged
    const int just_below = TranspositionTable::MATE_SCORE - TranspositionTable::MAX_PLY - 1;
    begin_section("score_to_tt: below-threshold score unchanged");
    EXPECT_EQ(TranspositionTable::score_to_tt(just_below, 5), just_below);
    end_section();

    // Mate in 3 (MATE_SCORE - 3 = 31997).  At ply 5:
    //   to_tt:   31997 + 5 = 32002
    //   from_tt: 32002 - 5 = 31997
    const int mate3 = TranspositionTable::MATE_SCORE - 3;
    int stored = TranspositionTable::score_to_tt(mate3, 5);

    begin_section("mate score: to_tt adds ply");
    EXPECT_EQ(stored, mate3 + 5);
    end_section();

    begin_section("mate score: round-trip at ply 5");
    EXPECT_EQ(TranspositionTable::score_from_tt(stored, 5), mate3);
    end_section();

    // Mated in 5 (−MATE_SCORE + 5 = −31995).  At ply 3:
    //   to_tt:   −31995 − 3 = −31998
    //   from_tt: −31998 + 3 = −31995
    const int mated5 = -(TranspositionTable::MATE_SCORE - 5);
    int stored2 = TranspositionTable::score_to_tt(mated5, 3);

    begin_section("mated score: to_tt subtracts ply");
    EXPECT_EQ(stored2, mated5 - 3);
    end_section();

    begin_section("mated score: round-trip at ply 3");
    EXPECT_EQ(TranspositionTable::score_from_tt(stored2, 3), mated5);
    end_section();

    // Different plies must be undone correctly
    const int mate1 = TranspositionTable::MATE_SCORE - 1;
    for (int ply = 0; ply <= 5; ply++) {
        char label[64];
        std::snprintf(label, sizeof(label), "mate round-trip ply %d", ply);
        begin_section(label);
        int s = TranspositionTable::score_to_tt(mate1, ply);
        EXPECT_EQ(TranspositionTable::score_from_tt(s, ply), mate1);
        end_section();
    }
}

static void test_hashfull() {
    TranspositionTable tt(1);

    begin_section("hashfull: 0 on empty table");
    EXPECT_EQ(tt.hashfull(), 0);
    end_section();

    // Fill many entries
    for (int i = 1; i <= 1000; i++) {
        Key k = static_cast<Key>(i) * 0x9E3779B97F4A7C15ULL;
        tt.store(k, 3, i % 400 - 200, TT_EXACT, MOVE_NONE, 0,
                 TranspositionTable::INF_EVAL);
    }

    begin_section("hashfull: > 0 after many stores");
    EXPECT(tt.hashfull() > 0);
    end_section();

    begin_section("hashfull: <= 1000 (valid permille range)");
    EXPECT(tt.hashfull() <= 1000);
    end_section();
}

static void test_deeper_entry_preferred() {
    // Storing a deeper exact entry for the same key should update the slot.
    TranspositionTable tt(1);
    const Key key = 0x2222222222222222ULL;

    tt.store(key, 2, 50,  TT_ALPHA, MOVE_NONE, 0, 50);
    tt.store(key, 8, 120, TT_EXACT, MOVE_NONE, 0, 120);

    TTEntry e{};
    bool found = tt.probe_copy(key, e);

    begin_section("deeper entry: found");
    EXPECT(found);
    end_section();

    begin_section("deeper entry: depth == 8");
    EXPECT(found && e.depth == 8);
    end_section();

    begin_section("deeper entry: flag == TT_EXACT");
    EXPECT(found && (e.flag_age & 3) == TT_EXACT);
    end_section();
}

static void test_age_replacement() {
    // After many new_search() calls the entry ages.  Re-storing a fresh entry
    // at the same key should update the score.
    TranspositionTable tt(1);
    const Key key = 0x3333333333333333ULL;

    tt.store(key, 4, 100, TT_EXACT, MOVE_NONE, 0, 100);

    // Advance many generations
    for (int i = 0; i < 20; i++) tt.new_search();

    // Re-store with a different score at same key (deeper)
    tt.store(key, 6, 200, TT_EXACT, MOVE_NONE, 0, 200);

    TTEntry e{};
    bool found = tt.probe_copy(key, e);

    begin_section("age: re-store same key updates score");
    EXPECT(found);
    if (found) {
        int s = TranspositionTable::score_from_tt(e.score, 0);
        EXPECT_EQ(s, 200);
    } else { ++g_total; }
    end_section();
}

static void test_tt_flags() {
    TranspositionTable tt(1);
    const Key key_ex = 0xAAAAAAAAAAAAAAAAULL;
    const Key key_al = 0xBBBBBBBBBBBBBBBBULL;
    const Key key_be = 0xCCCCCCCCCCCCCCCCULL;

    tt.store(key_ex, 5,  50, TT_EXACT, MOVE_NONE, 0, 50);
    tt.store(key_al, 5, -30, TT_ALPHA, MOVE_NONE, 0, -30);
    tt.store(key_be, 5,  80, TT_BETA,  MOVE_NONE, 0, 80);

    bool found;
    TTEntry e{};

    begin_section("flag: TT_EXACT stored and retrieved");
    found = tt.probe_copy(key_ex, e);
    EXPECT(found && (e.flag_age & 3) == TT_EXACT);
    end_section();

    begin_section("flag: TT_ALPHA stored and retrieved");
    found = tt.probe_copy(key_al, e);
    EXPECT(found && (e.flag_age & 3) == TT_ALPHA);
    end_section();

    begin_section("flag: TT_BETA stored and retrieved");
    found = tt.probe_copy(key_be, e);
    EXPECT(found && (e.flag_age & 3) == TT_BETA);
    end_section();
}

static void test_move_preserved() {
    // The move stored in TT should survive a probe
    TranspositionTable tt(1);
    const Key key = 0x4444444444444444ULL;
    const Move m  = make_promotion(B7, B8, QUEEN);

    tt.store(key, 6, 900, TT_EXACT, m, 0, 900);

    TTEntry e{};
    bool found = tt.probe_copy(key, e);

    begin_section("move preserved: found");
    EXPECT(found);
    end_section();

    begin_section("move preserved: promo queen round-trip");
    if (found) {
        Move rt = move_from_tt(e.move16);
        EXPECT_EQ(rt, m);
    } else { ++g_total; }
    end_section();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::printf("Transposition table tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nStore / probe\n");
    test_store_probe();

    std::printf("\nKey miss\n");
    test_key_miss();

    std::printf("\nClear\n");
    test_clear();

    std::printf("\nMate score adjustment\n");
    test_mate_score_adjustment();

    std::printf("\nHashfull\n");
    test_hashfull();

    std::printf("\nReplacement policy\n");
    test_deeper_entry_preferred();

    std::printf("\nAge replacement\n");
    test_age_replacement();

    std::printf("\nTT flags\n");
    test_tt_flags();

    std::printf("\nMove preservation\n");
    test_move_preserved();

    return harness_summary();
}
