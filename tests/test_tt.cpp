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

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>

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

// BAS-C05. The predecessor of this test constructed a torn key/payload pair and
// asserted the reader rejected it. Under the current layout that test cannot be
// written: the validated record is one 64-bit word, so a torn pair is
// unrepresentable rather than merely detected. The invariant worth asserting is
// therefore structural — every field that can decide a bound is genuinely
// carried by that one word, and none has been quietly moved out to make room.
static void test_validated_record_is_one_publication() {
    // Extremes at the real limits: deepest mate band, the INF_EVAL sentinel,
    // the depth=-1 qsearch sentinel, a full age byte.
    constexpr int mate_band = -(TranspositionTable::MATE_SCORE + TranspositionTable::MAX_PLY);
    const uint64_t word = tt_pack(0xBEEF, mate_band, TranspositionTable::INF_EVAL, -1, 0xFC);
    const TTEntry e = tt_unpack(word);

    begin_section("validated record: key/score/eval/depth/flag round-trip one word");
    EXPECT(e.key16 == 0xBEEF);
    EXPECT(e.score == mate_band);
    EXPECT(e.static_eval == TranspositionTable::INF_EVAL);
    EXPECT(e.depth == -1);
    EXPECT(e.flag_age == 0xFC);
    end_section();

    begin_section("validated record: the key is the low 16 bits probe_copy compares");
    EXPECT(static_cast<uint16_t>(word) == 0xBEEF);
    end_section();

    // Negative control for "a field was moved out of the word to make room":
    // perturbing any single validated field must change the published word.
    begin_section("validated record: every field is actually carried by the word");
    EXPECT(tt_pack(0xBEEE, mate_band, TranspositionTable::INF_EVAL, -1, 0xFC) != word);
    EXPECT(tt_pack(0xBEEF, mate_band + 1, TranspositionTable::INF_EVAL, -1, 0xFC) != word);
    EXPECT(tt_pack(0xBEEF, mate_band, TranspositionTable::INF_EVAL - 1, -1, 0xFC) != word);
    EXPECT(tt_pack(0xBEEF, mate_band, TranspositionTable::INF_EVAL, 0, 0xFC) != word);
    EXPECT(tt_pack(0xBEEF, mate_band, TranspositionTable::INF_EVAL, -1, 0xF8) != word);
    end_section();

    // An all-zero slot must still decode to key16 0 with TT_NONE, so a position
    // whose partial key is 0 does not read an empty slot as a hit.
    begin_section("validated record: an empty slot decodes to key16 0 and TT_NONE");
    EXPECT(tt_unpack(0).key16 == 0);
    EXPECT((tt_unpack(0).flag_age & 3) == TT_NONE);
    end_section();

    TranspositionTable tt(1);
    TTEntry probed{};
    begin_section("validated record: a zero partial key does not hit an empty slot");
    EXPECT(!tt.probe_copy(0x0000000000001234ULL, probed));
    end_section();
}

// The other half of the contract, asserted rather than assumed: `move16` lives
// outside the guarantee (DESIGN.md section 3). A move from a different
// publication cannot corrupt the record it arrives with, and every consumer
// validates it before use.
static void test_move_is_outside_the_guarantee() {
    TTCluster cluster;
    const uint64_t word = tt_pack(0x1357, 100, 200, 7, static_cast<uint8_t>(0x04 | TT_EXACT));
    cluster.store(0, word, static_cast<uint16_t>(move_to_tt(make_move(E2, E4))));

    // Simulate a later publication of a different position landing on this slot
    // between a reader's word load and its move load.
    cluster.moves[0].store(static_cast<uint16_t>(move_to_tt(make_move(D2, D4))),
                           std::memory_order_relaxed);

    const TTEntry e = tt_unpack(cluster.load_word(0));

    begin_section("move16: a foreign move cannot corrupt the validated record");
    EXPECT(e.key16 == 0x1357);
    EXPECT(e.score == 100);
    EXPECT(e.static_eval == 200);
    EXPECT(e.depth == 7);
    EXPECT((e.flag_age & 3) == TT_EXACT);
    end_section();

    // Documented, deliberate tolerance — not an accident. If this ever starts
    // returning the original move, the contract has changed and DESIGN.md
    // section 3 must change with it.
    begin_section("move16: it is explicitly NOT guaranteed to match the record");
    EXPECT(move_from_tt(cluster.load_move(0)) == make_move(D2, D4));
    end_section();
}

static void test_concurrent_publication_stays_coherent() {
    TranspositionTable tt(1);
    constexpr Key key_a = 0x1111000000000042ULL;
    constexpr Key key_b = 0x2222000000000042ULL;
    constexpr int iterations = 200000;
    std::atomic<bool> start{false};
    std::atomic<bool> failed{false};

    auto writer = [&](Key key, int score, int static_eval, Move move) {
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < iterations; ++i)
            tt.store(key, 12, score, TT_EXACT, move, 0, static_eval);
    };
    const Move move_a = make_move(E2, E4);
    const Move move_b = make_move(D2, D4);
    std::atomic<bool> move_torn{false};

    // The validated fields must all belong to THIS key's publication, key16
    // included. The move is checked only for being a value some thread actually
    // published — it is a single atomic field, so it can be foreign but never a
    // mixture of two moves. MOVE_NONE is admissible: it is the constructor's
    // value, and on a weakly ordered target it can still be visible when the
    // word from the store that replaced it already is.
    auto reader = [&](Key key, int score, int static_eval) {
        const auto key16 = static_cast<uint16_t>(key >> 48);
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < iterations; ++i) {
            TTEntry entry{};
            if (!tt.probe_copy(key, entry))
                continue;
            if (entry.key16 != key16 || entry.score != score
                || entry.static_eval != static_eval || entry.depth != 12
                || (entry.flag_age & 3) != TT_EXACT) {
                failed.store(true, std::memory_order_relaxed);
                return;
            }
            const Move seen = move_from_tt(entry.move16);
            if (seen != move_a && seen != move_b && seen != MOVE_NONE) {
                move_torn.store(true, std::memory_order_relaxed);
                return;
            }
        }
    };

    std::thread writer_a(writer, key_a, 111, 211, move_a);
    std::thread writer_b(writer, key_b, 122, 222, move_b);
    std::thread reader_a(reader, key_a, 111, 211);
    std::thread reader_b(reader, key_b, 122, 222);
    start.store(true, std::memory_order_release);
    writer_a.join();
    writer_b.join();
    reader_a.join();
    reader_b.join();

    begin_section("concurrent publication: every validated field matches the key");
    EXPECT(!failed.load(std::memory_order_relaxed));
    end_section();

    begin_section("concurrent publication: move16 is never a mixture of two moves");
    EXPECT(!move_torn.load(std::memory_order_relaxed));
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

    begin_section("score_from_tt: non-mate scores draw at 50-move limit");
    EXPECT_EQ(TranspositionTable::score_from_tt(200, 5, 100), 0);
    EXPECT_EQ(TranspositionTable::score_from_tt(-200, 5, 120), 0);
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

    begin_section("score_from_tt: mate scores ignore 50-move clamp");
    EXPECT_EQ(TranspositionTable::score_from_tt(stored, 5, 100), mate3);
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

    begin_section("hashfull: ignores entries from older generation");
    tt.new_search();
    EXPECT_EQ(tt.hashfull(), 0);
    end_section();
}

static void test_prefetch_is_safe() {
    TranspositionTable tt(1);

    begin_section("prefetch: empty key is safe");
    tt.prefetch(0);
    EXPECT(true);
    end_section();

    begin_section("prefetch: stored key is safe");
    const Key key = 0x5151515151515151ULL;
    tt.store(key, 4, 12, TT_EXACT, make_move(E2, E4), 0, 12);
    tt.prefetch(key);
    EXPECT(true);
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

/// The `Hash` byte contract (8.6.2a). Two bounds, both load-bearing:
///   upper — allocating MORE than the user asked for is the bug class that bit
///           the sibling engine (it sized a table in another structure's unit
///           and silently used 2x the configured Hash; in a tournament that
///           surfaces as swapping and time losses that read as weakness);
///   lower — the power-of-two cluster floor may waste at most half the budget,
///           so anything <= budget/2 means the sizing math itself regressed.
/// Verified as a real guard, not a tautology (negative controls run 2026-07-20,
/// both reverted): sizing the count in the WRONG UNIT — `bytes / 16` instead of
/// `bytes / sizeof(TTCluster)`, i.e. precisely the sibling-engine bug — fails
/// the upper bound (11 assertions, exit 1); under-allocating via
/// `bytes / (sizeof(TTCluster) * 4)` fails the lower bound (11 assertions).
/// Note the bounds are checked through `allocated_bytes()`, which derives from
/// `cluster_count_` — the same value `resize()` hands to `make_unique`, so it
/// is the true footprint and not an independently-drifting number.
static void test_allocation_within_budget() {
    static const size_t sizes_mb[] = {1, 2, 3, 4, 16, 17, 64, 100, 129};

    for (size_t mb : sizes_mb) {
        TranspositionTable tt(mb);
        const size_t budget = mb * 1024u * 1024u;

        char name[64];
        std::snprintf(name, sizeof(name), "Hash %zu MB: allocation within budget", mb);
        begin_section(name);
        EXPECT(tt.allocated_bytes() <= budget);
        EXPECT(tt.allocated_bytes() > budget / 2);
        end_section();
    }
}

/// The same contract must survive a resize in both directions — the path that
/// also exercises 8.6.2a's free-before-allocate ordering (the old table must be
/// released before the new one is requested, or a grow transiently needs the
/// sum of both).
static void test_resize_keeps_budget() {
    TranspositionTable tt(1);

    begin_section("resize grow 1 -> 64 MB stays within budget");
    tt.resize(64);
    EXPECT(tt.allocated_bytes() <= 64u * 1024u * 1024u);
    EXPECT(tt.allocated_bytes() > 32u * 1024u * 1024u);
    end_section();

    begin_section("resize shrink 64 -> 2 MB stays within budget");
    tt.resize(2);
    EXPECT(tt.allocated_bytes() <= 2u * 1024u * 1024u);
    EXPECT(tt.allocated_bytes() > 1u * 1024u * 1024u);
    end_section();

    begin_section("table is usable after resize");
    const Key key = 0xFEEDFACECAFEBEEFULL;
    tt.store(key, 7, 42, TT_EXACT, make_move(E2, E4), /*ply=*/0, 30);
    TTEntry e{};
    EXPECT(tt.probe_copy(key, e) && e.score == 42);
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

    std::printf("\nPublication coherence\n");
    test_validated_record_is_one_publication();
    test_move_is_outside_the_guarantee();
    test_concurrent_publication_stays_coherent();

    std::printf("\nClear\n");
    test_clear();

    std::printf("\nMate score adjustment\n");
    test_mate_score_adjustment();

    std::printf("\nHashfull\n");
    test_hashfull();

    std::printf("\nPrefetch\n");
    test_prefetch_is_safe();

    std::printf("\nReplacement policy\n");
    test_deeper_entry_preferred();

    std::printf("\nAge replacement\n");
    test_age_replacement();

    std::printf("\nTT flags\n");
    test_tt_flags();

    std::printf("\nMove preservation\n");
    test_move_preserved();

    std::printf("\nHash byte contract\n");
    test_allocation_within_budget();
    test_resize_keeps_budget();

    return harness_summary();
}
