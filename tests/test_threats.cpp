/// The threat producer against a brute-force attacker set: on random-walk
/// positions, every square's attackers of each class are found one square at
/// a time with Board::attackers_to and Board::is_square_attacked, and the
/// producer must agree bit for bit for both sides.

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "threats.h"
#include "zobrist.h"
#include "test_harness.h"

#include <cstdint>
#include <cstdio>
#include <random>

static const char* const SEED_FENS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1",
};

static Threats brute_force(const Board& b, Color us) {
    const Color them = ~us;
    const Bitboard occ = b.all_pieces();
    const Bitboard pawns = b.piece_bb(them, PAWN);
    const Bitboard minors = b.piece_bb(them, KNIGHT) | b.piece_bb(them, BISHOP);
    const Bitboard rooks = b.piece_bb(them, ROOK);
    Threats t;
    for (int s = 0; s < SQUARE_NB; ++s) {
        const Bitboard att = b.attackers_to(Square(s), occ, them);
        if (att & pawns)  t.by_pawn  |= sq_bb(s);
        if (att & minors) t.by_minor |= sq_bb(s);
        if (att & rooks)  t.by_rook  |= sq_bb(s);
        if (b.is_square_attacked(Square(s), them)) t.all |= sq_bb(s);
    }
    t.by_minor |= t.by_pawn;
    t.by_rook  |= t.by_minor;
    return t;
}

static bool same(const Threats& a, const Threats& b) {
    return a.by_pawn == b.by_pawn && a.by_minor == b.by_minor
        && a.by_rook == b.by_rook && a.all == b.all;
}

static void test_threats_match_brute_force() {
    begin_section("threats_against == brute-force attacker sets, both sides");
    std::mt19937_64 rng(0x7487EA75ULL);
    int positions = 0;
    int mismatches = 0;
    while (positions < 10000) {
        for (const char* fen : SEED_FENS) {
            Board b;
            b.set_fen(fen);
            const int plies = static_cast<int>(rng() % 24);
            for (int p = 0; p <= plies; ++p) {
                for (Color us : {WHITE, BLACK})
                    if (!same(threats_against(b, us), brute_force(b, us)))
                        ++mismatches;
                ++positions;
                MoveList legal;
                b.gen_legal(legal);
                if (legal.empty())
                    break;
                b.make_move(legal[static_cast<int>(rng() % static_cast<std::uint64_t>(legal.size()))]);
            }
        }
    }
    std::printf("%d positions ", positions);
    EXPECT_EQ(mismatches, 0);
    EXPECT(positions >= 10000);
    end_section();
}

static void test_threats_known_position() {
    begin_section("a lone black knight on e5 attacks exactly its eight squares");
    Board b;
    b.set_fen("4k3/8/8/4n3/8/8/8/4K3 w - - 0 1");
    const Threats t = threats_against(b, WHITE);
    EXPECT_EQ(t.by_pawn, Bitboard(0));
    EXPECT_EQ(t.by_minor, KnightAttacks[E5]);
    EXPECT_EQ(t.by_rook, KnightAttacks[E5]);
    EXPECT_EQ(t.all, KnightAttacks[E5] | KingAttacks[E8]);
    end_section();
}

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();

    std::printf("Threat producer\n");
    std::printf("===============\n");
    test_threats_known_position();
    test_threats_match_brute_force();
    return harness_summary();
}
