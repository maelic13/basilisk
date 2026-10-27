/// The selectivity core's move picker, on a random-walk corpus
/// with randomised histories so every stage and the partial sort are
/// exercised:
///   - every legal move is delivered exactly once;
///   - a legal TT move comes first and never again;
///   - an illegal or foreign TT move is never delivered;
///   - skip_quiets() at the start leaves exactly the capture stage (captures
///     and promotions); after the first quiet, that quiet and the capture
///     stage, and no other quiet; with direct checks kept, the capture stage
///     and exactly the quiets whose moved piece checks from its destination;
///   - the excluded move is never delivered.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "history_core.h"
#include "move_picker_core.h"
#include "zobrist.h"
#include "test_harness.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>

static const char* const SEED_FENS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1",
};

static bool capture_stage(const Board& b, Move m) {
    return b.piece_on(to_sq(m)) != NO_PIECE || move_type(m) == EN_PASSANT
        || move_type(m) == PROMOTION;
}

static void randomise(CoreHistoryTables& h, std::mt19937_64& rng) {
    auto r = [&](int limit) {
        return static_cast<int16_t>(int(rng() % uint64_t(2 * limit + 1)) - limit);
    };
    for (auto& a : h.main.entry) for (auto& b : a) for (auto& e : b) e = r(MainHistory::LIMIT);
    for (auto& a : h.capture.entry) for (auto& b : a) for (auto& e : b) e = r(CaptureHistory::LIMIT);
    for (auto& a : h.low_ply.entry) for (auto& b : a) for (auto& e : b) e = r(LowPlyHistory::LIMIT);
    for (auto& a : h.continuation->row)
        for (auto& b : a) for (auto& c : b) for (auto& row : c)
            for (auto& x : row.entry) for (auto& e : x) e = r(PieceToHistory::LIMIT);
    for (int i = 0; i < 64; ++i)
        for (auto& x : h.pawn->bucket[i].entry) for (auto& e : x) e = r(PawnHistoryBucket::LIMIT);
}

struct Pick {
    std::vector<Move> moves;
};

// A quiet whose moved piece gives check from its destination.
static bool direct_quiet_check(const Board& b, Move m) {
    const PieceType pt = type_of(b.piece_on(from_sq(m)));
    return pt != KING && (b.check_squares(pt, b.turn()) & sq_bb(to_sq(m))) != 0;
}

// Runs the picker to exhaustion. `skip_after_quiets` < 0 never skips; 0 skips
// before the first call; n > 0 skips once n quiets were delivered.
static Pick run(const Board& b, const CoreHistoryTables& h, Move tt, Move excluded,
                int depth, int ply, int skip_after_quiets, bool keep_direct_checks = false) {
    static ScoredMove buffer[MoveList::CAPACITY];
    const PieceToHistory* cont[6];
    for (int k = 0; k < 6; ++k)
        cont[k] = &h.continuation->row[k & 1][(k >> 1) & 1][KNIGHT + k % 4][(k * 13) % 64];
    CoreMovePicker picker(b, h, h.pawn->bucket[b.pawn_key_value() & 63], cont, tt, excluded,
                          depth, ply, nullptr, buffer);
    Pick p;
    int quiets = 0;
    if (skip_after_quiets == 0)
        picker.skip_quiets(keep_direct_checks);
    for (Move m; (m = picker.next()) != MOVE_NONE; ) {
        p.moves.push_back(m);
        if (!capture_stage(b, m) && ++quiets == skip_after_quiets)
            picker.skip_quiets(keep_direct_checks);
    }
    return p;
}

static std::vector<Move> sorted(std::vector<Move> v) {
    std::sort(v.begin(), v.end());
    return v;
}

static void test_picker_contract() {
    begin_section("picker: exhaustive, TT first, skip-quiets, foreign TT and excluded moves");
    std::mt19937_64 rng(0xC0DEB2C3ULL);
    auto h = std::make_unique<CoreHistoryTables>();
    randomise(*h, rng);

    int positions = 0, failures = 0, skipped_runs = 0, tt_runs = 0, foreign_runs = 0;
    int check_runs = 0;
    Move foreign = make_move(A1, A1);
    while (positions < 3000 && failures < 10) {
        Board b;
        b.set_fen(SEED_FENS[positions % 6]);
        const int plies = int(rng() % 30);
        for (int p = 0; p < plies; ++p) {
            MoveList l;
            b.gen_legal(l);
            if (l.empty()) break;
            b.make_move(l[int(rng() % uint64_t(l.size()))]);
        }
        MoveList legal_list;
        b.gen_legal(legal_list);
        if (legal_list.empty())
            continue;
        ++positions;
        const std::vector<Move> legal = sorted({legal_list.begin(), legal_list.end()});
        std::vector<Move> stage;
        for (Move m : legal) if (capture_stage(b, m)) stage.push_back(m);
        const int depth = 1 + int(rng() % 12);
        const int ply = int(rng() % 8);
        auto fail = [&](const char* what) {
            if (++failures <= 5)
                std::fprintf(stderr, "  %s: %s\n", what, b.get_fen().c_str());
        };

        // Exhaustive, no duplicates.
        const Pick all = run(b, *h, MOVE_NONE, MOVE_NONE, depth, ply, -1);
        if (sorted(all.moves) != legal) fail("not every legal move exactly once");

        // A legal TT move first, never twice.
        const Move tt = legal_list[int(rng() % uint64_t(legal_list.size()))];
        const Pick with_tt = run(b, *h, tt, MOVE_NONE, depth, ply, -1);
        ++tt_runs;
        if (with_tt.moves.empty() || with_tt.moves.front() != tt) fail("TT move not first");
        if (sorted(with_tt.moves) != legal) fail("TT move run not exhaustive");

        // A TT move from another position, illegal here, is skipped.
        if (std::find(legal.begin(), legal.end(), foreign) == legal.end()) {
            ++foreign_runs;
            const Pick f = run(b, *h, foreign, MOVE_NONE, depth, ply, -1);
            if (sorted(f.moves) != legal) fail("foreign TT move changed the delivered set");
        }
        foreign = tt;

        // The excluded move is never delivered.
        const Pick ex = run(b, *h, MOVE_NONE, tt, depth, ply, -1);
        std::vector<Move> without = legal;
        without.erase(std::find(without.begin(), without.end(), tt));
        if (sorted(ex.moves) != without) fail("excluded move delivered");

        // skip_quiets() at the start: exactly the capture stage.
        const Pick none = run(b, *h, MOVE_NONE, MOVE_NONE, depth, ply, 0);
        if (sorted(none.moves) != stage) fail("skip at start did not leave the capture stage");

        // skip_quiets() at the start, direct checks kept: the capture stage
        // and exactly the direct quiet checks.
        std::vector<Move> stage_and_checks = stage;
        for (Move m : legal)
            if (!capture_stage(b, m) && direct_quiet_check(b, m)) stage_and_checks.push_back(m);
        if (stage_and_checks.size() > stage.size()) ++check_runs;
        const Pick checks = run(b, *h, MOVE_NONE, MOVE_NONE, depth, ply, 0, true);
        if (sorted(checks.moves) != sorted(stage_and_checks))
            fail("skip keeping direct checks did not leave the capture stage and the checks");

        // skip_quiets() after the first quiet: that quiet and the capture stage.
        const Pick one = run(b, *h, MOVE_NONE, MOVE_NONE, depth, ply, 1);
        int quiets = 0;
        std::vector<Move> one_stage;
        for (Move m : one.moves) {
            if (capture_stage(b, m)) one_stage.push_back(m);
            else ++quiets;
        }
        if (sorted(one_stage) != stage) fail("skip after a quiet lost a capture-stage move");
        if (quiets != (stage.size() == legal.size() ? 0 : 1)) fail("skip after a quiet delivered another quiet");
        if (quiets == 1) ++skipped_runs;
    }
    std::printf("%d positions, %d TT runs, %d foreign-TT runs, %d skip-after-quiet runs, "
                "%d with direct quiet checks ",
                positions, tt_runs, foreign_runs, skipped_runs, check_runs);
    EXPECT_EQ(failures, 0);
    EXPECT(positions == 3000);
    EXPECT(skipped_runs > 1000);
    EXPECT(foreign_runs > 1000);
    EXPECT(check_runs > 300);
    end_section();
}

static void test_partial_sort() {
    begin_section("partial insertion sort: descending above the limit, a permutation");
    std::mt19937_64 rng(42);
    bool ok = true;
    for (int trial = 0; trial < 2000 && ok; ++trial) {
        ScoredMove v[64];
        const int n = int(rng() % 64);
        for (int i = 0; i < n; ++i)
            v[i] = {Move(i + 1), int(rng() % 20001) - 10000};
        std::vector<int> before;
        for (int i = 0; i < n; ++i) before.push_back(v[i].score);
        const int limit = int(rng() % 20001) - 10000;
        partial_insertion_sort(v, v + n, limit);
        std::vector<int> after;
        for (int i = 0; i < n; ++i) after.push_back(v[i].score);
        std::sort(before.begin(), before.end());
        std::vector<int> sorted_after = after;
        std::sort(sorted_after.begin(), sorted_after.end());
        ok = ok && before == sorted_after;
        // The prefix holds every score >= limit, in descending order.
        int k = 0;
        while (k < n && (k == 0 || v[k].score >= limit) && (k == 0 || v[k].score <= v[k - 1].score)) ++k;
        for (int i = k; i < n; ++i)
            ok = ok && v[i].score < limit;
    }
    EXPECT(ok);
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    std::printf("Core move picker\n");
    std::printf("================\n");
    test_partial_sort();
    test_picker_contract();
    return harness_summary();
}
