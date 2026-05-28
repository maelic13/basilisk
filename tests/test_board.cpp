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
        && a.plies_from_null == b.plies_from_null
        && a.fullmove_number == b.fullmove_number
        && a.ply             == b.ply
        && a.hash            == b.hash
        && a.pawn_key        == b.pawn_key
        && a.minor_key       == b.minor_key
        && a.nonpawn_key[WHITE] == b.nonpawn_key[WHITE]
        && a.nonpawn_key[BLACK] == b.nonpawn_key[BLACK]
        && a.king_sq[WHITE]  == b.king_sq[WHITE]
        && a.king_sq[BLACK]  == b.king_sq[BLACK];
}

static Key recompute_minor_key(const Board& b) {
    Key key = 0;
    for (Color c : {WHITE, BLACK}) {
        for (PieceType pt : {KNIGHT, BISHOP}) {
            Bitboard bb = b.pieces[c][pt];
            while (bb) {
                Square sq = Square(pop_lsb(bb));
                key ^= Zobrist::PieceKeys[c][pt][sq];
            }
        }
    }
    return key;
}

static Key recompute_nonpawn_key(const Board& b, Color c) {
    Key key = 0;
    for (PieceType pt : {KNIGHT, BISHOP, ROOK, QUEEN}) {
        Bitboard bb = b.pieces[c][pt];
        while (bb) {
            Square sq = Square(pop_lsb(bb));
            key ^= Zobrist::PieceKeys[c][pt][sq];
        }
    }
    return key;
}

static bool piece_keys_match_mailbox(const Board& b) {
    return b.minor_key == recompute_minor_key(b)
        && b.nonpawn_key[WHITE] == recompute_nonpawn_key(b, WHITE)
        && b.nonpawn_key[BLACK] == recompute_nonpawn_key(b, BLACK);
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

static void test_fen_validation() {
    Board b;
    const std::string original = b.get_fen();
    std::string error;

    auto expect_invalid_preserves = [&](const char* label, const char* fen,
                                        bool strict = false) {
        begin_section(label);
        error.clear();
        EXPECT(!b.try_set_fen(fen, &error, strict));
        EXPECT(!error.empty());
        EXPECT_STR(b.get_fen(), original);
        end_section();
    };

    begin_section("invalid FEN rejected without changing board");
    EXPECT(!b.try_set_fen("8/8/8/8/8/8/8/8 w - - 0 1", &error));
    EXPECT(!error.empty());
    EXPECT_STR(b.get_fen(), original);
    end_section();

    expect_invalid_preserves("malformed board rejected",
                             "9/8/8/8/8/8/8/4K2k w - - 0 1");

    expect_invalid_preserves("missing FEN fields rejected",
                             "4k3/8/8/8/8/8/8/4K3 w -");

    expect_invalid_preserves("extra FEN fields rejected",
                             "4k3/8/8/8/8/8/8/4K3 w - - 0 1 trailing");

    expect_invalid_preserves("invalid side rejected",
                             "4k3/8/8/8/8/8/8/4K3 x - - 0 1");

    expect_invalid_preserves("invalid castling rights rejected",
                             "4k3/8/8/8/8/8/8/4K3 w A - 0 1");

    expect_invalid_preserves("invalid en-passant square rejected",
                             "4k3/8/8/8/8/8/8/4K3 w - e4 0 1");

    expect_invalid_preserves("negative halfmove rejected",
                             "4k3/8/8/8/8/8/8/4K3 w - - -1 1");

    expect_invalid_preserves("too many pawns rejected",
                             "4k3/PPPPPPPP/P7/8/8/8/8/4K3 w - - 0 1");

    expect_invalid_preserves("pawns on back rank rejected",
                             "4k3/8/8/8/8/8/8/P3K3 w - - 0 1");

    expect_invalid_preserves("king can be captured rejected",
                             "4k3/8/8/8/8/8/4R3/4K3 w - - 0 1",
                             true);

    begin_section("fullmove zero tolerated");
    const char* fen =
        "r1bqkb1r/pppn1ppp/3p1n2/4p1B1/3PP3/2N5/PPP2PPP/R2QKBNR w KQkq e6 0 0";
    EXPECT(b.try_set_fen(fen, &error));
    EXPECT_STR(b.get_fen(), fen);
    end_section();

    begin_section("castling rights sanitized for missing rooks");
    EXPECT(b.try_set_fen("4k3/8/8/8/8/8/8/R3K3 w KQkq - 0 1", &error));
    EXPECT_EQ(b.castling_rights, WQ_CASTLE);
    EXPECT_STR(b.get_fen(), "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1");
    end_section();
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

static void test_incremental_piece_keys() {
    Board b;

    begin_section("piece keys match FEN placement");
    b.set_fen("4k3/8/3b4/8/2N1R3/8/8/4K3 w - - 0 1");
    EXPECT(piece_keys_match_mailbox(b));
    end_section();

    begin_section("piece keys update after minor move");
    b.make_move(make_move(C4, B6));
    EXPECT(piece_keys_match_mailbox(b));
    b.unmake_move(make_move(C4, B6));
    EXPECT(piece_keys_match_mailbox(b));
    end_section();

    begin_section("piece keys update after capture");
    b.make_move(make_move(C4, D6));
    EXPECT(piece_keys_match_mailbox(b));
    b.unmake_move(make_move(C4, D6));
    EXPECT(piece_keys_match_mailbox(b));
    end_section();

    begin_section("piece keys update after promotion and unmake");
    b.set_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    Key minor_before = b.minor_key;
    Key white_nonpawn_before = b.nonpawn_key[WHITE];
    Move promo = make_promotion(A7, A8, QUEEN);
    b.make_move(promo);
    EXPECT(piece_keys_match_mailbox(b));
    EXPECT(b.nonpawn_key[WHITE] != white_nonpawn_before);
    b.unmake_move(promo);
    EXPECT_EQ(b.minor_key, minor_before);
    EXPECT_EQ(b.nonpawn_key[WHITE], white_nonpawn_before);
    EXPECT(piece_keys_match_mailbox(b));
    end_section();

    begin_section("piece keys survive null move");
    b.set_fen("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    Key minor = b.minor_key;
    Key white_nonpawn = b.nonpawn_key[WHITE];
    Key black_nonpawn = b.nonpawn_key[BLACK];
    b.make_null_move();
    EXPECT_EQ(b.minor_key, minor);
    EXPECT_EQ(b.nonpawn_key[WHITE], white_nonpawn);
    EXPECT_EQ(b.nonpawn_key[BLACK], black_nonpawn);
    b.unmake_null_move();
    EXPECT(piece_keys_match_mailbox(b));
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

    begin_section("castling rook move gives check");
    b.set_fen("5k2/8/8/8/8/8/8/4K2R w K - 0 1");
    EXPECT(b.gives_check(make_castling(E1, G1)));
    end_section();
}

// ---------------------------------------------------------------------------
// 7. Strict move legality
// ---------------------------------------------------------------------------

static bool generated_legal_contains(const Board& b, Move move) {
    MoveList legal;
    b.gen_legal(legal);
    for (Move candidate : legal)
        if (candidate == move)
            return true;
    return false;
}

static void expect_illegal_move(const char* fen, Move move) {
    Board b;
    b.set_fen(fen);
    EXPECT(!b.is_legal(move));
    EXPECT(!generated_legal_contains(b, move));
}

static void test_strict_move_legality() {
    begin_section("illegal king move onto own pawn is rejected");
    expect_illegal_move("8/8/8/8/5P2/4K3/8/7k w - - 0 1",
                        make_move(E3, F4));
    end_section();

    begin_section("impossible king move is rejected");
    expect_illegal_move("8/8/8/8/5P2/4K3/8/7k w - - 0 1",
                        make_move(E3, E8));
    end_section();

    begin_section("tournament PV king-own-pawn move is rejected");
    expect_illegal_move("2k5/pp3pp1/5n2/2P5/bPP2P2/P3K3/6Pp/3Q1B1R w - - 0 23",
                        make_move(E3, F4));
    end_section();

    begin_section("tournament PV repeated king-own-pawn move is rejected");
    expect_illegal_move("2k5/pp3pp1/5n2/2P5/1PP2P2/P2BK1p1/2b3PP/7R w - - 2 24",
                        make_move(E3, F4));
    end_section();

    begin_section("malformed castling target is rejected");
    expect_illegal_move("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                        make_castling(E1, H1));
    end_section();
}

// ---------------------------------------------------------------------------
// 8. Castling rights propagation
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

    begin_section("rights without rooks do not allow castling");
    b.set_fen("4k3/8/8/8/8/8/8/4K3 w KQ - 0 1");
    MoveList legal;
    b.gen_legal(legal);
    bool saw_castle = false;
    for (Move m : legal)
        saw_castle = saw_castle || move_type(m) == CASTLING;
    EXPECT(!saw_castle);
    EXPECT(!b.is_legal(make_castling(E1, G1)));
    EXPECT(!b.is_legal(make_castling(E1, C1)));
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

    begin_section("EP capture can evade pawn check");
    b.set_fen("2r5/P3R3/3Q4/6pk/6Pp/5P1P/8/7K b - g3 0 1");
    EXPECT(b.is_in_check());
    Move ep = make_ep(H4, G3);
    EXPECT(b.is_legal(ep));
    MoveList legal;
    b.gen_legal(legal);
    bool saw_ep = false;
    for (Move move : legal)
        saw_ep = saw_ep || move == ep;
    EXPECT(saw_ep);
    b.make_move(ep);
    EXPECT(!b.is_square_attacked(b.king_sq[BLACK], WHITE));
    end_section();
}

// ---------------------------------------------------------------------------
// 9. Draw detection
// ---------------------------------------------------------------------------

static void test_draw_detection() {
    Board b;

    begin_section("50-move rule: halfmove_clock = 100");
    b.set_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 100 50");
    EXPECT(b.is_draw());
    end_section();

    begin_section("50-move rule: halfmove_clock = 99");
    b.set_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 99 50");
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

    begin_section("search repetition distinguishes root and in-tree repeat");
    b.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    b.make_move(make_move(G1, F3));
    b.make_move(make_move(G8, F6));
    b.make_move(make_move(F3, G1));
    b.make_move(make_move(F6, G8));
    EXPECT(!b.is_repetition(0));
    EXPECT(b.is_repetition(4));
    EXPECT(b.is_draw(4));
    end_section();

    begin_section("null move limits repetition scan");
    b.make_null_move();
    b.make_move(make_move(G8, F6));
    b.make_move(make_move(G1, F3));
    EXPECT(!b.is_repetition(10));
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
    int pfn0 = b.plies_from_null;
    b.make_null_move();
    EXPECT(b.hash != h0);
    EXPECT_EQ(b.plies_from_null, 0);
    b.unmake_null_move();
    EXPECT_EQ(b.hash, h0);
    EXPECT_EQ(b.plies_from_null, pfn0);
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

    begin_section("SEE detects defended losing capture");
    b.set_fen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(b.see(make_move(E5, G6)), -200);
    end_section();

    begin_section("threshold SEE matches full SEE");
    const char* fens[] = {
        "4k3/8/8/3n4/4P3/8/8/4K3 w - - 0 1",
        "4k2r/6P1/8/8/8/8/8/4K3 w - - 0 1",
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    };
    const int thresholds[] = {-1200, -600, -100, -50, 0, 50, 100, 300, 600, 1200};
    bool matches = true;
    for (const char* fen : fens) {
        b.set_fen(fen);
        MoveList legal;
        b.gen_legal(legal);
        for (Move m : legal) {
            const bool tactical = move_type(m) == PROMOTION
                               || move_type(m) == EN_PASSANT
                               || b.board_sq[to_sq(m)] != NO_PIECE;
            if (!tactical)
                continue;
            const int see = b.see(m);
            for (int threshold : thresholds) {
                if (b.see_ge(m, threshold) != (see >= threshold)) {
                    matches = false;
                    break;
                }
            }
        }
    }
    EXPECT(matches);
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

    std::printf("\nFEN validation\n");
    test_fen_validation();

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

    std::printf("\nIncremental piece keys\n");
    test_incremental_piece_keys();

    std::printf("\nCheck detection\n");
    test_check_detection();

    std::printf("\nStrict move legality\n");
    test_strict_move_legality();

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
