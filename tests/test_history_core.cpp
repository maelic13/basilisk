/// The selectivity core's history tables: every table's bound
/// and gravity, clear(), the between-search ageing, row addressing and the
/// footprint the architecture review sized.

#include "history_core.h"
#include "test_harness.h"

#include <cstdio>
#include <memory>

template<class Table>
static void check_bound_and_gravity(const char* name) {
    char label[128];
    std::snprintf(label, sizeof(label), "%s: saturates at +/-%d and is monotone", name, Table::LIMIT);
    begin_section(label);
    int16_t e = 0;
    bool within = true;
    bool monotone = true;
    for (int i = 0; i < 10000; ++i) {
        const int16_t before = e;
        Table::update(e, Table::LIMIT / 8);
        within = within && e <= Table::LIMIT && e >= -Table::LIMIT;
        monotone = monotone && e >= before;
    }
    EXPECT(within);
    EXPECT(monotone);
    EXPECT_EQ(e, Table::LIMIT);
    for (int i = 0; i < 10000; ++i) {
        const int16_t before = e;
        Table::update(e, -Table::LIMIT / 8);
        within = within && e <= Table::LIMIT && e >= -Table::LIMIT;
        monotone = monotone && e <= before;
    }
    EXPECT(within);
    EXPECT(monotone);
    EXPECT_EQ(e, -Table::LIMIT);
    // A bonus beyond the bound is clamped first: one step lands on the bound.
    e = 0;
    Table::update(e, 4 * Table::LIMIT);
    EXPECT_EQ(e, Table::LIMIT);
    Table::update(e, -4 * Table::LIMIT);
    EXPECT_EQ(e, -Table::LIMIT);
    // Gravity: the same bonus moves an entry less the closer it is to the bound.
    int16_t low = 0, high = static_cast<int16_t>(Table::LIMIT / 2);
    Table::update(low, Table::LIMIT / 8);
    const int low_step = low;
    const int16_t high_before = high;
    Table::update(high, Table::LIMIT / 8);
    EXPECT(high - high_before < low_step);
    end_section();
}

static void test_bounds() {
    check_bound_and_gravity<MainHistory>("main");
    check_bound_and_gravity<CaptureHistory>("capture");
    check_bound_and_gravity<PieceToHistory>("continuation");
    check_bound_and_gravity<PawnHistoryBucket>("pawn");
    check_bound_and_gravity<LowPlyHistory>("low-ply");
    check_bound_and_gravity<TTMoveHistory>("tt-move");
    check_bound_and_gravity<CorrectionBound>("correction");
    check_bound_and_gravity<PieceToCorrection>("continuation correction");

    begin_section("bounds are the cluster's");
    EXPECT_EQ(MainHistory::LIMIT, 7183);
    EXPECT_EQ(CaptureHistory::LIMIT, 10692);
    EXPECT_EQ(PieceToHistory::LIMIT, 30000);
    EXPECT_EQ(PawnHistoryBucket::LIMIT, 8192);
    EXPECT_EQ(LowPlyHistory::LIMIT, 7183);
    EXPECT_EQ(TTMoveHistory::LIMIT, 8192);
    EXPECT_EQ(CorrectionBound::LIMIT, 1024);
    EXPECT_EQ(PieceToCorrection::LIMIT, 1024);
    EXPECT_EQ(LowPlyHistory::PLIES, 5);
    EXPECT_EQ(PawnHistory::MAX_BUCKETS, 8192);
    end_section();
}

static void fill_some(CoreHistoryTables& h, int v) {
    h.main.entry[BLACK][C3][F6] = static_cast<int16_t>(v);
    h.capture.entry[KNIGHT][E5][QUEEN] = static_cast<int16_t>(v);
    h.low_ply.entry[4][G1][F3] = static_cast<int16_t>(v);
    h.tt_move.value = static_cast<int16_t>(v);
    h.continuation->row[1][0][BISHOP][B5].entry[ROOK][D1] = static_cast<int16_t>(v);
    h.pawn->bucket[PawnHistory::MAX_BUCKETS - 1].entry[QUEEN][H8] = static_cast<int16_t>(v);
    h.pawn_corr[WHITE][7] = static_cast<int16_t>(v);
    h.minor_corr[BLACK][9] = static_cast<int16_t>(v);
    h.nonpawn_corr[WHITE][BLACK][11] = static_cast<int16_t>(v);
    h.cont_corr->row[PAWN][A2].entry[KNIGHT][C3] = static_cast<int16_t>(v);
}

static void test_clear_and_halve() {
    auto h = std::make_unique<CoreHistoryTables>();

    begin_section("a new table set is zero, the sentinel row included");
    bool zero = true;
    for (auto& a : h->continuation->row)
        for (auto& b : a)
            for (auto& c : b)
                for (auto& row : c)
                    for (auto& r : row.entry)
                        for (int16_t x : r) zero = zero && x == 0;
    for (auto& bucket : h->pawn->bucket)
        for (auto& r : bucket.entry)
            for (int16_t x : r) zero = zero && x == 0;
    EXPECT(zero);
    EXPECT(h->sentinel_row() == &h->continuation->row[0][0][NO_PIECE_TYPE][0]);
    EXPECT(h->sentinel_corr_row() == &h->cont_corr->row[NO_PIECE_TYPE][0]);
    end_section();

    begin_section("age: the main history keeps 729/1024, the low-ply history restarts at 102");
    fill_some(*h, 1000);
    h->age(729);
    EXPECT_EQ(h->main.entry[BLACK][C3][F6], 711);             // 1000 * 729 / 1024
    EXPECT_EQ(h->low_ply.entry[4][G1][F3], CoreHistoryTables::LOW_PLY_REFILL);
    EXPECT_EQ(h->low_ply.entry[0][A1][A1], 102);
    EXPECT_EQ(h->capture.entry[KNIGHT][E5][QUEEN], 1000);
    EXPECT_EQ(h->continuation->row[1][0][BISHOP][B5].entry[ROOK][D1], 1000);
    EXPECT_EQ(h->pawn->bucket[PawnHistory::MAX_BUCKETS - 1].entry[QUEEN][H8], 1000);
    EXPECT_EQ(h->pawn_corr[WHITE][7], 1000);
    EXPECT_EQ(h->minor_corr[BLACK][9], 1000);
    EXPECT_EQ(h->nonpawn_corr[WHITE][BLACK][11], 1000);
    EXPECT_EQ(h->cont_corr->row[PAWN][A2].entry[KNIGHT][C3], 1000);
    EXPECT_EQ(h->tt_move.value, 1000);
    fill_some(*h, -1000);
    h->age(729);
    EXPECT_EQ(h->main.entry[BLACK][C3][F6], -711);
    h->age(1024);
    EXPECT_EQ(h->main.entry[BLACK][C3][F6], -711);            // 1024 is no decay
    end_section();

    begin_section("clear: every entry back to zero");
    fill_some(*h, 777);
    h->clear();
    EXPECT_EQ(h->main.entry[BLACK][C3][F6], 0);
    EXPECT_EQ(h->capture.entry[KNIGHT][E5][QUEEN], 0);
    EXPECT_EQ(h->low_ply.entry[4][G1][F3], 0);
    EXPECT_EQ(h->tt_move.value, 0);
    EXPECT_EQ(h->continuation->row[1][0][BISHOP][B5].entry[ROOK][D1], 0);
    EXPECT_EQ(h->pawn->bucket[PawnHistory::MAX_BUCKETS - 1].entry[QUEEN][H8], 0);
    EXPECT_EQ(h->pawn_corr[WHITE][7], 0);
    EXPECT_EQ(h->minor_corr[BLACK][9], 0);
    EXPECT_EQ(h->nonpawn_corr[WHITE][BLACK][11], 0);
    EXPECT_EQ(h->cont_corr->row[PAWN][A2].entry[KNIGHT][C3], 0);
    end_section();
}

static void test_row_addressing() {
    auto h = std::make_unique<CoreHistoryTables>();

    begin_section("a continuation row pointer addresses the full index");
    PieceToHistory* row = &h->continuation->row[1][1][KNIGHT][F3];
    row->entry[BISHOP][C4] = 1234;
    EXPECT_EQ(h->continuation->row[1][1][KNIGHT][F3].entry[BISHOP][C4], 1234);
    EXPECT(&row->entry[BISHOP][C4] == &h->continuation->row[1][1][KNIGHT][F3].entry[BISHOP][C4]);
    end_section();

    begin_section("a continuation-correction row pointer addresses the full index");
    PieceToCorrection* corr_row = &h->cont_corr->row[QUEEN][D8];
    corr_row->entry[ROOK][A1] = -321;
    EXPECT_EQ(h->cont_corr->row[QUEEN][D8].entry[ROOK][A1], -321);
    end_section();

    begin_section("a pawn bucket is the key under the mask, at every size");
    constexpr Key key = 0x9E3779B97F4A7C15ULL;
    for (int log2 = 11; log2 <= PawnHistory::MAX_BUCKETS_LOG2; ++log2) {
        const Key mask = (Key(1) << log2) - 1;
        PawnHistoryBucket& bucket = h->pawn->bucket[key & mask];
        bucket.entry[ROOK][E1] = static_cast<int16_t>(log2);
        EXPECT_EQ(h->pawn->bucket[key & mask].entry[ROOK][E1], log2);
        EXPECT((key & mask) < Key(PawnHistory::MAX_BUCKETS));
    }
    end_section();

    begin_section("footprint: the sizes the architecture review registered");
    EXPECT_EQ(sizeof(ContinuationHistory), size_t(1'605'632));   // 1,568 KiB
    EXPECT_EQ(sizeof(PawnHistory), size_t(7'340'032));           // 7,168 KiB at 8,192 buckets
    EXPECT_EQ(sizeof(MainHistory), size_t(16'384));
    EXPECT_EQ(sizeof(LowPlyHistory), size_t(40'960));
    EXPECT_EQ(sizeof(ContinuationCorrection), size_t(401'408));   // 392 KiB
    end_section();
}

int main() {
    std::printf("Core history tables\n");
    std::printf("===================\n");
    test_bounds();
    test_clear_and_halve();
    test_row_addressing();
    return harness_summary();
}
