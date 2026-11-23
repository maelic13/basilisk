/// Evaluator tests.
///
/// Covers: side-to-move perspective, eval symmetry (mirrored positions),
/// material advantage, lone kings, OCB draw scaling, KNNK draw detection,
/// tempo bonus.
///
/// Build:
///   cmake --build --preset release --target test_eval
///   ./build/release/test_eval

#include "bench.h"
#include "board.h"
#include "eval.h"
#include "attacks.h"
#include "bitboard.h"
#include "wac.h"
#include "zobrist.h"
#include "colour_mirror.h"
#include "test_harness.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int eval_fen(const char* fen) {
    Board b;
    b.set_fen(fen);
    Evaluator ev;
    return ev.evaluate(b);
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

static void test_pawnless_scaler_tie() {
    // Equal material totals (R+N+B = 1150 against Q + 2 pawns = 1150) with a
    // non-pawn gap inside a bishop, where only one side has pawns. Taking the
    // winning side from the totals made it White at the tie, so the position
    // and its colour mirror were scaled differently.
    const char* fen = "3qk3/pp6/8/8/8/8/8/RNB1K3 w - - 0 1";
    const std::string mirrored = mirror_fen(fen);
    begin_section("pawnless scaler: an equal-totals position equals its mirror");
    EXPECT_EQ(eval_fen(fen), eval_fen(mirrored.c_str()));
    end_section();
}

static void test_symmetry_sweep() {
    // Every position equals its colour mirror. The sample is versioned by its
    // sources and seed: the bench and WAC positions, odd and promoted material
    // (three rooks, two queens, three knights, same-colour bishops), each
    // followed by a seeded random walk.
    std::vector<std::string> sources;
    for (std::string_view fen : bench_fens())
        sources.emplace_back(fen);
    for (const WacPosition& w : wac_positions())
        sources.push_back(w.fen);
    static const char* odd[] = {
        "4k3/8/8/8/8/8/8/RRR1K3 w - - 0 1",
        "1r2k2r/8/8/8/8/8/8/R2RK2R w - - 0 1",
        "q3k2q/8/8/8/8/8/8/Q3K2Q w - - 0 1",
        "4k3/8/8/8/8/8/8/NNN1K3 w - - 0 1",
        "4k3/8/8/8/8/8/8/1B2KB1B w - - 0 1",
        "3qk3/pp6/8/8/8/8/8/RNB1K3 w - - 0 1",
        "r3k3/pppp4/8/8/8/8/4PPPP/QQ2K3 w - - 0 1",
        "2r1k1r1/8/8/3pp3/3PP3/8/8/RRR1K3 b - - 0 1",
        "nnn1k3/pp6/8/8/8/8/6PP/4KBBB w - - 0 1",
        "4k3/1q6/8/8/8/8/6R1/4K1R1 w - - 0 1",
    };
    for (const char* fen : odd)
        sources.emplace_back(fen);

    std::mt19937_64 rng(0xC0105EED11u);
    int checked = 0, asymmetric = 0;
    std::string first;
    for (const std::string& start : sources) {
        Board b;
        b.set_fen(start);
        for (int ply = 0; ply <= 8; ++ply) {
            const std::string fen = b.get_fen();
            const std::string mirrored = mirror_fen(fen.c_str());
            const int a = eval_fen(fen.c_str());
            const int m = eval_fen(mirrored.c_str());
            ++checked;
            if (a != m && asymmetric++ == 0)
                first = fen + " (" + std::to_string(a) + " vs " + std::to_string(m) + ")";
            MoveList legal;
            b.gen_legal(legal);
            if (legal.size() == 0)
                break;
            b.make_move(legal[static_cast<int>(rng() % static_cast<uint64_t>(legal.size()))]);
        }
    }

    begin_section("symmetry sweep: every sampled position equals its mirror");
    if (asymmetric)
        std::printf("    %d of %d asymmetric; first: %s\n", asymmetric, checked, first.c_str());
    EXPECT_EQ(asymmetric, 0);
    EXPECT(checked > 3000);
    end_section();
}

static void test_bare_king_material_is_monotone() {
    // Against a bare king more material never scores lower. From KBP-K every
    // promotion wins, and the order must be queen, rook, then knight or
    // bishop; the old overrides scored KBN-K at 15,600 or more against about
    // 2,400 for KQB-K, so the engine promoted to a knight. Each child has
    // Black to move, so White's score is the negated evaluation.
    auto white_after = [](const char* fen) { return -eval_fen(fen); };
    const int q = white_after("1B3Q2/8/8/2k2K2/8/8/8/8 b - - 0 77");
    const int r = white_after("1B3R2/8/8/2k2K2/8/8/8/8 b - - 0 77");
    const int n = white_after("1B3N2/8/8/2k2K2/8/8/8/8 b - - 0 77");
    const int bi = white_after("1B3B2/8/8/2k2K2/8/8/8/8 b - - 0 77");
    begin_section("bare king: a queen promotion outranks a rook, a rook a minor piece");
    EXPECT(q > r);
    EXPECT(r > n);
    EXPECT(r > bi);
    end_section();

    begin_section("bare king: shedding a pawn never raises the score");
    EXPECT(eval_fen("1B6/5P2/8/2k2K2/8/8/N7/8 w - - 0 77")
           >= eval_fen("1B6/8/8/2k2K2/8/8/N7/8 w - - 0 77"));
    EXPECT(eval_fen("1B6/5P2/8/2k2K2/8/8/8/R7 w - - 0 77")
           >= eval_fen("1B6/8/8/2k2K2/8/8/8/R7 w - - 0 77"));
    end_section();

    // The halfmove clock damps only the score above a tier's base. Damped
    // whole, the rook tier fell about 130 points a ply, more than the king
    // drive KR-K is mated with, and at clock 99 it fell below a minor-piece
    // mate at clock 0.
    const int r99 = white_after("1B3R2/8/8/2k2K2/8/8/8/8 b - - 99 77");
    const int q99 = white_after("1B3Q2/8/8/2k2K2/8/8/8/8 b - - 99 77");
    begin_section("bare king: the tier order holds at any halfmove clock");
    EXPECT(q99 > r);
    EXPECT(r99 > n);
    EXPECT(r99 > bi);
    end_section();

    begin_section("bare king: the clock damps the rook tier's score, not its base");
    EXPECT(r99 < r);
    EXPECT(r - r99 < (r - BARE_KING_ROOK_TIER) / 2 + 1);
    end_section();
}

static void test_top_tiers_read_no_fitted_weight() {
    // KQ-K and KR-K score their tier plus frozen material and the donor's
    // KXK drive (the C.2.8 registration's Amendment 2). Before, they added
    // the winner's ordinary score, so every refit reshaped the only gradient
    // these mates were converted with.
    static const char* const FENS[] = {
        "4K3/8/8/8/8/5k2/8/1R6 w - - 0 1",   // C.2.8 run 2's unconverted KR-K
        "8/8/3k4/8/8/2K5/8/Q7 w - - 0 1",
        "8/2P5/3k4/8/8/2K5/8/R7 w - - 0 1",
    };
    int before[3];
    for (int i = 0; i < 3; i++)
        before[i] = eval_fen(FENS[i]);
    const EvalParams saved = g_eval_params;
    for (int sq = 0; sq < 64; sq++) {
        for (int pt = 0; pt < 6; pt++) {
            g_eval_params.pst_eg[pt][sq] += 37 * (sq % 5) - 50;
            g_eval_params.pst_mg[pt][sq] -= 23 * (sq % 3);
        }
    }
    for (int i = 0; i < 15; i++) {
        g_eval_params.mob_r_eg[i] += 40 - 9 * i;
        g_eval_params.mob_r_mg[i] += 5 * i;
    }
    init_eval_tables();
    int after[3];
    for (int i = 0; i < 3; i++)
        after[i] = eval_fen(FENS[i]);
    g_eval_params = saved;
    init_eval_tables();
    begin_section("bare king: a top-tier score reads no fitted weight");
    for (int i = 0; i < 3; i++)
        EXPECT_EQ(before[i], after[i]);
    end_section();

    begin_section("bare king: a top-tier score stays inside its tier");
    const int r = eval_fen(FENS[0]), q = eval_fen(FENS[1]), rp = eval_fen(FENS[2]);
    EXPECT(r > BARE_KING_ROOK_TIER && r < BARE_KING_QUEEN_TIER);
    EXPECT(rp > BARE_KING_ROOK_TIER && rp < BARE_KING_QUEEN_TIER);
    EXPECT(q > BARE_KING_QUEEN_TIER);
    end_section();

    // The drive spans less than a pawn, so material orders the tier: a pawn
    // more wins whatever the kings, and the edge and the closer king win at
    // equal material.
    begin_section("bare king: the drive orders equal material, a pawn outranks it");
    const int centre_far = eval_fen("7K/8/8/8/3k4/8/8/R7 w - - 0 1");
    const int edge_near  = eval_fen("7R/8/8/8/8/8/2K5/k7 w - - 0 1");
    const int centre_far_pawn = eval_fen("7K/8/8/8/3k4/8/7P/R7 w - - 0 1");
    EXPECT(edge_near > centre_far);
    EXPECT(centre_far_pawn > edge_near);
    end_section();
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

    // 8.3: the OCB draw scale can never amplify -- capped at neutral 48/48.
    begin_section("OCB scale formula: <= neutral at every pawn count, monotone");
    for (int pawns = 0; pawns <= 16; pawns++) {
        EXPECT(ocb_draw_scale(pawns) <= 48);
        if (pawns > 0) EXPECT(ocb_draw_scale(pawns) >= ocb_draw_scale(pawns - 1));
    }
    EXPECT_EQ(ocb_draw_scale(0), 32);   // pure OCB: strongest draw pull
    EXPECT_EQ(ocb_draw_scale(4), 48);   // behaviour identical to pre-8.3 at <= 4
    EXPECT_EQ(ocb_draw_scale(16), 48);  // was 96 (2x amplification) pre-8.3
    end_section();

    // Behavioural check: an OCB position with a big material edge and many
    // pawns must not evaluate ABOVE its same-colour-bishop twin (bishop moved
    // one square: h5 light -> h6 dark, same colour as white's c3). Pre-8.3
    // the 14-pawn multiplier was 1.83x and this failed by hundreds of cp.
    begin_section("OCB with many pawns: no amplification vs same-colour twin");
    int ocb  = eval_fen("1k1q4/ppppppp1/8/7b/8/2B5/PPPPPPP1/1K1Q3R w - - 0 1");
    int same = eval_fen("1k1q4/ppppppp1/8/8/8/2B4b/PPPPPPP1/1K1Q3R w - - 0 1");
    EXPECT(std::abs(ocb) <= std::abs(same) + 80);  // 80cp PST/mobility slack
    end_section();

    // Sign preservation: black up a rook in OCB stays negative for white.
    begin_section("OCB scaling preserves score sign");
    int vneg = eval_fen("1k1r4/pppppppp/8/7b/8/2B5/PPPPPPPP/1K6 w - - 0 1");
    EXPECT(vneg < 0);
    end_section();
}

// ---------------------------------------------------------------------------
// 8.3b: enemy rook behind passer — decoupled activation (audit hce_analysis 2)
// ---------------------------------------------------------------------------

// White-POV eval regardless of side to move.
static int eval_white_pov(const char* fen) {
    Board b;
    b.set_fen(fen);
    Evaluator ev;
    int v = ev.evaluate(b);
    return (b.turn() == WHITE) ? v : -v;
}

// Net signed activation count of the enemy-rook-behind-passer term, measured
// by perturbing its weight (mg == eg, so the tapered blend is exact and the
// eval difference is precisely W per activation). +1 = a black rook behind a
// white passer (penalty to white), -1 = mirrored.
static int enemy_rook_passer_activations(const char* fen) {
    const EvalParams saved = g_eval_params;
    const int W = 64;
    g_eval_params.enemy_rook_passer_mg = 0;
    g_eval_params.enemy_rook_passer_eg = 0;
    init_eval_tables();
    const int base = eval_white_pov(fen);
    g_eval_params.enemy_rook_passer_mg = W;
    g_eval_params.enemy_rook_passer_eg = W;
    init_eval_tables();
    const int perturbed = eval_white_pov(fen);
    g_eval_params = saved;
    init_eval_tables();
    // Rounded division: the taper's truncation-toward-zero costs 1cp when
    // the perturbation flips the score's sign (diff comes out W-1).
    const int diff = base - perturbed;
    return (diff >= 0 ? diff + W / 2 : diff - W / 2) / W;
}

static void test_rook_passer_decoupling() {
    // Positions keep material balanced (lazy eval short-circuits ~700cp and
    // skips this term) and give both sides a pawn (the strong-side-no-pawns
    // near-draw scaler would crush the measured diff).

    // Enemy rook behind our passer with the friendly rook OFF the passer
    // file: pre-8.3b the penalty was nested under the friendly-rook-on-file
    // loop and never fired here.
    begin_section("enemy rook behind passer, friendly rook off-file: fires");
    EXPECT_EQ(enemy_rook_passer_activations(
        "4k3/p7/8/4P3/8/8/4r3/R5K1 w - - 0 1"), 1);
    end_section();

    // Two stacked friendly rooks on the file: pre-8.3b the enemy penalty was
    // counted once per friendly rook (twice); now exactly once.
    begin_section("stacked friendly rooks: enemy penalty counted once");
    EXPECT_EQ(enemy_rook_passer_activations(
        "3rk3/p7/8/4P3/4R3/4R3/4r3/6K1 w - - 0 1"), 1);
    end_section();

    begin_section("enemy rook IN FRONT of passer: no penalty");
    EXPECT_EQ(enemy_rook_passer_activations(
        "4k3/p3r3/8/4P3/8/8/8/R5K1 w - - 0 1"), 0);
    end_section();

    // Mirrored: white rook behind a black passer.
    begin_section("mirrored (white rook behind black passer): fires once");
    EXPECT_EQ(enemy_rook_passer_activations(
        "r3k3/4R3/8/8/4p3/8/P7/4K3 w - - 0 1"), -1);
    end_section();

    // A blocker between rook and passer still counts: the term is purely
    // geometric by design (documented in eval.cpp), same as the friendly term.
    begin_section("blocker between rook and passer: still geometric");
    EXPECT_EQ(enemy_rook_passer_activations(
        "1n2k3/p7/8/4P3/8/4N3/4r3/R5K1 w - - 0 1"), 1);
    end_section();

    // No passer on the rook's file: nothing fires.
    begin_section("rook behind a non-passed pawn: no penalty");
    EXPECT_EQ(enemy_rook_passer_activations(
        "4k3/p3p3/8/4P3/8/8/4r3/R5K1 w - - 0 1"), 0);
    end_section();
}

// ---------------------------------------------------------------------------
// 8.3c: attacked2 records two-pawn double attacks (audit hce_analysis 3)
// ---------------------------------------------------------------------------

// Net signed activation count of the threat_hanging term (same perturbation
// technique as enemy_rook_passer_activations).
static int threat_hanging_activations(const char* fen) {
    const EvalParams saved = g_eval_params;
    const int W = 64;
    g_eval_params.threat_hanging_mg = 0;
    g_eval_params.threat_hanging_eg = 0;
    init_eval_tables();
    const int base = eval_white_pov(fen);
    g_eval_params.threat_hanging_mg = W;
    g_eval_params.threat_hanging_eg = W;
    init_eval_tables();
    const int perturbed = eval_white_pov(fen);
    g_eval_params = saved;
    init_eval_tables();
    const int diff = perturbed - base;  // hanging is a bonus for the attacker
    return (diff >= 0 ? diff + W / 2 : diff - W / 2) / W;
}

static void test_attacked2_pawn_pairs() {
    // Positions carry shelter pawns on both wings so the perturbed eval stays
    // well inside +-200cp: at phase <= 6 the frozen endgame mate-drive adds a
    // ~44cp step the moment |score| crosses 200, which would pollute the
    // measured activation count.

    // Black knight d5 is defended once (rook d8) and attacked ONLY by the two
    // white pawns c4/e4. It is "hanging" (weak non-pawn attacked twice) iff
    // attacked2 records the two-pawn double attack -- pre-8.3c the pawn set
    // was seeded as one unit and this counted zero.
    begin_section("two pawns converging: defended piece counts as hanging");
    EXPECT_EQ(threat_hanging_activations(
        "3r3k/4pppp/8/3n4/2P1P3/8/6PP/R5NK w - - 0 1"), 1);
    end_section();

    // Control twin: single pawn attacker -- not attacked twice, not hanging.
    begin_section("single pawn attacker: not hanging");
    EXPECT_EQ(threat_hanging_activations(
        "3r3k/5ppp/8/3n4/4P3/8/6PP/R5NK w - - 0 1"), 0);
    end_section();

    // Mirrored: white knight d4 defended by Rd1, attacked by black pawns
    // c5/e5 only.
    begin_section("mirrored two-pawn double attack");
    EXPECT_EQ(threat_hanging_activations(
        "r5nk/6pp/8/2p1p3/3N4/8/4PPPP/3R3K w - - 0 1"), -1);
    end_section();
}

// ---------------------------------------------------------------------------
// 8.6: mate-drive gating (audit hce_analysis 7)
// ---------------------------------------------------------------------------

// eval(defender king toward edge) - eval(defender king central), white POV.
// The frozen mate-drive rewards edge-driving beyond the king PST. Measuring
// it directly is confounded by (a) lazy eval, which short-circuits above
// ~700cp and skips the term whenever a lone king is fully cornered, and
// (b) the king PST itself. Both are cancelled with a difference-of-
// differences: KQ-vs-KR keeps the eval in the (200, 700) active band at
// moderate king squares, White's king is equidistant from both defender-king
// test squares (so the mate-drive's king_dist term cancels), and the gate is
// toggled by an equidistant defender pawn (whose passed-pawn value therefore
// cancels too). The DiD is then purely the mate-drive's lk_center geometry.
static int edge_minus_central(const char* edge_fen, const char* central_fen) {
    return eval_white_pov(edge_fen) - eval_white_pov(central_fen);
}

static void test_mate_drive_gating() {
    // Defender king a5 (edge) vs d5 (central); White Ke1 is Chebyshev-distance
    // 4 from both. Winning KQ vs defender KR, all in the active band.
    const int on_pawnless = edge_minus_central(
        "8/8/8/k7/8/8/3r4/4K2Q w - - 0 1",     // bK a5, pawnless -> drive ON
        "8/8/8/3k4/8/8/3r4/4K2Q w - - 0 1");   // bK d5, pawnless -> drive ON
    // Defender pawn on b7 is Chebyshev-distance 2 from both a5 and d5, so its
    // eval is identical in both and cancels; its only role is to gate the
    // mate-drive off (defender no longer pawnless).
    const int off_with_pawn = edge_minus_central(
        "8/1p6/8/k7/8/8/3r4/4K2Q w - - 0 1",   // + bP b7 -> drive OFF
        "8/1p6/8/3k4/8/8/3r4/4K2Q w - - 0 1");
    std::printf("  pawn gate: on=%d off=%d (DiD=%d)\n",
                on_pawnless, off_with_pawn, on_pawnless - off_with_pawn);
    begin_section("mate-drive drives the pawnless defender to the edge");
    EXPECT(on_pawnless > 20);          // ~33cp edge preference when firing
    end_section();
    begin_section("mate-drive is gated off once the defender has a pawn");
    EXPECT(on_pawnless - off_with_pawn > 20);  // ~38cp isolated contribution
    end_section();

    // Mating-material side of the gate: a lone knight cannot force mate, and
    // the whole KN-vs-K position is scaled to a dead draw by apply_endgame, so
    // no spurious edge-driving can leak through. (Its should-block siblings —
    // Km-vs-K, KNN-vs-K — are likewise zeroed/overridden, which is why the
    // win_can_mate clause is belt-and-suspenders rather than independently
    // observable.)
    begin_section("no mate-drive leak in a non-mating pawnless ending");
    EXPECT_EQ(eval_fen("8/8/8/k7/8/8/8/4K2N w - - 0 1"), 0);
    EXPECT_EQ(eval_fen("8/8/8/3k4/8/8/8/4K2N w - - 0 1"), 0);
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

#ifdef BASILISK_TUNE
static EndgameOccurrenceCounters classify_endgame(const char* fen) {
    Board b;
    b.set_fen(fen);
    Evaluator ev;
    ev.diag_endgames = true;
    (void)ev.evaluate(b);
    return ev.endgame_occurrence;
}

static void test_endgame_occurrence_classifier() {
    begin_section("endgame occurrence: >7 men excluded");
    const auto opening = classify_endgame(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(opening.classified, 0);
    end_section();

    begin_section("endgame occurrence: KPK also counts KPsK");
    const auto kpk = classify_endgame("7k/8/8/8/8/8/P7/K7 w - - 0 1");
    EXPECT_EQ(kpk.classified, 1);
    EXPECT_EQ(kpk.kpk, 1);
    EXPECT_EQ(kpk.kpsk, 1);
    end_section();

    begin_section("endgame occurrence: symmetric KPKP counted once");
    const auto kpkp = classify_endgame("7k/7p/8/8/8/8/P7/K7 w - - 0 1");
    EXPECT_EQ(kpkp.kpkp, 1);
    end_section();

    begin_section("endgame occurrence: reversed KRPKB orientation");
    const auto krpkb = classify_endgame("4k3/8/8/8/8/p7/r7/B3K3 w - - 0 1");
    EXPECT_EQ(krpkb.krpkb, 1);
    end_section();

    begin_section("endgame occurrence: KQKRPs accepts multiple pawns");
    const auto kqkrps = classify_endgame("4k3/pp6/r7/8/8/8/Q7/4K3 w - - 0 1");
    EXPECT_EQ(kqkrps.kqkrps, 1);
    end_section();

    begin_section("endgame occurrence: KBNK overlaps valid KXK");
    const auto kbnk = classify_endgame("7k/8/8/8/8/8/BN6/K7 w - - 0 1");
    EXPECT_EQ(kbnk.kbnk, 1);
    EXPECT_EQ(kbnk.kxk, 1);
    end_section();

    begin_section("endgame occurrence: dead KBK/KNK excluded from KXK");
    const auto kbk = classify_endgame("7k/8/8/8/8/8/B7/K7 w - - 0 1");
    const auto knk = classify_endgame("7k/8/8/8/8/8/N7/K7 w - - 0 1");
    EXPECT_EQ(kbk.kxk, 0);
    EXPECT_EQ(knk.kxk, 0);
    end_section();

    begin_section("endgame occurrence: KBBK included in KXK");
    // a2 is light and b2 is dark: a genuine, mating bishop pair.
    const auto kbbk = classify_endgame("7k/8/8/8/8/8/BB6/K7 w - - 0 1");
    EXPECT_EQ(kbbk.kxk, 1);
    end_section();

    begin_section("endgame occurrence: same-coloured KBBK excluded from KXK");
    // b2 and d2 are both dark, so this promotion-reachable pair cannot mate.
    // apply_endgame's KXK gate tests colour, not count; the census must agree.
    const auto kbbk_same = classify_endgame("7k/8/8/8/8/8/1B1B4/K7 w - - 0 1");
    EXPECT_EQ(kbbk_same.classified, 1);
    EXPECT_EQ(kbbk_same.kxk, 0);
    end_section();

    begin_section("endgame occurrence: KQK and KRK remain in the KXK family");
    const auto kqk = classify_endgame("7k/8/8/8/8/8/Q7/K7 w - - 0 1");
    const auto krk = classify_endgame("7k/8/8/8/8/8/R7/K7 w - - 0 1");
    EXPECT_EQ(kqk.kxk, 1);
    EXPECT_EQ(krk.kxk, 1);
    end_section();
}

#endif

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

static void test_rook_ending_scaling() {
    // PLAN 6.5.a. These are draw-SCALING rules, not verdicts, and the property
    // that makes them safe is the floor: no rule here may assert a dead draw,
    // because none of them is backed by a tablebase. Checked against Syzygy on
    // 3,000 random KRPKR positions the unfloored rules called 18 genuinely WON
    // positions dead draws; with the floor, none (BAS-E54).

    // Philidor third-rank defence: white pawn on the fifth, black king on the
    // queening square, black rook cutting on the sixth. The control moves ONLY
    // the defending rook off the sixth, which is what the rule keys on.
    begin_section("Philidor KRPKR scores below the same position without the cut");
    int held = eval_fen("8/3k4/r7/3PK3/8/8/8/1R6 w - - 0 1");
    int uncut = eval_fen("r7/3k4/8/3PK3/8/8/8/1R6 w - - 0 1");
    EXPECT(held < uncut);
    end_section();

    // The floor, stated as a test: a scaled rook ending keeps a signal. Zero
    // here would mean a rule had asserted a draw it cannot prove.
    begin_section("scaled Philidor is discounted, never zeroed");
    EXPECT(held != 0);
    end_section();

    // KRPPKRP: neither white pawn is passed and the black king blockades in
    // front of both, so the extra pawn is heavily discounted.
    begin_section("blockaded KRPP-KRP keeps far less than a clean extra pawn");
    int blocked = eval_fen("7r/8/5k2/4p3/4PP2/8/8/R5K1 w - - 0 1");
    // The control moves only the defending king, two files, out of the zone
    // the rule keys on, as the Philidor test above moves only the rook. The
    // bound is a ratio, so a refit's larger scores do not break it (C-70).
    // Calibrated to discriminate: with the rule 22 of 63 (35%) on the header
    // and 53 of 146 (36%) on C.2.8's fit; without it 59 of 63 and 143 of 146.
    int unblocked = eval_fen("7r/8/3k4/4p3/4PP2/8/8/R5K1 w - - 0 1");
    EXPECT(blocked * 2 < unblocked);
    EXPECT(blocked != 0);
    end_section();
}

// ---------------------------------------------------------------------------
// PST orientation
//
// EvalParams stores every piece-square table with index 0 = a1 (White's
// view). PeSTO publishes its tables rank 8 first, so each needs a rank flip
// on import; seven of them once missed it and paid a White knight on f3 the
// f6 value. Fits have moved the values since, so the check is relative:
// each table must sit closer to the a1-first PeSTO table than to its rank
// mirror, by summed absolute difference.
//
// Fixture: PeSTO's tables exactly as published (Ronald Friederich, Chess
// Programming Wiki, CC BY-SA 3.0), rank 8 first, mg then eg, pawn..king.
// ---------------------------------------------------------------------------

static constexpr int PESTO_PUBLISHED[2][6][64] = {
    {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
            98,  134,   61,   95,   68,  126,   34,  -11,
            -6,    7,   26,   31,   65,   56,   25,  -20,
           -14,   13,    6,   21,   23,   12,   17,  -23,
           -27,   -2,   -5,   12,   17,    6,   10,  -25,
           -26,   -4,   -4,  -10,    3,    3,   33,  -12,
           -35,   -1,  -20,  -23,  -15,   24,   38,  -22,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        { -167,  -89,  -34,  -49,   61,  -97,  -15, -107,
           -73,  -41,   72,   36,   23,   62,    7,  -17,
           -47,   60,   37,   65,   84,  129,   73,   44,
            -9,   17,   19,   53,   37,   69,   18,   22,
           -13,    4,   16,   13,   28,   19,   21,   -8,
           -23,   -9,   12,   10,   19,   17,   25,  -16,
           -29,  -53,  -12,   -3,   -1,   18,  -14,  -19,
          -105,  -21,  -58,  -33,  -17,  -28,  -19,  -23 },
        // BISHOP
        {  -29,    4,  -82,  -37,  -25,  -42,    7,   -8,
           -26,   16,  -18,  -13,   30,   59,   18,  -47,
           -16,   37,   43,   40,   35,   50,   37,   -2,
            -4,    5,   19,   50,   37,   37,    7,   -2,
            -6,   13,   13,   26,   34,   12,   10,    4,
             0,   15,   15,   15,   14,   27,   18,   10,
             4,   15,   16,    0,    7,   21,   33,    1,
           -33,   -3,  -14,  -21,  -13,  -12,  -39,  -21 },
        // ROOK
        {   32,   42,   32,   51,   63,    9,   31,   43,
            27,   32,   58,   62,   80,   67,   26,   44,
            -5,   19,   26,   36,   17,   45,   61,   16,
           -24,  -11,    7,   26,   24,   35,   -8,  -20,
           -36,  -26,  -12,   -1,    9,   -7,    6,  -23,
           -45,  -25,  -16,  -17,    3,    0,   -5,  -33,
           -44,  -16,  -20,   -9,   -1,   11,   -6,  -71,
           -19,  -13,    1,   17,   16,    7,  -37,  -26 },
        // QUEEN
        {  -28,    0,   29,   12,   59,   44,   43,   45,
           -24,  -39,   -5,    1,  -16,   57,   28,   54,
           -13,  -17,    7,    8,   29,   56,   47,   57,
           -27,  -27,  -16,  -16,   -1,   17,   -2,    1,
            -9,  -26,   -9,  -10,   -2,   -4,    3,   -3,
           -14,    2,  -11,   -2,   -5,    2,   14,    5,
           -35,   -8,   11,    2,    8,   15,   -3,    1,
            -1,  -18,   -9,   10,  -15,  -25,  -31,  -50 },
        // KING
        {  -65,   23,   16,  -15,  -56,  -34,    2,   13,
            29,   -1,  -20,   -7,   -8,   -4,  -38,  -29,
            -9,   24,    2,  -16,  -20,    6,   22,  -22,
           -17,  -20,  -12,  -27,  -30,  -25,  -14,  -36,
           -49,   -1,  -27,  -39,  -46,  -44,  -33,  -51,
           -14,  -14,  -22,  -46,  -44,  -30,  -15,  -27,
             1,    7,   -8,  -64,  -43,  -16,    9,    8,
           -15,   36,   12,  -54,    8,  -28,   24,   14 },
    },
    {
        // PAWN
        {    0,    0,    0,    0,    0,    0,    0,    0,
           178,  173,  158,  134,  147,  132,  165,  187,
            94,  100,   85,   67,   56,   53,   82,   84,
            32,   24,   13,    5,   -2,    4,   17,   17,
            13,    9,   -3,   -7,   -7,   -8,    3,   -1,
             4,    7,   -6,    1,    0,   -5,   -1,   -8,
            13,    8,    8,   10,   13,    0,    2,   -7,
             0,    0,    0,    0,    0,    0,    0,    0 },
        // KNIGHT
        {  -58,  -38,  -13,  -28,  -31,  -27,  -63,  -99,
           -25,   -8,  -25,   -2,   -9,  -25,  -24,  -52,
           -24,  -20,   10,    9,   -1,   -9,  -19,  -41,
           -17,    3,   22,   22,   22,   11,    8,  -18,
           -18,   -6,   16,   25,   16,   17,    4,  -18,
           -23,   -3,   -1,   15,   10,   -3,  -20,  -22,
           -42,  -20,  -10,   -5,   -2,  -20,  -23,  -44,
           -29,  -51,  -23,  -15,  -22,  -18,  -50,  -64 },
        // BISHOP
        {  -14,  -21,  -11,   -8,   -7,   -9,  -17,  -24,
            -8,   -4,    7,  -12,   -3,  -13,   -4,  -14,
             2,   -8,    0,   -1,   -2,    6,    0,    4,
            -3,    9,   12,    9,   14,   10,    3,    2,
            -6,    3,   13,   19,    7,   10,   -3,   -9,
           -12,   -3,    8,   10,   13,    3,   -7,  -15,
           -14,  -18,   -7,   -1,    4,   -9,  -15,  -27,
           -23,   -9,  -23,   -5,   -9,  -16,   -5,  -17 },
        // ROOK
        {   13,   10,   18,   15,   12,   12,    8,    5,
            11,   13,   13,   11,   -3,    3,    8,    3,
             7,    7,    7,    5,    4,   -3,   -5,   -3,
             4,    3,   13,    1,    2,    1,   -1,    2,
             3,    5,    8,    4,   -5,   -6,   -8,  -11,
            -4,    0,   -5,   -1,   -7,  -12,   -8,  -16,
            -6,   -6,    0,    2,   -9,   -9,  -11,   -3,
            -9,    2,    3,   -1,   -5,  -13,    4,  -20 },
        // QUEEN
        {   -9,   22,   22,   27,   27,   19,   10,   20,
           -17,   20,   32,   41,   58,   25,   30,    0,
           -20,    6,    9,   49,   47,   35,   19,    9,
             3,   22,   24,   45,   57,   40,   57,   36,
           -18,   28,   19,   47,   31,   34,   39,   23,
           -16,  -27,   15,    6,    9,   17,   10,    5,
           -22,  -23,  -30,  -16,  -16,  -23,  -36,  -32,
           -33,  -28,  -22,  -43,   -5,  -32,  -20,  -41 },
        // KING
        {  -74,  -35,  -18,  -18,  -11,   15,    4,  -17,
           -12,   17,   14,   17,   17,   38,   23,   11,
            10,   17,   23,   15,   20,   45,   44,   13,
            -8,   22,   24,   27,   26,   33,   26,    3,
           -18,   -4,   21,   24,   27,   23,    9,  -11,
           -19,   -3,   11,   21,   23,   16,    7,   -9,
           -27,  -11,    4,   13,   14,    4,   -5,  -17,
           -53,  -34,  -21,  -11,  -28,  -14,  -24,  -43 },
    },
};

// The imbalance table answers by the piece counts, and a parameter change must
// not leave it answering on the old coefficients.
static void test_imbalance_cache() {
    const char* const fens[] = {
        "r3k3/pppp4/8/8/8/8/PPPP4/2B1K3 w - - 0 1",
        "r1bqkbnr/pppppppp/2n5/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1",
        "4k3/pp6/8/8/8/8/PP6/R3K3 w - - 0 1",
        "2r3k1/1q2Rp1p/p2p2p1/1p1P4/1Pp1P3/2Q5/1P4PP/6K1 w - - 0 1",
    };
    auto warm = std::make_unique<Evaluator>();
    begin_section("imbalance table: a warm evaluator reads as a fresh one");
    for (int round = 0; round < 2; ++round)
        for (const char* fen : fens) {
            Board b;
            b.set_fen(fen);
            auto fresh = std::make_unique<Evaluator>();
            EXPECT_EQ(warm->evaluate(b), fresh->evaluate(b));
        }
    end_section();

    Board b;
    b.set_fen(fens[0]);
    const int before = warm->evaluate(b);
    const int saved = g_eval_params.imb_linear[3];
    g_eval_params.imb_linear[3] = saved + 40;
    init_eval_tables();
    const int changed = warm->evaluate(b);
    auto fresh = std::make_unique<Evaluator>();
    const int expected = fresh->evaluate(b);
    g_eval_params.imb_linear[3] = saved;
    init_eval_tables();
    begin_section("imbalance table: new coefficients are read at once");
    EXPECT(changed != before);
    EXPECT_EQ(changed, expected);
    EXPECT_EQ(warm->evaluate(b), before);
    end_section();
}

static void test_pst_orientation() {
    static const char* const kNames[2][6] = {
        { "mg pawn", "mg knight", "mg bishop", "mg rook", "mg queen", "mg king" },
        { "eg pawn", "eg knight", "eg bishop", "eg rook", "eg queen", "eg king" },
    };
    const EvalParams p{};
    for (int phase = 0; phase < 2; phase++) {
        for (int piece = 0; piece < 6; piece++) {
            const int* table = phase == 0 ? p.pst_mg[piece] : p.pst_eg[piece];
            const int* published = PESTO_PUBLISHED[phase][piece];
            int to_a1_first = 0;  // distance to the correctly flipped table
            int to_mirror = 0;    // distance to the table left rank 8 first
            for (int sq = 0; sq < 64; sq++) {
                to_a1_first += std::abs(table[sq] - published[sq ^ 56]);
                to_mirror += std::abs(table[sq] - published[sq]);
            }
            begin_section((std::string(kNames[phase][piece]) + ": a1-first orientation").c_str());
            EXPECT(to_a1_first < to_mirror);
            end_section();
        }
    }
}

// ---------------------------------------------------------------------------
// Cache purity and the lazy checkpoint
// ---------------------------------------------------------------------------

struct Squares {
    Piece p[SQUARE_NB]{};
    Piece& operator[](int sq) { return p[sq]; }
    const Piece& operator[](int sq) const { return p[sq]; }
};

static Squares squares_of(const Board& b) {
    Squares s{};
    for (int sq = 0; sq < SQUARE_NB; ++sq)
        s[sq] = b.piece_on(Square(sq));
    return s;
}

// A FEN without castling or en passant, so that every variant below is a
// position of its own placement only.
static std::string fen_of(const Squares& s, Color stm) {
    static const char piece_chars[] = ".pnbrqk";
    std::string placement;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            const Piece p = s[make_square(File(f), Rank(r))];
            if (p == NO_PIECE) { ++empty; continue; }
            if (empty) { placement += static_cast<char>('0' + empty); empty = 0; }
            char ch = piece_chars[type_of(p)];
            if (color_of(p) == WHITE) ch = static_cast<char>(ch - 'a' + 'A');
            placement += ch;
        }
        if (empty) placement += static_cast<char>('0' + empty);
        if (r > 0) placement += '/';
    }
    return placement + (stm == WHITE ? " w" : " b") + " - - 0 1";
}

// The bench and WAC positions, each also after four and eight seeded random
// plies, without castling or en passant.
static std::vector<std::string> purity_sample() {
    std::vector<std::string> sources;
    for (std::string_view fen : bench_fens())
        sources.emplace_back(fen);
    for (const WacPosition& w : wac_positions())
        sources.push_back(w.fen);
    std::mt19937_64 rng(0xCAC4E5EEDull);
    std::vector<std::string> out;
    for (const std::string& start : sources) {
        Board b;
        b.set_fen(start);
        for (int ply = 0; ply <= 8; ++ply) {
            if (ply % 4 == 0)
                out.push_back(fen_of(squares_of(b), b.turn()));
            MoveList legal;
            b.gen_legal(legal);
            if (legal.size() == 0)
                break;
            b.make_move(legal[static_cast<int>(rng() % static_cast<uint64_t>(legal.size()))]);
        }
    }
    return out;
}

// Evaluates `first` and then `second` on one warm evaluator; true when the
// second reads as it does on an evaluator whose caches are empty.
static bool warm_reads_fresh(Evaluator& warm, Evaluator& fresh,
                             const std::string& first, const std::string& second) {
    Board a, b;
    a.set_fen(first);
    b.set_fen(second);
    warm.evaluate(a);
    const int w = warm.evaluate(b);
    fresh.clear_pawn_table();
    return w == fresh.evaluate(b);
}

static void test_cache_purity() {
    const std::vector<std::string> sample = purity_sample();
    auto warm = std::make_unique<Evaluator>();
    auto fresh = std::make_unique<Evaluator>();

    // The pawn cache is a function of the pawns alone: the same pawns with the
    // pieces removed, or with the kings moved, must read the entry the full
    // position left, and the full position the entry they left.
    int pawn_pairs = 0, pawn_stale = 0;
    std::string pawn_first;
    for (const std::string& fen : sample) {
        Board b;
        b.set_fen(fen);
        const Squares full = squares_of(b);
        Squares bare{};
        Squares moved = full;
        for (int sq = 0; sq < SQUARE_NB; ++sq)
            if (type_of(full[sq]) == PAWN || type_of(full[sq]) == KING)
                bare[sq] = full[sq];
        // Each king to the first empty square from its own corner.
        for (Color c : {WHITE, BLACK}) {
            const Square k = b.king_square(c);
            for (int i = 0; i < SQUARE_NB; ++i) {
                const int sq = c == WHITE ? i : SQUARE_NB - 1 - i;
                if (moved[sq] == NO_PIECE) {
                    moved[sq] = moved[k];
                    moved[k] = NO_PIECE;
                    break;
                }
            }
        }
        for (const Squares* v : {&bare, &moved}) {
            const std::string variant = fen_of(*v, b.turn());
            for (int order = 0; order < 2; ++order) {
                const bool ok = order == 0 ? warm_reads_fresh(*warm, *fresh, fen, variant)
                                           : warm_reads_fresh(*warm, *fresh, variant, fen);
                ++pawn_pairs;
                if (!ok && pawn_stale++ == 0)
                    pawn_first = fen + " / " + variant;
            }
        }
    }
    begin_section("pawn cache: a function of the pawns alone");
    if (pawn_stale)
        std::printf("    %d of %d pairs stale; first: %s\n", pawn_stale, pawn_pairs, pawn_first.c_str());
    EXPECT_EQ(pawn_stale, 0);
    EXPECT(pawn_pairs > 2000);
    end_section();

    // The imbalance cache is a function of the ten piece counts alone: the
    // file mirror keeps the counts and moves every piece (and every bishop to
    // the other colour); one piece fewer changes a count and must miss.
    int imb_pairs = 0, imb_stale = 0;
    std::string imb_first;
    for (const std::string& fen : sample) {
        Board b;
        b.set_fen(fen);
        const Squares full = squares_of(b);
        Squares mirrored{};
        Squares fewer = full;
        for (int sq = 0; sq < SQUARE_NB; ++sq)
            mirrored[sq ^ 7] = full[sq];
        for (int sq = 0; sq < SQUARE_NB; ++sq)
            if (full[sq] != NO_PIECE && type_of(full[sq]) != KING) {
                fewer[sq] = NO_PIECE;
                break;
            }
        for (const Squares* v : {&mirrored, &fewer}) {
            const std::string variant = fen_of(*v, b.turn());
            for (int order = 0; order < 2; ++order) {
                const bool ok = order == 0 ? warm_reads_fresh(*warm, *fresh, fen, variant)
                                           : warm_reads_fresh(*warm, *fresh, variant, fen);
                ++imb_pairs;
                if (!ok && imb_stale++ == 0)
                    imb_first = fen + " / " + variant;
            }
        }
    }
    begin_section("imbalance cache: a function of the ten piece counts alone");
    if (imb_stale)
        std::printf("    %d of %d pairs stale; first: %s\n", imb_stale, imb_pairs, imb_first.c_str());
    EXPECT_EQ(imb_stale, 0);
    EXPECT(imb_pairs > 2000);
    end_section();
}

// The groups whose terms sit before the lazy checkpoint: what a lazy-served
// score is made of. Every other group sits after it.
static bool before_lazy_checkpoint(const std::string& group) {
    static const char* const cheap[] = {
        // Material and the piece-square tables.
        "MgVal", "EgVal",
        "PstMgPawn", "PstMgKnight", "PstMgBishop", "PstMgRook", "PstMgQueen", "PstMgKing",
        "PstEgPawn", "PstEgKnight", "PstEgBishop", "PstEgRook", "PstEgQueen", "PstEgKing",
        // The imbalance.
        "ImbLinear", "ImbOur", "ImbTheir",
        // The pawn cache, and the passer terms that read occupancy alone.
        "PassedMg", "PassedEg", "DoubledMg", "DoubledEg", "IsolatedMg", "IsolatedEg",
        "ConnectedMg", "ConnectedEg", "BackwardMg", "BackwardEg",
        "ConnectedRankMg", "ConnectedRankEg", "WeakUnopposedMg", "WeakUnopposedEg",
        "BlockedPawnMg", "BlockedPawnEg", "PawnMajorityMg", "PawnMajorityEg",
        "PawnIslandsMg", "PawnIslandsEg", "PassFreeMg", "PassFreeEg", "PassSafeEg",
        // The bishop pair, the rook and knight bonuses.
        "BpMg", "BpEg", "RookOpenMg", "RookOpenEg", "RookSemiMg", "RookSemiEg",
        "Rook7thMg", "Rook7thEg", "KnightOutpostMg", "KnightOutpostEg",
    };
    for (const char* name : cheap)
        if (group == name)
            return true;
    return false;
}

static void test_lazy_checkpoint() {
    // Positions the lazy path certainly serves: one side keeps its army and
    // the other only its king and pawns, at least 1,500 centipawns of pieces
    // apart and never more than two pawns behind, so the cheap block's taper
    // clears the 700 margin whatever the positional terms say.
    std::vector<Board> positions;
    for (const std::string& fen : purity_sample()) {
        Board b;
        b.set_fen(fen);
        const Squares full = squares_of(b);
        for (Color strong : {WHITE, BLACK}) {
            const Color weak = ~strong;
            auto pieces = [&](Color c) {
                return popcount(b.piece_bb(c, KNIGHT)) * 320 + popcount(b.piece_bb(c, BISHOP)) * 330
                     + popcount(b.piece_bb(c, ROOK)) * 500 + popcount(b.piece_bb(c, QUEEN)) * 950;
            };
            if (pieces(strong) < 1500
                || popcount(b.piece_bb(strong, PAWN)) + 2 < popcount(b.piece_bb(weak, PAWN)))
                continue;
            Squares crushed = full;
            for (int sq = 0; sq < SQUARE_NB; ++sq)
                if (full[sq] != NO_PIECE && color_of(full[sq]) == weak
                    && type_of(full[sq]) != PAWN && type_of(full[sq]) != KING)
                    crushed[sq] = NO_PIECE;
            Board v;
            v.set_fen(fen_of(crushed, b.turn()));
            positions.push_back(v);
        }
    }

    auto ev = std::make_unique<Evaluator>();
    std::vector<int> base;
    for (const Board& b : positions)
        base.push_back(ev->evaluate(b));

    struct Group { std::string name; std::span<int> values; };
    std::vector<Group> groups;
#define X(name, member, len) groups.push_back({#name, eval_param_span(g_eval_params.member, (len))});
    EVAL_PARAM_LIST(X)
#undef X

    // Each group is perturbed by an amount that differs by slot, so no group
    // cancels against itself (both kings on their own squares, say).
    int cheap_groups = 0, full_groups = 0;
    std::string leaked, missing;
    for (const Group& g : groups) {
        const std::vector<int> saved(g.values.begin(), g.values.end());
        for (size_t i = 0; i < g.values.size(); ++i)
            g.values[i] += 40 + 3 * static_cast<int>(i);
        init_eval_tables();
        ev->clear_pawn_table();
        int changed = 0;
        for (size_t i = 0; i < positions.size(); ++i)
            changed += ev->evaluate(positions[i]) != base[i];
        std::copy(saved.begin(), saved.end(), g.values.begin());
        init_eval_tables();
        ev->clear_pawn_table();

        if (before_lazy_checkpoint(g.name)) {
            ++cheap_groups;
            if (changed == 0)
                missing += " " + g.name;
        } else {
            ++full_groups;
            if (changed != 0)
                leaked += " " + g.name + "(" + std::to_string(changed) + ")";
        }
    }

    begin_section("lazy checkpoint: every cheap term moves a lazy-served score");
    if (!missing.empty())
        std::printf("    cheap groups that moved nothing:%s\n", missing.c_str());
    EXPECT(missing.empty());
    EXPECT_EQ(cheap_groups, 50);
    EXPECT(positions.size() > 300);
    end_section();

    begin_section("lazy checkpoint: no term after it moves a lazy-served score");
    if (!leaked.empty())
        std::printf("    groups after the checkpoint that moved one:%s\n", leaked.c_str());
    EXPECT(leaked.empty());
    EXPECT(full_groups > 60);
    end_section();
}

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
    test_pawnless_scaler_tie();
    test_symmetry_sweep();
    test_bare_king_material_is_monotone();
    test_top_tiers_read_no_fitted_weight();

    std::printf("\nMaterial advantage\n");
    test_material_advantage();

    std::printf("\nLone kings\n");
    test_lone_kings();

    std::printf("\nOCB draw scaling\n");
    test_ocb_scaling();

    std::printf("\nRook-ending draw scaling (6.5.a)\n");
    test_rook_ending_scaling();

    std::printf("\nRook/passer decoupling\n");
    test_rook_passer_decoupling();

    std::printf("\nattacked2 two-pawn double attacks\n");
    test_attacked2_pawn_pairs();

    std::printf("\nMate-drive gating\n");
    test_mate_drive_gating();

    std::printf("\nKNNK draw detection\n");
    test_knnk_draw();

    std::printf("\nTempo bonus\n");
    test_tempo_bonus();

    std::printf("\nPST orientation\n");
    test_pst_orientation();

    std::printf("\nImbalance table\n");
    test_imbalance_cache();

    std::printf("\nCache purity\n");
    test_cache_purity();

    std::printf("\nLazy checkpoint\n");
    test_lazy_checkpoint();

#ifdef BASILISK_TUNE
    std::printf("\nEndgame occurrence classifier\n");
    test_endgame_occurrence_classifier();
#endif

    return harness_summary();
}
