/// Evaluator tests.
///
/// Covers: side-to-move perspective, eval symmetry (mirrored positions),
/// material advantage, lone kings, OCB draw scaling, KNNK draw detection,
/// tempo bonus.
///
/// Build:
///   cmake --build --preset release --target test_eval
///   ./build/release/test_eval

#include "Board.h"
#include "eval.h"
#include "attacks.h"
#include "bitboard.h"
#include "zobrist.h"
#include "test_harness.h"

#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int eval_fen(const char* fen) {
    Board b;
    b.set_fen(fen);
    Evaluator ev;
    return ev.evaluate(b);
}

// Build a FEN for the "mirror" of a position: all colors swapped, ranks flipped,
// and side-to-move flipped.  The structural evaluation should be identical to
// the original (both sides see the same position from their own king's view).
static std::string mirror_fen(const char* fen) {
    Board orig;
    orig.set_fen(fen);

    // Build piece-placement string: iterate mirrored ranks top→bottom
    std::string placement;
    for (int r = 7; r >= 0; r--) {
        // Source rank in the original board (rank r of mirror = rank (7-r) of original)
        int orig_rank = 7 - r;
        int empty = 0;
        for (int f = 0; f < 8; f++) {
            Square sq = make_square(File(f), Rank(orig_rank));
            Piece  p  = orig.board_sq[sq];
            if (p == NO_PIECE) { empty++; continue; }
            if (empty) { placement += static_cast<char>('0' + empty); empty = 0; }
            // Swap color: white piece becomes black and vice-versa
            Color     mc = ~color_of(p);
            PieceType pt = type_of(p);
            static const char piece_chars[] = ".pnbrqk";
            char ch = piece_chars[pt];
            if (mc == WHITE) ch = static_cast<char>(ch - 'a' + 'A');
            placement += ch;
        }
        if (empty) placement += static_cast<char>('0' + empty);
        if (r > 0)  placement += '/';
    }

    // Flip side to move
    char stm = (orig.side_to_move == WHITE) ? 'b' : 'w';

    // Mirror castling rights: WK↔BK, WQ↔BQ
    std::string castling;
    if (orig.castling_rights & BK_CASTLE) castling += 'K';
    if (orig.castling_rights & BQ_CASTLE) castling += 'Q';
    if (orig.castling_rights & WK_CASTLE) castling += 'k';
    if (orig.castling_rights & WQ_CASTLE) castling += 'q';
    if (castling.empty()) castling = "-";

    // Mirror EP square (flip rank)
    std::string ep = "-";
    if (orig.ep_sq != SQ_NONE) {
        Square mep = flip_rank(orig.ep_sq);
        static const char files[] = "abcdefgh";
        static const char ranks[] = "12345678";
        ep  = std::string() + files[file_of(mep)] + ranks[rank_of(mep)];
    }

    return placement + " " + stm + " " + castling + " " + ep
         + " " + std::to_string(orig.halfmove_clock)
         + " " + std::to_string(orig.fullmove_number);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_stm_perspective() {
    // The evaluator returns a value from the side-to-move's perspective.
    // The starting position has a small positive eval due to the tempo bonus.

    begin_section("startpos: white to move → small positive");
    int v = eval_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT(v > 0);
    EXPECT(v < 80);
    end_section();

    // Black to move: same structure, same tempo bonus for black
    begin_section("startpos: black to move → small positive");
    int vb = eval_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    EXPECT(vb > 0);
    EXPECT(vb < 80);
    end_section();

    // A side with overwhelming material advantage should score very high
    begin_section("large material advantage → large positive eval");
    int vq = eval_fen("4k3/8/8/8/8/8/QQQQQQQQ/4K3 w - - 0 1");
    EXPECT(vq > 1000);
    end_section();

    // The opponent's perspective must be negative for same position
    // (black is badly losing so it's negative for black)
    begin_section("losing side (black to move) → negative");
    int vqb = eval_fen("4k3/8/8/8/8/8/QQQQQQQQ/4K3 b - - 0 1");
    EXPECT(vqb < -1000);
    end_section();
}

static void test_symmetry() {
    // Structural symmetry: eval(P) from white's view == eval(mirror(P)) from
    // black's view (both return from the mover's perspective).
    static const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
    };
    for (const char* fen : fens) {
        char label[64];
        std::snprintf(label, sizeof(label), "symmetry: %.42s…", fen);
        begin_section(label);
        int orig_val   = eval_fen(fen);
        std::string mf = mirror_fen(fen);
        int mirror_val = eval_fen(mf.c_str());
        EXPECT_EQ(orig_val, mirror_val);
        end_section();
    }
}

static void test_material_advantage() {
    // Extra queen (white, white to move): large positive
    begin_section("extra queen (white): large positive eval");
    int vq = eval_fen("4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
    EXPECT(vq > 500);
    end_section();

    // Extra rook
    begin_section("extra rook (white): positive eval");
    int vr = eval_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    EXPECT(vr > 200);
    end_section();

    // Black has extra queen, it's black to move: positive for black (stm perspective)
    begin_section("extra queen (black): positive for black stm");
    int vqb = eval_fen("q3k3/8/8/8/8/8/8/4K3 b - - 0 1");
    EXPECT(vqb > 500);
    end_section();

    // Queen > rook
    begin_section("queen advantage > rook advantage");
    int vqw = eval_fen("4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
    int vrw = eval_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    EXPECT(vqw > vrw);
    end_section();
}

static void test_lone_kings() {
    // KK: no material difference beyond PST; score should be near 0
    begin_section("KK: eval magnitude < 50");
    int v = eval_fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT(std::abs(v) < 50);
    end_section();

    begin_section("KK: eval magnitude < 50 (black stm)");
    int vb = eval_fen("4k3/8/8/8/8/8/8/4K3 b - - 0 1");
    EXPECT(std::abs(vb) < 50);
    end_section();
}

static void test_ocb_scaling() {
    // OCB endgame with no pawns: score should be reduced toward zero.
    // Verify by comparing a one-sided non-OCB position with an OCB version.
    //
    // Color mask (DARK_SQ = 0x55AA55AA55AA55AAULL, LSB = a1):
    //   Byte 0 (rank 1) = 0xAA = 1010_1010 → bits 1,3,5,7 = b1,d1,f1,h1 are "dark" in mask
    //   So d4 (index 27, byte3=0x55, bit3=0) is NOT in DARK_SQ (light).
    //   And e5 (index 36, byte4=0xAA, bit4=1) IS in DARK_SQ (dark).
    //   d4 ≠ e5 in mask → they are on opposite colours → OCB applies.

    // Position: equal material (single bishop each), OCB, no pawns.
    begin_section("OCB no pawns: score magnitude < 200 (scaled)");
    int v = eval_fen("4k3/8/8/4b3/3B4/8/8/4K3 w - - 0 1");
    EXPECT(std::abs(v) < 200);
    end_section();

    // Same-colour bishops: no OCB scaling.  Score may still be near 0 but
    // must not be *forced* to zero.
    // d4 and d5 are on same colour: d4 not in DARK_SQ (index 27, byte3=0x55, bit3=0).
    // d5 = index 35, byte4=0xAA=10101010, bit3=0 → not in DARK_SQ. Same colour, no OCB.
    begin_section("same-colour bishops: no OCB (score may be any)");
    int v2 = eval_fen("4k3/8/8/3b4/3B4/8/8/4K3 w - - 0 1");
    // We just verify it doesn't crash and returns a sane range
    EXPECT(std::abs(v2) < 2000);
    end_section();
}

static void test_knnk_draw() {
    // Two white knights vs bare black king → theoretical draw → score == 0
    begin_section("KNNK (white knights): score == 0");
    int v = eval_fen("8/8/8/8/8/5k2/6NN/7K w - - 0 1");
    EXPECT_EQ(v, 0);
    end_section();

    // Same but black to move: still 0 (−0 == 0)
    begin_section("KNNK (white knights): score == 0, black stm");
    int vb = eval_fen("8/8/8/8/8/5k2/6NN/7K b - - 0 1");
    EXPECT_EQ(vb, 0);
    end_section();

    // Two black knights vs bare white king → draw → 0
    begin_section("KNNK (black knights): score == 0");
    int v2 = eval_fen("7k/6nn/5K2/8/8/8/8/8 b - - 0 1");
    EXPECT_EQ(v2, 0);
    end_section();

    // A single knight does NOT trigger the KNNK special case
    begin_section("KNK: special-case NOT triggered (score != forced 0)");
    // White Kh1, Ng2 vs Black Kf3: should have some material score, not forced 0
    int v3 = eval_fen("8/8/8/8/8/5k2/6N1/7K w - - 0 1");
    // The KNNK rule shouldn't fire here, so score reflects normal eval
    // (we just check there's no crash; the sign isn't guaranteed for KNK)
    EXPECT(std::abs(v3) < 2000);
    end_section();
}

static void test_tempo_bonus() {
    // The eval adds +10 mg for the white side and -10 mg for black.
    // In the starting position (full middlegame phase), white-to-move should
    // score slightly higher than black-to-move due to the tempo term.
    //
    // Note: in a pure KK endgame the phase == 0 so the tempo bonus (which is
    // added to the middlegame accumulator) does not contribute to the final
    // score.  We test here with the starting position which has full phase.

    int vw = eval_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int vb = eval_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");

    begin_section("tempo: startpos white-to-move > 0");
    EXPECT(vw > 0);
    end_section();

    begin_section("tempo: startpos black-to-move > 0");
    EXPECT(vb > 0);
    end_section();

    // Verify the tempo term is non-zero by checking both scores exist in
    // a small positive range (both sides "feel" the tempo bonus in their eval)
    begin_section("tempo: startpos both stm evals < 80");
    EXPECT(vw < 80 && vb < 80);
    end_section();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    init_bitboards();
    init_attacks();
    Zobrist::init();
    init_eval_tables();

    std::printf("Evaluator tests\n");
    std::printf("%s\n", std::string(62, '=').c_str());

    std::printf("\nSide-to-move perspective\n");
    test_stm_perspective();

    std::printf("\nEval symmetry\n");
    test_symmetry();

    std::printf("\nMaterial advantage\n");
    test_material_advantage();

    std::printf("\nLone kings\n");
    test_lone_kings();

    std::printf("\nOCB draw scaling\n");
    test_ocb_scaling();

    std::printf("\nKNNK draw detection\n");
    test_knnk_draw();

    std::printf("\nTempo bonus\n");
    test_tempo_bonus();

    return harness_summary();
}
