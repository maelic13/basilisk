/// Move encoding / decoding and UCI string tests.
///
/// Covers: MOVE_NONE / MOVE_NULL sentinels, normal move, castling, en passant,
/// promotions (all four types), move_to_uci, TT round-trip.
///
/// Build:
///   cmake --build --preset release --target test_move
///   ./build/release/test_move

#include "move.h"
#include "types.h"
#include "test_harness.h"

#include <string>

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_sentinels() {
    begin_section("MOVE_NONE == 0");
    EXPECT_EQ(MOVE_NONE, 0);
    end_section();

    begin_section("MOVE_NULL != MOVE_NONE");
    EXPECT(MOVE_NULL != MOVE_NONE);
    end_section();

    begin_section("move_to_uci(MOVE_NONE) == \"0000\"");
    EXPECT_STR(move_to_uci(MOVE_NONE), "0000");
    end_section();

    begin_section("move_type(MOVE_NONE) == NORMAL");
    EXPECT_EQ(move_type(MOVE_NONE), NORMAL);
    end_section();
}

static void test_normal_moves() {
    Move m = make_move(E2, E4);

    begin_section("normal: from_sq == E2");
    EXPECT_EQ(from_sq(m), E2);
    end_section();

    begin_section("normal: to_sq == E4");
    EXPECT_EQ(to_sq(m), E4);
    end_section();

    begin_section("normal: move_type == NORMAL");
    EXPECT_EQ(move_type(m), NORMAL);
    end_section();

    begin_section("normal: move_to_uci == \"e2e4\"");
    EXPECT_STR(move_to_uci(m), "e2e4");
    end_section();

    // Corner-to-corner move
    Move m2 = make_move(A1, H8);
    begin_section("normal: a1h8 from_sq / to_sq");
    EXPECT_EQ(from_sq(m2), A1);
    EXPECT_EQ(to_sq(m2), H8);
    end_section();

    begin_section("normal: a1h8 UCI == \"a1h8\"");
    EXPECT_STR(move_to_uci(m2), "a1h8");
    end_section();

    // All four file/rank extremes
    Move m3 = make_move(H1, A8);
    begin_section("normal: h1a8 round-trip");
    EXPECT_EQ(from_sq(m3), H1);
    EXPECT_EQ(to_sq(m3), A8);
    EXPECT_STR(move_to_uci(m3), "h1a8");
    end_section();
}

static void test_castling() {
    struct C { Square from; Square to; const char* uci; };
    static const C CASES[] = {
        { E1, G1, "e1g1" },  // white kingside
        { E1, C1, "e1c1" },  // white queenside
        { E8, G8, "e8g8" },  // black kingside
        { E8, C8, "e8c8" },  // black queenside
    };
    for (const auto& c : CASES) {
        Move m = make_castling(c.from, c.to);

        char label[64];
        std::snprintf(label, sizeof(label), "castling %s: move_type", c.uci);
        begin_section(label);
        EXPECT_EQ(move_type(m), CASTLING);
        end_section();

        std::snprintf(label, sizeof(label), "castling %s: from_sq", c.uci);
        begin_section(label);
        EXPECT_EQ(from_sq(m), c.from);
        end_section();

        std::snprintf(label, sizeof(label), "castling %s: to_sq", c.uci);
        begin_section(label);
        EXPECT_EQ(to_sq(m), c.to);
        end_section();

        std::snprintf(label, sizeof(label), "castling %s: UCI string", c.uci);
        begin_section(label);
        EXPECT_STR(move_to_uci(m), c.uci);
        end_section();
    }
}

static void test_en_passant() {
    // White pawn on d5 captures en passant on e6
    Move m = make_ep(D5, E6);

    begin_section("ep: move_type == EN_PASSANT");
    EXPECT_EQ(move_type(m), EN_PASSANT);
    end_section();

    begin_section("ep: from_sq == D5");
    EXPECT_EQ(from_sq(m), D5);
    end_section();

    begin_section("ep: to_sq == E6");
    EXPECT_EQ(to_sq(m), E6);
    end_section();

    begin_section("ep: UCI == \"d5e6\"");
    EXPECT_STR(move_to_uci(m), "d5e6");
    end_section();

    // Black pawn on e4 captures en passant on d3
    Move m2 = make_ep(E4, D3);
    begin_section("ep: black pawn e4d3 UCI == \"e4d3\"");
    EXPECT_EQ(move_type(m2), EN_PASSANT);
    EXPECT_STR(move_to_uci(m2), "e4d3");
    end_section();
}

static void test_promotions() {
    struct P { PieceType pt; char suffix; };
    static const P PROMOS[] = {
        { KNIGHT, 'n' },
        { BISHOP, 'b' },
        { ROOK,   'r' },
        { QUEEN,  'q' },
    };
    for (const auto& p : PROMOS) {
        Move m = make_promotion(A7, A8, p.pt);

        char label[80];
        std::snprintf(label, sizeof(label),
            "promo %c: move_type == PROMOTION", p.suffix);
        begin_section(label);
        EXPECT_EQ(move_type(m), PROMOTION);
        end_section();

        std::snprintf(label, sizeof(label),
            "promo %c: promo_type correct", p.suffix);
        begin_section(label);
        EXPECT_EQ(promo_type(m), p.pt);
        end_section();

        std::snprintf(label, sizeof(label),
            "promo %c: from_sq == A7", p.suffix);
        begin_section(label);
        EXPECT_EQ(from_sq(m), A7);
        end_section();

        std::snprintf(label, sizeof(label),
            "promo %c: to_sq == A8", p.suffix);
        begin_section(label);
        EXPECT_EQ(to_sq(m), A8);
        end_section();

        std::snprintf(label, sizeof(label),
            "promo %c: UCI suffix correct", p.suffix);
        begin_section(label);
        std::string expected = std::string("a7a8") + p.suffix;
        EXPECT_STR(move_to_uci(m), expected);
        end_section();
    }
    // Promotion from different file/color
    Move mq = make_promotion(H2, H1, QUEEN);
    begin_section("promo queen: h2h1q UCI");
    EXPECT_EQ(move_type(mq), PROMOTION);
    EXPECT_EQ(promo_type(mq), QUEEN);
    EXPECT_STR(move_to_uci(mq), "h2h1q");
    end_section();
}

static void test_tt_roundtrip() {
    struct Case { Move m; const char* label; };
    static const Case CASES[] = {
        { make_move(E2, E4),             "normal"    },
        { make_move(A1, H8),             "normal a1h8" },
        { make_castling(E1, G1),         "castling wk" },
        { make_castling(E8, C8),         "castling bq" },
        { make_ep(D5, E6),               "ep"        },
        { make_promotion(B7, B8, QUEEN), "promo-q"   },
        { make_promotion(G7, G8, KNIGHT),"promo-n"   },
        { make_promotion(C2, C1, ROOK),  "promo-r"   },
        { make_promotion(F2, F1, BISHOP),"promo-b"   },
    };
    for (const auto& c : CASES) {
        char label[80];
        std::snprintf(label, sizeof(label), "TT round-trip: %s", c.label);
        begin_section(label);
        Move rt = move_from_tt(move_to_tt(c.m));
        EXPECT_EQ(rt, c.m);
        end_section();
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::printf("Move encoding tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nSentinels\n");
    test_sentinels();

    std::printf("\nNormal moves\n");
    test_normal_moves();

    std::printf("\nCastling\n");
    test_castling();

    std::printf("\nEn passant\n");
    test_en_passant();

    std::printf("\nPromotions\n");
    test_promotions();

    std::printf("\nTT round-trip\n");
    test_tt_roundtrip();

    return harness_summary();
}
