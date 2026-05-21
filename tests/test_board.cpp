/// Board correctness tests.
///
/// Covers: FEN round-trip, starting-position invariants, perft (5 standard
/// positions), make/unmake idempotency, Zobrist hash invariants, check
/// detection, castling-rights propagation, en passant (lazy-EP aware),
/// draw detection (50-move rule, twofold repetition, insufficient material),
/// null move, and SEE.
///
/// Build and run:
///   cmake --preset release && cmake --build --preset release --target test_board
///   ./build/release/test_board
///
/// Or via CTest:
///   ctest --test-dir build/release -R test_board --output-on-failure

#include "Board.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"
#include "test_harness.h"

#include <cstring>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Board helpers
// ---------------------------------------------------------------------------

static std::vector<Move> legal_moves(const Board& b) {
    std::vector<Move> pseudo;
    pseudo.reserve(64);
    b.gen_pseudo_legal(pseudo);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());
    for (Move m : pseudo)
        if (b.is_legal(m)) legal.push_back(m);
    return legal;
}

static uint64_t perft(Board& b, int depth) {
    if (depth == 0) return 1;

    std::vector<Move> pseudo;
    pseudo.reserve(64);
    b.gen_pseudo_legal(pseudo);

    if (depth == 1) {
        uint64_t nodes = 0;
        for (Move m : pseudo)
            if (b.is_legal(m)) ++nodes;
        return nodes;
    }

    uint64_t nodes = 0;
    for (Move m : pseudo) {
        if (!b.is_legal(m)) continue;
        b.make_move(m);
        nodes += perft(b, depth - 1);
        b.unmake_move(m);
    }
    return nodes;
}

static bool boards_equal(const Board& a, const Board& b) {
    for (int c = 0; c < NCOLORS; c++)
        for (int pt = 0; pt < PIECE_TYPE_NB; pt++)
            if (a.pieces[c][pt] != b.pieces[c][pt]) return false;
    for (int c = 0; c < NCOLORS; c++)
        if (a.occupancy[c] != b.occupancy[c]) return false;
    if (a.all_occ != b.all_occ) return false;
    for (int s = 0; s < SQUARE_NB; s++)
        if (a.board_sq[s] != b.board_sq[s]) return false;
    return a.side_to_move    == b.side_to_move
        && a.ep_sq           == b.ep_sq
        && a.castling_rights == b.castling_rights
        && a.halfmove_clock  == b.halfmove_clock
        && a.fullmove_number == b.fullmove_number
        && a.ply             == b.ply
        && a.hash            == b.hash
        && a.pawn_key        == b.pawn_key
        && a.king_sq[WHITE]  == b.king_sq[WHITE]
        && a.king_sq[BLACK]  == b.king_sq[BLACK];
}

// ---------------------------------------------------------------------------
// 1. FEN round-trip
// ---------------------------------------------------------------------------

static void test_fen_roundtrip() {
    struct TestFen { const char* label; const char* fen; };
    static const TestFen FENS[] = {
        { "startpos",
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" },
        { "kiwipete",
          "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" },
        { "with ep square",
          "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1" },
        { "no castling rights",
          "4k3/8/8/8/8/8/8/4K3 w - - 0 1" },
        { "high halfmove + fullmove",
          "4k3/8/8/8/8/8/8/4K3 w - - 100 50" },
        { "promotions position",
          "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 0" },
        { "endgame position",
          "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" },
    };
    for (const auto& tf : FENS) {
        begin_section(tf.label);
        Board b;
        b.set_fen(tf.fen);
        std::string got = b.get_fen();
        EXPECT(got == tf.fen);
        if (got != std::string(tf.fen))
            std::fprintf(stderr,
                "\n    expected: %s\n    got:      %s\n", tf.fen, got.c_str());
        end_section();
    }
}

// ---------------------------------------------------------------------------
// 2. Starting position invariants
// ---------------------------------------------------------------------------

static void test_starting_position() {
    Board b;

    begin_section("piece counts");
    EXPECT_EQ(popcount(b.pieces[WHITE][PAWN]),   8);
    EXPECT_EQ(popcount(b.pieces[WHITE][KNIGHT]), 2);
    EXPECT_EQ(popcount(b.pieces[WHITE][BISHOP]), 2);
    EXPECT_EQ(popcount(b.pieces[WHITE][ROOK]),   2);
    EXPECT_EQ(popcount(b.pieces[WHITE][QUEEN]),  1);
    EXPECT_EQ(popcount(b.pieces[WHITE][KING]),   1);
    EXPECT_EQ(popcount(b.pieces[BLACK][PAWN]),   8);
    EXPECT_EQ(popcount(b.pieces[BLACK][KNIGHT]), 2);
    EXPECT_EQ(popcount(b.pieces[BLACK][BISHOP]), 2);
    EXPECT_EQ(popcount(b.pieces[BLACK][ROOK]),   2);
    EXPECT_EQ(popcount(b.pieces[BLACK][QUEEN]),  1);
    EXPECT_EQ(popcount(b.pieces[BLACK][KING]),   1);
    end_section();

    begin_section("side, castling, ep, clocks");
    EXPECT_EQ(b.side_to_move,    WHITE);
    EXPECT_EQ(b.castling_rights, ALL_CASTLING);
    EXPECT_EQ(b.ep_sq,           SQ_NONE);
    EXPECT_EQ(b.halfmove_clock,  0);
    EXPECT_EQ(b.fullmove_number, 1);
    end_section();

    begin_section("king squares and mailbox");
    EXPECT_EQ(b.king_sq[WHITE], E1);
    EXPECT_EQ(b.king_sq[BLACK], E8);
    EXPECT_EQ(b.board_sq[E1],   W_KING);
    EXPECT_EQ(b.board_sq[E8],   B_KING);
    EXPECT_EQ(b.board_sq[A1],   W_ROOK);
    EXPECT_EQ(b.board_sq[H8],   B_ROOK);
    end_section();

    begin_section("not in check, 20 legal moves");
    EXPECT(!b.is_in_check());
    EXPECT_EQ(static_cast<int>(legal_moves(b).size()), 20);
    end_section();
}

// ---------------------------------------------------------------------------
// 3. Perft correctness
// ---------------------------------------------------------------------------

struct PerftCase {
    const char* label;
    const char* fen;
    uint64_t    expected[6]; // depth 1..5 (index 0 unused)
    int         max_depth;
};

static const PerftCase PERFT_CASES[] = {
    { "startpos",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      { 0, 20, 400, 8902, 197281, 4865609 }, 5 },
    { "kiwipete",
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      { 0, 48, 2039, 97862, 4085603, 0 }, 4 },
    { "endgame",
      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      { 0, 14, 191, 2812, 43238, 674624 }, 5 },
    { "promotions",
      "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 0",
      { 0, 44, 1486, 62379, 2103487, 0 }, 4 },
    { "complex midgame",
      "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
      { 0, 46, 2079, 89890, 3894594, 0 }, 4 },
};

static void test_perft() {
    for (const auto& tc : PERFT_CASES) {
        for (int d = 1; d <= tc.max_depth; ++d) {
            char label[64];
            std::snprintf(label, sizeof(label), "%s depth %d", tc.label, d);
            begin_section(label);
            Board b;
            b.set_fen(tc.fen);
            uint64_t got = perft(b, d);
            EXPECT_EQ(got, tc.expected[d]);
            end_section();
        }
    }
}

static void test_quiet_generation() {
    static const char* POSITIONS[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/P6k/8/8/8/8/6p1/K7 w - - 0 1",
    };

    for (const char* fen : POSITIONS) {
        begin_section(fen);
        Board b;
        b.set_fen(fen);

        MoveList legal, quiets;
        b.gen_legal(legal);
        b.gen_legal_quiets(quiets);

        int expected_quiets = 0;
        for (Move m : legal) {
            bool is_cap = (b.board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
            bool is_promo = move_type(m) == PROMOTION;
            if (!is_cap && !is_promo) expected_quiets++;
        }

        EXPECT_EQ(quiets.size(), expected_quiets);
        for (Move m : quiets) {
            bool is_cap = (b.board_sq[to_sq(m)] != NO_PIECE) || (move_type(m) == EN_PASSANT);
            bool is_promo = move_type(m) == PROMOTION;
            EXPECT(!is_cap && !is_promo);
        }
        end_section();
    }
}

// ---------------------------------------------------------------------------
// 4. Make/unmake idempotency
// ---------------------------------------------------------------------------

static void test_make_unmake() {
    static const char* POSITIONS[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 0",
    };
    for (const char* fen : POSITIONS) {
        char label[60];
        std::snprintf(label, sizeof(label), "%.52s", fen);
        begin_section(label);
        Board b;
        b.set_fen(fen);
        Board orig = b;

        std::vector<Move> pseudo;
        pseudo.reserve(64);
        b.gen_pseudo_legal(pseudo);

        bool all_ok = true;
        for (Move m : pseudo) {
            if (!b.is_legal(m)) continue;
            b.make_move(m);
            b.unmake_move(m);
            if (!boards_equal(b, orig)) {
                all_ok = false;
                break;
            }
        }
        EXPECT(all_ok);
        end_section();
    }
}

// ---------------------------------------------------------------------------
// 5. Zobrist hash invariants
// ---------------------------------------------------------------------------

static void test_zobrist() {
    begin_section("hash changes after move");
    Board b;
    Key h0 = b.hash;
    b.make_move(make_move(E2, E4));
    EXPECT(b.hash != h0);
    end_section();

    begin_section("hash restored after unmake");
    b.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    h0 = b.hash;
    Move m = make_move(G1, F3);
    b.make_move(m);
    b.unmake_move(m);
    EXPECT_EQ(b.hash, h0);
    end_section();

    begin_section("transposition: same position same hash");
    // Ng1-f3, Ng8-f6, Nf3-g1, Nf6-g8 → back to startpos
    Board a;
    a.make_move(make_move(G1, F3));
    a.make_move(make_move(G8, F6));
    a.make_move(make_move(F3, G1));
    a.make_move(make_move(F6, G8));
    Board bref;   // fresh default-constructed startpos
    EXPECT_EQ(a.hash, bref.hash);
    end_section();

    begin_section("distinct positions have distinct hashes");
    Board b1, b2;
    b1.make_move(make_move(E2, E4));
    b2.make_move(make_move(D2, D4));
    EXPECT(b1.hash != b2.hash);
    end_section();
}

static void test_pawn_key() {
    begin_section("pawn key preserves pawn color");
    Board a, b;
    a.set_fen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");
    b.set_fen("4k3/8/8/3P4/4p3/8/8/4K3 w - - 0 1");
    EXPECT(a.pawn_key != b.pawn_key);
    end_section();

    begin_section("pawn key restored after make/unmake");
    Board c;
    c.set_fen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");
    Key pk = c.pawn_key;
    Move m = make_move(E4, D5);
    c.make_move(m);
    c.unmake_move(m);
    EXPECT_EQ(c.pawn_key, pk);
    end_section();

    begin_section("pawn key restored after null move");
    Key pk2 = c.pawn_key;
    c.make_null_move();
    c.unmake_null_move();
    EXPECT_EQ(c.pawn_key, pk2);
    end_section();
}

// ---------------------------------------------------------------------------
// 6. Check detection
// ---------------------------------------------------------------------------

static void test_check_detection() {
    Board b;

    begin_section("startpos: not in check");
    EXPECT(!b.is_in_check());
    end_section();

    begin_section("rook gives check on open file");
    // Black rook e7, white king e1, no pieces on e2-e6
    b.set_fen("4k3/4r3/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT(b.is_in_check());
    end_section();

    begin_section("rook check blocked by pawn");
    b.set_fen("4k3/4r3/8/8/8/4P3/8/4K3 w - - 0 1");
    EXPECT(!b.is_in_check());
    end_section();

    begin_section("knight gives check");
    // Black knight f3 attacks e1
    b.set_fen("4k3/8/8/8/8/5n2/8/4K3 w - - 0 1");
    EXPECT(b.is_in_check());
    end_section();

    begin_section("bishop gives check");
    // Black bishop b4 attacks e1 diagonally (path c3-d2 clear)
    b.set_fen("4k3/8/8/8/1b6/8/8/4K3 w - - 0 1");
    EXPECT(b.is_in_check());
    end_section();

    begin_section("bishop check blocked by own pawn");
    b.set_fen("4k3/8/8/8/1b6/2P5/8/4K3 w - - 0 1");
    EXPECT(!b.is_in_check());
    end_section();

    begin_section("black king is in check");
    // White queen on e7 adjacent to black king on e8 (same file, adjacent rank)
    b.set_fen("4k3/4Q3/8/8/8/8/8/4K3 b - - 0 1");
    EXPECT(b.is_in_check());
    end_section();
}

// ---------------------------------------------------------------------------
// 7. Castling rights propagation
// ---------------------------------------------------------------------------

static void test_castling_rights() {
    // Clean position with all four rooks and both kings at castling squares
    const char* fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    Board b;

    begin_section("initial: ALL_CASTLING");
    b.set_fen(fen);
    EXPECT_EQ(b.castling_rights, ALL_CASTLING);
    end_section();

    begin_section("white king move: WK+WQ removed");
    b.set_fen(fen);
    b.make_move(make_move(E1, E2));
    EXPECT_EQ(b.castling_rights, BK_CASTLE | BQ_CASTLE);
    end_section();

    begin_section("Ra1 move: WQ removed only");
    b.set_fen(fen);
    b.make_move(make_move(A1, A2));
    EXPECT_EQ(b.castling_rights, WK_CASTLE | BK_CASTLE | BQ_CASTLE);
    end_section();

    begin_section("Rh1 move: WK removed only");
    b.set_fen(fen);
    b.make_move(make_move(H1, H2));
    EXPECT_EQ(b.castling_rights, WQ_CASTLE | BK_CASTLE | BQ_CASTLE);
    end_section();

    begin_section("Ra1xa8: WQ and BQ removed");
    b.set_fen(fen);
    b.make_move(make_move(A1, A8)); // white rook captures black rook on a8
    EXPECT_EQ(b.castling_rights, WK_CASTLE | BK_CASTLE);
    end_section();

    begin_section("rights restored after unmake");
    b.set_fen(fen);
    Board orig = b;
    Move km = make_move(E1, E2);
    b.make_move(km);
    b.unmake_move(km);
    EXPECT_EQ(b.castling_rights, orig.castling_rights);
    end_section();
}

// ---------------------------------------------------------------------------
// 8. En passant (lazy-EP aware)
// ---------------------------------------------------------------------------

static void test_en_passant() {
    Board b;

    begin_section("lazy EP: e2-e4 from startpos, no ep set");
    // No black pawn on d4 or f4, so ep_sq stays SQ_NONE
    b.make_move(make_move(E2, E4));
    EXPECT_EQ(b.ep_sq, SQ_NONE);
    end_section();

    begin_section("lazy EP: d7-d5, white pawn on e5 → ep=d6");
    // After d7-d5, the white e5-pawn attacks d6, so ep_sq IS set
    b.set_fen("4k3/3p4/8/4P3/8/8/8/4K3 b - - 0 1");
    b.make_move(make_move(D7, D5));
    EXPECT_EQ(b.ep_sq, D6);
    end_section();

    begin_section("EP capture is legal");
    b.set_fen("4k3/3p4/8/4P3/8/8/8/4K3 b - - 0 1");
    b.make_move(make_move(D7, D5));
    EXPECT(b.is_legal(make_ep(E5, D6)));
    end_section();

    begin_section("EP capture updates board correctly");
    b.set_fen("4k3/3p4/8/4P3/8/8/8/4K3 b - - 0 1");
    b.make_move(make_move(D7, D5));
    b.make_move(make_ep(E5, D6));
    EXPECT_EQ(b.board_sq[D6], W_PAWN);   // white pawn landed
    EXPECT_EQ(b.board_sq[E5], NO_PIECE); // origin empty
    EXPECT_EQ(b.board_sq[D5], NO_PIECE); // captured black pawn removed
    end_section();

    begin_section("EP capture unmake restores board");
    b.set_fen("4k3/3p4/8/4P3/8/8/8/4K3 b - - 0 1");
    b.make_move(make_move(D7, D5));
    Board before_ep = b;
    b.make_move(make_ep(E5, D6));
    b.unmake_move(make_ep(E5, D6));
    EXPECT(boards_equal(b, before_ep));
    end_section();

    begin_section("FEN with ep square parsed and output correctly");
    b.set_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(b.ep_sq, E3);
    end_section();
}

// ---------------------------------------------------------------------------
// 9. Draw detection
// ---------------------------------------------------------------------------

static void test_draw_detection() {
    Board b;

    begin_section("50-move rule: halfmove_clock = 100");
    b.set_fen("4k3/8/8/8/8/8/8/4K3 w - - 100 50");
    EXPECT(b.is_draw());
    end_section();

    begin_section("50-move rule: halfmove_clock = 99");
    b.set_fen("4k3/8/8/8/8/8/8/4K3 w - - 99 50");
    EXPECT(!b.is_draw());
    end_section();

    begin_section("twofold repetition detected");
    b.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // Ng1-f3, Ng8-f6, Nf3-g1, Nf6-g8 returns to the starting position
    b.make_move(make_move(G1, F3));
    b.make_move(make_move(G8, F6));
    b.make_move(make_move(F3, G1));
    EXPECT(!b.is_draw()); // only 3 moves; not yet repeated
    b.make_move(make_move(F6, G8));
    EXPECT(b.is_draw());  // twofold
    end_section();

    begin_section("insufficient material: K vs K");
    b.set_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT(b.is_insufficient_material());
    end_section();

    begin_section("insufficient material: K+N vs K");
    b.set_fen("4k3/8/8/8/8/8/8/3NK3 w - - 0 1");
    EXPECT(b.is_insufficient_material());
    end_section();

    begin_section("insufficient material: K+B vs K");
    b.set_fen("4k3/8/8/8/8/8/8/3BK3 w - - 0 1");
    EXPECT(b.is_insufficient_material());
    end_section();

    begin_section("sufficient material: K+P vs K");
    b.set_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
    EXPECT(!b.is_insufficient_material());
    end_section();

    begin_section("sufficient material: K+R vs K");
    b.set_fen("4k3/8/8/8/8/8/8/3RK3 w - - 0 1");
    EXPECT(!b.is_insufficient_material());
    end_section();
}

// ---------------------------------------------------------------------------
// 10. Null move
// ---------------------------------------------------------------------------

static void test_null_move() {
    Board b;

    begin_section("null move flips side to move");
    b.make_null_move();
    EXPECT_EQ(b.side_to_move, BLACK);
    b.unmake_null_move();
    EXPECT_EQ(b.side_to_move, WHITE);
    end_section();

    begin_section("null move clears ep square");
    // FEN with an explicit ep square
    b.set_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(b.ep_sq, E3);
    b.make_null_move();
    EXPECT_EQ(b.ep_sq, SQ_NONE);
    b.unmake_null_move();
    EXPECT_EQ(b.ep_sq, E3); // restored
    end_section();

    begin_section("null move changes hash; unmake restores it");
    b.set_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    Key h0 = b.hash;
    b.make_null_move();
    EXPECT(b.hash != h0);
    b.unmake_null_move();
    EXPECT_EQ(b.hash, h0);
    end_section();

    begin_section("null move preserves castling rights");
    b.set_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    int cr = b.castling_rights;
    b.make_null_move();
    EXPECT_EQ(b.castling_rights, cr);
    b.unmake_null_move();
    EXPECT_EQ(b.castling_rights, cr);
    end_section();
}

// ---------------------------------------------------------------------------
// 11. SEE (Static Exchange Evaluation)
// ---------------------------------------------------------------------------

static void test_see() {
    Board b;

    begin_section("pawn captures undefended pawn: +100");
    // White pawn d4 × black pawn e5, no defenders
    b.set_fen("4k3/8/8/4p3/3P4/8/8/4K3 w - - 0 1");
    EXPECT_EQ(b.see(make_move(D4, E5)), 100);
    end_section();

    begin_section("rook captures undefended pawn: +100");
    b.set_fen("4k3/8/8/4p3/4R3/8/8/4K3 w - - 0 1");
    EXPECT_EQ(b.see(make_move(E4, E5)), 100);
    end_section();

    begin_section("pawn captures undefended knight: +300");
    b.set_fen("4k3/8/8/3n4/4P3/8/8/4K3 w - - 0 1");
    EXPECT_EQ(b.see(make_move(E4, D5)), 300);
    end_section();

    begin_section("pawn captures undefended queen: +900");
    b.set_fen("7k/8/8/3q4/4P3/8/8/K7 w - - 0 1");
    EXPECT_EQ(b.see(make_move(E4, D5)), 900);
    end_section();

    begin_section("rook captures undefended rook: +500");
    // No king recapture: kings are on a1 and h8
    b.set_fen("7k/4r3/8/8/4R3/8/8/K7 w - - 0 1");
    EXPECT_EQ(b.see(make_move(E4, E7)), 500);
    end_section();

    begin_section("SEE is non-negative for all legal captures");
    // Verify the engine never assigns negative SEE to any legal capture
    // from the kiwipete position (rich in pieces and exchange opportunities)
    b.set_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    std::vector<Move> caps;
    caps.reserve(32);
    b.gen_pseudo_legal_captures(caps);
    bool all_nonneg = true;
    for (Move m : caps) {
        if (!b.is_legal(m)) continue;
        if (b.see(m) < 0) { all_nonneg = false; break; }
    }
    EXPECT(all_nonneg);
    end_section();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();

    std::printf("Board correctness tests\n");
    std::printf("%s\n", std::string(65, '=').c_str());

    std::printf("\nFEN round-trip\n");
    test_fen_roundtrip();

    std::printf("\nStarting position invariants\n");
    test_starting_position();

    std::printf("\nPerft (move-generation correctness)\n");
    test_perft();

    std::printf("\nQuiet move generation\n");
    test_quiet_generation();

    std::printf("\nMake/unmake idempotency\n");
    test_make_unmake();

    std::printf("\nZobrist hash\n");
    test_zobrist();

    std::printf("\nPawn key\n");
    test_pawn_key();

    std::printf("\nCheck detection\n");
    test_check_detection();

    std::printf("\nCastling rights\n");
    test_castling_rights();

    std::printf("\nEn passant\n");
    test_en_passant();

    std::printf("\nDraw detection\n");
    test_draw_detection();

    std::printf("\nNull move\n");
    test_null_move();

    std::printf("\nSEE (Static Exchange Evaluation)\n");
    test_see();

    return harness_summary();
}
