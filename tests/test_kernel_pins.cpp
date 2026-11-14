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
#include <memory>
#include <random>
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
    // Verification at the depths these searches reach, in both its forms.
    for (int i = 0; i < 6; ++i) {
        const std::string fen(bench_fens()[size_t(i)]);
        search(fen, 12, [](SearchParams& p) { p.b3_nmp_verify_depth = 8; });
        search(fen, 12, [](SearchParams& p) { p.b3_nmp_verify = 2; });
    }

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

// ---- IIR's condition --------------------------------------------------------

void test_iir_condition() {
    begin_section("IIR: a PV or cut node off the PV, not in check, without a table move");
    struct Node {
        bool pv, cut, in_check, follow_pv;
        int depth;
        bool has_tt_move, stale, tt_found;
        int tt_depth;
        bool reduced;
    };
    // The minimum depth is 5 throughout.
    const Node cases[] = {
        // pv     cut    check  follow depth  tt     stale  found  ttd  reduced
        {true,  false, false, false,  5, false, false, false,  0, true},
        {false, true,  false, false,  5, false, false, false,  0, true},
        {false, false, false, false,  5, false, false, false,  0, false},  // an all node
        {true,  false, false, false,  4, false, false, false,  0, false},  // too shallow
        {true,  false, true,  false,  9, false, false, false,  0, false},  // in check
        {false, true,  false, true,   9, false, false, false,  0, false},  // on the previous PV
        {true,  false, false, false,  9, true,  false, true,   1, false},  // a table move
        {true,  false, false, false,  9, true,  true,  true,   5, true},   // stale: 4 plies shallower
        {true,  false, false, false,  9, true,  true,  true,   6, false},  // stale: 3 plies
        {false, true,  false, false,  9, true,  true,  false,  0, false},  // stale, no entry
    };
    for (const Node& n : cases)
        EXPECT_EQ(b3::iir_applies(n.pv, n.cut, n.in_check, n.follow_pv, n.depth, 5,
                                  n.has_tt_move, n.stale, n.tt_found, n.tt_depth),
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
    test_iir_condition();
    test_rule50_cutoff_guard();
    test_child_types();
    return harness_summary();
}
