/// Perft through the search's own move path: the staged picker the main
/// search and check evasions use, make and unmake, at the standard positions
/// and depths, with the published leaf counts.
///
/// The board's perft walks the plain legal generator, so a picker that drops,
/// repeats or invents a move would pass it and show only as a loss in games.
/// Here, at every node:
///   - the keys, the occupancy, the piece table and the checkers are
///     recomputed from scratch (Board::assert_ok), and unmake restores the
///     position exactly;
///   - every move the picker delivers passes the test a table move passes,
///     and the moves delivered are the legal moves, each once;
///   - the parent's moves are put through that test again, and at the
///     interior nodes the grandparent's too (the same side to move, a key
///     collision's realistic move): it accepts exactly those legal here;
/// and at every interior node the other stage sets deliver exactly what their
/// rule names:
///   - quiescence: the captures and every promotion, each once in its pick
///     order; the quiet moves that give check, by playing them; in check, the
///     evasion picker cut after its first move keeps that move and the
///     evasions of the capture stage;
///   - ProbCut: the captures and queen promotions whose SEE clears the
///     threshold, never an underpromotion, each once.
/// The table move each node offers is chosen from its key: none, a legal
/// move, or a grandparent's move, so the picker's own validation is walked.

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
#include <string>

namespace {

constexpr const char* STARTPOS   = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr const char* KIWIPETE   = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
constexpr const char* POSITION_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr const char* POSITION_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
constexpr const char* POSITION_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr const char* POSITION_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

struct Case {
    const char* name;
    const char* fen;
    int depth;
    uint64_t leaves;
};

// The standard depths; a build with assertions (the sanitizer suite) walks
// shallower.
#ifdef NDEBUG
constexpr Case CASES[] = {
    {"startpos",   STARTPOS,   5,  4'865'609},
    {"kiwipete",   KIWIPETE,   4,  4'085'603},
    {"position 3", POSITION_3, 6, 11'030'083},
    {"position 4", POSITION_4, 5, 15'833'292},
    {"position 5", POSITION_5, 4,  2'103'487},
    {"position 6", POSITION_6, 4,  3'894'594},
};
#else
constexpr Case CASES[] = {
    {"startpos",   STARTPOS,   3,  8'902},
    {"kiwipete",   KIWIPETE,   2,  2'039},
    {"position 3", POSITION_3, 4, 43'238},
    {"position 4", POSITION_4, 3,  9'467},
    {"position 5", POSITION_5, 2,  1'486},
    {"position 6", POSITION_6, 2,  2'079},
};
#endif

constexpr int MAX_DEPTH = 8;

// The sources CoreMovePicker reports, by index.
constexpr const char* SOURCE_NAMES[] = {"none", "TT", "good tactical", "quiet", "bad tactical"};

struct Walk {
    std::unique_ptr<CoreHistoryTables> history = std::make_unique<CoreHistoryTables>();
    const PieceToHistory* cont[6] = {};
    ScoredMove buffers[MAX_DEPTH + 1][MoveList::CAPACITY];
    ScoredMove scratch[MoveList::CAPACITY];

    uint64_t from_source[5] = {};
    uint64_t tt_offered[3] = {};
    uint64_t evasion_nodes = 0;
    uint64_t rechecks = 0, rechecks_legal = 0;
    uint64_t qs_nodes = 0, qs_captures = 0, qs_checks = 0, qs_evasion_cuts = 0;
    uint64_t probcut_nodes = 0, probcut_moves = 0, probcut_refused = 0;
    int failures = 0;
};

void fail(Walk& w, const Board& b, const char* what, Move m = MOVE_NONE) {
    if (++w.failures <= 5)
        std::fprintf(stderr, "\n  %s%s%s at %s", what, m != MOVE_NONE ? ": " : "",
                     m != MOVE_NONE ? move_to_uci(m).c_str() : "", b.get_fen().c_str());
}

// The state unmake must put back beyond what assert_ok rebuilds.
struct Restorable {
    Key key, pawn, minor, nonpawn_w, nonpawn_b;
    Bitboard occupied, checkers;
    int castling, rule50, fullmove;
    Square ep;
    size_t history;
    bool operator==(const Restorable&) const = default;
};

Restorable restorable(const Board& b) {
    return {b.position_key(), b.pawn_key_value(), b.minor_key_value(),
            b.nonpawn_key_value(WHITE), b.nonpawn_key_value(BLACK), b.all_pieces(),
            b.checking_pieces(), b.castling(), b.rule50_count(), b.fullmove(),
            b.ep_square(), b.history_size()};
}

struct Sorted {
    Move moves[MoveList::CAPACITY];
    int count = 0;
    bool contains(Move m) const { return std::binary_search(moves, moves + count, m); }
};

Sorted sorted(const Move* begin, const Move* end) {
    Sorted s;
    s.count = int(end - begin);
    std::copy(begin, end, s.moves);
    std::sort(s.moves, s.moves + s.count);
    return s;
}

bool same_set(Sorted a, const Sorted& want) {
    std::sort(a.moves, a.moves + a.count);
    return a.count == want.count && std::equal(a.moves, a.moves + a.count, want.moves);
}

uint64_t mix(Key key) { return (key * 0x9E3779B97F4A7C15ULL) >> 32; }

// The rules the quiescence and ProbCut sets are held to, stated here from the
// move's fields rather than read from the engine.
bool takes_or_promotes(const Board& b, Move m) {
    return b.piece_on(to_sq(m)) != NO_PIECE || move_type(m) == EN_PASSANT
        || move_type(m) == PROMOTION;
}

bool probcut_rule(const Board& b, Move m) {
    if (move_type(m) == PROMOTION)
        return promo_type(m) == QUEEN;
    return b.piece_on(to_sq(m)) != NO_PIECE || move_type(m) == EN_PASSANT;
}

// Quiescence's sets at a node not in check, and its evasion cut in check.
void check_quiescence(Walk& w, Board& b, const Sorted& legal, Move tt, int ply) {
    ++w.qs_nodes;
    if (b.is_in_check()) {
        // Quiescence cuts the quiets once an evasion holds: the first
        // move stays, and every evasion of the capture stage is delivered.
        CoreMovePicker picker(b, *w.history, w.history->pawn->bucket[b.pawn_key_value() & 63],
                              w.cont, tt, MOVE_NONE, 0, ply, nullptr, w.scratch);
        Sorted got;
        const Move first = picker.next();
        if (first != MOVE_NONE) {
            got.moves[got.count++] = first;
            picker.skip_quiets();
            for (Move m = picker.next(); m != MOVE_NONE; m = picker.next())
                got.moves[got.count++] = m;
        }
        Sorted want;
        for (int i = 0; i < legal.count; ++i)
            if (legal.moves[i] == first || takes_or_promotes(b, legal.moves[i]))
                want.moves[want.count++] = legal.moves[i];
        if (!same_set(got, want))
            fail(w, b, "quiescence's evasion cut delivered another set", first);
        ++w.qs_evasion_cuts;
        return;
    }

    // The captures and every promotion, in the capture stage's order with the
    // table move first, each once.
    MoveList captures;
    b.gen_legal_captures(captures);
    int n = 0;
    for (Move m : captures)
        w.scratch[n++] = {m, m == tt ? 10'000'000 : capture_order_score(b, *w.history, m)};
    Sorted got;
    for (int i = 0; i < n; ++i)
        got.moves[got.count++] = pick_next(w.scratch, i, n);
    Sorted want;
    for (int i = 0; i < legal.count; ++i)
        if (takes_or_promotes(b, legal.moves[i]))
            want.moves[want.count++] = legal.moves[i];
    if (!same_set(got, want))
        fail(w, b, "quiescence's captures are not the captures and promotions");
    w.qs_captures += uint64_t(got.count);

    // The quiet moves that give check, by playing each.
    MoveList checks;
    b.gen_quiet_checks(checks);
    Sorted want_checks;
    for (int i = 0; i < legal.count; ++i) {
        const Move m = legal.moves[i];
        if (takes_or_promotes(b, m))
            continue;
        b.make_move(m);
        const bool check = b.is_in_check();
        b.unmake_move(m);
        if (check)
            want_checks.moves[want_checks.count++] = m;
    }
    if (!same_set(sorted(checks.begin(), checks.end()), want_checks))
        fail(w, b, "quiescence's quiet checks are not the quiet moves that check");
    w.qs_checks += uint64_t(want_checks.count);
}

// ProbCut's set at a node not in check (ProbCut never runs in check).
void check_probcut(Walk& w, const Board& b, const Sorted& legal, Move tt) {
    if (b.is_in_check())
        return;
    ++w.probcut_nodes;
    const int threshold = int(mix(b.position_key()) % 801) - 400;
    ProbCutPicker picker(b, *w.history, tt, MOVE_NONE, threshold, w.scratch);
    Sorted got;
    for (Move m = picker.next(); m != MOVE_NONE; m = picker.next()) {
        if (got.count == MoveList::CAPACITY) {
            fail(w, b, "ProbCut delivered more moves than there are", m);
            return;
        }
        got.moves[got.count++] = m;
    }
    Sorted want;
    for (int i = 0; i < legal.count; ++i) {
        const Move m = legal.moves[i];
        if (!probcut_rule(b, m))
            continue;
        if (b.see_ge(m, threshold))
            want.moves[want.count++] = m;
        else
            ++w.probcut_refused;
    }
    if (!same_set(got, want))
        fail(w, b, "ProbCut did not deliver exactly its candidates clearing SEE");
    w.probcut_moves += uint64_t(got.count);
}

// The table's test on moves of other positions: it accepts exactly the legal.
void recheck(Walk& w, const Board& b, const Sorted& legal, const Sorted* earlier) {
    if (!earlier)
        return;
    for (int i = 0; i < earlier->count; ++i) {
        const Move m = earlier->moves[i];
        const bool member = legal.contains(m);
        ++w.rechecks;
        w.rechecks_legal += member;
        if (table_move_usable(b, m) != member)
            fail(w, b, member ? "the table's test refuses a legal move"
                              : "the table's test accepts an illegal move", m);
    }
}

uint64_t node(Walk& w, Board& b, int depth, int ply, const Sorted* parent,
              const Sorted* grandparent) {
    if (w.failures)
        return 0;
    if (!b.assert_ok()) {
        fail(w, b, "the position is inconsistent");
        return 0;
    }
    MoveList generated;
    b.gen_legal(generated);
    const Sorted legal = sorted(generated.begin(), generated.end());
    recheck(w, b, legal, parent);
    if (depth == 0)
        return 1;
    // The grandparent's moves, at the interior nodes: at the leaves they
    // would cost more than the rest of the walk.
    recheck(w, b, legal, grandparent);

    // The table move this node offers.
    const uint64_t r = mix(b.position_key());
    Move tt = MOVE_NONE;
    const int kind = int(r % 3);
    if (kind == 1 && legal.count)
        tt = legal.moves[(r >> 2) % uint64_t(legal.count)];
    else if (kind == 2 && grandparent && grandparent->count)
        tt = grandparent->moves[(r >> 2) % uint64_t(grandparent->count)];
    ++w.tt_offered[tt == MOVE_NONE ? 0 : kind];

    check_quiescence(w, b, legal, tt, ply);
    check_probcut(w, b, legal, tt);
    w.evasion_nodes += b.is_in_check();

    CoreMovePicker picker(b, *w.history, w.history->pawn->bucket[b.pawn_key_value() & 63],
                          w.cont, tt, MOVE_NONE, depth, ply, nullptr, w.buffers[ply]);
    Sorted delivered;
    uint64_t leaves = 0;
    for (Move m = picker.next(); m != MOVE_NONE; m = picker.next()) {
        ++w.from_source[int(picker.last_source())];
        if (!table_move_usable(b, m) || !legal.contains(m)) {
            fail(w, b, "the picker delivered a move that is not legal", m);
            return 0;
        }
        if (delivered.count == MoveList::CAPACITY) {
            fail(w, b, "the picker delivered more moves than there are", m);
            return 0;
        }
        delivered.moves[delivered.count++] = m;
        const Restorable before = restorable(b);
        b.make_move(m);
        leaves += node(w, b, depth - 1, ply + 1, &legal, parent);
        b.unmake_move(m);
        if (w.failures)
            return 0;
        if (!b.assert_ok() || !(restorable(b) == before)) {
            fail(w, b, "unmake did not restore the position", m);
            return 0;
        }
    }
    if (!same_set(delivered, legal))
        fail(w, b, "the picker's moves are not the legal moves, each once");
    return leaves;
}

void randomise(CoreHistoryTables& h, Walk& w) {
    std::mt19937_64 rng(0x5EA2C4BA7FULL);
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
    for (int k = 0; k < 6; ++k)
        w.cont[k] = &h.continuation->row[k & 1][(k >> 1) & 1][KNIGHT + k % 4][(k * 13) % 64];
}

void test_search_path_perft() {
    auto walk = std::make_unique<Walk>();
    Walk& w = *walk;
    randomise(*w.history, w);
    for (const Case& c : CASES) {
        const std::string title = std::string("search-path perft(") + std::to_string(c.depth)
                                + ") " + c.name;
        begin_section(title.c_str());
        Board b;
        b.set_fen(c.fen);
        const uint64_t leaves = node(w, b, c.depth, 0, nullptr, nullptr);
        EXPECT_EQ(w.failures, 0);
        EXPECT_EQ(leaves, c.leaves);
        end_section();
    }

    begin_section("every stage and every check reached");
    for (int s = 1; s < 5; ++s) {
        std::printf("\n    %-14s %llu", SOURCE_NAMES[s],
                    static_cast<unsigned long long>(w.from_source[s]));
        EXPECT(w.from_source[s] > 0);
    }
    std::printf("\n    table moves offered: none %llu, legal %llu, grandparent's %llu",
                static_cast<unsigned long long>(w.tt_offered[0]),
                static_cast<unsigned long long>(w.tt_offered[1]),
                static_cast<unsigned long long>(w.tt_offered[2]));
    std::printf("\n    evasion nodes %llu; table test rechecks %llu, %llu of them legal",
                static_cast<unsigned long long>(w.evasion_nodes),
                static_cast<unsigned long long>(w.rechecks),
                static_cast<unsigned long long>(w.rechecks_legal));
    std::printf("\n    quiescence nodes %llu: captures %llu, quiet checks %llu, evasion cuts %llu",
                static_cast<unsigned long long>(w.qs_nodes),
                static_cast<unsigned long long>(w.qs_captures),
                static_cast<unsigned long long>(w.qs_checks),
                static_cast<unsigned long long>(w.qs_evasion_cuts));
    std::printf("\n    ProbCut nodes %llu: delivered %llu, refused by SEE %llu\n  ",
                static_cast<unsigned long long>(w.probcut_nodes),
                static_cast<unsigned long long>(w.probcut_moves),
                static_cast<unsigned long long>(w.probcut_refused));
    for (uint64_t n : w.tt_offered)
        EXPECT(n > 0);
    EXPECT(w.evasion_nodes > 0);
    EXPECT(w.rechecks_legal > 0 && w.rechecks_legal < w.rechecks);
    EXPECT(w.qs_captures > 0);
    EXPECT(w.qs_checks > 0);
    EXPECT(w.qs_evasion_cuts > 0);
    EXPECT(w.probcut_moves > 0);
    EXPECT(w.probcut_refused > 0);
    end_section();
}

}  // namespace

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    std::printf("Search-path perft\n");
    std::printf("=================\n");
    test_search_path_perft();
    return harness_summary();
}
