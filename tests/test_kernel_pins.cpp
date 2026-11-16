/// Pins of what the node kernel does today, so that a later change to any of
/// them is a decision and not an accident:
///   - the node type each recursive call passes (PV or not, cut or not), read
///     from whole searches through the call-site log this test's engine
///     library is built with (src/search_child_log.h);
///   - the two meanings of "capture", one named predicate each, and their
///     agreement over the legal moves of a corpus, castling quiet in both;
///   - IIR's condition;
///   - the main search's refusal of a table cutoff from a rule-50 count of 96.
/// The table keeping its move when a store passes none is pinned in test_tt.

#include "attacks.h"
#include "bench.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "move_picker_core.h"
#include "search_b3.h"
#include "search_child_log.h"
#include "search_worker.h"
#include "test_harness.h"
#include "tt.h"
#include "zobrist.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <string>

#ifndef BASILISK_CHILD_LOG
#error "test_kernel_pins reads the call-site log: link basilisk_engine_child_log"
#endif

namespace {

// ---- The node type each recursive call passes ------------------------------

struct Type {
    bool pv;
    bool cut;
};

// What each site passes, from the caller's own type. Quiescence has no cut
// flag; its site reads `cut` false.
struct Rule {
    ChildSite site;
    const char* name;
    Type (*child)(bool parent_pv, bool parent_cut);
    bool reads_parent_cut;  // both parent types must then be seen
};

const Rule RULES[] = {
    {ChildSite::Root, "root", [](bool, bool) { return Type{true, false}; }, false},
    {ChildSite::NullMove, "null move", [](bool, bool) { return Type{false, false}; }, false},
    {ChildSite::NullVerify, "null verification", [](bool, bool) { return Type{false, false}; }, false},
    {ChildSite::ProbCutQs, "ProbCut quiescence", [](bool, bool) { return Type{false, false}; }, false},
    {ChildSite::ProbCutSearch, "ProbCut search", [](bool, bool cut) { return Type{false, !cut}; }, true},
    {ChildSite::Exclusion, "exclusion search", [](bool, bool cut) { return Type{false, cut}; }, true},
    {ChildSite::FirstMove, "first move", [](bool pv, bool cut) { return Type{pv, !pv && !cut}; }, true},
    {ChildSite::Reduced, "reduced search", [](bool, bool) { return Type{false, true}; }, false},
    {ChildSite::ReducedResearch, "reduced re-search", [](bool, bool cut) { return Type{false, !cut}; }, true},
    {ChildSite::ZeroWindow, "null-window search", [](bool, bool cut) { return Type{false, !cut}; }, true},
    {ChildSite::PvResearch, "PV re-search", [](bool, bool) { return Type{true, false}; }, false},
};
static_assert(std::size(RULES) == size_t(ChildSite::Count), "a site without a rule");

void search(const std::string& fen, int depth, void (*set)(SearchParams&) = nullptr) {
    TranspositionTable tt(16);
    std::atomic_bool stop{false};
    auto searcher = std::make_unique<Searcher>(tt, stop);
    Board b;
    b.set_fen(fen);
    SearchLimits lim;
    lim.depth = depth;
    if (set)
        set(lim.params);
    searcher->search(b, lim);
}

void test_child_types() {
    ChildLog& log = child_log();
    log = ChildLog{};
    for (int i = 0; i < 12; ++i)
        search(std::string(bench_fens()[size_t(i)]), 12);
    // Verification at the depths these searches reach.
    for (int i = 0; i < 6; ++i)
        search(std::string(bench_fens()[size_t(i)]), 12,
               [](SearchParams& p) { p.b3_nmp_verify_depth = 8; });

    begin_section("child types: every negamax call names its site");
    EXPECT_EQ(log.unnamed, uint64_t(0));
    end_section();

    for (const Rule& rule : RULES) {
        const std::string title = std::string("child types: ") + rule.name;
        begin_section(title.c_str());
        uint64_t total = 0, wrong = 0, by_parent_cut[2] = {};
        for (int ppv = 0; ppv < 2; ++ppv)
            for (int pcut = 0; pcut < 2; ++pcut)
                for (int cpv = 0; cpv < 2; ++cpv)
                    for (int ccut = 0; ccut < 2; ++ccut) {
                        const uint64_t n = log.calls[int(rule.site)][ppv][pcut][cpv][ccut];
                        if (!n)
                            continue;
                        total += n;
                        by_parent_cut[pcut] += n;
                        const Type want = rule.child(ppv, pcut);
                        if (want.pv != bool(cpv) || want.cut != bool(ccut)) {
                            wrong += n;
                            std::fprintf(stderr,
                                         "\n  parent pv %d cut %d passed pv %d cut %d, %llu times",
                                         ppv, pcut, cpv, ccut, static_cast<unsigned long long>(n));
                        }
                    }
        std::printf("%llu calls ", static_cast<unsigned long long>(total));
        EXPECT(total > 0);
        EXPECT_EQ(wrong, uint64_t(0));
        if (rule.reads_parent_cut) {
            EXPECT(by_parent_cut[0] > 0);
            EXPECT(by_parent_cut[1] > 0);
        }
        end_section();
    }
}

// ---- The two meanings of "capture" ----------------------------------------

void test_capture_predicates() {
    begin_section("captures: the move loop's and correction training's predicates");
    const char* const seeds[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/8/8/2pP4/8/8/1p4P1/R3K2R w KQkq c6 0 1",
    };
    std::mt19937_64 rng(0xCA97E5ULL);
    uint64_t moves = 0, castles = 0, en_passant = 0, quiet_promotions = 0,
             capture_promotions = 0, captures = 0, quiets = 0, wrong = 0;
    for (int walk = 0; walk < 3000; ++walk) {
        Board b;
        b.set_fen(seeds[size_t(walk) % std::size(seeds)]);
        const int plies = int(rng() % 24);
        for (int p = 0; p <= plies; ++p) {
            MoveList legal;
            b.gen_legal(legal);
            if (legal.empty())
                break;
            for (Move m : legal) {
                ++moves;
                const bool stage = is_capture_stage(b, m);
                const bool takes = captures_piece(b, m);
                const bool target = b.piece_on(to_sq(m)) != NO_PIECE;
                const MoveType mt = move_type(m);
                // Each predicate states its own meaning, and they differ
                // exactly on a promotion that takes nothing.
                bool ok = takes == (target || mt == EN_PASSANT)
                       && stage == (takes || mt == PROMOTION);
                if (mt == CASTLING) {
                    ++castles;
                    ok = ok && !stage && !takes;
                } else if (mt == EN_PASSANT) {
                    ++en_passant;
                    ok = ok && stage && takes;
                } else if (mt == PROMOTION) {
                    ++(target ? capture_promotions : quiet_promotions);
                    ok = ok && stage && takes == target;
                } else {
                    ++(target ? captures : quiets);
                    ok = ok && stage == target && takes == target;
                }
                wrong += !ok;
            }
            b.make_move(legal[int(rng() % uint64_t(legal.size()))]);
        }
    }
    std::printf("%llu moves: %llu castles, %llu en passant, %llu quiet and %llu capturing "
                "promotions ",
                static_cast<unsigned long long>(moves), static_cast<unsigned long long>(castles),
                static_cast<unsigned long long>(en_passant),
                static_cast<unsigned long long>(quiet_promotions),
                static_cast<unsigned long long>(capture_promotions));
    EXPECT_EQ(wrong, uint64_t(0));
    EXPECT(castles > 0);
    EXPECT(en_passant > 0);
    EXPECT(quiet_promotions > 0);
    EXPECT(capture_promotions > 0);
    EXPECT(captures > 0);
    EXPECT(quiets > 0);
    end_section();
}

// ---- The child's key before the make ----------------------------------------

// Every legal move of `b`, made: the predicted key is the made key outside
// the four kinds predicted_child_key names, and only those kinds may miss.
void check_predicted_keys(Board& b, uint64_t& moves, uint64_t& missed, uint64_t& wrong,
                          uint64_t (&missed_by_kind)[4]) {
    MoveList legal;
    b.gen_legal(legal);
    for (Move m : legal) {
        const Key predicted = predicted_child_key(b, m);
        const bool may_miss = predicted_key_may_miss(b, m);
        b.make_move(m);
        const bool exact = b.position_key() == predicted;
        b.unmake_move(m);
        ++moves;
        if (!exact) {
            ++missed;
            ++missed_by_kind[move_type(m) == NORMAL ? 3 : int(move_type(m)) - 1];
        }
        wrong += !exact && !may_miss;
        // Outside the four kinds a miss is the error the test exists for;
        // a castling or an en passant capture always misses.
        wrong += (move_type(m) == CASTLING || move_type(m) == EN_PASSANT) && exact;
    }
}

void test_predicted_key() {
    begin_section("A move's child key before the make: exact outside four kinds");
    uint64_t moves = 0, missed = 0, wrong = 0, missed_by_kind[4] = {};
    int positions = 0;
    std::ifstream cohort(BASILISK_TEST_GAME_COHORT);
    EXPECT(bool(cohort));
    for (std::string line; std::getline(cohort, line);) {
        std::istringstream fields(line.substr(0, line.find(';')));
        std::string board, side, castling, ep;
        if (!(fields >> board >> side >> castling >> ep))
            continue;
        Board b;
        b.set_fen(board + " " + side + " " + castling + " " + ep + " 0 1");
        check_predicted_keys(b, moves, missed, wrong, missed_by_kind);
        ++positions;
    }
    const uint64_t cohort_moves = moves, cohort_missed = missed;
    // Random walks from positions rich in the four kinds.
    const char* const seeds[] = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/8/8/2pP4/8/8/1p4P1/R3K2R w KQkq c6 0 1",
    };
    std::mt19937_64 rng(0x9E7C0DEULL);
    for (int walk = 0; walk < 2000; ++walk) {
        Board b;
        b.set_fen(seeds[size_t(walk) % std::size(seeds)]);
        for (int p = int(rng() % 24); p >= 0; --p) {
            check_predicted_keys(b, moves, missed, wrong, missed_by_kind);
            MoveList legal;
            b.gen_legal(legal);
            if (legal.empty())
                break;
            b.make_move(legal[int(rng() % uint64_t(legal.size()))]);
        }
    }
    std::printf("%d cohort positions, %llu moves, %llu missed; with the walks %llu moves, missed: "
                "%llu promotions, %llu en passant, %llu castles, %llu double pushes ",
                positions, static_cast<unsigned long long>(cohort_moves),
                static_cast<unsigned long long>(cohort_missed), static_cast<unsigned long long>(moves),
                static_cast<unsigned long long>(missed_by_kind[0]),
                static_cast<unsigned long long>(missed_by_kind[1]),
                static_cast<unsigned long long>(missed_by_kind[2]),
                static_cast<unsigned long long>(missed_by_kind[3]));
    EXPECT_EQ(positions, 1000);
    EXPECT_EQ(wrong, uint64_t(0));
    for (uint64_t kind : missed_by_kind)
        EXPECT(kind > 0);
    end_section();
}

// ---- IIR's condition --------------------------------------------------------

void test_iir_condition() {
    begin_section("IIR: a PV or cut node off the PV, not in check, without a table move");
    struct Node {
        bool pv, cut, in_check, follow_pv;
        int depth;
        bool has_tt_move;
        bool reduced;
    };
    // The minimum depth is 5 throughout.
    const Node cases[] = {
        // pv     cut    check  follow depth  tt     reduced
        {true,  false, false, false,  5, false, true},
        {false, true,  false, false,  5, false, true},
        {false, false, false, false,  5, false, false},  // an all node
        {true,  false, false, false,  4, false, false},  // too shallow
        {true,  false, true,  false,  9, false, false},  // in check
        {false, true,  false, true,   9, false, false},  // on the previous PV
        {true,  false, false, false,  9, true,  false},  // a table move
    };
    for (const Node& n : cases)
        EXPECT_EQ(b3::iir_applies(n.pv, n.cut, n.in_check, n.follow_pv, n.depth, 5, n.has_tt_move),
                  n.reduced);
    end_section();
}

// ---- The rule-50 guard on the main search's table cutoff --------------------

void test_rule50_cutoff_guard() {
    begin_section("table cutoff: taken below a rule-50 count of 96, refused from it");
    for (int count : {0, 1, 50, 94, 95})
        EXPECT(b3::tt_cutoff_rule50_allows(count));
    for (int count : {96, 97, 98, 99, 100, 150})
        EXPECT(!b3::tt_cutoff_rule50_allows(count));
    end_section();
}

}  // namespace

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();
    std::printf("Kernel pins\n");
    std::printf("===========\n");
    test_capture_predicates();
    test_predicted_key();
    test_iir_condition();
    test_rule50_cutoff_guard();
    test_child_types();
    return harness_summary();
}
